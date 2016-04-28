
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
typedef std::map<std::string, unsigned int> NodeUidMap;
typedef std::vector<std::string> NodeList;
typedef std::list<std::string> NodeMsgs;

#define LRU_SIZE 10
#define CRIT_RESEND_NUM 1000

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
        NodeUidMap uidMap;
        NodeMap nodeMap;
        NodeMap::iterator mapIter;
        NodeMap joinMsgs;
        int joinMax = -1;
        NodeList joinMaxes;
        NodeMap leaveMsgs;
        int leaveMax = -1;
        NodeList leaveMaxes;
        NodeList nodeList;
        NodeMap critMsgs;
        std::map<std::string, std::map<std::string, cMessage*>> pingReqTimers;
        std::map<std::string, std::string> pingReqSent;
        unsigned int lastIdx;
        std::string groupName;
        std::map<std::string, cGate*> groupConns;
        bool drawGroups;
        std::string leadName;
        unsigned int leadUid;
        bool debugging;

	protected:
		virtual void onBeacon(WaveShortMessage* wsm);
		virtual void onData(WaveShortMessage* wsm);
		void sendMessage(std::string blockedRoadId);
		virtual void handlePositionUpdate(cObject* obj);
		virtual void sendWSM(WaveShortMessage* wsm);

		virtual void handleSelfMsg(cMessage *msg);

		virtual void sendJoin();
		virtual void sendCritical(std::string roadId);
		virtual void sendPing(const char* nodeName);
		virtual int sendPingReq(std::string suspciousNode);
		virtual void sendFail(std::string failedNode);
		virtual void sendAck(std::string dst);
		virtual void sendAck(std::string sendTo, std::string destNode, std::string wsmData);

        virtual void onPing(WaveShortMessage* wsm);
        virtual void onPingReq(WaveShortMessage* wsm);
        virtual void onAck(WaveShortMessage* wsm);

        void saveNodeInfo(WaveShortMessage *wsm);
        const char* getNextNode();
		void addNode(const char* path, std::string name);
		void failNode(std::string name);
		void setTimer(const char* src, const char* dst, const char* data);
		void removeFromList(std::string name);
		void setUpdateMsgs(WaveShortMessage *wsm);
		void setCriticalMsgs(WaveShortMessage *wsm);
		void getUpdateMsgs(WaveShortMessage *wsm);
		void handleAccidentStart();
		void handleAccidentStop();
		void disconnectGroupMember(std::string name);
		void connectGroupMember(const char* path, std::string name);

		void checkLeader();
		/******************************************************************
		 *
		 * Simple Methods. Can be made inline
		 *
		 ******************************************************************/
		inline std::string getMyName() {
		    return mobility->getExternalId();
		}

		inline std::string getGroupName() {
		    // Updated in handlePositionUpdate
		    return groupName;
		}

		inline bool isLeader() {
		    return !getMyName().compare(leadName);
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

		inline bool hasNode(NodeMap someMap, std::string key) {
		    return someMap.find(key) != someMap.end();
		}

		template <class someValue>
		inline bool hasNode(std::map<std::string, someValue *> someMap, std::string key) {
		    return someMap.find(key) != someMap.end();
		}

		inline bool isForMe(WaveShortMessage* wsm) {
		    return std::string(wsm->getDst()) == getMyName() || std::string(wsm->getDst()) == std::string("*");
		}

		inline bool isMyGroup(WaveShortMessage* wsm) {
		    return std::string(wsm->getGroup()) == mobility->getRoadId() || std::string(wsm->getGroup()) == std::string("*");
		}

		inline bool isBroadcast(WaveShortMessage* wsm) {
		    return std::string(wsm->getGroup()) == std::string("*");
		}

		inline bool isMulticast(WaveShortMessage* wsm) {
		            return std::string(wsm->getDst()) == std::string("*");
        }

		inline bool isPingReqAck(std::string src) {
		    return pingReqSent.find(src) != pingReqSent.end();
		}

		inline void setDisplay(const char* color) {
		    findHost()->getDisplayString().updateWith(color);
		}

		inline int findEmptyGate(cModule *mod, const char* gate) {
		    int gSize = mod->gateSize(gate);

		    for(int i=0; i<gSize; i++) {
		        if(!(mod->gate(gate, i)->isConnectedInside()) && !(mod->gate(gate, i)->isConnectedOutside())){
		            return i + 1;
		        }
		    }

		    gSize++;
		    mod->setGateSize(gate, gSize);
		    return gSize;
		}


        /******************************************************************
         *
         * Debug Methods
         *
         ******************************************************************/
		inline void dumpMap() {
		    std::ostringstream os;
		    os << "{ " << getMyName() << ", ";
		    for(NodeMap::iterator it=nodeMap.begin(); it!=nodeMap.end(); ++it) {
		        os << it->first << ": " << status_strs[it->second] <<", ";
		    }
		    os << "}";
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

		inline void debug(std::string msg) {
		    if(debugging) {
		        std::cout <<  simTime().dbl() << " " << getParentModule()->getFullPath() << "\t" << getGroupName() << "\t" << getMyName()  << '\t' << msg << endl;
		    }
		}
};

#endif
