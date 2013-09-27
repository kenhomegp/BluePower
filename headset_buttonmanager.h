/****************************************************************************
FILE NAME
    headset_buttonmanager.h
    
DESCRIPTION
    
*/
#ifndef HEADSET_BUTTON_MANAGER_H
#define HEADSET_BUTTON_MANAGER_H

#include "headset_events.h"

#include "headset_private.h"

/* Persistent store event configuration definition */
typedef struct
{
 	unsigned    event:8;
 	unsigned 	type:8;
 	uint16  	pio_mask;
 	uint16  	state_mask;
}event_config_type;

typedef enum ButtonsTimeTag
{
    B_INVALID ,
    B_SHORT ,
    B_LONG  ,
    B_VERY_LONG , 
    B_DOUBLE ,
    B_REPEAT , 
    B_LOW_TO_HIGH ,
    B_HIGH_TO_LOW , 
    B_SHORT_SINGLE,
    B_LONG_RELEASE,
    B_VERY_LONG_RELEASE ,
    B_VERY_VERY_LONG ,
    B_VERY_VERY_LONG_RELEASE
}ButtonsTime_t ;

    /*usd byt the button manager*/
typedef struct ButtonEventsTag
{
    uint32        ButtonMask ;
    uint16        StateMask ;  
    
    
    unsigned     Duration:4 ; /*ButtonsTime_t*/
    unsigned     Event:12 ;
}ButtonEvents_t ;

#define BM_NUM_BUTTON_MATCH_PATTERNS 4

#define BM_NUM_BUTTONS_PER_MATCH_PATTERN 6 

#define BM_EVENTS_PER_BLOCK (25)


typedef struct ButtonPatternTag
{
	headsetEvents_t 		EventToSend ;
    uint16          NumberOfMatches ;
    uint32          ButtonToMatch[BM_NUM_BUTTONS_PER_MATCH_PATTERN] ;   
}ButtonMatchPattern_t ;


/* Definition of the button configuration */
typedef struct
{
    uint16 double_press_time;
    uint16 long_press_time;
    uint16 very_long_press_time; 
	uint16 repeat_time;
	uint16 very_very_long_press_time ;
  
	unsigned debounce_number:8 ;
  	unsigned debounce_period_ms:8;

}button_config_type;

	/*the buttons structure - part of the main app task*/
typedef struct
{
    TaskData    task;
	Task        client;   
    
    uint32      gBOldState  ;       /*the last state we received*/    
    uint32      gBDoubleState  ;    
    uint32 		gButtonLevelMask ;  /*mask used when detecting the PIOs changed*/
    uint32 		gBOldEdgeState ;
    
    button_config_type * button_config ; /*the button durations etc*/
    
	uint8    	gBDoubleTap ;
	unsigned 	gBTime:8 ; /**ButtonsTime_t   */
   
    ButtonEvents_t * gButtonEvents [2] ;/*pointer to the array of button event maps*/
             
    ButtonMatchPattern_t gButtonPatterns [BM_NUM_BUTTON_MATCH_PATTERNS]; /*the button match patterns*/
			
    uint16      gButtonMatchProgress[BM_NUM_BUTTON_MATCH_PATTERNS] ;  /*the progress achieved*/
    
    uint32      gPerformEdgeCheck;      /* bit mask of pio's that are configured for edge detect */
    uint32      gPerformLevelCheck;     /* bit mask of pio's that are configured for level detect */
    uint32      gOldPioState;           /* store of previous pio state for edge/level checking */

} ButtonsTaskData;


/****************************************************************************
NAME 
 buttonManagerInit

DESCRIPTION
 Initialises the button event 

RETURNS
    
*/
void buttonManagerInit ( void ) ;

/****************************************************************************
NAME 
 buttonManagerAddMapping

DESCRIPTION
 Maps a button event to a system event
        
    pButtonMask - 
    mask of the buttons that when pressed will generate an event
    e.g.  0x0001 = button PIO 0
    
          0x0003 = combination of PIO 0  and PIO 1
    pSystemEvent
        The Event to be signalled as define in headset_events.h
    pStateMask
        the states as defined in headset_states that the event will be generated in
    pDuration
        the Duration of the button press as defined in headset_buttons.h
        B_SHORT , B_LONG , B_VLONG, B_DOUBLE
          
RETURNS
    
*/
void buttonManagerAddMapping ( event_config_type * event_config , uint8 index) ;

/****************************************************************************
DESCRIPTION
 Adds a button pattern to match against
          
RETURNS
 void
*/    
void buttonManagerAddPatternMapping ( ButtonsTaskData *pButtonsTask, uint16 pSystemEvent , uint32 * pButtonsToMatch, uint16 lPatternIndex ) ;

/****************************************************************************
NAME 
 ButtonManagerConfigDurations
    
DESCRIPTION
 Wrapper method for the button Duration Setup
    
RETURNS

    void
*/   
void buttonManagerConfigDurations ( ButtonsTaskData *pButtonsTask, button_config_type * pButtons ) ; 

/****************************************************************************
NAME 
 BMButtonDetected

DESCRIPTION
 function call for when a button has been detected 
          
RETURNS
 void
*/    
void BMButtonDetected ( uint32 pButtonMask  , ButtonsTime_t pTime ) ;

/****************************************************************************
DESCRIPTION
 	perform an initial read of pios after configuration has been read as it is possible
    that pio states may have changed whilst the config was being read and now needs
    checking to see if any relevant events need to be generated 
*/

void BMCheckButtonsAfterReadingConfig( void );

#endif
