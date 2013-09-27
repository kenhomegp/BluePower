/****************************************************************************

FILE NAME
    headset_tones.h
    
DESCRIPTION
    header file which defines all of the tones which can be used in BC4 headset
    
*/  

#ifndef HEADSET_TONES_H
#define HEADSET_TONES_H



#define TONE_TYPE_RING_1 (0x60FF)
#define TONE_TYPE_RING_2 (0x60FE)

/****************************************************************************
NAME 
 TonesPlayEvent

DESCRIPTION
 function to indaicate an event by playing its associated tone uses underlying
    tones playback

RETURNS
 void
    
*/
void TonesPlayEvent ( headsetEvents_t pEvent ) ;

/****************************************************************************
NAME    
    TonesPlayTone
    
DESCRIPTION
  	Works out the correct volume to play tones/tts at
    
RETURNS
    void
*/
uint16 TonesGetToneVolume(bool PlayToneAtDefaultLevel);

/****************************************************************************
NAME    
    TonesPlayTone
    
DESCRIPTION
  	Playsback the tone given by the heasdsetTone_t index
    
RETURNS
    void
*/
void TonesPlayTone ( uint16 pTone , bool pCanQueue , bool PlayToneAtDefaultLevel) ;

/****************************************************************************
NAME    
    ToneTerminate
    
DESCRIPTION
  	function to terminate a ring tone prematurely.
    
RETURNS
    
*/
void ToneTerminate ( void ) ;

#endif

