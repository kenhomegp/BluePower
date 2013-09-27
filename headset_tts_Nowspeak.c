/****************************************************************************

FILE NAME
    headset_tts.c
    
DESCRIPTION
    module responsible for tts selection
    
*/

#include "headset_private.h"
#include "headset_debug.h"
#include "headset_tts.h"
#include "headset_events.h"
#include "headset_tones.h"
#include "headset_statemanager.h"
#include "vm.h"

#include <stddef.h>
#include <csrtypes.h>
#include <audio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <csr_tone_plugin.h> 

#ifdef CSR_SIMPLE_TEXT_TO_SPEECH
    #include <csr_simple_text_to_speech_plugin.h>
#endif

#ifdef ENABLE_VOICE_PROMPTS
    #include <csr_voice_prompts_plugin.h>
#endif

#ifdef DEBUG_TTS
    #define TTS_DEBUG(x) DEBUG(x)
#else
    #define TTS_DEBUG(x) 
#endif

#ifdef NowSpeak
#define TTS_VOL_SCALING_PERCENT (65)
#else
#define TTS_VOL_SCALING_PERCENT (100)

#endif

/****************************************************************************
VARIABLES      
*/
#define TTS_CALLERID_NAME    (1000)
#define TTS_CALLERID_NUMBER  (1001)

/****************************************************************************
NAME 
DESCRIPTION
RETURNS    
*/

void TTSConfigureVoicePrompts( uint8 size_index, void *index, uint16 no_tts_languages )
{
#ifdef CSR_VOICE_PROMPTS
    TTS_DEBUG(("Setup VP Indexing: %d prompts\n",size_index));

#ifdef TEXT_TO_SPEECH_LANGUAGESELECTION
    theHeadset.no_tts_languages = no_tts_languages;
#endif

    AudioVoicePromptsInit((TaskData *)&csr_voice_prompts_plugin, size_index, index, no_tts_languages);
#endif /* CSR_VOICE_PROMPTS */
}

/****************************************************************************
NAME 
DESCRIPTION
RETURNS    
*/
bool TTSPlayEvent ( headsetEvents_t pEvent )
{  
#ifdef TEXT_TO_SPEECH_PHRASES
    uint16 lEventIndex = pEvent - EVENTS_MESSAGE_BASE ;
    HeadsetTts_t* ptr = theHeadset.audioData.gEventTTSPhrases;
	TaskData * task = NULL;
    bool event_match = FALSE;
	
#ifdef FULL_TEXT_TO_SPEECH
	#ifdef NowSpeak
	task = (TaskData *) &nowspeak_text_to_speech_plugin;	
	#else	
	task = (TaskData *) &INSERT_TEXT_TO_SPEECH_PLUGIN_HERE;	
	#endif
#endif
 
#ifdef ENABLE_VOICE_PROMPTS
    task = (TaskData *) &csr_voice_prompts_plugin;
#endif
    /* ensure there are valid TTS events defined */
    if(ptr)
    {
        uint16 state_mask = 1 << stateManagerGetState();
        
        while(ptr->tts_id != TTS_NOT_DEFINED)
        {           
            /* Play TTS if the event matches and we're not in a blocked state */
            if((ptr->event == lEventIndex) && (ptr->state_mask & state_mask) && !(ptr->sco_block && theHeadset.sco_sink))
            {
#ifdef BHC612
				if(stateManagerGetState() == headsetLimbo)
					TTS_DEBUG(("TTS : headsetLimbo\n"));
#endif
			
                TTS_DEBUG(("TTS: EvPl[%x][%x][%x][%x]\n", pEvent, lEventIndex, ptr->event, ptr->tts_id )) ;
				
                event_match = TRUE;
                switch(pEvent)
                {
                    case EventMuteReminder:
                    case TONE_TYPE_RING_1:
                    case TONE_TYPE_RING_2:
                
       				    /* never queue mute reminders to protect against the case that the prompt is longer 
                           than the mute reminder timer */
       	                AudioPlayTTS (  task,
                                        (uint16) ptr->tts_id, 
                                        NULL, 
                                        0, 
                                        theHeadset.tts_language, 
                                        FALSE, 
                                        theHeadset.codec_task, 
                                        (TonesGetToneVolume(FALSE)*TTS_VOL_SCALING_PERCENT)/100, 
                                        theHeadset.features.stereo/*0*/);
                    break;
                    default:
                    /*call the audio manager to play the tts phrase*/
                        AudioPlayTTS (  task,
                                        (uint16) ptr->tts_id, 
                                        NULL, 
                                        0, 
                                        theHeadset.tts_language, 
                                        TRUE, 
                                        theHeadset.codec_task, 
                                        (TonesGetToneVolume(FALSE)*TTS_VOL_SCALING_PERCENT)/100, 
                                        theHeadset.features.stereo/*0*/);
                    break;
                }
            }   
            ptr++;
        }
    }
    return event_match;
#else
    return FALSE;
#endif /* TEXT_TO_SPEECH_PHRASES */
} 

/****************************************************************************
NAME 
    TTSPlayNumString
DESCRIPTION
    Play a numeric string using the TTS plugin
RETURNS    
*/

void TTSPlayNumString(uint16 size_num_string, uint8* num_string)
{
#ifdef TEXT_TO_SPEECH_DIGITS
#ifdef FULL_TEXT_TO_SPEECH
	#ifdef NowSpeak
	TaskData * task = (TaskData *) &nowspeak_text_to_speech_plugin;
	#else	
	TaskData * task = (TaskData *) &INSERT_TEXT_TO_SPEECH_PLUGIN_HERE;
	#endif
#endif

#ifdef CSR_SIMPLE_TEXT_TO_SPEECH
	TaskData * task = (TaskData *) &csr_simple_text_to_speech_plugin;
#endif
    
#ifdef ENABLE_VOICE_PROMPTS
    TaskData * task = (TaskData *) &csr_voice_prompts_plugin;
#endif

	TTS_DEBUG(("Nowspaek TTS_CALLERID_Number\n"));

    AudioPlayTTS (  task, 
					TTS_CALLERID_NUMBER, 
					num_string, 
					size_num_string, 
					theHeadset.tts_language, 
					FALSE,  
					theHeadset.codec_task, 
					(TonesGetToneVolume(FALSE)*TTS_VOL_SCALING_PERCENT)/100, 
					theHeadset.features.stereo/*0*/);
#endif
}

/****************************************************************************
NAME 
    TTSPlayNumber
DESCRIPTION
    Play a uint32 using the TTS plugin
RETURNS    
*/

void TTSPlayNumber(uint32 number)
{
#ifdef TEXT_TO_SPEECH_DIGITS
    char num_string[7];
    sprintf(num_string, "%06ld",number);
    TTSPlayNumString(strlen(num_string), (uint8*)num_string);
#endif
}

/****************************************************************************
NAME 
DESCRIPTION
RETURNS    
*/

bool TTSPlayCallerNumber( const uint16 size_number, const uint8* number ) 
{
#ifdef TEXT_TO_SPEECH_DIGITS
	
	if(theHeadset.RepeatCallerIDFlag && size_number > 0) 
	{
		theHeadset.RepeatCallerIDFlag = FALSE;
		TTSPlayNumString(size_number, (uint8*)number);
        return TRUE;
	}
#endif
    return FALSE;
}

/****************************************************************************
NAME 
DESCRIPTION
RETURNS    
*/

bool TTSPlayCallerName( const uint16 size_name, const uint8* name ) 
{
#ifdef TEXT_TO_SPEECH_NAMES
	#ifdef NowSpeak
	TaskData * task = (TaskData *) &nowspeak_text_to_speech_plugin;
    #else
	TaskData * task = (TaskData *) &INSERT_TEXT_TO_SPEECH_PLUGIN_HERE;
	#endif
    
	if(size_name > 0) 
	{
		TTS_DEBUG(("Nowspaek TTS_CALLERID_Name\n"));
	
		AudioPlayTTS (  task, 
						TTS_CALLERID_NAME, 
						(uint8*)name, 
						size_name, 
						theHeadset.tts_language, 
						FALSE,  
						theHeadset.codec_task, 
						(TonesGetToneVolume(FALSE)*TTS_VOL_SCALING_PERCENT)/100, 
						theHeadset.features.stereo/*0*/);
        return TRUE;
	}
#endif
    return FALSE;
}

/****************************************************************************
NAME    
    TTSTerminate
    
DESCRIPTION
  	function to terminate a TTS prematurely.
    
RETURNS
    
*/
void TTSTerminate( void )
{
#ifdef TEXT_TO_SPEECH
#ifdef CSR_SIMPLE_TEXT_TO_SPEECH
	TaskData * task = (TaskData *) &csr_simple_text_to_speech_plugin;
#endif
	
#ifdef FULL_TEXT_TO_SPEECH
	#ifdef NowSpeak
	TaskData * task = (TaskData *) &nowspeak_text_to_speech_plugin;
	#else
	TaskData * task = (TaskData *) &INSERT_TEXT_TO_SPEECH_PLUGIN_HERE;
	#endif
#endif
	
#ifdef ENABLE_VOICE_PROMPTS
    TaskData * task = (TaskData *) &csr_voice_prompts_plugin;
#endif
    /* Do nothing if TTS Terminate Disabled */
    if(!theHeadset.features.DisableTTSTerminate)
    {
        /* Make sure we cancel any pending TTS */
        MessageCancelAll(task, AUDIO_PLUGIN_PLAY_TTS_MSG);
        /* Stop the current TTS phrase */
	    AudioStopTTS( task ) ;
		
	    theHeadset.RepeatCallerIDFlag = TRUE;
    }
#endif /* TEXT_TO_SPEECH */
}  
      
/****************************************************************************
NAME    
    TTSSelectTTSLanguageMode
    
DESCRIPTION
  	function to select a TTS language.
    
RETURNS
    
*/
void TTSSelectTTSLanguageMode( void )
{
#ifdef TEXT_TO_SPEECH_LANGUAGESELECTION

#ifdef FULL_TEXT_TO_SPEECH
	#ifdef NowSpeak
	TaskData * task = (TaskData *) /*&INSERT_TEXT_TO_SPEECH_PLUGIN_HERE*/ 0;
    return;
	#else
	TaskData * task = (TaskData *) &INSERT_TEXT_TO_SPEECH_PLUGIN_HERE;
	#endif
#endif
	
#ifdef ENABLE_VOICE_PROMPTS
    TaskData * task = (TaskData *) &csr_voice_prompts_plugin;
#endif

	theHeadset.tts_language ++;
    
	if(theHeadset.tts_language >= theHeadset.no_tts_languages)
        theHeadset.tts_language = 0;
    
    AudioPlayTTS (  task,
                    (uint16) 0, 
                    NULL, 
                    0, 
                    theHeadset.tts_language, 
                    TRUE, 
                    theHeadset.codec_task, 
                    TonesGetToneVolume(FALSE), 
                    theHeadset.features.stereo/*0*/);
#endif
}

