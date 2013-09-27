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


/* PSKEY_USR_0 - Battery config */
static const uint16 battery_config[sizeof(power_config_type)+1] =
{
 	sizeof(power_config_type),
    0x8791,  /*Shutdown threshold       */   /*Gas Gauge Level 0*/   
    0x9BA5,  /*Gas Gauge Level 1	    */   /*Gas Gauge Level 2*/
    0xAF14,  /*Gas Gauge Level 3	    */   /*report Period*/
    0x00D9,  /*       */   /* Volt_enabled:1, Volt AIO(BATTERY_INTERNAL):3; Temp_enabled:1, Temp AIO (AIO1):3*/   
    0x0000,  /*lion th_m (U16)          */
    0x0000,  /*lion th_c (U16)          */
    0x0032,  /*lion td                  */   /*lion max temp*/
    0x0000   /*lion min temp            */   /*Unused*/    
};

/* PSKEY_USR_1 - Button config */
static const uint16 button_config[sizeof(button_config_type)+1] =
{
  	sizeof(button_config_type),
   	0x01F4, /* Button double press time */
   	0x03E8, /* Button long press time */
   	0x09C4, /* Button very long press time */
   	0x0320, /* Button repeat time */
   	0x1388, /* Button Very Very Long Duration*/
    0x0000  /* default debounce settings*/
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
#define NO_LED_FILTERS   (0x0009)
#define NO_LED_STATES    (0x000B)
#define NO_LED_EVENTS    (0x0005)
#define NO_TONE_EVENTS   (0x0024)
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
  	0x4000
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
  	0x1330, 0x9C01, 0x11E7, 0x22A0, 0x5800 , 0xAA00 
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
        0xfe0a, 0x0f0f  
 	,
};


/* PSKEY_USR_17 - hfp initialisation parameters */
static const uint16 hfpInit[sizeof(hfp_init_params)+1] =
{
  	7,
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
};

/* PSKEY_USR_18 - LED filter configuration */
static const uint16 led_filters[(sizeof(led_filter_config_type) * NO_LED_FILTERS)+1] =
{
 	sizeof(led_filter_config_type) * NO_LED_FILTERS,
/*1*/        0x1a00, 0x8100, 0x0000, 	/*filter on with low batt*/
/*2*/        0x2200, 0x0010, 0x0000, 	/*filter off with charger connected*/
/*3*/        0x2100, 0x0010, 0x0000, 	/*filter off with EVBATok*/    
/*4*/        0x2000, 0x800E, 0x8000,    
/*5*/        0x1E00, 0x800F, 0x8000,    
/*6*/        0x1E00, 0x0040, 0x0000,    
/*7*/        0x2000, 0x005F, 0x0000,    
/*8*/        0x2300, 0x0040, 0x0000,    
/*9*/        0x2300, 0x0050, 0x0000,     
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
                                            /* E - red , F - blu*/
	0x0164, 0x0000, 0x0000, 0x1EF4,
    0x0264, 0x0000, 0x0000, 0x1EF4,
    0x140A, 0x0A00, 0x0000, 0x2EF4,
    0x460A, 0x0000, 0x0000, 0x0EFC,
    0x8928, 0x0A00, 0x0002, 0x2EF4,    
};

/* PSKEY_USR_23 - System event configuration A */
static const uint16 events_a[(sizeof(event_config_type) * BM_EVENTS_PER_BLOCK)+1] =
{       
    sizeof(event_config_type) * BM_EVENTS_PER_BLOCK,
    0x0203, 0x0000, 0x7FFE,     /* EventPowerOff                            - B_VERY_LONG           - VREG                  */
    0x0102, 0x0000, 0x4001,     /* EventPowerOn                             - B_LONG                - VREG                  */
    0x030B, 0x0000, 0x4002,     /* EventEnterPairing                        - B_VERY_VERY_LONG      - VREG                  */
    0x0408, 0x0001, 0x200A,     /* EventInitateVoiceDial                    - B_SHORT_SINGLE        - MFB                   */

    0x0502, 0x0001, 0x0008,     /* EventLastNumberRedial                    - B_LONG                - MFB                   */
    0x4F04, 0x0001, 0x000A,     /* EventDialStoredNumber                    - B_DOUBLE              - MFB                   */
    0x0608, 0x0001, 0x0020,     /* EventAnswer                              - B_SHORT_SINGLE        - MFB                   */
    0x0702, 0x0001, 0x0020,     /* EventReject                              - B_LONG                - MFB                   */

    0x0808, 0x0001, 0x1050,     /* EventCancelEnd                           - B_SHORT_SINGLE        - MFB                   */
    0x0902, 0x0001, 0x1F40,     /* EventTransferToggle                      - B_LONG                - MFB                   */
    0x1C02, 0x0001, 0x0002,     /* EventEstablishSLC                        - B_LONG                - MFB                   */
    0x1C09, 0x0000, 0x4002,     /* EventEstablishSLC                        - B_LONG_RELEASE        - VREG                  */

    0x0F08, 0x0001, 0x0700,     /* EventThreeWayAcceptWaitingReleaseActive  - B_SHORT_SINGLE        - MFB                   */
    0x1004, 0x0001, 0x1340,     /* EventThreeWayAcceptWaitingHoldActive     - B_DOUBLE              - MFB                   */
    0x3A04, 0x0001, 0x0020,     /* EventPlaceIncomingCallOnHold             - B_DOUBLE              - MFB                   */
    0x3B08, 0x0001, 0x0800,     /* EventAcceptHeldIncomingCall              - B_SHORT_SINGLE        - MFB                   */

    0x3C02, 0x0001, 0x0800,     /* EventRejectHeldIncomingCall              - B_LONG                - MFB                   */
    0x2206, 0x0000, 0xBFFF,     /* EventChargerConnected                    - B_LOW_TO_HIGH         - CHG                   */
    0x2307, 0x0000, 0xBFFF,     /* EventChargerDisconnected                 - B_HIGH_TO_LOW         - CHG                   */
    0x1C0A, 0x0000, 0x4002,     /* EventEstablishSLC                        - B_VERY_LONG_RELEASE   - VREG                  */

    0x7202, 0x0001, 0x4006,     /* EventRssiPair                            - B_LONG                - VREG & MFB            */   
    0x8E04, 0x0001, 0x400A,     /* EventUpdateStoredNumber                  - B_DOUBLE              - VREG & MFB            */
    0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000,

    0x0000, 0x0000, 0x0000
};




/* PSKEY_USR_24 - System event configuration B */
static const uint16 events_b[(sizeof(event_config_type) * BM_EVENTS_PER_BLOCK)+1] =
{   
    sizeof(event_config_type) * BM_EVENTS_PER_BLOCK,
    0x0B01, 0x0002, 0x3FFE,     /* EventVolumeUp                            - B_SHORT               - VOL+                  */
    0x0B05, 0x0002, 0x3FFE,     /* EventVolumeUp                            - B_REPEAT              - VOL+                  */
    0x0C01, 0x0004, 0x3FFE,     /* EventVolumeDown                          - B_SHORT               - VOL-                  */
    0x0C05, 0x0004, 0x3FFE,     /* EventVolumeDown                          - B_REPEAT              - VOL-                  */

    0x0A01, 0x0006, 0x1F50,     /* EventToggleMute                          - B_SHORT               - VOL+ & VOL-           */
    0x0E01, 0x0005, 0x1340,     /* EventThreeWayReleaseAllHeld              - B_SHORT               - MFB & VOL-            */
    0x1101, 0x0003, 0x0300,     /* EventThreeWayAddHeldTo3Way               - B_SHORT               - MFB & VOL+            */
    0x1202, 0x0005, 0x0600,     /* EventThreeWayConnect2Disconnect          - B_LONG                - MFB & VOL-            */

    0x0501, 0x0003, 0x0040,     /* EventLastNumberRedial                    - B_SHORT               - MFB & VOL+            */
    0x0D03, 0x0006, 0x000E,     /* EventToggleVolume                        - B_VERY_LONG           - VOL+ & VOL-           */
    0x6201, 0x0007, 0x0006,     /* EventEnableMultipoint                    - B_SHORT               - MFB & VOL+ & VOL-     */
    0x6303, 0x0007, 0x0006,     /* EventDisableMultipoint                   - B_VERY_LONG           - MFB & VOL+ & VOL-     */

    0x140B, 0x0002, 0x5FFF,     /* EventResetPairedDeviceList               - B_VERY_VERY_LONG      - VREG & VOL-           */
    0x460B, 0x0006, 0x4001,     /* EventEnterDFUMode                        - B_VERY_VERY_LONG      - VREG & VOL+ & VOL-    */
    0x8608, 0x0000, 0x400A,     /* EventPbapDialMch                         - B_SHORT_SINGLE        - VREG                  */
    0x8704, 0x0000, 0x400A,     /* EventPbapDialIch                         - B_DOUBLE              - VREG                  */  

    0x0000, 0x0000, 0x0000,        
    0x0000, 0x0000, 0x0000,        
    0x0000, 0x0000, 0x0000,        
    0x0000, 0x0000, 0x0000,        

    0x0000, 0x0000, 0x0000,        
    0x0000, 0x0000, 0x0000,        
    0x0000, 0x0000, 0x0000,        
    0x0000, 0x0000, 0x0000,
    
    0x0000, 0x0000, 0x0000
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
