#include "veins/modules/application/ddash/DDashRSU11p.h"
#include "veins/base/messages/InterGeo_m.h"

using Veins::AnnotationManagerAccess;

Define_Module(DDashRSU11p);

static bool called = false;

void DDashRSU11p::initialize(int stage) {
	BaseWaveApplLayer::initialize(stage);
	if (stage == 0) {
		mobi = dynamic_cast<BaseMobility*> (getParentModule()->getSubmodule("mobility"));
		ASSERT(mobi);
		annotations = AnnotationManagerAccess().getIfExists();
		ASSERT(annotations);
		sentMessage = false;
	    //simulate asynchronous channel access
	    double maxOffset = par("maxOffset").doubleValue();
	            double offSet = dblrand() * (par("beaconInterval").doubleValue()/2);
	            offSet = offSet + floor(offSet/0.050)*0.050;
	            individualOffset = dblrand() * maxOffset;
	    heartbeat = new cMessage("heartbeat", SEND_BEACON_EVT);
	    scheduleAt(simTime() + offSet, heartbeat);
	}

    cMessage *msg = new cMessage("TEST");
    int n = this->getParentModule()->gateSize("nicGate");
/*
    for(int i=0; i<n; i++){
       cMessage *copy = msg->dup();
       cGate *gate = this->getParentModule()->getSubmodule("geoNic")->gateHalf("nicGate", cGate::OUTPUT, i);
       //send(copy, gate);
    }

	std::vector<const char*> names = this->getParentModule()->getGateNames();
	for(std::vector<const char*>::iterator it=names.begin(); it!=names.end(); ++it) {
	    std::cout<<*it<<endl;
	}
    // Module 'getIndex()' sends the first message
    if (getIndex() == 0 && strcmp(getName(), "street") == 0)
    {
        // Boot the process scheduling the initial message as a self-message.
        EV << "get name: " << getName() << "\n";
        TicTocMsg1 *msg = generateMessage();
        scheduleAt(0.0, msg);
    }
    */
    if(this->getParentModule()->getIndex() == 1 && !called) {
        called = true;
        InterGeo *copy = new InterGeo("INIT");
        cGate *gate = this->getParentModule()->getSubmodule("geoNic")->gateHalf("nicGate", cGate::OUTPUT, 0);
        send(copy, gate);
        std::cout << "Sending" << endl;

    }


}

void DDashRSU11p::onBeacon(WaveShortMessage* wsm) {
    if(flashOn) {
        findHost()->getDisplayString().updateWith("r=16,green");

    } else {
        findHost()->getDisplayString().updateWith("r=16,red");
    }
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
