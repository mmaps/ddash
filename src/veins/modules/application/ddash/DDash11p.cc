#include "veins/modules/application/ddash/DDash11p.h"

using Veins::TraCIMobilityAccess;
using Veins::AnnotationManagerAccess;

Define_Module(DDash11p);

void DDash11p::initialize(int stage) {
	BaseWaveApplLayer::initialize(stage);
    if (stage == 0) {
        EV << "DDash Stage 0" << endl;
        mobility = TraCIMobilityAccess().get(getParentModule());
        traci = mobility->getCommandInterface();
        traciVehicle = mobility->getVehicleCommandInterface();
        annotations = AnnotationManagerAccess().getIfExists();
        ASSERT(annotations);
        sentMessage = false;
        lastDroveAt = simTime();
        heartbeatMsg = new cMessage((mobility->getExternalId()).c_str(), PING);
        //simulate asynchronous channel access
        double maxOffset = par("maxOffset").doubleValue();
        double offSet = dblrand() * (par("beaconInterval").doubleValue()/2);
        offSet = offSet + floor(offSet/0.050)*0.050;
        individualOffset = dblrand() * maxOffset;
        scheduleAt(simTime() + offSet, heartbeatMsg);
        //mapIter = nodeMap.begin();
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


void DDash11p::sendPing(){
    WaveShortMessage *wsm;
    const char* node;
    node = getNextNode();
    wsm = prepareWSM(node, beaconLengthBits, type_CCH, beaconPriority, 0, -1);
    wsm->setKind(PING);
    wsm->setNodeMap(nodeMap);
    wsm->setNodeList(nodeList);
    wsm->setWsmData(getMyName().c_str());
    sendWSM(wsm);
    debug("PING " + std::string(node));
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
    if(std::string(wsm->getWsmData()) == mobility->getRoadId()) {
        debug(wsm->getName() + std::string(" joins road ") + mobility->getRoadId());
        addNode(wsm->getName());
    } else {
        debug("Not my road");
    }
}


void DDash11p::onPing(WaveShortMessage* wsm){
    if(wsm->getName() == mobility->getExternalId()) {
        debug("PINGED");

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

    } else {
        debug(std::string("Ping not for me ") + wsm->getName());
    }
}


void DDash11p::onPingReq(WaveShortMessage* wsm){}


void DDash11p::onAck(WaveShortMessage* wsm){}


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
    switch (msg->getKind()) {
        case PING: {
            if(nodeMap.empty()) {
                sendJoin();
            } else {
                sendPing();
            }
            scheduleAt(simTime() + par("beaconInterval").doubleValue(), heartbeatMsg);
            break;
        }
        default: {
            if (msg)
                DBG << "APP: Error: Got Self Message of unknown kind! Name: " << msg->getName() << endl;
            break;
        }
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
