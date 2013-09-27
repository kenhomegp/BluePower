/****************************************************************************

FILE NAME
    headset_tts.h
    
DESCRIPTION
    header file which defines the interface between the tts engine and the application
    
*/  

#ifndef HEADSET_TTS_H
#define HEADSET_TTS_H
#include "headset_debug.h"

/*#include <nowspeak_text_to_speech_plugin.h>*//*Nowspeak TTS/ASR*/

#define TTS_NOT_DEFINED (0xFF)

/****************************************************************************

*/
void TTSInit( void );

/****************************************************************************

*/
void TTSConfigureEvent( headsetEvents_t pEvent , uint16 pPhrase );

/****************************************************************************

*/
void TTSConfigureVoicePrompts( uint8 size_index, void* index, uint16 no_tts_languages );

/****************************************************************************

*/
bool TTSPlayEvent( headsetEvents_t pEvent );

/****************************************************************************
NAME 
    TTSPlayNumString
DESCRIPTION
    Play a numeric string using the TTS plugin
RETURNS    
*/
void TTSPlayNumString(uint16 size_num_string, uint8* num_string);

/****************************************************************************
NAME 
    TTSPlayNumber
DESCRIPTION
    Play a uint32 using the TTS plugin
RETURNS    
*/
void TTSPlayNumber(uint32 number);

/* **************************************************************************
   */


bool TTSPlayCallerNumber( const uint16 size_number, const uint8* number );

/****************************************************************************
NAME    
    TTSTerminate
    
DESCRIPTION
  	function to terminate a ring tone prematurely.
    
RETURNS
    
*/
bool TTSPlayCallerName( const uint16 size_name, const uint8* name );
   
/****************************************************************************
NAME    
    TTSTerminate
    
DESCRIPTION
  	function to terminate a ring tone prematurely.
    
RETURNS
    
*/
void TTSTerminate( void );

/****************************************************************************
NAME    
    TTSSelectTTSLanguageMode
    
DESCRIPTION
  	Move to next language
    
RETURNS
    
*/
void TTSSelectTTSLanguageMode( void );

#endif

