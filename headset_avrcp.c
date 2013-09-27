#ifdef ENABLE_AVRCP

/*!
@file    headset_avrcp.c
@brief   AVRCP functionality
*/

#include "headset_avrcp.h"
#include "headset_debug.h"
#include "headset_statemanager.h"

#include <bdaddr.h>
#include <string.h>


#ifdef DEBUG_AVRCP
#define AVRCP_DEBUG(x) DEBUG(x)
#else
#define AVRCP_DEBUG(x) 
#endif

#ifdef DEBUG_AVRCP
#define My_AVRCP_DEBUG(x) DEBUG(x)
#else
#define My_AVRCP_DEBUG(x)
#endif


/*********************** Helper Functions ***************************/

/*************************************************************************
NAME    
    avrcpSendControlMessage
    
DESCRIPTION
    Queue up message to send an AVRCP command based on pending_cmd flag.
    The flag is used so that a response to an AVRCP command must be received 
    before another AVRCP command can be sent.

**************************************************************************/
static void avrcpSendControlMessage(avrcp_controls control, uint16 Index)
{
	AVRCP_CONTROL_SEND_T *message = PanicUnlessNew(AVRCP_CONTROL_SEND_T);
	message->control = control;
    message->index = Index;
	MessageSendConditionally(&theHeadset.avrcp_link_data->avrcp_ctrl_handler, AVRCP_CONTROL_SEND, message, &theHeadset.avrcp_link_data->pending_cmd[Index]);
    
    if (theHeadset.avrcp_link_data->cmd_queue_size[Index] < AVRCP_MAX_PENDING_COMMANDS)
        theHeadset.avrcp_link_data->cmd_queue_size[Index]++;
    
    AVRCP_DEBUG(("    AVRCP pending commands %d\n", theHeadset.avrcp_link_data->cmd_queue_size[Index]));
}

/*************************************************************************
NAME    
    sendAvrcpPassthroughCmd
    
DESCRIPTION
    Sends an AVRCP PASSTHROUGH command. Sets the pending_cmd flag so that the device waits
    for a response before sending another command.

**************************************************************************/
static void sendAvrcpPassthroughCmd(avc_operation_id op_id, uint8 state, uint16 Index)
{
    theHeadset.avrcp_link_data->pending_cmd[Index] = TRUE;
    if (theHeadset.avrcp_link_data->cmd_queue_size[Index])
        theHeadset.avrcp_link_data->cmd_queue_size[Index]--;
    /* Send a key press */
    AvrcpPassthroughRequest(theHeadset.avrcp_link_data->avrcp[Index], subunit_panel, 0, state, op_id, 0, 0);   
}

/*************************************************************************
NAME    
    isAvrcpCategory1MetadataEnabled
    
DESCRIPTION
    Checks that the remote device has Category 1 Metadata enabled. This covers the Metadata 
    that was introduced in the AVRCP 1.3 specification.

RETURNS
    Returns TRUE if Category 1 Metadata enabled, FALSE otherwise.
**************************************************************************/
static bool isAvrcpCategory1MetadataEnabled(uint16 Index)
{
    if ((theHeadset.avrcp_link_data->features[Index] & AVRCP_CATEGORY_1) && (theHeadset.avrcp_link_data->extensions[Index] & AVRCP_VERSION_1_3))
        return TRUE;

    return FALSE;
}

/*************************************************************************
NAME    
    isAvrcpPlaybackStatusSupported
    
DESCRIPTION
    Checks that the remote device supports playback status changed notifications.

RETURNS
    Returns TRUE if supported, FALSE otherwise.
**************************************************************************/
static bool isAvrcpPlaybackStatusSupported(uint16 Index)
{
    if (theHeadset.avrcp_link_data->registered_events[Index] & (1 << avrcp_event_playback_status_changed))
        return TRUE;

    return FALSE;
}

/*************************************************************************
NAME    
    getAvrcpQueueSpace
    
DESCRIPTION
    Gets the free space in the AVRCP command queue

RETURNS
    Returns the number of AVRCP commands that can be queued.
**************************************************************************/
static uint16 getAvrcpQueueSpace(uint16 Index)
{
    return AVRCP_MAX_PENDING_COMMANDS - theHeadset.avrcp_link_data->cmd_queue_size[Index];
}



/*********************** AVRCP Local Message Handling ***************************/

/*************************************************************************
NAME    
    avrcpControlHandler
    
DESCRIPTION
    Handles all application created AVRCP messages.

**************************************************************************/
static void avrcpControlHandler(Task task, MessageId id, Message message)
{	
    if (id == AVRCP_CONTROL_SEND)
    {
        uint16 Index = ((AVRCP_CONTROL_SEND_T *)message)->index;
        
        /* ignore if not connected */
        if (!theHeadset.avrcp_link_data->connected[Index])
            return;
        
        AVRCP_DEBUG(("AVRCP_CONTROL_SEND :\n"));
        
        switch (((AVRCP_CONTROL_SEND_T *)message)->control)
        {
            case AVRCP_CTRL_PAUSE_PRESS:
                AVRCP_DEBUG(("  Sending Pause Pressed\n"));
                sendAvrcpPassthroughCmd(opid_pause, 0, Index);
                break;
            case AVRCP_CTRL_PAUSE_RELEASE:
                AVRCP_DEBUG(("  Sending Pause Released\n"));
                sendAvrcpPassthroughCmd(opid_pause, 1, Index);
                break;
            case AVRCP_CTRL_PLAY_PRESS:
                AVRCP_DEBUG(("  Sending Play Pressed\n"));
                sendAvrcpPassthroughCmd(opid_play, 0, Index);
                break;
            case AVRCP_CTRL_PLAY_RELEASE:
                AVRCP_DEBUG(("  Sending Play Released\n"));
                sendAvrcpPassthroughCmd(opid_play, 1, Index);
                break;
            case AVRCP_CTRL_FORWARD_PRESS:
                AVRCP_DEBUG(("  Sending Forward Pressed\n"));
                sendAvrcpPassthroughCmd(opid_forward, 0, Index);
                break;
            case AVRCP_CTRL_FORWARD_RELEASE:
                AVRCP_DEBUG(("  Sending Forward Released\n"));
                sendAvrcpPassthroughCmd(opid_forward, 1, Index);
                break;
            case AVRCP_CTRL_BACKWARD_PRESS:
                AVRCP_DEBUG(("  Sending Backward Pressed\n"));
                sendAvrcpPassthroughCmd(opid_backward, 0, Index);
                break;
            case AVRCP_CTRL_BACKWARD_RELEASE:
                AVRCP_DEBUG(("  Sending Backward Released\n"));
                sendAvrcpPassthroughCmd(opid_backward, 1, Index);
                break;
            case AVRCP_CTRL_STOP_PRESS:
                AVRCP_DEBUG(("  Sending Stop Pressed\n"));
                sendAvrcpPassthroughCmd(opid_stop, 0, Index);
                break;
            case AVRCP_CTRL_STOP_RELEASE:
                AVRCP_DEBUG(("  Sending Stop Released\n"));
                sendAvrcpPassthroughCmd(opid_stop, 1, Index);
                break;
            case AVRCP_CTRL_FF_PRESS:
                AVRCP_DEBUG(("  Sending FF Pressed\n"));
                sendAvrcpPassthroughCmd(opid_fast_forward, 0, Index);
                break;
            case AVRCP_CTRL_FF_RELEASE:
                AVRCP_DEBUG(("  Sending FF Released\n"));
                sendAvrcpPassthroughCmd(opid_fast_forward, 1, Index);
                break;
            case AVRCP_CTRL_REW_PRESS:
                AVRCP_DEBUG(("  Sending REW Pressed\n"));
                sendAvrcpPassthroughCmd(opid_rewind, 0, Index);
                break;
            case AVRCP_CTRL_REW_RELEASE:
                AVRCP_DEBUG(("  Sending REW Released\n"));
                sendAvrcpPassthroughCmd(opid_rewind, 1, Index);
                break;
            default:
                break;
        }
    }
    else if (id == AVRCP_CREATE_CONNECTION)
    {        
        AVRCP_DEBUG(("AVRCP_CREATE_CONNECTION :\n"));
        headsetAvrcpConnect(&((AVRCP_CREATE_CONNECTION_T *)message)->bd_addr, 0);
    }
}


/*********************** AVRCP Library Message Handling Functions ***************************/

/******************/
/* INITIALISATION */
/******************/

/*************************************************************************
NAME    
    headsetAvrcpInitComplete
    
DESCRIPTION
    Functionality as a result of receiving AVRCP_INIT_CFM from the AVRCP library.

**************************************************************************/
static void headsetAvrcpInitComplete(const AVRCP_INIT_CFM_T *msg)
{   
    /* check for successfull initialisation of AVRCP libraray */
    if(msg->status == avrcp_success)
    {
        AVRCP_DEBUG(("  AVRCP Init Success [SDP handle 0x%lx]\n", msg->sdp_handle));
    }
    else
    {
	    DEBUG(("    AVRCP Init Failed [Status %d]\n", msg->status));
        Panic();
    }
}

/******************************/
/* CONNECTION / DISCONNECTION */
/******************************/

/*************************************************************************
NAME    
    handleAvrcpConnectCfm
    
DESCRIPTION
    Functionality as a result of receiving AVRCP_CONNECT_CFM from the AVRCP library.

**************************************************************************/
static void handleAvrcpConnectCfm(AVRCP_CONNECT_CFM_T *msg)
{
    uint8 i = 0;
    
	if(msg->status == avrcp_success)
	{	
        AVRCP_DEBUG(("  AVRCP Connect Success [avrcp 0x%x][bdaddr %x:%x:%lx][sink 0x%x]\n",
                     (uint16)msg->avrcp,
                     msg->bd_addr.nap, msg->bd_addr.uap, msg->bd_addr.lap,
                     (uint16)msg->sink));
        
        /* If the headset is off then disconnect */
		if (stateManagerGetState() == headsetLimbo)
    	{      
       		AvrcpDisconnectRequest(msg->avrcp);
    	}
        else
        {
            uint16 a2dp_volume;
            
		    /* Ensure the underlying ACL is encrypted */       
		    ConnectionSmEncrypt(&theHeadset.task , msg->sink , TRUE);
        
            /* Get the supported profile extensions */
            AvrcpGetProfileExtensions(msg->avrcp);
            
            for_all_avrcp(i)
            {
                if (!theHeadset.avrcp_link_data->connected[i])
                {
                    theHeadset.avrcp_link_data->connected[i] = TRUE;
                    theHeadset.avrcp_link_data->bd_addr[i] = msg->bd_addr;
                    theHeadset.avrcp_link_data->play_status[i] = avrcp_play_status_stopped;                    
                    theHeadset.avrcp_link_data->avrcp[i] = msg->avrcp;
                    break;
                }
            }       
            
            headsetAvrcpUdpateActiveConnection();
                       
            if (!getA2dpVolume(&theHeadset.a2dp_link_data->bd_addr[i], &a2dp_volume))
                a2dp_volume = theHeadset.features.DefaultA2dpVolLevel;
                
            headsetAvrcpSetLocalVolume(i, a2dp_volume);  
            
            /* set play status depending on streaming state */          
            for_all_a2dp(i)
            {            
                if (theHeadset.a2dp_link_data->connected[i] && BdaddrIsSame(&theHeadset.a2dp_link_data->bd_addr[i], &theHeadset.avrcp_link_data->bd_addr[i]))
                {
                    theHeadset.a2dp_link_data->avrcp_support[i] = avrcp_support_supported;
                    /* check whether the a2dp connection is streaming data */
                    if (A2dpMediaGetState(theHeadset.a2dp_link_data->device_id[i], theHeadset.a2dp_link_data->stream_id[i]) == a2dp_stream_streaming)
                    {
                        headsetAvrcpSetPlayStatus(&theHeadset.a2dp_link_data->bd_addr[i], avrcp_play_status_playing);
                    }
                    break;
                } 
            }
        }        
    }
    else
    {
        AVRCP_DEBUG(("  AVRCP Connect Fail [status %d]\n", msg->status));
        for_all_a2dp(i)
        {            
            /* test to see if A2DP connection exists so that AVRCP status can be updated */
            if (theHeadset.a2dp_link_data->connected[i] && BdaddrIsSame(&theHeadset.a2dp_link_data->bd_addr[i], &msg->bd_addr))
            {
                if (theHeadset.a2dp_link_data->avrcp_support[i] == avrcp_support_unknown)
                {
                    /* try AVRCP connection again */
                    theHeadset.a2dp_link_data->avrcp_support[i] = avrcp_support_second_attempt;
                    headsetAvrcpConnect(&msg->bd_addr, 500);
                    AVRCP_DEBUG(("      AVRCP 2nd attempt\n"));
                    return;
                }
                else
                {
                    /* give up on connecting AVRCP */
                    theHeadset.a2dp_link_data->avrcp_support[i] = avrcp_support_unsupported;
                    AVRCP_DEBUG(("      AVRCP unsupported\n"));
                }
                break;
            } 
        }
    }
    
    if (theHeadset.avrcp_link_data->avrcp_manual_connect && BdaddrIsSame(&theHeadset.avrcp_link_data->avrcp_play_addr, &msg->bd_addr))
    {
        if (msg->status == avrcp_success)
        {
            AVRCP_DEBUG(("AVRCP manual connect - send play\n"));
            headsetAvrcpPlay();
        }
        theHeadset.avrcp_link_data->avrcp_manual_connect = FALSE;
        BdaddrSetZero(&theHeadset.avrcp_link_data->avrcp_play_addr);
    }    
}

/*************************************************************************
NAME    
    handleAvrcpConnectInd
    
DESCRIPTION
    Functionality as a result of receiving AVRCP_CONNECT_IND from the AVRCP library.

**************************************************************************/
static void handleAvrcpConnectInd(AVRCP_CONNECT_IND_T *msg)
{
    uint8 i = 0;
	bool accept = FALSE;	
	
    for_all_avrcp(i)
    {
        if (!theHeadset.avrcp_link_data->connected[i])
            accept = TRUE;
    }
    
    AVRCP_DEBUG(("  AVRCP Connect Response [accept %d]\n", accept));
            
	AvrcpConnectResponse(&theHeadset.task, msg->connection_id, msg->signal_id, accept);
}

/*************************************************************************
NAME    
    handleAvrcpDisconnectInd
    
DESCRIPTION
    Functionality as a result of receiving AVRCP_DISCONNECT_IND from the AVRCP library.

**************************************************************************/
static void handleAvrcpDisconnectInd(AVRCP_DISCONNECT_IND_T *msg)
{
    uint16 Index;
    
    if (headsetAvrcpGetIndexFromInstance(msg->avrcp, &Index))
    {
        theHeadset.avrcp_link_data->connected[Index] = FALSE;
        theHeadset.avrcp_link_data->pending_cmd[Index] = FALSE;
        theHeadset.avrcp_link_data->cmd_queue_size[Index] = 0;
        theHeadset.avrcp_link_data->registered_events[Index] = 0;
        theHeadset.avrcp_link_data->features[Index] = 0;
        theHeadset.avrcp_link_data->avrcp[Index] = NULL;
        headsetAvrcpUdpateActiveConnection();
    }    
}

/*****************/
/* AV/C COMMANDS */
/*****************/

/*************************************************************************
NAME    
    handleAvrcpPassthroughCfm
    
DESCRIPTION
    Functionality as a result of receiving AVRCP_PASSTHROUGH_CFM from the AVRCP library.

**************************************************************************/
static void handleAvrcpPassthroughCfm(AVRCP_PASSTHROUGH_CFM_T *msg)
{
    /* 
        Clearing the pending flag should allow another
        pending event to be delivered to controls_handler 
    */
    uint16 Index;
    
    if (headsetAvrcpGetIndexFromInstance(msg->avrcp, &Index))
    {
        theHeadset.avrcp_link_data->pending_cmd[Index] = FALSE;
    }
}

/*************************************************************************
NAME    
    handleAvrcpPassthroughInd
    
DESCRIPTION
    Functionality as a result of receiving AVRCP_PASSTHROUGH_IND from the AVRCP library.

**************************************************************************/
static void handleAvrcpPassthroughInd(AVRCP_PASSTHROUGH_IND_T *msg)
{
    uint16 Index;
    uint16 i;
    
    /* Acknowledge the request */	
	if ((msg->opid == opid_volume_up) || (msg->opid == opid_volume_down))
	{        
		/* The headset should accept volume up commands as it supports AVRCP TG category 2. */
		AvrcpPassthroughResponse(msg->avrcp, avctp_response_accepted);
	
		/* Adjust the local volume only if it is a press command and the A2DP is streaming */
		if (!msg->state && headsetAvrcpGetIndexFromInstance(msg->avrcp, &Index))
        {                 
            for_all_a2dp(i)
            {
                if(BdaddrIsSame(&theHeadset.a2dp_link_data->bd_addr[i], &theHeadset.avrcp_link_data->bd_addr[Index]))
                {              
                    if(theHeadset.sco_sink && (theHeadset.sco_sink == A2dpMediaGetSink(theHeadset.a2dp_link_data->device_id[i], theHeadset.a2dp_link_data->stream_id[i])))
                    {
                        if (msg->opid == opid_volume_up)
                        {
                            AVRCP_DEBUG(("   received vol up\n"));
    		    	        CheckVolumeA2dp(increase_volume);  
                        }
                        else
                        {
                            AVRCP_DEBUG(("   received vol down\n"));
                            CheckVolumeA2dp(decrease_volume);  
                        }
                    }
                }
            }
        }
	}	
	else
    {
        AVRCP_DEBUG(("   received invalid passthrough [%d]\n", msg->opid));
		/* The headset won't accept any other commands. */
    	AvrcpPassthroughResponse(msg->avrcp, avctp_response_not_implemented);
    }
}

/*************************************************************************
NAME    
    handleAvrcpUnitInfoInd
    
DESCRIPTION
    Functionality as a result of receiving AVRCP_UNITINFO_IND from the AVRCP library.

**************************************************************************/
static void handleAvrcpUnitInfoInd(AVRCP_UNITINFO_IND_T *msg)
{
    /* Headset is a CT and TG, so send the correct response to UnitInfo requests. */
	uint32 company_id = 0xffffff; /* IEEE RAC company ID can be used here */
	AvrcpUnitInfoResponse(msg->avrcp, TRUE, subunit_panel, 0, company_id);	
}

/*************************************************************************
NAME    
    handleAvrcpSubUnitInfoInd
    
DESCRIPTION
    Functionality as a result of receiving AVRCP_SUBUNITINFO_IND from the AVRCP library.

**************************************************************************/
static void handleAvrcpSubUnitInfoInd(AVRCP_SUBUNITINFO_IND_T *msg)
{
	/* Headset is a CT and TG, so send the correct response to SubUnitInfo requests. */
	uint8 page_data[4];
	page_data[0] = 0x48; /* subunit_type: panel; max_subunit_ID: 0 */
	page_data[1] = 0xff;
	page_data[2] = 0xff;
	page_data[3] = 0xff;
	AvrcpSubUnitInfoResponse(msg->avrcp, TRUE, page_data);	
}

/*************************************************************************
NAME    
    handleAvrcpVendorDependentInd
    
DESCRIPTION
    Functionality as a result of receiving AVRCP_VENDORDEPENDENT_IND from the AVRCP library.

**************************************************************************/
static void handleAvrcpVendorDependentInd(AVRCP_VENDORDEPENDENT_IND_T *msg)
{
    /*
        Reject all vendor requests.
    */
	AvrcpVendorDependentResponse(msg->avrcp, avctp_response_not_implemented);
}

/******************/
/* AVRCP Metadata */
/******************/

/*************************************************************************
NAME    
    handleAvrcpRegisterNotificationInd
    
DESCRIPTION
    Functionality as a result of receiving AVRCP_REGISTER_NOTIFICATION_IND from the AVRCP library.

**************************************************************************/
static void handleAvrcpRegisterNotificationInd(AVRCP_REGISTER_NOTIFICATION_IND_T *msg)
{
    uint16 Index;
    
    AVRCP_DEBUG(("   event_id [%d]\n", msg->event_id));
    
    switch (msg->event_id)
    {
        case avrcp_event_volume_changed:
            if (headsetAvrcpGetIndexFromInstance(msg->avrcp, &Index))
            {
                theHeadset.avrcp_link_data->registered_events[Index] |= (1 << msg->event_id);
                AVRCP_DEBUG(("   registered event [%d] index [%d]\n", theHeadset.avrcp_link_data->registered_events[Index], Index));
                AvrcpEventVolumeChangedResponse(theHeadset.avrcp_link_data->avrcp[Index], 
                                        avctp_response_interim,
                                        theHeadset.avrcp_link_data->absolute_volume[Index]);
            }
            break;
     
        default:
            /* other registrations should not be received */
            break;
    }
}

/*************************************************************************
NAME    
    handleAvrcpSetAbsVolInd
    
DESCRIPTION
    Functionality as a result of receiving AVRCP_SET_ABSOLUTE_VOLUME_IND from the AVRCP library.

**************************************************************************/
static void handleAvrcpSetAbsVolInd(AVRCP_SET_ABSOLUTE_VOLUME_IND_T *msg)
{
    uint16 Index;
    uint16 a2dp_volume;
            
    AVRCP_DEBUG(("   volume [%d]\n", msg->volume));
    
    if (headsetAvrcpGetIndexFromInstance(msg->avrcp, &Index))
    {        
        AvrcpSetAbsoluteVolumeResponse(theHeadset.avrcp_link_data->avrcp[Index], 
                                   avctp_response_accepted, 
                                   msg->volume);  
        
        theHeadset.avrcp_link_data->absolute_volume[Index] = msg->volume;
        
        /* convert AVRCP absolute volume to A2DP volume level */
        a2dp_volume = msg->volume / AVRCP_ABS_VOL_STEP_CHANGE;
                
        setA2dpVolume(&theHeadset.avrcp_link_data->bd_addr[Index], a2dp_volume);
    }
}

/*************************************************************************
NAME    
    handleAvrcpGetPlayStatusCfm
    
DESCRIPTION
    Functionality as a result of receiving AVRCP_GET_PLAY_STATUS_CFM from the AVRCP library.

**************************************************************************/
static void handleAvrcpGetPlayStatusCfm(AVRCP_GET_PLAY_STATUS_CFM_T *msg)
{
    uint16 Index;
                
    AVRCP_DEBUG(("   play status cfm [%d] [%d]\n", msg->status, msg->play_status));
    
    if ((msg->status == avrcp_success) && headsetAvrcpGetIndexFromInstance(msg->avrcp, &Index))
    {
        if ((msg->play_status == avrcp_play_status_fwd_seek) || (msg->play_status == avrcp_play_status_rev_seek))
            theHeadset.avrcp_link_data->play_status[Index] = 1 << msg->play_status;
        else
            theHeadset.avrcp_link_data->play_status[Index] = msg->play_status;
    }
}

/*************************************************************************
NAME    
    handleAvrcpPlayStatusChangedInd
    
DESCRIPTION
    Functionality as a result of receiving AVRCP_EVENT_PLAYBACK_STATUS_CHANGED_IND from the AVRCP library.

**************************************************************************/
static void handleAvrcpPlayStatusChangedInd(AVRCP_EVENT_PLAYBACK_STATUS_CHANGED_IND_T *msg)
{
    uint16 Index;
                
    AVRCP_DEBUG(("   play status ind [%d] [%d]\n", msg->response, msg->play_status));
    
    if (headsetAvrcpGetIndexFromInstance(msg->avrcp, &Index))
    {
        if ((msg->response == avctp_response_changed) || (msg->response == avctp_response_interim))
        {
            if ((msg->play_status == avrcp_play_status_fwd_seek) || (msg->play_status == avrcp_play_status_rev_seek))
                theHeadset.avrcp_link_data->play_status[Index] = 1 << msg->play_status;
            else
                theHeadset.avrcp_link_data->play_status[Index] = msg->play_status;
            
            /* store that this command is supported by remote device */
            theHeadset.avrcp_link_data->registered_events[Index] |= (1 << avrcp_event_playback_status_changed);
        
            if (msg->response == avctp_response_changed)
            {
                /* re-register to receive notifications */
                AvrcpRegisterNotificationRequest(msg->avrcp, avrcp_event_playback_status_changed, 0);                
            }
        }     
        else
        {
            /* assume not supported by remote device */
            theHeadset.avrcp_link_data->registered_events[Index] &= ~(1 << avrcp_event_playback_status_changed);
        }
    }
}


/********************/
/* LIBRARY SPECIFIC */
/********************/ 
            
/*************************************************************************
NAME    
    handleAvrcpGetExtensionsCfm
    
DESCRIPTION
    Functionality as a result of receiving AVRCP_GET_EXTENSIONS_CFM from the AVRCP library.

**************************************************************************/
static void handleAvrcpGetExtensionsCfm(AVRCP_GET_EXTENSIONS_CFM_T *msg)
{
    uint16 Index;
    
    if ((msg->status == avrcp_success) && headsetAvrcpGetIndexFromInstance(msg->avrcp, &Index))
    {
        theHeadset.avrcp_link_data->extensions[Index] = msg->extensions;  
        /* now retrieve supported features */
        AvrcpGetSupportedFeatures(msg->avrcp);
    }    
}

/*************************************************************************
NAME    
    handleAvrcpGetSupportedFeaturesCfm
    
DESCRIPTION
    Functionality as a result of receiving AVRCP_GET_SUPPORTED_FEATURES_CFM from the AVRCP library.

**************************************************************************/
static void handleAvrcpGetSupportedFeaturesCfm(AVRCP_GET_SUPPORTED_FEATURES_CFM_T *msg)
{
    uint16 Index;
    
    if ((msg->status == avrcp_success) && headsetAvrcpGetIndexFromInstance(msg->avrcp, &Index))
    {
        theHeadset.avrcp_link_data->features[Index] = msg->features;
        
        if (isAvrcpCategory1MetadataEnabled(Index))
        {            
            AVRCP_DEBUG(("   Category 1 Metadata enabled\n"));
            
            /* Get notified of change in playback status */
            AvrcpRegisterNotificationRequest(msg->avrcp, avrcp_event_playback_status_changed, 0);
        }
        else
        {
            AVRCP_DEBUG(("   No Metadata enabled\n"));           
        }                
    }
}


/*********************** Main Functions ***************************/

/*************************************************************************
NAME    
    headsetAvrcpInit
    
DESCRIPTION
    Initialise the AVRCP library.

**************************************************************************/
void headsetAvrcpInit(void)
{
    avrcp_init_params config;
    uint16 *buffer;

    AVRCP_DEBUG(("AVRCP: Avrcp Init\n"));
    
    /* Initialise AVRCP library */ 
    config.device_type = avrcp_target_and_controller;
    config.supported_controller_features = AVRCP_CATEGORY_1; /* mainly operates as controller */
    config.supported_target_features = AVRCP_CATEGORY_2; /* only target for volume commands */
    config.profile_extensions = AVRCP_VERSION_1_4; /* operate as AVRCP 1.4 device */
    
    AvrcpInit(&theHeadset.task, &config);
    
    /* initialise avrcp data memory (A2DP and AVRCP data share same memory allocation) */
    buffer = (uint16 *)theHeadset.a2dp_link_data;
	theHeadset.avrcp_link_data = (avrcp_data *)&buffer[sizeof(a2dp_data)];
    /* initialise structure to 0 */    
    memset(theHeadset.avrcp_link_data, 0, (sizeof(avrcp_data)));    
    /* Initialise local message handler */
    theHeadset.avrcp_link_data->avrcp_ctrl_handler.handler = avrcpControlHandler;
}


/*************************************************************************
NAME    
    headsetAvrcpDisconnect
    
DESCRIPTION
    Disconnect the AVRCP connection associated with the specified device.

**************************************************************************/
void headsetAvrcpDisconnect(const bdaddr *bd_addr)
{
    uint16 Index;
    
    AVRCP_DEBUG(("AVRCP: Avrcp Disconnect\n"));
    if (headsetAvrcpGetIndexFromBdaddr(bd_addr, &Index))
    {
        AvrcpDisconnectRequest(theHeadset.avrcp_link_data->avrcp[Index]);
    }
}


/*************************************************************************
NAME    
    headsetAvrcpDisconnectAll
    
DESCRIPTION
    Disconnect all AVRCP connections.

**************************************************************************/
void headsetAvrcpDisconnectAll(void)
{
    uint8 i;
    
    AVRCP_DEBUG(("AVRCP: Avrcp Disconnect All\n"));
    /* loop for all AVRCP connections */
    for_all_avrcp(i)
    {
        if (theHeadset.avrcp_link_data->connected[i])
            AvrcpDisconnectRequest(theHeadset.avrcp_link_data->avrcp[i]);
    }
    theHeadset.avrcp_link_data->avrcp_manual_connect = FALSE;
    BdaddrSetZero(&theHeadset.avrcp_link_data->avrcp_play_addr);
}


/*************************************************************************
NAME    
    headsetAvrcpConnect
    
DESCRIPTION
    Create an AVRCP connection with with the specified device.

**************************************************************************/
void headsetAvrcpConnect(const bdaddr *bd_addr, uint16 delay_time)
{
    uint16 connections = 0;
    uint16 i;
    
    if (!delay_time)
    {    
        AVRCP_DEBUG(("AVRCP: Avrcp Connect [%x:%x:%lx]\n", bd_addr->nap, bd_addr->uap, bd_addr->lap));
        for_all_avrcp(i)
        {
            if (theHeadset.avrcp_link_data->connected[i])
            {
                connections++;
                if (BdaddrIsSame(&theHeadset.avrcp_link_data->bd_addr[i], bd_addr))
                    return;
            }
        }
        if (connections < MAX_AVRCP_CONNECTIONS)
            AvrcpConnectRequest(&theHeadset.task, bd_addr);
    }
    else
    {
        AVRCP_CREATE_CONNECTION_T *message = PanicUnlessNew(AVRCP_CREATE_CONNECTION_T);
	    message->bd_addr = *bd_addr;
        AVRCP_DEBUG(("AVRCP: Avrcp delayed connect [%d]\n", delay_time));
        MessageSendLater(&theHeadset.avrcp_link_data->avrcp_ctrl_handler, AVRCP_CREATE_CONNECTION, message, delay_time);
    }
}


/*************************************************************************
NAME    
    headsetAvrcpPlay
    
DESCRIPTION
    Send an AVRCP_PLAY to the device with the currently active AVRCP connection.

**************************************************************************/
void headsetAvrcpPlay(void)
{
    uint16 Index = headsetAvrcpGetActiveConnection();
    
	AVRCP_DEBUG(("AVRCP: Avrcp Play\n"));
	
    if (theHeadset.avrcp_link_data->connected[Index] && (getAvrcpQueueSpace(Index) >= 2))
    {
        avrcpSendControlMessage(AVRCP_CTRL_PLAY_PRESS, Index);
        avrcpSendControlMessage(AVRCP_CTRL_PLAY_RELEASE, Index);
        if (!isAvrcpPlaybackStatusSupported(Index))
            theHeadset.avrcp_link_data->play_status[Index] = avrcp_play_status_playing;
    } 
}


/*************************************************************************
NAME    
    headsetAvrcpPause
    
DESCRIPTION
    Send an AVRCP_PAUSE to the device with the currently active AVRCP connection.

**************************************************************************/
void headsetAvrcpPause(void)
{
    uint16 Index = headsetAvrcpGetActiveConnection();
    
	AVRCP_DEBUG(("AVRCP: Avrcp Pause\n"));
	
    if (theHeadset.avrcp_link_data->connected[Index] && (getAvrcpQueueSpace(Index) >= 2))
    {
        avrcpSendControlMessage(AVRCP_CTRL_PAUSE_PRESS, Index);
        avrcpSendControlMessage(AVRCP_CTRL_PAUSE_RELEASE, Index);
        if (!isAvrcpPlaybackStatusSupported(Index))
            theHeadset.avrcp_link_data->play_status[Index] = avrcp_play_status_paused;
    }
}


/*************************************************************************
NAME    
    headsetAvrcpPlayPause
    
DESCRIPTION
    Send a AVRCP_PLAY / AVRCP_PAUSE based on play status, to the device with the currently active AVRCP connection.

**************************************************************************/
void headsetAvrcpPlayPause(void)
{
    uint16 Index = headsetAvrcpGetActiveConnection();
    
	My_AVRCP_DEBUG(("AVRCP: Avrcp "));
    if (theHeadset.avrcp_link_data->connected[Index])
    {
        if (theHeadset.avrcp_link_data->play_status[Index] == avrcp_play_status_playing)
        {
            headsetAvrcpPause();
			My_AVRCP_DEBUG(("Pause\n"));
        }
        else
        {
            headsetAvrcpPlay();
			My_AVRCP_DEBUG(("Play\n"));
        }
    }
    else
    {
        uint16 i;        
        for_all_a2dp(i)
        {
            /* see if an a2dp link is connected without AVRCP */
            if (theHeadset.a2dp_link_data->connected[i])
            {                
                if (theHeadset.a2dp_link_data->avrcp_support[i] != avrcp_support_unsupported)
                {
                    theHeadset.a2dp_link_data->avrcp_support[i] = avrcp_support_unknown;
                    theHeadset.avrcp_link_data->avrcp_manual_connect = TRUE;
                    theHeadset.avrcp_link_data->avrcp_play_addr = theHeadset.a2dp_link_data->bd_addr[i];
                    headsetAvrcpConnect(&theHeadset.a2dp_link_data->bd_addr[i], 0);
                    AVRCP_DEBUG(("AVRCP: Retry AVRCP connection\n"));
                }
                else
                {
                    /* TODO if AVRCP can't be connected, control stopping and starting of A2DP audio here */
                    AVRCP_DEBUG(("AVRCP: AVRCP not supported\n"));
                }
                return;
            }
        }
        AVRCP_DEBUG(("AVRCP: Establish SLC connections\n"));
        theHeadset.avrcp_link_data->avrcp_manual_connect = TRUE;
        MessageSend(&theHeadset.task, EventEstablishSLC, 0 );
    }
}


/*************************************************************************
NAME    
    headsetAvrcpStop
    
DESCRIPTION
    Send a AVRCP_STOP to the device with the currently active AVRCP connection.

**************************************************************************/
void headsetAvrcpStop(void)
{
    uint16 Index = headsetAvrcpGetActiveConnection();
    
	AVRCP_DEBUG(("AVRCP: Avrcp Stop\n"));
    
    if (theHeadset.avrcp_link_data->connected[Index] && (getAvrcpQueueSpace(Index) >= 2))
    {
        avrcpSendControlMessage(AVRCP_CTRL_STOP_PRESS, Index);
        avrcpSendControlMessage(AVRCP_CTRL_STOP_RELEASE, Index);
        if (!isAvrcpPlaybackStatusSupported(Index))
            theHeadset.avrcp_link_data->play_status[Index] = avrcp_play_status_stopped;
    }
}


/*************************************************************************
NAME    
    headsetAvrcpSkipForward
    
DESCRIPTION
    Send a AVRCP_FORWARD to the device with the currently active AVRCP connection.

**************************************************************************/
void headsetAvrcpSkipForward(void)
{
    uint16 Index = headsetAvrcpGetActiveConnection();
    
    AVRCP_DEBUG(("AVRCP: Avrcp Skip Fwd\n"));
    
    if (theHeadset.avrcp_link_data->connected[Index] && (getAvrcpQueueSpace(Index) >= 2))
    {
        avrcpSendControlMessage(AVRCP_CTRL_FORWARD_PRESS, Index);
        avrcpSendControlMessage(AVRCP_CTRL_FORWARD_RELEASE, Index);
    }
}


/*************************************************************************
NAME    
    headsetAvrcpSkipBackward
    
DESCRIPTION
    Send a AVRCP_BACKWARD to the device with the currently active AVRCP connection.

**************************************************************************/
void headsetAvrcpSkipBackward(void)
{
    uint16 Index = headsetAvrcpGetActiveConnection();
    
    AVRCP_DEBUG(("AVRCP: Avrcp Skip Bwd\n"));
   
    if (theHeadset.avrcp_link_data->connected[Index] && (getAvrcpQueueSpace(Index) >= 2))
    {
        avrcpSendControlMessage(AVRCP_CTRL_BACKWARD_PRESS, Index);
        avrcpSendControlMessage(AVRCP_CTRL_BACKWARD_RELEASE, Index);
    }
}


/*************************************************************************
NAME    
    headsetAvrcpFastForwardPress
    
DESCRIPTION
    Send a AVRCP_FAST_FORWARD pressed to the device with the currently active AVRCP connection.

**************************************************************************/
void headsetAvrcpFastForwardPress(void)
{
    uint16 Index = headsetAvrcpGetActiveConnection();
    
    AVRCP_DEBUG(("AVRCP: Avrcp FFWD Press\n"));
    
    if (theHeadset.avrcp_link_data->connected[Index] && (getAvrcpQueueSpace(Index) >= 1))
    {
        avrcpSendControlMessage(AVRCP_CTRL_FF_PRESS, Index);
        if (!isAvrcpPlaybackStatusSupported(Index))
        {
            /* use bitmask for fwd and rev so with AVRCP 1.0 devices can store previous play status before this operation */
            theHeadset.avrcp_link_data->play_status[Index] |= (1 << avrcp_play_status_fwd_seek);
            theHeadset.avrcp_link_data->play_status[Index] &= ~(1 << avrcp_play_status_rev_seek);
        }
    }
}


/*************************************************************************
NAME    
    headsetAvrcpFastForwardRelease
    
DESCRIPTION
    Send a AVRCP_FAST_FORWARD released to the device with the currently active AVRCP connection.

**************************************************************************/
void headsetAvrcpFastForwardRelease(void)
{
    uint16 Index = headsetAvrcpGetActiveConnection();
    
    AVRCP_DEBUG(("AVRCP: Avrcp FFWD Release\n"));
   
    if (theHeadset.avrcp_link_data->connected[Index] && (getAvrcpQueueSpace(Index) >= 1))
    {
        avrcpSendControlMessage(AVRCP_CTRL_FF_RELEASE, Index);
        if (!isAvrcpPlaybackStatusSupported(Index))
            theHeadset.avrcp_link_data->play_status[Index] &= ~(1 << avrcp_play_status_fwd_seek);
    }
}


/*************************************************************************
NAME    
    headsetAvrcpRewindPress
    
DESCRIPTION
    Send a AVRCP_REWIND pressed to the device with the currently active AVRCP connection.

**************************************************************************/
void headsetAvrcpRewindPress(void)
{
    uint16 Index = headsetAvrcpGetActiveConnection();
    
    AVRCP_DEBUG(("AVRCP: Avrcp REW Press\n"));
   
    if (theHeadset.avrcp_link_data->connected[Index] && (getAvrcpQueueSpace(Index) >= 1))
    {
        avrcpSendControlMessage(AVRCP_CTRL_REW_PRESS, Index);
        if (!isAvrcpPlaybackStatusSupported(Index))
        {
            theHeadset.avrcp_link_data->play_status[Index] |= (1 << avrcp_play_status_rev_seek);
            theHeadset.avrcp_link_data->play_status[Index] &= ~(1 << avrcp_play_status_fwd_seek);
        }
    }
}


/*************************************************************************
NAME    
    headsetAvrcpRewindRelease
    
DESCRIPTION
    Send a AVRCP_REWIND released to the device with the currently active AVRCP connection.

**************************************************************************/
void headsetAvrcpRewindRelease(void)
{
    uint16 Index = headsetAvrcpGetActiveConnection();
    
    AVRCP_DEBUG(("AVRCP: Avrcp REW Release\n"));
  
    if (theHeadset.avrcp_link_data->connected[Index] && (getAvrcpQueueSpace(Index) >= 1))
    {
        avrcpSendControlMessage(AVRCP_CTRL_REW_RELEASE, Index);
        if (!isAvrcpPlaybackStatusSupported(Index))
            theHeadset.avrcp_link_data->play_status[Index] &= ~(1 << avrcp_play_status_rev_seek);
    }
}


/*************************************************************************
NAME    
    headsetAvrcpVolumeStepChange
    
DESCRIPTION
    The volume has increased or decreased, so update the absolute volume (AVRCP 1.4) and notify 
    the remote device if this was a local change of volume, and it requested to be notified.

**************************************************************************/
void headsetAvrcpVolumeStepChange(volume_direction operation, uint16 step_change)
{
    uint8 i;
    
    AVRCP_DEBUG(("AVRCP: headsetAvrcpVolumeStepChange dir:[%d] step[%d]\n", operation, step_change));
       
    for_all_avrcp(i)
    {
        if (theHeadset.avrcp_link_data->connected[i])
        {            
            if (operation == increase_volume)
            {
                /* increase absolute volume by step change and check to see if max is reached */
                theHeadset.avrcp_link_data->absolute_volume[i] += (AVRCP_ABS_VOL_STEP_CHANGE * step_change);
                if (theHeadset.avrcp_link_data->absolute_volume[i] > AVRCP_MAX_ABS_VOL)
                    theHeadset.avrcp_link_data->absolute_volume[i] = AVRCP_MAX_ABS_VOL;
                AVRCP_DEBUG(("AVRCP: Avrcp Local Vol Increased [%d]\n", theHeadset.avrcp_link_data->absolute_volume[i]));
            }
            else if (operation == decrease_volume)
            {                
                /* decrease absolute volume by step change and check for minimum */
                if (theHeadset.avrcp_link_data->absolute_volume[i] >= ((AVRCP_ABS_VOL_STEP_CHANGE * step_change)))
                    theHeadset.avrcp_link_data->absolute_volume[i] -= (AVRCP_ABS_VOL_STEP_CHANGE * step_change);
                else
                    theHeadset.avrcp_link_data->absolute_volume[i] = 0;
                AVRCP_DEBUG(("AVRCP: Avrcp Local Vol Decreased [%d]\n", theHeadset.avrcp_link_data->absolute_volume[i]));
            }
                         
            if (theHeadset.avrcp_link_data->registered_events[i] & (1 << avrcp_event_volume_changed))
            {
                AVRCP_DEBUG(("  Notify remote device\n"));                
                AvrcpEventVolumeChangedResponse(theHeadset.avrcp_link_data->avrcp[i], 
                                        avctp_response_changed,
                                        theHeadset.avrcp_link_data->absolute_volume[i]);  
                /* reset registered volume notification */
                theHeadset.avrcp_link_data->registered_events[i] &= ~(1 << avrcp_event_volume_changed); 
            }
        }
    }
}

    
/*************************************************************************
NAME    
    headsetAvrcpSetLocalVolume
    
DESCRIPTION
    The volume has been locally set so update the absolute volume (AVRCP 1.4) and notify 
    the remote device if this was a local change of volume, and it requested to be notified.

**************************************************************************/
void headsetAvrcpSetLocalVolume(uint16 Index, uint16 a2dp_volume)    
{
    theHeadset.avrcp_link_data->absolute_volume[Index] = a2dp_volume * AVRCP_ABS_VOL_STEP_CHANGE;
        
    if (theHeadset.avrcp_link_data->registered_events[Index] & (1 << avrcp_event_volume_changed))
    {
        AVRCP_DEBUG(("  Notify remote device [local:%d][abs:%d]\n", a2dp_volume, theHeadset.avrcp_link_data->absolute_volume[Index]));        
        AvrcpEventVolumeChangedResponse(theHeadset.avrcp_link_data->avrcp[Index], 
                                        avctp_response_changed,
                                        theHeadset.avrcp_link_data->absolute_volume[Index]);  
        /* reset registered volume notification */
        theHeadset.avrcp_link_data->registered_events[Index] &= ~(1 << avrcp_event_volume_changed); 
    }
}


/*************************************************************************
NAME    
    headsetAvrcpGetIndexFromInstance
    
DESCRIPTION
    Retrieve the correct AVRCP connection index based on the AVRCP library instance pointer.
   
RETURNS
    Returns TRUE if the AVRCP connection was found, FASLE otherwise.
    The actual connection index is returned in the Index variable.

**************************************************************************/
bool headsetAvrcpGetIndexFromInstance(AVRCP *avrcp, uint16 *Index)
{
    uint8 i;
    
    /* go through Avrcp connections looking for device_id match */
    for_all_avrcp(i)
    {
        /* if the avrcp link is connected check for its AVRCP pointer */
        if (theHeadset.avrcp_link_data->connected[i])
        {
            /* if a device_id match is found return its value and a
               status of successfull match found */
            if (avrcp && (theHeadset.avrcp_link_data->avrcp[i] == avrcp))
            {
                *Index = i;
            	AVRCP_DEBUG(("AVRCP: getIndex = %d\n", i)); 
                return TRUE;
            }
        }
    }
    /* no matches found so return not successfull */    
    return FALSE;
}


/*************************************************************************
NAME    
    headsetAvrcpGetIndexFromBdaddr
    
DESCRIPTION
    Retrieve the correct AVRCP connection index based on the Bluetooth address.
   
RETURNS
    Returns TRUE if the AVRCP connection was found, FASLE otherwise.
    The actual connection index is returned in the Index variable.

**************************************************************************/
bool headsetAvrcpGetIndexFromBdaddr(const bdaddr *bd_addr, uint16 *Index)
{
    uint8 i;
    
    /* go through Avrcp connections looking for device_id match */
    for_all_avrcp(i)
    {
        /* if the avrcp link is connected check for its AVRCP pointer */
        if (theHeadset.avrcp_link_data->connected[i] && BdaddrIsSame(&theHeadset.avrcp_link_data->bd_addr[i], bd_addr))
        {
            *Index = i;
            AVRCP_DEBUG(("AVRCP: getIndex = %d\n", i)); 
            return TRUE;
        }
    }
    /* no matches found so return not successfull */    
    return FALSE;
}


/*************************************************************************
NAME    
    headsetAvrcpSetActiveConnection
    
DESCRIPTION
    Sets the active AVRCP connection based on Bluetooth address.

**************************************************************************/
void headsetAvrcpSetActiveConnection(const bdaddr *bd_addr)
{
    uint16 Index;
    
    if (headsetAvrcpGetIndexFromBdaddr(bd_addr, &Index))
    {
        theHeadset.avrcp_link_data->active_avrcp = Index;
    }
}


/*************************************************************************
NAME    
    headsetAvrcpGetActiveConnection
    
DESCRIPTION
    Gets the active AVRCP connection.

**************************************************************************/
uint16 headsetAvrcpGetActiveConnection(void)
{
    return theHeadset.avrcp_link_data->active_avrcp;
}


/*************************************************************************
NAME    
    headsetAvrcpUdpateActiveConnection
    
DESCRIPTION
    Updates the active AVRCP connection based on what is currently connected.

**************************************************************************/
void headsetAvrcpUdpateActiveConnection(void)
{
    uint8 i = 0;
    
    if (!theHeadset.avrcp_link_data->connected[headsetAvrcpGetActiveConnection()])
    {
        for_all_avrcp(i)
        {
            if (theHeadset.avrcp_link_data->connected[i])
            {
                theHeadset.avrcp_link_data->active_avrcp = i;
                break;
            }
        }
    }
}


/*************************************************************************
NAME    
    headsetAvrcpSetPlayStatus
    
DESCRIPTION
    Sets the play status but only for AVRCP 1.0 devices. For AVRCP 1.3 onwards, the remote
    device will notify of changes in play status.

**************************************************************************/
void headsetAvrcpSetPlayStatus(const bdaddr *bd_addr, uint16 play_status)
{
    uint16 Index;
    
    if (headsetAvrcpGetIndexFromBdaddr(bd_addr, &Index))
    {
        if (!isAvrcpPlaybackStatusSupported(Index))
        {
            AVRCP_DEBUG(("AVRCP: update play status %d\n", play_status));
            theHeadset.avrcp_link_data->play_status[Index] = play_status;
        }
    }
}


/*************************************************************************
NAME    
    headsetAvrcpCheckManualConnectReset
    
DESCRIPTION
    Checks if the manual AVRCP connect state needs resetting.

**************************************************************************/
bool headsetAvrcpCheckManualConnectReset(bdaddr *bd_addr)
{
    if (theHeadset.avrcp_link_data->avrcp_manual_connect)
    {
        if (BdaddrIsZero(&theHeadset.avrcp_link_data->avrcp_play_addr) || 
                ((bd_addr != NULL) && 
                 !BdaddrIsZero(&theHeadset.avrcp_link_data->avrcp_play_addr) && 
                 BdaddrIsSame(&theHeadset.avrcp_link_data->avrcp_play_addr, bd_addr))
        )
        {
            theHeadset.avrcp_link_data->avrcp_manual_connect = FALSE;
            BdaddrSetZero(&theHeadset.avrcp_link_data->avrcp_play_addr);
            return TRUE;
        }
    }
    return FALSE;
}


/*************************************************************************
NAME    
    headsetAvrcpHandleMessage
    
DESCRIPTION
    Handles AVRCP library messages.

**************************************************************************/
void headsetAvrcpHandleMessage(Task task, MessageId id, Message message)
{
    AVRCP_DEBUG(("AVRCP_MSG id=%x : \n", id));
    
    switch (id)
    {
        
/******************/
/* INITIALISATION */
/******************/
        
        case AVRCP_INIT_CFM:
            AVRCP_DEBUG(("AVRCP_INIT_CFM :\n"));
            headsetAvrcpInitComplete((AVRCP_INIT_CFM_T *) message);
            break;

/******************************/
/* CONNECTION / DISCONNECTION */
/******************************/            
            
        case AVRCP_CONNECT_CFM:
            AVRCP_DEBUG(("AVRCP_CONNECT_CFM :\n"));
            handleAvrcpConnectCfm((AVRCP_CONNECT_CFM_T *) message);
            break;
        case AVRCP_CONNECT_IND:
            AVRCP_DEBUG(("AVRCP_CONNECT_IND :\n"));
            handleAvrcpConnectInd((AVRCP_CONNECT_IND_T *) message);
            break;
        case AVRCP_DISCONNECT_IND:
            AVRCP_DEBUG(("AVRCP_DISCONNECT_IND :\n"));
            handleAvrcpDisconnectInd((AVRCP_DISCONNECT_IND_T *) message);
            break;
            
/*****************/
/* AV/C COMMANDS */
/*****************/            
        
        case AVRCP_PASSTHROUGH_CFM:
            AVRCP_DEBUG(("AVRCP_PASSTHROUGH_CFM :\n"));
    	    handleAvrcpPassthroughCfm((AVRCP_PASSTHROUGH_CFM_T *) message);
            break;    
        case AVRCP_PASSTHROUGH_IND:
            AVRCP_DEBUG(("AVRCP_PASSTHROUGH_IND :\n"));
    	    handleAvrcpPassthroughInd((AVRCP_PASSTHROUGH_IND_T *) message);
            break;
        case AVRCP_UNITINFO_IND:
            AVRCP_DEBUG(("AVRCP_UNITINFO_IND :\n"));
    	    handleAvrcpUnitInfoInd((AVRCP_UNITINFO_IND_T *) message);
            break;
        case AVRCP_SUBUNITINFO_IND:
            AVRCP_DEBUG(("AVRCP_SUBUNITINFO_IND :\n"));
    	    handleAvrcpSubUnitInfoInd((AVRCP_SUBUNITINFO_IND_T *) message);
            break;
        case AVRCP_VENDORDEPENDENT_IND:
            AVRCP_DEBUG(("AVRCP_VENDORDEPENDENT_IND :\n"));
    	    handleAvrcpVendorDependentInd((AVRCP_VENDORDEPENDENT_IND_T *) message);
            break;
   
/******************/
/* AVRCP Metadata */
/******************/            
            
        case AVRCP_REGISTER_NOTIFICATION_IND:
            AVRCP_DEBUG(("AVRCP_REGISTER_NOTIFICATION_IND :\n"));
            handleAvrcpRegisterNotificationInd((AVRCP_REGISTER_NOTIFICATION_IND_T *) message);
            break;
        case AVRCP_REGISTER_NOTIFICATION_CFM:
            AVRCP_DEBUG(("AVRCP_REGISTER_NOTIFICATION_CFM :\n"));            
            break;
        case AVRCP_SET_ABSOLUTE_VOLUME_IND:
            AVRCP_DEBUG(("AVRCP_SET_ABSOLUTE_VOLUME_IND :\n"));
            handleAvrcpSetAbsVolInd((AVRCP_SET_ABSOLUTE_VOLUME_IND_T *) message);
            break;
        case AVRCP_GET_PLAY_STATUS_CFM:
            AVRCP_DEBUG(("AVRCP_GET_PLAY_STATUS_CFM :\n"));
            handleAvrcpGetPlayStatusCfm((AVRCP_GET_PLAY_STATUS_CFM_T *) message);
            break;
        case AVRCP_EVENT_PLAYBACK_STATUS_CHANGED_IND:
            AVRCP_DEBUG(("AVRCP_EVENT_PLAYBACK_STATUS_CHANGED_IND :\n"));
            handleAvrcpPlayStatusChangedInd((AVRCP_EVENT_PLAYBACK_STATUS_CHANGED_IND_T *) message);
            break;

/********************/
/* LIBRARY SPECIFIC */
/********************/ 
            
        case AVRCP_GET_EXTENSIONS_CFM:
            AVRCP_DEBUG(("AVRCP_GET_EXTENSIONS_CFM :\n"));
            handleAvrcpGetExtensionsCfm((AVRCP_GET_EXTENSIONS_CFM_T *) message);
            break;
        case AVRCP_GET_SUPPORTED_FEATURES_CFM:
            AVRCP_DEBUG(("AVRCP_GET_SUPPORTED_FEATURES_CFM :\n"));
            handleAvrcpGetSupportedFeaturesCfm((AVRCP_GET_SUPPORTED_FEATURES_CFM_T *) message);
            break;
            
/*************/
/* UNHANDLED */
/*************/  
    
        default:
            break;
    }
}


#else /* ENABLE_AVRCP*/
static const int avrcp_disabled;
#endif /* ENABLE_AVRCP*/
