/****************************************************************************

FILE NAME
    headset_states
    
DESCRIPTION
    The Headset States
*/

#ifndef _HEADSET_STATES_H
#define _HEADSET_STATES_H

/*!
@file   headset_states.h
@brief  Header file for the headset application states

    This defines the headset states used in the headset application

*/

/*!
	@brief The supported headset states
*/
typedef enum
{
        /*! The headset is logically off but physically on - limbo */	
    headsetLimbo,
        /*! The headset is connectable - page scanning */
    headsetConnectable,
        /*! The headset is connectable and discoverable - page and inquiry scanning*/
    headsetConnDiscoverable,
        /*! The headset is connected to an AG*/
    headsetConnected,
        /*! The connected AG has an outgoing call in progress*/
    headsetOutgoingCallEstablish,
        /*! The connected AG has an incoming call in progress*/
    headsetIncomingCallEstablish,
        /*! The connected AG has an active call in progress and the audio is in the headset*/
    headsetActiveCallSCO ,
        /*! The headset is in test mode*/
    headsetTestMode ,
        /*! The connected AG has an active call and a second incoming call*/
    headsetThreeWayCallWaiting,
        /*! The connected AG has an active call and a second call on hold*/
    headsetThreeWayCallOnHold,
        /*! The connected AG has more than one active call*/
    headsetThreeWayMulticall,
        /*! The connected AG has an incoming call on hold*/
    headsetIncomingCallOnHold , 
        /*! The connected AG has an active call and the audio is in the handset*/
    headsetActiveCallNoSCO ,
        /*! The headset is streaming A2DP audio */
    headsetA2DPStreaming ,
        /* low battery state, won't actually change to this state but will be used for independant 
           low battery led warning */
    headsetLowBattery
} headsetState;

/*!
	@brief The maximum number of headset states -keep this value up to date if new stated are added
*/


#define HEADSET_NUM_STATES (headsetLowBattery + 1) 

/*
#define HEADSET_NUM_STATES (headsetwithDock + 1) 
*/



#endif

