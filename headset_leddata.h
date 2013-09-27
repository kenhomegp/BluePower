
/****************************************************************************

FILE NAME
    headset_leddata.h
    
DESCRIPTION
    data structures  /defines for use with the LED module
    
*/
#ifndef HEADSET_LED_DATA_H
#define HEADSET_LED_DATA_H

#include "headset_events.h"
#include "headset_states.h"

#define BHC612

/****************************************************************************
Types
*/
    /*the number of LEDS (including pin outputs that we support*/
#define HEADSET_NUM_LEDS (16)
#define NO_STATE_OR_EVENT 0xff
#define LM_MAX_NUM_PATTERNS (20)
#define LM_NUM_FILTER_EVENTS (20)


typedef enum LEDSpeedActionTag
{
    SPEED_MULTIPLY = 0,
    SPEED_DIVIDE  
}LEDSpeedActionTag ;

typedef enum LEDColTag
{
    LED_COL_EITHER ,
    LED_COL_LED_A ,
    LED_COL_LED_B ,
    LED_COL_LED_ALT ,    /*Alternate the colours*/
#ifdef BHC612
/*
    LED_COL_LED_BOTH ,
    LED_COL_LED_C ,
    LED_COL_LED_D ,
    LED_COL_LED_CD_ALT ,
    LED_COL_LED_CD_BOTH 	
*/
    LED_COL_LED_BOTH ,
    LED_COL_LED_AC_BD_ALT ,
    LED_COL_LED_AD ,
    LED_COL_LED_ACD ,
    LED_COL_UNDEFINED,
    LED_COL_LED_BTIDLE ,
    LED_COL_LED_BTCONNECTED ,
    LED_COL_LED_BTCONNECTED_1 ,
    LED_COL_LED_INCOMINGCALL ,
    LED_COL_LED_OUTGOINGCALL ,
    LED_COL_CALLCONNECTED ,
    LED_COL_LED_PAIRING ,
    LED_COL_LED_POWERON ,
    LED_COL_LED_POWEROFF ,
    LED_COL_LED_LOWBATT ,
    LED_COL_LED_GASGAUGE0 ,
    LED_COL_LED_GASGAUGE1 ,
    LED_COL_LED_GASGAUGE2 ,
    LED_COL_LED_GASGAUGE3 ,
    LED_COL_LED_CD_BOTH,
    LED_COL_LED_CD_ALT ,    
    LED_COL_LED_C ,
    LED_COL_LED_D
#else
    LED_COL_LED_BOTH 
#endif
}LEDColour_t ;

typedef struct LEDFilterTag
{
/*    headsetEvents_t     Event ;  */    /*The event to action the filter upon*/
    unsigned            Event:8 ;      /*The event to action the filter upon*/
    unsigned            Speed:8 ;      /*speed multiple o apply - 0 =  no speed multiple*/
    
	unsigned            IsFilterActive:1 ;
    unsigned            Dummy:1;
    unsigned            SpeedAction:2 ;/*which action to perform on the multiple  multiply or divide */
    unsigned            Colour:4 ;     /*Force LED to this colour pattern no matter what is defined in the state pattern*/    
    unsigned            FilterToCancel:4 ;
    unsigned            OverideLED:4;
    
    
    unsigned            OverideLEDActive:1 ;
    unsigned            Dummy2:2;
    unsigned            FollowerLEDActive:1 ;/*whether this filter defines a follower led*/
	unsigned            FollowerLEDDelay:4 ; /*the Delay before the following pattern starts*/ /*50ms (0 - 750ms)*/
    unsigned            OverideDisable:1 ; /* overide LED disable flag when filter active */
    unsigned            Dummy3:7;
}LEDFilter_t ;


    /*the led pattern type */
typedef struct LEDPatternTag
{
    unsigned          StateOrEvent:8;
    unsigned          OnTime:8     ; /*ms*/
    unsigned          OffTime:8    ; /*ms*/
    unsigned          RepeatTime:8 ; /*ms*/   
    unsigned          DimTime:8     ; /*Time to Dim this LED*/       
    unsigned          TimeOut:8     ; /*number of repeats*/
    unsigned          NumFlashes:4 ; /*how many flashes in the pattern*/          
    unsigned          LED_A:4      ; /*default first LED to use*/
    unsigned          LED_B:4      ; /*second LED to use*/     
    unsigned          OverideDisable:1; /* overide LED disable flag for this pattern */
    unsigned          Colour:3     ; /*which of the LEDS to use*/ 
}LEDPattern_t ;


typedef enum IndicationTypeTag
{
    IT_Undefined = 0 ,
    IT_StateIndication,
    IT_EventIndication    
    
}IndicationType_t ;

    /*the information required for a LED to be updated*/
typedef struct LEDActivityTag
{  
    unsigned         Index:7; /*what this led is displaying*/
    unsigned         NumFlashesComplete:8 ; /*how far through the pattern we currently are*/        
    unsigned         OnOrOff:1 ;
    
    unsigned         FilterIndex:4 ;/*the filter curently attached to this LED (0-15)*/    
    unsigned         Type:2 ; /*what this LED is displaying*/
    unsigned         NumRepeatsComplete:10;
        /*dimming*/
    unsigned         DimState:7  ; /*how far through the dim pattern we are*/
    unsigned         DimDir:1    ; /*which direction we are going*/
    unsigned         DimTime:8   ;
    
}LEDActivity_t ;


    /*the event message sent on completeion of an event */
typedef struct 
{
    uint16 Event ;  
    bool PatternCompleted ;
    
} LMEndMessage_t;
    

typedef struct LEDEventQueueTag
{
    unsigned Event1:8 ;
    unsigned Event2:8 ;
    unsigned Event3:8 ;
    unsigned Event4:8 ;    
} LEDEventQueue_t;


/*the tricolour led information*/
typedef struct PioTriColLedsTag
{
	unsigned TriCol_a:4;
	unsigned TriCol_b:4;
	unsigned TriCol_c:4;
	unsigned Unused1 :4;
}PioTriColLeds_t ;


   /*The LED task type*/
typedef struct
{
 	TaskData                task;
    LEDPattern_t            gStatePatterns [HEADSET_NUM_STATES];  /*the array of pointers to the state patterns */
    LEDPattern_t            gEventPatterns[LM_MAX_NUM_PATTERNS] ; /*the actual storage for he LED patterns pointed to by the configurable event * *     */
    
    uint16                  gStatePatternsAllocated;
    uint16                  gEventPatternsAllocated;
    
    LEDFilter_t             gEventFilters[LM_NUM_FILTER_EVENTS]  ;/*pointer to the array of LED Filter patterns */
    uint16                  gLMNumFiltersUsed ;
    
    uint16                  gTheActiveFilters ; /*Mask of Filters Active*/
    
    LEDActivity_t *         gActiveLEDS; /* the array of LED Activities*/
    
    
    unsigned                gLED_0_STATE:1 ;
    unsigned                gLED_1_STATE:1 ;
    
    unsigned                gLEDSStateTimeout:1 ; /*this is set to true if a state pattern has completed - reset if new event occurs*/
    unsigned                gLEDSEnabled:1 ;      /*global LED overide  - event drivedn to enable / disable all LED Indications*/  
    
    unsigned                gCurrentlyIndicatingEvent:1; /*if we are currently indicating an event*/
    
    unsigned 				gFollowing:1 ; /**do we currently have a follower active*/
    
    unsigned                gPatternsAllocated:8 ; /* number of patterns currently allocated, speeds up config loading */

    unsigned                gStateCanOverideDisable:1;
    unsigned                gFollowPin:4;
    
    LEDEventQueue_t         Queue ;
    PioTriColLeds_t         gTriColLeds ;

	#ifdef BHC612
	LEDColour_t		 PatchEventColor;
	LEDColour_t		 PatchStateColor;
	#endif
    
} LedTaskData;  

#define LED_SCALE_ON_OFF_TIME(x) (uint16)((x * 10) << theHeadset.features.LedTimeMultiplier )
#define LED_SCALE_REPEAT_TIME(x) (uint16)((x * 50) << theHeadset.features.LedTimeMultiplier )
#define FILTER_SCALE_DELAY_TIME(x) (uint16)(x << theHeadset.features.LedTimeMultiplier )
 
#endif

