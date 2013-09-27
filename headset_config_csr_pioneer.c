/****************************************************************************

FILE NAME
    headset_config_csr_pioneer.c
    
DESCRIPTION
    This file contains the default configuration parameters for a CSR Pioneer
 Mono Headset
*/



#ifndef BC4_HS_CONFIGURATOR
    #include "headset_config.h"
#else
    #include "Configurator_config_types.h"
#endif

#define BHC612_100_NewHW

/* PSKEY_USR_0 - Battery config */
static const uint16 battery_config[sizeof(power_config_type)+1] =
{
 	sizeof(power_config_type),
	#if 0
	0x8791,  /*Shutdown threshold       */   /*Gas Gauge Level 0*/   
    0x9BA5,  /*Gas Gauge Level 1	    */   /*Gas Gauge Level 2*/
    0xAF14,  /*Gas Gauge Level 3	    */   /*report Period*/
    0x00D9,  /*       */   /* Volt_enabled:1, Volt AIO(BATTERY_INTERNAL):3; Temp_enabled:1, Temp AIO (AIO1):3*/   
    0x0000,  /*lion th_m (U16)          */
    0x0000,  /*lion th_c (U16)          */
    0x0032,  /*lion td                  */   /*lion max temp*/
    0x0000   /*lion min temp            */   /*Unused*/    
	#endif
	/*Shutdown threshold = 150 x 20mV = 3000 mV*/
	/*Gas gauge lev 0 = 169 x 20mV = 3380 mV*/
	/*Gas gauge lev 1 = 186 x 20mV = 3720 mV*/
	/*Gas gauge lev 2 = 191 x 20mV = 3820 mV*/
	0x96A9,
	0xBABF,
	#if 0
	/*Gas gauge lev 3 = 203 x 20mV = 4060 mV*/
	0xCB0A, 
	0x42D8,
	#else
	/*Gas gauge lev 3 = 205 x 20mV = 4100 mV*/
	0xCD0A,
	0x62D8,
	#endif
	0x0000,  
	0x0000, 
	0x0032, 
	0x0000	
};

/* PSKEY_USR_1 - Button config */
static const uint16 button_config[sizeof(button_config_type)+1] =
{
  	sizeof(button_config_type),
	#if 0	
   	0x01F4, /* Button double press time */
   	0x03E8, /* Button long press time */
   	0x09C4, /* Button very long press time */
   	0x0320, /* Button repeat time */
   	0x1388, /* Button Very Very Long Duration*/
    0x0000  /* default debounce settings*/
    #endif
	#if 0	
	0x01F4, 
	0x0BB8, 
	0x2710, 
	0x0320, 
	0x2EE0, 
	0x0000
	#endif	
	0x01F4, 
	0x03E8,
	0x0BB8, 
	0x0320, 
	0x2710, 
	0x0000
};


/* PSKEY_USR_2 - buttonpatterns */
#define NUM_BUTTON_PATTERNS 4


static const uint16 button_pattern_config[(sizeof(button_pattern_config_type ) * NUM_BUTTON_PATTERNS) +1] = 
{
    sizeof(button_pattern_config_type ) * NUM_BUTTON_PATTERNS ,
    0   ,0,0,0,0,0,0 ,
    0   ,0,0,0,0,0,0 ,
    0   ,0,0,0,0,0,0 ,
    0   ,0,0,0,0,0,0    
};

/* PSKEY_USR_3 - Supported features */
static const uint16 supported_features_config[sizeof(config_uint16_type)] =
{
	1,
   	(   	
#ifdef INCLUDE_CVC   	
        HFP_NREC_FUNCTION        | 
#else
/*   	HFP_NREC_FUNCTION        | */
#endif
	 	HFP_THREE_WAY_CALLING    | 
	 	HFP_CLI_PRESENTATION     | 
 	 	HFP_VOICE_RECOGNITION    | 
	 	HFP_REMOTE_VOL_CONTROL   |
        HFP_ENHANCED_CALL_STATUS /*| 
        HFP_ENHANCED_CALL_CONTROL  */ 
    )
};

/* PSKEY_USR_4 - Input PIO Block */
static const uint16 input_pio_block[sizeof(config_uint16_type)] =
{
  	1,
  	0xffff
};

/* PSKEY_USR_5 - hfp supported features */
static const uint16 HFP_features_config[8] =
{
  	7,
    0x8001,
    (
    sync_hv1  |
    sync_hv2  | 
    sync_hv3  | 
    sync_ev3  |    
    sync_ev4  | 
    sync_ev5  |
  /*sync_2ev3 |*/
    sync_3ev3 |
  /*sync_2ev5 |*/
    sync_3ev5
    ),
    0x0000 , 0x1f40, 	/*8000hz bandwidth*/
    0x000c,				/*latency*/
    0x0000,				/*voice quality*/
    0x0002				/*retx effort*/
};

/* PSKEY_USR_6 - timeouts */
static const uint16 timeouts[sizeof(config_timeouts)] =
{
  	16,
	#if 0	
	0x012C ,	/*AutoSwitchOffTime_s*/
  	0x001E ,	/*AutocPowerOnTimeout_s*/ 
  	0x000A ,	/*NetworkServiceIndicatorRepeatTime_s*/  		
  	0x0003 ,	/*DisablePowerOffAcfterPowerOnTime_s*/ 
  	0x0258 ,	/*PairModeTimeout_s*/ 
  	0x000A ,    /*MuteRemindTime_s*/
    0x003C ,    /*Connectable Timeout*/
	0x0000 ,    /*PairModeTimeout if PDL empty*/
    0x0000 ,    /*ScrollPDLforReconnectionAttempts*/
	0x000F ,	/*EncryptionRefreshTimeout_m*/
	0x003C , 	/*InquiryTimeout_s*/
	0x0000 ,  	/*CheckRoleDelayTime_s*/
	0x0064 ,  	/*SecondAGConnectDelayTime_s*/
	0x0005 ,    /*missed call indicator period in second*/
    0x0005 ,    /*missed call indicator repeat times*/
    0x003C      /*a2dp link loss reconnection period in seconds*/
	#endif

	0x00B4 ,	/*AutoSwitchOffTime_s = 180 sec*/
	/*0x0003,  	AutoSwitchOffTime_s =  3 sec*/
	/*0x0001 ,	AutocPowerOnTimeout_s*/ 
	#if 0
	0x0078,  	/*AutocPowerOnTimeout_s = 120 s*/ 
	#else
  	0x0003,  	/*AutocPowerOnTimeout_s = 3 s*/ 
  	#endif
  	0x000A ,	/*NetworkServiceIndicatorRepeatTime_s*/  	
  	0x0003 ,	/*DisablePowerOffAcfterPowerOnTime_s*/ 
  	0x0096 ,	/*PairModeTimeout_s = 150 sec*/ 
  	0x000A ,    /*MuteRemindTime_s*/
    0x003C ,    /*Connectable Timeout*/
	/*0x0000 ,    PairModeTimeout if PDL empty = 0*/
	/*0x012C ,    PairModeTimeout if PDL empty = 300 s*/
	0x0096 , 	/*PairModeTimeout if PDL empty = 150 s*/
	/*0x003C ,    PairModeTimeout if PDL empty = 60 s*/
    0x0000 ,    /*ScrollPDLforReconnectionAttempts*/
	0x000F ,	/*EncryptionRefreshTimeout_m*/
	0x003C , 	/*InquiryTimeout_s*/
	0x0000 ,  	/*CheckRoleDelayTime_s*/
	/*0x0064 ,  	SecondAGConnectDelayTime_s = 100ms*/
	0x01F4 ,	/*SecondAGConnectDelayTime_s = 500ms*/
	0x0005 ,    /*missed call indicator period in second*/
    0x0005 ,    /*missed call indicator repeat times*/
    0x003C      /*a2dp link loss reconnection period in seconds*/
};

/* PSKEY_USR_7 - tri col leds */
static const uint16 tri_col_leds[sizeof(config_uint16_type)] =
{
  	1,
    0xFEE0
};



/* PSKEY_USR_8 - unused2*/
static const uint16 unused2[sizeof(config_uint16_type)] =
{
  	1,
  	0
};


#define NO_TTS_EVENTS    (0x0000)
#define NO_TTS_LANGUAGES (0x0001)
#if 1
#define NO_LED_FILTERS   (0x0009)
#else
#define NO_LED_FILTERS   (0x0000)
#endif
#define NO_LED_STATES    (0x000C)
#define NO_LED_EVENTS    (0x0005)
#if 0
#define NO_TONE_EVENTS   (0x0024)
#endif
#define NO_TONE_EVENTS   (0x0002)

#define NO_VP_SAMPLES    (0x0000)
#define USER_TONE_LENGTH (0x0000)

/* PSKEY_USR_9 - Number of tts events, 
                 tts supported language
				 Number of led filters
				 Number of led states
				 Number of tone events
                 Number of voice prompt samples
*/
static const uint16 lengths[sizeof(config_lengths_type)] =
{
  	sizeof(lengths_config_type) * 1,
	NO_TTS_EVENTS,    /* number of tts_event */
 	NO_TTS_LANGUAGES, /* the supported tts language */
	NO_LED_FILTERS,	  /* number of led filters */
	NO_LED_STATES,	  /* number of led states */
	NO_LED_EVENTS,    /* Number of led events */
	NO_TONE_EVENTS,   /* number of tone events */
    NO_VP_SAMPLES,     /* number of voice prompts */
    USER_TONE_LENGTH   /* length of usr 19 - user tone data */
};

/* PSKEY_USR_10 - Reserved */
static const uint16 unused10[sizeof(config_uint16_type)] =
{
  	1,
  	0
};

/* PSKEY_USR_11 - Tone event configuration */
static const uint16 tts[(sizeof(tts_config_type)*NO_TTS_EVENTS)+1] =
{
  	sizeof(tts_config_type) * NO_TTS_EVENTS,
};

/* PSKEY_USR_12 - volume orientation, led enable, and selected tts language*/
static const uint16 unused5[sizeof(config_uint16_type)] =
{
  	1,
	#ifdef MANDARIN_SUPPORT
	0x6000	/*Rubi_TTS-ASR = AUDIO_TTS_LANGUAGE_CHINESE_MANDARIN */
	#else
  	0x4400	/*Rubi_TTS-ASR = AUDIO_TTS_LANGUAGE_AMERICAN_ENGLISH */
  	#endif
};

/* PSKEY_USR_13 - Radio configuration parameters */
static const uint16 radio[sizeof(config_radio_type)] =
{
  	4,
  	0x0800, 0x0012, 0x0800, 0x0012
};

/* PSKEY_USR_14 - Subrate parameters
	Default: All Zero
*/
static const uint16 ssr_config[sizeof(subrate_t)+1] = 
{
	sizeof(subrate_t),	
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000
};

/* PSKEY_USR_15 - Features block */
static const uint16 features[sizeof(config_features_type)] =
{
  	6,
	#if 0	
  	0x1330, 0x9C01, 0x11E7, 0x22A0, 0x5800 , 0xAA00
  	#endif

	#if 0
	0x1B30, 0xBC01, 0x11E7, 0x21A0, 0x5800, 0xAA00 	/*Disable Power off after power on = true*/
													/*Automatically Reconnect on Power on = true*/
	#endif

	#if 0
	0x1930, 0xBC01, 0x11E7, 0x21A2, 0x5800, 0xAA00	/*Auto-Reconnect link loss : Enable*/
													/*Go connectable during link loss*/
													/*Disable Power off after Power on = false */
												   	/*Automatically Reconnect on Power on = true*/
	#endif
	
	0x1930, 0xBC01, 0x11E7, 0x21A0, 0x5800, 0xAA00	/*Auto-Reconnect link loss : Disable*/
													/*Go connectable during link loss*/
													/*Disable Power off after Power on = false */
												   	/*Automatically Reconnect on Power on = true*/
};

/* PSKEY_USR_16 - Volume gain mappings */
static const uint16 volume[33] =
{
  		32,
    	0x1001, 0x0000, 
        0x2004, 0x0101, 
        0x3104, 0x0202, 
        0x4204, 0x0303, 
        0x5304, 0x0404, 
        0x6404, 0x0505, 
        0x7504, 0x0606, 
        0x8604, 0x0707, 
        0x9704, 0x0808, 
        0xa804, 0x0909, 
        0xb904, 0x0a0a, 
        0xca04, 0x0b0b, 
        0xdb04, 0x0c0c, 
        0xec04, 0x0d0d, 
        0xfd04, 0x0e0e, 
        0xfe0a, 0x0f0f,
};


/* PSKEY_USR_17 - hfp initialisation parameters */
static const uint16 hfpInit[sizeof(hfp_init_params)+1] =
{
  	7,
	#if 0            
  	hfp_handsfree_profile | hfp_headset_profile,       /* supported profiles */
#ifdef INCLUDE_CVC   	
    HFP_NREC_FUNCTION | HFP_CODEC_NEGOTIATION | HFP_THREE_WAY_CALLING | HFP_CLI_PRESENTATION | HFP_VOICE_RECOGNITION | 
    HFP_REMOTE_VOL_CONTROL | HFP_ENHANCED_CALL_STATUS, /* supported features */
#else
    HFP_CODEC_NEGOTIATION | HFP_THREE_WAY_CALLING | HFP_CLI_PRESENTATION | HFP_VOICE_RECOGNITION | 
    HFP_REMOTE_VOL_CONTROL | HFP_ENHANCED_CALL_STATUS, /* supported features */
#endif
    hfp_wbs_codec_mask_cvsd | hfp_wbs_codec_mask_msbc, /* supported WBS codecs */
    0x0040,                                            /* optional indicators.service = hfp_indicator_on */
    0x0005,                                            /* Multipoint + Disable NREC */ 
    0x0A0A,                                            /* link loss timeout + link_loss_interval */ 
    0x1000                                             /* csr_features.batt_level = true */ 
    #endif
	
	0x0003, 											/*hfp_handsfree_profile | hfp_headset_profile, */
	0x00BF, 											/*Support featurees*/
	0x0003,												/*Support WBS codecs*/
	0x0040,                                            	/* optional indicators.service = hfp_indicator_on */
    0x0005,                                            	/* Multipoint + Disable NREC */ 
    /*0x0A0A,                                            	 				link loss timeout + link_loss_interval */ 
    0x0103,                                            	/* link loss timeout : 1 minute + link_loss_interval : 3 seconds */ 
    0x1000                                             	/* csr_features.batt_level = true */ 
};

/* PSKEY_USR_18 - LED filter configuration */
static const uint16 led_filters[(sizeof(led_filter_config_type) * NO_LED_FILTERS)+1] =
{
 	sizeof(led_filter_config_type) * NO_LED_FILTERS,
	#if 0	
/*1*/        0x1a00, 0x8100, 0x0000, 	/*filter on with low batt*/
/*2*/        0x2200, 0x0010, 0x0000, 	/*filter off with charger connected*/
/*3*/        0x2100, 0x0010, 0x0000, 	/*filter off with EVBATok*/    
/*4*/        0x2000, 0x800E, 0x8000,    
/*5*/        0x1E00, 0x800F, 0x8000,    
/*6*/        0x1E00, 0x0040, 0x0000,    
/*7*/        0x2000, 0x005F, 0x0000,    
/*8*/        0x2300, 0x0040, 0x0000,    
/*9*/        0x2300, 0x0050, 0x0000,
	#endif
	#if 0
	0x1A00, 0x8100, 0x0000, 
	0x2200, 0x0010, 0x0000, 
	0x2100, 0x0010, 0x0000, 
	0x2000, 0x001E, 0x0000, 
	0x1E00, 0x001F, 0x0000, 
	0x1E00, 0x0040, 0x0000, 
	0x2000, 0x005F, 0x0000, 
	0x2300, 0x0040, 0x0000, 
	0x2300, 0x0050, 0x0000,
	#endif
};

/* PSKEY_USR_19 - unused */
static const uint16 unused19[sizeof(config_uint16_type)] =
{
  	1,
  	0
};

/* PSKEY_USR_20 - LED state configuration */
static const uint16 led_states[(sizeof(led_config_type) * NO_LED_STATES)+1] =
{
  	sizeof(led_config_type) * NO_LED_STATES,
	#if 0	
    0x010A, 0x0A28, 0x0000, 0x2EF1,  /*connectable*/
	0x020A, 0x0A01, 0x0000, 0x2EF3,  /*conn_discoverable*/
	0x030A, 0x0A28, 0x0000, 0x3EF2,  /*connected*/
	
	0x0664, 0x6402, 0x4B00, 0x1EF2,  /*activecall SCO*/    	
	0x0864, 0x6401, 0x0000, 0x1EF1,  /*headsetThreeWayCallWaiting */
	0x0964, 0x6401, 0x0000, 0x1EF1,  /*headsetThreeWayCallOnHold*/
	0x0A64, 0x6401, 0x0000, 0x1EF1,  /*headsetThreeWay Multi Call	*/
	0x0C64, 0x6401, 0x0000, 0x1EF1,  /*headsetActiveCallNoSCO*/
	
	0x0432, 0x3201, 0x0000, 0x1EF2,  /*headsetOutgoingCallEstablish*/
	0x0532, 0x3201, 0x0000, 0x1EF2,  /*headsetIncomingCallEstablish*/
    0x0D32, 0x6401, 0x0000, 0x2EF2,  /*headsetA2DPStreaming*/
	#endif
	#ifdef BHC612_100_NewHW	/*BHC612-100 HW change, #define NewLED0*/
	0x0205, 0x0500, 0x0000, 0x8CF3, 
	0x0332, 0x005A, 0x1400, 0x1CF2, 
	0x0664, 0x6428, 0x0000, 0x1CF1,
	0x0864, 0x6401, 0x0000, 0x1CF1,
	0x0964, 0x6401, 0x0000, 0x1CF1,
	0x0A64, 0x6401, 0x0000, 0x1CF1,
	0x0C64, 0x6428, 0x0000, 0x1CF1,
	0x0419, 0x1905, 0x0000, 0x1CF1,
	0x050A, 0x0A0A, 0x0000, 0x5CF4,	/*headsetIncomingCallEstablish*/
	0x0E0A, 0x0A5C, 0x0000, 0x2EF2, /*headsetLowBattery*/
	0x0D32, 0x005A, 0x1400, 0x1CF2,
	0x0132, 0x005A, 0x0000, 0x1CF2,
	#else
	0x0205, 0x0500, 0x0000, 0x8EF3, /*Pairing , On=50ms , Off=50 , NumFlash=8*/
	0x0332, 0x005A, 0x1400, 0x1EF2, /*Connected , DimTime = 20*/
	0x0664, 0x6428, 0x3200, 0x1EF1, /*Active call , RepeatTime = 2000ms , DimTime = 50*/
	0x0864, 0x6401, 0x0000, 0x1EF1,
	0x0964, 0x6401, 0x0000, 0x1EF1,
	0x0A64, 0x6401, 0x0000, 0x1EF1,
	0x0C64, 0x6401, 0x0000, 0x1EF1,
	0x0419, 0x1905, 0x0000, 0x1EF1,
	0x050A, 0x0A0A, 0x0000, 0x5EF4,
	0x0D32, 0x6401, 0x0000, 0x2EF2,
	0x0132, 0x005A, 0x0000, 0x1EF2,
	0x0E0A, 0x0A5C, 0x0000, 0x2EF2,
	#endif
};

/* PSKEY_USR_21 - Voice Prompts Config */
static const uint16 vp_data[(sizeof(vp_config_type) + 1)] =
{
  	sizeof(vp_config_type),
    0001,                   /* Header in SPI flash */
    0000,0000               /* Address 000000 */
};


/* PSKEY_USR_22 - LED event configuration */
static const uint16 led_events[(sizeof(led_config_type) * NO_LED_EVENTS)+1] =
{
 	sizeof(led_config_type) * NO_LED_EVENTS,
	#if 0                                            /* E - red , F - blu*/
	0x0164, 0x0000, 0x0000, 0x1EF4,
    0x0264, 0x0000, 0x0000, 0x1EF4,
    0x140A, 0x0A00, 0x0000, 0x2EF4,
    0x460A, 0x0000, 0x0000, 0x0EFC,
    0x8928, 0x0A00, 0x0002, 0x2EF4,    
	#endif
	#ifdef BHC612_100_NewHW	/*BHC612-100 HW change, #define NewLED0*/
	0x0119, 0x1900, 0x0000, 0xACF4,
	0x0219, 0x1900, 0x0000, 0x1CF4,
	0x140A, 0x0A00, 0x0000, 0x3CF4,
	0x4619, 0x1900, 0x0000, 0x2CF4,
	0x17FF, 0x0000, 0x0000, 0x1CF4,
	#else
	0x0119, 0x1900, 0x0000, 0xAEF4,
	0x0219, 0x1900, 0x0000, 0x1EF4,
	0x140A, 0x0A00, 0x0000, 0x3EF4,
	0x4619, 0x1900, 0x0000, 0x2EF4,
	0x17FF, 0x0000, 0x0000, 0x1EF4,
	#endif
};

/*PIO Duration
Short (S) 			0x01
Long (L) 				0x02 
Very Long (V) 		0x03
Double (D) 			0x04
Repeat (R) 			0x05
RisingEdge (E) 		0x06
FallingEdge (F) 		0x07
Short SINGLE 			0x08
Long Release 			0x09
Very Long Release 		0x0a
Very Very Long Press 	0x0b 
Very Very Long Release 0x0c 
*/

/* PSKEY_USR_23 - System event configuration A */
static const uint16 events_a[(sizeof(event_config_type) * BM_EVENTS_PER_BLOCK)+1] =
{       
    	sizeof(event_config_type) * BM_EVENTS_PER_BLOCK,
	#ifdef BHC612_100_NewHW						/*#define PSToggleQuickly*/
	0x0207, 0x0000, 0x7FFE,
	0x0102, 0x0000, 0x4001,
	0x0303, 0x0004, 0x4006,		/*EnterPairing , Very long , States : Connectable/Pairing*/	
	0x0408, 0x0004, 0x4008,		/*InitialVoicedial , Short single , talk , Connected*/	
	#if 0
	0x4503, 0x0002, 0x5040,		/*EventCheckForAudioTransfer : Vol Down , Very long			==>TEST!!!*/
	#else
	0x0000, 0x0000, 0x0000,
	#endif
	0x0000, 0x0000, 0x0000, 
	0x0608, 0x0004, 0x6020, 
	0x0702, 0x0004, 0x4120,
	0x0801, 0x0004, 0x5050,		/*Short , Talk*/
	0x0000, 0x0000, 0x0000, 
	0x1C04, 0x0004, 0x400A,		/*EventEstablishSLC , double , Connectable&Connected*/
	0x0000, 0x0000, 0x0000,	
	0x0F01, 0x0004, 0x4200,		/*TWC:Accept a held or waiting call and release the active call , Short*/	
	0x1003, 0x0004, 0x5340,		/*TWC:Accept a held or waiting call and place the active call on hold , Very long*/
	0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,
	0x2206, 0x0000, 0xBFFE, 
	0x2307, 0x0000, 0xBFFE,
	#ifdef DEBUG_PRINT_ENABLED
	0x0101, 0x0004, 0x4001, 	/*DVT2_DEBUG_MODE*/
	#else
	0x0000, 0x0000, 0x0000, 	/*DVT2_PRINT_DISABLE*/
	#endif
	0x0000, 0x0000, 0x0000,
	0x6003, 0x0006, 0x4002, 	/*SelectTTSLanguageMode , Very long , Talk+Vol down */
	/*0x6101, 0x0004, 0x4002, 	ConfirmTTSLanguage, Short , Talk */	
	0x6108, 0x0004, 0x4002, 	/*ConfirmTTSLanguage, Short single, Talk */
	#ifdef Full_AVRCP
	0x9504,	0x0001, 0x6000,		/*EventAvrcpSkipForward*/
	0x9604,	0x0002, 0x6000		/*EventAvrcpSkipBackward*/
	#else
	0x0B04, 0x0001, 0x7040,		/*EventVolumeUp : Double*/
	0x0C04, 0x0002, 0x7040		/*EventVolumeDown : Double*/
	#endif
	#else
	0x0206, 0x2000, 0x3FFE,		/*DVT2*/
	0x0106, 0x0000, 0x4001,
	0x0303, 0x0004, 0x0006,		/*EnterPairing , Very long , States : Connectable/Pairing*/	
	0x0408, 0x0004, 0x0008,		/*InitialVoicedial , Short single , talk , Connected*/
	#if 0
	0x4503, 0x0002, 0x1040,		/*EventCheckForAudioTransfer : Vol Down , Very long			==>TEST!!!*/
	#else
	0x0000, 0x0000, 0x0000, 
	#endif
	0x0000, 0x0000, 0x0000, 
	0x0608, 0x0004, 0x2020, 
	0x0702, 0x0004, 0x0120,
	/*0x0802, 0x0004, 0x1050,		Long , Talk*/
	0x0801, 0x0004, 0x1050,		/*Short , Talk*/
	0x0000, 0x0000, 0x0000, 
	/*0x1C04, 0x0004, 0x0002,		EventEstablishSLC , double*/
	0x1C04, 0x0004, 0x000A,		/*EventEstablishSLC , double , Connectable&Connected*/
	0x0000, 0x0000, 0x0000,	
	/*0x0F02, 0x0004, 0x0300,		TWC:Accept a held or waiting call and release the active call*/
	/*0x0F02, 0x0004, 0x0200,		TWC:Accept a held or waiting call and release the active call*/
	0x0F01, 0x0004, 0x0200,		/*TWC:Accept a held or waiting call and release the active call , Short*/
	/*0x1004, 0x0004, 0x1340,		TWC:Accept a held or waiting call and place the active call on hold , Double*/
	#if 1	
	0x1003, 0x0004, 0x1340,		/*TWC:Accept a held or waiting call and place the active call on hold , Very long*/
	0x0000, 0x0000, 0x0000,
	#else
	0x1003, 0x0004, 0x0300,		/*TWC:Accept a held or waiting call and place the active call on hold , Very long*/
	0x1004,	0x0004,	0x1040,
	#endif
	0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,
	0x2206, 0x0000, 0xBFFE, 
	0x2307, 0x0000, 0xBFFE,
#ifdef DEBUG_PRINT_ENABLED
	0x0101, 0x0004, 0x0001, 	/*DVT2_DEBUG_MODE*/
#else
	0x0000, 0x0000, 0x0000, 	/*DVT2_PRINT_DISABLE*/
#endif
	0x0000, 0x0000, 0x0000,
	0x6003, 0x0006, 0x0002, 	/*SelectTTSLanguageMode , Very long , Talk+Vol down */
	/*0x6101, 0x0004, 0x0002, 	ConfirmTTSLanguage, Short , Talk */
	0x6108, 0x0004, 0x0002, 	/*ConfirmTTSLanguage, Short Single, Talk */
	#if 1
	#ifdef Full_AVRCP
		0x9504,	0x0001, 0x2000,		/*EventAvrcpSkipForward*/
		0x9604,	0x0002, 0x2000		/*EventAvrcpSkipBackward*/
	#else
		0x0B04, 0x0001, 0x3040,		/*EventVolumeUp : Double*/
		0x0C04, 0x0002, 0x3040		/*EventVolumeDown : Double*/
	#endif
	#else
	0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000
	#endif
	#endif
};

/* PSKEY_USR_24 - System event configuration B */
static const uint16 events_b[(sizeof(event_config_type) * BM_EVENTS_PER_BLOCK)+1] =
{   
    	sizeof(event_config_type) * BM_EVENTS_PER_BLOCK,	
	#ifdef BHC612_100_NewHW						/*#define PSToggleQuickly*/
	#if 1
	0x0B08, 0x0001, 0x7FF8,	/*EventVolumeUp : Short single*/
	0x0B05, 0x0001, 0x7FF8,	/*EventVolumeUp : Repeat*/
	0x0C08, 0x0002, 0x7FF8,	/*EventVolumeDown : Short single*/
	0x0C05, 0x0002, 0x7FF8, /*EventVolumeDown : Repeat*/
	#else
	0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,
	#endif
	0x0000, 0x0000, 0x0000,	/*EventTransferToggle*/ 
	0x8E04, 0x0001, 0x400A,	/*Rubidium ASR command , Double , Vol up , Connectable&Connected*/
	0x9303, 0x0004, 0x6008,	/*EventAvrcpPlayPause : Very long , Talk , headsetA2DPStreaming,Connected*/
	0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,
	0x7104, 0x0002, 0x4008,	/*Unused6(Change Primary phone) , Double Vol Down , Connected*/
	0x9203, 0x0006, 0xC004,	/*EventEnterBootMode2*/
	0x4603, 0x0005, 0xC004,
	0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,
	0x140B, 0x0006, 0x4004,
	0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,
	0x3203, 0x0004, 0x6008,	 /*EnableLEDs/Check/AVRCP_Play&Pause			- Very long                - MFB            */
	0x0000, 0x0000, 0x0000
	#else
	#if 1
	0x0B08, 0x0001, 0x3FF8,	/*EventVolumeUp : Short single*/
	0x0B05, 0x0001, 0x3FF8,	/*EventVolumeUp : Repeat*/
	0x0C08, 0x0002, 0x3FF8,	/*EventVolumeDown : Short single*/
	0x0C05, 0x0002, 0x3FF8, /*EventVolumeDown : Repeat*/
	#else
	0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,
	#endif
	/*0x0904, 0x0004, 0x1040,	EventTransferToggle : Transfer the audio of an active call to or from the AG , Talk , Double , ActiveCall,ActiveCall no SCO*/
	/*0x0903, 0x0004, 0x1040,	EventTransferToggle : Transfer the audio of an active call to or from the AG , Talk , Very long , ActiveCall,ActiveCall no SCO*/
	0x0000, 0x0000, 0x0000,	/*EventTransferToggle*/ 
	0x8E04, 0x0001, 0x000A,	/*Rubidium ASR command , Double , Vol up , Connectable&Connected*/
	0x9303, 0x0004, 0x2008,	/*EventAvrcpPlayPause : Very long , Talk , headsetA2DPStreaming,Connected*/
	0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,
	/*0x7101, 0x0006, 0x0008,	Unused6(Change Primary phone) , Talk + Vol Down , Short press , Connected*/
	0x7104, 0x0002, 0x0008,	/*Unused6(Change Primary phone) , Double Vol Down , Connected*/
	0x9203, 0x0006, 0x8004,	/*EventEnterBootMode2*/
	0x4603, 0x0005, 0x8004,
	0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,
	0x140B, 0x0006, 0x0004,
	0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000,
	/*0x3204, 0x0004, 0x2008,	 EnableLEDs/Check/AVRCP_Play&Pause			- B_DOUBLE                - MFB            */
	0x3203, 0x0004, 0x2008,	 /*EnableLEDs/Check/AVRCP_Play&Pause			- Very long                - MFB            */
	0x0000, 0x0000, 0x0000
	#endif
};

/* PSKEY_USR_25 - RSSI Pairing configuration */
static const uint16 rssi[sizeof(rssi_pairing_t)+1] =
{
    sizeof(rssi_pairing_t),
    0xFFBA,        /* Inquiry Tx Power               */
    0xFFEC,        /* RSSI Threshold                 */
    0x0005,        /* RSSI difference threshold	     */	
    0x0040, 0x0200,/* Class of device filter         */	
    0xFFEC,        /* Conn RSSI Threshold            */
    0x0005,        /* Conn RSSI difference threshold */	
};

/* PSKEY_USR_26 - Tone event configuration */
static const uint16 tone_events[(sizeof(tone_config_type) * NO_TONE_EVENTS)+1] =
{
  	sizeof(tone_config_type) * NO_TONE_EVENTS,
	0x1A18, /*Low Battery*/
	0x2616 /*Event Link Loss*/
	#if 0	
    0xFF1D, /*RingTone1*/
    0x1702, /*EventPairingSuccessful*/
    0x010B, /*EventPowerOn*/
    0x020C, /*EventPowerOff*/
    0x1C14, /*EventEstablishSLC*/
    0x1612, /*EventPairingFail*/
    0x030D, /*EventEnterPairing*/
    0x0502, /*EventLastNumberRedial*/
    0x060D, /*EventAnswer*/
    0x070E, /*EventReject*/
    0x0802, /*EventCancelEnd*/
    0x141A, /*EventResetPairedDeviceList*/
    0x280E, /*EventMuteOn*/
    0x290D, /*EventMuteOff*/
    0x2A1A, /*EventMuteReminder*/
    0x0902, /*EventTransferToggle*/
    0x0402, /*EventInitateVoiceDial*/
    0x623A, /*EventEnableMultipoint*/
    0x633B, /*EventDisableMultipoint*/
    0x6917, /*EventMultipointCallWaiting*/
    0xFE56, /*RingTone2*/
    0x0D46, /*EventToggleVolume*/
    0x2616, /*EventLinkLoss*/
    0x0E02, /*EventThreeWayReleaseAllHeld*/
    0x0F02, /*EventThreeWayAcceptWaitingReleaseActive*/
    0x1002, /*EventThreeWayAcceptWaitingHoldActive*/
    0x1102, /*EventThreeWayAddHeldTo3Way*/
    0x1202, /*EventThreeWayConnect2Disconnect*/
    0x3A02, /*EventPlaceIncomingCallOnHold*/
    0x3B0D, /*EventAcceptHeldIncomingCall*/
    0x3C0E, /*EventRejectHeldIncomingCall*/
    0x1A18, /*EventLowBattery*/
    0x7214, /*EventRssiPair*/
    0x7312, /*EventRssiPairReminder*/
    0x7412, /*EventRssiPairTimeout*/
    0x8950  /*EventPbapDialFail*/
    #endif
};


/* PSKEY_USR_27 - PIO assignment */
static const uint16 pio_block[(sizeof(config_pio_block_type))] =
{
  	2,
  	0xffff, 0xffff
};

/* PSKEY_USR_28 - unused*/
static const uint16 unused28[sizeof(config_uint16_type)] =
{
  	1,
  	0
};

/* PSKEY_USR_29 - Filter parameters
	Default: 02F0 0800 0400 099A 0295 0800 0400 08DC 0358 0000 0700 0100
*/
static const uint16 filter_config[sizeof(filter_t)+1] =
{
    sizeof(filter_t),
	0x02F0, 		/* FILTER: Gain									*/
	0x0800, 		/* FILTER: b01									*/
	0x0400, 		/* FILTER: b02									*/ 
	0x099A, 		/* FILTER: a01									*/ 
	0x0295, 		/* FILTER: a02									*/ 
	0x0800, 		/* FILTER: b11									*/ 
	0x0400, 		/* FILTER: b12									*/ 
	0x08DC, 		/* FILTER: a11									*/ 
	0x0358, 		/* FILTER: a12									*/ 
	0x0000,			/* FILTER: DC Block (1 = enable, 0 = disable) 	*/
	0x0700,			/* FILTER: VAD threshold						*/
	0x0100			/* FILTER: Echo reduction gain					*/
};


/* Default Configuration */
const config_type csr_pioneer_default_config = 
{
 	(key_type *)&battery_config,        	/* PSKEY_USR_0 - Battery configuration */
  	(key_type *)&button_config,       		/* PSKEY_USR_1 - Button configuration */
  	(key_type *)&button_pattern_config,		/* PSKEY_USR_2 - */
  	(key_type *)&supported_features_config, /* PSKEY_USR_3 - HFP Supported Features*/
  	(key_type *)&input_pio_block,        	/* PSKEY_USR_4 - Input pio block*/
  	(key_type *)&HFP_features_config,		/* PSKEY_USR_5 - The HFP Supported features*/
  	(key_type *)&timeouts, 		        	/* PSKEY_USR_6 - Timeouts*/
  	(key_type *)&tri_col_leds,             	/* PSKEY_USR_7 - Tri Colour LEDS */
  	(key_type *)&unused2,                   /* PSKEY_USR_8 - unused2*/
  	(key_type *)&lengths,                   /* PSKEY_USR_9 - length of tts, led filter, led states, led events, tones*/
  	(key_type *)&unused10,                  /* PSKEY_USR_10 - reserved */
  	(key_type *)&tts ,                  	/* PSKEY_USR_11 - unused4 */
  	(key_type *)&unused5,                   /* PSKEY_USR_12 - unused5*/
  	(key_type *)&radio,            			/* PSKEY_USR_13 - Radio configuration parameters */
	(key_type *)&ssr_config,				/* PSKEY_USR_14 - SSR parameters */
  	(key_type *)&features,          		/* PSKEY_USR_15 - Features block */
  	(key_type *)&volume,           			/* PSKEY_USR_16 - Volume gain mappings */
  	(key_type *)&hfpInit,       			/* PSKEY_USR_17 - HFP initialisation parameters */
  	(key_type *)&led_filters,         		/* PSKEY_USR_18 - LED filter configuration */
  	(key_type *)&unused19,        			/* PSKEY_USR_19 - unused */
  	(key_type *)&led_states,         		/* PSKEY_USR_20 - LED state configuration */
  	(key_type *)&vp_data,        			/* PSKEY_USR_21 - Voice Prompts Data */
  	(key_type *)&led_events,         		/* PSKEY_USR_22 - LED event configuration */
  	(key_type *)&events_a,          		/* PSKEY_USR_23 - Number of system events */
  	(key_type *)&events_b,           		/* PSKEY_USR_24 - System event configuration */
  	(key_type *)&rssi,       				/* PSKEY_USR_25 - RSSI configuration */
  	(key_type *)&tone_events,         		/* PSKEY_USR_26 - Tone event configuration */
  	(key_type *)&pio_block,          		/* PSKEY_USR_27 - PIO assignment */    
 	(key_type *)&unused28,                 	/* PSKEY_USR_28 - unused */
	(key_type *)&filter_config        		/* PSKEY_USR_29 - Filter parameters */
};
