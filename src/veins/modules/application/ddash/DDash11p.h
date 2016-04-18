
#ifndef DDash11p_H
#define DDash11p_H

#include "veins/modules/application/ieee80211p/BaseWaveApplLayer.h"
#include "veins/modules/mobility/traci/TraCIMobility.h"
#include "veins/modules/mobility/traci/TraCICommandInterface.h"
#include <map>
#include <utility>

using Veins::TraCIMobility;
using Veins::TraCICommandInterface;
using Veins::AnnotationManager;
typedef std::map<std::string, int> NodeMap;
typedef std::vector<std::string> NodeList;


class DDash11p : public BaseWaveApplLayer {
	public:
		virtual void initialize(int stage);
		virtual void receiveSignal(cComponent* source, simsignal_t signalID, cObject* obj);

	protected:
		TraCIMobility* mobility;
		TraCICommandInterface* traci;
		TraCICommandInterface::Vehicle* traciVehicle;
		AnnotationManager* annotations;
		simtime_t lastDroveAt;
		simtime_t timeoutTime;
		bool sentMessage;
		bool flashOn;
		bool sentJoinDbgMsg = false;
		bool sentPing = false;
        cMessage *heartbeatMsg;
        NodeMap nodeMap;
        NodeMap::iterator mapIter;
        NodeList nodeList;
        unsigned lastIdx;

	protected:
		virtual void onBeacon(WaveShortMessage* wsm);
		virtual void onData(WaveShortMessage* wsm);
		void sendMessage(std::string blockedRoadId);
		virtual void handlePositionUpdate(cObject* obj);
		virtual void sendWSM(WaveShortMessage* wsm);
		virtual void handleSelfMsg(cMessage *msg);
		virtual void sendJoin();
		virtual void sendPing();
        virtual void onJoin(WaveShortMessage* wsm);
        virtual void onPing(WaveShortMessage* wsm);
        virtual void onPingReq(WaveShortMessage* wsm);
        virtual void onAck(WaveShortMessage* wsm);
        const char* getNextNode();
		void addNode(const char* name);

		/******************************************************************
		 *
		 * Simple Methods. Can be made inline
		 *
		 ******************************************************************/
		std::string getMyName() {
		    return mobility->getExternalId();
		}

		bool isMyName(const char* nodeName) {
		    return mobility->getExternalId() == std::string(nodeName);
		}

		bool isMyName(std::string nodeName) {
		    return mobility->getExternalId() == nodeName;
		}

		bool hasNode(const char* nodeName) {
		    return nodeMap.find(std::string(nodeName)) != nodeMap.end();
		}

		bool hasNode(std::string nodeName) {
		    return nodeMap.find(nodeName) != nodeMap.end();
		}

        /******************************************************************
         *
         * Debug Methods
         *
         ******************************************************************/
		void dumpMap() {
		    for(NodeMap::iterator it=nodeMap.begin(); it!=nodeMap.end(); ++it) {
		        std::cout << it->first << ", ";
		    }
		    std::cout << endl;
		}
		void debug(std::string msg) {
		    std::cout << mobility->getExternalId() + " (@ " + mobility->getRoadId() + "): " + msg << endl;
		}
};

#endif
