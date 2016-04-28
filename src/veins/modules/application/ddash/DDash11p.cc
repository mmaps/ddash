#include "veins/modules/application/ddash/DDash11p.h"

using Veins::TraCIMobilityAccess;
using Veins::AnnotationManagerAccess;

Define_Module(DDash11p);

void DDash11p::initialize(int stage) {
	BaseWaveApplLayer::initialize(stage);
    if (stage == 0) {
        mobility = TraCIMobilityAccess().get(getParentModule());
        traci = mobility->getCommandInterface();
        traciVehicle = mobility->getVehicleCommandInterface();
        annotations = AnnotationManagerAccess().getIfExists();
        ASSERT(annotations);
        sentMessage = false;
        lastDroveAt = simTime();

        //simulate asynchronous channel access
        double maxOffset = par("maxOffset").doubleValue();
        double offSet = dblrand() * (par("protocolPeriod").doubleValue()/2);
        offSet = offSet + floor(offSet/0.050)*0.050;
        individualOffset = dblrand() * maxOffset;

        //Schedule the first heartbeat messages
        heartbeatMsg = new cMessage((mobility->getExternalId()).c_str(), HEARTBEAT);
        scheduleAt(simTime() + offSet, heartbeatMsg);

        //Timeout values
        timeout = par("timeoutPeriod").doubleValue();
        period = par("protocolPeriod").doubleValue();

        //Accident values
        accidentInterval = par("accidentInterval");
        accidentDuration = par("accidentDuration");
        accidentCount = par("accidentCount");
        if (accidentCount > 0) {
            //debug("I'm gonna crash!");
            simtime_t accidentStart = par("accidentStart");
            startAccidentMsg = new cMessage("scheduledAccident");
            startAccidentMsg->setKind(START_ACC);
            stopAccidentMsg = new cMessage("scheduledAccidentResolved");
            stopAccidentMsg->setKind(STOP_ACC);
            scheduleAt(simTime() + accidentStart, startAccidentMsg);
        }

        // Visual groups?
        drawGroups = par("drawGroups");

        // Leader Election
        leadName = getMyName();
        leadUid = carUid;

        // Debug
        debugging = par("debugging");
    }
}


/**********************************************************************************
 *
 * SEND METHODS
 *
 **********************************************************************************/
void DDash11p::sendJoin(){
    WaveShortMessage *wsm;
    NodeMsgs joinList;
    joinList.push_front(getMyName());

    wsm = prepareWSM("Join", beaconLengthBits, type_CCH, beaconPriority, 0, -1);
    wsm->setKind(PING);
    wsm->setJoinMsgs(joinList);
    wsm->setSrc(getMyName().c_str());
    wsm->setDst("*");
    wsm->setGroup(getGroupName().c_str());

    sendWSM(wsm);
    if(1)
    {
        debug("JOIN(" + getMyName() +") to " + getGroupName());
        sentJoinDbgMsg = true;
    }
}


void DDash11p::sendCritical(std::string roadId) {
    WaveShortMessage *wsm;
    critMsgs[roadId]++;

    wsm = prepareWSM("ACCIDENT", beaconLengthBits, type_CCH, beaconPriority, 0, -1);
    wsm->setKind(PING);
    wsm->setSrc(getMyName().c_str());
    setCriticalMsgs(wsm);

    // Broadcast
    wsm->setDst("*");
    wsm->setGroup("*");

    sendWSM(wsm);
}


void DDash11p::sendPing(const char* node){
    WaveShortMessage *wsm;

    wsm = prepareWSM("Ping", beaconLengthBits, type_CCH, beaconPriority, 0, -1);
    wsm->setKind(PING);
    setUpdateMsgs(wsm);

    wsm->setSrc(getMyName().c_str());
    wsm->setDst(node);
    wsm->setGroup(getGroupName().c_str());
    sendWSM(wsm);

    nodeMap[std::string(node)] = PINGWAIT;

    debug("PING " + std::string(node));

    if(isLeader() && !critMsgs.empty()) {
        debug("I'm the leader sending ACCIDENT");
        for(NodeMap::iterator it=critMsgs.begin(); it!=critMsgs.end(); it++) {
            sendCritical(it->first);
        }
    }
}


int DDash11p::sendPingReq(std::string nodeName){
    int kNodesMax = par("pingReqNum");
    int kNodes = 0;
    size_t nodeIdx;
    int numNodes = nodeMap.size();
    std::string middleNode;
    std::ostringstream os;

    if(numNodes - kNodesMax < 1) {
        os << "nodeMap.size: " << nodeMap.size() << " K: " << kNodesMax;
        debug("Not enough nodes for K " + os.str());
        return 0;
    }

    debug("Sending K nodes PINGREQ");
    for(kNodes=0; kNodes<kNodesMax; kNodes++) {
        nodeIdx = rand() % numNodes;
        if(nodeIdx >= nodeList.size()) {
            continue;
        }

        middleNode = nodeList[nodeIdx];
        if(!middleNode.compare(nodeName)) {
            continue;
        }

        int ns = nodeMap[middleNode];

        dumpNode(middleNode);

        if(ns != ALIVE) {
            continue;
        }


        WaveShortMessage *wsm = prepareWSM("PingReq", beaconLengthBits, type_CCH, beaconPriority, 0, -1);
        wsm->setKind(PINGREQ);
        setUpdateMsgs(wsm);

        wsm->setSrc(getMyName().c_str());
        wsm->setDst(middleNode.c_str());
        wsm->setGroup(getGroupName().c_str());
        wsm->setWsmData(nodeName.c_str());

        sendWSM(wsm);

        nodeMap[middleNode] = PINGREQWAIT;

        setTimer(getMyName().c_str(), middleNode.c_str(), nodeName.c_str());

        debug("Sent PINGREQ to " + middleNode + " for " + nodeName);
    }

    return 1;
}


void DDash11p::sendAck(std::string dst) {
    WaveShortMessage* wsm = prepareWSM("ACK", beaconLengthBits, type_CCH, beaconPriority, 0, -1);
    wsm->setKind(ACK);
    wsm->setSrc(getMyName().c_str());
    wsm->setDst(dst.c_str());
    wsm->setWsmData("");
    wsm->setGroup(getGroupName().c_str());
    sendWSM(wsm);
    debug("send ack to " + dst);
}


void DDash11p::sendAck(std::string src, std::string dst, std::string data) {
    WaveShortMessage* wsm = prepareWSM("PINGREQ_ACK", beaconLengthBits, type_CCH, beaconPriority, 0, -1);
    wsm->setKind(ACK);
    wsm->setSrc(src.c_str());
    wsm->setDst(dst.c_str());
    wsm->setWsmData(data.c_str());
    wsm->setGroup(getGroupName().c_str());
    sendWSM(wsm);
    debug("send ack to ping request to " + dst + " from " + data);
}


void DDash11p::sendFail(std::string nodeName) {
    debug("Preparing to send failing message: " + nodeName);
    WaveShortMessage* wsm = prepareWSM("Failure Detector msg", beaconLengthBits, type_CCH, beaconPriority, 0, -1);
    wsm->setKind(PING);
    wsm->setDst("*");
    wsm->setWsmData(nodeName.c_str());

    // delete from its own node
    leaveMsgs[nodeName]++;
    if(leaveMsgs[nodeName] >= leaveMax) {
        leaveMaxes.push_back(nodeName);
        leaveMax = leaveMsgs[nodeName];
    }
    removeFromList(nodeName);
    nodeMap.erase(nodeName);

    if(leaveMsgs.size() > LRU_SIZE) {
        leaveMsgs.erase(leaveMaxes.back());
        leaveMaxes.pop_back();
    }

    // update joining and leaving messages, and send with msg
    setUpdateMsgs(wsm);

    sendWSM(wsm);
    debug("Fail " + nodeName);
}

void DDash11p::sendMessage(std::string blockedRoadId) {
    sentMessage = true;
    t_channel channel = dataOnSch ? type_SCH : type_CCH;
    WaveShortMessage* wsm = prepareWSM("data", dataLengthBits, channel, dataPriority, -1,2);
    wsm->setWsmData(blockedRoadId.c_str());
    sendWSM(wsm);
}

void DDash11p::sendWSM(WaveShortMessage* wsm) {
    wsm->setSenderPath(this->getParentModule()->getFullPath().c_str());
    sendDelayedDown(wsm,individualOffset);
}

/**********************************************************************************
 *
 * RECEIVE METHODS
 *
 **********************************************************************************/

void DDash11p::onPing(WaveShortMessage* wsm){
    if(isMyGroup(wsm) && isForMe(wsm)) {
        std::string sender = std::string(wsm->getSrc());
        //debug("PING from " + sender);
        if(!isBroadcast(wsm)) {
            saveNodeInfo(wsm);
            sendAck(sender);
        }
        getUpdateMsgs(wsm);
    }
}

void DDash11p::onPingReq(WaveShortMessage* wsm) {
    if(isMyGroup(wsm) && isForMe(wsm)) {
        //debug("PING REQ received from " + std::string(wsm->getSrc()) + " for " + std::string(wsm->getWsmData()));
        saveNodeInfo(wsm);
        getUpdateMsgs(wsm);
        setTimer(wsm->getSrc(), wsm->getWsmData(), "");
        sendPing(wsm->getWsmData());
        pingReqSent[wsm->getWsmData()] = wsm->getSrc();
    }
}

void DDash11p::onAck(WaveShortMessage* wsm){
    std::string src = std::string(wsm->getSrc());
    std::string data = std::string(wsm->getWsmData());

    if(isMyGroup(wsm) && isForMe(wsm)) {
        debug("ACK from " + src + ", data: " + data);

        getUpdateMsgs(wsm);

        if(!hasNode(src)) {
            addNode(wsm->getSenderPath(), src);
        } else {
            nodeMap[src] = ALIVE;
        }

        if(data != "") {
            nodeMap[data] = ALIVE;
        }

        if(isPingReqAck(src)) {
            sendAck(getMyName(), pingReqSent[src], src);
            pingReqSent.erase(src);
        }
    }
}


void DDash11p::onBeacon(WaveShortMessage* wsm) {
    if(flashOn) {
        findHost()->getDisplayString().updateWith("r=16,red");
        flashOn = false;
    } else {
        findHost()->getDisplayString().updateWith("r=16,green");
        flashOn = true;
    }
}

void DDash11p::onData(WaveShortMessage* wsm) {
    findHost()->getDisplayString().updateWith("r=16,green");
}

void DDash11p::receiveSignal(cComponent* source, simsignal_t signalID, cObject* obj) {
    Enter_Method_Silent();
    if (signalID == mobilityStateChangedSignal) {
        handlePositionUpdate(obj);
    }
}


/**********************************************************************************
 *
 * HANDLER METHODS
 *
 **********************************************************************************/

void DDash11p::handleSelfMsg(cMessage* msg) {
    std::string dst;
    WaveShortMessage *wsm;
    switch (msg->getKind()) {
        case HEARTBEAT:
            if(nodeMap.empty()) {
                sendJoin();
            } else {
                const char* nodeName = getNextNode();
                if(nodeName == nullptr) {
                    if(!sentNoOneDbgMsg) {
                        debug("No one to ping");
                        sentNoOneDbgMsg = true;
                    }
                    scheduleAt(simTime() + par("beaconInterval").doubleValue(), heartbeatMsg);
                    return;
                }

                setTimer(getMyName().c_str(), nodeName, "");
                sendPing(nodeName);
                sentNoOneDbgMsg = false;
            }

            scheduleAt(simTime() + par("beaconInterval").doubleValue(), heartbeatMsg);
            break;

        case TIMEOUT:
            wsm = check_and_cast<WaveShortMessage*>(msg);
            dst = std::string(wsm->getDst());

            if(nodeMap[dst] == PINGWAIT) {
                // timeout msg from sender to middle node, asking ping-req help
                debug("Timeout on PINGWAIT for " + dst);
                if(hasNode(dst)) {
                    // Must check, otherwise we are setting a blank node back!
                    if(sendPingReq(dst)) {
                        setTimer(getMyName().c_str(), wsm->getDst(), "");
                        nodeMap[dst] = PINGWAIT2;
                    }
                }
            } else if(nodeMap[dst] == PINGWAIT2) {
                // timeout msg from middle node to suspicious node
                debug("Timeout on PINGWAIT2. Fail(" + dst + ")");
                failNode(dst);

            } else if(nodeMap[dst] == PINGREQWAIT) {
                debug("Timeout on PINGREQ. Resetting " + dst);
                if(hasNode(dst)) {
                    nodeMap[dst] = ALIVE;
                }
            }
            break;

        case START_ACC:
            handleAccidentStart();
            scheduleAt(simTime() + accidentDuration, stopAccidentMsg);
            accidentCount--;
            break;

        case STOP_ACC:
            handleAccidentStop();
            if(accidentCount > 0) {
                scheduleAt(simTime() + accidentInterval, startAccidentMsg);
            }
            break;

        default:
            if (msg)
                DBG << "APP: Error: Got Self Message of unknown kind! Name: " << msg->getName() << endl;
            break;
    }
    // Setup Leader
    checkLeader();
}


void DDash11p::handlePositionUpdate(cObject* obj) {
    BaseWaveApplLayer::handlePositionUpdate(obj);
    cGate* cg;
    bool connected = false;

    if(isLeader()) {
        getDisplayString().setTagArg("t", 0, "LEADER");
    }

    if(roadChanged()) {
        debug("Leaving group " + groupName + ". Join group " + mobility->getRoadId());

        groupName = mobility->getRoadId();
        leaveMsgs.clear();
        joinMsgs.clear();
        groupConns.clear();
        nodeMap.clear();
        assert(nodeMap.empty());
        nodeList.clear();
        lastIdx = 0;
        leadName = std::string("");

        for(int i=0; i<this->getParentModule()->gateSize("gIn"); i++) {
            if(this->getParentModule()->gate("gIn", i)->isConnectedOutside()) {
                connected = true;
            }
        }

        for(int i=0; i<this->getParentModule()->gateSize("gOut"); i++) {
            cg = this->getParentModule()->gate("gOut", i);
            if(cg) {
                cg->disconnect();
            }
        }
        this->getParentModule()->setGateSize("gOut", 0);

        if(!connected) {
            this->getParentModule()->setGateSize("gIn", 0);
        }
    }
}


void DDash11p::handleAccidentStart() {
    //debug("*** Start Accident ***");

    setDisplay("r=16,red");
    traciVehicle->setSpeed(0);

    sendCritical(getGroupName());
}


void DDash11p::handleAccidentStop() {
    //debug("*** End Accident ***");
    setDisplay("r=0,-");
    traciVehicle->setSpeed(-1);
}


/**********************************************************************************
 *
 * STATE METHODS
 *
 **********************************************************************************/
void DDash11p::saveNodeInfo(WaveShortMessage *wsm) {
    const char* sender = wsm->getSrc();
    if(!hasNode(sender)) {
        //debug("Sender is new: " + std::string(sender));
        addNode(wsm->getSenderPath(), std::string(sender));
    }
}


void DDash11p::failNode(std::string name) {
    debug("Fail(" + name + ")");
    leaveMsgs[name]++;
    removeFromList(name);
    nodeMap.erase(name);
    disconnectGroupMember(name);
}


void DDash11p::addNode(const char* path, std::string name) {
    if(hasNode(nodeMap, name)) {
        return;
    }

    //debug("addNode " + std::string(name));

    nodeList.push_back(std::string(name));

    nodeMap[std::string(name)] = ALIVE;

    if(drawGroups) {
        connectGroupMember(path, name);
    }

    if(nodeMap.size() == 1) {
        mapIter = nodeMap.begin();
        lastIdx = 0;
    }
}


const char* DDash11p::getNextNode() {
    std::string next;
    size_t count = 0;

    do {
        //debug("Getting next PING target");
        //std::cout << "lastidx: " << lastIdx << endl;
        //std::cout << "nodelist: " << nodeList.size() << endl;

        if(count == nodeMap.size() || nodeList.size() == lastIdx) {
            next = "";
            if(lastIdx == nodeMap.size()) {
                lastIdx = 0;
            }
            return nullptr;
        }

        next = nodeList[lastIdx];
        lastIdx++;
        if(lastIdx == nodeMap.size()) {
            lastIdx = 0;
        }

        count++;
        //dumpNode(next);
    } while(nodeMap[next] != ALIVE);


    return next.c_str();
}

// schedule TIMEOUT message
void DDash11p::setTimer(const char* src, const char* dst, const char* data) {
    WaveShortMessage* msg = new WaveShortMessage("", TIMEOUT);
    msg->setSrc(src);
    msg->setDst(dst);
    msg->setWsmData(data);
    pingReqTimers[std::string(src)][std::string(dst)] = msg;
    scheduleAt(simTime() + timeout, msg);
}


void DDash11p::removeFromList(std::string name) {
    debug("Remove from list: " + name);

    //dumpList();

    for (size_t i = 0; i < nodeList.size(); i++) {
        if (!name.compare(nodeList[i])) {
            nodeList.erase(nodeList.begin() + i);
            break;
        }
    }

    if(lastIdx >= nodeList.size()) {
        lastIdx = 0;
    }

    //dumpList();
}

// add join and leave membership list to message
void DDash11p::setUpdateMsgs(WaveShortMessage* wsm) {
    NodeMsgs msgs;

    for(NodeMap::iterator it=joinMsgs.begin(); it!=joinMsgs.end(); it++) {
        msgs.push_front(it->first);
    }
    wsm->setJoinMsgs(msgs);

    msgs.clear();

    for(NodeMap::iterator it=leaveMsgs.begin(); it!=leaveMsgs.end(); it++) {
        msgs.push_front(it->first);
    }
    wsm->setLeaveMsgs(msgs);

    msgs.clear();

}

void DDash11p::setCriticalMsgs(WaveShortMessage* wsm) {
    NodeMsgs msgs;
    for(NodeMap::iterator it=critMsgs.begin(); it!=critMsgs.end(); it++) {
        msgs.push_front(it->first);
    }
    wsm->setCriticalMsgs(msgs);
}

// update join and leave membership list through receiving message
void DDash11p::getUpdateMsgs(WaveShortMessage* wsm) {
    NodeMsgs joins = wsm->getJoinMsgs();
    NodeMsgs leaves = wsm->getLeaveMsgs();
    NodeMsgs crits = wsm->getCriticalMsgs();

    for(std::string s: crits) {

        critMsgs[s]++;

        if(s.compare(getGroupName())) {
            if(critMsgs[s] == 1) {
                //debug("Accident(" + s + "). Re-routing...");
                traciVehicle->changeRoute(s, 9999);
                setDisplay("r=8,yellow");
            }
        } else {
           //debug("Accident(" + s + "). Unavoidable!");
        }


    }

    for(std::string s: joins) {
        if(!s.compare(getMyName())) {
            continue;
        }

        //debug("Update= join(" + s + ")");
        joinMsgs[s]++;
        if(joinMsgs[s] >= joinMax) {
            joinMaxes.push_back(s);
            joinMax = joinMsgs[s];
        }

        if(!isMyName(s) && !hasNode(s)) {
             //debug("New node " + s);
             addNode(wsm->getSenderPath(), s.c_str());
        }
    }

    for(std::string s: leaves) {
        if(!s.compare(getMyName())) {
            continue;
        }
        leaveMsgs[s]++;
        //debug("Update= leave(" + s + ")");
        if(leaveMsgs[s] >= leaveMax) {
            leaveMaxes.push_back(s);
            leaveMax = leaveMsgs[s];
        }
        removeFromList(s);
        nodeMap.erase(s);
        disconnectGroupMember(s);
    }

    if(leaveMsgs.size() > LRU_SIZE) {
        leaveMsgs.erase(leaveMaxes.back());
        leaveMaxes.pop_back();
    }

    if(joinMsgs.size() > LRU_SIZE) {
        joinMsgs.erase(joinMaxes.back());
        joinMaxes.pop_back();
    }


}


void DDash11p::connectGroupMember(const char* path, std::string name) {
    cModule *thatMod, *thisMod;
    cGate *thatGate, *thisGate;
    int thatSize=0, thisSize;
    cChannel *cChan;

    if(hasNode(groupConns, name)) {
        //std::cout << getMyName() << " duplicate: " << name << endl;
        return;
    }

    thatMod = simulation.getModuleByPath(path);
    thisMod = this->getParentModule();

    if(thatMod) {
        thatSize = findEmptyGate(thatMod, "gIn");
        thisSize = findEmptyGate(thisMod, "gOut");

        //thatMod->setGateSize("gIn", thatSize);
        //thisMod->setGateSize("gOut", thisSize);

        thatGate = thatMod->gate("gIn", thatSize-1);
        thisGate = thisMod->gate("gOut", thisSize-1);

        cChan = thisGate->connectTo(thatGate, cIdealChannel::create("test"));

        if(cChan) {
            cChan->getDisplayString().parse("ls=blue,3");
        }
    }

    //std::cout << getMyName() <<" Connecting gate to: "<< name << " "<<  thisSize-1 << " Total=" << thisSize << endl;

    groupConns[name] = thisGate;
}


void DDash11p::disconnectGroupMember(std::string name) {
    if(hasNode(groupConns, name)) {

        //std::cout<<  getMyName() <<" Disconnecting: "<< name;

        groupConns[name]->disconnect();
        //std::cout <<endl;
        groupConns.erase(name);
    } else {
        //std::cout << getMyName() << " not connected to " << name << endl;

        //for(std::map<std::string, cGate*>::iterator it=groupConns.begin(); it!=groupConns.end(); it++) {
            //std:: cout << it->first << ", ";
        //}
        //std::cout << endl;
    }
}

void DDash11p::checkLeader() {
    std::string old = leadName;
    std::string candidate;

    // Get the highest ID node
    if(!nodeMap.empty()) {
        candidate = nodeMap.rbegin()->first;

        if(getMyName().compare(candidate) < 0) {
            debug("Not leader");
            findHost()->getDisplayString().removeTag("i");
            findHost()->getDisplayString().removeTag("is");
            leadName = candidate;
        } else if(!isLeader()) {
            debug("Set leader string");
            setDisplay("i=status/checkmark;is=vs;");
            leadName = getMyName();
        }
    } else {
        if(!isLeader()) {
            debug("Set leader string, solo");
            setDisplay("i=status/checkmark;is=vs;");
            leadName = getMyName();
        }
    }

    if(old.compare(leadName)) {
        debug("New leader: " + leadName);
    }
}
