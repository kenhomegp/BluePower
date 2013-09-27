/****************************************************************************
FILE NAME
    headset_buttons.h
    
DESCRIPTION
    
*/
#ifndef HEADSET_BUTTONS_H
#define HEADSET_BUTTONS_H

#include "headset_buttonmanager.h"

/****************************************************************************
NAME 
 buttonManagerInit

DESCRIPTION
 Initialises the button event 

RETURNS
 void
    
*/
void ButtonsInit (  ButtonsTaskData *pButtonsTask ) ;


/****************************************************************************
DESCRIPTION
 	Called after the configuration has been read and will trigger buttons events
    if a pio has been pressed or held whilst the configuration was still being loaded
    , i.e. the power on button press    
*/
void ButtonsCheckForChangeAfterInit( void );



#endif
