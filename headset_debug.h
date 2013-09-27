/***************************************************************************

FILE NAME
    headset_debug.h
    
DESCRIPTION
    
*/

#ifndef _HEADSET_DEBUG_H_
#define _HEADSET_DEBUG_H_


#ifndef RELEASE_BUILD /*allows the release build to ensure all of the below are removed*/
    
    /*The individual configs*/
    
 
#ifndef DO_NOT_DOCUMENT

#endif 

 /*end of DO_NOT_DOCUMENT*/
        
    /*The global debug enable*/ 
    #define DEBUG_PRINT_ENABLEDx

    #ifdef DEBUG_PRINT_ENABLED
        #define DEBUG(x) {printf x;}

        /*The individual Debug enables*/
            /*The main system messages*/
        #define DEBUG_MAINx
                /*The button manager */
        #define DEBUG_BUT_MANx
            /*The low level button parsing*/
        #define DEBUG_BUTTONSx
            /*The call manager*/
        #define DEBUG_CALL_MANx
            /*The multipoint manager*/
        #define DEBUG_MULTI_MANx
            /*headset_audio.c*/
        #define DEBUG_AUDIOx
         /* headset_slc.c */
        #define DEBUG_SLCx
            /*The charger */
        #define CHARGER_DEBUGx 
            /*The battery */
        #define BAT_DEBUGx
            /*The config manager */
        #define DEBUG_CONFIGx
            /*The LED manager */
        #define DEBUG_LMx
            /*The Lower level LED drive */
        #define DEBUG_LEDSx
            /*The Lower lvel PIO drive*/
        #define DEBUG_PIOx
            /*The power Manager*/
        #define DEBUG_POWERx
            /*tones*/
        #define DEBUG_TONESx
            /*Volume*/
        #define DEBUG_VOLUMEx
            /*State manager*/
        #define DEBUG_STATESx
          	/*authentication*/
        #define DEBUG_AUTHx
            /*dimming LEDs*/
        #define DEBUG_DIMx

        #define DEBUG_A2DPx

        #define DEBUG_TTSx
		
        #define DEBUG_FILTER_ENABLEx

        #define DEBUG_CSR2CSRx     
		
	 #define DEBUG_TempMonitorx

	 #define MyDEBUG_LMx /*BHC612 debug*/

	#define MYDEBUG_DIMx /*BHC612 debug*/

	/*debug memory allocations in the application*/
	#define DEBUG_MALLOCx
            
       /*debug PBAP*/
       #define DEBUG_PBAPx
       #define DEBUG_VCARDx

	/* LED Debug */
	#define MyDEBUG_LEDx

	#define PowerBlueMMIx

	/* ATI SoundClear plug-in*/
	/*#define APPDEBUGSOUNDCLEARPLUGINx*/

	#define DEBUG_MPMAINx

	#define DEBUG_MY_SLCx

	#define DEBUG_RUBI_TTSASRx

	#define My_DEBUGx

	#define DEBUG_AVRCPx

    #else
        #define DEBUG(x) 
    #endif /*DEBUG_PRINT_ENABLED*/

        /* If you want to carry out cVc license key checking in Production test
           Then this needs to be enabled */
    #define CVC_PRODTESTx
    
    #ifdef CVC_PRODTEST
        #define CVC_PRODTEST_PASS           0x0001
        #define CVC_PRODTEST_FAIL           0x0002
        #define CVC_PRODTEST_NO_CHECK       0x0003
        #define CVC_PRODTEST_FILE_NOT_FOUND 0x0004
    #endif

#else /*RELEASE_BUILD*/    

/*used by the build script to include the debug but none of the individual debug components*/
    #ifdef DEBUG_BUILD 
        #define DEBUG(x) {printf x;}
    #else
        #define DEBUG(x) 
    #endif
        
#endif


#ifdef BC5_MULTIMEDIA
    #define NO_BOOST_CHARGE_TRAPS
	#undef ENABLE_ENERGY_FILTER
   /* #define ENABLE_VOICE_PROMPTS */
#endif

#ifdef BC5_MULTIMEDIA_ROM
	#undef ENABLE_ENERGY_FILTER
	#define ENABLE_VOICE_PROMPTS
#endif

#ifdef BC6_AUDIO_ROM
	#define ENABLE_ENERGY_FILTER
	#define INCLUDE_NO_DSP_AURISTREAM
	#define ENABLE_VOICE_PROMPTS
#endif

#ifdef BC6_AUDIO_MM_ROM 
	#undef ENABLE_ENERGY_FILTER
	#define INCLUDE_NO_DSP_AURISTREAMx
	#define ENABLE_VOICE_PROMPTS
#endif

/* Text to Speech and Voice Prompts */

#ifdef FULL_TEXT_TO_SPEECH
 #define TEXT_TO_SPEECH
 #define TEXT_TO_SPEECH_DIGITS
 #define TEXT_TO_SPEECH_PHRASES
 #define TEXT_TO_SPEECH_NAMES
 #define TEXT_TO_SPEECH_LANGUAGESELECTION
#endif
 
#ifdef CSR_SIMPLE_TEXT_TO_SPEECH
 #define TEXT_TO_SPEECH
 #define TEXT_TO_SPEECH_DIGITS
#endif

#ifdef ENABLE_VOICE_PROMPTS
 #define CSR_VOICE_PROMPTS
 #define TEXT_TO_SPEECH
 #define TEXT_TO_SPEECH_DIGITS
 #define TEXT_TO_SPEECH_PHRASES
 #define TEXT_TO_SPEECH_LANGUAGESELECTION
#endif


#endif /*_HEADSET_DEBUG_H_*/

