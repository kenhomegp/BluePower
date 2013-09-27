/****************************************************************************

FILE NAME
    headset_led_manager.h
    
DESCRIPTION
    
*/
#ifndef HEADSET_LED_MANAGER_H
#define HEADSET_LED_MANAGER_H



#ifdef ROM_LEDS

#include "headset_private.h"
#include "headset_states.h"
#include "headset_events.h"


    void LedManagerMemoryInit(void);
    void LEDManagerInit ( void ) ;

	void LEDManagerIndicateEvent ( MessageId pEvent ) ;

	void LEDManagerIndicateState ( headsetState pState )  ;


	void LedManagerDisableLEDS ( void ) ;
	void LedManagerEnableLEDS  ( void ) ;

	void LedManagerToggleLEDS  ( void )  ;

	void LedManagerResetLEDIndications ( void ) ;

	void LEDManagerResetStateIndNumRepeatsComplete  ( void ) ;


	void LEDManagerCheckTimeoutState( void );
        
	#ifdef DEBUG_LM
		void LMPrintPattern ( LEDPattern_t * pLED ) ;
	#endif

#else
	
	#include "leds.h"
#include "headset_events.h"		
#include "headset_states.h"		
		
		
void ledsIndicateEvent( headsetEvents_t event )  ;
void ledsIndicateState( headsetState state )  ;


	#define LEDManagerIndicateEvent(y) ledsIndicateEvent(y) 

	#define LEDManagerIndicateState(y) ledsIndicateState(y) 


	#define LedManagerToggleLEDS(x) 
	#define LedManagerEnableLEDS(x)
	#define LedManagerDisableLEDS(x)

#endif

		
		
		
#endif

