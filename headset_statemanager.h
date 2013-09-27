/****************************************************************************

FILE NAME
    headset_statemanager.h
    
DESCRIPTION
    main headset state information
    
*/


/*!
@file	headset_statemanager.h
@brief	Header file for the state handling of the headset application.

		Provides single point of entry and exit for all of the headset states
*/

#ifndef _HEADSET_STATE_MANAGER_H
#define _HEADSET_STATE_MANAGER_H

#include "headset_private.h"
#include "headset_states.h"



/*!
	@brief Instruct the headset to enter the connectable state

	@param pApp The main application task
	
	@param req_disc If the headset is currently connected then request a HfpDisconnect
	
	After this funciton is called, the headset will be in page scanning mode
*/
void stateManagerEnterConnectableState( bool req_disc ) ;

/*!
	@brief Instruct the headset to enter the connected state

	@param pApp The main application task
	
	If the headset was previously discoverable or connectable, then disables these modes
	
	If the headset was previously in an active call the EventEndOfCall is sent
*/
void stateManagerEnterConnectedState( void ) ;

/*!
	@brief Instruct the headset to enter the connectable/Discoveranble state

	@param pApp The main application task
	
	After this function is callerd, then the headset will be both page scanning and 
    inquiry scanning
*/
void stateManagerEnterConnDiscoverableState( void ) ;

/*!
	@brief Instruct the headset to enter the Incoming call state

	@param pApp The main application task
	
	This occurs after the HFP call setup indicator has changed
	or in HSP mode, the RING has been detected
	
	If the headset was previously in a non-conencted state and the feature bit to 
    auto answer is enabled, then the headset will attempt to answer the call
    
    Any Incoming RING PIO is set high
*/
void stateManagerEnterIncomingCallEstablishState( void ) ;

/*!
	@brief Instruct the headset to enter the outgoing call state

	@param pApp The main application task
	
	This occurs after the HFP call setup indicator has changed
	Any outgoing RING is set high	
*/
void stateManagerEnterOutgoingCallEstablishState( void ) ;

/*!
	@brief Instruct the headset to enter the active call state

	@param pApp The main application task
	
	This occurs after the HFP call indicator has changed
	
	Incoming / outgoing PIOs are cleared 
	Any active call PIO is set 
	
*/
void stateManagerEnterActiveCallState( void ) ;

/*!
	@brief Instruct the headset to begin powering off

	@param pApp The main application task
	
	Disconnects any active SLC (connection)
	
	and waits for the Limbo timeout until an actual power off 
*/
void stateManagerEnterPoweringOffState( void ) ;

/*!
	@brief Instruct the headset to enter the Limbo state

	@param pApp The main application task
	
	The headset is now logically off but physically still on.
	
	This occurs on power on before a power on event has been received 
    OR after a power off event has ben received and before the headset is 
    physically shut down.
    
    The headset remains in this state for the Limbo Timout which can 
    be configured in PSKEY_USR_6 (PSKEY_TIMEOUTS)) 
*/
void stateManagerEnterLimboState( void ) ;

/*!
	@brief Instruct the headset to check the Limbo state

	@param pApp The main application task
	
	This occurs periodically to check if the headset can physically power
	down (after the limbo timeout)
	
	If the charger is not connected then the headset will physically power down 
	
	If the charger is connected, then the device remains in the limbo state in order to 
	service any charger LED indications
*/
void stateManagerUpdateLimboState( void ) ;

/*!
	@brief Instruct the headset to physically power on

	@param pApp The main application task
	
	This occurs only after receiveing a power on event
	After this call, the headset will be in the conenctable state
*/
void stateManagerPowerOn( void ) ;

/*!
	@brief Instruct the headset to enter the three way call waiting state

	@param pApp The main application task	
*/
void stateManagerEnterThreeWayCallWaitingState( void ) ;

/*!
	@brief Instruct the headset to enter the three way call on hold state

	@param pApp The main application task	
*/
void stateManagerEnterThreeWayCallOnHoldState( void ) ;

/*!
	@brief Instruct the headset to enter the three way multi party call state

	@param pApp The main application task	
*/
void stateManagerEnterThreeWayMulticallState( void ) ;

/*!
	@brief Instruct the headset to enter the single incoming on hold state

	@param pApp The main application task	
*/
void stateManagerEnterIncomingCallOnHoldState( void ) ;

/*!
	@brief request the current state of the headset 

    @return The current state of the headset	
*/
headsetState stateManagerGetState ( void ) ;

/*!
	@brief request whether or not the headset is connected 

    @return TRUE if connected
*/
bool stateManagerIsConnected ( void ) ;

/*!
	@brief Instruct the headset to enter the test mode state

	@param pApp The main application task	
*/
void stateManagerEnterTestModeState ( void ) ;

/*!
	
	@brief helper function to get the size of the paired device list

	@return size of the paired device list

*/
uint16 stateManagerGetPdlSize ( void );

/****************************************************************************
NAME	
	stateManagerEnterA2dpStreamingState

DESCRIPTION
    enter A2DP streaming state if not showing any active call states
RETURNS
	void
    
*/
void stateManagerEnterA2dpStreamingState(void);


#endif

