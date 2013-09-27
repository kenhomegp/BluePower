/*!
@file    headset_a2dp.c
@brief   a2dp initialisation and control functions
*/

#include "headset_debug.h"
#include "headset_statemanager.h"
#include "headset_states.h"
#include "headset_a2dp.h"
#include "headset_private.h"
#include "headset_debug.h"
#include "headset_slc.h"
#include "headset_audio.h"

#ifdef ENABLE_AVRCP
#include "headset_tones.h"
#endif        

#include <bdaddr.h>
#include <a2dp.h>
#include <codec.h>
#include <connection.h>
#include <hfp.h>
#include <stdlib.h>
#include <memory.h>
#include <panic.h>
#include <ps.h>

#include <csr_a2dp_decoder_common_plugin.h>
#include "default_aac_service_record.h"

#include "headset_tts.h"

#ifdef DEBUG_A2DP
#define A2DP_DEBUG(x) DEBUG(x)
#else
#define A2DP_DEBUG(x) 
#endif

#include <rubidium_text_to_speech_plugin.h> 


static const sep_config_type sbc_sep = { SBC_SEID, KALIMBA_RESOURCE_ID, sep_media_type_audio, a2dp_sink, 1, 0, sizeof(sbc_caps_sink), sbc_caps_sink };

/* not all codecs are available for some configurations, include this define to have access to all codec types  */
#ifdef A2DP_EXTRA_CODECS
    static const sep_config_type mp3_sep = { MP3_SEID, KALIMBA_RESOURCE_ID, sep_media_type_audio, a2dp_sink, 1, 0, sizeof(mp3_caps_sink), mp3_caps_sink };
    static const sep_config_type aac_sep = { AAC_SEID, KALIMBA_RESOURCE_ID, sep_media_type_audio, a2dp_sink, 1, 0, sizeof(aac_caps_sink), aac_caps_sink };
    static const sep_config_type faststream_sep = { FASTSTREAM_SEID, KALIMBA_RESOURCE_ID, sep_media_type_audio, a2dp_sink, 1, 0, sizeof(faststream_caps_sink), faststream_caps_sink };
#endif

#define NUM_OPTIONAL_CODECS	(NUM_SEPS-1)

typedef struct
{
	unsigned				seid:8;		/* The unique ID for the SEP. */
	unsigned				bit:8;		/* The bit position in PSKEY_USR_xxx to enable the codec. */
	const sep_config_type	*config;	/* The SEP config data. These configs are defined above. */
	TaskData				*plugin;	/* The audio plugin to use. */
} optional_codec_type;

#ifdef A2DP_EXTRA_CODECS
    /* Table which indicates which A2DP codecs are avaiable on the headset.
       Add any other codecs to the bottom of the table.
    */
    static const optional_codec_type optionalCodecList[NUM_OPTIONAL_CODECS] = 
    {
    	{MP3_SEID, MP3_CODEC_BIT, &mp3_sep, (TaskData *)&csr_mp3_decoder_plugin}
       ,{AAC_SEID, AAC_CODEC_BIT, &aac_sep, (TaskData *)&csr_aac_decoder_plugin}
       ,{FASTSTREAM_SEID, FASTSTREAM_CODEC_BIT, &faststream_sep, (TaskData *)&csr_faststream_sink_plugin}
    };
#endif

/****************************************************************************
  FUNCTIONS
*/


/*************************************************************************
NAME    
    InitA2dp
    
DESCRIPTION
    This function initialises the A2DP library and supported codecs

RETURNS
    A2DP_INIT_CFM message returned, handled by A2DP message handler
**************************************************************************/
void InitA2dp(void)
{
#ifdef A2DP_EXTRA_CODECS
    bool aac_enable = FALSE;
    uint16 i;
#endif    
    sep_data_type seps[NUM_SEPS];
    uint16 number_of_seps = 0;
	seps[number_of_seps].in_use = FALSE;
	
	A2DP_DEBUG(("INIT: A2DP\n")); 

  	/*allocate the memory for the a2dp link data */
#ifdef ENABLE_AVRCP
    theHeadset.a2dp_link_data = mallocPanic( sizeof(a2dp_data) + sizeof(avrcp_data) );
#else    
	theHeadset.a2dp_link_data = mallocPanic( sizeof(a2dp_data));
#endif    
    /* initialise structure to 0 */    
    memset(theHeadset.a2dp_link_data, 0, ( sizeof(a2dp_data)));  

    /* initialise device and stream id's to invalid as 0 is a valid value */
/***************************************/
/* THIS FAILS TO COMPILE PROPERLY USING NATIVE COMPILER */
/*    for(i=0;i<(a2dp_secondary+1);i++)*/
    
/*    {*/
        theHeadset.a2dp_link_data->device_id[0] = INVALID_DEVICE_ID;
        theHeadset.a2dp_link_data->stream_id[0] = INVALID_STREAM_ID;   
        theHeadset.a2dp_link_data->device_id[1] = INVALID_DEVICE_ID;
        theHeadset.a2dp_link_data->stream_id[1] = INVALID_STREAM_ID;   
/*    }*/
    
    /* only continue and initialise the A2DP library if it's actually required,
       library functions will return false if it is uninitialised */
    if(theHeadset.features.EnableA2dpStreaming)
    {
        /* Currently, we just support MP3 as optional codec so it is enabled in features block
        If more optional codecs are supported, we need use seperate PSKEY to enable codec ! */	        
        seps[number_of_seps].sep_config = &sbc_sep;
        /* one stream end point available */
        number_of_seps++;
    
        
#ifdef A2DP_EXTRA_CODECS
        for (i=0; i<NUM_OPTIONAL_CODECS; i++)
        {
            if (theHeadset.features.A2dpOptionalCodecsEnabled & (1<<optionalCodecList[i].bit))
            {
                if ((1<<optionalCodecList[i].bit) & (1<<AAC_CODEC_BIT))
                    aac_enable = TRUE;
                
                seps[number_of_seps].sep_config = optionalCodecList[i].config;
       
                seps[number_of_seps].in_use = FALSE;
                number_of_seps++;
                A2DP_DEBUG(("INIT: Optional Codec Enabled %d\n",i)); 
            }
        }
#endif
    
    
#ifdef A2DP_EXTRA_CODECS
        /* when using aac codec specify the service entry */
        if (aac_enable)	
        {   
            service_record_type sdp;
    
            sdp.size_service_record_a = sizeof(a2dp__aac_sink_service_record);
            sdp.service_record_a = a2dp__aac_sink_service_record;
            sdp.size_service_record_b = 0;
            sdp.service_record_b = NULL;
      
            A2DP_DEBUG(("INIT: AAC Enabled\n")); 
    
            /* Initialise the A2DP library */
            A2dpInit(&theHeadset.task, A2DP_INIT_ROLE_SINK, &sdp, number_of_seps, seps,theHeadset.conf->timeouts.A2dpLinkLossReconnectionTime_s);
        }
        else
#endif
        /* when not using aac use the default service record entry */
        {
            A2DP_DEBUG(("INIT: AAC not Enabled\n")); 
            /* Initialise the A2DP library */
            A2dpInit(&theHeadset.task, A2DP_INIT_ROLE_SINK, NULL, number_of_seps, seps,theHeadset.conf->timeouts.A2dpLinkLossReconnectionTime_s);
        }
    }
}

/*************************************************************************
NAME    
    getA2dpIndex
    
DESCRIPTION
    This function tries to find a device id match in the array of a2dp links 
    to that device id passed in

RETURNS
    match status of true or false
**************************************************************************/
bool getA2dpIndex(uint16 DeviceId, uint16 * Index)
{
    uint8 i;
    
    /* go through A2dp connection looking for device_id match */
    for_all_a2dp(i)
    {
        /* if the a2dp link is connected check its device id */
        if(theHeadset.a2dp_link_data->connected[i])
        {
            /* if a device_id match is found return its value and a
               status of successfull match found */
            if(theHeadset.a2dp_link_data->device_id[i] == DeviceId)
            {
                *Index = i;
            	A2DP_DEBUG(("A2dp: getIndex = %d\n",i)); 
                return TRUE;
            }
        }
    }
    /* no matches found so return not successfull */    
    return FALSE;
}

/*************************************************************************
NAME    
    getA2dpPlugin
    
DESCRIPTION
    This function returns the task of the appropriate audio plugin to be used
    for the selected codec type when connecting audio

RETURNS
    task of relevant audio plugin
**************************************************************************/
Task getA2dpPlugin(uint8 seid)
{
	if (seid == SBC_SEID)
    
		return (TaskData *)&csr_sbc_decoder_plugin;
	
#ifdef A2DP_EXTRA_CODECS
    {
    	uint16 i;
        
        for (i=0; i<NUM_OPTIONAL_CODECS; i++)
    	{
    		if (optionalCodecList[i].seid == seid)
    			return optionalCodecList[i].plugin;
    	}
    }
#endif
    
	/* No plugin found so Panic */
	Panic();
	
	return 0;
}

/****************************************************************************
NAME    
    headsetA2dpInitComplete
    
DESCRIPTION
    Headset A2DP initialisation has completed, check for success. 

RETURNS
    void
**************************************************************************/
void headsetA2dpInitComplete(const A2DP_INIT_CFM_T *msg)
{   
    /* check for successfull initialisation of A2DP libraray */
    if(msg->status == a2dp_success)
    {
        A2DP_DEBUG(("A2DP Init Success\n"));
    }
    else
    {
	    DEBUG(("A2DP Init Failed [Status %d]\n", msg->status));
        Panic();
    }
}

/*************************************************************************
NAME    
    handleA2DPSignallingConnected
    
DESCRIPTION
    handle a successfull confirm of a signalling channel connected

RETURNS
    
**************************************************************************/
void handleA2DPSignallingConnected(a2dp_status_code status, uint16 DeviceId, bdaddr SrcAddr)
{
    /* check for successfull connection */
    if (status != a2dp_success)
    {
		A2DP_DEBUG(("Signalling Failed [Status %d]\n", status));
						
        /* connection was not successfull, if the headset does not have two devices
           connected to it try and connect another one */
#ifdef ENABLE_AVRCP
        headsetAvrcpCheckManualConnectReset(&SrcAddr);        
#endif        
    }
    /* connection was successfull */
    else
	{
		/* check for a link loss condition, if the device has suffered a link loss and was
           succesfully reconnected by the a2dp library a 'signalling connected' event will be 
           generated, check for this and retain previous connected ID for this indication */
        if(((theHeadset.a2dp_link_data->connected[a2dp_primary])&&(BdaddrIsSame(&SrcAddr, &theHeadset.a2dp_link_data->bd_addr[a2dp_primary])))||
           ((theHeadset.a2dp_link_data->connected[a2dp_secondary])&&(BdaddrIsSame(&SrcAddr, &theHeadset.a2dp_link_data->bd_addr[a2dp_secondary]))))
        {
            /* reconnection is the result of a link loss, don't assign a new id */    		
            A2DP_DEBUG(("Signalling Connected following link loss [Status %d]\n", status));

			#ifdef New_MMI
			if(theHeadset.BHC612_LinkLoss)
			{
				theHeadset.BHC612_LinkLoss = false;
				theHeadset.BHC612_LinkLossReconnect = true;
				A2DP_DEBUG(("A2DP:Clear BHC612_LinkLoss flag!!!\n"));
				if(theHeadset.sco_sink != 0)
				{
					AudioDisconnect();
            		theHeadset.sco_sink = 0;
					A2DP_DEBUG(("Audio Disconnect!!\n"));
				}
				/*MessageSendLater(&theHeadset.task, EventEstablishSLC,0, 2500);Check BT connection again!*/
				MessageSendLater(&theHeadset.task, EventSpare2 , 0 , 2500);/*Recover Battery icon*/
			}
			#endif
        }
        else
        {
            /* store the device_id for the new connection in the first available storage position */
            if(!theHeadset.a2dp_link_data->connected[a2dp_primary])
            {
            	A2DP_DEBUG(("Signalling Success, Primary ID = %x\n",DeviceId));
                theHeadset.a2dp_link_data->connected[a2dp_primary] = TRUE;
                theHeadset.a2dp_link_data->device_id[a2dp_primary] = DeviceId;
                theHeadset.a2dp_link_data->bd_addr[a2dp_primary] = SrcAddr;            
		#ifdef BHC612		
		A2DP_DEBUG(("Primary BD addr : nap %04x uap %02x lap %08lx\n",SrcAddr.nap,SrcAddr.uap,SrcAddr.lap));			
		#endif
            }
            /* this is the second A2DP signalling connection */
            else
            {
            	A2DP_DEBUG(("Signalling Success, Secondary ID = %x\n",DeviceId));
                theHeadset.a2dp_link_data->connected[a2dp_secondary] = TRUE;
                theHeadset.a2dp_link_data->device_id[a2dp_secondary] = DeviceId;
                theHeadset.a2dp_link_data->bd_addr[a2dp_secondary] = SrcAddr;            
		#ifdef BHC612		
		A2DP_DEBUG(("Secondary BD addr : nap %04x uap %02x lap %08lx\n",SrcAddr.nap,SrcAddr.uap,SrcAddr.lap));			
		#endif
            }
        }
        
  		/* Ensure the underlying ACL is encrypted */       
        ConnectionSmEncrypt( &theHeadset.task , A2dpSignallingGetSink(DeviceId) , TRUE );

        /* We are now connected */
		if (stateManagerGetState() < headsetConnected)
            stateManagerEnterConnectedState(); 	
	
        /* update number of connected devices */
	    theHeadset.conf->no_of_profiles_connected = GetNumberOfConnectedDevices();

		/* If we were in pairing mode then update HFP state also */
		if (stateManagerGetState() == headsetConnDiscoverable)
			stateManagerEnterConnectableState(FALSE);
	
		/* If the headset is off then disconnect */
		if (stateManagerGetState() == headsetLimbo)
    	{      
       		A2dpSignallingDisconnectRequest(DeviceId);
    	}
		else
		{
	   		uint8 lAttributes[ATTRIBUTE_SIZE];
            uint16 Id;
						
			/* For a2dp connected Tone only */
			MessageSend(&theHeadset.task,  EventA2dpConnected, 0);	
					
            /* find structure index of deviceId */
            if(getA2dpIndex(DeviceId, &Id))
            {
                /* Retrieve attributes for this device */
       			if (ConnectionSmGetAttributeNow(ATTRIBUTE_PSKEY_BASE, &SrcAddr, ATTRIBUTE_SIZE, lAttributes))
    			{
                    /* if no volume info stored then use default volume level */
				    if (lAttributes[attribute_a2dp_volume])	
				    	theHeadset.a2dp_link_data->gAvVolumeLevel[Id] = lAttributes[attribute_a2dp_volume];
				    else
				    	theHeadset.a2dp_link_data->gAvVolumeLevel[Id] = theHeadset.features.DefaultA2dpVolLevel;                   	                               
                    
                    /* if no clock mismatch info stored then use default */
				    if (lAttributes[attribute_a2dp_clock_mismatch])	
				    	theHeadset.a2dp_link_data->clockMismatchRate[Id] = lAttributes[attribute_a2dp_clock_mismatch];
				    else
				    	theHeadset.a2dp_link_data->clockMismatchRate[Id] = 0;               
    	            
                    A2DP_DEBUG(("Signalling Success, Volume = %x, Clock Mismatch = %x\n",
                                theHeadset.a2dp_link_data->gAvVolumeLevel[Id],theHeadset.a2dp_link_data->clockMismatchRate[Id]));

                    /* store the profile information for this recently connected device */    
                    StoreProfilePsKeyInfo(headset_a2dp, 0, Id);   
                }
                /* check on signalling check indication if the a2dp was previously in a suspended state,
                   this can happen if the headset has suspended a stream and the phone has chosen to drop
                   the signalling channel completely, open the media connection */
                if(theHeadset.a2dp_link_data->SuspendState[Id])
                    A2dpMediaOpenRequest(theHeadset.a2dp_link_data->device_id[Id]);
			}   
            
#ifdef ENABLE_AVRCP
            if (theHeadset.avrcp_link_data->avrcp_manual_connect)
                theHeadset.avrcp_link_data->avrcp_play_addr = SrcAddr;
            headsetAvrcpConnect(&theHeadset.a2dp_link_data->bd_addr[Id], DEFAULT_AVRCP_CONNECTION_DELAY);     
#endif            
		}
	}
}

/*************************************************************************
NAME    
    handleA2DPOpenInd
    
DESCRIPTION
    handle an indication of an media channel open request, decide whether 
    to accept or reject it

RETURNS
    
**************************************************************************/
void handleA2DPOpenInd(uint16 DeviceId)
{
   	A2DP_DEBUG(("A2dp: OpenInd DevId = %d\n",DeviceId)); 

    
    /* accept this media connection */
    if(A2dpMediaOpenResponse(DeviceId, TRUE))    
    {
        uint16 Id;
		A2DP_DEBUG(("Open Success\n"));
           
        /* find structure index of deviceId */
        if(getA2dpIndex(DeviceId, &Id))
            theHeadset.a2dp_link_data->device_id[Id] = DeviceId;

    }
}

/*************************************************************************
NAME    
    handleA2DPOpenCfm
    
DESCRIPTION
    handle a successfull confirm of a media channel open

RETURNS
    
**************************************************************************/
void handleA2DPOpenCfm(uint16 DeviceId, uint16 StreamId, a2dp_status_code status)
{
	/* ensure successfull confirm status */
	if (status == a2dp_success)
	{
        uint16 Id;
		A2DP_DEBUG(("Open Success\n"));
           
        /* find structure index of deviceId */
        if(getA2dpIndex(DeviceId, &Id))
        {
            /* set the current seid */         
            theHeadset.a2dp_link_data->device_id[Id] = DeviceId;
            theHeadset.a2dp_link_data->stream_id[Id] = StreamId;
        }
        
        /* Start the Streaming if if in the suspended state */
        if(theHeadset.a2dp_link_data->SuspendState[Id])
        {
            A2dpMediaStartRequest(DeviceId, StreamId);
      		A2DP_DEBUG(("Open Success - suspended - start streaming\n"));
        }
	}
	else
	{
		A2DP_DEBUG(("Open Failure [result = %d]\n", status));
	}	
}

/*************************************************************************
NAME    
    handleA2DPClose
    
DESCRIPTION
    handle the close of a media channel 

RETURNS
    
**************************************************************************/
static void handleA2DPClose(uint16 DeviceId, uint16 StreamId, a2dp_status_code status)
{		
    
    /* check the status of the close indication/confirm */    
    if(status == a2dp_success)
    {        
        uint16 Id;

        Sink sink = A2dpSignallingGetSink(DeviceId);
        
       	A2DP_DEBUG(("A2dp: Close DevId = %d, StreamId = %d\n",DeviceId,StreamId)); 

        /* route the audio using the appropriate codec/plugin */
 	    audioHandleRouting();

        /* update the link policy */
	    slcSetA2DPLinkPolicy(DeviceId, StreamId, sink);

        /* change headset state if currently in one of the A2DP specific states */
        if(stateManagerGetState() == headsetA2DPStreaming)
        {
            /* the enter connected state function will determine if the signalling
               channel is still open and make the approriate state change */
            stateManagerEnterConnectedState();
        }
        
        /* user configurable event notification */
        MessageSend(&theHeadset.task, EventA2dpDisconnected, 0);
        
        /* find structure index of deviceId */
        if(getA2dpIndex(DeviceId, &Id))
        {
            /* store current volume level in ps */
            StoreProfilePsKeyInfo(headset_a2dp, 0, Id);   
        }
 
#ifdef ENABLE_AVRCP
        {
            /* assume device is stopped for AVRCP 1.0 devices */
            if(getA2dpIndex(DeviceId, &Id))
                headsetAvrcpSetPlayStatus(&theHeadset.a2dp_link_data->bd_addr[Id], avrcp_play_status_stopped);
        }
#endif        
	}
    else
       	A2DP_DEBUG(("A2dp: Close FAILED status = %d\n",status)); 

}

/*************************************************************************
NAME    
    handleA2DPSignallingDisconnected
    
DESCRIPTION
    handle the disconnection of the signalling channel
RETURNS
    
**************************************************************************/
void handleA2DPSignallingDisconnected(uint16 DeviceId, a2dp_status_code status,  bdaddr SrcAddr)
{
    uint16 Id;

    /* check for successful disconnection status */
    if((status == a2dp_success)&&(getA2dpIndex(DeviceId, &Id)))
    {
       	A2DP_DEBUG(("A2dp: SigDiscon DevId = %d\n",DeviceId)); 
        /* update the a2dp parameter values */
        theHeadset.a2dp_link_data->connected[Id] = FALSE;
        theHeadset.a2dp_link_data->device_id[Id] = INVALID_DEVICE_ID;
        theHeadset.a2dp_link_data->stream_id[Id] = INVALID_STREAM_ID; 
#ifdef ENABLE_AVRCP
        theHeadset.a2dp_link_data->avrcp_support[Id] = avrcp_support_unknown;
#endif        

        /* update number of connected devices */
	    theHeadset.conf->no_of_profiles_connected = GetNumberOfConnectedDevices();

        /*if the headset is off then this is disconnect as part of the power off cycle*/	
	    if ( stateManagerGetState() != headsetLimbo)
        {
            /* Update the app state if we are on and both */
	        if ((stateManagerIsConnected()) && (!theHeadset.conf->no_of_profiles_connected))
	            stateManagerEnterConnectableState( FALSE ) ;
        }

#ifdef ENABLE_AVRCP
        headsetAvrcpDisconnect(&SrcAddr);     
#endif

    }    
    else
       	A2DP_DEBUG(("A2dp: Sig Discon FAILED status = %d\n",status)); 

}
            
/*************************************************************************
NAME    
    handleA2DPStartStreaming
    
DESCRIPTION
    handle the indication of media start from either the ind or the cfm
RETURNS
    
**************************************************************************/
void handleA2DPStartStreaming(uint16 DeviceId, uint16 StreamId, a2dp_status_code status)
{
    /* check success status of indication or confirm */
    if(status == a2dp_success)
    {
        uint16 Id;     
        Sink sink = A2dpMediaGetSink(DeviceId, StreamId);
 
        A2DP_DEBUG(("A2dp: StartStreaming DevId = %d, StreamId = %d\n",DeviceId,StreamId));    
        /* find structure index of deviceId */
		
        if(getA2dpIndex(DeviceId, &Id))
        {
			#ifdef Rubidium
			if(theHeadset.TTS_ASR_Playing)
			{
				ToneTerminate();
				TTSTerminate();
				#if 0
				AudioDisconnect();
				theHeadset.sco_sink = 0;
				#endif
    			UnloadRubidiumEngine();
				theHeadset.TTS_ASR_Playing = false;
				A2DP_DEBUG(("A2dp : Stop TTS_ASR!!!\n"));
			}
			#endif
			
            /* route the audio using the appropriate codec/plugin */
      	    audioHandleRouting();
       	    /* enter the stream a2dp state if not in a call */
   	        stateManagerEnterA2dpStreamingState();
           /* update the link policy */
    	    slcSetA2DPLinkPolicy(DeviceId, StreamId, sink);
           /* set the current seid */         
            theHeadset.a2dp_link_data->stream_id[Id] = StreamId;
            /* store current volume level in ps */
            StoreProfilePsKeyInfo(headset_a2dp, 0, Id);   

#ifdef ENABLE_AVRCP
            /* any AVRCP commands should be targeted to the device which is streaming A2DP */ 
            headsetAvrcpSetActiveConnection(&theHeadset.a2dp_link_data->bd_addr[Id]);    
            /* assume device is playing for AVRCP 1.0 devices */
            headsetAvrcpSetPlayStatus(&theHeadset.a2dp_link_data->bd_addr[Id], avrcp_play_status_playing);
#endif 
        }
    }
    else
       	A2DP_DEBUG(("A2dp: StartStreaming FAILED status = %d\n",status)); 

}             

/*************************************************************************
NAME    
    handleA2DPSuspendStreaming
    
DESCRIPTION
    handle the indication of media suspend from either the ind or the cfm
RETURNS
    
**************************************************************************/
void handleA2DPSuspendStreaming(uint16 DeviceId, uint16 StreamId, a2dp_status_code status)
{
    uint16 Id;
    Sink sink = A2dpMediaGetSink(DeviceId, StreamId);
    
    /* if the suspend was not successfull, issue a close instead */
    if(status == a2dp_rejected_by_remote_device)
    {
       	A2DP_DEBUG(("A2dp: Suspend Failed= %x, try close DevId = %d, StreamId = %d\n",status,DeviceId,StreamId)); 
        /* suspend failed so close media streaming instead */
        A2dpMediaCloseRequest(DeviceId, StreamId);
    }
    /* check success status of indication or confirm */
    else 
    {
       	A2DP_DEBUG(("A2dp: Suspend Ok DevId = %d, StreamId = %d\n",DeviceId,StreamId)); 
        /* no longer streaming so enter connected state if applicable */    	
        if(stateManagerGetState() == headsetA2DPStreaming)
        {
            /* the enter connected state function will determine if the signalling
               channel is still open and make the approriate state change */
            stateManagerEnterConnectedState();
        }
        /* route the audio using the appropriate codec/plugin */
 	    audioHandleRouting();
        /* update the link policy */
  	    slcSetA2DPLinkPolicy(DeviceId, StreamId, sink);
        /* find structure index of deviceId */
        if(getA2dpIndex(DeviceId, &Id))
        {
            /* store current volume level in ps */
            StoreProfilePsKeyInfo(headset_a2dp, 0, Id);   
        }
    }
#ifdef ENABLE_AVRCP
    /* assume device is paused for AVRCP 1.0 devices */
    headsetAvrcpSetPlayStatus(&theHeadset.a2dp_link_data->bd_addr[Id], avrcp_play_status_paused);
#endif
}
  
/*************************************************************************
NAME    
    SuspendA2dpStream
    
DESCRIPTION
    called when it is necessary to suspend an a2dp media stream due to 
    having to process a call from a different AG 
RETURNS
    
**************************************************************************/
void SuspendA2dpStream(uint8 index)
{
   	A2DP_DEBUG(("A2dp: Suspend A2DP Stream %x\n",index)); 

    /* set the local suspend status indicator */
    theHeadset.a2dp_link_data->SuspendState[index] = TRUE;

    /* attempt to suspend stream, if not successfull then close it */
    if(!A2dpMediaSuspendRequest(theHeadset.a2dp_link_data->device_id[index], theHeadset.a2dp_link_data->stream_id[index]))
    {
        /* suspend failed so close media streaming */
        A2dpMediaCloseRequest(theHeadset.a2dp_link_data->device_id[index], theHeadset.a2dp_link_data->stream_id[index]);
    }

    /* no longer streaming so enter connected state if applicable */    	
    if(stateManagerGetState() == headsetA2DPStreaming)
    {
        /* the enter connected state function will determine if the signalling
           channel is still open and make the approriate state change */
        stateManagerEnterConnectedState();
    }
}
   

#ifdef ENABLE_AVRCP
/*************************************************************************
NAME    
    getA2dpVolume
    
DESCRIPTION
    Retrieve the A2DP volume for the connection to the device with the address specified.
    
RETURNS
    Returns TRUE if the volume was retrieved, FALSE otherwise.
    The actual volume is returned in the a2dp_volume variable.
    
**************************************************************************/
bool getA2dpVolume(const bdaddr *bd_addr, uint16 *a2dp_volume)
{
    uint8 i;
    
    /* go through A2dp connection looking for match */
    for_all_a2dp(i)
    {
        /* if the a2dp link is connected check its bdaddr */
        if(theHeadset.a2dp_link_data->connected[i])
        {
            /* if a match is found return its volume level and a
               status of successfull match found */
            if(BdaddrIsSame(&theHeadset.a2dp_link_data->bd_addr[i], bd_addr))
            {
                *a2dp_volume = theHeadset.a2dp_link_data->gAvVolumeLevel[i];
            	A2DP_DEBUG(("A2dp: getVolume = %d\n", i)); 
                return TRUE;
            }
        }
    }
    /* no matches found so return not successfull */    
    return FALSE;
}  


/*************************************************************************
NAME    
    setA2dpVolume
    
DESCRIPTION
    Sets the A2DP volume for the connection to the device with the address specified.
    
RETURNS
    Returns TRUE if the volume was set, FALSE otherwise.
    
**************************************************************************/
bool setA2dpVolume(const bdaddr *bd_addr, uint16 a2dp_volume)
{
    uint8 i;
    
    /* go through A2dp connection looking for match */
    for_all_a2dp(i)
    {
        /* if the a2dp link is connected check its bdaddr */
        if(theHeadset.a2dp_link_data->connected[i])
        {
            /* if a match is found set its volume level and a
               status of successfull match found */
            if(BdaddrIsSame(&theHeadset.a2dp_link_data->bd_addr[i], bd_addr))
            {
                /* get current volume for this profile */
                uint16 lOldVol = theHeadset.a2dp_link_data->gAvVolumeLevel[i];
                
                theHeadset.a2dp_link_data->gAvVolumeLevel[i] = a2dp_volume;                                                                      
              
            	A2DP_DEBUG(("A2dp: setVolume = %d\n", i));
                
                if(theHeadset.sco_sink && (theHeadset.sco_sink == A2dpMediaGetSink(theHeadset.a2dp_link_data->device_id[i], theHeadset.a2dp_link_data->stream_id[i])))
                {
                    if(theHeadset.a2dp_link_data->gAvVolumeLevel[i] == VOLUME_A2DP_MAX_LEVEL)  
                    	{
                        MessageSend ( &theHeadset.task , EventVolumeMax , 0 );
				A2DP_DEBUG(("A2dp: A2DP_MAX_LEVEL\n"));
                    	}
					
                    if(theHeadset.a2dp_link_data->gAvVolumeLevel[i] == VOLUME_A2DP_MIN_LEVEL)                             
                        MessageSend ( &theHeadset.task , EventVolumeMin , 0 );

					#if 1
					VolumeSetA2dp(i, lOldVol, 0);
					#else
                    			VolumeSetA2dp(i, lOldVol);
					#endif
                }
                return TRUE;
            }
        }
    }
    /* no matches found so return not successfull */    
    return FALSE;
}    
#endif

/*************************************************************************
NAME    
    handleA2DPStoreClockMismatchRate
    
DESCRIPTION
    handle storing the clock mismatch rate for the active stream
RETURNS
    
**************************************************************************/
void handleA2DPStoreClockMismatchRate(uint16 clockMismatchRate)   
{
    a2dp_stream_state a2dpStatePri = a2dp_stream_idle;
    a2dp_stream_state a2dpStateSec = a2dp_stream_idle;   
    Sink a2dpSinkPri = 0;
    Sink a2dpSinkSec = 0;
        
    /* if a2dp connected obtain the current streaming state for primary a2dp connection */
    if(theHeadset.a2dp_link_data->connected[a2dp_primary])
    {
        a2dpStatePri = A2dpMediaGetState(theHeadset.a2dp_link_data->device_id[a2dp_primary], theHeadset.a2dp_link_data->stream_id[a2dp_primary]) ;
        a2dpSinkPri = A2dpMediaGetSink(theHeadset.a2dp_link_data->device_id[a2dp_primary], theHeadset.a2dp_link_data->stream_id[a2dp_primary]) ;
    }    

    /* if a2dp connected obtain the current streaming state for secondary a2dp connection */
    if(theHeadset.a2dp_link_data->connected[a2dp_secondary])
    {
        a2dpStateSec = A2dpMediaGetState(theHeadset.a2dp_link_data->device_id[a2dp_secondary], theHeadset.a2dp_link_data->stream_id[a2dp_secondary]) ;
        a2dpSinkSec = A2dpMediaGetSink(theHeadset.a2dp_link_data->device_id[a2dp_secondary], theHeadset.a2dp_link_data->stream_id[a2dp_secondary]) ;
    } 
    
    /* Determine which a2dp source this is for */
    if((a2dpStatePri == a2dp_stream_streaming) && (a2dpSinkPri == theHeadset.sco_sink))  
    {
        A2DP_DEBUG(("A2dp: store pri. clk mismatch = %x\n", clockMismatchRate));
        theHeadset.a2dp_link_data->clockMismatchRate[a2dp_primary] = clockMismatchRate;
    }
    else if((a2dpStateSec == a2dp_stream_streaming) && (a2dpSinkSec == theHeadset.sco_sink))  
    {
        A2DP_DEBUG(("A2dp: store sec. clk mismatch = %x\n", clockMismatchRate));
        theHeadset.a2dp_link_data->clockMismatchRate[a2dp_secondary] = clockMismatchRate;
    }
    else
    {
        A2DP_DEBUG(("A2dp: ERROR NO A2DP STREAM, clk mismatch = %x\n", clockMismatchRate));
    }
}
                
/*************************************************************************
NAME    
    handleA2DPMessage
    
DESCRIPTION
    A2DP message Handler, this function handles all messages returned
    from the A2DP library and calls the relevant functions if required

RETURNS
    
**************************************************************************/
void handleA2DPMessage( Task task, MessageId id, Message message )
{
    A2DP_DEBUG(("A2DP_MSG id=%x : \n",id));
    
    switch (id)
    {
/******************/
/* INITIALISATION */
/******************/
        
        /* confirmation of the initialisation of the A2DP library */
        case A2DP_INIT_CFM:
            A2DP_DEBUG(("A2DP_INIT_CFM : \n"));
            headsetA2dpInitComplete((A2DP_INIT_CFM_T *) message);
        break;

/*****************************/        
/* SIGNALING CHANNEL CONTROL */
/*****************************/

        /* indication of a remote source trying to make a signalling connection */		
	    case A2DP_SIGNALLING_CONNECT_IND:
		{
	        A2DP_DEBUG(("A2DP_SIGNALLING_CHANNEL_CONNECT_IND : \n"));
            /* before accepting check there isn't already a signalling channel connected to another AG */		
            if ((theHeadset.features.EnableA2dpStreaming)&&
                ((!theHeadset.a2dp_link_data->connected[a2dp_primary])||
                (!theHeadset.a2dp_link_data->connected[a2dp_secondary])))
        	{
            	A2DP_DEBUG(("Accept\n"));
				A2dpSignallingConnectResponse(((A2DP_SIGNALLING_CONNECT_IND_T *)message)->device_id,TRUE);
#ifdef ENABLE_AVRCP
                headsetAvrcpCheckManualConnectReset(&((A2DP_SIGNALLING_CONNECT_IND_T *)message)->addr);        
#endif                
        	}
			else
        	{

                A2DP_DEBUG(("Reject\n"));
				A2dpSignallingConnectResponse(((A2DP_SIGNALLING_CONNECT_IND_T *)message)->device_id,FALSE);
        	}
		}
		break;

        /* confirmation of a signalling connection attempt, successfull or not */
	    case A2DP_SIGNALLING_CONNECT_CFM:
            A2DP_DEBUG(("A2DP_SIGNALLING_CHANNEL_CONNECT_CFM : \n"));
	    	handleA2DPSignallingConnected(((A2DP_SIGNALLING_CONNECT_CFM_T*)message)->status, 
	    								  ((A2DP_SIGNALLING_CONNECT_CFM_T*)message)->device_id, 
                                          ((A2DP_SIGNALLING_CONNECT_CFM_T*)message)->addr);
	    break;
        
        /* indication of a signalling channel disconnection having occured */
    	case A2DP_SIGNALLING_DISCONNECT_IND:
            A2DP_DEBUG(("A2DP_SIGNALLING_CHANNEL_DISCONNECT_IND : \n"));
        	handleA2DPSignallingDisconnected(((A2DP_SIGNALLING_DISCONNECT_IND_T*)message)->device_id,
                                             ((A2DP_SIGNALLING_DISCONNECT_IND_T*)message)->status,
                                             ((A2DP_SIGNALLING_DISCONNECT_IND_T*)message)->addr);
		break;
        
/*************************/        
/* MEDIA CHANNEL CONTROL */        
/*************************/
        
        /* indication of a remote device attempting to open a media channel */      
        case A2DP_MEDIA_OPEN_IND:
            A2DP_DEBUG(("A2DP_OPEN_IND : \n"));
        	handleA2DPOpenInd(((A2DP_MEDIA_OPEN_IND_T*)message)->device_id);
        break;
		
        /* confirmation of request to open a media channel */
        case A2DP_MEDIA_OPEN_CFM:
            A2DP_DEBUG(("A2DP_OPEN_CFM : \n"));
        	handleA2DPOpenCfm(((A2DP_MEDIA_OPEN_CFM_T*)message)->device_id, 
    						  ((A2DP_MEDIA_OPEN_CFM_T*)message)->stream_id, 
    						  ((A2DP_MEDIA_OPEN_CFM_T*)message)->status);
        break;
        	
        /* indication of a request to close the media channel, remotely generated */
        case A2DP_MEDIA_CLOSE_IND:
            A2DP_DEBUG(("A2DP_CLOSE_IND : \n"));
            handleA2DPClose(((A2DP_MEDIA_CLOSE_IND_T*)message)->device_id,
                            ((A2DP_MEDIA_CLOSE_IND_T*)message)->stream_id,
                            ((A2DP_MEDIA_CLOSE_IND_T*)message)->status);
        break;

        /* confirmation of the close of the media channel, locally generated  */
        case A2DP_MEDIA_CLOSE_CFM:
           A2DP_DEBUG(("A2DP_CLOSE_CFM : \n"));
           handleA2DPClose(0,0,a2dp_success);
        break;

/**********************/          
/*  STREAMING CONTROL */
/**********************/          
        
        /* indication of start of media streaming from remote source */
        case A2DP_MEDIA_START_IND:
            A2DP_DEBUG(("A2DP_START_IND : \n"));
         	handleA2DPStartStreaming(((A2DP_MEDIA_START_IND_T*)message)->device_id,
                                     ((A2DP_MEDIA_START_IND_T*)message)->stream_id,
                                       a2dp_success);
        break;
		
        /* confirmation of a local request to start media streaming */
        case A2DP_MEDIA_START_CFM:
            A2DP_DEBUG(("A2DP_START_CFM : \n"));
    	    handleA2DPStartStreaming(((A2DP_MEDIA_START_CFM_T*)message)->device_id,
                                     ((A2DP_MEDIA_START_CFM_T*)message)->stream_id,
                                     ((A2DP_MEDIA_START_CFM_T*)message)->status);
        break;
        
        case A2DP_MEDIA_SUSPEND_IND:
            A2DP_DEBUG(("A2DP_SUSPEND_IND : \n"));
        	handleA2DPSuspendStreaming(((A2DP_MEDIA_SUSPEND_IND_T*)message)->device_id,
                                       ((A2DP_MEDIA_SUSPEND_IND_T*)message)->stream_id,
                                         a2dp_success);
        break;
		
        case A2DP_MEDIA_SUSPEND_CFM:
            A2DP_DEBUG(("A2DP_SUSPEND_CFM : \n"));
        	handleA2DPSuspendStreaming(((A2DP_MEDIA_SUSPEND_CFM_T*)message)->device_id,
                                       ((A2DP_MEDIA_SUSPEND_CFM_T*)message)->stream_id,
                                       ((A2DP_MEDIA_SUSPEND_CFM_T*)message)->status);
        break;

/*************************/
/* MISC CONTROL MESSAGES */
/*************************/
        
        /* link loss indication */
        case A2DP_SIGNALLING_LINKLOSS_IND:
             A2DP_DEBUG(("A2DP_SIGNALLING_LINKLOSS_IND : \n"));
			MessageSend(&theHeadset.task,  EventLinkLoss, 0);	
        break;           
		
	    case A2DP_ENCRYPTION_CHANGE_IND:
            A2DP_DEBUG(("A2DP_ENCRYPTION_CHANGE_IND : \n"));
		break;
			
        default:       
	    	A2DP_DEBUG(("A2DP UNHANDLED MSG: 0x%x\n",id));
        break;
    }    
}
