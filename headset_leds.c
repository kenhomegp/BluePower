/****************************************************************************

FILE NAME
    headset_leds.c
    
DESCRIPTION
    Module responsible for managing the LED outputs and the pios 
    configured as led outputs
    
*/

#ifdef ROM_LEDS

#include "headset_leds.h"
#include "headset_private.h"
#include "headset_led_manager.h"
#include "headset_statemanager.h"
#include "headset_pio.h"

#include <panic.h>
#include <stddef.h>
#include <led.h>
#include <string.h>

#ifdef DEBUG_LEDS
#define LED_DEBUG(x) {printf x;}
#else
#define LED_DEBUG(x) 
#endif

#ifdef DEBUG_DIM
#define DIM_DEBUG(x) DEBUG(x)
#else
#define DIM_DEBUG(x) 
#endif

#ifdef BHC612
#ifdef MyDEBUG_LED
#define MYLED_DEBUG(x) {printf x;}
#else
#define MYLED_DEBUG(x) 
#endif
#ifdef PowerBlueMMI
#define PowerBlueMMI_DEBUG(x) {printf x;}
#else
#define PowerBlueMMI_DEBUG(x)
#endif
#endif

#define LEDS_STATE_START_DELAY_MS 300

 /*internal message handler for the LED callback messages*/
static void LedsMessageHandler( Task task, MessageId id, Message message ) ;

 /*helper functions for the message handler*/
static uint16 LedsApplyFilterToTime     ( uint16 pTime )  ;
static LEDColour_t LedsGetPatternColour ( const LEDPattern_t * pPattern ) ;

 /*helper functions to change the state of LED pairs depending on the pattern being played*/
static void LedsTurnOffLEDPair ( LEDPattern_t * pPattern, bool pUseOveride  ) ;
static void LedsTurnOnLEDPair  ( LEDPattern_t * pPattern , LEDActivity_t * pLED );

    /*method to complete an event*/
static void LedsEventComplete ( LEDActivity_t * pPrimaryLed , LEDActivity_t * pSecondaryLed ) ;
    /*method to indicate that an event has been completed*/
static void LedsSendEventComplete ( headsetEvents_t pEvent , bool pPatternCompleted ) ;

    /*filter enable - check methods*/
static bool LedsIsFilterEnabled ( uint16 pFilter ) ;
static void LedsEnableFilter ( uint16 pFilter , bool pEnable) ;

static void LedsHandleOverideLED ( bool pOnOrOff ) ;

	/*Follower LED helper functions*/
static bool LedsCheckFiltersForLEDFollower( void ) ;

static uint16 LedsGetLedFollowerRepeatTimeLeft( LEDPattern_t* pPattern) ;


static uint16 LedsGetLedFollowerStartDelay( void ) ;

static void LedsSetEnablePin ( bool pOnOrOff ) ;

static void ledsTurnOnAltLeds(uint8 On_LedA, uint8 Off_LedB);

#define DIM_NUM_STEPS (0xf)
#define DIM_STEP_SIZE ((4096) / (DIM_NUM_STEPS + 1) ) 
#define DIM_PERIOD    (0x0)

static void PioSetLed ( uint16 pPIO , bool pOnOrOff ) ;


/****************************************************************************
NAME	
	PioSetLedPin

DESCRIPTION
    Fn to change set an LED attached to a PIO, a special LED pin , or a tricolour LED
    
RETURNS
	void
*/
void PioSetLedPin ( uint16 pPIO , bool pOnOrOff ) 
{
    LED_DEBUG(("LM : SetLed [%x][%x] \n", pPIO , pOnOrOff )) ;

    /*handle tricolour LEDS first */
	switch (pPIO)
	{		
		case (11):
		{		/*use the configured LED pair*/
			PioSetLed (theHeadset.theLEDTask->gTriColLeds.TriCol_a, pOnOrOff) ;
			PioSetLed (theHeadset.theLEDTask->gTriColLeds.TriCol_b, pOnOrOff) ;		
		}
		break ;
		case (12):						
		{	
			#ifdef NewLED0
			PioSetLed (pPIO , pOnOrOff) ;
			#else
			/*use the configured LED pair*/
			PioSetLed (theHeadset.theLEDTask->gTriColLeds.TriCol_b, pOnOrOff) ;
			PioSetLed (theHeadset.theLEDTask->gTriColLeds.TriCol_c, pOnOrOff) ;		
			#endif
		}
		break ;
		case (13) :
		{		/*use the configured LED pair*/
			PioSetLed (theHeadset.theLEDTask->gTriColLeds.TriCol_c, pOnOrOff) ;
			PioSetLed (theHeadset.theLEDTask->gTriColLeds.TriCol_a, pOnOrOff) ;	
		}
		break ;		
		default:
		{	
            /*a single LED pin to update*/
			PioSetLed (pPIO , pOnOrOff) ;
		}	
		break ;
	}
}	

/****************************************************************************
NAME	
	PioSetLed

DESCRIPTION
   Internal fn to change set an LED attached to a PIO or a special LED pin 
    
RETURNS
	void
*/
static void PioSetLed ( uint16 pPIO , bool pOnOrOff ) 
{	
    LEDActivity_t * gActiveLED = &theHeadset.theLEDTask->gActiveLEDS[pPIO];
    
   /* LED pins are special cases*/
    if ( pPIO == 14)        
    {
        /*if ( gActiveLED->DimTime > 0 )*/ /*if this is a dimming led / pattern*/
#ifdef New_MMI
	 if ( gActiveLED->DimTime > 0 && theHeadset.BHC612_DOCKMMI == 0)
#else
	if ( gActiveLED->DimTime > 0 )
#endif
        {
            if (theHeadset.theLEDTask->gLED_0_STATE != pOnOrOff) /*if the request is to do the same as what we are doing then ignore*/
            {
                    /*set led to max or min depending on whether we think the led is on or off*/
                gActiveLED->DimState = (DIM_NUM_STEPS * !pOnOrOff) ; 
                gActiveLED->DimDir   = pOnOrOff ; /*1=go up , 0 = go down**/ 
                        
                LedConfigure(LED_0, LED_DUTY_CYCLE, (gActiveLED->DimState * (DIM_STEP_SIZE)));
                LedConfigure(LED_0, LED_PERIOD, DIM_PERIOD );
                
                DIM_DEBUG(("DIM: Set LED [%d][%x][%d]\n" ,pPIO ,gActiveLED->DimState ,gActiveLED->DimDir  )) ;
                LedConfigure(LED_0, LED_ENABLE, TRUE);
                /*send the first message*/
                MessageCancelAll ( &theHeadset.theLEDTask->task, (DIM_MSG_BASE + pPIO) ) ;                
                MessageSendLater ( &theHeadset.theLEDTask->task, (DIM_MSG_BASE + pPIO) ,0 ,gActiveLED->DimTime ) ;       
              
                theHeadset.theLEDTask->gLED_0_STATE = pOnOrOff ;
            }
        }
        else
        {              
            DIM_DEBUG(("DIM 0 N:[%d]\n" , pOnOrOff)) ;
		    LedConfigure(LED_0, LED_ENABLE, pOnOrOff ) ;
            LedConfigure(LED_0, LED_DUTY_CYCLE, (0xfff));
            LedConfigure(LED_0, LED_PERIOD, DIM_PERIOD );
            theHeadset.theLEDTask->gLED_0_STATE = pOnOrOff ;
        }
    }
    else if (pPIO == 15 )
    {
        /*if ( gActiveLED->DimTime > 0 )*/ /*if this is a dimming led / pattern*/
#ifdef New_MMI
	 if ( gActiveLED->DimTime > 0 && theHeadset.BHC612_DOCKMMI == 0)	
#else
	 if ( gActiveLED->DimTime > 0 )
#endif
        {
            if (theHeadset.theLEDTask->gLED_1_STATE != pOnOrOff) /*if the request is to do the same as what we are doing then ignore*/
            {
                   /*set led to max or min depending on whether we think the led is on or off*/
                gActiveLED->DimState = (DIM_NUM_STEPS * !pOnOrOff) ; 
                gActiveLED->DimDir   = pOnOrOff ; /*1=go up , 0 = go down**/ 
                                    
                LedConfigure(LED_1, LED_DUTY_CYCLE, (gActiveLED->DimState * (DIM_STEP_SIZE)));
                LedConfigure(LED_1, LED_PERIOD, DIM_PERIOD );
                
                DIM_DEBUG(("DIM: Set LED [%d][%x][%d]\n" ,pPIO ,gActiveLED->DimState , gActiveLED->DimDir  )) ;
                LedConfigure(LED_1, LED_ENABLE, TRUE);
                   /*send the first message*/
                MessageCancelAll ( &theHeadset.theLEDTask->task, (DIM_MSG_BASE + pPIO) ) ;                
                MessageSendLater ( &theHeadset.theLEDTask->task, (DIM_MSG_BASE + pPIO) ,0 ,gActiveLED->DimTime ) ;

                theHeadset.theLEDTask->gLED_1_STATE = pOnOrOff ;                                  
            }
        }
        else
        {
            DIM_DEBUG(("DIM 1 N:[%d]\n" , pOnOrOff)) ;
            LedConfigure(LED_1, LED_ENABLE, pOnOrOff ) ;
            LedConfigure(LED_1, LED_DUTY_CYCLE, (0xfff));
            LedConfigure(LED_1, LED_PERIOD, DIM_PERIOD );
            theHeadset.theLEDTask->gLED_1_STATE = pOnOrOff ;
        }
    }
    else
    {
        PioSetPio (pPIO , pOnOrOff) ;
    }
}


/****************************************************************************
NAME	
	PioSetDimState  
	
DESCRIPTION
    Update funtion for a led that is currently dimming
    
RETURNS
	void
*/
void PioSetDimState ( uint16 pPIO )
{
    uint16 lDim = 0x0000 ;
    LEDActivity_t *gActiveLED = &theHeadset.theLEDTask->gActiveLEDS[pPIO];

    if (gActiveLED->DimDir && gActiveLED->DimState >= DIM_NUM_STEPS )
    {      
        lDim = 0xFFF;
        DIM_DEBUG(("DIM:+[F] [ON]\n" ));
    }
    else if ( !gActiveLED->DimDir && gActiveLED->DimState == 0x0 )
    {
        lDim = 0 ;
        DIM_DEBUG(("DIM:-[0] [OFF]\n" ));
    }
    else
    {
        if(gActiveLED->DimDir)
            gActiveLED->DimState++ ;
        else
            gActiveLED->DimState-- ;
        
        DIM_DEBUG(("DIM:Direction [%x], DimState:[%x], DimTime:[%x]\n", gActiveLED->DimDir, gActiveLED->DimState, gActiveLED->DimTime));
        
        lDim = (gActiveLED->DimState * (DIM_STEP_SIZE) ) ;
        
        MessageCancelAll ( &theHeadset.theLEDTask->task, (DIM_MSG_BASE + pPIO) ) ;                
        MessageSendLater ( &theHeadset.theLEDTask->task, (DIM_MSG_BASE + pPIO) , 0 , gActiveLED->DimTime ) ;    
    }    
    
    if (pPIO == 14)
    {
        LedConfigure(LED_0, LED_DUTY_CYCLE, lDim);
        LedConfigure(LED_0, LED_PERIOD, DIM_PERIOD );  
        LedConfigure(LED_0, LED_ENABLE, TRUE ) ;
    }
    else if (pPIO ==15)
    {        
        LedConfigure(LED_1, LED_DUTY_CYCLE, lDim);
        LedConfigure(LED_1, LED_PERIOD, DIM_PERIOD );  
        LedConfigure(LED_1, LED_ENABLE, TRUE ) ;
    }
    
}

/****************************************************************************
NAME 
 LedsInit

DESCRIPTION
 	Initialise the Leds data
RETURNS
 void
    
*/
void LedsInit ( void ) 
{
        /*Set the callback handler for the task*/
    theHeadset.theLEDTask->task.handler = LedsMessageHandler ;
    
    theHeadset.theLEDTask->gCurrentlyIndicatingEvent = FALSE ;
    	/*set the tricolour leds to known values*/
    theHeadset.theLEDTask->gTriColLeds.TriCol_a = 0 ;
    theHeadset.theLEDTask->gTriColLeds.TriCol_b = 0 ;
    theHeadset.theLEDTask->gTriColLeds.TriCol_c = 0 ;
    
    theHeadset.theLEDTask->gFollowing = FALSE ; 
}

     
/****************************************************************************
NAME 
 LedsCheckForFilter

DESCRIPTION
 This function checksif a filter has been configured for the given event, 
    if it has then activates / deactivates the filter 
    
    Regardless of whether a filter has been activated or not, the event is signalled as 
    completed as we have now deaklt with it (only checked for a filter if a pattern was not
    associated.

RETURNS
 void
    
*/       
void LedsCheckForFilter ( headsetEvents_t pEvent ) 
{
    uint16 lFilterIndex = 0 ;
    
    for (lFilterIndex = 0 ; lFilterIndex < theHeadset.theLEDTask->gLMNumFiltersUsed ; lFilterIndex ++ )
    { 
        LEDFilter_t *lEventFilter = &(theHeadset.theLEDTask->gEventFilters [ lFilterIndex ]);
        
        if ( (uint16)(lEventFilter->Event + EVENTS_MESSAGE_BASE) == pEvent )
        {
            if (lEventFilter->IsFilterActive)
            {
                /* Check filter isn't already enabled */
                if (!LedsIsFilterEnabled(lFilterIndex))
                {
                    /* Enable filter */
                    LedsEnableFilter (lFilterIndex , TRUE) ;
            
                    /* If it is an overide fLED filter and the currently playing pattern is OFF then turn on the overide led immediately*/
                    if ( lEventFilter->OverideLEDActive )
                    {
                        
                        uint16 lOverideLEDIndex = lEventFilter->OverideLED ;                    
                        
                        /* this should only happen if the led in question is currently off*/
                        if ( theHeadset.theLEDTask->gActiveLEDS[lOverideLEDIndex].OnOrOff == LED_OFF)
                        {
                             LED_DEBUG(("LED: FilEnable Turn on[%d][%d] \n",lFilterIndex + 1 , lOverideLEDIndex  )) ;
                             PioSetLedPin ( lOverideLEDIndex , LED_ON) ;
                        }
                    }
                }
            }
            else
            {
                 uint16 lFilterToCancel = lEventFilter->FilterToCancel ;
                /*disable the according filter*/
                 if ( lFilterToCancel != 0 )
                 {
                     uint16 lFilterToCancelIndex = lFilterToCancel - 1 ;
                     LEDFilter_t *lEventFilter1  = &(theHeadset.theLEDTask->gEventFilters [ lFilterToCancelIndex ]);
                     uint16 lOverideLEDIndex     = lEventFilter1->OverideLED ;
                    
                     LED_DEBUG(("LED: FilCancel[%d][%d] [%d]\n",lFilterIndex + 1 , lFilterToCancel , lFilterToCancelIndex )) ;
                     
                        /*lFilter To cancel = 1-n, LedsEbnable filter requires 0-n */
                     LedsEnableFilter (lFilterToCancelIndex , FALSE ) ;
                     
                     if ( theHeadset.theLEDTask->gActiveLEDS[lOverideLEDIndex].OnOrOff == LED_OFF)
                     {   /*  LedsHandleOverideLED ( theHeadset.theLEDTask , LED_OFF) ;*/
                         if ( lEventFilter1->OverideLEDActive )
                         {
                             LED_DEBUG(("LED: FilCancel Turn off[%d][%d] [%d]\n",lFilterIndex + 1 , lFilterToCancel , lFilterToCancelIndex )) ;
	          	             PioSetLedPin ( lOverideLEDIndex, LED_OFF) ;                
                             
                             /* it is possible for the cancel filter to turn off leds used in a solid led
                                state indication such as a solid blue pairing indication, should the charger be
                                removed and then reinserted the solid blue state is turned off, this call will reset
                                the state indication and turn it back on again */
                             LEDManagerIndicateState ( stateManagerGetState () ) ;                          

                         }    
                     }                           
                 }
                 else
                 {
                    LED_DEBUG(("LED: Fil !\n")) ;
                 }
            }
            LED_DEBUG(("LM : Filter Found[%d]A[%x] [%d]\n", lFilterIndex + 1,  pEvent , theHeadset.theLEDTask->gEventFilters[ lFilterIndex ].IsFilterActive )) ;
       }
    }
}
/****************************************************************************
NAME 
    ledsIndicateLedsPattern

DESCRIPTION
 	Given the indication type and leds pattern, Play the LED Pattern
RETURNS
    void
*/
void ledsIndicateLedsPattern(LEDPattern_t *lPattern, uint8 lIndex, IndicationType_t Ind_type)
{
    uint8 lPrimaryLED     = lPattern->LED_A;
    uint8 lSecondaryLED   = lPattern->LED_B;

    #ifdef DEBUG_LM
    	LMPrintPattern( lPattern ) ;
    #endif
        
    if(Ind_type == IT_EventIndication)
    {
        /*if the PIO we want to use is currently indicating an event then do interrupt the event*/
        MessageCancelAll (&theHeadset.theLEDTask->task, lPrimaryLED ) ;
        MessageCancelAll (&theHeadset.theLEDTask->task, lSecondaryLED ) ;
    }
        
    /*cancel all led state indications*/
    /*Find the LEDS that are set to indicate states and Cancel the messages,
      -do not want to indicate more than one state at a time */
    LedsIndicateNoState ( ) ;
    
    /*now set up and start the event indication*/
    LedsSetLedActivity ( &theHeadset.theLEDTask->gActiveLEDS[lPrimaryLED] , Ind_type , lIndex  , lPattern->DimTime ) ;
    /*Set the Alternative LED up with the same info*/
    LedsSetLedActivity ( &theHeadset.theLEDTask->gActiveLEDS[lSecondaryLED] , Ind_type , lIndex , lPattern->DimTime ) ;



    /* - need to set the LEDS to a known state before starting the pattern*/
    LedsTurnOffLEDPair (lPattern , TRUE ) ;
   
    /*Handle permanent output leds*/
    if ( lPattern->NumFlashes == 0 )
    {
        /*set the pins on or off as required*/
        if ( LED_SCALE_ON_OFF_TIME(lPattern->OnTime) > 0 )
        {
            LED_DEBUG(("LM :ST PIO_ON\n")) ;
            LedsTurnOnLEDPair ( lPattern , &theHeadset.theLEDTask->gActiveLEDS[lPrimaryLED]) ;
        }
        else if ( LED_SCALE_ON_OFF_TIME(lPattern->OffTime) > 0 )
        {
            LED_DEBUG(("LM :ST PIO_OFF\n")) ;
            LedsTurnOffLEDPair ( lPattern , TRUE) ;
            
            LedsSendEventComplete ( lPattern->StateOrEvent + EVENTS_MESSAGE_BASE , TRUE ) ;
            /*If we are turning a pin off the revert to state indication*/
            LedsEventComplete ( &theHeadset.theLEDTask->gActiveLEDS[lPrimaryLED] , &theHeadset.theLEDTask->gActiveLEDS[lSecondaryLED] ) ;
        }   
    }
    else
    {
        if(Ind_type == IT_EventIndication)
        {
        	LED_DEBUG(("Event LED task start!\n"));
            MessageSend (&theHeadset.theLEDTask->task, lPrimaryLED, 0) ;
            theHeadset.theLEDTask->gCurrentlyIndicatingEvent = TRUE ;
        }
        else
        {
        	LED_DEBUG(("State LED task start!\n"));
            /*send the first message for this state LED indication*/ 
            MessageSendLater (&theHeadset.theLEDTask->task , lPrimaryLED, 0 , LEDS_STATE_START_DELAY_MS ) ;
        }
    }
}

/****************************************************************************
NAME 
 LedsIndicateNoState

DESCRIPTION
	remove any state indications as there are currently none to be displayed
RETURNS
 void
    
*/
void LedsIndicateNoState ( void  )  
{
     /*Find the LEDS that are set to indicate states and Cancel the messages,
    -do not want to indicate more than one state at a time*/
    uint16 lLoop = 0;

    for ( lLoop = 0 ; lLoop < HEADSET_NUM_LEDS ; lLoop ++ )
    {
        LEDActivity_t *lActiveLeds = &theHeadset.theLEDTask->gActiveLEDS[lLoop];
        
        if (lActiveLeds->Type == IT_StateIndication)
        {
            MessageCancelAll ( &theHeadset.theLEDTask->task, lLoop ) ; 
            lActiveLeds->Type =  IT_Undefined ;
            
            LED_DEBUG(("LED: CancelStateInd[%x]\n" , lLoop)) ;
            
            LedsTurnOffLEDPair ( &theHeadset.theLEDTask->gStatePatterns[lActiveLeds->Index] ,TRUE) ;                    
        }
    }
}

/****************************************************************************
NAME 
 LedActiveFiltersCanOverideDisable

DESCRIPTION
    Check if active filters disable the global LED disable flag.
RETURNS 
 	bool
*/
bool LedActiveFiltersCanOverideDisable( void )
{
    uint16 lFilterIndex ;
    
    for (lFilterIndex = 0 ; lFilterIndex< LM_NUM_FILTER_EVENTS ; lFilterIndex++ )
    {
        if ( LedsIsFilterEnabled(lFilterIndex) )
        {
            /* check if this filter overides LED disable flag */
            if ( theHeadset.theLEDTask->gEventFilters[lFilterIndex].OverideDisable)
                return TRUE;
        }
    }
    return FALSE;
}

/****************************************************************************
NAME 
 LEDManagerMessageHandler

DESCRIPTION
 The main message handler for the LED task. Controls the PIO in question, then 
    queues a message back to itself for the next LED update.

RETURNS
 void
    
*/
static void LedsMessageHandler( Task task, MessageId id, Message message )
{  
    bool lOldState = LED_OFF ;
    uint16 lTime   = 0 ;
    LEDColour_t lColour ;    
    
    LEDActivity_t * lLED   = &theHeadset.theLEDTask->gActiveLEDS[id] ;
    LEDPattern_t *  lPattern = NULL ;
    bool lPatternComplete = FALSE ;
    
    if (id < DIM_MSG_BASE )
    {
        
        /*which pattern are we currently indicating for this LED pair*/
        if ( lLED->Type == IT_StateIndication)
        {
            /* this is a STATE indication */        
            lPattern = &theHeadset.theLEDTask->gStatePatterns[ lLED->Index] ;
        }
        else
        {   /*is an event indication*/
            lPattern = &theHeadset.theLEDTask->gEventPatterns [ lLED->Index ] ;
        }
        
        /* get which of the LEDs we are interested in for the pattern we are dealing with */
        lColour = LedsGetPatternColour ( lPattern ) ;
         
        /*get the state of the LED we are dealing with*/
        lOldState = theHeadset.theLEDTask->gActiveLEDS [ lPattern->LED_A ].OnOrOff ;
     
        LED_DEBUG(("LM : LED[%d] [%d] f[%d]of[%d]\n", id ,lOldState , lLED->NumFlashesComplete , lPattern->NumFlashes )) ;
             
        /*  is LED currently off? */
        if (lOldState == LED_OFF)
        {
            /* led is off so start the new pattern by turning LED on */
            lTime = LED_SCALE_ON_OFF_TIME(lPattern->OnTime) ;

            LED_DEBUG(("LED: set ON time [%x]\n",lTime)) ;   
                
            /*Increment the number of flashes*/
            lLED->NumFlashesComplete++ ;
                  
            LED_DEBUG(("LED: Pair On\n")) ;
			/*MYLED_DEBUG(("LED: Pair On\n")) ;*/

		    LedsTurnOnLEDPair ( lPattern , lLED ) ;           
#ifdef New_MMI
		    if((theHeadset.theLEDTask->PatchStateColor == LED_COL_LED_OUTGOINGCALL) && lLED->NumFlashesComplete == 8)
		    {
			 	lLED->NumFlashesComplete = 0;
				
				if(theHeadset.theLEDTask->PatchStateColor != LED_COL_LED_OUTGOINGCALL)
				{
					PioSetPio (14 , LED_OFF);
		    		PioSetPio (15 , LED_OFF);
					PioSetLed(BHC612_LED_0 , LED_OFF);
					PioSetLed(15 , LED_OFF);
				}
				else
				{
					PioSetPio (14 , LED_OFF);
		    		PioSetPio (15 , LED_OFF);
					PioSetLed(BHC612_LED_0 , LED_OFF);
					PioSetLed(15 , LED_ON);
				}
		    }
			
			/*if(theHeadset.theLEDTask->PatchStateColor == LED_COL_LED_PAIRING && (lLED->NumFlashesComplete == 6))*/
			if(theHeadset.theLEDTask->PatchStateColor == LED_COL_LED_PAIRING && (lLED->NumFlashesComplete == 8))	
			#if 0
			if(theHeadset.theLEDTask->PatchStateColor == LED_COL_LED_PAIRING && (lLED->NumFlashesComplete == 24 || lLED->NumFlashesComplete == 12))	
			#endif
			{				
				/*if(lLED->NumFlashesComplete == 6)*/
				if(lLED->NumFlashesComplete == 8)
				{
					lLED->NumFlashesComplete = 0;
					lPattern->OnTime = 0x04;
					PioSetPio (14 , LED_OFF);
			    	PioSetPio (15 , LED_OFF);
					PioSetLed(BHC612_LED_0 , LED_OFF);
					PioSetLed(15 , LED_OFF);
				}				
			}
		    	else if(theHeadset.theLEDTask->PatchStateColor == LED_COL_LED_BTCONNECTED)		
		    	{
				if(lLED->NumFlashesComplete == 2)		
		    		{		    			
			 			lLED->NumFlashesComplete = 0;
						theHeadset.theLEDTask->PatchStateColor = LED_COL_LED_BTCONNECTED_1;
						lPattern->OnTime = 50;
						lPattern->OffTime = 50;
                    				lPattern->RepeatTime = 0;
						lPattern->NumFlashes = 5;
					
					}
		    	}
		    	else if(theHeadset.theLEDTask->PatchStateColor == LED_COL_LED_BTCONNECTED_1)
		    	{
				if(lLED->NumFlashesComplete == 5)		
		    		{		    			
			 		lLED->NumFlashesComplete = 0;
					PioSetPio (14 , LED_OFF);
	    				PioSetPio (15 , LED_OFF);
					PioSetLed(BHC612_LED_0 , LED_OFF);
					PioSetLed(15 , LED_OFF);
				}
		    	}	
		    	else if(theHeadset.theLEDTask->PatchStateColor == LED_COL_LED_BTIDLE)
		    	{
				if(lLED->NumFlashesComplete == 1)		
			    	{		    			
				 	lLED->NumFlashesComplete = 0;
					lPattern->OnTime = 50;
					lPattern->OffTime = 0;
					lPattern->NumFlashes = 0;
					lPattern->RepeatTime = 90;				
		    			PioSetPio (15 , LED_OFF);
					PioSetLed(15 , LED_OFF);
				}								
		    	}		
#endif		 	
        }
        else
        {   
            /*restart the pattern if we have palayed all of the required flashes*/
	     #ifdef New_MMI
		 /*
		 if ( (lLED->NumFlashesComplete >= lPattern->NumFlashes && theHeadset.BHC612_DOCKMMI == 0) || (lLED->NumFlashesComplete >= lPattern->NumFlashes*4 && theHeadset.BHC612_DOCKMMI == 1) )	
		 */
		 if ( (lLED->NumFlashesComplete >= lPattern->NumFlashes && theHeadset.BHC612_DOCKMMI == 0 && theHeadset.DockLED == 0) || (lLED->NumFlashesComplete >= (lPattern->NumFlashes*4) && theHeadset.BHC612_DOCKMMI == 1 && theHeadset.DockLED == 0))	
		 #else
            if ( lLED->NumFlashesComplete >= lPattern->NumFlashes )
	     #endif		
            {
			#ifdef New_MMI
			if(lLED->NumFlashesComplete >= (lPattern->NumFlashes*4))
			{
				theHeadset.BHC612_DOCKMMI = 0;
				
				PowerBlueMMI_DEBUG(("###Charging LED 30 sec,complete!,%d,%d,%d\n",theHeadset.BHC612_StopChargingLED,theHeadset.BHC612_DOCKMMI,theHeadset.BHC612_UNDOCKMMI));
			}
			#endif
			
                lTime = LED_SCALE_REPEAT_TIME(lPattern->RepeatTime) ;
                lLED->NumFlashesComplete = 0 ;       
                    /*inc the Num times the pattern has been played*/
                lLED->NumRepeatsComplete ++ ;
                LED_DEBUG(("LED: Pat Rpt[%d] [%d][%d]\n",lTime, lLED->NumRepeatsComplete , lPattern->TimeOut)) ;
          
                /*if a single pattern has completed*/
                if ( LED_SCALE_REPEAT_TIME(lPattern->RepeatTime) == 0 ) 
                {
                    LED_DEBUG(("LED: PC: Rpt\n")) ;
                    lPatternComplete = TRUE ;
                }
                   /*a pattern timeout has occured*/
                if ( ( lPattern->TimeOut !=0 )  && ( lLED->NumRepeatsComplete >= lPattern->TimeOut) )
                {
                    lPatternComplete = TRUE ;
                    LED_DEBUG(("LED: PC: Rpt b\n")) ;
                }              
                
                /*if we have reached the end of the pattern and are using a follower then revert to the orig pattern*/
                if (theHeadset.theLEDTask->gFollowing)
                {
                    theHeadset.theLEDTask->gFollowing = FALSE ;
                    lTime = LedsGetLedFollowerRepeatTimeLeft( lPattern ) ;    
                }
                else
                {
                    /*do we have a led follower filter and are we indicating a state, if so use these parameters*/
                    if (lLED->Type == IT_StateIndication)
                    {
                        if( LedsCheckFiltersForLEDFollower( ) )
                        {
                            lTime = LedsGetLedFollowerStartDelay( ) ;       
                            theHeadset.theLEDTask->gFollowing = TRUE ;
                        }
                    }    
                 }            
            } 
            else /*otherwise set up for the next flash*/
            {
                lTime = LED_SCALE_ON_OFF_TIME(lPattern->OffTime) ;
                LED_DEBUG(("LED: set OFF time [%x]\n",lTime)) ;   
			#ifdef New_MMI
				/*MYLED_DEBUG(("LED: set OFF time [%d]\n",lTime)) ; */
			#endif
            }             
        						
    		lColour = LedsGetPatternColour ( lPattern ) ;
						
#ifdef New_MMI
			if ( lColour != LED_COL_LED_ALT && theHeadset.theLEDTask->PatchEventColor != LED_COL_LED_POWERON && theHeadset.theLEDTask->PatchEventColor != LED_COL_LED_POWEROFF && theHeadset.theLEDTask->PatchStateColor != LED_COL_LED_PAIRING && theHeadset.theLEDTask->PatchStateColor != LED_COL_LED_BTCONNECTED && theHeadset.theLEDTask->PatchStateColor != LED_COL_LED_BTCONNECTED_1 && theHeadset.theLEDTask->PatchStateColor != LED_COL_LED_OUTGOINGCALL && theHeadset.theLEDTask->PatchStateColor != LED_COL_LED_BTIDLE)
			#if 0
			if ( lColour != LED_COL_LED_ALT && theHeadset.theLEDTask->PatchEventColor != LED_COL_LED_POWERON && theHeadset.theLEDTask->PatchEventColor != LED_COL_LED_POWEROFF && theHeadset.theLEDTask->PatchStateColor != LED_COL_LED_PAIRING && theHeadset.theLEDTask->PatchStateColor != LED_COL_LED_BTCONNECTED && theHeadset.theLEDTask->PatchStateColor != LED_COL_LED_BTCONNECTED_1 && theHeadset.theLEDTask->PatchStateColor != LED_COL_LED_INCOMINGCALL && theHeadset.theLEDTask->PatchStateColor != LED_COL_LED_OUTGOINGCALL)
			#endif
			{
				if(theHeadset.theLEDTask->PatchEventColor != LED_COL_LED_LOWBATT && theHeadset.theLEDTask->PatchEventColor != LED_COL_LED_GASGAUGE0 && theHeadset.theLEDTask->PatchEventColor != LED_COL_LED_GASGAUGE1 && theHeadset.theLEDTask->PatchEventColor != LED_COL_LED_GASGAUGE2 && theHeadset.theLEDTask->PatchEventColor != LED_COL_LED_GASGAUGE3)
				{
#else
			if ( lColour != LED_COL_LED_ALT )
			{
#endif
	                	/*turn off both LEDS*/
	                	LED_DEBUG(("LED: Pair OFF\n")) ;   

	                	if ( (lTime == 0 ) && ( lPatternComplete == FALSE ) )
	   		       		{
	    	            		/*ie we are switching off for 0 time - do not use the overide led as this results in a tiny blip*/
	            	    		LedsTurnOffLEDPair ( lPattern , FALSE) ;
	        	    	}
	   		       else
	   	    	    	{
	            	    		LedsTurnOffLEDPair ( lPattern , TRUE) ;
	            		}
#ifdef New_MMI
				}
#endif
			}
			else
			{
				/*signal that we are off even though we are not*/
				theHeadset.theLEDTask->gActiveLEDS [ lPattern->LED_A ].OnOrOff  = FALSE ;                     
			}
		}
      
       
        /*handle the completion of the pattern or send the next update message*/
        if (lPatternComplete)
        {
            LED_DEBUG(("LM : PatternComplete [%x][%x]  [%x][%x]\n" , theHeadset.theLEDTask->gActiveLEDS[lPattern->LED_B].Index, lLED->Index , theHeadset.theLEDTask->gActiveLEDS[lPattern->LED_B].Type , lLED->Type    )) ;

			/*set the type of indication for both LEDs as undefined as we are now indicating nothing*/
            if ( theHeadset.theLEDTask->gActiveLEDS[id].Type == IT_EventIndication )
            {
                      /*signal the completion of an event*/
                LedsSendEventComplete ( lPattern->StateOrEvent + EVENTS_MESSAGE_BASE , TRUE ) ;
                    /*now complete the event, and indicate a new state if required*/        
                LedsEventComplete ( lLED , &theHeadset.theLEDTask->gActiveLEDS[lPattern->LED_B] ) ;
		#ifdef New_MMI
				MYLED_DEBUG(("PatternComplete\n"));

				if(theHeadset.theLEDTask->PatchEventColor == LED_COL_LED_POWERON || theHeadset.theLEDTask->PatchEventColor == LED_COL_LED_POWEROFF || theHeadset.theLEDTask->PatchStateColor == LED_COL_LED_PAIRING || theHeadset.theLEDTask->PatchStateColor == LED_COL_LED_OUTGOINGCALL)
				{
					LedsTurnOffLEDPair ( lPattern , FALSE) ;

					if(theHeadset.theLEDTask->PatchEventColor == LED_COL_LED_POWEROFF)
					{
						if (stateManagerGetState() == headsetLimbo)
						{
							MessageSendLater ( &theHeadset.task , EventLimboTimeout , 0 , D_SEC(1) ) ;
						}	
					}
				}

				if(theHeadset.theLEDTask->PatchEventColor == LED_COL_LED_LOWBATT || theHeadset.theLEDTask->PatchEventColor == LED_COL_LED_GASGAUGE0 || theHeadset.theLEDTask->PatchEventColor == LED_COL_LED_GASGAUGE1 || theHeadset.theLEDTask->PatchEventColor == LED_COL_LED_GASGAUGE2 || theHeadset.theLEDTask->PatchEventColor == LED_COL_LED_GASGAUGE3)
					LedsTurnOffLEDPair ( lPattern , FALSE) ;

				if(theHeadset.BHC612_PairSuccess == 1)
				{
					theHeadset.BHC612_PairSuccess = 0;
				}
				if(theHeadset.theLEDTask->PatchEventColor != LED_COL_UNDEFINED)
				{
					theHeadset.theLEDTask->PatchEventColor = LED_COL_UNDEFINED;
				}

				/*theHeadset.PowerStationOnOff = ChargerVOff;*/					
		#endif

            }  
            else if (theHeadset.theLEDTask->gActiveLEDS[id].Type == IT_StateIndication )
            {
                /*then we have completed a state indication and the led pattern is now off*/    
                /*Indicate that we are now with LEDS disabled*/
               theHeadset.theLEDTask->gLEDSStateTimeout = TRUE ;
		#ifdef New_MMI
			   theHeadset.theLEDTask->PatchStateColor = LED_COL_UNDEFINED;
			   theHeadset.theLEDTask->PatchEventColor = LED_COL_UNDEFINED;
		#endif
            }
            
            /* ensure leds are turned off when pattern completes as when using an alternating pattern
               leds are now left on to provide a better smoother transistion between colours */
            if ( lColour == LED_COL_LED_ALT )
            {
                LedsTurnOffLEDPair ( lPattern , TRUE) ;
            }
        }
        else
        {   
            /*apply the filter in there is one  and schedule the next message to handle for this led pair*/
            lTime = LedsApplyFilterToTime ( lTime ) ;
            MessageSendLater (&theHeadset.theLEDTask->task , id , 0 , lTime ) ;
			LED_DEBUG(("LM : PatternNotComplete  Time=[%x] [%x][%x]  [%x][%x]\n" ,lTime, theHeadset.theLEDTask->gActiveLEDS[lPattern->LED_B].Index, lLED->Index , theHeadset.theLEDTask->gActiveLEDS[lPattern->LED_B].Type , lLED->Type    )) ;
			#ifdef New_MMI
			/*MYLED_DEBUG(("LM : PatternNotComplete\n")) ;*/ 
			#endif
		} 
        
    }
    else
    {
        /*DIMMING LED Update message */       
        PioSetDimState ( (id - DIM_MSG_BASE) );
    }
}


/****************************************************************************
NAME 
 LedsTurnOnAltLeds

DESCRIPTION
    Fn to turn on the LEDs with Alt LED colour pattern
    
RETURNS
 void
*/
static void ledsTurnOnAltLeds(uint8 On_LedA, uint8 Off_LedB)
{
    uint8 TriA = theHeadset.theLEDTask->gTriColLeds.TriCol_a;
    uint8 TriB = theHeadset.theLEDTask->gTriColLeds.TriCol_b;
    uint8 TriC = theHeadset.theLEDTask->gTriColLeds.TriCol_c;
    
    switch (Off_LedB)  /*if a is a tri colour, then do not turn off the led == b*/
    {
		case (11):
			if (On_LedA != TriA)
				PioSetLedPin ( TriA, LED_OFF) ;

			if (On_LedA != TriB)
				PioSetLedPin ( TriB, LED_OFF) ;						
		break ;					
		case(12):
			if (On_LedA != TriB)
				PioSetLedPin ( TriB, LED_OFF) ;
						
			if (On_LedA != TriC)
				PioSetLedPin ( TriC, LED_OFF) ;
		break ;
		case(13):
			if (On_LedA != TriC)
				PioSetLedPin ( TriC, LED_OFF) ;
						
			if (On_LedA != TriA)								
				PioSetLedPin ( TriA, LED_OFF) ;
		break ;
		default:
			/*if b is a tricolor and one of b == a then dont turn off a */	
			switch ( On_LedA )
			{
				case(11):
					if  ( (TriA != Off_LedB) && (TriB != Off_LedB) )								
                        PioSetLedPin (  Off_LedB , LED_OFF )  ;
				break ;
				case(12):
					if  ( (TriB != Off_LedB) && (TriC != Off_LedB) )								
						PioSetLedPin (  Off_LedB , LED_OFF )  ;
				break ;
				case(13):
					if  ( (TriC != Off_LedB) && (TriA != Off_LedB) )								
						PioSetLedPin (  Off_LedB , LED_OFF )  ;
				break ;
				default:
						PioSetLedPin (  Off_LedB , LED_OFF )  ;                               
                break ;
			}					
         break ;						
	}
				
	/*now turn the other LED on*/
	PioSetLedPin (  On_LedA , LED_ON )  ;                
}


/****************************************************************************
NAME 
 LMTurnOnLEDPair

DESCRIPTION
    Fn to turn on the LED associated with the pattern / LEDs depending upon the 
    colour 
    
RETURNS
 void
*/
static void LedsTurnOnLEDPair ( LEDPattern_t * pPattern , LEDActivity_t * pLED )
{
    LEDColour_t lColour = LedsGetPatternColour ( pPattern ) ;   
    
    /* to prevent excessive stack usage when compiled in native mode only perform one read and convert of these
       4 bit parameters */
    uint8 LedA = pPattern->LED_A; 
    uint8 LedB = pPattern->LED_B; 


#ifdef New_MMI

	uint8	PatchLEDColor = lColour;

	if(theHeadset.theLEDTask->PatchEventColor != LED_COL_UNDEFINED && theHeadset.theLEDTask->PatchStateColor == LED_COL_UNDEFINED)
	{
		PatchLEDColor = theHeadset.theLEDTask->PatchEventColor;
	}
	else if(theHeadset.theLEDTask->PatchStateColor != LED_COL_UNDEFINED && theHeadset.theLEDTask->PatchEventColor == LED_COL_UNDEFINED)
	{
		PatchLEDColor = theHeadset.theLEDTask->PatchStateColor;
	}
	else
	{
		LED_DEBUG(("Unknown state!!!\n"));
	}

#endif

    /*LED_DEBUG(("LM : TurnOnPair ,color = [%d]\n ",lColour )) ;*/
	LED_DEBUG(("LM : TurnOnPair ,color = [%d]\n ",PatchLEDColor )) ;
	/*PowerBlueMMI_DEBUG(("*"));*/
    
    if (theHeadset.theLEDTask->gFollowing )
    {	 /*turn of the pair of leds (and dont use an overide LED */
        
        /* obtain the PIO to drive */
        uint16 lLED = theHeadset.theLEDTask->gFollowPin; 
                
        LedsTurnOffLEDPair ( pPattern , FALSE) ;
        
        /* check to ensure it was possible to retrieve PIO, the filter may have been cancelled */
        if(lLED <= HEADSET_NUM_LEDS)
        {
            /* set the LED specified in the follower filter */
            PioSetLedPin ( lLED , LED_ON );
        }
    }
    else
    {/*we are not following*/
            /*Turn on the LED enable pin*/    
        LedsSetEnablePin ( LED_ON )  ;

        
#ifdef New_MMI
		switch (PatchLEDColor)
#else
		switch (lColour )
#endif
		{
        case LED_COL_LED_A:
    
            LED_DEBUG(("LED: A ON[%x][%x]\n", LedA , LedB)) ;            
			/*PowerBlueMMI_DEBUG(("LED:A ON\n"));*/
            if (LedA != LedB)
            {
                if(!isOverideFilterActive(LedB))
                {
                	#ifdef New_MMI
                	if(theHeadset.BHC612_BTOUTCALL == 0)
                	{
                    		PioSetLedPin ( LedB , LED_OFF )  ;
                	}
			else
			{
				PioSetLedPin ( LedB , LED_ON )  ;
			}
			#else
				PioSetLedPin ( LedB , LED_OFF )  ;
			#endif
                }
            }
			
            PioSetLedPin ( LedA , LED_ON )  ;

		#ifdef New_MMI
			/*#ifdef PSBlueLED*/
			#if 0
			if(theHeadset.theLEDTask->gLEDSEnabled == TRUE)
			{
				PioSetPio(PS_blue_led, PS_blue_led_on);
			}
			#endif
			
			PioSetPio (14 , LED_OFF) ;
			PioSetPio (15 , LED_OFF) ;
		#endif

        break;
        case LED_COL_LED_B:			
            LED_DEBUG(("LED: B ON[%x][%x]\n", LedA , LedB)) ;
			/*PowerBlueMMI_DEBUG(("LED B On\n"));*/
            if (LedA != LedB)
            {
                if(!isOverideFilterActive( LedA))
                {
				#ifdef New_MMI
					if(theHeadset.DockLED == 0)
						PioSetLedPin ( LedA , LED_OFF )  ;
				#else
                    	PioSetLedPin ( LedA , LED_OFF )  ;
				#endif
                }
            }

		#ifdef New_MMI
			/*if(theHeadset.BHC612_BTIDLE == 0 && theHeadset.BHC612_PSConnected == 0)*/		
			if((theHeadset.BHC612_BTIDLE == 0) && (theHeadset.BHC612_PSConnected == 0) && (theHeadset.BHC612_LinkLoss == false))		
			{
				/*PowerBlueMMI_DEBUG(("LED B On\n"));*/
				PioSetLedPin ( LedB , LED_ON );
			}
		#else
            PioSetLedPin ( LedB , LED_ON )  ;
		#endif

		#ifdef New_MMI
			if(theHeadset.BHC612_BTIDLE == 1 || theHeadset.BHC612_BTCONNECT == 1)
			{
				/*Headset is docking ,flashing charging LED for 30 seconds*/
				if(theHeadset.BHC612_DOCKMMI == 1 && theHeadset.BHC612_UNDOCKMMI == 0)
				{				
					if(theHeadset.DockLED == 0)
					{
						pPattern->OnTime = 25;
						pPattern->OffTime = 25;
						pPattern->RepeatTime = 0;
						pPattern->NumFlashes = 15;					
						
						theHeadset.DockLED++;

						theHeadset.BHC612_ToggleLED = 1;

						PioSetLedPin ( LedB , LED_ON );
						PioSetLedPin ( LedA , LED_ON );
						PioSetPio(14 , LED_ON);
						PioSetPio(15 , LED_ON);
						
						/*PowerBlueMMI_DEBUG(("Chg LED Init..\n"));*/
					}
					else
					{
						#if 1
						/*if(theHeadset.BHC612_StopChargingLED)*/
						if((theHeadset.BHC612_StopChargingLED == 1) || (theHeadset.BHC612_Chargefull == 1))	
							pLED->NumFlashesComplete = 60;
						#endif	
					
						if(theHeadset.batt_level == POWER_BATT_LOW || theHeadset.batt_level == POWER_BATT_LEVEL0)
						{							
							PioSetLedPin ( LedA , LED_OFF );
							PioSetPio(14 , LED_OFF);
							PioSetPio(15 , LED_OFF);
							/*PowerBlueMMI_DEBUG(("Chg:batt low,lev0\n"));*/
							
							if(theHeadset.BHC612_ToggleLED == 1)
							{
								PioSetLedPin ( LedB , LED_OFF );
								theHeadset.BHC612_ToggleLED = 0;
							}
							else
							{
								PioSetLedPin ( LedB , LED_ON );
								theHeadset.BHC612_ToggleLED = 1;
							}
						}
						else if(theHeadset.batt_level == POWER_BATT_LEVEL1)
						{					
							PioSetLedPin ( LedB , LED_ON );
							PioSetPio(14 , LED_OFF);
							PioSetPio(15 , LED_OFF);
							/*PowerBlueMMI_DEBUG(("Chg:batt level_1\n"));*/
							
							if(theHeadset.BHC612_ToggleLED == 1)
							{
								PioSetLed(BHC612_LED_0 , LED_OFF);
								theHeadset.BHC612_ToggleLED = 0;
							}
							else
							{
								PioSetLed(BHC612_LED_0 , LED_ON);
								theHeadset.BHC612_ToggleLED = 1;
							}
						}
						else if(theHeadset.batt_level == POWER_BATT_LEVEL2)
						{
							PioSetLedPin ( LedB , LED_ON );
							PioSetLed(BHC612_LED_0 , LED_ON);
							PioSetPio(15 , LED_OFF);
							/*PowerBlueMMI_DEBUG(("Chg:batt level_2\n"));*/
							
							if(theHeadset.BHC612_ToggleLED == 1)
							{
								PioSetPio(14 , LED_OFF);
								theHeadset.BHC612_ToggleLED = 0;
							}
							else
							{
								PioSetPio(14 , LED_ON);
								theHeadset.BHC612_ToggleLED = 1;
							}
						}
						else if(theHeadset.batt_level == POWER_BATT_LEVEL3)
						{
							PioSetLedPin ( LedB , LED_ON );
							PioSetLed(BHC612_LED_0 , LED_ON);
							PioSetPio(14 , LED_ON);
							/*PowerBlueMMI_DEBUG(("Chg:batt level_3\n"));*/

							if(theHeadset.BHC612_Chargefull == 0)
							{
								if(theHeadset.BHC612_ToggleLED == 1)
								{
									PioSetPio(15 , LED_OFF);
									theHeadset.BHC612_ToggleLED = 0;
								}
								else
								{
									PioSetPio(15 , LED_ON);
									theHeadset.BHC612_ToggleLED = 1;
								}
							}
							else
							{
								PioSetPio(15 , LED_ON);
							}
						}

						if(pLED->NumFlashesComplete == 60)
						{
							theHeadset.DockLED = 0;
							/*theHeadset.BHC612_DOCKMMI = 0;*/

							pPattern->OnTime = 50;
							pPattern->OffTime = 0;
							pPattern->NumFlashes = 1;						
							pPattern->RepeatTime = 90;
							/*PowerBlueMMI_DEBUG(("End..\n"));*/
						}

						/*PowerBlueMMI_DEBUG(("led step=%d\n",pLED->NumFlashesComplete));*/
					}
				}

				/*Undock headset,show battery level*/
				if(theHeadset.BHC612_UNDOCKMMI == 1 && theHeadset.BHC612_DOCKMMI == 1)
				{
					if(theHeadset.DockLED == 0)
					{						
						/*pPattern->OnTime = 50;*/
						pPattern->OnTime = 10;
						pPattern->OffTime = 0;
						/*pPattern->RepeatTime = 45;*/
						pPattern->RepeatTime = 50;
						pPattern->NumFlashes = 6;					
						
						theHeadset.DockLED++;

						/*PowerBlueMMI_DEBUG(("*Init \n"));*/

						PioSetLed( 15 , LED_ON );
					}
					else if(theHeadset.DockLED == 1)
					{
						/*PowerBlueMMI_DEBUG(("*BATT LED_1 \n"));*/
						if(theHeadset.batt_level == POWER_BATT_LOW || theHeadset.batt_level == POWER_BATT_LEVEL0)
						{
							/*PowerBlueMMI_DEBUG(("batt low,lev0\n"));*/
						}
						else if(theHeadset.batt_level == POWER_BATT_LEVEL1)
						{
							/*PowerBlueMMI_DEBUG(("batt lev1\n"));*/
							PioSetLed(BHC612_LED_0 , LED_ON);
						}
						else if(theHeadset.batt_level == POWER_BATT_LEVEL2)
						{
							/*PowerBlueMMI_DEBUG(("batt lev2\n"));*/
							PioSetLed(BHC612_LED_0 , LED_ON);
						}
						else if(theHeadset.batt_level == POWER_BATT_LEVEL3)
						{
							/*PowerBlueMMI_DEBUG(("batt lev3\n"));*/
							PioSetLed(BHC612_LED_0 , LED_ON);
						}
						theHeadset.DockLED++;
					}
					else if(theHeadset.DockLED == 2)
					{
						/*PowerBlueMMI_DEBUG(("*BATT LED_2 \n"));*/
						if(theHeadset.batt_level == POWER_BATT_LOW || theHeadset.batt_level == POWER_BATT_LEVEL0)
						{

						}
						else if(theHeadset.batt_level == POWER_BATT_LEVEL1)
						{
							PioSetLed(BHC612_LED_0 , LED_ON);
						}
						else if(theHeadset.batt_level == POWER_BATT_LEVEL2)
						{
							PioSetLed(BHC612_LED_0 , LED_ON);
							PioSetPio(14 , LED_ON);
						}
						else if(theHeadset.batt_level == POWER_BATT_LEVEL3)
						{
							PioSetLed(BHC612_LED_0 , LED_ON);
							PioSetPio(14 , LED_ON);

						}
						theHeadset.DockLED++;
					}
					else if(theHeadset.DockLED == 3)
					{
						/*PowerBlueMMI_DEBUG(("*BATT LED_3 \n"));*/
						if(theHeadset.batt_level == POWER_BATT_LOW || theHeadset.batt_level == POWER_BATT_LEVEL0)
						{

						}
						else if(theHeadset.batt_level == POWER_BATT_LEVEL1)
						{
							PioSetLed(BHC612_LED_0 , LED_ON);
						}
						else if(theHeadset.batt_level == POWER_BATT_LEVEL2)
						{
							PioSetLed(BHC612_LED_0 , LED_ON);
							PioSetPio(14 , LED_ON);
						}
						else if(theHeadset.batt_level == POWER_BATT_LEVEL3)
						{
							PioSetLed(BHC612_LED_0 , LED_ON);
							PioSetPio(14 , LED_ON);
							PioSetPio(15 , LED_ON);
						}
						
						theHeadset.DockLED++;
					}
					else if(theHeadset.DockLED == 4)
					{
						/*PowerBlueMMI_DEBUG(("*BATT LED_4 \n"));*/
						if(theHeadset.batt_level == POWER_BATT_LOW || theHeadset.batt_level == POWER_BATT_LEVEL0)
						{

						}
						else if(theHeadset.batt_level == POWER_BATT_LEVEL1)
						{
							PioSetLed(BHC612_LED_0 , LED_ON);
						}
						else if(theHeadset.batt_level == POWER_BATT_LEVEL2)
						{
							PioSetLed(BHC612_LED_0 , LED_ON);
							PioSetPio(14 , LED_ON);
						}
						else if(theHeadset.batt_level == POWER_BATT_LEVEL3)
						{
							PioSetLed(BHC612_LED_0 , LED_ON);
							PioSetPio(14 , LED_ON);
							PioSetPio(15 , LED_ON);
						}
						
						theHeadset.DockLED++;

						pPattern->OnTime = 250;
					}					
					else if(theHeadset.DockLED == 6)
					{
						/*PowerBlueMMI_DEBUG(("*BATT LED_6 \n"));*/

						PioSetLed(15 , LED_OFF);
						PioSetPio(15 , LED_OFF);
						PioSetLed(BHC612_LED_0 , LED_OFF);
						PioSetPio(14 , LED_OFF);

						theHeadset.DockLED = 0;
						/*theHeadset.BHC612_UNDOCKMMI = 0;*/
						theHeadset.BHC612_DOCKMMI = 0;

						pPattern->OnTime = 50;
						pPattern->OffTime = 0;
						pPattern->NumFlashes = 1;						
						pPattern->RepeatTime = 90;
					}		
					else
					{
						/*PowerBlueMMI_DEBUG(("*delay..\n"));*/

						theHeadset.DockLED++;
					}
				}
			}
			else
			{
				/*PowerBlueMMI_DEBUG(("??\n"));*/
				PioSetPio (14 , LED_OFF) ;
				PioSetPio (15 , LED_OFF) ;
			}
		#endif
        break;

#ifdef New_MMI
	 case LED_COL_LED_BTIDLE:	     
	    	MYLED_DEBUG(("BTIDLE_LED\n"));
		PioSetLed(15 , LED_ON);
		PioSetPio (15 , LED_ON);  
	     break;		 
	 case LED_COL_LED_BTCONNECTED_1:	
	     if (pLED->NumFlashesComplete == 1 )
    	 {            
		 PioSetPio (15 , LED_ON) ;
		 PioSetLed(15 , LED_ON);		 
	     }
	     else
	     {	     	
		 PioSetPio (15 , LED_OFF) ;
		 PioSetLed(15 , LED_OFF);		 		 
	     }
	     break;	
	 case LED_COL_LED_BTCONNECTED:
	      if (pLED->NumFlashesComplete == 1 )
            {
		PioSetPio (14 , LED_ON) ;
		PioSetPio (15 , LED_ON) ;
		PioSetLed(15 , LED_ON);
		PioSetLed(BHC612_LED_0 , LED_ON);
            }
	     else if (pLED->NumFlashesComplete == 2 )
	     {
	 	PioSetLed(BHC612_LED_0 , LED_OFF);
		PioSetLed(15 , LED_OFF);
		PioSetPio (15 , LED_OFF) ;
		PioSetPio (14 , LED_OFF) ;
	     }
	 break;
	 case LED_COL_LED_C:
		PioSetPio (14 , LED_ON) ;
	 break;
	 case LED_COL_LED_D:
		PioSetPio (15 , LED_ON) ;
	 break;	
	 case LED_COL_LED_CD_BOTH:
	 	LED_DEBUG(("LED_CD_BOTH_ON\n"));
	 	PioSetPio (14 , LED_ON) ;
		PioSetPio (15 , LED_ON) ;
	 break;	
	 case LED_COL_LED_CD_ALT:
            if (pLED->NumFlashesComplete % 2 )
            {
		PioSetPio (14 , LED_ON) ;
		PioSetPio (15 , LED_OFF) ;
            }
	     else
	     {
	 	PioSetPio (14 , LED_OFF) ;
		PioSetPio (15 , LED_ON) ;
	     }
	 break;	
	case LED_COL_LED_OUTGOINGCALL:
		if(pLED->NumFlashesComplete % 2)
		{
			PioSetLed (BHC612_LED_0 , LED_OFF) ;
		}
		else
		{
			PioSetLed (BHC612_LED_0 , LED_ON) ;
		}
		break;
	case LED_COL_LED_PAIRING:
		if(theHeadset.BHC612_PAIRMODE == 0)
		{
			/*PowerBlueMMI_DEBUG(("P"));*/

			if(pLED->NumFlashesComplete == 1)
			{
				PioSetLed(15 , LED_ON);
			}
			else if(pLED->NumFlashesComplete == 2)
			{
				PioSetLed(BHC612_LED_0 , LED_ON);
			}
			else if(pLED->NumFlashesComplete == 3)
			{
				PioSetPio(14 , LED_ON);
			}
			else if(pLED->NumFlashesComplete == 4)
			{
				PioSetLed(15 , LED_OFF);
				PioSetPio(15 , LED_ON);
			}			
			else if(pLED->NumFlashesComplete == 5)
			{
				PioSetLed(BHC612_LED_0 , LED_OFF);
			}
			else if(pLED->NumFlashesComplete == 6)
			{
				PioSetPio(14 , LED_OFF);
				/*theHeadset.BHC612_PAIRMODE = 0;*/
			}
			else if(pLED->NumFlashesComplete == 7)
			{
				PioSetPio(15 , LED_OFF);
				/*theHeadset.BHC612_PAIRMODE = 0;*/
				pPattern->OnTime = 50;
			}
			else if(pLED->NumFlashesComplete == 8)
			{
				theHeadset.BHC612_PAIRMODE = 0;
			}
		}
		else
		{
			if(pLED->NumFlashesComplete == 1)
			{
				PioSetPio(15 , LED_ON);
			}
			else if(pLED->NumFlashesComplete == 2)
			{
				PioSetPio(14 , LED_ON);
			}
			else if(pLED->NumFlashesComplete == 3)
			{
				PioSetPio(15 , LED_OFF);
			}
			else if(pLED->NumFlashesComplete == 4)
			{
				PioSetPio(14 , LED_OFF);
			}
			else if(pLED->NumFlashesComplete == 5)
			{
				PioSetPio(14 , LED_ON);
			}
			else if(pLED->NumFlashesComplete == 6)
			{
				PioSetLed(BHC612_LED_0 , LED_ON);
			}
			else if(pLED->NumFlashesComplete == 7)
			{
				PioSetPio(14 , LED_OFF);
			}
			else if(pLED->NumFlashesComplete == 8)
			{
				PioSetLed(BHC612_LED_0 , LED_OFF);
			}	
			else if(pLED->NumFlashesComplete == 9)
			{
				PioSetLed(BHC612_LED_0 , LED_ON);
			}
			else if(pLED->NumFlashesComplete == 10)
			{
				PioSetLed(15 , LED_ON);
			}
			else if(pLED->NumFlashesComplete == 11)
			{
				PioSetLed(BHC612_LED_0 , LED_OFF);
			}
			else if(pLED->NumFlashesComplete == 12)
			{
				PioSetLed(15 , LED_OFF);
				theHeadset.BHC612_PAIRMODE = 0;
			}
		}
		break;	
	case LED_COL_LED_POWERON:
		if(pLED->NumFlashesComplete == 1)
		{
			PioSetLed(15 , LED_ON);
		}
		else if(pLED->NumFlashesComplete == 2)
		{
			PioSetLed(15 , LED_ON);
			PioSetLed(BHC612_LED_0 , LED_ON);
		}
		else if(pLED->NumFlashesComplete == 3)
		{
			PioSetLed(15 , LED_ON);
			PioSetLed(BHC612_LED_0 , LED_ON);
			PioSetPio(14 , LED_ON);
		}
		else if(pLED->NumFlashesComplete == 4)
		{
			PioSetLed(15 , LED_ON);
			PioSetLed(BHC612_LED_0 , LED_ON);
			PioSetPio(14 , LED_ON);
			PioSetPio(15 , LED_ON);
		}
		else if(pLED->NumFlashesComplete == 5)
		{	
			if(theHeadset.batt_level == POWER_BATT_LOW || theHeadset.batt_level == POWER_BATT_LEVEL0)
			{
				PioSetLed(15 , LED_ON);
				PioSetLed(BHC612_LED_0 , LED_OFF);
				PioSetPio(14 , LED_OFF);
				PioSetPio(15 , LED_OFF);
			}
			else if(theHeadset.batt_level == POWER_BATT_LEVEL1)
			{
				PioSetLed(15 , LED_ON);
				PioSetLed(BHC612_LED_0 , LED_ON);
				PioSetPio(14 , LED_OFF);
				PioSetPio(15 , LED_OFF);
			}
			else if(theHeadset.batt_level == POWER_BATT_LEVEL2)
			{
				PioSetLed(15 , LED_ON);
				PioSetLed(BHC612_LED_0 , LED_ON);
				PioSetPio(14 , LED_ON);
				PioSetPio(15 , LED_OFF);
			}
			else if(theHeadset.batt_level == POWER_BATT_LEVEL3)
			{
				PioSetLed(15 , LED_ON);
				PioSetLed(BHC612_LED_0 , LED_ON);
				PioSetPio(14 , LED_ON);
				PioSetPio(15 , LED_ON);
			}
		}
	 break;
	 case LED_COL_LED_POWEROFF:
		if(pLED->NumFlashesComplete == 1)
		{		
			if(theHeadset.batt_level == POWER_BATT_LOW ||theHeadset.batt_level == POWER_BATT_LEVEL0)
			{
				PioSetLed(15 , LED_ON);
				PioSetLed(BHC612_LED_0 , LED_OFF);
				PioSetPio(14 , LED_OFF);
				PioSetPio(15 , LED_OFF);
			}
			else if(theHeadset.batt_level == POWER_BATT_LEVEL1)
			{
				PioSetLed(15 , LED_ON);
				PioSetLed(BHC612_LED_0 , LED_ON);
				PioSetPio(14 , LED_OFF);
				PioSetPio(15 , LED_OFF);
			}
			else if(theHeadset.batt_level == POWER_BATT_LEVEL2)
			{
				PioSetLed(15 , LED_ON);
				PioSetLed(BHC612_LED_0 , LED_ON);
				PioSetPio(14 , LED_ON);
				PioSetPio(15 , LED_OFF);
			}
			else if(theHeadset.batt_level == POWER_BATT_LEVEL3)
			{
				PioSetLed(15 , LED_ON);
				PioSetLed(BHC612_LED_0 , LED_ON);
				PioSetPio(14 , LED_ON);
				PioSetPio(15 , LED_ON);
			}		
		}
		#if 0
		else if(pLED->NumFlashesComplete == 7)
		{
			PioSetLed(15 , LED_ON);
			PioSetLed(BHC612_LED_0 , LED_ON);
			PioSetPio(14 , LED_ON);
			PioSetPio(15 , LED_ON);
		}
		else if(pLED->NumFlashesComplete == 8)
		{
			PioSetPio(15 , LED_OFF);
		}
		else if(pLED->NumFlashesComplete == 9)
		{
			PioSetPio(14 , LED_OFF);
		}
		else if(pLED->NumFlashesComplete == 10)
		{
			PioSetLed(BHC612_LED_0 , LED_OFF);
		}
		else if(pLED->NumFlashesComplete == 11)
		{
			PioSetLed(15 , LED_OFF);
		}
		#endif
	 break;
#endif	 
        case LED_COL_LED_ALT:
                   
            if (pLED->NumFlashesComplete % 2 )
            {
                LED_DEBUG(("LED: A ALT On[%x],Off[%x]\n", LedB , LedA)) ;
                ledsTurnOnAltLeds(LedB, LedA);
            }
            else
            {
                LED_DEBUG(("LED: B ALT On[%x],Off[%x]\n", LedA , LedB)) ;
                ledsTurnOnAltLeds(LedA, LedB);
            }        

	#ifdef New_MMI
		PioSetPio (14 , LED_OFF) ;
		PioSetPio (15 , LED_OFF) ;
	#endif			
        break;
        case LED_COL_LED_BOTH:

            LED_DEBUG(("LED: AB Both[%x][%x]\n", LedA , LedB)) ;
            PioSetLedPin (  LedA , LED_ON )  ;
            PioSetLedPin (  LedB , LED_ON )  ;

	#ifdef New_MMI
		if(theHeadset.BHC612_BTINCCALL == 1)
		{
			PioSetPio (14 , LED_ON) ;
			PioSetPio (15 , LED_ON) ;
			
			#ifdef PSBlueLED
			if(theHeadset.theLEDTask->gLEDSEnabled == TRUE)
			{
				PioSetPio(PS_blue_led, PS_blue_led_on);
			}
			#endif
		}
		else if(theHeadset.BHC612_PairSuccess == 1)
		{
			/*4 LEds on*/
			PioSetPio (14 , LED_ON) ;
			PioSetPio (15 , LED_ON) ;
		}
		else
		{
			PioSetPio (14 , LED_OFF) ;
			PioSetPio (15 , LED_OFF) ;
		}
	#endif
        break;
        default:
            LED_DEBUG(("LM : ?Col\n")) ;
        break;
        }
    }
 
    /* only process the overide filter if not an alternating pattern or a permanently on pattern otherwise
       led will be turned off */
    if((lColour != LED_COL_LED_ALT)&&(pPattern->NumFlashes))
    {
        /*handle an overide LED if there is one will also dealit is different to one of the pattern LEDS)*/
        if(!isOverideFilterActive(LedA))
        {
            LED_DEBUG(("LM : TurnOnPair - Handle Overide\n" )) ;
			MYLED_DEBUG(("LM : TurnOnPair - Handle Overide\n" )) ;
            LedsHandleOverideLED (   LED_OFF ) ;
        }
    }
    
    pLED->OnOrOff = TRUE ;
        
}


/****************************************************************************
NAME 
 LMTurnOffLEDPair

DESCRIPTION
    Fn to turn OFF the LEDs associated with the pattern
    
RETURNS
 void
*/
static void LedsTurnOffLEDPair ( LEDPattern_t * pPattern  , bool pUseOveride ) 
{
    LED_DEBUG(("LM : TurnOffPair, pUseOveride = %d\n",pUseOveride )) ;
	MYLED_DEBUG(("LM : LedsTurnOffLEDPair\n")) ;
	/*PowerBlueMMI_DEBUG(("#"));*/

    /*turn off both LEDS*/
    if(!isOverideFilterActive( pPattern->LED_A))
    {
        LED_DEBUG(("LM : TurnOffPair - OVR A%x \n", pPattern->LED_A )) ;

	#ifdef New_MMI
		if(theHeadset.BHC612_DOCKMMI == 0)
		{
			/*PowerBlueMMI_DEBUG(("@"));*/
			PioSetLedPin ( pPattern->LED_A , LED_OFF )  ;
		}
	#else
        PioSetLedPin ( pPattern->LED_A , LED_OFF )  ;
	#endif
	}
    
    if(!isOverideFilterActive( pPattern->LED_B))
    {
        LED_DEBUG(("LM : TurnOffPair - OVR B %x \n", pPattern->LED_B )) ;

	#ifdef New_MMI		
		if(theHeadset.BHC612_DOCKMMI == 0 && theHeadset.BHC612_BTOUTCALL == 0)
		{			
			/*PowerBlueMMI_DEBUG(("&"));*/		
			PioSetLedPin ( pPattern->LED_B , LED_OFF )  ;
		}
	#else
        PioSetLedPin ( pPattern->LED_B , LED_OFF )  ;
	#endif
    }

#ifdef New_MMI
	if(theHeadset.theLEDTask->PatchEventColor == LED_COL_LED_POWERON || theHeadset.theLEDTask->PatchEventColor == LED_COL_LED_POWEROFF)
	{
		PioSetPio (14, LED_OFF) ;
    	PioSetPio (15 , LED_OFF) ;
		PioSetLed(15 , LED_OFF);
		PioSetLed(15 , LED_OFF);
    	LED_DEBUG(("LM : TurnOffPair,LED_COL_LED_POWERON/OFF\n"));
	}
	else if(theHeadset.theLEDTask->PatchEventColor == LED_COL_LED_LOWBATT || theHeadset.theLEDTask->PatchEventColor == LED_COL_LED_GASGAUGE0 || theHeadset.theLEDTask->PatchEventColor == LED_COL_LED_GASGAUGE1 || theHeadset.theLEDTask->PatchEventColor == LED_COL_LED_GASGAUGE2 || theHeadset.theLEDTask->PatchEventColor == LED_COL_LED_GASGAUGE3)
	{
		PioSetPio (14, LED_OFF) ;
    	PioSetPio (15 , LED_OFF) ;
		PioSetLed(15 , LED_OFF);
		PioSetLed(15 , LED_OFF);
	}
	else
	{
		MYLED_DEBUG(("chk 1\n"));
		
		/*PowerBlueMMI_DEBUG(("chk %d,%d ",theHeadset.BHC612_DOCKMMI,theHeadset.BHC612_UNDOCKMMI));*/
		if(theHeadset.DockLED == 0 )
		{
			/*PowerBlueMMI_DEBUG(("1\n"));*/
			PioSetPio (14, LED_OFF) ;
			PioSetPio (15 , LED_OFF);
		}

		/*PowerBlueMMI_DEBUG(("2\n"));*/
	
		if(theHeadset.BHC612_BTINCCALL)
		{					
			#ifdef PSBlueLED
			if(theHeadset.theLEDTask->gLEDSEnabled == TRUE)
			{
				PioSetPio(PS_blue_led, PS_blue_led_off);
				/*PowerBlueMMI_DEBUG(("BT LED Off 2\n"));*/
			}
			#endif
			
			if(theHeadset.DockLED == 1)
			{
                /*PowerBlueMMI_DEBUG(("3\n"));*/
				PioSetPio (14, LED_OFF) ;
				PioSetPio (15 , LED_OFF);
			}
		}
		
	}
#endif
	
        /*handle an overide LED if we want to use one*/
    if ( pUseOveride )
    {
        LedsHandleOverideLED ( LED_ON ) ;
    }
	
    theHeadset.theLEDTask->gActiveLEDS [ pPattern->LED_A ].OnOrOff  = FALSE ;        
    
        /*Turn off the LED enable pin*/  

    LedsSetEnablePin ( LED_OFF )  ;
}


/****************************************************************************
NAME 
 LedsHandleOverideLED

DESCRIPTION
    Enables / diables any overide LEDS if there are some    
RETURNS
*/
static void LedsHandleOverideLED ( bool pOnOrOff ) 
{   
    uint16 lFilterIndex = 0 ;
    
    for (lFilterIndex = 0 ; lFilterIndex< LM_NUM_FILTER_EVENTS ; lFilterIndex++ )
    {
        if ( LedsIsFilterEnabled(lFilterIndex) )
        {
             if ( theHeadset.theLEDTask->gEventFilters[lFilterIndex].OverideLEDActive )
             {
                    /*Overide the Off LED with the Overide LED*/
                    LED_DEBUG(("LM: LEDOveride [%d] [%d]\n" , theHeadset.theLEDTask->gEventFilters[lFilterIndex].OverideLED , pOnOrOff)) ;    
                    PioSetLedPin ( theHeadset.theLEDTask->gEventFilters[lFilterIndex].OverideLED , pOnOrOff) ;   
             }    
        }
    }  
}


/****************************************************************************
NAME 
 LMGetPatternColour

DESCRIPTION
    Fn to determine the LEDColour_t of the LED pair we are currently playing
    takes into account whether or not a filter is currently active
    
RETURNS
 LEDColour_t
*/
static LEDColour_t LedsGetPatternColour ( const  LEDPattern_t * pPattern )
{
    uint16 lFilterIndex = 0 ;
        /*sort out the colour of the LED we are interested in*/
    LEDColour_t lColour = pPattern->Colour ;
   
    for (lFilterIndex = 0 ; lFilterIndex< LM_NUM_FILTER_EVENTS ; lFilterIndex++ )
    {
        if ( LedsIsFilterEnabled(lFilterIndex) )
        {
            if ( theHeadset.theLEDTask->gEventFilters[lFilterIndex].Colour != LED_COL_EITHER )
            {
                    /*Overide the Off LED with the Overide LED*/
                lColour = theHeadset.theLEDTask->gEventFilters[lFilterIndex].Colour;   
            } 
        }
    }
    return lColour ;
}


/****************************************************************************
NAME 
 LMApplyFilterToTime

DESCRIPTION
    Fn to change the callback time if a filter has been applied - if no filter is applied
    just returns the original time
    
RETURNS
 uint16 the callback time
*/
static uint16 LedsApplyFilterToTime ( uint16 pTime ) 
{
    uint16 lFilterIndex = 0 ;
    uint16 lTime = pTime ; 
    
    for (lFilterIndex = 0 ; lFilterIndex< LM_NUM_FILTER_EVENTS ; lFilterIndex++ )
    {
        if ( LedsIsFilterEnabled(lFilterIndex) )
        {
            LEDFilter_t *lEventFilter = &(theHeadset.theLEDTask->gEventFilters[lFilterIndex]);
            
            if ( lEventFilter->Speed )
            {
                if (lEventFilter->SpeedAction == SPEED_MULTIPLY)
                {
                    LED_DEBUG(("LED: FIL_MULT[%d]\n" , lEventFilter->Speed )) ;
                    lTime *= lEventFilter->Speed ;
                }
                else /*we want to divide*/
                {
                    if (lTime)
                    {
                       LED_DEBUG(("LED: FIL_DIV[%d]\n" , lEventFilter->Speed )) ;
                      lTime /= lEventFilter->Speed ;
                    }
                }
            }
        }
    }

    return lTime ;
}




/****************************************************************************
NAME 
 LEDManagerSendEventComplete

DESCRIPTION
    Sends a message to the main task thread to say that an event indication has been completed
    
    
RETURNS
 void
*/
void LedsSendEventComplete ( headsetEvents_t pEvent , bool pPatternCompleted )
{
    if ( (pEvent > EVENTS_MESSAGE_BASE) && (pEvent <= EVENTS_LAST_EVENT ) )
    {   
        LMEndMessage_t * lEventMessage = mallocPanic ( sizeof(LMEndMessage_t) ) ;
         
        /*need to add the message containing the EventType here*/
        lEventMessage->Event = pEvent  ;
        lEventMessage->PatternCompleted =  pPatternCompleted ;
                        
        LED_DEBUG(("LM : lEvCmp[%x] [%x]\n",lEventMessage->Event , lEventMessage->PatternCompleted )) ;
            
        MessageSend ( &theHeadset.task , EventLEDEventComplete , lEventMessage ) ;
    }
}

/****************************************************************************
NAME 
 LedsEventComplete

DESCRIPTION
    signal that a given event indicatio has completed
RETURNS
 	void
*/
static void LedsEventComplete ( LEDActivity_t * pPrimaryLed , LEDActivity_t * pSecondaryLed ) 
{       
    pPrimaryLed->Type = IT_Undefined ;
    
    pSecondaryLed->Type = IT_Undefined ;
    
    theHeadset.theLEDTask->gCurrentlyIndicatingEvent = FALSE ;
}        
/****************************************************************************
NAME 
 LedsEnableFilter

DESCRIPTION
    enable / disable a given filter ID
RETURNS
 	void
*/
static void LedsEnableFilter ( uint16 pFilter , bool pEnable)
{
    uint16 lOldMask = theHeadset.theLEDTask->gTheActiveFilters ;
    
    if (pEnable)
    {
        /*to set*/
        theHeadset.theLEDTask->gTheActiveFilters |= (  0x1 << pFilter ) ;
        LED_DEBUG(("LED: EnF [%x] [%x] [%x]\n", lOldMask , theHeadset.theLEDTask->gTheActiveFilters , pFilter))    ;
    }
    else
    {
        /*to unset*/
        theHeadset.theLEDTask->gTheActiveFilters &= (0xFFFF - (  0x1 << pFilter ) ) ;
        LED_DEBUG(("LED: DisF [%x] [%x] [%x]\n", lOldMask , theHeadset.theLEDTask->gTheActiveFilters , pFilter))    ;
    }
    
    /* Check if we should indicate state */
    if ((theHeadset.theLEDTask->gEventFilters[pFilter].OverideDisable) && (lOldMask != theHeadset.theLEDTask->gTheActiveFilters))
        LEDManagerIndicateState ( stateManagerGetState () ) ;                          
}

/****************************************************************************
NAME 
 LedsIsFilterEnabled

DESCRIPTION
    determine if a filter is enabled
RETURNS
 	bool - enabled or not
*/
static bool LedsIsFilterEnabled ( uint16 pFilter )
{
    bool lResult = FALSE ;
    
    if ( theHeadset.theLEDTask->gTheActiveFilters & (0x1 << pFilter ) )
    {
        lResult = TRUE ;
    }
    
    return lResult ;
}

/****************************************************************************
NAME 
 LedsSetLedActivity

DESCRIPTION
    Sets a Led Activity to a known state
RETURNS
 void
*/
void LedsSetLedActivity ( LEDActivity_t * pLed , IndicationType_t pType , uint16 pIndex , uint16 pDimTime)
{
    
    
    pLed->Type               = pType ;
    pLed->Index              = pIndex ;
    pLed->DimTime            = pDimTime ;   
    /*LED_DEBUG(("LED[%d]\n" , pDimTime)) ; */
  
}
/****************************************************************************
NAME 
	LedsCheckFiltersForLEDFollower
DESCRIPTION
    determine if a follower is currently active
RETURNS
 	bool - active or not
*/
static bool LedsCheckFiltersForLEDFollower( void )
{
    uint16 lResult = FALSE ;    
    uint16 lFilterIndex = 0 ;
    
    for (lFilterIndex = 0 ; lFilterIndex< LM_NUM_FILTER_EVENTS ; lFilterIndex++ )
    {
        if ( LedsIsFilterEnabled(lFilterIndex) )
        {
            LEDFilter_t *lEventFilter = &(theHeadset.theLEDTask->gEventFilters[lFilterIndex]);
                
            /*if this filter defines a lefd follower*/
            if ( lEventFilter->FollowerLEDActive)
            {
                theHeadset.theLEDTask->gFollowPin = lEventFilter->OverideLED;
                lResult = TRUE ;
            }    
        }
    }
    return lResult ;
}
/****************************************************************************
NAME 
	LedsGetLedFollowerRepeatTimeLeft
DESCRIPTION
    calculate the new repeat time based upon the follower led delay and the normal repeat time
RETURNS
 	uint16 - updated repeat time to use
*/
static uint16 LedsGetLedFollowerRepeatTimeLeft( LEDPattern_t * pPattern) 
{
    uint16 lTime = LED_SCALE_REPEAT_TIME(pPattern->RepeatTime) ;
    uint16 lPatternTime = ( ( LED_SCALE_ON_OFF_TIME(pPattern->OnTime)  *  pPattern->NumFlashes) + 
                            ( LED_SCALE_ON_OFF_TIME( pPattern->OffTime) * (pPattern->NumFlashes - 1 ) )   +
                            ( LedsGetLedFollowerStartDelay() ) ) ;
                            
    if(lPatternTime < lTime )
    {
        lTime = lTime - lPatternTime ;
        LED_DEBUG(("LED: FOllower Rpt [%d] = [%d] - [%d]\n " , lTime , LED_SCALE_REPEAT_TIME(pPattern->RepeatTime) , lPatternTime)) ;
    }
    
    return lTime ;        
}
/****************************************************************************
NAME 
	LedsGetLedFollowerStartDelay
DESCRIPTION
    get the delay associated with a follower led pin
RETURNS
 	uint16 - delay to use for the follower
*/             
static uint16 LedsGetLedFollowerStartDelay( void )
{
    uint16 lDelay = 0 ;
    uint16 lFilterIndex =0 ;    
    
    for (lFilterIndex = 0 ; lFilterIndex< LM_NUM_FILTER_EVENTS ; lFilterIndex++ )
    {
        if ( LedsIsFilterEnabled(lFilterIndex) )
        {
                /*if this filter defines a lefd follower*/
            if ( theHeadset.theLEDTask->gEventFilters[lFilterIndex].FollowerLEDActive)
            {		 /*the led to use to follow with*/
                LED_DEBUG(("LM: LEDFollower Led[%d] Delay[%d]\n" , theHeadset.theLEDTask->gEventFilters[lFilterIndex].OverideLED ,
                                                                   FILTER_SCALE_DELAY_TIME(theHeadset.theLEDTask->gEventFilters[lFilterIndex].FollowerLEDDelay))) ;    
                lDelay = FILTER_SCALE_DELAY_TIME(theHeadset.theLEDTask->gEventFilters[lFilterIndex].FollowerLEDDelay) * 50 ;
            }    
        }
    }

    return lDelay ;
}

/****************************************************************************
NAME 
 LedsResetAllLeds

DESCRIPTION
    resets all led pins to off and cancels all led and state indications
RETURNS
 void
*/
void LedsResetAllLeds ( void ) 
{   
		/*Cancel all event indications*/ 
    uint16 lLoop = 0;
    
    for ( lLoop = 0 ; lLoop < HEADSET_NUM_LEDS ; lLoop ++ )
    {
        if (theHeadset.theLEDTask->gActiveLEDS[lLoop].Type == IT_EventIndication)
        {
            MessageCancelAll ( &theHeadset.theLEDTask->task, lLoop ) ; 
            theHeadset.theLEDTask->gActiveLEDS[lLoop].Type =  IT_Undefined ;
            
            LED_DEBUG(("LED: CancelEventInd[%x]\n" , lLoop)) ;
            LedsTurnOffLEDPair( &theHeadset.theLEDTask->gEventPatterns[theHeadset.theLEDTask->gActiveLEDS[lLoop].Index] ,TRUE) ;
        }
    }
    	/*cancel all state indications*/
    LedsIndicateNoState ( )  ;   
}


/****************************************************************************
NAME 
 LedsSetEnablePin

DESCRIPTION
    if configured sets a pio as a led enable pin
RETURNS
 void
*/
static void LedsSetEnablePin ( bool pOnOrOff ) 
{
#ifndef New_MMI
    if ( theHeadset.PIO->LedEnablePIO < 0xF ) 
    {
        PioSetLedPin ( theHeadset.PIO->LedEnablePIO , pOnOrOff ) ;
    }
#endif	
}


/****************************************************************************
NAME 
 isOverideFilterActive

DESCRIPTION
    determine if an overide filter is currently active and driving one of the
    leds in which case return TRUE to prevent it being turned off to display 
    another pattern, allows solid red with flashing blue with no interuption in
    red for example.
RETURNS
    true or false
*/
bool isOverideFilterActive ( uint8 Led ) 
{  
    uint16 lFilterIndex = 0 ;
 
    /* determine whether feature to make an overide filter drive the led permanently regardless of 
       any intended flash pattern for that led is enabled */
    if(theHeadset.features.OverideFilterPermanentlyOn)
    {
        /* permanent overide filter led indication is enabled, this means that an active override
           filter will drive its configured led regardless of any other patterns configured for that
           led */
        for (lFilterIndex = 0 ; lFilterIndex< LM_NUM_FILTER_EVENTS ; lFilterIndex++ )
        {
            /* look for any active filters */
            if ( LedsIsFilterEnabled(lFilterIndex) )
            {
                /* if this is an overide filter driving an led check to see if the passed in LED
                   requires that this led be turned off, if it does then stop the led being turned off
                   otherwise allow it to be turned off as usual */
                 if ( theHeadset.theLEDTask->gEventFilters[lFilterIndex].OverideLEDActive )
                 {
                    /* if overide led is active and is driving the passed in led stop this led being turned off */
                    if ((theHeadset.theLEDTask->gEventFilters[lFilterIndex].OverideLED)&&
                        (theHeadset.theLEDTask->gEventFilters[lFilterIndex].OverideLED == Led))
                    {
                        return TRUE;                    
                    }
                }    
            }
        }  
    }
    /* permanent overide filter led drive is diabled so allow led pattern to be indicated */
    else
    {
        return FALSE;
    }
    
    /* default case whereby led can be driven normally */
    return FALSE;    
}

#else
	static const int dummy;
#endif
