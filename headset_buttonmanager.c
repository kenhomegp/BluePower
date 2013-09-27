/****************************************************************************
FILE NAME
    main.c        

DESCRIPTION
    This is the button manager for BC4-Headset
    This file provides a configurable wrapper for the button messages and
    converts them into the standard system messages which are passed to the
    main message handler - main.c

NOTES
 
*/
#include "headset_private.h"
#include "headset_buttonmanager.h"
#include "headset_statemanager.h"
#include "headset_buttons.h"
#include "headset_volume.h"
#include "headset_slc.h"
#include "headset_led_manager.h"

#include <stddef.h>
#include <csrtypes.h>
#include <panic.h>
#include <stdlib.h>
#include <string.h>

#include "headset_events.h"

#ifdef BHC612
#include "headset_configmanager.h"
#endif

#ifdef DEBUG_BUT_MAN
    #define BM_DEBUG(x)
    #define BM_DEBUG1(x) DEBUG(x)
    
    const char * const gDebugTimeStrings[13] = {"Inv" , 
    											"Short", 
                                                "Long" ,
                                                "VLong" , 
                                                "Double" ,
                                                "Rpt" , 
                                                "LToH" , 
                                                "HToL" , 
                                                "ShSingle",
                                                "Long Release",
                                                "VLong Release",
                                                "V V Long" ,
                                                "VV Long Release"} ;
                 
#else
    #define BM_DEBUG(x) 
	#define BM_DEBUG1(x) 
#endif

/*
 LOCAL FUNCTION PROTOTYPES
 */
static void BMCheckForButtonMatch ( uint32 pButtonMask , ButtonsTime_t  pDuration  )  ;

#ifdef BHC612
static const uint8 ButtonPatternDuration[6] = { B_SHORT , B_SHORT , B_LONG , B_LONG , B_SHORT , B_SHORT};

static void BMCheckForButtonPatternMatch ( uint32 pButtonMask , ButtonsTime_t  pDuration) ;
#else
static void BMCheckForButtonPatternMatch ( uint32 pButtonMask ) ;
#endif

/****************************************************************************
VARIABLES  
*/

#define BM_NUM_BLOCKS (2)
#define BM_NUM_CONFIGURABLE_EVENTS (BM_EVENTS_PER_BLOCK * BM_NUM_BLOCKS)

#define BUTTON_PIO_DEBOUNCE_NUM_CHECKS  (4)
#define BUTTON_PIO_DEBOUNCE_TIME_MS     (15)

/****************************************************************************
  FUNCTIONS
*/

/****************************************************************************
NAME 
 	buttonManagerInit
*/  
void buttonManagerInit ( void ) 
{   	
			/*put the buttons task and the button patterens in a single memory block*/
	int lSize = sizeof(ButtonsTaskData) ; /* + (sizeof(ButtonMatchPattern_t) * BM_NUM_BUTTON_MATCH_PATTERNS) ) ;*/
		
	/*allocate the memory*/
	theHeadset.theButtonsTask = mallocPanic( lSize );

    /* initialise structure */    
    memset(theHeadset.theButtonsTask, 0, lSize);  
	
    theHeadset.theButtonsTask->client = &theHeadset.task;
 
	/*create the array of Button Events that we are going to poulate*/    
    theHeadset.theButtonsTask->gButtonEvents[0] = (ButtonEvents_t * ) ( mallocPanic( sizeof( ButtonEvents_t ) * BM_EVENTS_PER_BLOCK ) ) ;
    theHeadset.theButtonsTask->gButtonEvents[1]= (ButtonEvents_t * ) ( mallocPanic( sizeof( ButtonEvents_t ) * BM_EVENTS_PER_BLOCK ) ) ;
    
      /*init the PIO button routines with the Button manager Task data */ 
    ButtonsInit( theHeadset.theButtonsTask ) ; 
}

/****************************************************************************

DESCRIPTION
	Wrapper method for the button Duration Setup
	configures the button durations to the user values

*/   
void buttonManagerConfigDurations ( ButtonsTaskData *pButtonsTask, button_config_type * pButtons )
{
    pButtonsTask->button_config = (button_config_type *)pButtons ;
	
	if ((pButtons->debounce_number == 0 ) || ( pButtons->debounce_period_ms == 0 ) )
	{
		/*use defaults*/
		DEBUG(("BM: DEFAULT button debounce\n")) ;
		pButtonsTask->button_config->debounce_number =  BUTTON_PIO_DEBOUNCE_NUM_CHECKS;
		pButtonsTask->button_config->debounce_period_ms = BUTTON_PIO_DEBOUNCE_TIME_MS ;		
	}
	else
	{
		DEBUG(("BM: Debounce[%x][%x]\n" , pButtonsTask->button_config->debounce_number ,pButtonsTask->button_config->debounce_period_ms)) ;
	}
}

/****************************************************************************
NAME	
	buttonManagerAddMapping     
*/     
void buttonManagerAddMapping ( event_config_type * event_config, uint8 index ) 
{
    
    ButtonsTaskData * lButtonsTask = theHeadset.theButtonsTask ;                   
    ButtonEvents_t * lButtonEvent;
    
    /* obtain next free button position in array */
    if(index < BM_EVENTS_PER_BLOCK)
    {
        lButtonEvent = &lButtonsTask->gButtonEvents[0][index];
    }
    /* second set of button events as first set of 25 events is now full*/
    else
    {
        lButtonEvent = &lButtonsTask->gButtonEvents[1][index - BM_EVENTS_PER_BLOCK];
    }
  
    if ( lButtonEvent )
    {
        lButtonEvent->ButtonMask = (uint32)(event_config->pio_mask | (uint32)((uint32)((uint32)event_config->state_mask & 0xC000 ) << 10)) ;
        lButtonEvent->Duration   = (ButtonsTime_t)event_config->type ;
        lButtonEvent->Event      = event_config->event  ;
        lButtonEvent->StateMask  = event_config->state_mask & 0x3fff ;
    
        /* look for edge detect config and add the pio's used to the check for edge detect */
        if((lButtonEvent->Duration == B_LOW_TO_HIGH)||(lButtonEvent->Duration == B_HIGH_TO_LOW))
        {
            /* add pio mask bit to U16 mask store */
            lButtonsTask->gPerformEdgeCheck |= lButtonEvent->ButtonMask;
        }
        /* otherwise must be a level check */
        else
        {
            lButtonsTask->gPerformLevelCheck |= lButtonEvent->ButtonMask;
        }               
        
        /*register the buttons we are interested in with the buttons task*/               
        lButtonsTask->gButtonLevelMask |= lButtonEvent->ButtonMask ;  
    }
    else
    {
        BM_DEBUG(("_!BM1\n")) ;
    }
        
}


/****************************************************************************
DESCRIPTION
 	add a new button pattern mapping
*/
void buttonManagerAddPatternMapping ( ButtonsTaskData *pButtonsTask, uint16 pSystemEvent , uint32 * pButtonsToMatch, uint16 lPatternIndex ) 
{   
    
    uint16 lButtonIndex = 0 ;

    /*adds a button pattern map*/
    if (pButtonsTask->gButtonPatterns[lPatternIndex].EventToSend == B_INVALID )
    {
        pButtonsTask->gButtonPatterns[lPatternIndex].EventToSend = pSystemEvent ;
    
        for (lButtonIndex = 0 ; lButtonIndex < BM_NUM_BUTTONS_PER_MATCH_PATTERN ; lButtonIndex++)
        {
            pButtonsTask->gButtonPatterns[lPatternIndex].ButtonToMatch[lButtonIndex] = pButtonsToMatch[lButtonIndex] ;
        
            if (pButtonsTask->gButtonPatterns[lPatternIndex].ButtonToMatch[lButtonIndex] != 0)
            {
                pButtonsTask->gButtonPatterns[lPatternIndex].NumberOfMatches = lButtonIndex + 1;
            }
        }
        
        BM_DEBUG1(("BM: But Pat  [%x] ,[%lx][%lx][%lx][%lx][%lx][%lx] [%d]\n" ,  pButtonsTask->gButtonPatterns[lPatternIndex].EventToSend
                                                                        , pButtonsTask->gButtonPatterns[lPatternIndex].ButtonToMatch[0]
                                                                        , pButtonsTask->gButtonPatterns[lPatternIndex].ButtonToMatch[1]
                                                                        , pButtonsTask->gButtonPatterns[lPatternIndex].ButtonToMatch[2]
                                                                        , pButtonsTask->gButtonPatterns[lPatternIndex].ButtonToMatch[3]
                                                                        , pButtonsTask->gButtonPatterns[lPatternIndex].ButtonToMatch[4]
                                                                        , pButtonsTask->gButtonPatterns[lPatternIndex].ButtonToMatch[5]                                                              
                                                                        , pButtonsTask->gButtonPatterns[lPatternIndex].NumberOfMatches )) ;
    }    
    
}

/****************************************************************************
NAME	
	BMButtonDetected

DESCRIPTION
	function call for when a button has been detected 
RETURNS
	void
    
*/
void BMButtonDetected ( uint32 pButtonMask , ButtonsTime_t pTime  )
{
 
    BM_DEBUG(("BM : But [%lx] [%s]\n" ,pButtonMask ,  gDebugTimeStrings[pTime]  )) ;
 
        /*perform the search over both blocks*/
    BMCheckForButtonMatch ( pButtonMask  , pTime ) ;
    
        /*only use regular button presses for the pattern matching to make life simpler*/
    if ( ( pTime == B_SHORT ) || (pTime == B_LONG ) )
    {
    	#ifdef BHC612
        BMCheckForButtonPatternMatch (  pButtonMask  ,pTime ) ;
	#else
	BMCheckForButtonPatternMatch (  pButtonMask ) ;
	#endif
    }   
}


/****************************************************************************
NAME 
 BMCheckForButtonMatch
    
DESCRIPTION
 function to check a button for a match in the button events map - sends a message
    to a connected task with the corresponding event
    
RETURNS

    void
*/   
static void BMCheckForButtonMatch ( uint32 pButtonMask , ButtonsTime_t  pDuration ) 
{
    uint16 lBlockIndex = 0 ; 
    uint16 lEvIndex = 0 ;                             
    
        /*each block*/
    for (lBlockIndex = 0 ; lBlockIndex < BM_NUM_BLOCKS ; lBlockIndex++)
    {       /*Each Entry*/        
        for (lEvIndex = 0 ; lEvIndex < BM_EVENTS_PER_BLOCK ; lEvIndex ++)
        { 
            ButtonEvents_t * lButtonEvent = &theHeadset.theButtonsTask->gButtonEvents [lBlockIndex] [ lEvIndex ] ;
            /*if the event is valid*/
            if ( lButtonEvent != NULL)
            {            
                if (lButtonEvent->ButtonMask == pButtonMask )
                {                          
                    /*we have a button match*/
                    if ( lButtonEvent->Duration == pDuration )
                    {           
                        if ( (lButtonEvent->StateMask) & ( ( 1 << stateManagerGetState () )) )
                        {                                
                            BM_DEBUG(("BM : State Match [%lx][%x]\n" , pButtonMask , lButtonEvent->Event)) ;
                            
                            /* if the led's are disabled and the feature bit to ignore a button press when the
                               led's are enabled is true then ignore this button press and just re-enable the leds */

#ifdef ROM_LEDS
							if(theHeadset.theLEDTask->gLEDSStateTimeout && theHeadset.features.IgnoreButtonPressAfterLedEnable)
                            {
								#ifndef New_MMI
									LEDManagerCheckTimeoutState();
								#endif
                            }
                            /* all other cases the button generated event is processed as normal */
                            else
#endif
							{
                                /*we have fully matched an event....so tell the main task about it*/
                                MessageSend( theHeadset.theButtonsTask->client, (lButtonEvent->Event + EVENTS_MESSAGE_BASE) , 0 ) ;								
                            }
                            /* check whether headset needs to be made connectable as a result of a button press
                               on a multipoint headset */
                            if(theHeadset.features.GoConnectableButtonPress)
                            {
                                slcMultipointCheckConnectableState();
                            }

                        }
                    }
                }
            }
        }
    }
}

#ifdef BHC612
/****************************************************************************
DESCRIPTION
 	check to see if a button pattern has been matched
*/
static void BMCheckForButtonPatternMatch ( uint32 pButtonMask , ButtonsTime_t  pDuration ) 
{
    uint16 lIndex = 0 ;
    
    ButtonsTaskData * lButtonsTask = theHeadset.theButtonsTask ;
    
    BM_DEBUG1(("BM: Pat[%lx],%lx,%lx,%lx,%lx,%lx,%lx\n", pButtonMask,lButtonsTask->gButtonPatterns[0].ButtonToMatch[0], lButtonsTask->gButtonPatterns[0].ButtonToMatch[1],lButtonsTask->gButtonPatterns[0].ButtonToMatch[2],lButtonsTask->gButtonPatterns[0].ButtonToMatch[3],lButtonsTask->gButtonPatterns[0].ButtonToMatch[4],lButtonsTask->gButtonPatterns[0].ButtonToMatch[5])) ;
    
    for (lIndex = 0; lIndex < BM_NUM_BUTTON_MATCH_PATTERNS ; lIndex++ )
    { 

        BM_DEBUG1(("BM: Check Match - progress= [%d]\n", lButtonsTask->gButtonMatchProgress[lIndex] )) ;

        if ( (lButtonsTask->gButtonPatterns[lIndex].ButtonToMatch[lButtonsTask->gButtonMatchProgress[lIndex]] == pButtonMask) && (pDuration == ButtonPatternDuration[lButtonsTask->gButtonMatchProgress[lIndex]]))
        {
                    /*we have matched a button*/
            lButtonsTask->gButtonMatchProgress[lIndex]++ ;
            
            BM_DEBUG1(("BM: Pat Prog[%d][%x]\n", lIndex , lButtonsTask->gButtonMatchProgress[lIndex]  )) ;
                    
                
            if (lButtonsTask->gButtonMatchProgress[lIndex] >= lButtonsTask->gButtonPatterns[lIndex].NumberOfMatches)
            {
                        /*we have matched a pattern*/
                BM_DEBUG1(("BM: Pat Match[%d] Ev[%x]\n", lIndex ,lButtonsTask->gButtonPatterns[lIndex].EventToSend)) ;
                
                lButtonsTask->gButtonMatchProgress[lIndex] = 0 ;

		#ifdef BHC612
			if (stateManagerGetState() == headsetConnDiscoverable)
			{
				theHeadset.EnableATiSPP = true;
				BM_DEBUG1(("###Enable ATi SPP###\n"));
				/*configManagerWriteSessionData();*/
			}
		#else
                MessageSend( lButtonsTask->client, lButtonsTask->gButtonPatterns[lIndex].EventToSend , 0 ) ;
		#endif
            }
            
        }       
        else
        {
            lButtonsTask->gButtonMatchProgress[lIndex] = 0 ;
                /*special case = if the last button pressed was the same as the first button*/
            if ( lButtonsTask->gButtonPatterns [ lIndex ].ButtonToMatch[0]== pButtonMask)            
            {
                lButtonsTask->gButtonMatchProgress[lIndex] = 1 ;
            
            }
        }
    }
}
#else
/****************************************************************************
DESCRIPTION
 	check to see if a button pattern has been matched
*/
static void BMCheckForButtonPatternMatch ( uint32 pButtonMask  ) 
{
    uint16 lIndex = 0 ;
    
    ButtonsTaskData * lButtonsTask = theHeadset.theButtonsTask ;
    
    BM_DEBUG1(("BM: Pat[%lx],%lx,%lx,%lx,%lx,%lx,%lx\n", pButtonMask,lButtonsTask->gButtonPatterns[0].ButtonToMatch[0], lButtonsTask->gButtonPatterns[0].ButtonToMatch[1],lButtonsTask->gButtonPatterns[0].ButtonToMatch[2],lButtonsTask->gButtonPatterns[0].ButtonToMatch[3],lButtonsTask->gButtonPatterns[0].ButtonToMatch[4],lButtonsTask->gButtonPatterns[0].ButtonToMatch[5])) ;
    
    for (lIndex = 0; lIndex < BM_NUM_BUTTON_MATCH_PATTERNS ; lIndex++ )
    { 

        BM_DEBUG1(("BM: Check Match - progress= [%d]\n", lButtonsTask->gButtonMatchProgress[lIndex] )) ;

        if ( lButtonsTask->gButtonPatterns[lIndex].ButtonToMatch[lButtonsTask->gButtonMatchProgress[lIndex]] == pButtonMask )
        {
                    /*we have matched a button*/
            lButtonsTask->gButtonMatchProgress[lIndex]++ ;
            
            BM_DEBUG1(("BM: Pat Prog[%d][%x]\n", lIndex , lButtonsTask->gButtonMatchProgress[lIndex]  )) ;
                    
                
            if (lButtonsTask->gButtonMatchProgress[lIndex] >= lButtonsTask->gButtonPatterns[lIndex].NumberOfMatches)
            {
                        /*we have matched a pattern*/
                BM_DEBUG1(("BM: Pat Match[%d] Ev[%x]\n", lIndex ,lButtonsTask->gButtonPatterns[lIndex].EventToSend)) ;
                
                lButtonsTask->gButtonMatchProgress[lIndex] = 0 ;

		#ifdef BHC612
			if (stateManagerGetState() == headsetConnDiscoverable)
			{
				theHeadset.EnableATiSPP = true;
				BM_DEBUG1(("###Enable ATi SPP###\n"));
				configManagerWriteSessionData();
			}
		#else
                MessageSend( lButtonsTask->client, lButtonsTask->gButtonPatterns[lIndex].EventToSend , 0 ) ;
		#endif
            }
            
        }       
        else
        {
            lButtonsTask->gButtonMatchProgress[lIndex] = 0 ;
                /*special case = if the last button pressed was the same as the first button*/
            if ( lButtonsTask->gButtonPatterns [ lIndex ].ButtonToMatch[0]== pButtonMask)            
            {
                lButtonsTask->gButtonMatchProgress[lIndex] = 1 ;
            
            }
        }
    }
}
#endif

/****************************************************************************
DESCRIPTION
 	perform an initial read of pios following configuration reading as it is possible
    that pio states may have changed whilst the config was being read and now needs
    checking to see if any relevant events need to be generated 
*/

void BMCheckButtonsAfterReadingConfig( void )
{
    ButtonsCheckForChangeAfterInit();
}
