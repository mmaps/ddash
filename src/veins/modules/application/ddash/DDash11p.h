
#ifndef DDash11p_H
#define DDash11p_H

#include "veins/modules/application/ieee80211p/BaseWaveApplLayer.h"
#include "veins/modules/mobility/traci/TraCIMobility.h"
#include "veins/modules/mobility/traci/TraCICommandInterface.h"
#include <map>
#include <utility>
#include <list>
#include <string.h>

using Veins::TraCIMobility;
using Veins::TraCICommandInterface;
using Veins::AnnotationManager;
typedef std::map<std::string, int> NodeMap;
typedef std::vector<std::string> NodeList;
typedef std::list<std::string> NodeMsgs;

#define LRU_SIZE 10

class DDash11p : public BaseWaveApplLayer {
	public:
		virtual void initialize(int stage);
		virtual void receiveSignal(cComponent* source, simsignal_t signalID, cObject* obj);

		enum accidentMessageKinds {
		    START_ACC, STOP_ACC
		};
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
		bool sentNoOneDbgMsg = false;
		double timeout;
		double period;
        cMessage *heartbeatMsg;
        cMessage *timeoutMsg;
        int accidentCount;
        simtime_t accidentInterval;
        simtime_t accidentDuration;
        cMessage *startAccidentMsg;
        cMessage *stopAccidentMsg;
        NodeMap nodeMap;
        NodeMap::iterator mapIter;
        NodeMap joinMsgs;
        int joinMax = -1;
        NodeList joinMaxes;
        NodeMap leaveMsgs;
        int leaveMax = -1;
        NodeList leaveMaxes;
        NodeList nodeList;
        std::map<std::string, std::map<std::string, cMessage*>> pingReqTimers;
        std::map<std::string, std::string> pingReqSent;
        unsigned lastIdx;
        std::string groupName;

	protected:
		virtual void onBeacon(WaveShortMessage* wsm);
		virtual void onData(WaveShortMessage* wsm);
		void sendMessage(std::string blockedRoadId);
		virtual void handlePositionUpdate(cObject* obj);
		virtual void sendWSM(WaveShortMessage* wsm);

		virtual void handleSelfMsg(cMessage *msg);

		virtual void sendJoin();
		virtual void sendPing(const char* nodeName);
		virtual void sendPingReq(std::string suspciousNode);
		virtual void sendFail(std::string failedNode);
		virtual void sendAck(std::string dst);
		virtual void sendAck(std::string sendTo, std::string destNode, std::string wsmData);
		virtual void forwardAck(WaveShortMessage* wsm);

        virtual void onPing(WaveShortMessage* wsm);
        virtual void onPingReq(WaveShortMessage* wsm);
        virtual void onAck(WaveShortMessage* wsm);

        void saveNodeInfo(WaveShortMessage *wsm);
        const char* getNextNode();
		void addNode(const char* name);
		void failNode(std::string name);
		void setTimer(const char* src, const char* dst, const char* data);
		void removeFromList(std::string name);
		void setUpdateMsgs(WaveShortMessage *wsm);
		void getUpdateMsgs(WaveShortMessage *wsm);
		void handleAccidentStart();
		void handleAccidentStop();

		/******************************************************************
		 *
		 * Simple Methods. Can be made inline
		 *
		 ******************************************************************/
		inline std::string getMyName() {
		    return mobility->getExternalId();
		}

		inline std::string getGroup() {
		    // Updated in handlePositionUpdate
		    return groupName;
		}

		inline bool roadChanged() {
		    return groupName.compare(mobility->getRoadId());
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

		bool hasNode(NodeMap someMap, std::string key) {
		    return someMap.find(key) != someMap.end();
		}

		inline bool isForMe(WaveShortMessage* wsm) {
		    return std::string(wsm->getDst()) == getMyName() || std::string(wsm->getDst()) == std::string("*");
		}

		inline bool isMyGroup(WaveShortMessage* wsm) {
		    return std::string(wsm->getGroup()) == mobility->getRoadId();
		}

		inline bool isPingReqAck(std::string src) {
		    return pingReqSent.find(src) != pingReqSent.end();
		}

		inline void setColor(const char* color) {
		    findHost()->getDisplayString().updateWith(color);
		}
        /******************************************************************
         *
         * Debug Methods
         *
         ******************************************************************/
		inline void dumpMap() {
		    std::ostringstream os;
		    os << "MAP: " << getMyName() << ", ";
		    for(NodeMap::iterator it=nodeMap.begin(); it!=nodeMap.end(); ++it) {
		        os << it->first << ": " << it->second <<", ";
		    }
		    debug(os.str());
		}

		inline void dumpList() {
		    std::ostringstream os;
		    os << "LIST: " << getMyName() << ", ";
		    for(std::string s: nodeList) {
		        os << s << ", ";
		    }
		    debug(os.str());
		}

		inline void dumpNode(std::string name) {
		    std::ostringstream os;
		    os << "Node(" << name << "): ";
	        switch(nodeMap[name]){
                case ALIVE:
                    os << "alive";
                    break;
                case PINGWAIT:
                    os << "ping wait";
                    break;
                case PINGWAIT2:
                    os << "ping wait 2";
                    break;
                case PINGREQWAIT:
                    os << "ping request wait";
                    break;
                default:
                    os << "something else";
	        }
	        debug(os.str());
		}

		void debug(std::string msg) {
		    std::cout <<  simTime().dbl() << " (" << getGroup() << ") " << getMyName() << ": " << msg << endl;
		}
};

#endif
