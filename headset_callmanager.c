/****************************************************************************
FILE NAME
    headset_callmanager.h      

DESCRIPTION
    This is the call manager for BC4-Headset
    

NOTES

*/
#include "headset_callmanager.h"
#include "headset_statemanager.h"
#include "headset_tones.h"
#include "headset_audio.h"
#include "headset_private.h"
#include "headset_configmanager.h"
#include "headset_slc.h"
#include "headset_tts.h"
#include "headset_multipoint.h"

#include <ps.h>
#include <connection.h>
#include <hfp.h>
#include <stdlib.h>
#include <audio.h>
#include <string.h>
#include <sink.h>

#ifdef DEBUG_CALL_MAN
    #define CM_DEBUG(x) DEBUG(x)
#else
    #define CM_DEBUG(x) 
#endif

#ifdef Rubidium
#include <rubidium_text_to_speech_plugin.h> 
extern char	TTS_text[50];
#endif

#define SIZE_PSKEY_PHONE_NUMBER 20
#define SIZE_PHONE_NUMBER       21

/****************************************************************************
NAME    
    headsetHandleRingInd
    
DESCRIPTION
    Received a RING indication from the AG.

RETURNS
    void
*/
void headsetHandleRingInd( const HFP_RING_IND_T * pInd )
{       
    /* check whether the ring ind needs to be shown as a call waiting beep
       for multipoint operation, if not multipoint function returns false */
    if(!MPCheckRingInd(pInd))
    {
        /*  Determine which AG has the outband ring (if applicable) and play
    		appropriate tone. */
        if(!pInd->in_band)
        {
             /* determine whether this is AG1 or AG2 and play appropriate tone */
             if(pInd->priority == hfp_primary_link)
             {
            		CM_DEBUG(("CM: OutBandRing - AG1 play ring 1\n")) ;		
			#ifdef Rubidiumx
    				TonesPlayEvent(TONE_TYPE_RING_1);
			#endif
             }
             /* this is AG2 so play ring tone for AG2 */
             else if(pInd->priority == hfp_secondary_link)
             {
            		CM_DEBUG(("CM: OutBandRing - AG2 play ring 2\n")) ;		
			#ifdef Rubidiumx
    				TonesPlayEvent(TONE_TYPE_RING_2);
			#endif
             }
        } 
        else 
        	CM_DEBUG(("CM: inBandRing - no tone played\n")) ;		
    }
}

/****************************************************************************
NAME    
    headsetAnswerCall
    
DESCRIPTION
    Answer an incoming call from the headset

RETURNS
    void
*/
void headsetAnswerOrRejectCall( bool Action )
{
    hfp_link_priority priority;
    
    /* get profile if AG with incoming call */
    priority = HfpLinkPriorityFromCallState(hfp_call_state_incoming);

    CM_DEBUG(("CM: Answer Call on AG%x\n",priority)) ;

    /* if match found */
    if(priority)
    {
        /* answer call */
        HfpCallAnswerRequest(priority, Action);
    }
    /*terminate the ring tone*/
    ToneTerminate();
}


/****************************************************************************
NAME    
    hfpHeadsetHangUpCall
    
DESCRIPTION
    Hang up the call from the headset.

RETURNS
    void
*/
void headsetHangUpCall( void )
{
    /* determine which is the current active call */  
    hfp_link_priority priority;
    
    /* get profile if AG with active call */
    if(theHeadset.sco_sink && (theHeadset.conf->no_of_profiles_connected > 1))
    {
        /* multipoint with more than one AG connected, get the profile of the currently
           active AG */
        priority = HfpLinkPriorityFromAudioSink(theHeadset.sco_sink);
    }
    /* non multipoint or only one AG connected so use AG state */
    else
    {
        priority = HfpLinkPriorityFromCallState(hfp_call_state_active);
        /* if no active call check for an outgoing call to terminate */
        if(!priority)
            priority = HfpLinkPriorityFromCallState(hfp_call_state_outgoing);
    }

    /* if match found */
    if(priority)
    {
        /* answer call */
        CM_DEBUG(("CM: HangUp Call on AG%x\n",priority)) ;
        HfpCallTerminateRequest(priority);
    }
    else
    {
        /* get profile if AG with held active call state */
        priority = HfpLinkPriorityFromCallState(hfp_call_state_held_active);
        /* if found terminate call by means of AT+CHLD cmd */
        if(priority)
        {
            CM_DEBUG(("CM: HangUp TWC active Call on AG1\n")) ;
            HfpCallHoldActionRequest(hfp_primary_link, hfp_chld_release_active_accept_other, 0);
        }
    }
        
    /*if we are muted - then un mute when ending the call*/
    if (theHeadset.gMuted )
    {
       MessageSend(&theHeadset.task , EventMuteOff , 0) ;   
  	}
}

/****************************************************************************
NAME    
    headsetTransferAudio
    
DESCRIPTION
    If the audio is at the headset end transfer it back to the AG and
    vice versa.

RETURNS
    void
*/
bool headsetTransferToggle( uint16 eventId )
{
    /* Get the link to toggle audio on */
    hfp_link_priority priority = audioGetLinkPriority(FALSE);
    sync_pkt_type packet_type = theHeadset.HFP_supp_features.supportedSyncPacketTypes;
	Sink audio_sink;
    
    /* Mask out all but SCO packet types if feature enabled */
    if(theHeadset.features.audio_sco)
        packet_type &= sync_all_sco;
    
    CM_DEBUG(("CM: Audio Transfer\n")) ;

    CM_DEBUG(("packet_type = %x\n",packet_type)) ;
    
    /* Perform audio transfer */
    HfpAudioTransferRequest(priority, hfp_audio_transfer, packet_type, NULL );
	
    /* If we're about to disconnect audio... */
    if(HfpLinkGetAudioSink(priority, &audio_sink))
    {
        /* Flag that the event tone should be delayed until after the SCO _and_ the audio
        plugin have been removed (B-48360). */
        uint8 index = PROFILE_INDEX(priority);
        theHeadset.profile_data[index].status.play_tone_on_sco_disconnect = eventId;
        return FALSE;
    }
    
    return TRUE;
}


/****************************************************************************
NAME    
    headsetInitiateLNR
    
DESCRIPTION
    If HFP and connected - issues command
    If HFP and not connected - connects and issues if not in call
    If HSP sends button press

RETURNS
    void
*/
void headsetInitiateLNR ( hfp_link_priority priority )
{

    CM_DEBUG(("CM: LNR\n")) ;

    /* if headset not connected to any AG initiate a connection */
    if (!stateManagerIsConnected() )
    {
#ifdef ENABLE_AVRCP       
        headsetAvrcpCheckManualConnectReset(NULL);
#endif        
        MessageSend ( &theHeadset.task , EventEstablishSLC , 0 ) ;
        headsetQueueEvent(EventLastNumberRedial) ;
    }
    /* have at least one connection */    
    else
    {
        uint8 Option = hfp_primary_link;

		#ifdef ActiveRubiASR
			#if 0
			if(strcmp(TTS_text , ""))
			{
				CM_DEBUG(("CM : Rubi > UnloadRubiEngine***\n"));	

				TTSTerminate();

				AudioDisconnect();
				/* clear sco_sink value to indicate no routed audio */
				theHeadset.sco_sink = 0;

				UnloadRubidiumEngine();
				memset(TTS_text, 0, sizeof(TTS_text));  
			}
			#endif
			if(theHeadset.TTS_ASR_Playing)
			{
				CM_DEBUG(("CM : Rubi > UnloadRubiEngine***\n"));	

				TTSTerminate();

				AudioDisconnect();
				/* clear sco_sink value to indicate no routed audio */
				theHeadset.sco_sink = 0;

				UnloadRubidiumEngine();

				theHeadset.TTS_ASR_Playing = false;
			}
		#endif
        
        CM_DEBUG(("CM: LNR Connected\n")) ;
   
        /* for multipoint use there are two options */
        if(theHeadset.MultipointEnable) 
        {
            /* these being to use separate LNR buttons, one for AG1 and one for AG2
              	 or use one LNR event and dial using the AG that last made an outgoing
               	call */
               	
            if(theHeadset.features.SeparateLNRButtons)
            {
            	#ifdef BHC612
					#ifdef MP_Config1_
						Option = theHeadset.last_outgoing_ag;
					#else
						Option = priority;				
					#endif
				#else	
	                CM_DEBUG(("CM: LNR use separate LNR buttons\n")) ;
	                Option = priority;
				#endif
            }
            /* dial using AG that made last outgoing call */
            else
            {
                CM_DEBUG(("CM: LNR use last outgoing AG = %d\n",theHeadset.last_outgoing_ag)) ;
                Option = theHeadset.last_outgoing_ag;
            }
        }
        CM_DEBUG(("CM: LNR on AG %d\n",Option)) ;
        /* perform last number redial */        
        HfpDialLastNumberRequest(Option);
    }
}

/****************************************************************************
NAME    
    headsetInitiateVoiceDial
    
DESCRIPTION
    If HFP and connected - issues command
    If HFP and not connected - connects and issues if not in call
    If HSP sends button press

RETURNS
    void
*/
void headsetInitiateVoiceDial ( hfp_link_priority priority )
{
    CM_DEBUG(("CM: VD\n")) ;
	
    if (!stateManagerIsConnected() )
    {	
#ifdef ENABLE_AVRCP
        headsetAvrcpCheckManualConnectReset(NULL);
#endif        
        MessageSend ( &theHeadset.task , EventEstablishSLC , 0 ) ;
        headsetQueueEvent( EventInitateVoiceDial ) ;
        
        theHeadset.VoiceRecognitionIsActive = FALSE ;
    }
    else
    {
        uint8 Option = hfp_primary_link;
    
        CM_DEBUG(("CM: VD Connected\n")) ;
        
        /* for multipoint use there are two options */
        if(theHeadset.MultipointEnable) 
        {
            /* these being to use separate VD buttons, one for AG1 and one for AG2
               or use one VD event and dial using the AG that last made an outgoing
               call */
            if(theHeadset.features.SeparateVDButtons)
            {
                Option = priority;
		CM_DEBUG(("priority...\n"));		
            }
            /* voice dial using AG that made last outgoing call */
            else
            {
                Option = theHeadset.last_outgoing_ag;
		CM_DEBUG(("last outgoing AG...\n"));
            }
        }             
        /* if voice dialling is supported, send command */        
        if (theHeadset.conf->supp_features_local & HFP_VOICE_RECOGNITION ) 
        {    
            CM_DEBUG(("CM: VoiceDial on %d\n",Option)) ;
            HfpVoiceRecognitionEnableRequest(Option, TRUE);    
            theHeadset.VoiceRecognitionIsActive = TRUE ;
        }   
    }    
}
/****************************************************************************
NAME    
    headsetCancelVoiceDial
    
DESCRIPTION
    cancels a voice dial request
   
RETURNS
    void
*/
void headsetCancelVoiceDial ( hfp_link_priority priority )
{
    uint8 Option = hfp_primary_link;
    
    CM_DEBUG(("CM: VD Cancelled")) ;
        
    /* for multipoint use there are two options */
    if(theHeadset.MultipointEnable) 
    {
        /* these being to use separate VD buttons, one for AG1 and one for AG2
           or use one VD event and dial using the AG that last made an outgoing
           call */
        if(theHeadset.features.SeparateVDButtons)
        {
            Option = priority;
        }
        /* voice dial using AG that made last outgoing call */
        else
        {
            Option = theHeadset.last_outgoing_ag;
        }
    }    

    /*if we support voice dialing*/
    if (theHeadset.conf->supp_features_local & HFP_VOICE_RECOGNITION ) 
    {
        /*if we believe voice dial is currently active*/
        if ( theHeadset.VoiceRecognitionIsActive)
        {
            HfpVoiceRecognitionEnableRequest(Option, FALSE);                    
            theHeadset.VoiceRecognitionIsActive = FALSE ;        
        }
    }
}

/****************************************************************************
NAME    
    headsetQueueEvent
    
DESCRIPTION
    Queues an event to be sent once the headset is connected

RETURNS
    void
*/
void headsetQueueEvent ( headsetEvents_t pEvent )
{
    CM_DEBUG(("CM: QQ Ev[%x]\n", pEvent)) ;
    theHeadset.conf->gEventQueuedOnConnection = (pEvent - EVENTS_MESSAGE_BASE);
}

/****************************************************************************
NAME    
    headsetRecallQueuedEvent
    
DESCRIPTION
    Checks to see if an event was Queued and issues it

RETURNS
    void
*/
void headsetRecallQueuedEvent ( void )
{
    /*this is currently only applicable to  LNR and voice Dial but does not care */
    if ((theHeadset.conf->gEventQueuedOnConnection != (EventInvalid - EVENTS_MESSAGE_BASE)))
    {
        switch (stateManagerGetState() )
        {
            case headsetIncomingCallEstablish:
            case headsetOutgoingCallEstablish:
            case headsetActiveCallSCO:            
            case headsetActiveCallNoSCO:       
            case headsetThreeWayCallWaiting:
            case headsetThreeWayCallOnHold:
            case headsetThreeWayMulticall:
                /* Do Nothing Message Gets ignored*/
            break ;
            default:
               /* delay sending of queued message by 1 second as some phones are ignoring the AT command when
                  using native firmware as the connection time is much quicker using that firmware */             
               MessageSendLater ( &theHeadset.task , (theHeadset.conf->gEventQueuedOnConnection + EVENTS_MESSAGE_BASE), 0 , 1500) ; 
            break;
        }
    }    
    
    /*reset the queued event*/
	headsetClearQueueudEvent(); 
}

/****************************************************************************
NAME    
    headsetClearQueueudEvent
    
DESCRIPTION
    Clears the QUEUE - used on failure to connect / power on / off etc

RETURNS
    void
*/
void headsetClearQueueudEvent ( void )
{
    /*this resets the queue - on a conenction fail / power off etc*/
    headsetQueueEvent(EventInvalid) ;
}


/****************************************************************************
NAME    
    headsetCheckForAudioTransfer
    
DESCRIPTION
    checks on connection for an audio connction and performs a transfer if not present

RETURNS
    void
*/
void headsetCheckForAudioTransfer ( void )
{
    headsetState lState = stateManagerGetState() ;
    
    CM_DEBUG(("CALL: Tx[%d] [%x]\n", lState , (int)theHeadset.sco_sink)) ;
/*
typedef enum
{
    hfp_call_state_idle, 				0
    hfp_call_state_incoming, 			1
    hfp_call_state_incoming_held,		2
    hfp_call_state_outgoing,			3
    hfp_call_state_active,			4
    hfp_call_state_twc_incoming,		5
    hfp_call_state_twc_outgoing,		6
    hfp_call_state_held_active,		7
    hfp_call_state_held_remaining,		8
    hfp_call_state_multiparty			9
} hfp_call_state;
*/
    
    switch (lState)
    {
        case headsetIncomingCallEstablish :
        case headsetThreeWayCallWaiting :
        case headsetThreeWayCallOnHold :
        case headsetThreeWayMulticall :
        case headsetIncomingCallOnHold : 
		#ifdef Rubidium
		case headsetActiveCallSCO :
			if(theHeadset.sco_sink != 0)
				break;
		#endif
        case headsetActiveCallNoSCO :
        {              
            Sink sink;            
            hfp_call_state state;  
            hfp_link_priority priority = hfp_invalid_link;
              
            /* check call state and sink of AG1 */
            if((HfpLinkGetCallState(hfp_primary_link, &state))&&(state == hfp_call_state_active)&&
               (HfpLinkGetAudioSink(hfp_primary_link, &sink))&&(!sink))
            {
                priority = hfp_primary_link;
				CM_DEBUG(("Pri state[%d], sink[%x]\n", state , (int)sink)) ;
            }
			else
			{
				#ifdef New_MMI
				if(theHeadset.MultipointEnable)
				{
		            /* or check call state and sink of AG2 */
		            if((HfpLinkGetCallState(hfp_secondary_link, &state))&&(state == hfp_call_state_active)&&
		                    (HfpLinkGetAudioSink(hfp_secondary_link, &sink))&&(!sink))    
		            {
		                priority = hfp_secondary_link;
						CM_DEBUG(("Sec state[%d], sink[%x]\n", state , (int)sink)) ;
		            }
				}
				#else
				if((HfpLinkGetCallState(hfp_secondary_link, &state))&&(state == hfp_call_state_active)&&
						(HfpLinkGetAudioSink(hfp_secondary_link, &sink))&&(!sink))	  
				{
					priority = hfp_secondary_link;
					CM_DEBUG(("Sec state[%d], sink[%x]\n", state , (int)sink)) ;
				}
				#endif
			}
			
            /* if call found with no audio */
            if (priority)
            {
				hfp_audio_params * audio_params = NULL;
                HfpAudioTransferRequest(priority, 
                                        hfp_audio_to_hfp , 
                                        theHeadset.HFP_supp_features.supportedSyncPacketTypes,
                                        audio_params );
				CM_DEBUG(("*** headsetActiveCallNoSCO , HfpAudioTransfer\n")) ;	

				#if 0
				theHeadset.BHC612_BattMeterOK = false;
				MessageSendLater(&theHeadset.task, EventSpare2 , 0 , 2500);/*Recover Battery icon*/
				#endif
	        }    
        }
        break ;
        default:
        break;
    }
}


/****************************************************************************
NAME    
    headsetUpdateStoredNumber
    
DESCRIPTION
	Request a number to store from the primary AG
    
RETURNS
    void
*/
void headsetUpdateStoredNumber (void)
{
    Sink sink;
    
    /* validate HFP available for use, i.e. AG connected */
    if((HfpLinkGetSlcSink(hfp_primary_link, &sink))&&SinkIsValid(sink))
    {
        /* Request user select a number on the AG */
        HfpVoiceTagNumberRequest(hfp_primary_link);
    }
    else
    {
        /* Connect and queue request */
#ifdef ENABLE_AVRCP
        headsetAvrcpCheckManualConnectReset(NULL);
#endif        
        MessageSend ( &theHeadset.task , EventEstablishSLC , 0 ) ;
        headsetQueueEvent( EventUpdateStoredNumber ) ;
    }
}


/****************************************************************************
NAME    
    headsetWriteStoredNumber
    
DESCRIPTION
	Store number obtained via HfpRequestNumberForVoiceTag in 
    PSKEY_PHONE_NUMBER
    
RETURNS
    void
*/
void headsetWriteStoredNumber ( HFP_VOICE_TAG_NUMBER_IND_T* ind )
{
    /* validate length of number returned */
    if((ind->size_phone_number)&&(ind->size_phone_number<SIZE_PSKEY_PHONE_NUMBER))
    {
        uint16 phone_number_key[SIZE_PSKEY_PHONE_NUMBER];
        
        CM_DEBUG(("Store Number "));
        
        /* Make sure the phone number key is all zero to start */
        memset(phone_number_key, 0, SIZE_PSKEY_PHONE_NUMBER*sizeof(uint16));   
        
        /* Write phone number into key array */
        memmove(phone_number_key,ind->phone_number,ind->size_phone_number);        
        
        /* Write to PS */
        PsStore(PSKEY_PHONE_NUMBER, &phone_number_key, ind->size_phone_number);
    }
}


/****************************************************************************
NAME    
    headsetDialStoredNumber
    
DESCRIPTION
	Dials a number stored in PSKEY_PHONE_NUMBER
    If HFP and connected - issues command
    If HFP and not connected - connects and issues if not in call
    If HSP sends button press


RETURNS
    void
*/
void headsetDialStoredNumber ( void )
{	
    uint16 ret_len;
	Sink   sink;
    uint16 phone_number_key[SIZE_PSKEY_PHONE_NUMBER];
    
	CM_DEBUG(("headsetDialStoredNumber\n")) ;
    
    if ((ret_len = PsRetrieve(PSKEY_PHONE_NUMBER, phone_number_key, SIZE_PSKEY_PHONE_NUMBER )))
	{        
        if((HfpLinkGetSlcSink(hfp_primary_link, &sink)) && SinkIsValid(sink))
        {
            /* Send the dial request now */
            CM_DEBUG(("CM:Dial Stored Number (Connected) len=%x\n",ret_len)) ;  
            HfpDialNumberRequest(hfp_primary_link, ret_len, (uint8 *)&phone_number_key[0]);  
        }
        else
        {
            /* Not connected, connect and queue the dial request */
#ifdef ENABLE_AVRCP
            headsetAvrcpCheckManualConnectReset(NULL);
#endif            
            MessageSend ( &theHeadset.task , EventEstablishSLC , 0 ) ;
            headsetQueueEvent( EventDialStoredNumber ) ;
        }
    }
    else
	{
        /*The PSKEY could not be read*/
        MessageSend(&theHeadset.task, EventUpdateStoredNumber, 0);
    }
}

/****************************************************************************
NAME    
    headsetPlaceIncomingCallOnHold
    
DESCRIPTION
	looks for an incoming call and performs the twc hold incoming call function

RETURNS
    void
*/
void headsetPlaceIncomingCallOnHold(void)
{
     hfp_call_state CallState;
     /* find incoming call to hold */
     if((HfpLinkGetCallState(hfp_primary_link, &CallState))&&
        (CallState == hfp_call_state_incoming))
     {
         CM_DEBUG(("MAIN: Hold incoming Call on AG1\n")) ;
         HfpResponseHoldActionRequest(hfp_primary_link, hfp_hold_incoming_call);
     }
     else if((HfpLinkGetCallState(hfp_secondary_link, &CallState))&&
             (CallState == hfp_call_state_incoming))
     {
         CM_DEBUG(("MAIN: Hold incoming Call on AG2\n")) ;
         HfpResponseHoldActionRequest(hfp_secondary_link, hfp_hold_incoming_call);                
     }
 }

/****************************************************************************
NAME    
    headsetAcceptHeldIncomingCall
    
DESCRIPTION
	looks for a held incoming call and performs the twc accept held incoming
    call function

RETURNS
    void
*/
void headsetAcceptHeldIncomingCall(void)
 {
     hfp_call_state CallState;
     /* find incoming held call */
     if((HfpLinkGetCallState(hfp_primary_link, &CallState))&&
        (CallState == hfp_call_state_incoming_held))
     {
         CM_DEBUG(("MAIN: Hold incoming Call on AG1\n")) ;
         HfpResponseHoldActionRequest(hfp_primary_link, hfp_accept_held_incoming_call);
     }
     else if((HfpLinkGetCallState(hfp_secondary_link, &CallState))&&
             (CallState == hfp_call_state_incoming_held))
     {
         CM_DEBUG(("MAIN: Hold incoming Call on AG2\n")) ;
         HfpResponseHoldActionRequest(hfp_secondary_link, hfp_accept_held_incoming_call);                
     }
 }
 
/****************************************************************************
NAME    
    headsetRejectHeldIncomingCall
    
DESCRIPTION
	looks for a held incoming call and performs the twc reject held incoming
    call function

RETURNS
    void
*/
void headsetRejectHeldIncomingCall(void)
 {
     hfp_call_state CallState;
     /* find incoming held call */
     if((HfpLinkGetCallState(hfp_primary_link, &CallState))&&
        (CallState == hfp_call_state_incoming_held))
     {
         CM_DEBUG(("MAIN: Hold incoming Call on AG1\n")) ;
         HfpResponseHoldActionRequest(hfp_primary_link, hfp_reject_held_incoming_call);
     }
     else if((HfpLinkGetCallState(hfp_secondary_link, &CallState))&&
             (CallState == hfp_call_state_incoming_held))
     {
         CM_DEBUG(("MAIN: Hold incoming Call on AG2\n")) ;
         HfpResponseHoldActionRequest(hfp_secondary_link, hfp_reject_held_incoming_call);                
     }
 }
