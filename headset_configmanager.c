/****************************************************************************
FILE NAME
    headset_configmanager.c
    
DESCRIPTION
    Configuration manager for the headset - resoponsible for extracting user information out of the 
    PSKEYs and initialising the configurable nature of the headset components
    
*/

#include "headset_configmanager.h"
#include "headset_config.h"
#include "headset_buttonmanager.h"
#include "headset_led_manager.h"
#include "headset_statemanager.h"
#include "headset_powermanager.h"
#include "headset_volume.h"

#include "headset_private.h"
#include "headset_events.h"
#include "headset_tones.h"
#include "headset_tts.h"
#include "headset_audio.h"

#include "headset_pio.h"

#include <csrtypes.h>
#include <ps.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <panic.h>

#ifdef DEBUG_CONFIG
#define CONF_DEBUG(x) DEBUG(x)
#else
#define CONF_DEBUG(x) 
#endif

#define HCI_PAGESCAN_INTERVAL_DEFAULT  (0x800)
#define HCI_PAGESCAN_WINDOW_DEFAULT   (0x12)
#define HCI_INQUIRYSCAN_INTERVAL_DEFAULT  (0x800)
#define HCI_INQUIRYSCAN_WINDOW_DEFAULT   (0x12)


#define DEFAULT_VOLUME_MUTE_REMINDER_TIME_SEC 10

#ifdef Rubidium
extern uint16	Language;
#endif

/****************************************************************************
NAME 
  	configManagerKeyLengths

DESCRIPTION
 	Read the lengths of ps keys for tts, tone and led configuration
 
RETURNS
  	void
*/ 
static void configManagerKeyLengths(uint16 configID, lengths_config_type * pLengths )
{		
	ConfigRetrieve(configID , PSKEY_LENGTHS, pLengths, sizeof(lengths_config_type));
		
	DEBUG(("CONF: LENGTHS [%x][%x][%x][%x][%x][%x]\n" , pLengths->no_tts,
											pLengths->no_tts_languages,
											pLengths->no_led_filter,
											pLengths->no_led_states,
											pLengths->no_led_events,
											pLengths->no_tones )) ;
											
#ifdef ROM_LEDS
	/* Set led lengths straight away */											
	theHeadset.theLEDTask->gStatePatternsAllocated = pLengths->no_led_states;
	theHeadset.theLEDTask->gEventPatternsAllocated = pLengths->no_led_events;
	theHeadset.theLEDTask->gLMNumFiltersUsed = pLengths->no_led_filter;
#endif
}

/****************************************************************************
NAME 
  	configManagerButtons

DESCRIPTION
 	Read the system event configuration from persistent store and configure
  	the buttons by mapping the associated events to them
 
RETURNS
  	void
*/  

static void configManagerButtons( uint16 pConfigID )
{  
	/* Allocate enough memory to hold event configuration */
    event_config_type* configA = (event_config_type*) mallocPanic(BM_EVENTS_PER_BLOCK * sizeof(event_config_type));
    event_config_type* configB = (event_config_type*) mallocPanic(BM_EVENTS_PER_BLOCK * sizeof(event_config_type));
   
    uint16 n;
    uint8  i = 0;
 
    ConfigRetrieve(pConfigID , PSKEY_EVENTS_A, configA, BM_EVENTS_PER_BLOCK * sizeof(event_config_type)) ;
    ConfigRetrieve(pConfigID , PSKEY_EVENTS_B, configB, BM_EVENTS_PER_BLOCK * sizeof(event_config_type)) ; 
  
        /* Now we have the event configuration, map required events to system events */
    for(n = 0; n < BM_EVENTS_PER_BLOCK; n++)
    { 
        CONF_DEBUG(("Co : AddMap Ev[%x][%x]\n", configA[n].event , configB[n].event )) ;
                       
           /* check to see if a valid pio mask is present, this includes the upper 2 bits of the state
              info as these are being used for bc5 as vreg enable and charger detect */
        if ( (configA[n].pio_mask)||(configA[n].state_mask & 0xC000))
            buttonManagerAddMapping ( &configA[n], i++ ); 
               
        if ( (configB[n].pio_mask)||(configB[n].state_mask & 0xC000))
            buttonManagerAddMapping ( &configB[n], i++ ); 
                                                           
   	}
    

    free(configA) ;
    free(configB) ; 
    
    /* perform an initial pio check to see if any pio changes need processing following the completion
       of the configuration ps key reading */
    BMCheckButtonsAfterReadingConfig();


}
/****************************************************************************
NAME 
  	configManagerButtonPatterns

DESCRIPTION
  	Read and configure any buttonpattern matches that exist
 
RETURNS

*/
static void configManagerButtonPatterns( uint16 pConfigID ) 
{  
      		/* Allocate enough memory to hold event configuration */
    button_pattern_config_type* config = (button_pattern_config_type*) mallocPanic(BM_NUM_BUTTON_MATCH_PATTERNS * sizeof(button_pattern_config_type));
   
    CONF_DEBUG(("Co: No Button Patterns - %d\n", BM_NUM_BUTTON_MATCH_PATTERNS));
   
        /* Now read in event configuration */
    if(config)
    {     
        if(ConfigRetrieve(pConfigID , PSKEY_BUTTON_PATTERN_CONFIG, config, BM_NUM_BUTTON_MATCH_PATTERNS * sizeof(button_pattern_config_type)))
        {
            uint16 n;
     
           /* Now we have the event configuration, map required events to system events */
            for(n = 0; n < BM_NUM_BUTTON_MATCH_PATTERNS ; n++)
            {	 
     	      CONF_DEBUG(("Co : AddPattern Ev[%x]\n", config[n].event )) ;

			  #ifdef BHC612
				if(n == 0)
				{
					config[0].event = 0x6000;
					config[0].pattern[0] = 2;/*Vol - */
					config[0].pattern[1] = 4;/*Talk*/
					config[0].pattern[2] = 1;/*Vol +*/
					config[0].pattern[3] = 6;/*Vol - & Talk*/
					config[0].pattern[4] = 5;/*Vol + & Talk*/
					config[0].pattern[5] = 2;/*Vol -*/
				}
			  #endif
                        
          			   /* Map PIO button event to system events in specified states */
          	    buttonManagerAddPatternMapping ( theHeadset.theButtonsTask , config[n].event , config[n].pattern, n ) ;
            }
        }
        else
 	    {
 	      CONF_DEBUG(("Co: !EvLen\n")) ;
        }
        free (config) ;
    }    
}

#ifdef ROM_LEDS





/****************************************************************************
NAME 
  	configManagerLEDS

DESCRIPTION
  	Read the system LED configuration from persistent store and configure
  	the LEDS 
 
RETURNS
  	void
*/ 
static void configManagerLEDS( uint16 pConfigID )
{ 
  	/* 1. LED state configuration */
    	
    /* Providing there are states to configure */
    if((theHeadset.theLEDTask->gStatePatternsAllocated > 0) && (theHeadset.theLEDTask->gStatePatternsAllocated < HEADSET_NUM_STATES))
    {      		
   		ConfigRetrieve(pConfigID , PSKEY_LED_STATES, &theHeadset.theLEDTask->gStatePatterns, theHeadset.theLEDTask->gStatePatternsAllocated * sizeof(LEDPattern_t));
  	}      	    
    
    /* 2. LED event configuration */
      
    /* Providing there are events to configure */
    if((theHeadset.theLEDTask->gEventPatternsAllocated > 0) && (theHeadset.theLEDTask->gEventPatternsAllocated < EVENTS_MAX_EVENTS))
    {      		
   		ConfigRetrieve(pConfigID , PSKEY_LED_EVENTS, &theHeadset.theLEDTask->gEventPatterns, theHeadset.theLEDTask->gEventPatternsAllocated * sizeof(LEDPattern_t));
  	} 	    

    /* 3. LED event filter configuration */
	
    /* Providing there are states to configure */
    if((theHeadset.theLEDTask->gLMNumFiltersUsed > 0) && (theHeadset.theLEDTask->gLMNumFiltersUsed < LM_NUM_FILTER_EVENTS))
    {                    
           /* read from ps straight into filter config structure */
    	ConfigRetrieve(pConfigID , PSKEY_LED_FILTERS, &theHeadset.theLEDTask->gEventFilters, theHeadset.theLEDTask->gLMNumFiltersUsed * sizeof(LEDFilter_t));
    }
     
    /*tri colour behaviour*/  	
  	ConfigRetrieve(pConfigID , PSKEY_TRI_COL_LEDS, &theHeadset.theLEDTask->gTriColLeds,  sizeof(uint16)) ; 	
  	DEBUG(("CONF: TRICOL [%x][%x][%x]\n" , theHeadset.theLEDTask->gTriColLeds.TriCol_a ,
  										   theHeadset.theLEDTask->gTriColLeds.TriCol_b ,
  										   theHeadset.theLEDTask->gTriColLeds.TriCol_c )) ;
    
    
}

#endif
/****************************************************************************
NAME 
  	configManagerFeatureBlock

DESCRIPTION
  	Read the system feature block and configure system accordingly
 
RETURNS
  	void
*/
static void configManagerFeatureBlock( uint16 pConfigID ) 
{
    uint8 i;
	
    
    /* Read the feature block from persistent store */
  	ConfigRetrieve(pConfigID , PSKEY_FEATURE_BLOCK, &theHeadset.features, sizeof(feature_config_type)) ; 	
	
	/*Set the default volume level*/
    for(i=0;i<MAX_PROFILES;i++)
    {
        theHeadset.profile_data[i].audio.gSMVolumeLevel = theHeadset.features.DefaultVolume ;  
    }    
}


/****************************************************************************
NAME 
  	configManagerUserDefinedTones

DESCRIPTION
  	Attempt to read the user configured tones, if data exists it will be in the following format:
    
    uint16 offset in array to user tone 1,
    uint16 offset in array to user tone ....,
    uint16 offset in array to user tone 8,
    uint16[] user tone data
    
    To play a particular tone it can be access via gVariableTones, e.g. to access tone 1
    
    theHeadset.audioData.gConfigTones->gVariableTones[0] + (uint16)*theHeadset.audioData.gConfigTones->gVariableTones[0]
    
    or to access tone 2

    theHeadset.audioData.gConfigTones->gVariableTones[0] + (uint16)*theHeadset.audioData.gConfigTones->gVariableTones[1]
    
    and so on
 
RETURNS
  	void
*/
static void configManagerUserDefinedTones( uint16 pConfigID, uint16 KeyLength ) 
{
    /* if the keyLength is zero there are no user tones so don't malloc any memory */
    if(KeyLength)
    {
        /* malloc only enough memory to hold the configured tone data */
        uint16 * configTone = mallocPanic(KeyLength * sizeof(uint16));
    
        /* retrieve pskey data up to predetermined max size */
        uint16 size_ps_key = ConfigRetrieve( pConfigID, PSKEY_CONFIG_TONES , configTone , KeyLength );
      
        CONF_DEBUG(("Co : Configurable Tones Malloc size [%x]\n", KeyLength )) ;
        
        /* is there any configurable tone data, if present update pointer to tone data */
        if (size_ps_key)
        {
            /* the data is in the form of 8 x uint16 audio note start offsets followed by the 
               up to 8 lots of tone data */
            theHeadset.audioData.gConfigTones->gVariableTones = (ringtone_note*)&configTone[0];
        
        }
        /* no user configured tone data is available, so free previous malloc as not in use */
        else
        {
            /* no need to waste memory */
        	free(configTone);      
        }
    }
    /* no tone data available, ensure data pointer is null */
    else
    {
            theHeadset.audioData.gConfigTones->gVariableTones = NULL;
    }
}

/****************************************************************************
NAME 
  configManagerTimeouts

DESCRIPTION
  Read and configure the automatic switch off time if the range allows
    Also now handles the power on timeout
 
RETURNS
  void
*/ 
static void configManagerConfiguration ( uint16 pConfigID )
{	
    ConfigRetrieve(pConfigID , PSKEY_PIO_BLOCK, theHeadset.PIO, sizeof(PIO_block_t));
	ConfigRetrieve(pConfigID , PSKEY_INPUT_PIO_BLOCK, &theHeadset.conf->input_PIO, sizeof(input_pio_config_type));
	ConfigRetrieve(pConfigID , PSKEY_TIMEOUTS, (void*)&theHeadset.conf->timeouts, sizeof(Timeouts_t) ) ;
	ConfigRetrieve(pConfigID , PSKEY_RSSI_PAIRING, (void*)&theHeadset.conf->rssi, sizeof(rssi_pairing_t) ) ;
	ConfigRetrieve(pConfigID , PSKEY_FILTER, (void*)&theHeadset.conf->filter, sizeof(filter_t) ) ;
}

/****************************************************************************
NAME 
  	configManagerButtonDurations

DESCRIPTION
  	Read the button configuration from persistent store and configure
  	the button durations
 
RETURNS
  	void
*/ 
static void configManagerButtonDurations( uint16 pConfigID )
{
    if(ConfigRetrieve(pConfigID , PSKEY_BUTTON_CONFIG, theHeadset.buttons_duration, sizeof(button_config_type)))
 	{
			/*buttonmanager keeps buttons block*/
  		buttonManagerConfigDurations ( theHeadset.theButtonsTask ,  theHeadset.buttons_duration ); 
    }
}
    

/****************************************************************************
NAME 
 	configManagerPower

DESCRIPTION
 	Read the Power Manager configuration
 
RETURNS
 	void
*/ 
static void configManagerPower( uint16 pConfigID )
{
 	/* 1. Read in the battery monitoring configuration */
	ConfigRetrieve(pConfigID , PSKEY_BATTERY_CONFIG, (void*)&((theHeadset.conf->power).battery_config), sizeof(power_config_type) ) ;
   
  	/* 2. Setup the power manager */
    powerManagerConfig(&theHeadset.conf->power);
}

/****************************************************************************
NAME 
  	configManagerRadio

DESCRIPTION
  	Read the Radio configuration parameters
 
RETURNS
  	void
*/ 

static void configManagerRadio( uint16 pConfigID )
{ 
  	if(!ConfigRetrieve(pConfigID , PSKEY_RADIO_CONFIG, theHeadset.radio, sizeof(radio_config_type)))
  	{
    	/* Assume HCI defaults */
    	theHeadset.radio->page_scan_interval = HCI_PAGESCAN_INTERVAL_DEFAULT;
    	theHeadset.radio->page_scan_window = HCI_PAGESCAN_WINDOW_DEFAULT;
    	theHeadset.radio->inquiry_scan_interval = HCI_INQUIRYSCAN_INTERVAL_DEFAULT;
    	theHeadset.radio->inquiry_scan_interval = HCI_INQUIRYSCAN_WINDOW_DEFAULT;
  	}
}

/****************************************************************************
NAME 
  	configManagerVolume

DESCRIPTION
  	Read the Volume Mappings and set them up
 
RETURNS
  	void
*/ 
static void configManagerVolume( uint16 pConfigID )
{
    uint16 lSize = (VOL_NUM_VOL_SETTINGS * sizeof(VolMapping_t) );
    
    ConfigRetrieve(pConfigID , PSKEY_SPEAKER_GAIN_MAPPING, theHeadset.audioData.gVolMaps, lSize) ;
}

/****************************************************************************
NAME 
  	configManagerReset

DESCRIPTION
    Resets the Paired Device List - Reboots if PSKEY says to do so
 
RETURNS
  	void
*/ 

void configManagerReset( void ) 
{
    CONF_DEBUG(("CO: Reset\n")) ;
       
        /* Delete the Connecction Libs Paired Device List */
    ConnectionSmDeleteAllAuthDevices(ATTRIBUTE_PSKEY_BASE);
	
}
/****************************************************************************
NAME 
  	configManagerEventTone

DESCRIPTION
  	Configure an event tone only if one is defined
 
RETURNS
  	void
*/ 
static void configManagerEventTones( uint16 pConfigID, uint16 no_tones )
{ 
  	/* First read the number of events configured */	
  	if(no_tones)		
  	{
        /* Now read in tones configuration */
        if(ConfigRetrieve(pConfigID , PSKEY_TONES, theHeadset.audioData.gEventTones, no_tones * sizeof(tone_config_type)))
        {
			/*set the last tone (empty) - used by the play tone routine to identify the last tone*/
			theHeadset.audioData.gEventTones[no_tones].tone = TONE_NOT_DEFINED ;			
        }  
     }                    
}            



/****************************************************************************
NAME 
  	configManagerEventTone

DESCRIPTION
  	Configure an event tts phrase only if one is defined
 
RETURNS
  	void
*/ 
static void configManagerEventTTSPhrases( uint16 pConfigID , uint16 no_tts )
{
#ifdef TEXT_TO_SPEECH_PHRASES
  	/* cheack the number of events configured and the supported tts languages */
	if(no_tts)
  	{
		tts_config_type * config;
		
        /* Allocate enough memory to hold event configuration */
    	config = (tts_config_type *) mallocPanic((no_tts + 1) * sizeof(tts_config_type));

    	if (config)
    	{
         	/* Now read in tones configuration */
        	if(ConfigRetrieve(pConfigID , PSKEY_TTS, config, no_tts * sizeof(tts_config_type)))
        	{
          		theHeadset.audioData.gEventTTSPhrases = (HeadsetTts_t*)config;
                theHeadset.audioData.gEventTTSPhrases[no_tts].tts_id = TTS_NOT_DEFINED;
            }
        }
    }
#endif
}



static void configManagerVoicePromptsInit( uint16 pConfigID , uint16 no_vp , uint16 no_tts_languages )
{
#ifdef CSR_VOICE_PROMPTS
  	/* cheack the number of events configured and the supported tts languages */
	if(no_vp)		
  	{
		vp_config_type * config = NULL;
        uint16 size_vp_config = sizeof(vp_config_type);
        
        /* Allocate enough memory to hold event configuration */
        config = (vp_config_type *) mallocPanic(size_vp_config);
        
    	if (config)
    	{
            /* Read in the PSKEY that tells us where the prompt header is */
        	if(ConfigRetrieve(pConfigID , PSKEY_VOICE_PROMPTS, config, size_vp_config))
            {
                TTSConfigureVoicePrompts(no_vp, config, no_tts_languages);
            }
        }
    }
#endif /* CSR_VOICE_PROMPTS */
}



/****************************************************************************
NAME 
 	InitConfigMemory_1

DESCRIPTION
    Dynamic allocate memory slots.  
    Memory slots available to the application are limited, so store multiple configuration items in one slot.
 
RETURNS
 	void
*/ 

static void InitConfigMemory_1(lengths_config_type * keyLengths)
{
    uint16 lSize = 0;
    uint16 *buffer;
    uint16 pos=0, pos1=0, pos2;
	uint16 no_tones = keyLengths->no_tones;
    
	/*** One memory slot ***/
	/* Allocate memory for Button data and volume mapping */
    pos  = sizeof(button_config_type);
    pos1 = pos + sizeof(ConfigTone_t);
    lSize = pos1 + (VOL_NUM_VOL_SETTINGS * sizeof(VolMapping_t) ); 
    
	buffer = mallocPanic( lSize );
	CONF_DEBUG(("INIT: Malloc size 1: [%d]\n",lSize));
    
	/* Store pointer to button task memory */
	theHeadset.buttons_duration = (button_config_type *)&buffer[0];
    /* The config tone */
	theHeadset.audioData.gConfigTones = (ConfigTone_t *)&buffer[pos];
	/* The volume configuaration */
	theHeadset.audioData.gVolMaps = (VolMapping_t *)&buffer[pos1];
    
    /* ***************************************************** */
    /*** One memory slot ***/
	/* Allocate memory for SSR data and Event Tone */
    /* Added PIO block and radio */
    pos = sizeof(subrate_t);
    pos1 = pos + ((no_tones +1) * sizeof(tone_config_type));
    pos2 = pos1 + sizeof(PIO_block_t);
    lSize = pos2 + sizeof(radio_config_type);
    
    buffer = mallocPanic( lSize );
 	CONF_DEBUG(("INIT: Malloc size 2: [%d]\n",lSize));   

    /* Store pointer to SSR data */
    theHeadset.ssr_data = (subrate_t *)&buffer[0];
    /* Hold event tone configuration */
    theHeadset.audioData.gEventTones = (HeadsetTone_t*)&buffer[pos];
    /* PIOs assigned to fixed events */
    theHeadset.PIO = (PIO_block_t*)&buffer[pos1];
    /* radio configuration data */
    theHeadset.radio = (radio_config_type*)&buffer[pos2];
}



/****************************************************************************
NAME 
 	configManagerHFP_SupportedFeatures

DESCRIPTION
    Gets the HFP Supported features set from PS
 
RETURNS
 	void
*/
void configManagerHFP_SupportedFeatures( void )
{
    uint16 lConfigID  = get_config_id ( PSKEY_CONFIGURATION_ID ) ;
       
    ConfigRetrieve( lConfigID , PSKEY_ADDITIONAL_HFP_SUPPORTED_FEATURES , &theHeadset.HFP_supp_features , sizeof( HFP_features_type ) ) ;

    if ( ! theHeadset.HFP_supp_features.HFP_Version )
    {
            /*then set the defaults*/
        theHeadset.HFP_supp_features.HFP_Version = headset_hfp_version_1_0 ;
        theHeadset.HFP_supp_features.supportedSyncPacketTypes = (sync_hv1 | sync_hv2 | sync_hv3) ;        
    }
    else
    {
           /*make sure the mask we have read in is within range*/
        uint16 lAllPktTypes = ( sync_hv1  | sync_hv2  |  sync_hv3 | 
                                sync_ev3  | sync_ev4  |  sync_ev5 | 
                                sync_2ev3 | sync_3ev3 | sync_2ev5 | sync_3ev5 ) ;
                                 
        uint16 lPSPktTypes = theHeadset.HFP_supp_features.supportedSyncPacketTypes ;
        
        CONF_DEBUG(("CONF: Feat[%x]&[%x] = [%x]\n" , lAllPktTypes , lPSPktTypes , (lAllPktTypes & lPSPktTypes) )) ;
        theHeadset.HFP_supp_features.supportedSyncPacketTypes =  (lAllPktTypes & lPSPktTypes) ;
    }
}

/****************************************************************************
NAME 
 	configManagerHFP_Init

DESCRIPTION
    Gets the HFP initialisation parameters from PS
 
RETURNS
 	void
*/
void configManagerHFP_Init( hfp_init_params * hfp_params )
{     
    uint16 lConfigID  = get_config_id ( PSKEY_CONFIGURATION_ID ) ;

    ConfigRetrieve( lConfigID , PSKEY_HFP_INIT , hfp_params , sizeof( hfp_init_params ) ) ;
}

/*************************************************************************
NAME    
    ConfigManagerSetupSsr
    
DESCRIPTION
    This function attempts to retreive SSR parameters from the PS, setting
	them to zero if none are found

RETURNS

*/
static void configManagerSetupSsr( uint16 pConfigID  ) 
{
	CONF_DEBUG(("CO: SSR\n")) ;

	/* Get the SSR params from the PS/Config if there */
	if(!ConfigRetrieve(pConfigID , PSKEY_SSR_PARAMS, theHeadset.ssr_data, sizeof(subrate_t)))
	{
		CONF_DEBUG(("CO: SSR Defaults\n")) ;
		/* If we failed to read in params then set the pointer to NULL */
		theHeadset.ssr_data = NULL;
	}
}

/****************************************************************************
NAME 
 	configManagerReadSessionData
*/ 
static void configManagerReadSessionData( uint16 pConfig ) 
{
    session_data_type lTemp ;
   
    /*read in the volume orientation*/
    ConfigRetrieve( pConfig , PSKEY_VOLUME_ORIENTATION , &lTemp , sizeof( uint16 ) ) ;
  
    theHeadset.VolumeOrientationIsInverted = lTemp.vol_orientation ;
#ifdef ROM_LEDS
    /* if the feature bit to reset led disable state after a reboot is set then enable the leds
       otherwise get the led enable state from ps */
    if(!theHeadset.features.ResetLEDEnableStateAfterReset)
    {
        theHeadset.theLEDTask->gLEDSEnabled    = lTemp.led_disable ;
    }
    else
    {
        theHeadset.theLEDTask->gLEDSEnabled    = TRUE;   
    }
#endif	

#ifdef Rubidium
	
	/*theHeadset.Rubi_enable : 0 /1 (Rubidium TTS/ASR Enable/Disable)*/
	theHeadset.Rubi_enable = lTemp.Rubidium_enable;

    /*theHeadset.tts_language                = Language;*/    
    Language = lTemp.tts_language;
	theHeadset.tts_language = 0;

#ifdef Three_Language
	if((Language != AUDIO_TTS_LANGUAGE_AMERICAN_ENGLISH) && (Language != AUDIO_TTS_LANGUAGE_SPANISH_EUROPEAN) && (Language != AUDIO_TTS_LANGUAGE_FRENCH))
#else
	#ifdef MANDARIN_SUPPORT
	if((Language != AUDIO_TTS_LANGUAGE_AMERICAN_ENGLISH) &&  (Language != AUDIO_TTS_LANGUAGE_CHINESE_MANDARIN))
	#else
    if((Language != AUDIO_TTS_LANGUAGE_AMERICAN_ENGLISH) &&  (Language != AUDIO_TTS_LANGUAGE_SPANISH_EUROPEAN))
	#endif
#endif
	{	
    	CONF_DEBUG(("TTS : Reset default...%d\n",Language));
		#ifdef MANDARIN_SUPPORT
		Language = AUDIO_TTS_LANGUAGE_CHINESE_MANDARIN;
		#else
		Language = AUDIO_TTS_LANGUAGE_AMERICAN_ENGLISH;
		#endif
		
		theHeadset.tts_language = 0;
    }

#else
    theHeadset.tts_language                = lTemp.tts_language;      
#endif

    theHeadset.MultipointEnable            = lTemp.multipoint_enable; 

#ifdef ENABLE_ENERGY_FILTER
    theHeadset.iir_enabled                 = lTemp.iir_enable;
#endif

#ifdef T3ProductionTest
	theHeadset.ProductionData			   = lTemp.Productionflag;
    CONF_DEBUG(("CONF : Rd Vol Inverted [%c], LED Disable [%c], Multipoint Enable [%c], IIR Enable [%c]\n", theHeadset.VolumeOrientationIsInverted ? 'T':'F' ,
 																							 lTemp.led_disable ? 'T':'F' ,
																							 theHeadset.MultipointEnable ? 'T':'F',
                                                                                             theHeadset.iir_enabled ? 'T':'F'));
                                                                                         
#else	
	theHeadset.lbipmEnable				   = lTemp.lbipm_enable;

    CONF_DEBUG(("CONF : Rd Vol Inverted [%c], LED Disable [%c], Multipoint Enable [%c], IIR Enable [%c], LBIPM Enable [%c]\n", theHeadset.VolumeOrientationIsInverted ? 'T':'F' ,
 																							 lTemp.led_disable ? 'T':'F' ,
																							 theHeadset.MultipointEnable ? 'T':'F',
                                                                                             theHeadset.iir_enabled ? 'T':'F',
                                                                                             theHeadset.lbipmEnable ? 'T':'F')) ;
#endif
}

/****************************************************************************
NAME 
 	configManagerWriteSessionData

*/ 
void configManagerWriteSessionData( void )
{
    session_data_type lTemp ;

    lTemp.vol_orientation   = theHeadset.gVolButtonsInverted;
	
#ifdef ROM_LEDS	
    lTemp.led_disable       = theHeadset.theLEDTask->gLEDSEnabled;
#endif	

#ifdef Rubidium
    lTemp.tts_language      = Language;
	lTemp.Rubidium_enable	= 0;/*Enable*/
#else
    lTemp.tts_language      = theHeadset.tts_language;
#endif

    lTemp.multipoint_enable = theHeadset.MultipointEnable;

#ifdef ENABLE_ENERGY_FILTER
    lTemp.iir_enable        = theHeadset.iir_enabled ;
#endif

#ifdef T3ProductionTest

	lTemp.Productionflag	= theHeadset.ProductionData;

	CONF_DEBUG(("CONF : PS Write Vol Inverted[%c], LED Disable [%c], Multipoint Enable [%c], IIR Enable [%c]\n" , 
					theHeadset.gVolButtonsInverted ? 'T':'F' , 
					lTemp.led_disable ? 'T':'F',
					theHeadset.MultipointEnable ? 'T':'F',
					theHeadset.iir_enabled ? 'T':'F'));

#else
	lTemp.lbipm_enable      = theHeadset.lbipmEnable;
	CONF_DEBUG(("CONF : PS Write Vol Inverted[%c], LED Disable [%c], Multipoint Enable [%c], IIR Enable [%c], LBIPM Enable [%c]\n" , 
					theHeadset.gVolButtonsInverted ? 'T':'F' , 
					lTemp.led_disable ? 'T':'F',
					theHeadset.MultipointEnable ? 'T':'F',
					theHeadset.iir_enabled ? 'T':'F',
					theHeadset.lbipmEnable ? 'T':'F'));

#endif
	lTemp.unused = 0;

    (void) PsStore(PSKEY_VOLUME_ORIENTATION, &lTemp, sizeof(session_data_type)); 
}

/****************************************************************************
NAME 
  	configManagerRestoreDefaults

DESCRIPTION
    Restores default PSKEY settings.
	This function restores the following:
		1. PSKEY_VOLUME_ORIENTATION
		2. PSKEY_PHONE_NUMBER
		3. Clears the paired device list
		4. Enables the LEDs
 
RETURNS
  	void
*/ 

void configManagerRestoreDefaults( void ) 
{
#ifdef ChineseTTS
	configManagerReadSessionData(0);
#else

    CONF_DEBUG(("CO: Restore Defaults\n")) ;
    /*Set local values*/
	theHeadset.gVolButtonsInverted = FALSE ;
    
    theHeadset.tts_language = FALSE ;
    theHeadset.MultipointEnable = FALSE ;
#ifdef ENABLE_ENERGY_FILTER
    theHeadset.iir_enabled = FALSE  ;
#endif

#ifdef T3ProductionTest
	theHeadset.ProductionData = 0;
#else
	theHeadset.lbipmEnable = FALSE ;
#endif
    
	/*Reset PSKEYS*/
	(void)PsStore ( PSKEY_VOLUME_ORIENTATION , 0 , 0 ) ;
    
	/*Call function to reset the PDL*/
	configManagerReset();
	
#ifdef ROM_LEDS
	if	(!(theHeadset.theLEDTask->gLEDSEnabled))
	{
		/*Enable the LEDs*/
		MessageSend (&theHeadset.task , EventEnableLEDS , 0) ;
	}
#endif
#endif
}
                
/****************************************************************************
NAME 
  	configManagerInit

DESCRIPTION
  	The Configuration Manager is responsible for reading the user configuration
  	from the persistent store are setting up the system.  Each system component
  	is initialised in order.  Where appropriate, each configuration parameter
  	is limit checked and a default assigned if found to be out of range.

RETURNS
  	void
    
*/

void configManagerInit( void )  
{ 
	/* use a memory allocation for the lengths data to reduce stack usage */
    lengths_config_type * keyLengths = mallocPanic(sizeof(lengths_config_type));
	
        /*get the config ID**/    
    uint16 lConfigID = get_config_id ( PSKEY_CONFIGURATION_ID ) ;
	
		/* Read key lengths */
    configManagerKeyLengths(lConfigID, keyLengths);			

        /* Allocate the memory required for the configuration data */
    InitConfigMemory_1(keyLengths);
    
  	    /* Read and configure the button durations */
  	configManagerButtonDurations(lConfigID );

#ifndef T3ProductionTest	
  	    /* Read the system event configuration and configure the buttons */
    configManagerButtons(lConfigID );
#endif

    /*configures the pattern button events*/
    configManagerButtonPatterns(lConfigID ) ;

        /*Read and configure the event tones*/
    configManagerEventTones( lConfigID, keyLengths->no_tones ) ;
        /* Read and configure the system features */
  	configManagerFeatureBlock( lConfigID );	
        /* Read and configure the user defined tones */    
  	configManagerUserDefinedTones( lConfigID, keyLengths->userTonesLength );	
    
#ifdef ROM_LEDS	
  	    /* Read and configure the LEDs */
    configManagerLEDS(lConfigID);
#endif    
        /* Read and configure the voume settings */
  	configManagerVolume( lConfigID );
 
        /* Read and configure the automatic switch off time*/
    configManagerConfiguration( lConfigID );
 
  	    /* Read and configure the power management system */
  	configManagerPower( lConfigID );
 
  	    /* Read and configure the radio parameters */
  	configManagerRadio( lConfigID );

#ifdef BHC612
       /* Read and configure the volume orientation, LED Disable state, and tts_language */
	configManagerReadSessionData ( lConfigID ) ;
#else
	configManagerReadSessionData ( lConfigID ) ;
#endif

#ifdef T3ProductionTest
	configManagerButtons(lConfigID );
#endif
        /* Read and configure the sniff sub-rate parameters */
	configManagerSetupSsr ( lConfigID ) ; 
	
    configManagerEventTTSPhrases( lConfigID, keyLengths->no_tts ) ;

    configManagerVoicePromptsInit( lConfigID, keyLengths->no_vp , keyLengths->no_tts_languages );
    
    /* release the memory used for the lengths key */
    free(keyLengths);
}

#ifdef New_MMI
void LED_Reconfig(void)
{
	configManagerLEDS(0);
}
#endif

