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
    }
}


/**********************************************************************************
 *
 * SEND METHODS
 *
 **********************************************************************************/
void DDash11p::sendWSM(WaveShortMessage* wsm) {
	sendDelayedDown(wsm,individualOffset);
}


void DDash11p::sendJoin(){
    WaveShortMessage *wsm;
    wsm = prepareWSM(mobility->getExternalId(), beaconLengthBits, type_CCH, beaconPriority, 0, -1);
    wsm->setKind(JOIN);
    wsm->setWsmData(mobility->getRoadId().c_str());
    sendWSM(wsm);
    if(!sentJoinDbgMsg) {
        debug("JOIN");
        sentJoinDbgMsg = true;
    }
}


void DDash11p::sendPing(const char* node){
    WaveShortMessage *wsm;

    wsm = prepareWSM("", beaconLengthBits, type_CCH, beaconPriority, 0, -1);
    wsm->setKind(PING);
    wsm->setNodeMap(nodeMap);
    wsm->setNodeList(nodeList);

    wsm->setSrc(getMyName().c_str());
    wsm->setDst(node);

    sendWSM(wsm);

    nodeMap[std::string(node)] = PINGWAIT;

    debug("PING " + std::string(node));
}


void DDash11p::sendPingReq(std::string nodeName){
    int kNodesMax = par("pingReqNum");
    int kNodes = 0;
    int nodeIdx;
    int numNodes = nodeMap.size();
    WaveShortMessage *wsm;
    std::string middleNode;

    if(numNodes - kNodesMax > 1) {

        while(kNodes < kNodesMax) {

            /* Select random node for middle man node */
            nodeIdx = rand() % numNodes;
            middleNode = nodeList[nodeIdx];

            if(middleNode != nodeName && nodeMap[middleNode] == ALIVE) {
                wsm = prepareWSM("", beaconLengthBits, type_CCH, beaconPriority, 0, -1);
                wsm->setKind(PINGREQ);
                wsm->setNodeMap(nodeMap);
                wsm->setNodeList(nodeList);

                wsm->setSrc(getMyName().c_str());
                wsm->setDst(middleNode.c_str());

                wsm->setWsmData(nodeName.c_str());

                sendWSM(wsm);

                nodeMap[nodeName] = PINGWAIT2;
                nodeMap[middleNode] = PINGREQWAIT;

                setTimer(getMyName().c_str(), middleNode.c_str(), nodeName.c_str());
                kNodes++;
            }

        }

    }
}


void DDash11p::sendAck(std::string dst) {
    WaveShortMessage* wsm = prepareWSM("ACK", beaconLengthBits, type_CCH, beaconPriority, 0, -1);
    wsm->setKind(ACK);
    wsm->setSrc(dst.c_str());
    wsm->setWsmData("");
    sendWSM(wsm);
    debug("send ack to" + dst);
}


void DDash11p::sendAck(std::string src, std::string dst, std::string data) {
    WaveShortMessage* wsm = prepareWSM("PINGREQ_ACK", beaconLengthBits, type_CCH, beaconPriority, 0, -1);
    wsm->setKind(ACK);
    wsm->setSrc(src.c_str());
    wsm->setDst(dst.c_str());
    wsm->setWsmData(data.c_str());
    sendWSM(wsm);
    debug("send ack to ping request to " + dst + " from " + data);
}


void DDash11p::forwardAck(WaveShortMessage* wsm) {}


void DDash11p::sendFail(std::string nodeName) {
    WaveShortMessage* wsm = prepareWSM("", beaconLengthBits, type_CCH, beaconPriority, 0, -1);
    wsm->setKind(FAIL);
    wsm->setDst("*");
    wsm->setWsmData(nodeName.c_str());
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


/**********************************************************************************
 *
 * RECEIVE METHODS
 *
 **********************************************************************************/
void DDash11p::onJoin(WaveShortMessage* wsm){
    if(isMyGroup(wsm)) {
        debug(wsm->getName() + std::string(" joins road ") + mobility->getRoadId());
        addNode(wsm->getName());
    }
}


void DDash11p::onPing(WaveShortMessage* wsm){
    if(isForMe(wsm)) {
        debug("PING received");
        std::string sender = std::string(wsm->getWsmData());
        saveNodeInfo(wsm);
        sendAck(sender);
    }
}


void DDash11p::onPingReq(WaveShortMessage* wsm){
    if(isForMe(wsm)) {
        debug("PING REQ received");
        saveNodeInfo(wsm);
        setTimer(wsm->getSrc(), wsm->getWsmData(), "");
        sendPing(wsm->getWsmData());
        pingReqSent[wsm->getWsmData()] = wsm->getSrc();
    }
}


void DDash11p::onAck(WaveShortMessage* wsm){
    const char* src = wsm->getSrc();
    std::string data = std::string(wsm->getWsmData());

    if(isForMe(wsm)) {
        debug("ACK received");

        nodeMap[src] = ALIVE;

        if(data != "") {
            nodeMap[data] = ALIVE;
        }

        if(isPingReqAck(std::string(src))) {
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
                setTimer(getMyName().c_str(), nodeName, "");
                sendPing(nodeName);
            }

            scheduleAt(simTime() + par("beaconInterval").doubleValue(), heartbeatMsg);
            break;

        case TIMEOUT:
            wsm = check_and_cast<WaveShortMessage*>(msg);
            dst = std::string(wsm->getDst());

            if(nodeMap[dst] == PINGWAIT) {
                setTimer(getMyName().c_str(), wsm->getDst(), "");
                sendPingReq(dst);
            } else if(nodeMap[dst] == PINGWAIT2) {
                sendFail(dst);
            } else if(nodeMap[dst] == PINGREQWAIT) {
                sendFail(wsm->getWsmData());
                nodeMap[dst] = ALIVE;
            }
            break;

        default:
            if (msg)
                DBG << "APP: Error: Got Self Message of unknown kind! Name: " << msg->getName() << endl;
            break;
    }
}


void DDash11p::handlePositionUpdate(cObject* obj) {
    BaseWaveApplLayer::handlePositionUpdate(obj);
}


/**********************************************************************************
 *
 * STATE METHODS
 *
 **********************************************************************************/
void DDash11p::saveNodeInfo(WaveShortMessage *wsm) {
    const char* sender = wsm->getWsmData();
    if(!hasNode(sender)) {
        debug("Sender is new: " + std::string(sender));
        addNode(sender);
    }

    for(std::string s: wsm->getNodeList()) {
        if(!isMyName(s) && !hasNode(s)) {
            debug("New node " + s);
            addNode(s.c_str());
        }
    }
}

void DDash11p::addNode(const char* name) {
    debug("addNode " + std::string(name));

    if(nodeMap.find(std::string(name)) == nodeMap.end()) {
        nodeList.push_back(std::string(name));
        debug("NodeList: " + nodeList.back());
    }

    nodeMap[std::string(name)] = ALIVE;

    if(nodeMap.size() == 1) {
        mapIter = nodeMap.begin();
        lastIdx = 0;
    }

    dumpMap();
    debug("NodeMap: " + std::string(nodeMap.find(std::string(name))->first));
}


const char* DDash11p::getNextNode() {
    const char* next = nodeList[lastIdx].c_str();
    lastIdx++;
    if(lastIdx == nodeMap.size()) {
        lastIdx = 0;
    }
    return next;
}


void DDash11p::setTimer(const char* src, const char* dst, const char* data) {
    WaveShortMessage* msg = new WaveShortMessage("", TIMEOUT);
    msg->setSrc(src);
    msg->setDst(dst);
    msg->setWsmData(data);
    pingReqTimers[std::string(src)][std::string(dst)] = msg;
    scheduleAt(simTime() + timeout, msg);
}
