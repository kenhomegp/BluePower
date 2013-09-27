
#ifndef MULTI_LEDS_H
#define MULTI_LEDS_H


#include <stdlib.h>
#include <stdio.h>

/*INSERTED_CODE_HERE*/

typedef enum LedPatternTag
{
    LEDS_OFF ,
    BLUE_ONE_SEC_ON_RPT ,
    RED_ONE_SEC_ON_RPT ,
    BLUE_SHORT_ON_RPT ,
    RED_SHORT_ON_RPT ,
    BLUE_TWO_FLASHES_RPT ,
    RED_TWO_FLASHES_RPT ,
    BLUE_THREE_FLASHES_RPT ,
    RED_THREE_FLASHES_RPT ,
    RED_BLUE_ALT_RPT_FAST ,
    RED_BLUE_ALT_RPT ,
    RED_BLUE_BOTH_RPT_FAST ,
    RED_BLUE_BOTH_RPT ,
    LEDS_EVENT_POWER_ON ,
    LEDS_EVENT_POWER_OFF

} LedPattern_t ;
/*END_OF_INSERTED_CODE*/


/****************************************************************************
NAME	
	ledsPlay

DESCRIPTION
    Play an LED pattern. 
    
    If a repeating pattern is already playing 
    - then this will be interuppted and the new pattern (non repeating or 
      repeating)will be played. If the new pattern is non-repeating then the 
      interrupted pattern will be resumed after completion of the 
      non-repeating pattern.
    
    If a non-repeating pattern is currently playing    
    - if the new pattern is also a non-repeating pattern, then returns false 
      (caller is responsible for queuing LEDS).
    - if the new pattern is a repeating pattern, then this will be played on
      completion of the non-repeating current pattern.
    
RETURNS
	bool (whether the LED Pattern has been started or not)
*/
bool ledsPlay ( LedPattern_t pNewPattern ) ;

#endif

