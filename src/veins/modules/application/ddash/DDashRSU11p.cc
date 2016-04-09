#include "veins/modules/application/ddash/DDashRSU11p.h"

using Veins::AnnotationManagerAccess;

Define_Module(DDashRSU11p);

void DDashRSU11p::initialize(int stage) {
	BaseWaveApplLayer::initialize(stage);
	if (stage == 0) {
		mobi = dynamic_cast<BaseMobility*> (getParentModule()->getSubmodule("mobility"));
		ASSERT(mobi);
		annotations = AnnotationManagerAccess().getIfExists();
		ASSERT(annotations);
		sentMessage = false;
	}

    cMessage *msg = new cMessage("TEST");
    int n = this->getParentModule()->gateSize("nicGate");
    std::cout<<"N: "<<n<<endl;

    for(int i=0; i<n; i++){
       cMessage *copy = msg->dup();
       cGate *gate = this->getParentModule()->getSubmodule("geoNic")->gateHalf("nicGate", cGate::OUTPUT, i);
       send(copy, gate);
    }

	std::vector<const char*> names = this->getParentModule()->getGateNames();
	for(std::vector<const char*>::iterator it=names.begin(); it!=names.end(); ++it) {
	    std::cout<<*it<<endl;
	}

}

void DDashRSU11p::onBeacon(WaveShortMessage* wsm) {

}

void DDashRSU11p::onData(WaveShortMessage* wsm) {
	findHost()->getDisplayString().updateWith("r=16,green");

	annotations->scheduleErase(1, annotations->drawLine(wsm->getSenderPos(), mobi->getCurrentPosition(), "blue"));

	if (!sentMessage) sendMessage(wsm->getWsmData());
}

void DDashRSU11p::sendMessage(std::string blockedRoadId) {
	sentMessage = true;
	t_channel channel = dataOnSch ? type_SCH : type_CCH;
	WaveShortMessage* wsm = prepareWSM("data", dataLengthBits, channel, dataPriority, -1,2);
	wsm->setWsmData(blockedRoadId.c_str());
	sendWSM(wsm);
}
void DDashRSU11p::sendWSM(WaveShortMessage* wsm) {
	sendDelayedDown(wsm,individualOffset);
}
