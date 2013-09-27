/****************************************************************************

FILE NAME
    headset_init.c

DESCRIPTION
    

NOTES

*/


/****************************************************************************
    Header files
*/
#include "headset_private.h"
#include "headset_init.h"
#include "headset_config.h"
#include "headset_statemanager.h"
#include "headset_led_manager.h"
#include "headset_tones.h"
#include "headset_dut.h"
#include "charger.h"
#include "headset_a2dp.h"
#include "headset_tts.h"
#include "headset_buttons.h"
#include "headset_slc.h"
#include "headset_callmanager.h"

#include <ps.h>
#include <stdio.h>
#include <connection.h>
#include <hfp.h>
#include <pio.h>
#include <panic.h>
#include <stdlib.h>
#include <string.h>
#include <vm.h>
#include <codec.h>

/* PS keys */
#define PS_BDADDR 			(0x001)
#define PS_HFP_POWER_TABLE 	(0x360)

Configuration_t* configurationInit(void) ;

/* Lower power table for the HFP library */
const lp_power_table hfp_default_powertable[2]=
{
	/* mode,    	min_interval, max_interval, attempt, timeout, duration */
	{lp_passive,	0,	    	  0,			0,		 0,		30},    /* Passive mode 30 seconds */
    {lp_sniff,		800,	      800,			1,		 8,	    0 }     /* Enter sniff mode (500mS)*/
};


/* Lower power table for the HFP library when an audio connection is open */
const lp_power_table hfp_default_powertable_sco[2]=
{
	/* mode,    	min_interval, max_interval, attempt, timeout, duration */
	{lp_passive,		0,	    	  0,			0,		 0,		30},    /*Passive mode 30 seconds */
    {lp_sniff,		  160,	        160,			1,		 8,	    0}     /* Enter sniff mode (100mS)*/
};

/* Lower power table for the A2DP. */
const lp_power_table a2dp_stream_powertable[]=
{
	/* mode,    	min_interval, max_interval, attempt, timeout, duration */
	{lp_passive,	0,	    	  0,			0,		 0,		0} 		/* Go into passive mode and stay there */
};


/****************************************************************************
NAME	
	SetupPowerTable

DESCRIPTION
	Attempts to obtain a low power table from the Ps Key store.  If no table 
	(or an incomplete one) is found	in Ps Keys then the default is used.
	
RETURNS
	void
*/
void SetupPowerTable( void )
{

    bool Status = FALSE;
    
    /* malloc storage for power table entries */ 
    power_table *PowerTable = (power_table*)PanicUnlessMalloc(((sizeof(lp_power_table) * MAX_POWER_TABLE_ENTRIES) + sizeof(uint16)));
	
    /* attempt to retrieve all power table entries from ps */
    uint16 size_ps_key = PsFullRetrieve(PS_HFP_POWER_TABLE, PowerTable, ((sizeof(lp_power_table) * MAX_POWER_TABLE_ENTRIES) + sizeof(uint16)));

    /* initialise user power table */
    theHeadset.user_power_table = 0;

    /* check whether any pskey data exists */    
    if (size_ps_key)
    {
        /* */
        if(((!theHeadset.features.EnableA2dpStreaming)&&(PowerTable->SCOEntries)&&(PowerTable->normalEntries)&&
           (size_ps_key == ((sizeof(lp_power_table)*PowerTable->normalEntries)+(sizeof(lp_power_table)*PowerTable->SCOEntries)+(sizeof(uint16)))))
           ||
           ((theHeadset.features.EnableA2dpStreaming)&&(PowerTable->SCOEntries)&&(PowerTable->normalEntries)&&
            (PowerTable->A2DPSigEntries)&&(PowerTable->A2DPStreamEntries)&&
            (size_ps_key == ((sizeof(lp_power_table)*PowerTable->normalEntries)+
                             (sizeof(lp_power_table)*PowerTable->SCOEntries)+
                             (sizeof(lp_power_table)*PowerTable->A2DPSigEntries)+
                             (sizeof(lp_power_table)*PowerTable->A2DPStreamEntries)+                                 
                             (sizeof(uint16)))))
          )
     	{	
            /* Use user defined power table */
    		theHeadset.user_power_table = PowerTable;
            /* pskey format is correct */
            Status = TRUE;
            DEBUG(("User Power Table - Norm[%x] Sco[%x] Sig[%x] Stream[%x]\n",PowerTable->normalEntries,PowerTable->SCOEntries,PowerTable->A2DPSigEntries,PowerTable->A2DPStreamEntries));
        }
	}
	
    /* check whether pskey exists and is correctly formated */
    if(!Status)
	{	/* No/incorrect power table defined in Ps Keys - use default table */
		free(PowerTable);
        PowerTable = NULL;
        DEBUG(("No User Power Table\n"));
	}
}


/****************************************************************************
NAME    
    headsetHfpInit
    
DESCRIPTION
    Initialise HFP library

RETURNS
    void
*/

void headsetHfpInit( void )
{

    hfp_init_params hfp_params;
    
    memset(&hfp_params, 0, sizeof(hfp_init_params));
    
    /* initialise the memory for the timers */
    theHeadset.conf = configurationInit();
    
    headsetClearQueueudEvent(); 
    
    /* initialise the no of profiles variables before use */
    theHeadset.conf->no_of_profiles_connected = 0;

    /* get the extra hfp supported features such as supported sco packet types 
       from pskey user 5 */
    configManagerHFP_SupportedFeatures();

    DEBUG(("INIT: HFP Supp %s\n", (theHeadset.HFP_supp_features.HFP_Version == headset_hfp_version_1_0) ? "HFP1.0" : (theHeadset.HFP_supp_features.HFP_Version == headset_hfp_version_1_5) ? "HFP1.5" : "HFP1.6" )) ;
        
    /* initialise the hfp library with parameters read from PSKEY USER 17*/    
    configManagerHFP_Init(&hfp_params);  

    /* update HFP supported features like TWC */
    theHeadset.conf->supp_features_local = hfp_params.supported_features;
    
#ifdef APPINCLSOUNDCLEAR
	hfp_params.supported_features |= HFP_NREC_FUNCTION;
	hfp_params.disable_nrec = 1;
#endif

	/*hfp_params.disable_nrec = 0;*/

    /* initialise hfp library with pskey read configuration */
    HfpInit(&theHeadset.task, &hfp_params, NULL);
       
}

     


/****************************************************************************
NAME    
    headsetInitComplete
    
DESCRIPTION
    Headset initialisation has completed. 

RETURNS
    void
*/
void headsetInitComplete( const HFP_INIT_CFM_T *cfm )
{
    uint8 i;
    /* Make sure the profile instance initialisation succeeded. */
    if (cfm->status == hfp_init_success)
    {
        /* initialise connection status for this instance */            
        for(i=0;i<2;i++)
        {
            theHeadset.profile_data[i].status.play_tone_on_sco_disconnect = EventInvalid;
            theHeadset.profile_data[i].status.list_id = INVALID_LIST_ID;
        }                
        
        /* Disable SDP security */
        ConnectionSmSetSecurityLevel(0,1,ssp_secl4_l0,TRUE,FALSE,FALSE);
				
        /* Require MITM on the MUX (incomming and outgoing)*/
        if(theHeadset.features.ManInTheMiddle)
        {
            ConnectionSmSetSecurityLevel(0,3,ssp_secl4_l3,TRUE,TRUE,FALSE);
        }
        
        /* Register a service record if there is one to be found */
        if (get_service_record_length() )
        {
            ConnectionRegisterServiceRecord(&theHeadset.task, get_service_record_length(), get_service_record()  );
        }
            
        /* Initialise Inquiry Data to NULL */
        theHeadset.inquiry_data = NULL;
            					                        
        /* initialise the A2DP library */
        InitA2dp();        
        
#ifdef ENABLE_AVRCP
        /* initialise the AVRCP library */
        headsetAvrcpInit();
#endif        

        /*if we receive the init message in the correct state*/    
        if ( stateManagerGetState() == headsetLimbo )
        {    										
#ifdef ENABLE_PBAP						
        	/* If hfp has been initialised successfully, start initialising PBAP */		            
            DEBUG(("INIT: PBAP Init start\n"));                		
		    initPbap();
#else
            /*init the configurable parameters*/
            InitUserFeatures();                   
#endif        
        }

        /* try to get a power table entry from ps if one exists after having read the user features as
           A2DP enable state is used to determine size of power table entry */
        SetupPowerTable();
    }
    else
        /* If the profile initialisation has failed then things are bad so panic. */
        Panic();
}


/*************************************************************************
NAME    
    InitEarlyUserFeatures
    
DESCRIPTION
    This function initialises the configureation that is required early 
    on in the start-up sequence. 

RETURNS

*/
void InitEarlyUserFeatures ( void ) 
{   
	ChargerConfigure(CHARGER_SUPPRESS_LED0, TRUE);		
	
#ifdef ROM_LEDS	
    /* initialise memory for the led manager */
    LedManagerMemoryInit();
	LEDManagerInit( ) ;
#endif    
    	
    /* Initialise the Button Manager */
    buttonManagerInit() ;  
		
    /* Once system Managers are initialised, load up the configuration */
    configManagerInit();    
}   


/*************************************************************************
NAME    
    InitUserFeatures
    
DESCRIPTION
    This function initialises all of the user features - this will result in a
    poweron message if a user event is configured correctly and the headset will 
    complete the power on

RETURNS

*/
void InitUserFeatures ( void ) 
{
    /* Set to a known value*/
    theHeadset.VoiceRecognitionIsActive = FALSE ;
	theHeadset.RepeatCallerIDFlag       = TRUE;
	
    /* Enter the limbo state as we may be ON due to a charger being plugged in */
    stateManagerEnterLimboState();    
    
    if (theHeadset.VolumeOrientationIsInverted)
    {
        MessageSend ( &theHeadset.task , EventVolumeOrientationInvert , 0 ) ;
    }
	
#ifdef ROM_LEDS	
    /* set the LED enable disable state which now persists over a reset */
    if (theHeadset.theLEDTask->gLEDSEnabled)
    {
        LedManagerEnableLEDS () ;
    }
    else
    {
        LedManagerDisableLEDS () ;
    }
#endif        
    
	/*Check to see if an initial reading is enabled*/
	if (theHeadset.features.InitialBatteryLevelCheck)
	{
		BatteryUserInitiatedRead();
	}
	
	
    /* Switch the Audio CODECs power mode (TRUE = low power, FALSE = normal power). */
    SourceConfigure(StreamAudioSource(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A),
                                      STREAM_CODEC_LOW_POWER_OUTPUT_STAGE_ENABLE, 
                                      theHeadset.features.UseLowPowerAudioCodecs ? TRUE : FALSE);
    
    SourceConfigure(StreamAudioSource(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_B),
                                      STREAM_CODEC_LOW_POWER_OUTPUT_STAGE_ENABLE, 
                                      theHeadset.features.UseLowPowerAudioCodecs ? TRUE : FALSE);
    
        /*use the microphone pre-amp*/
    SourceConfigure(StreamAudioSource(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A),
                                      STREAM_CODEC_MIC_INPUT_GAIN_ENABLE, 
                                      theHeadset.features.UseLowPowerAudioCodecs ? TRUE : FALSE);
	
    SourceConfigure(StreamAudioSource(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_B),
                                      STREAM_CODEC_MIC_INPUT_GAIN_ENABLE, 
                                      theHeadset.features.UseLowPowerAudioCodecs ? TRUE : FALSE);
	
	/* Set inquiry tx power and RSSI inquiry mode */					
    ConnectionWriteInquiryTx(theHeadset.conf->rssi.tx_power);
	ConnectionWriteInquiryMode(&theHeadset.task, inquiry_mode_rssi);
	
	/* initialise HSP incoming call flag to indicate not ringing */
	theHeadset.HSPIncomingCallInd = FALSE;
	
	/*automatically power on the heasdet as soon as init is complete*/
	if ((theHeadset.features.AutoPowerOnAfterInitialisation)&&(!ChargerIsChargerConnected()))
	{
		#if 1
		MessageSend( &theHeadset.task , EventPowerOn , NULL ) ;
		#endif
		DEBUG(("Auto PowerOn\n"));  
	}
    
}



/****************************************************************************
NAME	
	timersMemoryInit

DESCRIPTION
	Initialise memory for the timers

RETURNS
	void
    
*/
Configuration_t* configurationInit(void) 
{
	/* Allocate memory to hold the power state */
	return (Configuration_t*) mallocPanic(sizeof(Configuration_t));
}

