/****************************************************************************

FILE NAME
    headset_configmanager.h
    
DESCRIPTION
    Configuration manager for the headset - resoponsible for extracting user information out of the 
    PSKEYs and initialising the configurable nature of the headset components
    
*/
#ifndef HEADSET_CONFIG_MANAGER_H
#define HEADSET_CONFIG_MANAGER_H

#include "headset_private.h"
#include <audio.h>

/* Persistent store key allocation - it is of vital importantance that this enum is in numerical
   order with no gaps as it is used as an index into the default configuration structure */
#define PSKEY_BASE  (0)

/***********************************************************************/
/***********************************************************************/
/* ***** do not alter order or insert gaps as headset will panic ***** */
/***********************************************************************/
/***********************************************************************/
enum
{
 	PSKEY_BATTERY_CONFIG     				= PSKEY_BASE,
 	PSKEY_BUTTON_CONFIG     				= 1,
 	PSKEY_BUTTON_PATTERN_CONFIG     		= 2,
 	PSKEY_HFP_SUPPORTED_FEATURES			= 3,
 	PSKEY_INPUT_PIO_BLOCK        			= 4,
 	PSKEY_ADDITIONAL_HFP_SUPPORTED_FEATURES	= 5,
 	PSKEY_TIMEOUTS                  		= 6,
 	PSKEY_TRI_COL_LEDS        				= 7,
 	PSKEY_SW_VERSION_NUMBER    				= 8,
	PSKEY_LENGTHS                    		= 9,
 	PSKEY_CODEC_NEGOTIATION    				= 10,
 	PSKEY_TTS                				= 11, 
	PSKEY_VOLUME_ORIENTATION    			= 12,
 	PSKEY_RADIO_CONFIG      				= 13,
 	PSKEY_SSR_PARAMS              			= 14,
 	PSKEY_FEATURE_BLOCK     				= 15,
 	PSKEY_SPEAKER_GAIN_MAPPING				= 16,
 	PSKEY_HFP_INIT		     				= 17,
 	PSKEY_LED_FILTERS      					= 18,
 	PSKEY_CONFIG_TONES	     				= 19,    /* used to configure the user defined tones */
 	PSKEY_LED_STATES      					= 20,
 	PSKEY_VOICE_PROMPTS	     				= 21,
	PSKEY_LED_EVENTS      					= 22,
 	PSKEY_EVENTS_A      					= 23,
 	PSKEY_EVENTS_B    						= 24,
 	PSKEY_RSSI_PAIRING             			= 25,
	PSKEY_TONES                     		= 26,
    PSKEY_PIO_BLOCK              			= 27,
    PSKEY_UNUSED28   						= 28,
    PSKEY_FILTER                 			= 29,
 	PSKEY_PHONE_NUMBER						= 30,
	PSKEY_CONFIGURATION_ID					= 31
};

/* Index to PSKEY PSKEY_LENGTHS */
enum
{
 	LENGTHS_NO_TTS     				= 0,
 	LENGTHS_NO_LED_FILTERS,
 	LENGTHS_NO_LED_STATES,
 	LENGTHS_NO_LED_EVENTS,
 	LENGTHS_NO_TONES,
    LENGTHS_NO_VP_SAMPLES
};

/* Persistant store vol orientation definition */
typedef struct
{
    unsigned    vol_orientation:1 ;   
    unsigned    led_disable:1 ;
	unsigned    tts_language:4;
    unsigned    multipoint_enable:1; 
    unsigned    iir_enable:1;
    unsigned    lbipm_enable:1;
#ifdef T3ProductionTest
	#ifdef Rubidium
	unsigned 	unused:4 ;
	unsigned 	Rubidium_enable:1;
	#else
	unsigned 	unused:5 ;
	#endif
	unsigned    Productionflag:2;
#else
    unsigned    unused:7 ;
#endif
}session_data_type;

#define MAX_EVENTS ( EVENTS_MAX_EVENTS )


/* Persistent store LED configuration definition */
typedef struct
{
 	unsigned 	state:8;
 	unsigned 	on_time:8;
 	unsigned 	off_time:8;
 	unsigned  	repeat_time:8;
 	unsigned  	dim_time:8;
 	unsigned  	timeout:8;
 	unsigned 	number_flashes:4;
 	unsigned 	led_a:4;
 	unsigned 	led_b:4;
    unsigned    overide_disable:1;
 	unsigned 	colour:3;
}led_config_type;

typedef struct
{
 	unsigned 	event:8;
 	unsigned 	speed:8;
     
    unsigned 	active:1;
    unsigned    dummy:1 ;
 	unsigned 	speed_action:2;
 	unsigned  	colour:4;
    unsigned    filter_to_cancel:4 ;
    unsigned    overide_led:4 ;   
    
    unsigned    overide_led_active:1 ;
    unsigned    dummy2:2;
    unsigned    follower_led_active:1 ;
    unsigned    follower_led_delay_50ms:4;
    unsigned    overide_disable:1 ;
    unsigned    dummy3:7 ;
    
    
}led_filter_config_type;

#define MAX_STATES (HEADSET_NUM_STATES)
#define MAX_LED_EVENTS (20)
#define MAX_LED_STATES (HEADSET_NUM_STATES)
#define MAX_LED_FILTERS (LM_NUM_FILTER_EVENTS)

/* LED patterns */
typedef enum
{
 	led_state_pattern,
 	led_event_pattern
}configType;

typedef struct 
{
    unsigned event:8;
    unsigned tone:8;
}tone_config_type ;

typedef HeadsetTts_t tts_config_type ;

#define MAX_VP_SAMPLES (15)

/* Pull in voice_prompts_index type from audio_plugin_if.h */
typedef voice_prompts_index vp_config_type;

typedef struct
{
    uint16 event;
    uint32 pattern[6];
}button_pattern_config_type ;

typedef struct
{
    uint16 no_tts;
	uint16 no_tts_languages;
	uint16 no_led_filter;
	uint16 no_led_states;
	uint16 no_led_events;
	uint16 no_tones;
    uint16 no_vp;
    uint16 userTonesLength;
}lengths_config_type ;

/* Pairing timeout action */
enum
{
 	PAIRTIMEOUT_CONNECTABLE   		= 0,
 	PAIRTIMEOUT_POWER_OFF     		= 1,
 	PAIRTIMEOUT_POWER_OFF_IF_NO_PDL = 2
};


/****************************************************************************
NAME 
 	configManagerInit

DESCRIPTION
 	Initialises all subcomponents in order

RETURNS
 	void
    
*/
void configManagerInit( void );

/****************************************************************************
NAME 
 	configManagerReset

DESCRIPTION
    Resets the Paired Device List - Reboots if PSKEY says to do so
 
RETURNS
 	void
*/ 
void configManagerReset( void ) ;     

/****************************************************************************
NAME 
 	configManagerHFP_SupportedFeatures

DESCRIPTION
    Gets the 1.5 Supported features set from PS - exposed as this needs to be done prior to a HFPInit() 
 
RETURNS
 	void
*/ 
void configManagerHFP_SupportedFeatures( void ) ;

/****************************************************************************
NAME 
 	configManagerHFP_Init

DESCRIPTION
    Gets the HFP initialisation parameters from PS
 
RETURNS
 	void
*/
void configManagerHFP_Init( hfp_init_params * hfp_params );

/****************************************************************************
NAME 
 	configManagerWriteSessionData

DESCRIPTION
    Stores the persistent session data across power cycles.
	This includes information like volume button orientation 
	TTS language etc.
 
RETURNS
 	void
*/ 
void configManagerWriteSessionData( void ) ;

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

void configManagerRestoreDefaults( void ) ;

/****************************************************************************
*/

void LED_Reconfig(void);
#endif   
