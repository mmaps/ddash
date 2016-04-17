#include "veins/modules/application/ddash/DDash11p.h"

using Veins::TraCIMobilityAccess;
using Veins::AnnotationManagerAccess;

//const simsignalwrap_t DDash11p::parkingStateChangedSignal = simsignalwrap_t(TRACI_SIGNAL_PARKING_CHANGE_NAME);

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
        pingMsg = new cMessage((mobility->getExternalId()).c_str(), PING);
        //simulate asynchronous channel access
        double maxOffset = par("maxOffset").doubleValue();
        double offSet = dblrand() * (par("beaconInterval").doubleValue()/2);
        offSet = offSet + floor(offSet/0.050)*0.050;
        individualOffset = dblrand() * maxOffset;
        scheduleAt(simTime() + offSet, pingMsg);
        //mapIter = nodeMap.begin();
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
	/*
	annotations->scheduleErase(1, annotations->drawLine(wsm->getSenderPos(), mobility->getPositionAt(simTime()), "blue"));

	if (mobility->getRoadId()[0] != ':') traciVehicle->changeRoute(wsm->getWsmData(), 9999);
	if (!sentMessage) sendMessage(wsm->getWsmData());
	*/
}

void DDash11p::sendMessage(std::string blockedRoadId) {
	sentMessage = true;

	t_channel channel = dataOnSch ? type_SCH : type_CCH;
	WaveShortMessage* wsm = prepareWSM("data", dataLengthBits, channel, dataPriority, -1,2);
	wsm->setWsmData(blockedRoadId.c_str());
	sendWSM(wsm);
}

void DDash11p::receiveSignal(cComponent* source, simsignal_t signalID, cObject* obj) {
	Enter_Method_Silent();
	if (signalID == mobilityStateChangedSignal) {
		handlePositionUpdate(obj);
	}
}

void DDash11p::handlePositionUpdate(cObject* obj) {
	BaseWaveApplLayer::handlePositionUpdate(obj);

	// stopped for for at least 10s?
	if (mobility->getSpeed() < 1) {
		if (simTime() - lastDroveAt >= 60) {
			findHost()->getDisplayString().updateWith("r=16,red");
			if (!sentMessage) sendMessage(mobility->getRoadId());
		}
	}
	else {
		lastDroveAt = simTime();
	}
}
void DDash11p::sendWSM(WaveShortMessage* wsm) {
	sendDelayedDown(wsm,individualOffset);
}

void DDash11p::handleSelfMsg(cMessage* msg) {
    switch (msg->getKind()) {
        case PING: {
            if(nodeMap.empty()) {
                sendJoin();
            } else {
                sendPing();
            }
            scheduleAt(simTime() + par("beaconInterval").doubleValue(), pingMsg);
            break;
        }
        default: {
            if (msg)
                DBG << "APP: Error: Got Self Message of unknown kind! Name: " << msg->getName() << endl;
            break;
        }
    }
}

void DDash11p::sendJoin(){
    WaveShortMessage *wsm;
    wsm = prepareWSM(mobility->getExternalId(), beaconLengthBits, type_CCH, beaconPriority, 0, -1);
    wsm->setKind(JOIN);
    wsm->setWsmData(mobility->getRoadId().c_str());
    sendWSM(wsm);
    EV << "Sent JOIN!" << endl;
}

void DDash11p::sendPing(){
    WaveShortMessage *wsm;
    const char* node;
    node = getNextNode();
    wsm = prepareWSM(node, beaconLengthBits, type_CCH, beaconPriority, 0, -1);
    wsm->setKind(PING);
    //wsm->setNodeMap(nodeMap);
    wsm->setNodeList(nodeList);
    sendWSM(wsm);
    std::cout << "Sent PING to " << node << endl;
}

void DDash11p::onJoin(WaveShortMessage* wsm){
    if(std::string(wsm->getWsmData()) == mobility->getRoadId()) {
        std::cout << "A buddy on my road" << endl;
        addNode(wsm->getName());
    } else {
        std::cout << "Not my road: " << wsm->getWsmData() << " vs " << mobility->getRoadId().c_str()<< endl;
    }
}

void DDash11p::onPing(WaveShortMessage* wsm){
    if(wsm->getName() == mobility->getExternalId()) {
        std::cout << "Got a ping!" << endl;
        NodeList list = wsm->getNodeList();
        for(std::string name: list) {
            std::cout << "Importing: " << name << endl;
            if(nodeMap.find(name.c_str()) != nodeMap.end()) {
                nodeMap[name.c_str()] = ALIVE;
                nodeList.push_back(name);
            }
        }
    } else {
        std::cout << "Ping not for me " << wsm->getName() << endl;
    }
}

void DDash11p::onPingReq(WaveShortMessage* wsm){}
void DDash11p::onAck(WaveShortMessage* wsm){}

void DDash11p::addNode(const char* name) {
    bool wasEmpty = nodeMap.empty();
    std::cout << "Adding: " << name << endl;

    nodeList.push_back(std::string(name));

    nodeMap[std::string(name).c_str()] = ALIVE;

    if(wasEmpty) {
        lastIdx = 0;
        std::cout << "Init map iter " << endl;
        mapIter = nodeMap.begin();
        std::cout << "Init with: " << mapIter->first << endl;
    }
}

const char* DDash11p::getNextNode() {
    const char* name = nodeList[lastIdx].c_str();
    lastIdx++;

    if(lastIdx == nodeMap.size()) {
        lastIdx = 0;
    }

    return name;
}
