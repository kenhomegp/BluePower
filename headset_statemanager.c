/****************************************************************************

FILE NAME
    headset_statemanager.h
    
DESCRIPTION
    state machine helper functions used for state changes etc - provide single 
    state change points etc for the headset app
*/

#include "headset_statemanager.h"
#include "headset_led_manager.h"
#include "headset_buttonmanager.h"
#include "headset_dut.h"
#include "headset_pio.h"

#include "headset_scan.h"
#include "headset_slc.h"

#include "headset_callmanager.h"

#include "headset_tones.h"

#ifdef ENABLE_AVRCP
#include "headset_avrcp.h"    
#endif 

#ifdef TEST_HARNESS
#include "test_headset.h"
#endif

#include <ps.h>
#include <stdlib.h>
#include <psu.h>

#ifdef DEBUG_STATES
#define SM_DEBUG(x) DEBUG(x)

const char * const gHSStateStrings [ HEADSET_NUM_STATES ] = {
                               "Limbo",
                               "Connectable",
                               "ConnDisc",
                               "Connected",
                               "Out",
                               "Inc",
                               "ActiveCallSCO",
                               "TESTMODE",
                               "TWCWaiting",
                               "TWCOnHold",
                               "TWMulticall",
                               "IncCallOnHold",
                               "ActiveNoSCO",
                               "headsetA2DPStreaming",
                               "headsetLowBattery"} ;
                               
#else
#define SM_DEBUG(x) 
#endif

#define SM_LIMBO_TIMEOUT_SECS 5

/****************************************************************************
VARIABLES
*/

    /*the headset state variable - accessed only from below fns*/
static headsetState gTheHeadsetState ;


/****************************************************************************
FUNCTIONS
*/

static void stateManagerSetState ( headsetState pNewState ) ;
static void stateManagerResetPIOs ( void ) ;
#ifdef EnablePhysicallyPowerOff
static void stateManagerPhysicallyPowerOff ( void ) ;
#endif

/****************************************************************************
NAME	
	stateManagerSetState

DESCRIPTION
	helper function to Set the current headset state
    provides a single state change point and passes the information
    on to the managers requiring state based responses
    
RETURNS
	
    
*/
static void stateManagerSetState ( headsetState pNewState )
{
	SM_DEBUG(("SM:[%s]->[%s][%d]\n",gHSStateStrings[stateManagerGetState()] , gHSStateStrings[pNewState] , pNewState ));
    
    if ( pNewState < HEADSET_NUM_STATES )
    {
        if (pNewState != gTheHeadsetState )
        {				
			#ifdef New_MMI
				if(pNewState == headsetA2DPStreaming)
				{
					theHeadset.BHC612_Chargefull = 0;
					PioSetPio(battery_low_io , PowerBlu_Connect);
					theHeadset.BHC612_BatteryLowState = PowerBlu_Connect;
				}
			#endif

			#if 1
				if((pNewState == headsetIncomingCallEstablish) || (pNewState == headsetOutgoingCallEstablish))
				{
					if((theHeadset.BHC612_DOCKMMI == 0) && (theHeadset.BHC612_UNDOCKMMI == 1))
					{
						LEDManagerIndicateState ( pNewState ) ;
					}
					else
					{						
						theHeadset.BHC612_StopChargingLED = 1;
					}
				}
				else
				{
					LEDManagerIndicateState ( pNewState ) ;
				}
			#else			
                /*inform the LED manager of the current state to be displayed*/
            	LEDManagerIndicateState ( pNewState ) ;
            #endif
        #ifdef TEST_HARNESS
            vm2host_send_state(pNewState);
        #endif
            
        }
        else
        {
            /*we are already indicating this state no need to re*/
        }
   
        gTheHeadsetState = pNewState ;
   
    }
    else
    {
        SM_DEBUG(("SM: ? [%s] [%x]\n",gHSStateStrings[ pNewState] , pNewState)) ;
    }
        
    /*if we are in chargererror then reset the leds and reset the error*/
    if (ChargerStatus() == DISABLED_ERROR)
    {
           /* Cancel current LED indication */
	   MessageSend(&theHeadset.task, EventCancelLedIndication, 0);	
	       /* Indicate charger error */
	   MessageSend(&theHeadset.task, EventChargeError, 0);
    }

}


/****************************************************************************
NAME	
	stateManagerGetState

DESCRIPTION
	helper function to get the current headset state

RETURNS
	the Headset State information
    
*/
headsetState stateManagerGetState ( void )
{
    headsetState lHeadsetState = gTheHeadsetState ;
    return lHeadsetState ;
}

/****************************************************************************
NAME	
	stateManagerGetPdlSize

DESCRIPTION
	helper function to get the size of the paired device list

RETURNS
	size of the paired device list

*/
uint16 stateManagerGetPdlSize ( void )
{
	uint16 lNumDevices = 0 ;
 	uint16 i = 0 ;	
	uint16 * ltdi = malloc( sizeof(uint16) * 8 );
	
	if (ltdi == NULL)
		return 0;

 	if (PsRetrieve ( 41 , ltdi , sizeof(uint16) * 8 ) )
 	{
 		SM_DEBUG(("SM: PDL PSR OK "));
		for (i = 0 ; i < 8 ; i ++ )
    	{	
    		SM_DEBUG(("[%d]" ,ltdi[i] )) ;
    	}	
    	SM_DEBUG(("\n")) ;
    		
    	/*get num devices in list*/	
    	for (i = 0 ; i < 8 ; i ++ )
    	{
    		if (ltdi[i] > lNumDevices )
    		{
    			lNumDevices = ltdi[i] ;
    			SM_DEBUG(("SM: Num D [%d]\n", lNumDevices)) ;				
    		}
    	}
	} 
    else
    {
    	SM_DEBUG(("SM: PS FAIL\n")) ;
    }
    free (ltdi);
	
	return lNumDevices;
}


/****************************************************************************
NAME	
	stateManagerEnterConnectableState

DESCRIPTION
	single point of entry for the connectable state - enters discoverable state 
    if configured to do so

RETURNS
	void
    
*/
void stateManagerEnterConnectableState ( bool req_disc )
{   
    headsetState lOldState = stateManagerGetState() ;
     
    if ( stateManagerIsConnected() && req_disc )
    {       /*then we have an SLC active*/
       headsetDisconnectAllSlc();
    }
        /*disable the ring PIOs if enabled*/
    stateManagerResetPIOs();
        /* Make the headset connectable */
    headsetEnableConnectable();
    stateManagerSetState ( headsetConnectable ) ;

	#ifdef New_MMI
	theHeadset.BHC612_TEMP = 0;
	theHeadset.PressCallButton = TRUE;
	theHeadset.DockLED = 0;
	/*theHeadset.BHC612_MPReconnect = false;*/
	theHeadset.BHC612_LinkLoss = false;
	theHeadset.BHC612_BattMeterOK = false;
	theHeadset.BHC612_LinkLossReconnect = false;
	theHeadset.BHC612_VPLinkLoss = 0;
	theHeadset.VoicePromptNotComplete = FALSE;
	theHeadset.BHC612_CallEnded = 0;
	#endif
    
        /*determine if we have got here after a DiscoverableTimeoutEvent*/
    if ( lOldState == headsetConnDiscoverable )
    {       /*disable the discoverable mode*/
        if (!theHeadset.features.RemainDiscoverableAtAllTimes)
        {
            headsetDisableDiscoverable();
        }
        MessageCancelAll ( &theHeadset.task , EventPairingFail ) ;        
    }
    else
    {
        /*if we want to auto enter pairing mode*/
        if ( theHeadset.features.pair_mode_en )
        {    
            stateManagerEnterConnDiscoverableState();
        }  
        SM_DEBUG(("SM: Disco %X RSSI %X\n", theHeadset.features.DiscoIfPDLLessThan, theHeadset.features.PairIfPDLLessThan));
        /* If any of the other action on power on features are enabled... */
        if (theHeadset.features.DiscoIfPDLLessThan)
        {
            /* Get the PDL size */
        	uint16 lNumDevices = stateManagerGetPdlSize();
    		SM_DEBUG(("SM: Num Devs %d\n",lNumDevices));
            /* Check if we want to go discoverable */
    		if ( lNumDevices < theHeadset.features.DiscoIfPDLLessThan )
    		{
    			SM_DEBUG(("SM: NumD [%d] <= DiscoD [%d]\n" , lNumDevices , theHeadset.features.DiscoIfPDLLessThan))
    		   	stateManagerEnterConnDiscoverableState();
    		}
    	}	
	}
}

/****************************************************************************
NAME	
	stateManagerEnterConnDiscoverableState

DESCRIPTION
	single point of entry for the connectable / discoverable state 
    uses timeout if configured
RETURNS
	void
    
*/
void stateManagerEnterConnDiscoverableState ( void )
{
    if(theHeadset.features.DoNotDiscoDuringLinkLoss && HfpLinkLoss())
    {
        /*if we are in link loss and do not want to go discoverable during link loss then ignore*/                    
    }
    else
    {    
        if ( stateManagerIsConnected() )
        {
                /*then we have an SLC active*/
           headsetDisconnectAllSlc();
        }  
     
        /* Make the headset connectable */
        headsetEnableConnectable();
    
        /* Make the headset discoverable */  
        headsetEnableDiscoverable();    
        
        /* If there is a timeout - send a user message*/
		if ( theHeadset.conf->timeouts.PairModeTimeoutIfPDL_s != 0 )
		{
			/* if there are no entries in the PDL, then use the second
			   pairing timer */
			uint16 lNumDevices = stateManagerGetPdlSize();
			if( lNumDevices != 0)
			{	/* paired devices in list, use original timer if it exists */
				if( theHeadset.conf->timeouts.PairModeTimeout_s != 0 )
				{
					SM_DEBUG(("SM : Pair [%x]\n" , theHeadset.conf->timeouts.PairModeTimeout_s )) ;
					MessageSendLater ( &theHeadset.task , EventPairingFail , 0 , D_SEC(theHeadset.conf->timeouts.PairModeTimeout_s) ) ;
				}
			}
			else
			{	/* no paired devices in list, use secondary timer */
	            SM_DEBUG(("SM : Pair (no PDL) [%x]\n" , theHeadset.conf->timeouts.PairModeTimeoutIfPDL_s )) ;
				MessageSendLater ( &theHeadset.task , EventPairingFail , 0 , D_SEC(theHeadset.conf->timeouts.PairModeTimeoutIfPDL_s) ) ;
			}			            			
		}
        else if ( theHeadset.conf->timeouts.PairModeTimeout_s != 0 )
        {
            SM_DEBUG(("SM : Pair [%x]\n" , theHeadset.conf->timeouts.PairModeTimeout_s )) ;
            
            MessageSendLater ( &theHeadset.task , EventPairingFail , 0 , D_SEC(theHeadset.conf->timeouts.PairModeTimeout_s) ) ;
        }
        else
        {
            SM_DEBUG(("SM : Pair Indefinetely\n")) ;
        }
        /* Disable the ring PIOs if enabled*/
        stateManagerResetPIOs();
       
    	/* The headset is now in the connectable/discoverable state */
        stateManagerSetState(headsetConnDiscoverable);
    }
}


/****************************************************************************
NAME	
	stateManagerEnterConnectedState

DESCRIPTION
	single point of entry for the connected state - disables disco / connectable modes
RETURNS
	void
    
*/
void stateManagerEnterConnectedState ( void )
{
	#ifdef New_MMI
	bool Voiceprompt = FALSE;
	#endif
	
    if (stateManagerGetState () != headsetConnected )
    {
            /*make sure we are now neither connectable or discoverable*/
        SM_DEBUG(("SM:Remain in Disco Mode [%c]\n", (theHeadset.features.RemainDiscoverableAtAllTimes?'T':'F') )) ;
        
        if (!theHeadset.features.RemainDiscoverableAtAllTimes)
        {
            headsetDisableDiscoverable();    
        }
        
        /* for multipoint connections need to remain connectable after 1 device connected */
        if(!theHeadset.MultipointEnable)
        {
            headsetDisableConnectable();        
        }
        else
        {
            /* if both profiles are now connected disable connectable mode */
            if(theHeadset.conf->no_of_profiles_connected == MAX_MULTIPOINT_CONNECTIONS)
            {
                /* two devices connected */
                headsetDisableConnectable();                    
            }
            else
            {
                /* remain connectable for a further 'x' seconds to allow a second 
                   AG to be connected if non-zero, otherwise stay connecatable forever */
                if(theHeadset.conf->timeouts.ConnectableTimeout_s)
                {
                    MessageSendLater(&theHeadset.task, EventConnectableTimeout, 0, D_SEC(theHeadset.conf->timeouts.ConnectableTimeout_s));
                }
            }
        }
    
        switch ( stateManagerGetState() )
        {    
            case headsetIncomingCallEstablish:
                if (theHeadset.conf->timeouts.MissedCallIndicateTime_s != 0 && 
                    theHeadset.conf->timeouts.MissedCallIndicateAttemps != 0)
                {
                    theHeadset.MissedCallIndicated = theHeadset.conf->timeouts.MissedCallIndicateAttemps;
                    
                    MessageSend   (&theHeadset.task , EventMissedCall , 0 ) ; 
                }
            case headsetActiveCallSCO:            
            case headsetActiveCallNoSCO:       
            case headsetThreeWayCallWaiting:
            case headsetThreeWayCallOnHold:
            case headsetThreeWayMulticall:
            case headsetOutgoingCallEstablish:
                    /*then we have just ended a call*/
				#ifdef New_MMI
					Voiceprompt = TRUE;
				#else
                	MessageSend ( &theHeadset.task , EventEndOfCall , 0 ) ;
				#endif
            break ;
            default:
            break ;
        }
    
	
        MessageCancelAll ( &theHeadset.task , EventPairingFail ) ;
        
   		/*disable the ring PIOs if enabled*/
        stateManagerResetPIOs();
    
        /* when returning to connected state, check for the prescence of any A2DP instances
           if found enter the appropriate state */
        if((theHeadset.a2dp_link_data->connected[a2dp_primary])||(theHeadset.a2dp_link_data->connected[a2dp_secondary]))
        {
            SM_DEBUG(("SM:A2dp connected\n")) ;
            /* are there any A2DP instances streaming audio? */
            if((A2dpMediaGetState(theHeadset.a2dp_link_data->device_id[a2dp_primary], 
				theHeadset.a2dp_link_data->stream_id[a2dp_primary]) == a2dp_stream_streaming)||
               (A2dpMediaGetState(theHeadset.a2dp_link_data->device_id[a2dp_secondary], 
               theHeadset.a2dp_link_data->stream_id[a2dp_secondary]) == a2dp_stream_streaming))
            {
                SM_DEBUG(("SM:A2dp connected - still streaming\n")) ;
                stateManagerSetState( headsetA2DPStreaming );
				theHeadset.PressCallButton = TRUE;
				MessageCancelAll(&theHeadset.task, EventEndOfCall);
            }
            else
            {
				SM_DEBUG(("SM:A2DP connected, not streaming\n"));

				if(Voiceprompt)
				{
					MessageSend ( &theHeadset.task , EventEndOfCall , 0 ) ;
				}
				
                stateManagerSetState( headsetConnected );

				#ifdef New_MMI
				theHeadset.BHC612_StopChargingLED = 0;
				#endif

				#if 0
				if(theHeadset.VoicePromptNotComplete)
				{
					theHeadset.VoicePromptNotComplete = FALSE;
					MessageSend ( &theHeadset.task , EventEndOfCall , 0 ) ;
					SM_DEBUG(("SM:End of call!\n"));
				}
				#endif
            }
        }
        /* no A2DP connections, go to connected state */
        else 
        {
            if(Voiceprompt)
            {
				MessageSend ( &theHeadset.task , EventEndOfCall , 0 ) ;
            }
			
            stateManagerSetState( headsetConnected );

			#ifdef New_MMI
			theHeadset.BHC612_StopChargingLED = 0;
			#endif
        }

   }     
}
/****************************************************************************
NAME	
	stateManagerEnterIncomingCallEstablishState

DESCRIPTION
	single point of entry for the incoming call establish state
RETURNS
	void
    
*/
void stateManagerEnterIncomingCallEstablishState ( void )
{
	if(gTheHeadsetState == headsetConnected)
		theHeadset.BHC612_CallEnded = 1;
	else
		theHeadset.BHC612_CallEnded = 0;
   
    stateManagerSetState( headsetIncomingCallEstablish );
        
    	/*if we enter this state directly*/
    MessageCancelAll ( &theHeadset.task , EventPairingFail ) ;
		
    if (theHeadset.PIO->IncomingRingPIO != 0xF )
    {
        PioSetPio ( theHeadset.PIO->IncomingRingPIO , TRUE) ;
    }	

	#ifdef New_MMI	/*TEST!!!*/
	headsetCheckForAudioTransfer();
	#endif
}

/****************************************************************************
NAME	
	stateManagerEnterOutgoingCallEstablishState

DESCRIPTION
	single point of entry for the outgoing call establish state
RETURNS
	void
    
*/
void stateManagerEnterOutgoingCallEstablishState ( void )
{
	if(gTheHeadsetState == headsetConnected)
		theHeadset.BHC612_CallEnded = 1;
	else
		theHeadset.BHC612_CallEnded = 0;

    stateManagerSetState( headsetOutgoingCallEstablish );
    
    	/*if we enter this state directly*/
    MessageCancelAll ( &theHeadset.task , EventPairingFail ) ;
	
    if (theHeadset.PIO->OutgoingRingPIO != 0xF )
    {
        PioSetPio ( theHeadset.PIO->OutgoingRingPIO , TRUE) ;
    }
}
/****************************************************************************
NAME	
	stateManagerEnterActiveCallState

DESCRIPTION
	single point of entry for the active call state
RETURNS
	void
    
*/
void stateManagerEnterActiveCallState ( void )   
{
	if((stateManagerGetState() == headsetOutgoingCallEstablish) ||
	   (stateManagerGetState() == headsetIncomingCallEstablish))
	{
		/* If a call is being answered then send call answered event */
		MessageSend ( &theHeadset.task , EventCallAnswered , 0 ) ;
	}

	#ifdef CTIA_Test
	if((gTheHeadsetState == headsetConnected))
	{
		theHeadset.BHC612_CallEnded = 1;
	}
	#endif
	
    if (theHeadset.sco_sink)
    {	
        stateManagerSetState(  headsetActiveCallSCO );
    }
    else
    {
        stateManagerSetState( headsetActiveCallNoSCO );
    }
	
    	/*disable the ring PIOs if enabled*/
    if (theHeadset.PIO->IncomingRingPIO !=0xF )
    {
        PioSetPio ( theHeadset.PIO->IncomingRingPIO , FALSE) ;
    }
    if ( theHeadset.PIO->OutgoingRingPIO !=0xF )
    {
        PioSetPio ( theHeadset.PIO->OutgoingRingPIO , FALSE) ;
    }    

        /*enable the active call PIO if there is one*/
    if ( theHeadset.PIO->CallActivePIO !=0xF )
    {
        PioSetPio ( theHeadset.PIO->CallActivePIO , TRUE) ;
    }  
	
    	/*if we enter this state directly*/
    MessageCancelAll ( &theHeadset.task , EventPairingFail ) ;

}


/****************************************************************************
NAME	
	stateManagerEnterA2dpStreamingState

DESCRIPTION
    enter A2DP streaming state if not showing any active call states
RETURNS
	void
    
*/
void stateManagerEnterA2dpStreamingState(void)
{
	SM_DEBUG(("SM:EnterA2DPStreaming!!!\n"));
    /* only allow change to A2DP connected state if not currenltly showing 
       any active call states */
    if(stateManagerGetState() == headsetConnected) 
	{
        stateManagerSetState(  headsetA2DPStreaming );
	}
}

#ifdef EnablePhysicallyPowerOff
/****************************************************************************
NAME	
	stateManagerPhysicallyPowerOff

DESCRIPTION
	actually power down the device
RETURNS
	void
    
*/
static void stateManagerPhysicallyPowerOff ( void ) 
{
    SM_DEBUG(("SM : Power Off\n--goodbye--\n")) ;
    
        /*used as a power on indication if required*/
    if ( theHeadset.PIO->PowerOnPIO !=0xF)
    {
        PioSetPio ( theHeadset.PIO->PowerOnPIO , FALSE) ;
    }

    PioSetPowerPin   (  FALSE ) ;
    
}
#endif

/****************************************************************************
NAME	
	stateManagerPowerOn

DESCRIPTION
	Power on the deviece by latching on the power regs
RETURNS
	void
    
*/
void stateManagerPowerOn( void ) 
{
    SM_DEBUG(("--hello--\nSM : PowerOn\n"));

	/* Check for DUT mode enable */
	if(!checkForDUTModeEntry())
	{
        /*cancel the event power on message if there was one*/
    	MessageCancelAll ( &theHeadset.task , EventLimboTimeout ) ;
            
    	PioSetPowerPin ( TRUE ) ;

    	if ( theHeadset.PIO->PowerOnPIO !=0xF )
    	{
        	PioSetPio ( theHeadset.PIO->PowerOnPIO , TRUE) ;
    	}
		
		/* Reset sec mode config - always turn off debug keys on power on! */
		ConnectionSmSecModeConfig(&theHeadset.task, cl_sm_wae_acl_none, FALSE, TRUE);

		#ifdef T3ProductionTest
			if(theHeadset.ProductionData == 2)/*T2*/
			{
				MessageSendLater (&theHeadset.task , EventEnterDFUMode , 0, 3000 );
				return;
			}
		#endif
  
		stateManagerEnterConnectableState( TRUE );
        
        if(theHeadset.features.PairIfPDLLessThan || theHeadset.features.AutoReconnectPowerOn)
        {
            uint16 lNumDevices = stateManagerGetPdlSize();
        
            /* Check if we want to start RSSI pairing */
            if( lNumDevices < theHeadset.features.PairIfPDLLessThan )
            {
                SM_DEBUG(("SM: NumD [%d] <= PairD [%d]\n" , lNumDevices , theHeadset.features.PairIfPDLLessThan))
                stateManagerEnterConnDiscoverableState();
                MessageSend(&theHeadset.task, EventRssiPair, 0);
            }
            else if ((theHeadset.features.AutoReconnectPowerOn)||(theHeadset.features.AutoA2dpReconnectPowerOn))
            {
                SM_DEBUG(("SM: Auto Reconnect, PDL = %d\n",lNumDevices)) ;
#ifdef ENABLE_AVRCP
                headsetAvrcpCheckManualConnectReset(NULL);
#endif                
				
				#ifdef BHC612
					if(lNumDevices != 0)
						MessageSend ( &theHeadset.task , EventEstablishSLC , 0 ) ;
				#else
		                MessageSend ( &theHeadset.task , EventEstablishSLC , 0 ) ;
				#endif
            }
        }
	}
}

/****************************************************************************
NAME	
	stateManagerIsConnected

DESCRIPTION
    Helper method to see if we are connected or not   
*/
bool stateManagerIsConnected ( void )
{
    bool lIsConnected = FALSE ;
    
    switch (stateManagerGetState() )
    {
        case headsetLimbo:
        case headsetConnectable:
        case headsetConnDiscoverable:
        case headsetTestMode:
            lIsConnected = FALSE ;    
        break ;
        
        default:
            lIsConnected = TRUE ;
        break ;
    }
    return lIsConnected ;
}

/****************************************************************************
NAME	
	stateManagerEnterLimboState

DESCRIPTION
    method to provide a single point of entry to the limbo /poweringOn state
RETURNS	
    
*/
void stateManagerEnterLimboState ( void )
{
    uint8 index;
    
    SM_DEBUG(("SM: Enter Limbo State[%d]\n" , theHeadset.conf->timeouts.AutoPowerOnTimeout_s)); 
      
    /*Cancel inquiry if in progress*/
    slcStopInquiry();

    /* Disconnect any slc's and cancel any further connection attempts including link loss */
    MessageCancelAll(&theHeadset.task, EventContinueSlcConnectRequest);
    MessageCancelAll(&theHeadset.task, EventLinkLoss);
    headsetDisconnectAllSlc();
    
#ifdef ENABLE_AVRCP
    headsetAvrcpDisconnectAll();
#endif      
    
    /* disconnect any a2dp signalling channels */
    for(index = a2dp_primary; index < (a2dp_secondary+1); index++)
    {
        /* is a2dp connected? */
        if(theHeadset.a2dp_link_data->connected[index])
        {
            SM_DEBUG(("SM: Enter Limbo State - disc A2DP ID=%x\n",index)); 
            /* disconnect signalling channel */
            A2dpSignallingDisconnectRequest(theHeadset.a2dp_link_data->device_id[index]);
        }
    }  
    
    /* reset the pdl list indexes in preparation for next boot */
    slcReset();
    
        /*in limbo, the internal regs must be latched on (smps and LDO)*/
	#if 0	
    PioSetPowerPin ( TRUE ) ;
	#endif

        /*make sure we are neither connectable or discoverable*/
    headsetDisableDiscoverable();    
    headsetDisableConnectable();  

    SM_DEBUG(("Limbo Timeout = %d\n",theHeadset.conf->timeouts.AutoPowerOnTimeout_s))	;

        /*set a timeout so that we will turn off eventually anyway*/
    MessageSendLater ( &theHeadset.task , EventLimboTimeout , 0 , D_SEC(theHeadset.conf->timeouts.AutoPowerOnTimeout_s) ) ;

    stateManagerSetState( headsetLimbo );
}

/****************************************************************************
NAME	
	stateManagerUpdateLimboState

DESCRIPTION
    method to update the limbo state and power off when necessary
RETURNS	
    
*/
void stateManagerUpdateLimboState ( void ) 
{
        /*then we are entering here as a result of a power off*/
        if( !ChargerIsChargerConnected() )/*ChargerStatus = NO_POWER*/
        {
            /*power has been removed and we are logically off so switch off*/
            SM_DEBUG(("SM: LimboDiscon\n")) ;

		 	#ifdef BHC612
	     			SM_DEBUG(("*****headset : PhysicallyPowerOff*****\n")) ;
				#ifdef PSBlueLED 
		 		PioSetPio(PS_blue_led, PS_blue_led_off);
				#endif
	     		#endif

			#ifdef EnablePhysicallyPowerOff
				if(PsuGetVregEn() == 0)	/*Vreg detect Low : PowerOff*/
				{
            				stateManagerPhysicallyPowerOff();
				}
			#endif

			#ifdef CriticalBatteryPowerOff
				if(stateManagerGetState() == headsetLowBattery)
					stateManagerPhysicallyPowerOff();
			#endif
        }
        else
        {
            /*this means connected*/
            SM_DEBUG(("SM: LimboConn\n")) ;

		 	#ifdef BHC612
				#ifdef PSBlueLED
		 		PioSetPio(PS_blue_led, PS_blue_led_off);
				#endif
		 		/*stateManagerEnterConnectableState(FALSE);*/
				/*stateManagerPhysicallyPowerOff();*/
	     	#endif
			
            /*stay in this state until a charger event or a power on occurs*/
        }  
}

/****************************************************************************
NAME	
	stateManagerEnterTestModeState

DESCRIPTION
    method to provide a single point of entry to the test mode state
RETURNS	
    
*/
void stateManagerEnterTestModeState ( void )
{
    stateManagerSetState( headsetTestMode );
}

/****************************************************************************
NAME	
	stateManagerEnterCallWaitingState

DESCRIPTION
    method to provide a single point of entry to the 3 way call waiting state
RETURNS	
    
*/
void stateManagerEnterThreeWayCallWaitingState ( void ) 
{
    stateManagerSetState( headsetThreeWayCallWaiting );
	
    	/*if we enter this state directly*/
    MessageCancelAll ( &theHeadset.task , EventPairingFail ) ;
    
    /* if a single AG is connected with multipoint enabled and hs is in twc call waiting, disable
       connectable to prevent a second AG from connecting */
    if((theHeadset.MultipointEnable)&&(theHeadset.conf->no_of_profiles_connected < MAX_MULTIPOINT_CONNECTIONS))
    {
        /* make hs no longer connectable */
        headsetDisableConnectable();        
    }
}


void stateManagerEnterThreeWayCallOnHoldState ( void ) 
{   
    stateManagerSetState( headsetThreeWayCallOnHold );
	
    	/*if we enter this state directly*/
    MessageCancelAll ( &theHeadset.task , EventPairingFail ) ;
}

void stateManagerEnterThreeWayMulticallState ( void ) 
{
    stateManagerSetState( headsetThreeWayMulticall );
	
    	/*if we enter this state directly*/
    MessageCancelAll ( &theHeadset.task , EventPairingFail ) ;
}


void stateManagerEnterIncomingCallOnHoldState ( void )
{
    stateManagerSetState( headsetIncomingCallOnHold );
	
    	/*if we enter this state directly*/
    MessageCancelAll ( &theHeadset.task , EventPairingFail ) ;
}

static void stateManagerResetPIOs ( void )
{
        /*disable the ring PIOs if enabled*/
    if ( theHeadset.PIO->IncomingRingPIO !=0xF )
    {
        PioSetPio ( theHeadset.PIO->IncomingRingPIO , FALSE) ;
    }
    if ( theHeadset.PIO->OutgoingRingPIO != 0xF )
    {
        PioSetPio ( theHeadset.PIO->OutgoingRingPIO , FALSE) ;
    }    
        /*diaable the active call PIO if there is one*/
    if ( theHeadset.PIO->CallActivePIO !=0xF)
    {
        PioSetPio ( theHeadset.PIO->CallActivePIO , FALSE) ;
    }     
}

