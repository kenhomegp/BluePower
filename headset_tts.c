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

#include <rubidium_text_to_speech_plugin.h> 
#ifdef CallerIDLedDebug
#include <led.h>
#endif

#if 1
	/*extern char		TTS_text[20];*/
	extern char		TTS_text[50];
	extern uint16	Language;
	extern const 	TaskData rubidium_tts_asr; 
    	extern /*E_RUNNING_OPTIONS*/uint16   Kalimba_mode;		/* current mode that Kalimba is running in 	*/
    	extern /*E_NODES*/uint16 			ASR_active_model;	/* current active menu node (.mdl file) for ASR  */
	extern bool		TipCounter;
	extern bool		RepeatCallerIDflag;
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
	task = (TaskData *) &rubidium_tts_asr;	
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
                TTS_DEBUG(("TTS: EvPl[%x][%x][%x][%x]\n", pEvent, lEventIndex, ptr->event, ptr->tts_id )) ;
                event_match = TRUE;
                switch(pEvent)
                {
                    case EventMuteReminder:
                    case TONE_TYPE_RING_1:
                    case TONE_TYPE_RING_2:
#if 1               
       				    /* never queue mute reminders to protect against the case that the prompt is longer 
                           than the mute reminder timer */
       	                AudioPlayTTS (  task,
                                        (uint16) ptr->tts_id, 
                                        NULL, 
                                        0, 
                                        theHeadset.tts_language, 
                                        FALSE, 
                                        theHeadset.codec_task, 
                                        TonesGetToneVolume(FALSE), 
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
                                        TonesGetToneVolume(FALSE), 
                                        theHeadset.features.stereo/*0*/);
						
                    break;
#endif				
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

extern AUDIO_t	AUDIO;
extern void soundClearDisconnect( void );
/****************************************************************************
NAME 
    TTSPlayNumString
DESCRIPTION
    Play a numeric string using the TTS plugin
RETURNS    
*/

void TTSPlayNumString(uint16 size_num_string, uint8* num_string)
{
	int i;  
/*#ifdef TEXT_TO_SPEECH_DIGITS*/
#ifdef CallerNumerFilter
	int j;
#endif

#if 0

#ifdef FULL_TEXT_TO_SPEECH
	TaskData * task = (TaskData *) &rubidium_tts_asr;
#endif

#ifdef CSR_SIMPLE_TEXT_TO_SPEECH
	TaskData * task = (TaskData *) &csr_simple_text_to_speech_plugin;
#endif
    
#ifdef ENABLE_VOICE_PROMPTS
    TaskData * task = (TaskData *) &csr_voice_prompts_plugin;
#endif
	
#endif	

#if 1    /* Temporarily removed @@@*/
	TaskData * task = (TaskData *) &rubidium_tts_asr;
	memset(TTS_text, 0, sizeof(TTS_text));	

	TTS_DEBUG(("Rubi > TTSPlayNumString...\n"));

#ifdef CallerNumerFilter
	j = 0;
	for(i=0;i<(int)size_num_string;i++)
	{
		if(((char)(num_string[i]) >= 0x30) && ((char)(num_string[i]) <= 0x39))
		{
			TTS_text[j]=(char)(num_string[i]);
			j++;
		}
	}
	TTS_text[j]=0;
#else
    	for(i=0;i<(int)size_num_string;i++)
       	TTS_text[i]=(char)(num_string[i]);
    	TTS_text[i]=0;
#endif

	#if 0
    AUDIO_BUSY	= NULL;
    AUDIO.plugin= NULL;
    UnloadRubidiumEngine();
    
    ToneTerminate();
    soundClearDisconnect();
	#endif
	
	TipCounter = 1;
	RepeatCallerIDflag = 0;
        
	#ifdef Barge_In_enable
    	Kalimba_mode 		= 4;  /*TTS_ASR_BARGE_IN_MODE*/
	#else
		Kalimba_mode 		= 3; /*TTS_ASR_MODE*/
	#endif
        
    ASR_active_model	= 0; /*RINGING_NODE*/

	#if 1
	theHeadset.TTS_ASR_Playing = true;
	#endif

    AudioPlayTTS(	task, 
					(uint16)0, 								/*TTS_CALLERID_NUMBER, */
					(uint8*)TTS_text,					 	/*num_string, */
					sizeof(TTS_text), 						/*size_num_string, */
					/*AUDIO_TTS_LANGUAGE_AMERICAN_ENGLISH,	theHeadset.tts_language, */
					Language,
					FALSE,									/*FALSE*/  
					theHeadset.codec_task,					/*theHeadset.codec_task, */
					0, 										/*TonesGetToneVolume(FALSE), */
					0										/*theHeadset.features.stereo	*/
				);
	#ifdef CallerIDLedDebug
		LedConfigure(LED_1, LED_ENABLE, 1 ) ;
		LedConfigure(LED_1, LED_DUTY_CYCLE, (0xfff));
		LedConfigure(LED_1, LED_PERIOD, 0 );
	#endif
#endif 
/*#endif*/
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
    /*sprintf(num_string, "%06ld",number);*/
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
   	/*DEBUG(("Arrived at TTSPlayCallerNumber..."));*/
	
/*#ifdef TEXT_TO_SPEECH_DIGITS*/
	
	/*if(theHeadset.RepeatCallerIDFlag && size_number > 0)*/ 
	if((theHeadset.RepeatCallerIDFlag) && (size_number > 0) && (theHeadset.TTS_ASR_Playing == false))
	{
		theHeadset.RepeatCallerIDFlag = FALSE;
		TTSPlayNumString(size_number, (uint8*)number);
          	DEBUG((" Completed.\n"));
        return TRUE;
	}
/*#endif*/
          	TTS_DEBUG((" skipped.\n"));
    return FALSE;
}

/****************************************************************************
NAME 
DESCRIPTION
RETURNS    
*/

bool TTSPlayCallerName( const uint16 size_name, const uint8* name ) 
{
   int i;
#ifdef ChineseTTS
	int j;
	#if 0
	char *temp = (char*) mallocPanic((int)(size_name) * sizeof(char));
	#endif
	uint16	k,TTS_Language;
	uint8	TTS_index;
	uint8	UTF_8;
#endif
	
#if 1    /* Temporarily removed @@@*/
	TaskData * task = (TaskData *) &rubidium_tts_asr;   
	memset(TTS_text, 0, sizeof(TTS_text));	

	/*#ifdef Rubi_Test	Example of UTF-8 chinese*/
	#ifdef ChineseTTS
	/*if((size_name > 0) && (Language == AUDIO_TTS_LANGUAGE_CHINESE_MANDARIN))*/
	if((theHeadset.RepeatCallerIDFlag) &&  (size_name > 0)  && (theHeadset.TTS_ASR_Playing == false))
	{
		theHeadset.RepeatCallerIDFlag = FALSE;

		TTS_Language = Language;

		TTS_DEBUG(("##Rubi >  TTSPlayCallerName...,%d\n",(int)size_name)); 

		/*if(Language == AUDIO_TTS_LANGUAGE_CHINESE_MANDARIN)*/
		if(theHeadset.BHC612_BattMeterOK)/*IPone , Unicode*/	
		{
			#if 0
			/*Convert string to int*/
			for(i=0;i<(int)size_name;i++)
			{
				*(temp+i) = (char)(name[i]);
				/*TTS_DEBUG(("Unicode = %c\n",*(temp+i)));*/
			}
			#endif

			k = 0;
			TTS_index = 0;

			for(j=0;j<(int)size_name;j++)
			{
				if((j *4) >=  (int)size_name)
					break;
			
				for(i=0;i<4;i++)
				{
					if(i == 0)
					{
						if(((char)(name[j*4+i]) >= 0x30) &&  ((char)(name[j*4+i]) <= 0x39))/*0 ~ 9*/
							k |= (((char)(name[j*4+i]) - 0x30) << 12);
						else if(((char)(name[j*4+i]) >= 0x41) &&  ((char)(name[j*4+i]) <= 0x5A))/*A ~ Z*/
							k |= (((char)(name[j*4+i]) - 0x37) << 12);
						
						#if 0
						if((*(temp+(j*4+i)) >= 0x30) &&  (*(temp+(j*4+i)) <= 0x39))/*0 ~ 9*/
							k |= ((*(temp+(j*4+i)) - 0x30) << 12);
						else if((*(temp+(j*4+i)) >= 0x41) &&  (*(temp+(j*4+(j*4+i))) <= 0x5A))/*A ~ Z*/
							k |= ((*(temp+(j*4+i)) - 0x37) << 12);
						#endif
					}
					else if(i == 1)
					{
						if(((char)(name[j*4+i]) >= 0x30) &&  ((char)(name[j*4+i]) <= 0x39))/*0 ~ 9*/
							k |= (((char)(name[j*4+i]) - 0x30) << 8);
						else if(((char)(name[j*4+i]) >= 0x41) &&  ((char)(name[j*4+i]) <= 0x5A))/*A ~ Z*/
							k |= (((char)(name[j*4+i]) - 0x37) << 8);
						
						#if 0
						if((*(temp+(j*4+i)) >= 0x30) &&  (*(temp+(j*4+i)) <= 0x39))
							k |= ((*(temp+(j*4+i)) - 0x30) << 8);
						else if((*(temp+(j*4+i)) >= 0x41) &&  (*(temp+(j*4+i)) <= 0x5A))
							k |= ((*(temp+(j*4+i)) - 0x37) << 8);
						#endif
					}
					else if(i == 2)
					{
						if(((char)(name[j*4+i]) >= 0x30) &&  ((char)(name[j*4+i]) <= 0x39))/*0 ~ 9*/
							k |= (((char)(name[j*4+i]) - 0x30) << 4);
						else if(((char)(name[j*4+i]) >= 0x41) &&  ((char)(name[j*4+i]) <= 0x5A))/*A ~ Z*/
							k |= (((char)(name[j*4+i]) - 0x37) << 4);
						
						#if 0
						if((*(temp+(j*4+i)) >= 0x30) &&  (*(temp+(j*4+i)) <= 0x39))
							k |= ((*(temp+(j*4+i)) - 0x30) << 4);
						else if((*(temp+(j*4+i)) >= 0x41) &&  (*(temp+(j*4+i)) <= 0x5A))
							k |= ((*(temp+(j*4+i)) - 0x37) << 4);
						#endif
					}
					else if(i == 3)
					{
						if(((char)(name[j*4+i]) >= 0x30) &&  ((char)(name[j*4+i]) <= 0x39))/*0 ~ 9*/
							k |= (((char)(name[j*4+i]) - 0x30));
						else if(((char)(name[j*4+i]) >= 0x41) &&  ((char)(name[j*4+i]) <= 0x5A))/*A ~ Z*/
							k |= (((char)(name[j*4+i]) - 0x37));
						
						#if 0
						if((*(temp+(j*4+i)) >= 0x30) &&  (*(temp+(j*4+i)) <= 0x39))
							k |= ((*(temp+(j*4+i)) - 0x30));
						else if((*(temp+(j*4+i)) >= 0x41) &&  (*(temp+(j*4+i)) <= 0x5A))
							k |= ((*(temp+(j*4+i)) - 0x37));
						#endif

						if(k < 128)
						{							
							TTS_DEBUG(("ASCII = %X\n",k));
						}
						else
						{
							TTS_DEBUG(("Unicode = %X\n",k));
						}
						
						/*Convert Unicode(2 bytes) to UTF-8(3 bytes)*/
						/*if((k >= 0x800))*/
						if(k >= 0x4E00)	/*Chinese Unicode : 0x4E00 ~ 0x9FBF*/ 	
						{						
							UTF_8 = 0xe0 | (k >> 12);
							TTS_text[TTS_index] = UTF_8;
							TTS_index++;						
							TTS_DEBUG(("UTF-8 = %X",UTF_8));
							UTF_8 = 0x80 | ((k >> 6)&0x3f);
							TTS_text[TTS_index] = UTF_8;
							TTS_index++;
							TTS_DEBUG(("%X",UTF_8));
							UTF_8 = 0x80 | (k & 0x3f);
							TTS_text[TTS_index] = UTF_8;
							TTS_index++;
							TTS_DEBUG(("%X\n",UTF_8));

							if(Language == AUDIO_TTS_LANGUAGE_AMERICAN_ENGLISH)
							{
								#if 0
								TTS_Language = AUDIO_TTS_LANGUAGE_CHINESE_MANDARIN;
								theHeadset.BHC612_Rubi_Lan_Changed = TRUE;
								#else
								/*TTSPlayCallerNumber*/
								theHeadset.BHC612_Rubi_Lan_Changed = FALSE;
								theHeadset.RepeatCallerIDFlag = TRUE;
								return FALSE;
								#endif
							}
						}
						else
						{
							if(((k >= 0x41) && (k <= 0x5A)) || ((k >= 0x61) && (k <= 0x7A)))	/*Find A ~ Z , a ~ z*/
							{
								TTS_text[TTS_index] = k;
								TTS_index++;
								
								if(Language == AUDIO_TTS_LANGUAGE_CHINESE_MANDARIN)
								{
									#if 0
									TTS_Language = AUDIO_TTS_LANGUAGE_AMERICAN_ENGLISH;
									theHeadset.BHC612_Rubi_Lan_Changed = TRUE;
									#else
									/*TTSPlayCallerNumber*/
									theHeadset.BHC612_Rubi_Lan_Changed = FALSE;
									theHeadset.RepeatCallerIDFlag = TRUE;
									return FALSE;
									#endif
								}

								#if 0
								free(temp);
								theHeadset.RepeatCallerIDFlag = TRUE;
								TTS_DEBUG(("####TTSPlayCallerName end!\n"));
								return FALSE;
								#endif
							}			
							
						}
						
						k = 0;					
					}
				}
			}

			TTS_text[TTS_index] = 0;
			TTS_index++;
			TTS_DEBUG(("TTS_text = %d\n",TTS_index));

			#if 0
			free(temp);		
			#endif
		}
		/*else if(Language == AUDIO_TTS_LANGUAGE_AMERICAN_ENGLISH)*/
		else
		{
			for(i=0;i<(int)size_name;i++)
				TTS_text[i]=(char)(name[i]);
			TTS_text[i]=0;

			TTS_index = (int)size_name;
		}
				
		#if 0
		AUDIO_BUSY	= NULL;
		AUDIO.plugin= NULL;
		TTSTerminate();
		UnloadRubidiumEngine();
		ToneTerminate();		
		soundClearDisconnect();
		#endif
		
		TipCounter = 1;
		RepeatCallerIDflag = 0;
	
		#ifdef Barge_In_enable
			Kalimba_mode		= 4;  /*TTS_ASR_BARGE_IN_MODE*/
		#else
			Kalimba_mode		= 3; /*TTS_ASR_MODE*/
		#endif
	
		ASR_active_model	= 0; /*RINGING_NODE*/
	
		#if 1
		theHeadset.TTS_ASR_Playing = true;
		#endif

		if(Language != TTS_Language)
		{
			/*Testing!!!
			memset(TTS_text, 0, sizeof(TTS_text));  
			strcpy(TTS_text,"Alex");
			*/
			
			Language = TTS_Language;
			
			TTS_DEBUG(("###Special case...2,%d\n",TTS_Language));
			AudioPlayTTS(	task, 
						(uint16)0,								/*TTS_CALLERID_NUMBER, */
						(uint8*)TTS_text,						/*num_string, */
						sizeof(TTS_text),						/*size_num_string, */
						/*AUDIO_TTS_LANGUAGE_AMERICAN_ENGLISH,	theHeadset.tts_language, */
						Language,
						FALSE,									/*FALSE*/  
						theHeadset.codec_task,					/*theHeadset.codec_task, */
						0,										/*TonesGetToneVolume(FALSE), */
						0										/*theHeadset.features.stereo	*/
						);

		}
		else
		{
			AudioPlayTTS(	task, 
						(uint16)0,								/*TTS_CALLERID_NUMBER, */
						(uint8*)TTS_text,						/*num_string, */
						sizeof(TTS_text),						/*size_num_string, */
						/*AUDIO_TTS_LANGUAGE_AMERICAN_ENGLISH,	theHeadset.tts_language, */
						Language,
						FALSE,									/*FALSE*/  
						theHeadset.codec_task,					/*theHeadset.codec_task, */
						0,										/*TonesGetToneVolume(FALSE), */
						0										/*theHeadset.features.stereo	*/
						);
		}
	
		DEBUG((" Completed.\n"));
		return TRUE;
	}
	#else	/*#ifdef ChineseTTSx*/
 	/*if(theHeadset.RepeatCallerIDFlag &&  size_name > 0)*/
	if((theHeadset.RepeatCallerIDFlag) && (size_name > 0) && (theHeadset.TTS_ASR_Playing == false))
	{
        theHeadset.RepeatCallerIDFlag = FALSE;

	 TTS_DEBUG(("***  TTSPlayCallerName...,%d\n",(int)size_name)); 

        for(i=0;i<(int)size_name;i++)
            TTS_text[i]=(char)(name[i]);
        TTS_text[i]=0;

	   	#if 0
        AUDIO_BUSY	= NULL;
        AUDIO.plugin= NULL;		
		TTSTerminate();		
        UnloadRubidiumEngine();        
        ToneTerminate();		
        soundClearDisconnect();
		#endif

		TipCounter = 1;
		RepeatCallerIDflag = 0;

		#ifdef Barge_In_enable
    		Kalimba_mode 		= 4;  /*TTS_ASR_BARGE_IN_MODE*/
		#else
			Kalimba_mode 		= 3; /*TTS_ASR_MODE*/
		#endif
        
        ASR_active_model	= 0; /*RINGING_NODE*/

		#if 1
		theHeadset.TTS_ASR_Playing = true;
		#endif
		
        AudioPlayTTS(	task, 
    					(uint16)0, 								/*TTS_CALLERID_NUMBER, */
    					(uint8*)TTS_text,					 	/*num_string, */
    					sizeof(TTS_text), 						/*size_num_string, */
    					/*AUDIO_TTS_LANGUAGE_AMERICAN_ENGLISH,	theHeadset.tts_language, */
						Language,
						FALSE,									/*FALSE*/  
    					theHeadset.codec_task,					/*theHeadset.codec_task, */
    					0, 										/*TonesGetToneVolume(FALSE), */
    					0										/*theHeadset.features.stereo	*/
    				);

 
       	DEBUG((" Completed.\n"));		
        return TRUE;
	}
    #endif    
#endif   
   	TTS_DEBUG((" skipped.\n"));
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
/*#ifdef TEXT_TO_SPEECH*/
#ifdef CSR_SIMPLE_TEXT_TO_SPEECH
	TaskData * task = (TaskData *) &csr_simple_text_to_speech_plugin;
#endif
	
#ifdef FULL_TEXT_TO_SPEECH
	TaskData * task = (TaskData *) &rubidium_tts_asr;
#endif
	
#ifdef ENABLE_VOICE_PROMPTS
    TaskData * task = (TaskData *) &csr_voice_prompts_plugin;
#endif
    /* Do nothing if TTS Terminate Disabled */
	TaskData * task = (TaskData *) &rubidium_tts_asr;
    if(!theHeadset.features.DisableTTSTerminate)
    {
        /* Make sure we cancel any pending TTS */
        MessageCancelAll(task, AUDIO_PLUGIN_PLAY_TTS_MSG);
        /* Stop the current TTS phrase */
	    AudioStopTTS( task ) ;
		
	    theHeadset.RepeatCallerIDFlag = TRUE;
    }
/*#endif*/ /* TEXT_TO_SPEECH */
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
	TaskData * task = (TaskData *) &rubidium_tts_asr;
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
                    theHeadset.features.stereo);
#endif
}

