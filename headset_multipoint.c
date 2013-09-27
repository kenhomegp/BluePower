/****************************************************************************

FILE NAME
    headset_multipoint.h      

DESCRIPTION
    This is the multipoint manager 
    

NOTES

*/
#include "headset_multipoint.h"
#include "headset_callmanager.h"
#include "headset_statemanager.h"
#include "headset_tones.h"
#include "headset_audio.h"
#include "headset_private.h"
#include "headset_configmanager.h"
#include "headset_slc.h"
#include "headset_tts.h"

#include <ps.h>
#include <connection.h>
#include <hfp.h>
#include <stdlib.h>
#include <audio.h>

#ifdef DEBUG_MULTI_MAN
    #define MP_DEBUG(x) DEBUG(x)
#else
    #define MP_DEBUG(x) 
#endif

#ifdef Rubidium
#include <rubidium_text_to_speech_plugin.h> 
#endif

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


/****************************************************************************
NAME    
    headsetHandleCallInd
    
DESCRIPTION
   handle a call setup indication from the AG or AG's in the case of multipoint

RETURNS
    void
*/
void headsetHandleCallInd(const HFP_CALL_STATE_IND_T *pInd)
{
	#ifdef Rubi_TTS
	Sink sinkAG;
	#endif

    /* initialise call state of other AG to idle, should a second AG not exist the 
       get call state function will fail */
    hfp_call_state CallStateOtherAG = hfp_call_state_idle;
                
    /* get call state of other AG */
    HfpLinkGetCallState(OTHER_PROFILE(pInd->priority), &CallStateOtherAG);
    
    MP_DEBUG(("MP: CallInd [%d] on AG%d, OtherAG state = [%d]\n",pInd->call_state, pInd->priority, CallStateOtherAG)) ;		

    /* determine call state */
    switch(pInd->call_state)
    {
       /* no longer in call */ 
       case hfp_call_state_idle:
            /* if state was incoming call terminate ringtone */
            if(stateManagerGetState() == headsetIncomingCallEstablish)
            {
                /* Terminate ring tone */
                ToneTerminate();
#ifdef Rubidiumx
                TTSTerminate();
#endif
            }
      		/* determine whether there is another AG and it has an active call 
               of some description on it */
            if(CallStateOtherAG >= hfp_call_state_active)
            {
                /* update the sco state of the dropped call to indicate the sco is about to be
                   disconnected which will allow the audio routing handler to immediately route
                   the audio of the other active call that was previously on hold*/
                setScoPriorityFromHfpPriority(pInd->priority, sco_about_to_disconnect);                    
                /* is the remaining call active or held? */
                if(getScoPriorityFromHfpPriority(OTHER_PROFILE(pInd->priority)) == sco_held_call)
                {
                    /* take this call off hold and route its audio */
                    setScoPriorityFromHfpPriority(OTHER_PROFILE(pInd->priority), sco_active_call);                    
                }
                /* down to one call now so change to active call state */        
                stateManagerEnterActiveCallState();
            }
            /* check for incoming call on other AG */
            else if(CallStateOtherAG == hfp_call_state_incoming)
            {
               /* indication of an incoming call */     
               stateManagerEnterIncomingCallEstablishState();
            }
            /* non multipoint use or only one active call */
            else
            {
                MP_DEBUG(("MP: CallInd idle on AG%d = connected\n",pInd->priority)) ;		
                /* call now finished, update headset state */
                stateManagerEnterConnectedState();

				#ifdef Rubi_TTS
				if(HfpLinkGetAudioSink(pInd->priority, &sinkAG))
				{
					if(theHeadset.sco_sink == 0)
						theHeadset.sco_sink = sinkAG;
				}
				#endif
            }
       break;
       
       /* incoming call indication */
       case hfp_call_state_incoming:
          MP_DEBUG(("MP: CallInd incoming on AG%d = IncCallEst\n",pInd->priority)) ;	
       		
           /* determine the number of AG's & active or held calls */
           if(CallStateOtherAG >= hfp_call_state_active)
           {
               /* change state to TWC call waiting and indicate call via mp call waiting event */
               stateManagerEnterThreeWayCallWaitingState();                                                 
           }
           /* non multipoint or only 1 call */
           else
           {
               /* indication of an incoming call */     
               stateManagerEnterIncomingCallEstablishState();
           }
 
       break;

       /* indication of an incoming call being put on hold */
       case hfp_call_state_incoming_held:
           /* determine the number of AG's & active or held calls */
           if(CallStateOtherAG >= hfp_call_state_active)
           {
               /* change state to TWC call waiting and indicate call via mp call waiting event */
               stateManagerEnterThreeWayCallOnHoldState();                                                 
           }
           /* non multipoint or only 1 call */
           else
           {
               /* change to incoming call on hold state */
               stateManagerEnterIncomingCallOnHoldState();
           }
       break;   		
       
       /* indication of an outgoing call */
       case hfp_call_state_outgoing:
    	   MP_DEBUG(("MP: CallInd outgoing on AG%d = outgoingEstablish\n",pInd->priority)) ;		
           /* outgoing call establish, now in active call state */
           setScoPriorityFromHfpPriority(pInd->priority, sco_active_call);                    
           /* determine the number of AG's & active or held calls */
           if(CallStateOtherAG >= hfp_call_state_active)
           {
               /* set the sco state of the current active call to on hold */
               setScoPriorityFromHfpPriority(OTHER_PROFILE(pInd->priority), sco_held_call);                    
               /* enter on hold state */
               stateManagerEnterThreeWayCallOnHoldState();
           }
           /* non multipoint or only call */
           else
           {
                /* indication of an outgoing call */     
                stateManagerEnterOutgoingCallEstablishState();
           }
       break;

       /* call is now in active call state, answered incoming or outgoing call */
       case hfp_call_state_active:
     	   MP_DEBUG(("MP: CallInd active on AG%d = ActiveCall\n",pInd->priority)) ;		

#ifdef NowSpeak           
     	   /* ensure TTS caller announce is terminated */           
     	   TTSTerminate();
#endif

#ifdef Rubi_TTS
			if(theHeadset.TTS_ASR_Playing == true)
			{
				TTSTerminate();
				UnloadRubidiumEngine();
				theHeadset.TTS_ASR_Playing = false;
    			MP_DEBUG(("Rubi > Stop Caller ID...\n" )) ;	
			}
#endif

           /* if this is the indication of a call with in band ring having been answered,
              update the audio priority as this is now an active call */
           if(getScoPriorityFromHfpPriority(pInd->priority) == sco_inband_ring)
           {
               /* now in active call audio state */
               setScoPriorityFromHfpPriority(pInd->priority, sco_active_call);                    
           }
           
           /* determine the number of AG's & active or held calls */
           if(CallStateOtherAG >= hfp_call_state_active)
           {
               /* set the sco state of the current active call to on hold */
               setScoPriorityFromHfpPriority(OTHER_PROFILE(pInd->priority), sco_held_call);                    
               /* enter on hold state */
               stateManagerEnterThreeWayCallOnHoldState();
           }
           /* non multipoint or only active call */
           else
           {
               /* check whether the other AG has an incoming call on it, use case of power on
                  to two incoming calls, if so go to twc waiting state such that second incoming
                  call can be handled by use of twc events */
               if(CallStateOtherAG == hfp_call_state_incoming)
                   stateManagerEnterThreeWayCallWaitingState();   
               /* no second incoming call on other AG */
               else
                   stateManagerEnterActiveCallState();
           }
       break;

       /* indication of a second incoming call */
       case hfp_call_state_twc_incoming:
      	   MP_DEBUG(("MP: CallInd twcIncoming on AG%d = callWaiting\n",pInd->priority)) ;		
           /* indication of a three way call situation with an incoming 
              second call */
           stateManagerEnterThreeWayCallWaitingState();   
       break;

       /* indication of a second call which is an outgoing call */
       case hfp_call_state_twc_outgoing:
      	   MP_DEBUG(("MP: CallInd twcOutgoing on AG%d = CallOnHold\n",pInd->priority)) ;		
           /* twc outgoing call establish, now in call on hold call state */
           setScoPriorityFromHfpPriority(pInd->priority, sco_active_call);                    
           /* determine the number of AG's & active or held calls */
           if(CallStateOtherAG >= hfp_call_state_active)
           {
               /* set the sco state of the current active call to on hold */
               setScoPriorityFromHfpPriority(OTHER_PROFILE(pInd->priority), sco_held_call);                    
           }      
           /* indication of a twc on hold state change due to
              second outgoing call */
           stateManagerEnterThreeWayCallOnHoldState();
       break;

       case hfp_call_state_held_active:
      	   MP_DEBUG(("MP: CallInd heldActive on AG%d = CallOnHold\n",pInd->priority)) ;	
           stateManagerEnterThreeWayCallOnHoldState();
       break;

       case hfp_call_state_held_remaining:
      	   MP_DEBUG(("MP: CallInd heldRemaining on AG%d = >\n",pInd->priority)) ;		
       break;

       case hfp_call_state_multiparty:
       	   MP_DEBUG(("MP: CallInd multiparty on AG%d = twcMultiCall>\n",pInd->priority)) ;		
           /* indication of conference call */
           stateManagerEnterThreeWayMulticallState();
       break;      
    }
    
    /* route the appropriate audio */
    audioHandleRouting();                

	#ifdef Rubidium
		if(pInd->call_state == hfp_call_state_held_remaining)
		{
			/*#ifdef Three_Language*/
			#if defined(Three_Language) || defined(MANDARIN_SUPPORT)
				if(theHeadset.sco_sink != 0)
				{
					AudioDisconnect();
            		/* clear sco_sink value to indicate no routed audio */
            		theHeadset.sco_sink = 0;
					MP_DEBUG(("MP: Call on hold\n")) ;
					theHeadset.BHC612_TEMP = 3;
					MessageSendLater (&theHeadset.task , EventSpare3 , 0, 250 ) ; 
				}
			#endif
		}
	#endif

}

/****************************************************************************
NAME    
    MPCheckRingInd
    
DESCRIPTION
   check whether multipoint is enabled and there is more than one call,
   if so indicate a multipoint call waiting event, if not return false

RETURNS
   true or false as to whether the ring ind has been handled
*/
bool MPCheckRingInd(const HFP_RING_IND_T * pInd)
{  
    /* determine the number of AG's & active or held calls */
    if((theHeadset.MultipointEnable) && 
       (theHeadset.sco_sink) &&
       (HfpLinkPriorityFromAudioSink(theHeadset.sco_sink) != pInd->priority))
    {
        /* already have another call so indicate a call waiting event */
        MessageSend(&theHeadset.task,EventMultipointCallWaiting,0);
        /* ring ind handled so return true */
        return TRUE;
    }  
    /* not a multipoint twc ring ind situation */
    return FALSE;    
}

/****************************************************************************
NAME    
    MpReleaseAllHeld
    
DESCRIPTION
   terminate call found in the on hold audio state

RETURNS

*/
void MpReleaseAllHeld(void)
{
    /* there may be up three lots of calls to release, determine which AG's
       have held calls and whether multipoint is in use also */
    hfp_call_state state;  

    MP_DEBUG(("MP: ReleaseAllHeld \n")) ;		

    /* check call state of AG1, if call state matches held active or twc waiting 
       then return link priority value */
    if((HfpLinkGetCallState(hfp_primary_link, &state))&&
       ((state == hfp_call_state_held_active)||(state == hfp_call_state_twc_incoming)))
    {
        /* perform release all held on appropriate AG */
        HfpCallHoldActionRequest(hfp_primary_link, hfp_chld_release_held_reject_waiting, 0);
    }
   
    /* check whether multipoint has two calls in progress in which case one will be on hold
       so find profile for this and end its call */
    if(state == hfp_call_state_incoming)
        headsetAnswerOrRejectCall(FALSE);
    /* or a held mp call */
    if(getScoPriorityFromHfpPriority(hfp_primary_link) == sco_held_call)
        HfpCallTerminateRequest(hfp_primary_link);   
            
    /* check call state of AG2, if call state matches then return link priority value */
    if((HfpLinkGetCallState(hfp_secondary_link, &state))&&
       ((state == hfp_call_state_held_active)||(state == hfp_call_state_twc_incoming)))
    {
        /* perform release all held on appropriate AG */
        HfpCallHoldActionRequest(hfp_secondary_link, hfp_chld_release_held_reject_waiting, 0);
    }
    
    /* check whether multipoint has two calls in progress in which case one will be on hold
       so find profile for this and end its call */
    if(state == hfp_call_state_incoming)
        headsetAnswerOrRejectCall(FALSE);
    /* or a held mp call */
    if(getScoPriorityFromHfpPriority(hfp_secondary_link) == sco_held_call)
        HfpCallTerminateRequest(hfp_secondary_link);   

}

/****************************************************************************
NAME    
    MpAcceptWaitingReleaseActive
    
DESCRIPTION
   terminate call found in the active audio state and answer incoming call
   audio con/discon will take care of audio routing

RETURNS

*/
void MpAcceptWaitingReleaseActive(void)
{
    hfp_link_priority priorityActive;
    hfp_call_state CallStateAG1 = hfp_call_state_idle;
    hfp_call_state CallStateAG2 = hfp_call_state_idle;

    /* determine which is the currently active call */
    priorityActive = HfpLinkPriorityFromAudioSink(theHeadset.sco_sink);
    /* determine the call state of AG1 */
    HfpLinkGetCallState(hfp_primary_link, &CallStateAG1);
    /* determine the call state of AG2 */
    HfpLinkGetCallState(hfp_secondary_link, &CallStateAG2);

    MP_DEBUG(("MP: MpAcceptWaitingReleaseActive Priority = %d, State AG1 = %d, AG2 = %d\n",priorityActive,CallStateAG1,CallStateAG2)) ;		

    /*  or AG1 active but call waiting is on AG2 and AG2 has only one call */
    if((priorityActive == hfp_primary_link)&&((CallStateAG2 == hfp_call_state_incoming)||
                                              (CallStateAG2 == hfp_call_state_twc_incoming)||
                                              (CallStateAG2 == hfp_call_state_held_active)))
    {
        /* incoming on AG2, this is only call so needs answer call on AG2
           and accept waiting release active on AG1 */  
        MpDropCall(hfp_primary_link);
        /* call waiting is single call on AG2 */
        if(CallStateAG2 == hfp_call_state_incoming)        
        {
            /* accept waiting call */
            HfpCallAnswerRequest(hfp_secondary_link, TRUE);
        }
        /* call waiting is second call on AG2 */
        else
        {
            /* accept call on AG2 */
            HfpCallHoldActionRequest(hfp_secondary_link, hfp_chld_hold_active_accept_other, 0);
        }            
        /* audio call swap required as accepting call on other AG */
        setScoPriorityFromHfpPriority(hfp_primary_link, sco_held_call);                    
        setScoPriorityFromHfpPriority(hfp_secondary_link, sco_active_call);                    
    }
    /*  or AG2 active but call waiting is on AG1 and AG1 has only one call */
    else if((priorityActive == hfp_secondary_link)&&((CallStateAG1 == hfp_call_state_incoming)||
                                                     (CallStateAG1 == hfp_call_state_twc_incoming)))
    {
        /* incoming on AG1, this is only call so needs answer call on AG1
           and accept waiting release active CHLD = 1 on AG2 */   
        MpDropCall(hfp_secondary_link);
        
        if(CallStateAG1 == hfp_call_state_incoming)        
        {
            /* accept waiting call */
            HfpCallAnswerRequest(hfp_primary_link, TRUE);
        }
        else
        {
            /* accept call on AG1 */
            HfpCallHoldActionRequest(hfp_primary_link, hfp_chld_hold_active_accept_other, 0);           
        }
        /* audio call swap required as accepting call on other AG */
        setScoPriorityFromHfpPriority(hfp_primary_link, sco_active_call);                    
        setScoPriorityFromHfpPriority(hfp_secondary_link, sco_held_call);                    
    }
    /* AG1 is active with incoming call on AG1, cover both call waiting and call held cases
       whereby CHLD = 1 is required to drop active call */
    else if((priorityActive == hfp_primary_link)&&((CallStateAG1 == hfp_call_state_twc_incoming)||
                                                   (CallStateAG1 == hfp_call_state_held_active)||
                                                   (CallStateAG1 == hfp_call_state_multiparty)))
    {
        /* active call is on AG1 with call waiting on AG1 so send CHLD = 1, rel and accept */   
        HfpCallHoldActionRequest(hfp_primary_link, hfp_chld_release_active_accept_other, 0);
        /* no call swap required as already on AG1 with incoming call */
    }
    /* or AG2 is active with incoming call on AG2, cover both call waiting and call held cases
       whereby CHLD = 1 is required to drop active call */
    else if((priorityActive == hfp_secondary_link)&&((CallStateAG2 == hfp_call_state_twc_incoming)||
                                                     (CallStateAG2 == hfp_call_state_held_active)||
                                                     (CallStateAG2 == hfp_call_state_multiparty)))
    {
        /* active call is on AG2 with call waiting on AG2 so send CHLD = 1, rel and accept */   
        HfpCallHoldActionRequest(hfp_secondary_link, hfp_chld_release_active_accept_other, 0);        
        /* no call swap required as already on AG2 with incoming call */
    }
    /* one call on each AG, drop the active call */
    else 
    {
        /* if there is an active call with sco */
        if(priorityActive)
            MpDropCall(priorityActive);
        /* if there is no active call with sco, check for active call without sco as being the only
           other possibilty */
        else
        {
            /* does AG1 have an active call without sco ? */
            if((CallStateAG1 == hfp_call_state_active)&&
               (getScoPriorityFromHfpPriority(hfp_primary_link) == sco_none))
            {
                MpDropCall(hfp_primary_link);
            }
            /* does AG2 have an active call without sco ? */
            else if((CallStateAG2 == hfp_call_state_active)&&
                    (getScoPriorityFromHfpPriority(hfp_secondary_link) == sco_none))
            {
                MpDropCall(hfp_secondary_link);
            }
        }
    }
    
    /* redo audio routing */
    audioHandleRouting();

}

/****************************************************************************
NAME    
    MpAcceptWaitingHoldActive
    
DESCRIPTION
   change state of active call to on hold state and answer incoming call, audio
   conn/discon will take care of routing audio
   
RETURNS

*/
void MpAcceptWaitingHoldActive(void)
{
    hfp_link_priority priority = hfp_invalid_link;
    uint8 NoOfCallsAG1 = 0;
    uint8 NoOfCallsAG2 = 0;
    hfp_call_state CallStateAG1 = hfp_call_state_idle;
    hfp_call_state CallStateAG2 = hfp_call_state_idle;
    hfp_link_priority priority_audio_sink = HfpLinkPriorityFromAudioSink(theHeadset.sco_sink);
    
    /* determine the number of calls on this headset */
    /* find number of calls on AG 1 */
    if(HfpLinkGetCallState(hfp_primary_link, &CallStateAG1))
    {
        /* if no TWC state then only 1 call */
        if((CallStateAG1 == hfp_call_state_active)||
           (CallStateAG1 == hfp_call_state_twc_incoming)||
           (CallStateAG1 == hfp_call_state_multiparty))
            NoOfCallsAG1 = 1;
        else if(CallStateAG1 == hfp_call_state_held_active)
            NoOfCallsAG1 = 2;
    }    
    /* find number of calls on AG 2 */
    if(HfpLinkGetCallState(hfp_secondary_link, &CallStateAG2))
    {
        /* if no TWC then only 1 call */
        if((CallStateAG2 == hfp_call_state_active)||
           (CallStateAG2 == hfp_call_state_twc_incoming)||
           (CallStateAG2 == hfp_call_state_multiparty))
            NoOfCallsAG2 = 1;
        else if(CallStateAG2 == hfp_call_state_held_active)
            NoOfCallsAG2 = 2;
    }

    MP_DEBUG(("MP: HoldSwap State AG1 = %d,%d State AG2 = %d,%d \n",CallStateAG1,NoOfCallsAG1,CallStateAG2,NoOfCallsAG2)) ;		

	#ifdef ReleaseHeldCall
		/*#ifdef MANDARIN_SUPPORT*/
		#if 1
		if((CallStateAG1 == hfp_call_state_held_remaining && NoOfCallsAG1 == 0) && (CallStateAG2 == hfp_call_state_idle && NoOfCallsAG2 == 0))
		{
			MP_DEBUG(("MP: Release held call, sco = %d,%d\n",(uint16)theHeadset.sco_sink,theHeadset.PressCallButton)) ;

			if((theHeadset.BHC612_TEMP == 0) && (theHeadset.PressCallButton == true))
			{
				theHeadset.BHC612_TEMP = 2;

				if(theHeadset.sco_sink == 0)
				{
					MessageSendLater (&theHeadset.task , EventSpare3 , 0, 100 );
					MP_DEBUG(("MP: Prompt 'Connected'\n"));
				}

				return;
			}						

			if((theHeadset.BHC612_TEMP == 0) && (theHeadset.PressCallButton == false))
			{
				theHeadset.PressCallButton = true;
				MP_DEBUG(("MP: PressCallButton : [T]\n"));
			}
		}
		#endif
	#endif
	
    /* look for twc call waiting AG state */
    priority = HfpLinkPriorityFromCallState(hfp_call_state_twc_incoming);  
    /* if not found look for outgoing call state to swap between active and twc outgoing */    
    if(!priority)    
    {
        /* look for held call */
        priority = HfpLinkPriorityFromCallState(hfp_call_state_held_active);
        /* if not found look for outgoing call state to swap between active and twc outgoing */    
        if(!priority)    
        {
            /* look for outgoing call */
            priority = HfpLinkPriorityFromCallState(hfp_call_state_twc_outgoing);
            /* if not found get active call instead */
            if(!priority)    
            {
                /* look for active call */
                priority = HfpLinkPriorityFromCallState(hfp_call_state_active);
                /* finally look for held remaining in the case of a single call that has been put on hold */
                if(!priority)    
                {
                    /* look for active call */
                    priority = HfpLinkPriorityFromCallState(hfp_call_state_held_remaining);
                }                
            }
        }        
    }
    
    /* if there is not an active call on each AG but one active call on one AG and
       an incoming call on the other AG */
    if(!(NoOfCallsAG1 && NoOfCallsAG2)&&
       ((NoOfCallsAG1 && (CallStateAG2 == hfp_call_state_incoming))||
        (NoOfCallsAG2 && (CallStateAG1 == hfp_call_state_incoming)))
      )       
    {
        /* answer call on incoming AG and hold call audio on active AG */
        MpTwcAnswerCall();
        /* set the held call index such that it will correctly switch to the next
           call next time */
        theHeadset.HeldCallIndex = NoOfCallsAG1 + 1; 
    }
    /* if there are calls on both AG's and an incoming then answer it and hold other or
       if there are no incoming calls then cycle around the existing calls in order */
    else if(NoOfCallsAG1 && NoOfCallsAG2)
    {
        /* is there another incoming call to answer? */
        if((NoOfCallsAG1 && (CallStateAG2 == hfp_call_state_incoming))||
           (NoOfCallsAG2 && (CallStateAG1 == hfp_call_state_incoming))
          )
        {
            /* answer incoming call and hold others */            
            MpTwcAnswerCall();
        }
        /* or a call waiting on one of the AGs */        
        else if(CallStateAG1 == hfp_call_state_twc_incoming)
        {
            HfpCallHoldActionRequest(hfp_primary_link, hfp_chld_hold_active_accept_other, 0);           
            /* ensure the audio of answered call is being routed */
            if(getScoPriorityFromHfpPriority(hfp_primary_link) == sco_held_call)        
            {
                setScoPriorityFromHfpPriority(hfp_primary_link, sco_active_call);                     
                setScoPriorityFromHfpPriority(hfp_secondary_link, sco_held_call);                     
            }
        }
        /* or a call waiting on other AG */        
        else if(CallStateAG2 == hfp_call_state_twc_incoming)
        {
            HfpCallHoldActionRequest(hfp_secondary_link, hfp_chld_hold_active_accept_other, 0);
            /* ensure the audio of answered call is being routed */
            if(getScoPriorityFromHfpPriority(hfp_secondary_link) == sco_held_call)        
            {
                setScoPriorityFromHfpPriority(hfp_secondary_link, sco_active_call);                     
                setScoPriorityFromHfpPriority(hfp_primary_link, sco_held_call);                     
            }
        }
        /* no incoming call so cycle around existing calls */
        else
        {
            /* there are calls on both AG's, therefore cycle round the available calls via
               the call index pointer in the order of AG1 then AG2 and back to AG1 calls */
            /* swap to next call */
            theHeadset.HeldCallIndex ++;
            
            MP_DEBUG(("MP: HoldSwap multiple calls show %d\n",theHeadset.HeldCallIndex)) ;		

            /* is this on AG1 or AG2 or back to AG1 ?*/
            if(theHeadset.HeldCallIndex > NoOfCallsAG1)
            {
                /* is index still a call on AG2 ? */
                if(theHeadset.HeldCallIndex <= (NoOfCallsAG1 + NoOfCallsAG2))
                {                        
                    MP_DEBUG(("MP: HoldSwap call on AG2 \n")) ;		
                    /* if AG2 is not the routed audio then switch to it */
                    if(priority_audio_sink != hfp_secondary_link)
                    {
                        MP_DEBUG(("MP: HoldSwap swap audio routing to AG2 \n")) ;		
                        /* the headset is not currently routing the audio from this AG so swap over */
                        setScoPriorityFromHfpPriority(hfp_primary_link, sco_held_call);                    
                        setScoPriorityFromHfpPriority(hfp_secondary_link, sco_active_call);                    
                    }
                    /* audio already routed from this AG, switch calls if applicable */
                    else
                    {
                        /* if there are multiple calls on AG2 then swap between then */
                        MP_DEBUG(("MP: HoldSwap Send CHLD to AG2 \n")) ;		
                        HfpCallHoldActionRequest(hfp_secondary_link, hfp_chld_hold_active_accept_other, 0);
                    }
                }
                /* index higher than all calls so back to beginning */
                else
                {
                    MP_DEBUG(("MP: HoldSwap call on AG1 - rollover - show 0\n")) ;		
                    theHeadset.HeldCallIndex = 1;
                    /* if there were two calls on the secondary AG swap them again to maintain
                       the same scrolling order next time around */
                    if(NoOfCallsAG2 > 1)
                       HfpCallHoldActionRequest(hfp_secondary_link, hfp_chld_hold_active_accept_other, 0);                           
                    /* if there were two calls on the primary AG also swap them again to maintain
                       the same scrolling order next time around */
                    if(NoOfCallsAG1 > 1)
                       HfpCallHoldActionRequest(hfp_primary_link, hfp_chld_hold_active_accept_other, 0);                           
                    /* back to AG1, is this routed */
                    if(priority_audio_sink != hfp_primary_link)
                    {
                        MP_DEBUG(("MP: HoldSwap swap audio on AG 1 \n")) ;		
                        /* the headset is not currently routing the audio from this AG so swap over */
                        setScoPriorityFromHfpPriority(hfp_primary_link, sco_active_call);                    
                        setScoPriorityFromHfpPriority(hfp_secondary_link, sco_held_call);                                              
                    }
                }
            }
            /* index is a call on AG1 */
            else
            {
                MP_DEBUG(("MP: HoldSwap call on AG1 \n")) ;		
                /* if AG1 is not the routed audio then switch to it */
                if(priority_audio_sink != hfp_primary_link)
                {
                    MP_DEBUG(("MP: HoldSwap swap audio to AG1 \n")) ;		
                    /* the headset is not currently routing the audio from this AG so swap over */
                    setScoPriorityFromHfpPriority(hfp_primary_link, sco_active_call);                    
                    setScoPriorityFromHfpPriority(hfp_secondary_link, sco_held_call);                    
                }
                /* audio already routed from this AG, switch calls if applicable */
                else
                {
                    /* if there are multiple calls on AG2 then swap between then */
                    MP_DEBUG(("MP: HoldSwap send CHLD to AG1 \n")) ;		
                    HfpCallHoldActionRequest(hfp_primary_link, hfp_chld_hold_active_accept_other, 0);
                }
            }
        }
    }
    /* no other calls so this is request to hold incoming call */
    else if(priority)
    {
        MP_DEBUG(("MP: AccWaitHoldAct - no calls chld=2 = %d \n",priority)) ;		
        /* perform CHLD = 2 command to put incoming call on hold */
        if (theHeadset.conf->supp_features_local & HFP_THREE_WAY_CALLING)
            HfpCallHoldActionRequest(priority, hfp_chld_hold_active_accept_other, 0);                               
    }  
    
    /* redo audio routing */
    audioHandleRouting();
}

/****************************************************************************
NAME    
    mpHandleCallWaitingInd
    
DESCRIPTION
   determine whether a call waiting indication notification needs to be played
   if the audio of the AG is not currently being routed
   
RETURNS

*/
void mpHandleCallWaitingInd(HFP_CALL_WAITING_IND_T * pInd)
{
    MP_DEBUG(("MP: mpHandleCallWaitingInd from AG%d, sco is from AG%d\n",pInd->priority,HfpLinkPriorityFromAudioSink(theHeadset.sco_sink))) ;		
    /* determine if the indication of a call waiting has been sent from the AG which
       has its audio currently routed, if not it will not be heard and will need to be
       generated locally */
    if(pInd->priority != HfpLinkPriorityFromAudioSink(theHeadset.sco_sink))
    {
        /* generate event to play call waiting indication */
        MessageSend(&theHeadset.task,EventMultipointCallWaiting,0);        
    }    
}



/****************************************************************************
NAME    
    MpTwcAnswerCall
    
DESCRIPTION
   Called when the headset is in a twc operating state to answer a call on a 
   second AG. If the headset is currently routing audio, put this on hold ready to
   route the call once answered.
   
RETURNS

*/
void MpTwcAnswerCall(void)
{
    hfp_link_priority priority = hfp_invalid_link;

    
    /* get profile if AG with incoming call */
    priority = HfpLinkPriorityFromCallState(hfp_call_state_incoming);
    /* if no incoming calls check for presence of two active calls and
       swap between those two calls */
    if(priority)
    {
        /* determine whether headset has audio routed, if so hold this audio */
        if(theHeadset.sco_sink)
        {
            /* change audio sco priority of current active call, when incoming call is 
               answered audio will get routed automatically as it will have a higher priority */
            setScoPriorityFromHfpPriority(HfpLinkPriorityFromAudioSink(theHeadset.sco_sink), sco_held_call);                    
        }
        /* accept waiting call */
        HfpCallAnswerRequest(priority, TRUE);    
    }
}

/****************************************************************************
NAME    
    MpDropCall
    
DESCRIPTION
   performs a call drop on passed in AG depending upon call status, if the AG
   has a single call a CHUP command will be sent, if the AG has multiple calls
   the CHLD = 1, release active accept waiting will be sent 
   
RETURNS

*/
void MpDropCall(hfp_link_priority priority)
{
    hfp_call_state CallState;
                
    /* get call state AG to end call on */
    if(HfpLinkGetCallState(priority, &CallState))
    {
        /* if only one call send the chup command to end the call */
        if(CallState == hfp_call_state_active)
            HfpCallTerminateRequest(priority);    
        /* if two calls then send the accept waiting release active command */
        else if (CallState == hfp_call_state_held_active)
            HfpCallHoldActionRequest(priority, hfp_chld_release_active_accept_other, 0);
    }  
}

/****************************************************************************
NAME    
    MpHandleConferenceCall
    
DESCRIPTION
   can be passed true or false, when true a conference call will be created if
   an AG is found with 2 calls on it, one active and one held, when false an
   explicit call transfer will be performed if a conference call can be found, i.e.
   the AG is disconnected from the conference call 
   
RETURNS

*/
void MpHandleConferenceCall(bool create)
{
    hfp_link_priority priority = hfp_invalid_link;
    hfp_call_state CallState;
                
    /* determine if the AG which the user is currently listening to is capable of
       performing the conference call function */
    if((theHeadset.conf->supp_features_local & HFP_THREE_WAY_CALLING)&&(theHeadset.sco_sink))
    {
        /* obtain AG currently listening to */
        priority = HfpLinkPriorityFromAudioSink(theHeadset.sco_sink);
        
        /* get call state AG to end call on */
        if(HfpLinkGetCallState(priority, &CallState))
        {
            /* if trying to create a conference call and AG has two calls */
            if((create)&&(CallState == hfp_call_state_held_active))
                HfpCallHoldActionRequest(priority, hfp_chld_add_held_to_multiparty, 0);
            /* if trying to disconnect from a conference call and AG has a conference call */
            else if((!create)&&(CallState == hfp_call_state_multiparty))
                HfpCallHoldActionRequest(priority, hfp_chld_join_calls_and_hang_up, 0);
        }
    }
}
