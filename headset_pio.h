/****************************************************************************

FILE NAME
    headset_pio.h
    
DESCRIPTION
    Part of the ledmanager Module responsible for managing the PIO outputs excluding LEDs
    
*/

#ifndef HEADSET_PIO_H
#define HEADSET_PIO_H
#include "headset_private.h"
#include "pio.h"


#ifdef BC4_AUDIO_FLASH
	#define PioSetMicrophoneBias(x) PioSetMicBias(x) 
#else
	#define PioSetMicrophoneBias(x)  
#endif


/****************************************************************************
NAME	
	LEDManagerSetPowerOff

DESCRIPTION
    Set / Clear a power pin for the headset
    
RETURNS
	void
*/
void PioSetPowerPin ( bool enable ) ;

/****************************************************************************
NAME	 
	PioSetPio

DESCRIPTION
    Fn to change a PIO

RETURNS
	void
*/
void PioSetPio ( uint16 pPIO , bool pOnOrOff  ) ;


#endif
