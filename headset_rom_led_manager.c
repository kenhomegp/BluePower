/****************************************************************************

FILE NAME
    headset_LEDmanager.c
    
DESCRIPTION
    Module responsible for managing the PIO outputs including LEDs
    
*/

#ifdef ROM_LEDS
/****************************************************************************
INCLUDES
*/
#include "headset_led_manager.h"
#include "headset_private.h"


#include "headset_configmanager.h"
#include "headset_statemanager.h"
#include "headset_leds.h"
#include "headset_leddata.h"
#include "headset_pio.h"
#include "headset_powermanager.h"

#include <stddef.h>
#include <pio.h>
#include <stdlib.h>
#include <string.h>


#ifdef BHC612
	#include "headset_dut.h"
#endif


/****************************************************************************
DEFINITIONS
*/


#ifdef DEBUG_LM
#define LM_DEBUG(x) {printf x;}
#else
#define LM_DEBUG(x) 
#endif

#ifdef BHC612
#ifdef MyDEBUG_LM
/*
#define MYLM_DEBUG(x) {printf x;}
*/
#define MYLM_DEBUG(x)
#define MYLM_DEBUG1(x) {printf x;}
#else
#define MYLM_DEBUG(x) 
#define MYLM_DEBUG1(x) 
#endif

#endif

/***************************************************************************/

#ifdef New_MMI
static const uint8 PatchStateLEDTable[15][3] =
{
       /* {headsetState , PatchColor , Enable } */
	{0x00,0x00,0x00},									/*headsetLimbo*/
	{0x01,LED_COL_LED_BTIDLE,0x00}, 					/*headsetConnectable*/
	{0x02,LED_COL_LED_PAIRING,0x01}, 				/*headsetConnDiscoverable*/
	{0x03,LED_COL_LED_BTCONNECTED,0x00}, 			/*headsetConnected*/
	{0x04,LED_COL_LED_OUTGOINGCALL,0x00}, 			/*headsetOutgoingCallEstablish*/
	{0x05,LED_COL_LED_INCOMINGCALL,0x00}, 			/*headsetIncomingCallEstablish*/
	{0x06,0x00,0x00}, 								/*headsetActiveCallSCO*/
	{0x07,0x00,0x00}, 								/*headsetTestMode*/
	{0x08,0x00,0x00}, 								/*headsetThreeWayCallWaiting*/
	{0x09,0x00,0x00}, 								/*headsetThreeWayCallOnHold*/
	{0x0a,0x00,0x00},									/*headsetThreeWayMulticall*/
	{0x0b,0x00,0x00}, 								/*headsetIncomingCallOnHold*/
	{0x0c,0x00,0x00}, 								/*headsetActiveCallNoSCO*/
	{0x0d,0x00,0x00}, 								/*headsetA2DPStreaming*/
	{0x0e,0x00,0x00} 									/*headsetLowBattery*/
};

static const uint8 PatchEventLEDTable[16][2] =
{
	/* { EventIndex , PatchColor } */
	#ifdef New_MMI
		#ifdef EnablePowerOnLED
		{0x01,LED_COL_LED_POWERON}, 			/* Power on , Event number : 0x6001*/
		{0x02,LED_COL_LED_POWEROFF},			/* Power off , Event number : 0x6002*/
		#else
		{0x01,0x00},
		{0x02,0x00},
		#endif
	#endif
	{0x1a,0x00},
	{0x41,0x00},
	{0x42,0x00},
	{0x43,0x00},
	{0x44,0x00},
	{0x00,0x00},
	{0x00,0x00},
	{0x00,0x00},
	{0x00,0x00},
	{0x00,0x00},
	{0x00,0x00},
	{0x00,0x00},
	{0x00,0x00},
	{0x00,0x00}
};
#endif

/****************************************************************************
LOCAL FUNCTION PROTOTYPES
*/


 /*methods to allocate/ initialise the space for the patterns and mappings*/
static void LEDManagerInitStatePatterns   ( void ) ;
static void LEDManagerInitEventPatterns   ( void ) ;
static void LEDManagerInitActiveLEDS      ( void ) ;
static void LEDManagerCreateFilterPatterns( void ) ;

/****************************************************************************
NAME	
	ledManagerMemoryInit

DESCRIPTION
	Initialise memory for led manager, this has moved from theHeadset as ran 
    out of globals space.

RETURNS
	void
    
*/
void LedManagerMemoryInit(void) 
{
	/* Allocate memory to hold the led manager states */
	theHeadset.theLEDTask = mallocPanic(sizeof(LedTaskData));
    theHeadset.theLEDTask->gActiveLEDS = (LEDActivity_t *)mallocPanic(sizeof(LEDActivity_t) * HEADSET_NUM_LEDS);
    memset(theHeadset.theLEDTask->gActiveLEDS, 0, (sizeof(LEDActivity_t) * HEADSET_NUM_LEDS));
}

/****************************************************************************
NAME 
 PIOManagerInit

DESCRIPTION
 Initialises LED manager

RETURNS
 void
    
*/
void LEDManagerInit ( void ) 
{
        
    LM_DEBUG(("LM Init :\n")) ;
   
    /* reset the number of allocated pattern positions */
    theHeadset.theLEDTask->gPatternsAllocated = 0;
                
    LM_DEBUG(("LM : p[%x][%x][%x]\n" ,  (int)theHeadset.theLEDTask->gStatePatterns ,
                                        (int)theHeadset.theLEDTask->gEventPatterns ,
                                        (int)theHeadset.theLEDTask->gActiveLEDS    
            )) ;
    
    /*create the patterns we want to use*/
    LEDManagerInitStatePatterns ( ) ;
    LEDManagerInitActiveLEDS( ) ;
    LEDManagerInitEventPatterns( ) ;
    
    theHeadset.theLEDTask->Queue.Event1 = 0 ;
    theHeadset.theLEDTask->Queue.Event2 = 0 ;
    theHeadset.theLEDTask->Queue.Event3 = 0 ;
    theHeadset.theLEDTask->Queue.Event4 = 0 ;
    
    /*the filter information*/
    LEDManagerCreateFilterPatterns( ) ;
    
    LedsInit ( ) ;

}
/****************************************************************************
NAME 
 LEDManagerInitActiveLEDS

DESCRIPTION
 Creates the active LED space for the number of leds the system supports

RETURNS
 void
    
*/
static void LEDManagerInitActiveLEDS ( void ) 
{
    uint16 lIndex = 0; 
 
    for ( lIndex= 0 ; lIndex < HEADSET_NUM_LEDS ; lIndex ++ )
    {
        LedsSetLedActivity ( &theHeadset.theLEDTask->gActiveLEDS [ lIndex ] , IT_Undefined , 0 , 0 ) ;    
    }
}
/****************************************************************************
NAME 
 LEDManagerInitStatePatterns

DESCRIPTION
 Creates the state patterns space for the system states

RETURNS
 void
    
*/
static void LEDManagerInitStatePatterns ( void ) 
{
    uint16 lIndex = 0; 
 
    for ( lIndex= 0 ; lIndex < HEADSET_NUM_STATES ; lIndex ++ )
    {
        LEDPattern_t *lStatePattern = &(theHeadset.theLEDTask->gStatePatterns[lIndex]);
        
        memset(lStatePattern, 0, sizeof(LEDPattern_t));
        lStatePattern->Colour     = LED_COL_LED_A ;  
    }
     
}
/****************************************************************************
NAME 
 LEDManagerInitEventPatterns

DESCRIPTION
 inits the Event pattern pointers

RETURNS
 void
    
*/
static void LEDManagerInitEventPatterns ( void )
{
    uint16 lIndex = 0; 
 
    for ( lIndex= 0 ; lIndex < LM_MAX_NUM_PATTERNS ; lIndex ++ )
    {
        LEDPattern_t *lEventPattern = &(theHeadset.theLEDTask->gEventPatterns[lIndex]);
        
        memset(lEventPattern, 0, sizeof(LEDPattern_t));
        lEventPattern->Colour     = LED_COL_LED_A ;  
    } 
}
/****************************************************************************
NAME 
 LEDManagerCreateFilterPatterns

DESCRIPTION
 Creates the Filter patterns space 

RETURNS
 void
    
*/
static void LEDManagerCreateFilterPatterns ( void )
{
    uint16 lIndex = 0 ;
    

    for (lIndex = 0 ; lIndex < LM_NUM_FILTER_EVENTS ; lIndex++ )
    {
        LEDFilter_t *lEventFilter = &(theHeadset.theLEDTask->gEventFilters [ lIndex ]);
        
        memset(lEventFilter, 0, sizeof(LEDFilter_t));
    }
    
    theHeadset.theLEDTask->gLMNumFiltersUsed = 0 ;

    theHeadset.theLEDTask->gTheActiveFilters = 0x0000 ;
}
 

#ifdef DEBUG_LM
/****************************************************************************
NAME 
 LMPrintPattern

DESCRIPTION
    debug fn to output a LED pattern
    
RETURNS
 void
*/

void LMPrintPattern ( LEDPattern_t * pLED ) 
{
#ifdef DEBUG_PRINT_ENABLED
    const char * const lColStrings [ 5 ] =   {"LED_E ","LED_A","LED_B","ALT","Both"} ;
    if(pLED)
    {
        LM_DEBUG(("[%d][%d] [%d][%d][%d] ", pLED->LED_A , pLED->LED_B, pLED->OnTime ,pLED->OffTime ,pLED->RepeatTime)) ;  
        LM_DEBUG(("[%d] [%d] [%s]\n",       pLED->NumFlashes, pLED->TimeOut, lColStrings[pLED->Colour])) ;    
        LM_DEBUG(("[%d]\n",       pLED->OverideDisable)) ;    
    }
    else
    {
        LM_DEBUG(("LMPrintPattern = NULL \n")) ;  
    }
#endif

}
#endif

/****************************************************************************
NAME 
 LEDManagerIndicateEvent

DESCRIPTION
 displays event notification
    This function also enables / disables the event filter actions - if a normal event indication is not
    associated with the event, it checks to see if a filer is set up for the event 

RETURNS
 void
    
*/

void LEDManagerIndicateEvent ( MessageId pEvent ) 
{
#ifdef New_MMI
	uint8 i,j,lPatternIndex;
#else
	uint8 i,lPatternIndex;
#endif

    uint16 lEventIndex = pEvent - EVENTS_MESSAGE_BASE ;
    LEDPattern_t * lPattern = NULL;
    
    lPatternIndex = NO_STATE_OR_EVENT;
    LM_DEBUG(("LM IndicateEvent [%x]\n", lEventIndex)) ;   
    MYLM_DEBUG(("LM IndicateEvent [%x]\n", lEventIndex)) ; 
    
    /* search for a matching event */
    for(i=0;i<theHeadset.theLEDTask->gEventPatternsAllocated;i++)
    {
        if(theHeadset.theLEDTask->gEventPatterns[i].StateOrEvent == lEventIndex)
        {
            lPatternIndex = i;
            lPattern      = &theHeadset.theLEDTask->gEventPatterns[i];
            break;
        }
    }

    /*if there is an event configured*/
    if ( lPatternIndex != NO_STATE_OR_EVENT )
    {    
        /*only indicate if LEDs are enabled*/
        if ((theHeadset.theLEDTask->gLEDSEnabled ) ||
            (lPattern->OverideDisable) ||
            LedActiveFiltersCanOverideDisable( ))
        {
    
            LM_DEBUG(("LM : IE[%x]\n",pEvent )) ;

		#ifdef New_MMI
			if(lEventIndex == EventPbapDialFail) /* EventPbapDialFai : 0x6089 */
				return;
			
			for(i = 0 ; i < 16 ; i++)
			{
				for(j = 0 ; j < 2 ; j++)
				{
					if(lEventIndex == PatchEventLEDTable[i][j] && j == 0)
					{
						/*if(theHeadset.theLEDTask->PatchEventColor != LED_COL_LED_POWERON)*/
						if(theHeadset.theLEDTask->PatchEventColor != LED_COL_LED_POWERON && theHeadset.theLEDTask->PatchEventColor != LED_COL_LED_POWEROFF)
						{
							theHeadset.theLEDTask->PatchEventColor = PatchEventLEDTable[i][j+1];
							theHeadset.theLEDTask->PatchStateColor = LED_COL_UNDEFINED;
							MYLM_DEBUG(("Event Pattern index = %d\n",lPatternIndex));
							MYLM_DEBUG(("LED On = %d\n",lPattern->OnTime));	
							MYLM_DEBUG(("LED Off = %d\n",lPattern->OffTime));
							MYLM_DEBUG(("LED Repeat = %d\n",lPattern->RepeatTime));
							MYLM_DEBUG(("LED Numflash = %d\n",lPattern->NumFlashes));
							break;
						}
						else
							return;
					}		
				}
			}		
		#endif
 
            /*only update if wer are not currently indicating an event*/
            if ( ! theHeadset.theLEDTask->gCurrentlyIndicatingEvent )
            {
            	LM_DEBUG(("LM : ledsIndicateLedsPattern \n")) ;
                ledsIndicateLedsPattern(lPattern, lPatternIndex, IT_EventIndication);
            }    
            else
            {
                if (theHeadset.features.QueueLEDEvents )
                {
                    /*try and add it to the queue*/
                    LM_DEBUG(("LM: Queue LED Event [%x]\n" , pEvent )) ;
					
					if ( theHeadset.theLEDTask->Queue.Event1 == 0)
                    {
                        theHeadset.theLEDTask->Queue.Event1 = ( pEvent - EVENTS_MESSAGE_BASE) ;
                    }
                    else if ( theHeadset.theLEDTask->Queue.Event2 == 0)
                    {
                        theHeadset.theLEDTask->Queue.Event2 = ( pEvent - EVENTS_MESSAGE_BASE) ;
                    }
                    else if ( theHeadset.theLEDTask->Queue.Event3 == 0)
                    {
                        theHeadset.theLEDTask->Queue.Event3 = ( pEvent - EVENTS_MESSAGE_BASE) ;
                    }
                    else if ( theHeadset.theLEDTask->Queue.Event4 == 0)
                    {
                        theHeadset.theLEDTask->Queue.Event4 = ( pEvent - EVENTS_MESSAGE_BASE) ;
                    }    
                    else
                    {
                        LM_DEBUG(("LM: Err Queue Full!!\n")) ;
                    }
                }    
            }
        }
        else
        {
            LM_DEBUG(("LM : No IE disabled\n")) ;
        }  
    }
    else
    {
        LM_DEBUG(("LM: NoEvPatCfg %x\n",pEvent )) ;
    }
    
    /*indicate a filter if there is one present*/
    LedsCheckForFilter ( pEvent ) ;
	
}
/****************************************************************************
NAME	
	LEDManagerIndicateState

DESCRIPTION
	displays state indication information

RETURNS
	void
    
*/

void LEDManagerIndicateState ( headsetState pState )  
{
#ifdef New_MMI
    uint8 i,j,lPatternIndex;
#else
    uint8 i,lPatternIndex;
#endif

    LEDPattern_t * lPattern = NULL;
     
    lPatternIndex = NO_STATE_OR_EVENT;

	LM_DEBUG(("LM IndicateState [%d]\n", pState)) ;   
	MYLM_DEBUG(("LM IndicateState [%d]\n", pState)) ;  
	/*MYLM_DEBUG1(("LM IndicateState [%d]\n", pState)) ;*/  
	
	#if 0
	if((theHeadset.BHC612_DOCKMMI != 0) && (theHeadset.BHC612_UNDOCKMMI != 0))
	{
		MYLM_DEBUG1(("##### LM:State change = %d\n",pState));
		/*Charging LED*/
		theHeadset.BHC612_UNDOCKMMI = 0;
		theHeadset.BHC612_DOCKMMI = 0;
		theHeadset.DockLED = 0;
	}
	#endif
   
#ifdef MONO_A2DP_STREAMING
	pState = stateManagerGetCombinedLEDState(pState);
#endif
	
    /* search for a matching state */
	for(i=0;i<theHeadset.theLEDTask->gStatePatternsAllocated;i++)
    {
        if(theHeadset.theLEDTask->gStatePatterns[i].StateOrEvent == pState)
        {
            /* force indicated state to that of Low Battery configured pattern */
            lPatternIndex = i;
            lPattern = &theHeadset.theLEDTask->gStatePatterns[i];
            break;
        }
    }       

	#if 1
	if(lPatternIndex == 9)
	{
		lPattern->OnTime = 50;
		lPattern->OffTime = 0;
		lPattern->RepeatTime = 90;
		lPattern->NumFlashes = 1;
	}
	#endif
    
    /* check for low battery warning and determine if a pattern exists for the low battery
       state, if no pattern exists then do nothing otherwise show low battery pattern without
       actually changing headset state */
    if((BatteryIsBatteryLow()) && (lPatternIndex != NO_STATE_OR_EVENT))
    {
        /* check for pattern match on low battery state */
        for(i=0;i<theHeadset.theLEDTask->gStatePatternsAllocated;i++)
        {
            /* if low batt pattern exists then change headset state, otherwise leave as is */
            if(theHeadset.theLEDTask->gStatePatterns[i].StateOrEvent == headsetLowBattery)
            {
                /* force indicated state to that of Low Battery configured pattern */
				theHeadset.Rubi_enable = 1;
                pState   = headsetLowBattery;   
                lPattern = &theHeadset.theLEDTask->gStatePatterns[i];
#ifdef New_MMI
				  lPatternIndex = i;
				  MYLM_DEBUG(("headsetLowBattery LED pattern!!!\n"));
				  MYLM_DEBUG(("Pattern index = %d\n",i));
			 	  MYLM_DEBUG(("LED On = %d\n",lPattern->OnTime));	
				  MYLM_DEBUG(("LED Off = %d\n",lPattern->OffTime));
				  MYLM_DEBUG(("LED Repeat = %d\n",lPattern->RepeatTime));
				  MYLM_DEBUG(("LED Numflash = %d\n",lPattern->NumFlashes));
				  MYLM_DEBUG(("LED_A  = %d\n",lPattern->LED_A));
				  MYLM_DEBUG(("LED_B = %d\n",lPattern->LED_B));
				  MYLM_DEBUG(("Colour = %d\n",lPattern->Colour));
#endif		  
                break;
            }
        }      
    }
    
    if(lPatternIndex != NO_STATE_OR_EVENT)	
    {
        /*if there is a pattern associated with the state and not disabled, indicate it*/
        theHeadset.theLEDTask->gStateCanOverideDisable = lPattern->OverideDisable;
            
        /* only indicate if LEDs are enabled*/
        if ((theHeadset.theLEDTask->gLEDSEnabled ) ||
            (lPattern->OverideDisable)             ||
            LedActiveFiltersCanOverideDisable( ))
        {
            LM_DEBUG(("LM : IS[%x]\n", pState)) ;

			#if 1
			if((theHeadset.Rubi_enable == 1) && (pState != headsetLowBattery))
				theHeadset.Rubi_enable = 0;
			#endif
			
            if (( theHeadset.theLEDTask->gActiveLEDS[lPattern->LED_A].Type != IT_EventIndication  )
                 && ( theHeadset.theLEDTask->gActiveLEDS[lPattern->LED_B].Type != IT_EventIndication  ) )
            {            
#ifdef New_MMI
		theHeadset.BHC612_BTIDLE = 0;
		theHeadset.BHC612_BTCONNECT = 0;
		theHeadset.BHC612_BTINCCALL = 0;
		theHeadset.BHC612_BTOUTCALL = 0;

		PioSetPio (14, LED_OFF) ;
		PioSetPio (15 , LED_OFF);

		#ifdef PSBlueLED
		if(pState !=  headsetActiveCallSCO)
		{
			PioSetPio(PS_blue_led, PS_blue_led_off);
			MYLM_DEBUG(("BT LED Off 3\n"));
		}
		else
		{
			if(pState > headsetConnected)
				PioSetPio(PS_blue_led, PS_blue_led_on);
		}
		#endif
		
		if(pState == headsetThreeWayCallWaiting || pState == headsetThreeWayCallOnHold || pState == headsetThreeWayMulticall)
			return;

		#ifdef Rubidium
			/*if(pState == headsetA2DPStreaming)
				return;*/
		#else
		if(pState == headsetActiveCallNoSCO || pState == headsetA2DPStreaming)
			return;
		#endif
				
		for(i = 0 ; i < 15 ; i++)
		{
			for(j = 0 ; j < 3 ; j++)
			{
				if(pState == PatchStateLEDTable[i][j] && j == 0 && PatchStateLEDTable[i][j+2] == 0x01)/* Patch State LED */
				{
					theHeadset.theLEDTask->PatchStateColor = PatchStateLEDTable[i][j+1];
					theHeadset.theLEDTask->PatchEventColor = LED_COL_UNDEFINED;
					MYLM_DEBUG(("Patch State[%d] LED\n",pState));

					MYLM_DEBUG(("State Pattern index = %d\n",lPatternIndex));
					MYLM_DEBUG(("LED On = %d\n",lPattern->OnTime));	
					MYLM_DEBUG(("LED Off = %d\n",lPattern->OffTime));
					MYLM_DEBUG(("LED Repeat = %d\n",lPattern->RepeatTime));
					MYLM_DEBUG(("LED Numflash = %d\n",lPattern->NumFlashes));
					break;
				}	
				else if(pState == PatchStateLEDTable[i][j] && j == 0 && PatchStateLEDTable[i][j+2] == 0x00)/*Disable State LED */
				{
					if(pState == PatchStateLEDTable[i][j+1] != 0)
					{
						MYLM_DEBUG(("Discard state LED\n"));
						return;
					}
					else
					{
						/*State change*/
						if(theHeadset.theLEDTask->PatchStateColor != LED_COL_UNDEFINED)
						{
							theHeadset.theLEDTask->PatchStateColor = LED_COL_UNDEFINED;

							#ifdef PSBlueLED
							PioSetPio(PS_blue_led, PS_blue_led_off);
							MYLM_DEBUG(("BT LED Off 4\n"));
							#endif
							
							MYLM_DEBUG(("State chaged!Clear PatchStateColor flag.\n"));
						}
					}
				}
			}
		}

		if(theHeadset.theLEDTask->PatchStateColor == LED_COL_LED_BTCONNECTED)
		{
			if(lPattern->RepeatTime == 40 && lPattern->NumFlashes == 1)
				theHeadset.theLEDTask->PatchStateColor = LED_COL_LED_BTCONNECTED_1;
		}

		if(pState == headsetConnectable)
		{
			MYLM_DEBUG(("BHC612_BTIDLE flag enable!\n"));
			theHeadset.BHC612_BTIDLE = 1;
			theHeadset.BHC612_BTCONNECT = 0;
			theHeadset.BHC612_BTINCCALL = 0;
			theHeadset.BHC612_BTOUTCALL = 0;	
		}

		/*if(pState == headsetConnected)*/
		if((pState == headsetConnected) || (pState == headsetA2DPStreaming))	
		{
			theHeadset.BHC612_BTCONNECT = 1;
			theHeadset.BHC612_BTIDLE = 0;
			theHeadset.BHC612_BTINCCALL = 0;
			theHeadset.BHC612_BTOUTCALL = 0;		
		}

		if(pState == headsetIncomingCallEstablish)
		{
			theHeadset.BHC612_BTINCCALL = 1;
			theHeadset.BHC612_BTCONNECT = 0;
			theHeadset.BHC612_BTIDLE = 0;
			theHeadset.BHC612_BTOUTCALL = 0;	
		}

		if(pState == headsetOutgoingCallEstablish)
		{
			theHeadset.BHC612_BTOUTCALL = 1;
			theHeadset.BHC612_BTINCCALL = 0;
			theHeadset.BHC612_BTCONNECT = 0;
			theHeadset.BHC612_BTIDLE = 0;
		}
		
		MYLM_DEBUG1(("MYLM : IS[%x],lPatternIndex = %d\n", pState , lPatternIndex)) ;
		ledsIndicateLedsPattern(lPattern, lPatternIndex, IT_StateIndication);

		#ifdef StandbySavingPower
			/*if(pState == headsetConnected && theHeadset.theLEDTask->gLEDSEnabled == TRUE)*/
			if((pState == headsetConnected || pState == headsetActiveCallSCO || pState == headsetActiveCallNoSCO) && (theHeadset.theLEDTask->gLEDSEnabled == TRUE))
			{			
				if(ChargerIsChargerConnected() == FALSE)
				{
					MessageCancelAll(&theHeadset.task, EventDisableLEDS);
					if(pState == headsetConnected)
						MessageSendLater(&theHeadset.task, EventDisableLEDS,0, D_MIN(StandbyTimeout));
					else
						MessageSendLater(&theHeadset.task, EventDisableLEDS,0, D_MIN(TalkTimeout));
				}
			}
			else
				MessageCancelAll(&theHeadset.task, EventDisableLEDS);
		#endif
			
#else			
                /*Indicate the LED Pattern of Event/State*/
                ledsIndicateLedsPattern(lPattern, lPatternIndex, IT_StateIndication);
#endif
            }
        }
        else
        {           
            LM_DEBUG(("LM: NoStCfg[%x]\n",pState)) ;
            LedsIndicateNoState ( ) ; 
        }
    }
    else
    {
        LM_DEBUG(("LM : DIS NoStCfg[%x]\n", pState)) ;
        LedsIndicateNoState ( );
    }
	
}

/****************************************************************************
NAME	
	LedManagerDisableLEDS

DESCRIPTION
    Disable LED indications
RETURNS
	void
    
*/
void LedManagerDisableLEDS ( void )
{
    LM_DEBUG(("LM Disable LEDS\n")) ;

    /*turn off all current LED Indications if not overidden by state or filter */
    if (!theHeadset.theLEDTask->gStateCanOverideDisable && !LedActiveFiltersCanOverideDisable())
    {
        LedsIndicateNoState ( ) ;
    }    
    
    theHeadset.theLEDTask->gLEDSEnabled = FALSE ;
}

/****************************************************************************
NAME	
	LedManagerEnableLEDS

DESCRIPTION
    Enable LED indications
RETURNS
	void
    
*/
void LedManagerEnableLEDS ( void )
{
    LM_DEBUG(("LM Enable LEDS\n")) ;
    
    theHeadset.theLEDTask->gLEDSEnabled = TRUE ;
         
    LEDManagerIndicateState ( stateManagerGetState() ) ;    
}


/****************************************************************************
NAME	
	LedManagerToggleLEDS

DESCRIPTION
    Toggle Enable / Disable LED indications
RETURNS
	void
    
*/
void LedManagerToggleLEDS ( void ) 
{
    if ( theHeadset.theLEDTask->gLEDSEnabled )
    {
   		MessageSend (&theHeadset.task , EventDisableLEDS , 0) ;
    }
    else
    {
   		MessageSend (&theHeadset.task , EventEnableLEDS , 0) ;
    }
}

/****************************************************************************
NAME	
	LedManagerResetLEDIndications

DESCRIPTION
    Resets the LED Indications and reverts to state indications
	Sets the Flag to allow the Next Event to interrupt the current LED Indication
    Used if you have a permanent LED event indication that you now want to interrupt
RETURNS
	void
    
*/
void LedManagerResetLEDIndications ( void )
{    
    LedsResetAllLeds ( ) ;
    
    theHeadset.theLEDTask->gCurrentlyIndicatingEvent = FALSE ;
    
    LEDManagerIndicateState (stateManagerGetState() ) ;
}

/****************************************************************************
NAME	
	LEDManagerResetStateIndNumRepeatsComplete

DESCRIPTION
    Resets the LED Number of Repeats complete for the current state indication
       This allows the time of the led indication to be reset every time an event 
       occurs.
RETURNS
	void
    
*/
void LEDManagerResetStateIndNumRepeatsComplete  ( void ) 
{
    uint8 i,lPatternIndex;   
    LEDPattern_t * lPattern = NULL;
    
    /*get state*/
    headsetState lState = stateManagerGetState() ;
    
    /*get pattern*/   
    lPatternIndex = NO_STATE_OR_EVENT;
        
    /* search for a matching state */
    for(i=0;i<theHeadset.theLEDTask->gStatePatternsAllocated;i++)
    {
        if(theHeadset.theLEDTask->gStatePatterns[i].StateOrEvent == lState)
        {
            /* force indicated state to that of Low Battery configured pattern */
            lPattern = &theHeadset.theLEDTask->gStatePatterns[i] ;
            lPatternIndex = i;
            break;
        }
    }       
       
    /* does pattern exist for this state */
    if (lPattern)
    {
        LEDActivity_t * lLED   = &theHeadset.theLEDTask->gActiveLEDS[lPattern->LED_A] ;
        if (lLED)
        {
            /*reset num repeats complete to 0*/
            lLED->NumRepeatsComplete = 0 ;
        }    
    }
}

/****************************************************************************
NAME	
	LEDManagerCheckTimeoutState

DESCRIPTION
    checks the led timeout state and resets it if required, this function is called from
    an event or volume button press to re-enable led indications as and when required
    to do so 
RETURNS
	void
    
*/
void LEDManagerCheckTimeoutState( void )
{
    /*handles the LED event timeouts - restarts state indications if we have had a user generated event only*/
    if (theHeadset.theLEDTask->gLEDSStateTimeout)
    {   
        /* send message that can be used to show an led pattern when led's are re-enabled following a timeout */
        MessageSend( &theHeadset.task, EventResetLEDTimeout, 0);
    }
    else
    {
        /*reset the current number of repeats complete - i.e restart the timer so that the leds will disable after
          the correct time*/
        LEDManagerResetStateIndNumRepeatsComplete  ( ) ;
    }
}

#else

static const int temp ;

#endif

