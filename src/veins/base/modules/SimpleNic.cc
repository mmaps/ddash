//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include <omnetpp.h>
#include <iostream>
#include "veins/modules/application/ieee80211p/BaseWaveApplLayer.h"
#include "veins/modules/world/annotations/AnnotationManager.h"
#include "veins/base/messages/InterGeo_m.h"

using Veins::AnnotationManager;

class SimpleNic: public cSimpleModule {
    char messageName[20];
    int msgId = 0;
public:
    virtual InterGeo *generateMessage();
    virtual void forwardMessage(InterGeo *msg);
protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
};

Define_Module(SimpleNic);

void SimpleNic::initialize() {

}

void SimpleNic::handleMessage(cMessage *msg) {
    /*
    std::cout << "Received msg: " << msg << endl;

    int n = this->gateSize("nicGate");
    for(int i=0; i<n; i++) {
        cMessage *msg = new cMessage("RECEIVED");
        send(msg, "nicGate$o", i);
    } */

        // if GeoNode get the message
        InterGeo *ttmsg = check_and_cast<InterGeo *>(msg);

        if (strcmp(messageName, ttmsg->getName()) == 0) {
            EV << "already received (GeoNode).\n";
            delete ttmsg;
            return;
        } else {
            strcpy(messageName, ttmsg->getName());
        }

        if (ttmsg->getHopCount() >= 2) {
            delete ttmsg;
            return;
        }

        // We need to forward the message.
        forwardMessage(ttmsg);
}


InterGeo *SimpleNic::generateMessage()
{
    // Produce source and destination addresses.
    int src = getIndex();   // our module index

    msgId++;
    char msgname[20];
    sprintf(msgname, "tic-%d-%d", src, msgId);

    // Create message object and set source and destination field.
    InterGeo *msg = new InterGeo(msgname);
    msg->setSource(src);

    return msg;
}

void SimpleNic::forwardMessage(InterGeo *msg)
{
    // Increment hop count.
    msg->setHopCount(msg->getHopCount()+1);

    int n = gateSize("nicGate");

    // EV << error:"Size of n equals to " << n << "\n";

    for (int k = 0; k < n; k++) {
        EV << "Forwarding message " << msg << " on gate[" << k << "]\n";
        InterGeo *copy = msg->dup();
        send(copy, "nicGate$o", k);
    }
}
