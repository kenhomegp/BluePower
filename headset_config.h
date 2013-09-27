/****************************************************************************

FILE NAME
    headset_config.h
    
DESCRIPTION
    
*/

#ifndef _HEADSET_CONFIG_H_
#define _HEADSET_CONFIG_H_


#include "headset_configmanager.h"
#include "headset_powermanager.h"
#include "headset_states.h"
#include "headset_volume.h"
#include "headset_buttonmanager.h"

/* Key defintion structure */
typedef struct
{
 	uint16    length;
 	const uint16* data;
}key_type;

typedef struct
{
 	uint16     length;
 	uint16     value[1];
}config_uint16_type;


typedef struct
{
  	uint16         length;
  	uint16         value[sizeof(power_config_type)];
}config_battery_type;


typedef struct
{
   	uint16         length;
   	uint16         value[sizeof(button_config_type)];
}config_button_type;

typedef struct
{
    uint16         length ;
    uint16         value[sizeof(button_pattern_config_type) * BM_NUM_BUTTON_MATCH_PATTERNS]; 
}config_button_pattern_type ;

typedef struct
{
 	uint16     length;
 	uint16     value[sizeof(radio_config_type)];
}config_radio_type;

typedef struct
{
    uint16 length ;
    uint16      value[sizeof(HFP_features_type)];
}config_HFP_type ;

typedef struct
{
 	uint16     length;
 	uint16     value[sizeof(feature_config_type)];
}config_features_type;


typedef struct
{
 	uint16     length;
 	uint16     value[sizeof(VolMapping_t) * VOL_NUM_VOL_SETTINGS];
}config_volume_type;


typedef struct
{
 	uint16     length;
 	uint16     value[sizeof(led_filter_config_type) * MAX_LED_FILTERS];
}config_led_filters_type;


typedef struct
{
 	uint16     length;
 	uint16     value[sizeof(led_config_type) * MAX_LED_STATES];
}config_led_states_type;


typedef struct
{
    uint16 length;
    uint16      value[sizeof(vp_config_type) * MAX_VP_SAMPLES];
} config_vp_type;


typedef struct
{
 	uint16     length;
 	uint16     value[sizeof(led_config_type) * MAX_LED_EVENTS];
}config_led_events_type;


typedef struct
{
 	uint16     length;
 	uint16     value[sizeof(event_config_type) * BM_EVENTS_PER_BLOCK]; 
}config_events_type;


typedef struct
{
 	uint16     length;
 	uint16     value[sizeof(tone_config_type) * MAX_EVENTS]; 
}config_tone_events_type;


typedef struct
{
 	uint16     length;
 	uint16     value[sizeof(lengths_config_type)];     
}config_lengths_type;

typedef struct
{
 	uint16     length;
 	uint16     value[sizeof(tts_config_type) * MAX_EVENTS]; 
}config_tts_events_type;

typedef struct
{
 	uint16     length;
 	uint16     value[sizeof(PIO_block_t)];
}config_pio_block_type;

typedef struct
{
	uint16     length;
	uint16    value[sizeof(Timeouts_t)] ; 
}config_timeouts ;

typedef struct
{
	uint16		length;
	uint16		value[sizeof(filter_t)];
} config_filter_type;

typedef struct
{
	uint16		length;
	uint16		value[sizeof(subrate_t)];
} config_ssr_type;

typedef struct
{
    uint16         length ;
    uint16         value[sizeof(rssi_pairing_t)]; 
} config_rssi_type ;

typedef struct
{
 	const key_type*  	battery_config;     	    /* PSKEY_USR_0 - Battery configuration */
 	const key_type*  	button_config;     		    /* PSKEY_USR_1 - Button configuration */
 	const key_type*     button_pattern_config;  /* PSKEY_USR_2 - Button Sequence Patterns*/
	const key_type*  	supported_features_config;	/* PSKEY_USR_3 - HFP supported features block */
	const key_type*  	input_pio_block;            /* PSKEY_USR_4 - Input PIO Block */
	const key_type* 	HFP_features_config;    	/* PSKEY_USR_5 - HFP Features */
 	const key_type*  	timeouts_config;  		    /* PSKEY_USR_6 - timeouts / counters*/
 	const key_type*  	tri_col_leds;               /* PSKEY_USR_7 - UNUSED*/
 	const key_type*  	unused8;             	    /* PSKEY_USR_8 -  UNUSED*/
 	const key_type*  	lengths;  			    	/* PSKEY_USR_9 -  key lengths*/
 	const key_type*  	codec_negotiation;     	    /* PSKEY_USR_10 - Codec Negotiation set-up*/
 	const key_type*     tts;                	    /* PSKEY_USR_11 - TTS event configuration */
 	const key_type*  	volume_orientation ;        /* PSKEY_USR_12 - UNUSED*/
 	const key_type*  	radio;       			    /* PSKEY_USR_13 - Radio configuration parameters */
	const key_type*		ssr_data;					/* PSKEY_USR_14 - SSR parameters */
 	const key_type*  	features;      			    /* PSKEY_USR_15 - Features block */
 	const key_type*  	volume;       			    /* PSKEY_USR_16 - Volume gain mappings */
 	const key_type*  	unused17;         			/* PSKEY_USR_17 - unused */
 	const key_type* 	led_filters;     	    	/* PSKEY_USR_18 - LED filter configuration */
 	const key_type*  	unused19;     	    		/* PSKEY_USR_19 - unused */
 	const key_type* 	led_states;      	    	/* PSKEY_USR_20 - LED state configuration */
    const key_type*     vp_data;                    /* PSKEY_USR_21 - Voice Prompts parameters */
 	const key_type* 	led_events;      		    /* PSKEY_USR_22 - LED event configuration */
 	const key_type*  	events_a;     			    /* PSKEY_USR_23 - System event configuration */
 	const key_type*  	events_b;       	        /* PSKEY_USR_24 - System event configuration */
 	const key_type*  	rssi;         				/* PSKEY_USR_25 - RSSI Configuration parameters */
 	const key_type* 	tones;       		    	/* PSKEY_USR_26 - Tone event configuration */
 	const key_type* 	pio_block;      	    	/* PSKEY_USR_27 - PIO assignment */
    const key_type*  	unused28;			        /* PSKEY_USR_28 - unused */
	const key_type*  	filter;			         	/* PSKEY_USR_29 - Filter parameters */
} config_type;



/****************************************************************************
NAME 
 	get_config_id

DESCRIPTION
 	This function is called to read the configuration ID
 
RETURNS
 	Defaults to config 0 if the key doesn't exist
*/
uint16 get_config_id ( uint16 key ) ;


/****************************************************************************
NAME 
 	ConfigRetrieve

DESCRIPTION
 	This function is called to read a configuration key.  If the key exists
 	in persistent store it is read from there.  If it does not exist then
 	the default is read from constant space
 
RETURNS
 	0 if no data was read otherwise the length of data
*/
uint16 ConfigRetrieve(uint16 config_id, uint16 key, void* data, uint16 len);


/****************************************************************************
NAME 
 	get_service_record

DESCRIPTION
 	This function is called to get a special service record associated with a 
 	given configuration
 
RETURNS
 	a pointer to the service record
*/
uint8 * get_service_record ( void ) ;

/****************************************************************************
NAME 
 	get_service_record_length

DESCRIPTION
 	This function is called to get the length of a special service record 
 	associated with a given configuration
 
RETURNS
 	the length of the service record
*/
uint16 get_service_record_length ( void ) ;


#endif /* _HEADSET_CONFIG_H_ */
