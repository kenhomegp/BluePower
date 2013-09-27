/****************************************************************************
FILE NAME
    headset_callmanager.h      

DESCRIPTION
    This is the call manager for BC4-Headset
    

NOTES

*/
#ifndef _HS_CALL_MANAGER_H_
#define _HS_CALL_MANAGER_H_

#include <hfp.h>
#include "headset_private.h"

/****************************************************************************
NAME    
    headsetHandleRingInd
DESCRIPTION
   handle a ring indication from the AG

RETURNS
    void
*/
void headsetHandleRingInd( const HFP_RING_IND_T * pInd );

/****************************************************************************
NAME    
    headsetAnswerCall
    
DESCRIPTION
    Answer or Reject an incoming call from the headset

RETURNS
    void
*/
void headsetAnswerOrRejectCall( bool Action );

/****************************************************************************
NAME    
    hfpHeadsetHangUpCall
    
DESCRIPTION
    Hang up the call from the headset.

RETURNS
    void
*/
void headsetHangUpCall( void );

/****************************************************************************
NAME    
    headsetTransferToggle
    
DESCRIPTION
    If the audio is at the headset end transfer it back to the AG and
    vice versa.

RETURNS
    FALSE if an audio transfer to the AG has been initiated.
    TRUE otherwise.
*/
bool headsetTransferToggle( uint16 eventId ) ;
        
/****************************************************************************
NAME    
    headsetInitiateLNR
    
DESCRIPTION
    If HFP and connected - issues command
    If HFP and not connected - connects and issues if not in call
    If HSP sends buton press

RETURNS
    void
*/
void headsetInitiateLNR ( hfp_link_priority priority );

/****************************************************************************
NAME    
    headsetInitiateVoiceDial
    
DESCRIPTION
    If HFP and connected - issues command
    If HFP and not connected - connects and issues if not in call
    If HSP sends buton press

RETURNS
    void
*/
void headsetInitiateVoiceDial ( hfp_link_priority priority );

/****************************************************************************
NAME    
    headsetInitiateLNR
    
DESCRIPTION
    cancels a voice dial request
   
RETURNS
    void
*/
void headsetCancelVoiceDial ( hfp_link_priority priority );

/****************************************************************************
NAME    
    headsetRecallQueuedEvent
    
DESCRIPTION
    Checks to see if an event was Queued and issues it - used on connection

RETURNS
    void
*/
void headsetRecallQueuedEvent ( void ) ;

/****************************************************************************
NAME    
    headsetClearQueueudEvent
    
DESCRIPTION
    Clears the QUEUE - used on failure to connect / power on / off etc

RETURNS
    void
*/
void headsetClearQueueudEvent ( void ) ;

/****************************************************************************
NAME    
    headsetCheckForAudioTransfer
    
DESCRIPTION
    checks on connection for an audio connction and performs a transfer if not present

RETURNS
    void
*/
void headsetCheckForAudioTransfer ( void ) ;

/****************************************************************************
NAME    
    headsetUpdateStoredNumber
    
DESCRIPTION
	Request a number to store from the primary AG
    
RETURNS
    void
*/
void headsetUpdateStoredNumber (void);

/****************************************************************************
NAME    
    headsetWriteStoredNumber
    
DESCRIPTION
	Store number obtained via HfpRequestNumberForVoiceTag in 
    PSKEY_PHONE_NUMBER
    
RETURNS
    void
*/
void headsetWriteStoredNumber ( HFP_VOICE_TAG_NUMBER_IND_T* ind );

/****************************************************************************
NAME
    headsetDialStoredNumber
    
DESCRIPTION
    checks on connection for an audio connction and dials a stored number from 
	the config

RETURNS
    void
*/
void headsetDialStoredNumber ( void ) ;

/****************************************************************************
NAME    
    headsetQueueEvent
    
DESCRIPTION
    Queues an event to be sent once the headset is connected

RETURNS
    void
*/
void headsetQueueEvent ( headsetEvents_t pEvent );

/****************************************************************************
NAME    
    headsetPlaceIncomingCallOnHold
    
DESCRIPTION
	looks for an incoming call and performs the twc hold incoming call function

RETURNS
    void
*/
void headsetPlaceIncomingCallOnHold(void);

/****************************************************************************
NAME    
    headsetAcceptHeldIncomingCall
    
DESCRIPTION
	looks for a held incoming call and performs the twc accept held incoming
    call function

RETURNS
    void
*/
void headsetAcceptHeldIncomingCall(void);

/****************************************************************************
NAME    
    headsetRejectHeldIncomingCall
    
DESCRIPTION
	looks for a held incoming call and performs the twc reject held incoming
    call function

RETURNS
    void
*/
void headsetRejectHeldIncomingCall(void);


#endif

