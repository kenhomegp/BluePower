#include <app/message/system_message.h>
#include <connection.h>
#include <codec_.h>
#include <stdio.h>
#include <energy.h>

#include "headset_private.h"
#include "headset_energy_filter.h"

#ifdef ENABLE_ENERGY_FILTER		

#ifdef DEBUG_FILTER_ENABLE
	 #define DEBUG_FILTER(x) {printf x;}
#else
	#define DEBUG_FILTER(x) 
#endif

typedef enum {
	VOICE_ACTIVITY_OFF = 0,
	VOICE_ACTIVITY_ON = 1
} VoiceActivity;

/* The current Voice Activity State */
static VoiceActivity voice_activity = 0;

/*
	Private function used internally to change dynamically the filter 
	gain and the energy thresholds.
*/
static void set_filter(void) {
#ifndef APPINCLSOUNDCLEAR
	uint16 i;
	uint16 filter[10];
	uint16 threshold_upper = 0;
	uint16 threshold_lower = 0;
	
	switch (voice_activity) {
		case VOICE_ACTIVITY_OFF:
			threshold_lower = 0x0000; /* can't become smaller */
			threshold_upper = theHeadset.conf->filter.vad_threshold;
			filter[0] = theHeadset.conf->filter.coefficients[0];
			break;
		case VOICE_ACTIVITY_ON:
			threshold_lower = theHeadset.conf->filter.vad_threshold;
			threshold_upper = 0xFFFF; /* can't become bigger*/
			filter[0] = theHeadset.conf->filter.echo_reduction_gain;
			break;
	}
	for (i=1; i<10; i++) {
		filter[i] = theHeadset.conf->filter.coefficients[i];
	}
	
	CodecSetIIRFilterA(TRUE, filter);
	CodecSetIIRFilterB(TRUE, filter);
	
	EnergyEstimationSetBounds(theHeadset.sco_sink, threshold_lower, threshold_upper);
#endif
}

/**
 	Switch On the wind noise filter
 */
void Filter_On(void) 
{
	DEBUG_FILTER(("[FILTER] ON\n"));
	
	MessageSinkTask(theHeadset.sco_sink, &theHeadset.task);
	voice_activity = VOICE_ACTIVITY_OFF;
	set_filter();
}

/**
 	Switch Off the wind noise filter
 */
void Filter_Off(void) 
{
	DEBUG_FILTER(("[FILTER] OFF\n"));
	CodecSetIIRFilterA(FALSE, theHeadset.conf->filter.coefficients);
	CodecSetIIRFilterB(FALSE, theHeadset.conf->filter.coefficients);
	
	EnergyEstimationOff(theHeadset.sco_sink);
}

/**
 	Energy changed handler
 */
void Filter_EnergyChanged(Message message) {
	MessageEnergyChanged* m;
	m = (MessageEnergyChanged*) message;
	
	if (m->over_threshold) {
		DEBUG_FILTER(("[FILTER] VOICE_ACTIVITY_ON\n"));
		voice_activity = VOICE_ACTIVITY_ON;
	} else {
 		DEBUG_FILTER(("[FILTER] VOICE_ACTIVITY_OFF\n"));
		voice_activity = VOICE_ACTIVITY_OFF;
	}
	set_filter();
}

#else
static const int dummy ;

#endif

