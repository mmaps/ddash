
#ifndef TDDashRSU11p_H
#define TDDashRSU11p_H

#include "veins/modules/application/ieee80211p/BaseWaveApplLayer.h"
#include "veins/modules/world/annotations/AnnotationManager.h"
#include <iostream>

using Veins::AnnotationManager;

/**
 * Small RSU Demo using 11p
 */
class DDashRSU11p : public BaseWaveApplLayer {
	public:
		virtual void initialize(int stage);
	protected:
		AnnotationManager* annotations;
		BaseMobility* mobi;
		bool sentMessage;
		bool flashOn;
	protected:
		virtual void onBeacon(WaveShortMessage* wsm);
		virtual void onData(WaveShortMessage* wsm);
		void sendMessage(std::string blockedRoadId);
		virtual void sendWSM(WaveShortMessage* wsm);
		cMessage* heartbeat;
};

#endif
