
#include "leds.h"

#include <led.h>
#include <pio.h>
#include <message.h>
#include <panic.h>

#define DEBUG_LEDSx

#ifdef DEBUG_LEDS
#define LED_DEBUG(x) {printf x;}
#else
#define LED_DEBUG(x) 
#endif

#define ON  (0x1)
#define OFF (0x0)
#define RPT (0x1)

#define LED_UPDATE_MSG (0x0)

typedef struct ledEntryTag
{
    uint16   PioMask;           /* mask of PIOs */
    unsigned On         :1;
    unsigned Time       :15;    /* ms */   
}ledEntry_t;

typedef struct ledHeaderTag
{
    unsigned     num_entries    :8;
    unsigned     reserved       :7;
    unsigned     repeat         :1;
} ledHeader_t;    

typedef struct ledbTag 
{
    ledHeader_t  header;
    ledEntry_t * entries;
} led_t;
    

/*START_OF_INSERTED_CODE*/



#define SPECIAL_LED_PINS

/*All of The LED pins used*/
static const int gLedPinsUsed = 0xC000 ; 

 /*LEDS_OFF*/ 
static const ledEntry_t pattern_LEDS_OFF [ 1 ] = 
{
    { 0xC000 , OFF , 0    }  
}; 
/*BLUE_ONE_SEC_ON_RPT*/ 
static const ledEntry_t pattern_BLUE_ONE_SEC_ON_RPT [ 2 ] = 
{
    { 0x4000 , ON  , 1000 }  , 
    { 0x4000 , OFF , 200  }  
}; 
/*RED_ONE_SEC_ON_RPT*/ 
static const ledEntry_t pattern_RED_ONE_SEC_ON_RPT [ 2 ] = 
{
    { 0x8000 , ON  , 1000 }  , 
    { 0x8000 , OFF , 200  }  
}; 
/*BLUE_SHORT_ON_RPT*/ 
static const ledEntry_t pattern_BLUE_SHORT_ON_RPT [ 2 ] = 
{
    { 0x4000 , ON  , 100  }  , 
    { 0x4000 , OFF , 2400 }  
}; 
/*RED_SHORT_ON_RPT*/ 
static const ledEntry_t pattern_RED_SHORT_ON_RPT [ 2 ] = 
{
    { 0x8000 , ON  , 100  }  , 
    { 0x8000 , OFF , 2400 }  
}; 
/*BLUE_TWO_FLASHES_RPT*/ 
static const ledEntry_t pattern_BLUE_TWO_FLASHES_RPT [ 4 ] = 
{
    { 0x4000 , ON  , 100  }  , 
    { 0x4000 , OFF , 100  }  , 
    { 0x4000 , ON  , 100  }  , 
    { 0x4000 , OFF , 2400 }  
}; 
/*RED_TWO_FLASHES_RPT*/ 
static const ledEntry_t pattern_RED_TWO_FLASHES_RPT [ 4 ] = 
{
    { 0x8000 , ON  , 100  }  , 
    { 0x8000 , OFF , 100  }  , 
    { 0x8000 , ON  , 100  }  , 
    { 0x8000 , OFF , 2400 }  
}; 
/*BLUE_THREE_FLASHES_RPT*/ 
static const ledEntry_t pattern_BLUE_THREE_FLASHES_RPT [ 6 ] = 
{
    { 0x4000 , ON  , 100  }  , 
    { 0x4000 , OFF , 100  }  , 
    { 0x4000 , ON  , 100  }  , 
    { 0x4000 , OFF , 100  }  , 
    { 0x4000 , ON  , 100  }  , 
    { 0x4000 , OFF , 2400 }  
}; 
/*RED_THREE_FLASHES_RPT*/ 
static const ledEntry_t pattern_RED_THREE_FLASHES_RPT [ 6 ] = 
{
    { 0x8000 , ON  , 100  }  , 
    { 0x8000 , OFF , 100  }  , 
    { 0x8000 , ON  , 100  }  , 
    { 0x8000 , OFF , 100  }  , 
    { 0x8000 , ON  , 100  }  , 
    { 0x8000 , OFF , 2400 }  
}; 
/*RED_BLUE_ALT_RPT_FAST*/ 
static const ledEntry_t pattern_RED_BLUE_ALT_RPT_FAST [ 4 ] = 
{
    { 0x8000 , ON  , 100  }  , 
    { 0x8000 , OFF , 100  }  , 
    { 0x4000 , ON  , 100  }  , 
    { 0x4000 , OFF , 100  }  
}; 
/*RED_BLUE_ALT_RPT*/ 
static const ledEntry_t pattern_RED_BLUE_ALT_RPT [ 4 ] = 
{
    { 0x8000 , ON  , 1000 }  , 
    { 0x8000 , OFF , 1000 }  , 
    { 0x4000 , ON  , 1000 }  , 
    { 0x4000 , OFF , 1000 }  
}; 
/*RED_BLUE_BOTH_RPT_FAST*/ 
static const ledEntry_t pattern_RED_BLUE_BOTH_RPT_FAST [ 2 ] = 
{
    { 0xC000 , ON  , 100  }  , 
    { 0xC000 , OFF , 100  }  
}; 
/*RED_BLUE_BOTH_RPT*/ 
static const ledEntry_t pattern_RED_BLUE_BOTH_RPT [ 2 ] = 
{
    { 0xC000 , ON  , 1000 }  , 
    { 0xC000 , OFF , 1000 }  
}; 
/*LEDS_EVENT_POWER_ON*/ 
static const ledEntry_t pattern_LEDS_EVENT_POWER_ON [ 2 ] = 
{
    { 0xC000 , ON  , 1000 }  , 
    { 0xC000 , OFF , 100  }  
}; 
/*LEDS_EVENT_POWER_OFF*/ 
static const ledEntry_t pattern_LEDS_EVENT_POWER_OFF [ 6 ] = 
{
    { 0x8000 , ON  , 100  }  , 
    { 0x8000 , OFF , 100  }  , 
    { 0x8000 , ON  , 100  }  , 
    { 0x8000 , OFF , 100  }  , 
    { 0x8000 , ON  , 100  }  , 
    { 0x8000 , OFF , 100  }  
}; 


#define LED_NUM_PATTERNS ( 15 )

/*The LED entries*/
static const led_t gLeds [ LED_NUM_PATTERNS ] = 
{
    {  { 1,  0x00 , RPT }, (ledEntry_t *) pattern_LEDS_OFF } ,

    {  { 2,  0x00 , RPT }, (ledEntry_t *) pattern_BLUE_ONE_SEC_ON_RPT } ,

    {  { 2,  0x00 , RPT }, (ledEntry_t *) pattern_RED_ONE_SEC_ON_RPT } ,

    {  { 2,  0x00 , RPT }, (ledEntry_t *) pattern_BLUE_SHORT_ON_RPT } ,

    {  { 2,  0x00 , RPT }, (ledEntry_t *) pattern_RED_SHORT_ON_RPT } ,

    {  { 4,  0x00 , RPT }, (ledEntry_t *) pattern_BLUE_TWO_FLASHES_RPT } ,

    {  { 4,  0x00 , RPT }, (ledEntry_t *) pattern_RED_TWO_FLASHES_RPT } ,

    {  { 6,  0x00 , RPT }, (ledEntry_t *) pattern_BLUE_THREE_FLASHES_RPT } ,

    {  { 6,  0x00 , RPT }, (ledEntry_t *) pattern_RED_THREE_FLASHES_RPT } ,

    {  { 4,  0x00 , RPT }, (ledEntry_t *) pattern_RED_BLUE_ALT_RPT_FAST } ,

    {  { 4,  0x00 , RPT }, (ledEntry_t *) pattern_RED_BLUE_ALT_RPT } ,

    {  { 2,  0x00 , RPT }, (ledEntry_t *) pattern_RED_BLUE_BOTH_RPT_FAST } ,

    {  { 2,  0x00 , RPT }, (ledEntry_t *) pattern_RED_BLUE_BOTH_RPT } ,

    {  { 2,  0x00 , OFF }, (ledEntry_t *) pattern_LEDS_EVENT_POWER_ON } ,

    {  { 6,  0x00 , OFF }, (ledEntry_t *) pattern_LEDS_EVENT_POWER_OFF } 
};

/*END_OF_INSERTED_CODE*/


/* The LEDs task data. */
typedef struct
{
	TaskData task;
        
    /* Current pattern being played. */
    LedPattern_t gCurrentPattern;   
    /* Position in the sequence of playing the current pattern. */   
    uint16 gCurrentPatternPosition;
    /* Current repeating pattern being played */
    LedPattern_t gRepeatingPattern;
    /* Position in the sequence through that repeating pattern */
    uint16 gRepeatingPatternPosition;
    
} LedState_t;


/* This is the main LED state - initialised when used. */
LedState_t * LED = NULL;

/* Local Functions */
static void ledsInit ( void );
static void ledsSet ( uint16 pLedMask , bool pOnOrOff );
static void ledsSetPio ( uint16 pPIOMask , bool pOnOrOff  );
static void ledsHandler (Task task, MessageId id, Message data);
static bool ledsConfigPattern ( LedPattern_t  pNewPattern  ) ;


/****************************************************************************
NAME	
	ledsPlay

DESCRIPTION
    Play an LED pattern. 
    
    If a repeating pattern is already playing 
    - then this will be interrupted and the new pattern (non repeating or 
      repeating) will be played. If the new pattern is non-repeating then 
      the interrupted pattern will be resumed after completion of the 
      non-repeating pattern.
    
    If a non-repeating pattern is currently playing    
    - if the new pattern is also a non-repeating pattern, then returns false 
      (caller is responsible for queuing LEDS).
    - if the new pattern is a repeating pattern, then this will be played on 
      completion of the non-repeating current pattern.
    
RETURNS
	void
*/
bool ledsPlay ( LedPattern_t pNewPattern  ) 
{
    bool lUpdate;
    
    /* Init the LED structure if required. */
    if ( ! LED )
    {
        ledsInit();
    }
    /* Ensure range is valid. */
    if (pNewPattern > (LED_NUM_PATTERNS-1) ) 
        return FALSE;
    
    /* Function which configures the requested pattern, if possible. */
    lUpdate = ledsConfigPattern ( pNewPattern  ) ;
    
    if (lUpdate)
    {
        MessageFlushTask ( &LED->task );
                       
        LED_DEBUG(("LED: Play [%d]\n", pNewPattern));
       
        ledsSet ( gLedPinsUsed , OFF );
                  
        MessageSend ( &LED->task , LED_UPDATE_MSG , 0 );
    }
    
    return lUpdate;
}

/****************************************************************************
NAME	
	ledsHandler

DESCRIPTION
    Update the LED playback.
    Turn the LEDs On or Off according to the next element in the pattern.
    If the pattern has completed and is non repeating, then restarts the 
    pattern.

RETURNS
	void
*/
static void ledsHandler(Task task, MessageId id, Message data) 
{
   LED_DEBUG(("LED: [%d][%d][%d]\n " , 
                    LED->gCurrentPattern , 
                    LED->gCurrentPatternPosition , 
                    gLeds[LED->gCurrentPattern].header.num_entries ));
          
    switch (id)
    {
        case ( LED_UPDATE_MSG ) :
        {

        LED_DEBUG(("LED Mask[%x] On[%d] Time[%d]\n" , 
                            gLeds[LED->gCurrentPattern]
                                .entries[LED->gCurrentPatternPosition]
                                    .PioMask , 
                            gLeds[LED->gCurrentPattern]
                                .entries[LED->gCurrentPatternPosition].On ,
                            gLeds[LED->gCurrentPattern]i
                                .entries[LED->gCurrentPatternPosition].Time));  

            /* The pattern has completed. */
            if (LED->gCurrentPatternPosition >= 
                            gLeds[LED->gCurrentPattern].header.num_entries)
            {  
                if (gLeds[LED->gCurrentPattern].header.repeat)
                {      
                    /* Reset the repeating pattern. */
                    LED_DEBUG(("LED: Repeat\n"));
                    LED->gCurrentPatternPosition = 0;
                }
                else
                {       
                    /* One shot pattern is complete. */
                    /* Return to playing the repeating pattern. */
                    LED->gCurrentPattern         = LED->gRepeatingPattern;
                    LED->gCurrentPatternPosition = 
                            LED->gRepeatingPatternPosition;       
                    
                    if (gLeds[LED->gCurrentPattern].header.repeat)
                    {   
                        /* Reset the repeating pattern. */
                        LED_DEBUG(("LED: Repeat 2\n"));
                        LED->gCurrentPatternPosition = 0;
                    }       
                }
            }               
            /* Cancel all pending LED update messages. */
            MessageFlushTask ( task );            

            ledsSet ( gLeds[LED->gCurrentPattern]
                            .entries[LED->gCurrentPatternPosition].PioMask, 
                      gLeds[LED->gCurrentPattern]
                            .entries[LED->gCurrentPatternPosition].On );
            
                    
            /* Only send a message if we are not a permanently on or off 
             * pattern.
             */   
            if ( ( gLeds[LED->gCurrentPattern].header.num_entries) != 1 ) 
            {
                MessageSendLater ( task, 
                                   LED_UPDATE_MSG, 
                                   0, 
                                   (gLeds[LED->gCurrentPattern]
                                        .entries[LED->gCurrentPatternPosition]
                                            .Time ) );
            }
            
            LED->gCurrentPatternPosition ++;                   
        }
        break;
        
        default:
        break;
    }  
}

/****************************************************************************
NAME	
	ledsInit

DESCRIPTION
    Initialise the LED Library.
    
RETURNS
	void
*/
static void ledsInit( void ) 
{   
    LED = PanicUnlessNew ( LedState_t );
    
	LED->task.handler              = ledsHandler;
	LED->gCurrentPatternPosition   = 0;
	LED->gCurrentPattern           = 0;
	LED->gCurrentPatternPosition   = 0;
	LED->gRepeatingPattern         = 0;
	LED->gRepeatingPatternPosition = 0;
}

/****************************************************************************
NAME	
	ledsSet

DESCRIPTION
    Set / Clear the LED pins or PIOs.
    
RETURNS
	void
*/
static void ledsSet (uint16 pLedMask , bool pOnOrOff ) 
{	
    uint16 lMask = 0xffff;
    
#ifdef SPECIAL_LED_PINS    

  /* LED pins are special cases. */
    if ( pLedMask & 0x8000 )        
        LedConfigure(LED_0, LED_ENABLE, pOnOrOff); 
    if ( pLedMask & 0x4000 )
        LedConfigure(LED_1, LED_ENABLE, pOnOrOff); 

    /* Set the remaining PIOs. */    
    lMask = 0x3fff;
#endif

    /* set the PIOs. */    
    ledsSetPio ( ( pLedMask & lMask ) , pOnOrOff );
    
}

/****************************************************************************
NAME	
	ledsSetPio

DESCRIPTION
    Function to set / clear the mask of PIOs required.
    
RETURNS
	void
*/
static void ledsSetPio ( uint16 pPioMask , bool pOnOrOff  ) 
{
    uint16 lPinVals = 0;
    
    if ( pOnOrOff == TRUE )    
    {
        lPinVals = pPioMask ;
    }
    else
    {
        /* Clear the corresponding bit. */
        lPinVals = 0x0000;
    }
    /* (mask, bits) setting bit to a '1' sets the corresponding PIO for 
     * output.
     */
    PioSetDir32( pPioMask , pPioMask );   
    /* Set the value of the pin to the corresponding value. */         
    PioSet32( pPioMask , lPinVals );     
}

/****************************************************************************
NAME	
	ledsConfigPattern

DESCRIPTION
    Function to configure the led that has been requested, returns false if
    the led cannot be played at this time.
    
RETURNS
	bool (if an update to the LEDS required)
*/
static bool ledsConfigPattern ( LedPattern_t  pNewPattern  ) 
{
    bool lUpdate = TRUE;

    LED_DEBUG(("LED: Play Curr[%x] New [%x]\n", 
                            gLeds[LED->gCurrentPattern].header.repeat, 
                            gLeds[pNewPattern].header.repeat ));    

    /* If current pattern is repeating */
    if ( gLeds[LED->gCurrentPattern].header.repeat )
    {       
        LED_DEBUG(("1\n"));

        /*If new pattern is repeating */
        if ( gLeds[pNewPattern].header.repeat )
        {
            /* then interrupt the pattern with the new repeating pattern. */
            LED_DEBUG(("2\n"));            
            LED->gCurrentPatternPosition   = 0;
            LED->gCurrentPattern           = pNewPattern;
            
            LED->gRepeatingPattern         = pNewPattern;
            LED->gRepeatingPatternPosition = 0;            
        }
        /* Interrupt the current pattern with a repeating pattern. */
        else 
        {
            LED_DEBUG(("3\n"));
            /* Then store the current pattern to be resumed */
            LED->gRepeatingPattern         = LED->gCurrentPattern;
            LED->gRepeatingPatternPosition = LED->gCurrentPatternPosition; 
            /* and start the requested pattern. */
            LED->gCurrentPattern = pNewPattern;
            LED->gCurrentPatternPosition = 0;            
        }
    }
    /* Current pattern is non repeating. */
    else 
    {
        /*if the new pattern is repeating */
        if ( gLeds[pNewPattern].header.repeat ) 
        {       
            /* then store this to be resumed. */
            LED_DEBUG(("4\n"));            
            LED->gRepeatingPattern         = pNewPattern;
            LED->gRepeatingPatternPosition = 0;            
        }
        /* The new pattern is also non-repeating and can't be currently 
         * played. 
         */
        else 
        {
            LED_DEBUG(("5\n"));                
            lUpdate = FALSE;
        }
    }       
    return lUpdate;
}


