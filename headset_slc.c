/****************************************************************************

FILE NAME
    headset_slc.c

DESCRIPTION
    manages the conenction / reconnection to the last / default users

NOTES

*/

/****************************************************************************
    Header files
*/
#include "headset_private.h"
#include "headset_scan.h"
#include "headset_slc.h"

#include "headset_statemanager.h"
#include "headset_configmanager.h"
#include "headset_callmanager.h"
#include "headset_volume.h"
#include "headset_led_manager.h"
#include "headset_a2dp.h"

#ifdef ENABLE_PBAP
#include "headset_pbap.h"
#endif

#include <connection.h>
#include <hfp.h>
#include <panic.h>
#include <ps.h>
#include <bdaddr.h>
#include <stddef.h>
#include <sink.h>
#include <string.h> /* for memset */
#include <a2dp.h>


#ifdef DEBUG_SLC
    #define SLC_DEBUG(x) DEBUG(x)
    #ifdef DEBUG_PRINT_ENABLED
        static const char * const gDebugReconStrings[8] = { "AR_LastConnected",
                                                            "AR_List"    
                                                          };
    #endif
#else
    #define SLC_DEBUG(x) 
#endif   

#ifdef DEBUG_MY_SLC
    #define MY_SLC_DEBUG(x) DEBUG(x)
#else
    #define MY_SLC_DEBUG(x) 
#endif   
        
        
typedef enum NextPagingTag 
{
    PageUnknown   ,
    PageLastAG    , 
    PageFromList  ,
    PageComplete
}NextPage_t;


typedef struct slcDataTag
{
    unsigned gCallTransferInProgress:1 ;
    unsigned gSlcConnectRemote:1;
    unsigned gAttemptedListID:8 ;    
    unsigned gListID:4 ;
    unsigned gPdlSize:4 ;
}slcData_t;

static slcData_t gSlcData = {FALSE, FALSE, 0, 0, 0} ;

extern const lp_power_table hfp_default_powertable[2];
extern const lp_power_table hfp_default_powertable_sco[2];
extern const lp_power_table a2dp_stream_powertable[1];

#define NUM_INQ_DEVS 3
#define NOT_IN_DEVS 0xFF

/* Set or unset inquiry results block, no point making a function as one line */
#define blockInquiryResults(block) \
    theHeadset.block_inq_res = block;

#define slcInquiryGetDiffThreshold() \
    ((theHeadset.rssi_action == rssi_connecting) ? (int)(theHeadset.conf->rssi.conn_diff_threshold) : (int)(theHeadset.conf->rssi.diff_threshold))

#define slcInquiryGetThreshold() \
    ((theHeadset.rssi_action == rssi_connecting) ? theHeadset.conf->rssi.conn_threshold : theHeadset.conf->rssi.threshold)

#ifdef Rubidium
extern char	TTS_text[50];
#endif

/****************************************************************************
NAME    
    clearInquiryData
    
DESCRIPTION
    Reset inquiry data to initial values
RETURNS
    void
*/
static void clearInquiryData( void )
{
    if(theHeadset.inquiry_data)
    {
        uint8 i;
        for(i=0; i<NUM_INQ_DEVS; i++)
        {
            /* Reset address & peak RSSI value */
            BdaddrSetZero(&theHeadset.inquiry_data[i].bd_addr);
            theHeadset.inquiry_data[i].rssi = slcInquiryGetThreshold();
        }
    }
}

/****************************************************************************
NAME    
    slcStartInquiry
    
DESCRIPTION
    Kick off inquiry
RETURNS
    void
*/
void slcStartInquiry( void )
{
    if(!theHeadset.inquiry_data)
    {
        SLC_DEBUG(("SLC: Start Inquiry\n" )) ;
        
        /* Go discoverable (and disconnect any active SLC) */
        if(theHeadset.rssi_action == rssi_pairing)
            stateManagerEnterConnDiscoverableState() ;
        slcReset();
        
        /* Allocate space to store inquiry results */
        theHeadset.inquiry_data = mallocPanic(NUM_INQ_DEVS * sizeof(inquiry_data_t));

        clearInquiryData();

        blockInquiryResults(FALSE);

        /* Increase page timeout */
        ConnectionSetPageTimeout(16384);

        /* Start a periodic inquiry, this will keep going until we cancel (either on
        successful connection or EventRssiPairTimeout) */
        ConnectionInquirePeriodic(&theHeadset.task, 8, 16, 0x9E8B33, 16, 5, theHeadset.conf->rssi.cod_filter);

        /* Send a reminder event */
        MessageSendLater(&theHeadset.task, EventRssiPairReminder, 0, D_SEC(INQUIRY_REMINDER_TIMEOUT_SECS));

        /* Send timeout if enabled */
        if(theHeadset.conf->timeouts.InquiryTimeout_s)
            MessageSendLater(&theHeadset.task, EventRssiPairTimeout, 0, D_SEC(theHeadset.conf->timeouts.InquiryTimeout_s));
    }
}

/****************************************************************************
NAME    
    slcStopInquiry
    
DESCRIPTION
    If inquiry in progress throw away the results and cancel further CL messages
RETURNS
    void
*/
void slcStopInquiry( void )
{
    SLC_DEBUG(("SLC: Inquiry Tidy Up\n" )) ;
    /* Free space used for inquiry results */
    if(theHeadset.inquiry_data)
    {
        blockInquiryResults(TRUE);

        SLC_DEBUG(("SLC: Free Inquiry Data\n" )) ;
        free(theHeadset.inquiry_data);
        theHeadset.inquiry_data = NULL;

        /* Restore Page Timeout */
        ConnectionSetPageTimeout(0);

        /* Cancel the inquiry */
        SLC_DEBUG(("SLC: Cancel Inquiry\n" )) ;
        ConnectionInquireCancel(&theHeadset.task);
        MessageCancelAll(&theHeadset.task, CL_DM_INQUIRE_RESULT);
        MessageCancelFirst(&theHeadset.task, EventRssiPairReminder);
        if(theHeadset.conf->timeouts.InquiryTimeout_s)
            MessageCancelFirst(&theHeadset.task, EventRssiPairTimeout);
        
        MessageCancelAll(&theHeadset.task, EventContinueSlcConnectRequest);
        MessageSend(&theHeadset.task, EventContinueSlcConnectRequest, 0);
        theHeadset.rssi_action = rssi_none;
    }
}

/****************************************************************************
NAME    
    slcInquiryCheckAddr
    
DESCRIPTION
    Helper function to check if an address matches dev 0 or 1
RETURNS
    idx if addr is matched, otherwise 0xFF (NOT_IN_DEVS)
*/
static bool slcInquiryCheckAddr(bdaddr *bd_addr)
{
    uint8 idx;
    /* Don't bother checking last dev as we don't stash the addr anyway */
    for(idx=0; idx < (NUM_INQ_DEVS-1); idx++)
        if(BdaddrIsSame(bd_addr, &theHeadset.inquiry_data[idx].bd_addr))
            return idx;
    return NOT_IN_DEVS;
}

/****************************************************************************
NAME    
    slcInquiryConnectIndex
    
DESCRIPTION
    Helper function to connect to inquiry result at position idx
RETURNS
    void
*/
static void slcInquiryConnectIndex(uint8 idx)
{
    /* Check idx is valid (0 or 1) */
    if(theHeadset.inquiry_data)
    {
        if(idx < (NUM_INQ_DEVS - 1))
        {
            bdaddr conn_addr = theHeadset.inquiry_data[idx].bd_addr;
            int16  conn_rssi = theHeadset.inquiry_data[idx].rssi;
            
            /* Check there's a valid result at position idx */
            if( (!BdaddrIsZero(&conn_addr)))
            {
                /* Don't check if in PDL unless connecting or feature bit enabled */
                if( (theHeadset.rssi_action == rssi_pairing) || (!theHeadset.features.RssiConnCheckPdl) || 
                    ConnectionSmGetAttributeNow(ATTRIBUTE_PSKEY_BASE, &conn_addr, 0, NULL) )
                {
                    /* Check that we don't already have max connections permitted */
                    if( ( (theHeadset.MultipointEnable) && (theHeadset.conf->no_of_profiles_connected < MAX_MULTIPOINT_CONNECTIONS) ) ||
                        ( (!theHeadset.MultipointEnable) && (!theHeadset.conf->no_of_profiles_connected) ) )
                    {
                        /* Allow 2 close devices if multipoint enabled, otherwise just one */
                        uint8 cmp_idx   = theHeadset.MultipointEnable ? 2 : 1;
                        int16 cmp_rssi = theHeadset.inquiry_data[cmp_idx].rssi;
                        /* Check that difference threshold criteria are met */
                        if((conn_rssi - cmp_rssi) >= slcInquiryGetDiffThreshold())
                        {
                            /* Ensure the link key for this device is deleted before the connection attempt,
                            prevents reconnection problems with BT2.1 devices.*/
                            if(theHeadset.rssi_action == rssi_pairing)
                                ConnectionSmDeleteAuthDevice(&conn_addr);
                        
                            /* Block handling any more results until we're done with this connect attempt */
                            blockInquiryResults(TRUE);
                
                            /* reset connection via remote ag instead of headset flag */
                            gSlcData.gSlcConnectRemote = FALSE;
              
                            /* Issue a connect request for HFP */
                            HfpSlcConnectRequest(&conn_addr, hfp_handsfree_and_headset, hfp_handsfree_all);
                            
                            theHeadset.rssi_attempting = idx;
                            SLC_DEBUG(("SLC: Inquire Connect Addr %x,%x,%lx RSSI: %d\n", conn_addr.nap,
                                                                                         conn_addr.uap,
                                                                                         conn_addr.lap, 
                                                                                         conn_rssi )) ;
                            return;
                        }
                    }
                }
            }
        }
        /* Connect attempt failed - Are we connecting or pairing? */
        if(theHeadset.rssi_action == rssi_connecting || (theHeadset.rssi_action == rssi_pairing && idx > 0))
        {
            /* Stop the inquiry and fall back on reconnect action */
            slcStopInquiry();
        }
        else
        {
            /* Unblock results and keep inquiring */
            clearInquiryData();
            blockInquiryResults(FALSE);
        }
    }
}

/****************************************************************************
NAME    
    slcHandleInquiryResult
    
DESCRIPTION
    Inquiry result received
RETURNS
    void
*/
void slcHandleInquiryResult( CL_DM_INQUIRE_RESULT_T* result )
{
    /* Check inquiry data is valid (if not we must have cancelled) */
    if(theHeadset.inquiry_data && !theHeadset.block_inq_res)
    {
#ifdef DEBUG_SLC
        uint8 debug_idx;
        SLC_DEBUG(("SLC: Inquiry Result %x Addr %x,%x,%lx RSSI: %d\n", result->status,
                                                                       result->bd_addr.nap,
                                                                       result->bd_addr.uap,
                                                                       result->bd_addr.lap, 
                                                                       result->rssi )) ;

        for(debug_idx=0; debug_idx<NUM_INQ_DEVS; debug_idx++)
            SLC_DEBUG(("SLC: [Addr %x,%x,%lx RSSI: %d]\n", theHeadset.inquiry_data[debug_idx].bd_addr.nap,
                                                           theHeadset.inquiry_data[debug_idx].bd_addr.uap,
                                                           theHeadset.inquiry_data[debug_idx].bd_addr.lap, 
                                                           theHeadset.inquiry_data[debug_idx].rssi )) ;
#endif
        if(result->status == inquiry_status_result)
        {
            /* Cache result if RSSI is higher than entry 0 or 1 */
            if (result->rssi > theHeadset.inquiry_data[0].rssi || result->rssi > theHeadset.inquiry_data[1].rssi)
            {
                /* Check if the discovered device is already in idx 0 or 1 */
                uint8 idx = slcInquiryCheckAddr(&result->bd_addr);
                if(idx != NOT_IN_DEVS)
                {
                    /* Device is in the list, update the RSSI */
                    if(result->rssi > theHeadset.inquiry_data[idx].rssi)
                        theHeadset.inquiry_data[idx].rssi = result->rssi;
                }
                else
                {
                    /* Device not in devs, overwrite whichever of 0 or 1 has lowest RSSI */
                    idx = (theHeadset.inquiry_data[1].rssi >= theHeadset.inquiry_data[0].rssi) ? 0 : 1;
                    
                    /* Before overwriting update the baseline RSSI */
                    theHeadset.inquiry_data[2].rssi = theHeadset.inquiry_data[idx].rssi;

                    /* Overwrite */
                    theHeadset.inquiry_data[idx].bd_addr = result->bd_addr;
                    theHeadset.inquiry_data[idx].rssi = result->rssi;
                }
                /* Re order 0 and 1 if necessary */
                if(theHeadset.inquiry_data[1].rssi > theHeadset.inquiry_data[0].rssi)
                {
                    bdaddr temp_addr = theHeadset.inquiry_data[0].bd_addr;
                    int16  temp_rssi = theHeadset.inquiry_data[0].rssi;
                    theHeadset.inquiry_data[0].bd_addr = theHeadset.inquiry_data[1].bd_addr;
                    theHeadset.inquiry_data[0].rssi    = theHeadset.inquiry_data[1].rssi;
                    theHeadset.inquiry_data[1].bd_addr = temp_addr;
                    theHeadset.inquiry_data[1].rssi    = temp_rssi;
                }
            }
            else if (result->rssi > theHeadset.inquiry_data[2].rssi)
            {
                /* Check if this is device 0 or 1 */
                if (slcInquiryCheckAddr(&result->bd_addr) == NOT_IN_DEVS)
                {
                    /* Store baseline RSSI if not */
                    theHeadset.inquiry_data[2].rssi = result->rssi;
                }
            }
        }
        else
        {
            SLC_DEBUG(("SLC: Inquiry Complete\n"));
            /* Attempt to connect to device */
            slcInquiryConnectIndex(0);
        }
    }
}

/****************************************************************************
NAME    
    slcConnectFail
    
DESCRIPTION
    SLC failed to connect
RETURNS
    void
*/
void slcConnectFail( hfp_link_priority priority )
{
    
    SLC_DEBUG(("SLC: ConnFail\n" )) ;
        
    if(theHeadset.inquiry_data)
    {
        /* Make sure we dont ever leave block flag set by mistake */
        blockInquiryResults(FALSE);
        return;
    }

	#ifdef Rubidium
		if(theHeadset.MultipointEnable == 0)
			MessageSend(&theHeadset.task, EventReconnectFailed, 0);
	#else
    /* Send event to signify that all reconnection attempts failed */
    MessageSend(&theHeadset.task, EventReconnectFailed, 0);
	#endif
        
    /*clear the queue*/
    headsetClearQueueudEvent() ;
        
    /* continue the connection sequence, this may or may not make any further
       connection attempts depending upon multipoint configuration */
    if(!gSlcData.gSlcConnectRemote && theHeadset.rssi_action != rssi_pairing)
    {
        /* continue trying to connect to next device in 2 seconds time to
           allow paging between connection attempts */
        MessageSendLater(&theHeadset.task,EventContinueSlcConnectRequest,0,2000);
    }
        
    /* if set to repeat a connection attempt decrement this as an attempt has occured */
    if(theHeadset.conf->NoOfReconnectionAttempts)
        theHeadset.conf->NoOfReconnectionAttempts--;
    
    /* reset AG instigated connection flag */    
    gSlcData.gSlcConnectRemote = FALSE;

}


/****************************************************************************
NAME    
    headsetHandleSlcConnectInd
    
DESCRIPTION
    Handle a request to establish an SLC from the AG.

RETURNS
    void
*/
void headsetHandleSlcConnectInd( const HFP_SLC_CONNECT_IND_T *ind )
{
    if(ind->accepted)
    {
        /* Cancel inquiry and throw away results if one is in progress */
        slcStopInquiry();

#ifdef MP_Config2
		if(theHeadset.MultipointEnable)
		{
			SLC_DEBUG(("SLC: BHC612_PairSuccess = %d\n", theHeadset.BHC612_PairSuccess));

			if(theHeadset.BHC612_PairSuccess == 1)
				gSlcData.gSlcConnectRemote = FALSE;
			else
				gSlcData.gSlcConnectRemote = TRUE;
		}
#else
		/* set flag indicating a connection is in progress */
		gSlcData.gSlcConnectRemote = TRUE;
#endif
    }
}

/****************************************************************************
NAME    
    headsetHandleSlcDisconnectInd
    
DESCRIPTION
    Indication that the SLC has been released.

RETURNS
    void
*/
void headsetHandleSlcDisconnectInd( const HFP_SLC_DISCONNECT_IND_T *ind )
{	    
    SLC_DEBUG(("SLC: slc DiscInd for index %d, status = %d\n",ind->priority, ind->status)) ;     
        
    /* another connection dropped, update number of current connections */
    theHeadset.conf->no_of_profiles_connected = GetNumberOfConnectedDevices();
	SLC_DEBUG(("no_of_profiles_connected = %d\n",theHeadset.conf->no_of_profiles_connected));
    
    if(ind->status == hfp_disconnect_success || ind->status == hfp_disconnect_link_loss)
    {
        /* store volume info */
        StoreProfilePsKeyInfo(headset_hfp, ind->priority, 0);   
	
        /*if the headset is off then this is disconnect as part of the power off cycle - dont re-enable connectable*/	
    	if ( stateManagerGetState() != headsetLimbo)
        {
        	/* for multipoint operation need to re-enable connectable mode when one device is dropped */
    	    if(theHeadset.MultipointEnable)
    	    {
    	        SLC_DEBUG(("SLC: Go Conn \n" ));
    	            
    	        /* at least one device disconnected, re-enable connectable for another 60 seconds */
    	        headsetEnableConnectable();                    
    	        
    	        /* remain connectable for a further 'x' seconds to allow a second 
    	           AG to be connected if non-zero, otherwise stay connecatable forever */
    	        if(theHeadset.conf->timeouts.ConnectableTimeout_s)
     	            MessageSendLater(&theHeadset.task, EventConnectableTimeout, 0, D_SEC(theHeadset.conf->timeouts.ConnectableTimeout_s));
    	    }
        }
    
        /*a disconnect in active call state is a call transfer*/
        if ( (stateManagerGetState() == headsetActiveCallSCO) || 
             (stateManagerGetState() == headsetActiveCallNoSCO) )
        {
		    gSlcData.gCallTransferInProgress = TRUE ;           
        }
        else
        {
		    gSlcData.gCallTransferInProgress = FALSE ;	
        }
    
        /* if not a link loss reset the last outgoing AG as AG1 will no longer exist now */        
        theHeadset.last_outgoing_ag = hfp_primary_link;

        /* reset the list id of the device just dropped */              
        theHeadset.profile_data[PROFILE_INDEX(ind->priority)].status.list_id = INVALID_LIST_ID;
        
        /* If primary disconnected */
        if(ind->priority == hfp_primary_link)
        {
            /* ...and we have a secondary link it will be promoted to primary */
            if(theHeadset.profile_data[PROFILE_INDEX(hfp_secondary_link)].status.list_id != INVALID_LIST_ID)
            {
                /* Block copy secondary data to primary location */
                theHeadset.profile_data[PROFILE_INDEX(hfp_primary_link)] = theHeadset.profile_data[PROFILE_INDEX(hfp_secondary_link)];
                /* Secondary link no longer exists, set it to invalid */
                theHeadset.profile_data[PROFILE_INDEX(hfp_secondary_link)].status.list_id = INVALID_LIST_ID;

		#ifdef BHC612
			#ifdef MP_Config1_
				if(theHeadset.MultipointEnable)
					ConnectionSmUpdateMruDevice( &theHeadset.BHC612_SecondaryBTA) ;
			#endif
		#endif
            }
        }

	#ifdef BHC612
		if(ind->priority == hfp_secondary_link && theHeadset.MultipointEnable == 1)
        	{
			SLC_DEBUG(("***hfp_secondary_link disconnect!***\n"));
		}

		/*Print profile_data*/
		#if 0
		MY_SLC_DEBUG(("hfp_primary_link,list id = %x\n",theHeadset.profile_data[PROFILE_INDEX(hfp_primary_link)].status.list_id));
		MY_SLC_DEBUG(("hfp_secondary_link,list id = %x\n",theHeadset.profile_data[PROFILE_INDEX(hfp_secondary_link)].status.list_id));
		#endif
	#endif
    }
    

    /*if the headset is off then this is disconnect as part of the power off cycle*/	
	if ( stateManagerGetState() != headsetLimbo)
    {
	        /* Update the app state if we are on and both */
	    if ((stateManagerIsConnected()) && (!theHeadset.conf->no_of_profiles_connected))
	    {
	        stateManagerEnterConnectableState( FALSE ) ;
	    }
    }
    
/*#ifdef Rubidium*/
#if 0
	if(theHeadset.MultipointEnable == 1)
	{
		if(ind->priority == hfp_primary_link)
		{
			theHeadset.BHC612_PhoneChanged = 1;
			
			/*MessageSendLater (&theHeadset.task , EventSLCDisconnected , 0, 3000 ) ;*/
			MessageSend(&theHeadset.task , EventSLCDisconnected , 0) ;
			
			SLC_DEBUG(("State = %d,no_of_profiles_connected = %d\n",stateManagerGetState(),theHeadset.conf->no_of_profiles_connected ));
		}
		else
		{
			theHeadset.BHC612_PhoneChanged = 0;
			SLC_DEBUG(("Disconnect : voice prompt 1\n"));
		}
	}
	else
		MessageSend(&theHeadset.task , EventSLCDisconnected , 0) ;
#else
    MessageSend(&theHeadset.task , EventSLCDisconnected , 0) ;
#endif
}


/****************************************************************************
NAME    
    headsetHandleSlcConnectCfm
    
DESCRIPTION
    Confirmation that the SLC has been established (or not).

RETURNS
    void
*/
bool headsetHandleSlcConnectCfm( const HFP_SLC_CONNECT_CFM_T *cfm )
{
    bool lResult = FALSE ;
    bdaddr ag_addr;
    /* volume info for device being connected to */
    uint8 psdata[ATTRIBUTE_SIZE];
   
    /* Resume handling inquiry results (results will be blocked again if we attempt to 
       reconnect hsp) */
    blockInquiryResults(FALSE);
    
    /* Check the status of the SLC attempt */
    if (cfm->status == hfp_connect_success)
    {   
        SLC_DEBUG(("SLC: ConnCfm - Success, ListID = %d \n",gSlcData.gAttemptedListID)) ;
              
        /* store the position in the PDL of this attempted connection, used for link loss and
           preventing connecting to the same ListID in future connection attempts, having just connected this
           will always be the primary, ID=0 but subject to change if multipoint is enabled */
        theHeadset.profile_data[PROFILE_INDEX(cfm->priority)].status.list_id = PROFILE_INDEX(hfp_primary_link);            
                
        SLC_DEBUG(("RSSI attempting %d\n", theHeadset.rssi_attempting))
        if(theHeadset.rssi_action != rssi_none)
        {
            /* Block inquiry results, unblocked during continuation */
            blockInquiryResults(TRUE);
        }
        
        /* set connection type and state */
        theHeadset.profile_data[PROFILE_INDEX(cfm->priority)].status.profile_type = cfm->profile;            
        slcProfileConnected(cfm->priority, cfm->sink);       
        
        lResult = TRUE ;  
        
	    /* have now connected a device, reset flag that allows a different LED state following initial
           power up before a first connection is made */
        theHeadset.powerup_no_connection = FALSE;
        
        /* send message to do indicate a stop of the paging process when in connectable state */
        if(theHeadset.paging_in_progress)
        {
            MessageSend(&theHeadset.task, EventStopPagingInConnState ,0);  
        }
        
        /* if headset initiated connection */
        if(!gSlcData.gSlcConnectRemote)
        {
#ifdef MP_Config2
			if(theHeadset.MultipointEnable)
			{
				gSlcData.gPdlSize = stateManagerGetPdlSize();

				MessageSendLater(&theHeadset.task,EventContinueSlcConnectRequest,0,2000);
			}
#else
			SLC_DEBUG(("SLC : Connect next device\n"));
			
	       	/* If no event is queued send button press */
	       	if(!theHeadset.conf->gEventQueuedOnConnection)
	        	HfpHsButtonPressRequest(cfm->priority);
				
           	/* Continue trying to connect to next device in 0.1 seconds time to allow current device to finish connecting */
           	MessageSendLater(&theHeadset.task,EventContinueSlcConnectRequest,0,theHeadset.conf->timeouts.SecondAGConnectDelayTime_s);
#endif
        }
        
        /* set volume of connecting AG, obtain address of AG to get volume data from ps */    
        if(HfpLinkGetBdaddr(cfm->priority, &ag_addr))
        {
            /* get psdata for this connection if available */
            if(ConnectionSmGetAttributeNow(ATTRIBUTE_PSKEY_BASE, &ag_addr, ATTRIBUTE_SIZE, psdata))
            {
                /* ps read ok, use this volume level */
                theHeadset.profile_data[PROFILE_INDEX(cfm->priority)].audio.gSMVolumeLevel = psdata[attribute_hfp_volume];
            }       
            else
            {
                /* in case it is not possible to get volume level from ps */
                theHeadset.profile_data[PROFILE_INDEX(cfm->priority)].audio.gSMVolumeLevel = theHeadset.features.DefaultVolume;
            }
        }

        /* update AG volume level using stored level read from headset ps */
		#if 1
		VolumeSendAndSetHeadsetVolume ( theHeadset.profile_data[PROFILE_INDEX(cfm->priority)].audio.gSMVolumeLevel ,TRUE , cfm->priority, 0) ;
		#else
        VolumeSendAndSetHeadsetVolume ( theHeadset.profile_data[PROFILE_INDEX(cfm->priority)].audio.gSMVolumeLevel ,TRUE , cfm->priority) ;
		#endif

#if 0
		/*Print profile_data*/
		MY_SLC_DEBUG(("list id = %x\n",theHeadset.profile_data[PROFILE_INDEX(hfp_primary_link)].status.list_id));
		MY_SLC_DEBUG(("list id = %x\n",theHeadset.profile_data[PROFILE_INDEX(hfp_secondary_link)].status.list_id));
#endif
		
        /* auto answer call if ringing */
        if ((theHeadset.features.AutoAnswerOnConnect)&&(HfpLinkPriorityFromCallState(hfp_call_state_incoming)))
        {      
             /*then attempt to answer the call*/
             MessageSend (&theHeadset.task , EventAnswer , 0 ) ;
             SLC_DEBUG(("SLC: AutoAnswer triggered\n")) ;
             return TRUE;
        }    
		
#ifdef ENABLE_PBAP 
        /* Connect the PBAP link of this device */
        pbapConnect(ag_addr);
#endif			

    }
    /* failed to connect due to rejection by ag, try hsp instead of hfp */
    else if (cfm->status == hfp_connect_sdp_fail)
    {
        SLC_DEBUG(("SLC: ConnCfm - Failed\n")) ;
        
        /*only continue if not already connected or multipoint enabled in which continue to connect second AG */
        if ((!stateManagerIsConnected())||(theHeadset.MultipointEnable))
        {				
            /* We failed to find the hfp profile */
            SLC_DEBUG(("SLC: CFM HSP Fail\n")) ;
            
            /* We try the HSP after we've tried HFP so this AG supports neither, give up */
            slcConnectFail(cfm->priority);       
        }    
        clearInquiryData();
    }
    /* all other status values including timeout */
    else
    {
        SLC_DEBUG(("SLC: ConnCfm - Timeout\n")) ;
        
        /* a connection timeout will arrive here, need to report fail for multipoint
           connections also such that a link loss retry will be performed */
        if ( !stateManagerIsConnected() || theHeadset.MultipointEnable)  /*only continue if not already connected*/
        {   
            /* Update local state to reflect this */    
            slcConnectFail(cfm->priority);       
        }
        clearInquiryData();
    }
 
    /* if using multipoint and both devices are connected disable connectable */
    if((theHeadset.MultipointEnable) && (theHeadset.conf->no_of_profiles_connected == MAX_MULTIPOINT_CONNECTIONS))
    {
        /* cancel any outstanding timer messages to cancel connectable */
        MessageCancelAll(&theHeadset.task, EventConnectableTimeout);
        /* at least one device disconnected, renable connectable for another 60 seconds */
        headsetDisableConnectable();  
        
        SLC_DEBUG(("SLC: disable Conn \n" ));
    }
    
    /* reset connection via remote ag instead of headset flag */
    gSlcData.gSlcConnectRemote = FALSE;

    return lResult ;
}

/****************************************************************************
NAME    
    slcProfileConnected
    
DESCRIPTION
    Indicate that a profile has connected 

RETURNS
    void
*/
void slcProfileConnected( hfp_link_priority priority, Sink sink )
{
	bdaddr ag_addr;
    
	SLC_DEBUG(("SLC: Pro Connected[%x]\n", (int)sink)) ;
#ifdef BHC612	
	SLC_DEBUG(("link_priority = %x\n",priority));
#endif
	
    /* another connection made, update number of current connections */
    theHeadset.conf->no_of_profiles_connected = GetNumberOfConnectedDevices();

 	SLC_DEBUG(("SLC: Pro Connected[%x], NoOfDev=%x\n", (int)sink,theHeadset.conf->no_of_profiles_connected)) ;

	/* Enter connected state if applicable*/
    if ( !stateManagerIsConnected() )
    {
        stateManagerEnterConnectedState();
	}
     
    if ( theHeadset.features.EncryptOnSLCEstablishment )
    {       /*ensure the underlying ACL is encrypted*/       
        ConnectionSmEncrypt( &theHeadset.task , sink , TRUE );
    }
    
	/* Store the address as the last used AG which can be obtained from the sink */
	if (SinkGetBdAddr(sink, &ag_addr))
    {
		gSlcData.gCallTransferInProgress = FALSE ;

		#ifdef BHC612
			/*
			SLC_DEBUG(("Get Primary BT Addr: nap %04x uap %02x lap %08lx\n",ag_addr.nap,ag_addr.uap,ag_addr.lap));
			*/
			if(priority == hfp_primary_link)
			{
				theHeadset.BHC612_PrimaryBTA = ag_addr;
				#if 0
				MY_SLC_DEBUG(("Get Primary BT Addr: nap %04x uap %02x lap %08lx\n",ag_addr.nap,ag_addr.uap,ag_addr.lap));
				#endif
			}

			if(priority == hfp_secondary_link)
			{
				theHeadset.BHC612_SecondaryBTA = ag_addr;
				#if 0
				MY_SLC_DEBUG(("Get Secondary BT Addr: nap %04x uap %02x lap %08lx\n",ag_addr.nap,ag_addr.uap,ag_addr.lap));
				#endif
			}
			
		#endif
		
        /* always shuffle the pdl into mru order */        
  		SLC_DEBUG(("SLC:shuf-conn\n")) ;
        ConnectionSmUpdateMruDevice( &ag_addr ) ;
 
        /* if using multi point check if this is the indication of the secondary device
           connecting, in which case shuffle list again with the AG_1 bdaddr to retain
           correct connection order of AG1 & AG2 */
        if((theHeadset.MultipointEnable && theHeadset.conf->no_of_profiles_connected > 1)&&
           (priority == hfp_secondary_link))
        {
            Sink sink;
            /* shuffle list again based on AG 1 bdaddr to have index 0 as AG1 and
               index 1 as AG2 */
            SLC_DEBUG(("SLC:shuf-conn again\n")) ;
            
            /* get bdaddr for the first of the two connections to make this first
               entry in the pdl list again */
            if((HfpLinkGetSlcSink(hfp_primary_link, &sink))&&(SinkGetBdAddr(sink, &ag_addr)))
            {
            	#ifdef BHC612
			    /*SLC_DEBUG(("Get Secondary BT Addr: nap %04x uap %02x lap %08lx\n",ag_addr.nap,ag_addr.uap,ag_addr.lap));*/			
		        #endif
                ConnectionSmUpdateMruDevice( &ag_addr ) ;
            }
            
            /* the position of the current connected device has moved again within the PDL index, it is
               now in position 1, the second in list */      
            theHeadset.profile_data[PROFILE_INDEX(priority)].status.list_id = (hfp_secondary_link - 1);          

		#ifdef BHC612
			#if 0
			/*Print profile_data*/
			MY_SLC_DEBUG(("list id = %x\n",theHeadset.profile_data[PROFILE_INDEX(hfp_primary_link)].status.list_id));
			MY_SLC_DEBUG(("list id = %x\n",theHeadset.profile_data[PROFILE_INDEX(hfp_secondary_link)].status.list_id));
			#endif
		#endif
        }
        else
        {
            /* the position of the current connected device has moved within the PDL index, it is
               now in position 0, first in list */
            theHeadset.profile_data[PROFILE_INDEX(priority)].status.list_id = (hfp_primary_link - 1);        

	#if 0
		/*Print profile_data*/
		MY_SLC_DEBUG(("list id = %x\n",theHeadset.profile_data[PROFILE_INDEX(hfp_primary_link)].status.list_id));
		MY_SLC_DEBUG(("list id = %x\n",theHeadset.profile_data[PROFILE_INDEX(hfp_secondary_link)].status.list_id));
	#endif
        }
    }   

	#ifdef CTIA_Test
	theHeadset.features.UseDiffConnectedEventAtPowerOn = 1;
	#endif

    /* if the feature bit to generate a different connected event immediately following a power on
       to indicate a different led pattern for the first SLC connect attempt is used then generate
       a different connected event if this is the first connection attempt following power on */
    if((theHeadset.features.UseDiffConnectedEventAtPowerOn)&&(theHeadset.powerup_no_connection))
    {
        /* Send a user event to the app for indication purposes*/
		#ifdef CTIA_Test
		MessageSendLater(&theHeadset.task , EventSLCConnectedAfterPowerOn , NULL , 1500);
		#else
        	MessageSend (&theHeadset.task , EventSLCConnectedAfterPowerOn , NULL );
		#endif
    }
    else
    {
        /* Send a user event to the app for indication purposes*/
		#if 0
		MessageSendLater(&theHeadset.task , EventSLCConnected , NULL , 1500 );
		#else
        MessageSend (&theHeadset.task , EventSLCConnected , NULL );
		#endif
    }
	   
    /*re-set the network indicator on connection*/
    theHeadset.NetworkIsPresent = TRUE ;
            
	/* Read the remote supported features of the AG */
	ConnectionReadRemoteSuppFeatures(&theHeadset.task, sink);	
	
    /* attempt to pull the audio across if not already present, delay by 5 seconds
       to prevent a race condition occuring with certain phones */
    MessageSendLater ( &theHeadset.task , EventCheckForAudioTransfer , 0 , 5000 ) ;
   
    /* set timeout to 5 seconds */
    ConnectionSetLinkSupervisionTimeout(sink, 0x1f80);
    
    /* Send our link policy settings */
	slcSetLinkPolicy(priority, sink);
    
    /* Send a delayed message to request a role indication and make necessary changes as appropriate */
    MessageSendLater (&theHeadset.task , EventCheckRole , 0 , theHeadset.conf->timeouts.CheckRoleDelayTime_s );

    /* enable the call waiting indications from the AG, will be used for multipoint call
       waiting indications when AG is not currently routing its sco to hs */
    HfpCallWaitingEnableRequest(priority, TRUE);

#ifdef Rubidium
	/* enable caller ID indications from the AG */
    HfpCallerIdEnableRequest(priority, TRUE);
	MY_SLC_DEBUG(("!***!\n"));
#endif
}       

 
/****************************************************************************
NAME    
    headsetDisconnectAllSlc
    
DESCRIPTION
    Disconnect all the SLC's 

RETURNS
    void
*/
void headsetDisconnectAllSlc( void )
{
    
    SLC_DEBUG(("SLC: DisconAllSLC\n")) ;
    /* disconnect any connections */    
   	HfpSlcDisconnectRequest(hfp_primary_link);
   	HfpSlcDisconnectRequest(hfp_secondary_link);

#ifdef ENABLE_PBAP        
    /* Disconnect all PBAP links */
    pbapDisconnect();
#endif	
}

/****************************************************************************
NAME    
    slcEstablishSLCRequest
    
DESCRIPTION
    Request to create a connection to a remote AG.

RETURNS
    void
*/

void slcEstablishSLCRequest ( void )
{
    /* only attempt a connection is the headset is able to do so, 1 connection without multipoint only */
    if((!theHeadset.conf->no_of_profiles_connected)||
       ((theHeadset.MultipointEnable)&&(theHeadset.conf->no_of_profiles_connected < MAX_MULTIPOINT_CONNECTIONS)))     
    {
        theHeadset.rssi_action = rssi_connecting;
        if(slcDetermineConnectAction() & AR_Rssi)
        {
            slcStartInquiry();
        }
        else
        {
            SLC_DEBUG(("SLC: slcEstablishSLCRequest - MP = [%c] \n",theHeadset.MultipointEnable ? 'T':'F' ));
                              
            /* Cancel inquiry and throw away results if one is in progress (also resets rssi_action) */
            slcStopInquiry();

            /* reset connection via remote ag instead of headset flag */
            gSlcData.gSlcConnectRemote = FALSE;
	
            /*Paging attempts as per PSKEY's complete*/            
            SLC_DEBUG(("SLC: List Id [%d]\n" , gSlcData.gListID)) ;
            
            /* reset list to start at beginning if not already connected or to second in list if first is
               already connected */    
            gSlcData.gListID = gSlcData.gAttemptedListID = 0;
			
            /* get the number of devices in the PDL */
            gSlcData.gPdlSize = stateManagerGetPdlSize();

#ifdef BHC612
		SLC_DEBUG(("[gListID = %d,gPdlSize = %d]\n", gSlcData.gListID,gSlcData.gPdlSize));
#endif

            /* attempt to connect to first item in list */ 
            slcAttemptConnection();
        }
    }
}


/****************************************************************************
NAME    
    slcContinueEstablishSLCRequest
    
DESCRIPTION
    continue the connection request to create a connection to a remote AG.

RETURNS
    void
*/

void slcContinueEstablishSLCRequest( void )
{   
   
    SLC_DEBUG(("### SLC: ContSLCReq, listId = %d\n",gSlcData.gListID)) ;

    /* is multipoint enabled ?, if so see if necessary to connect further devices, also check for 
       non multipoint and no devices, otherwise exit */
    if((((theHeadset.MultipointEnable)&&(theHeadset.conf->no_of_profiles_connected < MAX_MULTIPOINT_CONNECTIONS)) ||
       ((!theHeadset.MultipointEnable)&&(!theHeadset.conf->no_of_profiles_connected))))
    {
        if(slcDetermineConnectAction() & AR_Rssi)
        {
            /* attempt to connect to next item in inquiry_data */
            slcInquiryConnectIndex(theHeadset.rssi_attempting + 1);
        }
        else
        {
            if(slcGetNextListID())
            {
                SLC_DEBUG(("SLC: PDL entry available for connection , listId = %d\n",gSlcData.gListID)) ;
			
                /* attempt to connect to next item in list */ 
                slcAttemptConnection();
            }
        }
    }
    /* Need to handle this case to stop RSSI when all devs connected */
    else if(slcDetermineConnectAction() & AR_Rssi)
    {
        /* All done, stop inquiring */
        slcStopInquiry();
    }

    SLC_DEBUG(("SLC: StopReq\n")) ;
}       

/****************************************************************************
NAME    
    slcAttemptConnection
    
DESCRIPTION
    attemp connection to next item in pdl

RETURNS
    void
*/
void slcAttemptConnection(void)
{
    bdaddr  ag_addr;
	
#ifdef DEBUG_SLC
	uint16	i;
#endif
    
    /* volume info for device being connected to */
    uint8 psdata[ATTRIBUTE_SIZE];
       
    /* store current index in case it fails */
    gSlcData.gAttemptedListID = gSlcData.gListID;

#ifdef BHC612
		SLC_DEBUG(("*** slcAttemptConnection ***\n"));
		SLC_DEBUG(("[gSlcData.gAttemptedListID = %d]\n", gSlcData.gAttemptedListID));
#endif
       
    /* attempt to obtain the device attributes for the current ListID required */
    if(ConnectionSmGetIndexedAttributeNow( ATTRIBUTE_PSKEY_BASE , gSlcData.gListID , ATTRIBUTE_SIZE, psdata, &ag_addr ))
    {
	        /* device exists, determine whether the device supports HFP/HSP, A2DP or both and connect
	           as appropriate */
	        SLC_DEBUG(("SLC: slcAttConn, listId = %d, attrib = %x\n",gSlcData.gListID,psdata[attribute_profiles_supported])) ;
	        
	        /* does device support HFP? */        
	        if(psdata[attribute_profiles_supported] & headset_hfp)        
	        {
	            SLC_DEBUG(("SLC: slcAttConn, HFP listId = %d\n",gSlcData.gListID)) ;
	            HfpSlcConnectRequest(&ag_addr,hfp_handsfree_and_headset, hfp_handsfree_all);
	        }
	        
	        /* does device support A2DP and is the feature to reconnect A2DP enabled? */
	        if (theHeadset.features.EnableA2dpStreaming &&(psdata[attribute_profiles_supported] & headset_a2dp))
	        {  
	            SLC_DEBUG(("SLC: slcAttConn, A2DP listId = %d\n",gSlcData.gListID)) ;
	            /* attempt connection to device supporting A2DP */
	            A2dpSignallingConnectRequest(&ag_addr);
	        }

		#ifdef BHC612
	            SLC_DEBUG(("slcAttemptConnection: [AG addr %x,%x,%lx]\n", ag_addr.nap,
	                                                           ag_addr.uap,
	                                                           ag_addr.lap)) ;
	#endif
    }   

	#ifdef DEBUG_SLC
		i = gSlcData.gListID;
		if(i == 0 )
			i++;
		else
			i--;
		if(ConnectionSmGetIndexedAttributeNow( ATTRIBUTE_PSKEY_BASE , i , ATTRIBUTE_SIZE, psdata, &ag_addr ))
		{
            		SLC_DEBUG(("###[AG2 addr %x,%x,%lx]\n", ag_addr.nap,
                                                           ag_addr.uap,
                                                           ag_addr.lap)) ;
		}
	#endif
    
    SLC_DEBUG(("SLC: slcAttConnm\n")) ;
                   
}

/****************************************************************************
NAME    
    slcDetermineConnectAction
    
DESCRIPTION
    Request to determine the connection action required, 

RETURNS
    required action based on current headset state
*/

ARAction_t slcDetermineConnectAction( void )
{
    ARAction_t lARAction = AR_LastConnected ;
    
    /*handle call transfer action*/
    if ( gSlcData.gCallTransferInProgress )
    {
        lARAction = theHeadset.features.ActionOnCallTransfer ;            
        SLC_DEBUG(("SLC: DCAction-Last-Call Tx[%d]\n" , lARAction));
    }
    else
    {
        /*we are freshly powered on / Not connected - use poweron action*/
        lARAction = theHeadset.features.ActionOnPowerOn ;
        SLC_DEBUG(("SLC: DCAction-PwrOn-[%d]\n", lARAction));
    }
    
    /* Mask out RSSI bit if rssi action has completed */
    if(theHeadset.rssi_action == rssi_none)
        lARAction &= 0x01;
    else if(theHeadset.rssi_action == rssi_pairing)
        lARAction |= AR_Rssi;
    return lARAction;
}    	

/****************************************************************************
NAME    
    headsetHandleRemoteSuppFeatures
    
DESCRIPTION
    Supported features of the remote device contained in the message if the 
    read succeeded. 

RETURNS
    void
*/
void headsetHandleRemoteSuppFeatures( const CL_DM_REMOTE_FEATURES_CFM_T *cfm )
{
    /* 
        If the read request succeeded then store the first word of the supported features 
        We should in theory store all four words but currently we only need the info in 
        the first word so for the sake of efficiency for the moment only store that.
    */
    if (cfm->status == hci_success)
	{
        theHeadset.conf->supp_features_remote = cfm->features[0];
	} 
}


/****************************************************************************
NAME	
	slcSetA2DPLinkPolicy

DESCRIPTION
	set the link policy requirements based on current headset audio state 
	
RETURNS
	void
*/
void slcSetA2DPLinkPolicy(uint16 DeviceId, uint16 StreamId, Sink sink )
{
    Sink sinkAG1,sinkAG2 = NULL;
    
    /* obtain any sco sinks */
    HfpLinkGetAudioSink(hfp_primary_link, &sinkAG1);
    HfpLinkGetAudioSink(hfp_secondary_link, &sinkAG2);
    
    /* determine if the connection is currently streaming and there are no scos currently open */    
    if((!sinkAG1 && !sinkAG2)&&(A2dpMediaGetState(DeviceId, StreamId) == a2dp_stream_streaming))
    {
       /* is there a user power table available from ps ? */
       if((theHeadset.user_power_table) && (theHeadset.user_power_table->A2DPStreamEntries))
	   {	            
  	       DEBUG(("SLC: SetLinkPolicy - A2dp user table \n"))

           /* User supplied power table for A2DP role */
       	   ConnectionSetLinkPolicy(sink, 
                                   theHeadset.user_power_table->A2DPStreamEntries ,
                                   &theHeadset.user_power_table->powertable[ theHeadset.user_power_table->normalEntries + theHeadset.user_power_table->SCOEntries + theHeadset.user_power_table->A2DPSigEntries ]
                                   );  
       }
       /* no user power table so use default A2DP table */
       else
       {	
  	       DEBUG(("SLC: SetLinkPolicy - A2dp default table \n" ));    
	       ConnectionSetLinkPolicy(sink, 1 ,a2dp_stream_powertable);
       }                         
    }
    /* if not streaming a2dp check for the prescence of sco data and if none found go to normal settings */
    else if((!sinkAG1 && !sinkAG2)&&
            (A2dpMediaGetState(theHeadset.a2dp_link_data->device_id[a2dp_primary], theHeadset.a2dp_link_data->stream_id[a2dp_primary])!= a2dp_stream_streaming)&&            
            (A2dpMediaGetState(theHeadset.a2dp_link_data->device_id[a2dp_secondary], theHeadset.a2dp_link_data->stream_id[a2dp_secondary])!= a2dp_stream_streaming))
    {
        /* set normal link policy settings */
        slcSetLinkPolicyNormal(sink);
    }           
}

/****************************************************************************
NAME	
	slcSetLinkPolicyNormal

DESCRIPTION
	set the link policy based on no a2dp streaming or sco 
	
RETURNS
	void
*/
void slcSetLinkPolicyNormal(Sink sink)
{
   /* Set up our sniff sub rate params for SLC */
   if(theHeadset.ssr_data)
    	ConnectionSetSniffSubRatePolicy(sink, theHeadset.ssr_data->slc_params.max_remote_latency, theHeadset.ssr_data->slc_params.min_remote_timeout, theHeadset.ssr_data->slc_params.min_local_timeout);
	
   /* audio not active, normal role, check for user defined power table */
   if((theHeadset.user_power_table)&&(theHeadset.user_power_table->normalEntries))
   {	              
       DEBUG(("SLC: SetLinkPolicy - norm user table \n" ));    
       /* User supplied power table */
   	   ConnectionSetLinkPolicy(sink, theHeadset.user_power_table->normalEntries ,&theHeadset.user_power_table->powertable[0]);               
   }
   /* no user defined power table so use default normal power table */       
   else
   {	
       DEBUG(("SLC: SetLinkPolicy - norm default table \n" ));    
       ConnectionSetLinkPolicy(sink, 2 ,hfp_default_powertable);
   }              
}

/****************************************************************************
NAME	
	HfpSetLinkPolicy

DESCRIPTION
	set the link policy requirements based on current headset audio state 
	
RETURNS
	void
*/
void slcSetLinkPolicy(hfp_link_priority priority, Sink slcSink )
{
    Sink audioSink;
    
    /* determine if there are any sco sinks */
    if(HfpLinkGetAudioSink(priority, &audioSink)&&(audioSink))
    {
	   /* Set up our sniff sub rate params for SCO */
	   if(theHeadset.ssr_data)
		   ConnectionSetSniffSubRatePolicy(slcSink, theHeadset.ssr_data->sco_params.max_remote_latency, theHeadset.ssr_data->sco_params.min_remote_timeout, theHeadset.ssr_data->sco_params.min_local_timeout);
	   
       /* is there a user power table available from ps ? */
       if((theHeadset.user_power_table) && (theHeadset.user_power_table->SCOEntries))
	   {	            
  	       DEBUG(("SLC: SetLinkPolicy - sco user table \n" ));    
           /* User supplied power table for SCO role */
       	   ConnectionSetLinkPolicy(slcSink, 
                                   theHeadset.user_power_table->SCOEntries ,
                                   &theHeadset.user_power_table->powertable[ theHeadset.user_power_table->normalEntries ]
                                   );               
       }
       /* no user power table so use default SCO table */
       else
       {	
  	       DEBUG(("SLC: SetLinkPolicy - sco default table \n" ));    
	       ConnectionSetLinkPolicy(slcSink, 2 ,hfp_default_powertable_sco);
       }              
    }
    /* default of no a2dp streaming and no sco link */
    else
    {
        /* set normal link policy settings */
        slcSetLinkPolicyNormal(slcSink);
    }           
}


/****************************************************************************
NAME    
    slcMultipointCheckConnectableState
    
DESCRIPTION
    when in multi point mode check to see if headset can be made connectable
    , this will be when only one AG is currently connected. this function will
    be called upon certain button presses which will reset the 60 second timer
    and allow a second AG to connect should the headset have become non discoverable
    
RETURNS
    none
*/
void slcMultipointCheckConnectableState( void )
{    
    /* only applicable to multipoint headsets and don't go connectable when taking or making
       an active call */
    if((theHeadset.MultipointEnable)&&(stateManagerGetState() < headsetOutgoingCallEstablish))
    {
       /* if only one hfp instance is connected then set connectable to active */
       if(theHeadset.conf->no_of_profiles_connected < 2)
       {
            SLC_DEBUG(("SLC: MP Go Conn \n" ));
            
            /* make headset connectable */
            headsetEnableConnectable();
         
            /* cancel any currently running timers that would disable connectable mode */
            MessageCancelAll( &theHeadset.task, EventConnectableTimeout );
            
            /* remain connectable for a further 'x' seconds to allow a second 
               AG to be connected if non-zero, otherwise stay connecatable forever */
            if(theHeadset.conf->timeouts.ConnectableTimeout_s)
            {
                MessageSendLater(&theHeadset.task, EventConnectableTimeout, 0, D_SEC(theHeadset.conf->timeouts.ConnectableTimeout_s));
            }
       }
       /* otherwise do nothing */
    }
}

/****************************************************************************
NAME    
    StoreProfilePsKeyInfo
    
DESCRIPTION
    stores the current values of volume and profile data in the attribute ps key
    , determine the current device and store in appropriate key,(User 32 to 39)

RETURNS
    void
*/
void StoreProfilePsKeyInfo(headset_link_type LinkType, hfp_link_priority priority, uint16 A2dpIndex)
{
	uint8  lAttributes [attribute_a2dp_clock_mismatch + 1] ;
    bdaddr SrcAddr;
    
#ifdef BHC612
	SLC_DEBUG(("StoreProfilePsKeyInfo,Link priority = %x\n",priority));
#endif
    /* initialise array of attributes */
    memset(lAttributes, 0, ATTRIBUTE_SIZE );
    
    /* for hfp devices get the attributes from the list_id of the connected device */
    if(LinkType == headset_hfp)
    {    	                
        /* get the current device attributes */
        ConnectionSmGetIndexedAttributeNow( ATTRIBUTE_PSKEY_BASE , theHeadset.profile_data[PROFILE_INDEX(priority)].status.list_id, ATTRIBUTE_SIZE, lAttributes, &SrcAddr );               

        SLC_DEBUG(("SLC: readPS hfp prof=%x hfpvol=%x a2dpvol=%x a2dpclkm=%x\n",lAttributes[0],lAttributes[1],lAttributes[2],lAttributes[3]));
    }
    /* for A2dp devices get the device attributes */
    else
    {
        /* get bdaddr from the stored a2dp_link_data structure */
        SrcAddr = theHeadset.a2dp_link_data->bd_addr[A2dpIndex];
        ConnectionSmGetAttributeNow(ATTRIBUTE_PSKEY_BASE, &SrcAddr, ATTRIBUTE_SIZE, lAttributes);

        SLC_DEBUG(("SLC: readPS A2dp addr=%x : %x : %lx\n",SrcAddr.nap, SrcAddr.uap, SrcAddr.lap ));        
        SLC_DEBUG(("SLC: readPS A2dp prof=%x hfpvol=%x a2dpvol=%x a2dpclkm=%x\n",lAttributes[0],lAttributes[1],lAttributes[2],lAttributes[3]));
    }
 
    /* check for link type, this could be hfp or a2dp */
    if(LinkType == headset_hfp && priority)
    {
       /* set HFP available flag */
       lAttributes[attribute_profiles_supported] |= headset_hfp;
       /* set the current HFP volume */
       lAttributes[attribute_hfp_volume] = (uint8) (theHeadset.profile_data[PROFILE_INDEX(priority)].audio.gSMVolumeLevel);
    }
    /* if A2DP link type */
    else if(LinkType == headset_a2dp)
    {
        /* set A2DP available flag */
        lAttributes[attribute_profiles_supported] |= headset_a2dp;
        /* set a2dp volume and supported codecs */        
        lAttributes[attribute_a2dp_volume] = (uint8) theHeadset.a2dp_link_data->gAvVolumeLevel[A2dpIndex];
        /* set clock mismatch rate */
        lAttributes[attribute_a2dp_clock_mismatch] = (uint8) theHeadset.a2dp_link_data->clockMismatchRate[A2dpIndex];
    }
    /* commit new settings to eeprom */
    ConnectionSmPutAttribute(ATTRIBUTE_PSKEY_BASE, &SrcAddr, ATTRIBUTE_SIZE ,  lAttributes ); 
    SLC_DEBUG(("Attributes Written = prof: %X hfp vol %x a2dp vol %x\n",lAttributes[0],lAttributes[1],lAttributes[2]));
}

/****************************************************************************
NAME    
    slcReset
    
DESCRIPTION
    reset the pdl connection pointer

RETURNS
    none
*/
void slcReset(void)
{
    /* reset back to start of pdl */
    gSlcData.gListID = 0;
#ifdef BHC612
	SLC_DEBUG(("Clear gListID\n"));
#endif
}

/****************************************************************************
NAME    
    headsetHandleServiceIndicator
    
DESCRIPTION
    Interprets the service Indicator messages and sends the appropriate Headset message 

RETURNS
    void
*/
void headsetHandleServiceIndicator ( const HFP_SERVICE_IND_T *pInd ) 
{
    /* only update the state if not set to network is present, this prevents repeated
       network is present indications from re-enabling the led's if they have gone to 
       sleep (timeout period) */  
    if(pInd->service != theHeadset.NetworkIsPresent)
    {

        if ( pInd->service )
        {
            MessageSend(&theHeadset.task , EventNetworkOrServicePresent , 0 ) ;
            theHeadset.NetworkIsPresent = TRUE ;
        }
        else /*the network service is OK*/
        {
            /*should only send this if not currently sending it*/
            if (theHeadset.NetworkIsPresent)
            {
                MessageSend(&theHeadset.task , EventNetworkOrServiceNotPresent  , 0 ) ;     
                theHeadset.NetworkIsPresent = FALSE ;
            }
        }
    }
}




/****************************************************************************
NAME    
    slcHandleRoleConfirm
    
DESCRIPTION
    this is a function checks the returned role of the headset and makes the decision of
    whether to change it or not, if it  needs changing it sends a role change reuest

RETURNS
    nothing
*/
void slcHandleRoleConfirm(CL_DM_ROLE_CFM_T *cfm)
{
    SLC_DEBUG(("RoleConfirm, sink = %x role=%s\n", (unsigned int)cfm->sink, (cfm->role == hci_role_master) ? "master" : "slave"));
   
    /* ensure role read successfully */
    if ((cfm->status == hci_success)&&(!theHeadset.features.DisableRoleSwitching))
    {
	    /* when multipoint enabled connect as master, this can be switched to slave
    	   later on if required when only 1 ag is connected */
	    if((theHeadset.MultipointEnable)  && (theHeadset.conf->no_of_profiles_connected > 1))
    	{            
            /* if not already master set role to be master */
            if(cfm->role != hci_role_master)
            {
         	    SLC_DEBUG(("SLC:Set dev as Ms %x\n",(unsigned int)cfm->sink)) ;

                /* set role to master for this connection */
    	        ConnectionSetRole(&theHeadset.task, cfm->sink, hci_role_master);             
            }
            else
            {
                SLC_DEBUG(("SLC: role not set, already master\n")) ;
            }
	    }
        /* non multipoint case, headset needs to be slave */
	    else
	    {  
            /* if not already master set role to be master */
            if(cfm->role != hci_role_slave)
            {
                SLC_DEBUG(("SLC:Set as Slave\n")) ;
	            
                /* set role to slave as only one AG connected */
    			if(theHeadset.user_power_table)
    	  		{	              
    				ConnectionSetRole(&theHeadset.task, cfm->sink, theHeadset.user_power_table->normalRole );
    			}
    			else
    			{
    	        	ConnectionSetRole(&theHeadset.task, cfm->sink, hci_role_slave );
    			}
    	    }
            else
            {
                 SLC_DEBUG(("SLC: role not set, already slave\n")) ;
            }
        }
	}
    /* check for failure of role switch due to AG having a sco open, if this is the case then
       reschedule the role switch until it is successfull or fails completely */
    else if(cfm->status == hci_error_role_change_not_allowed)
    {
		 uint16* lSink = malloc (sizeof(uint16) ) ;	   
         
		 *lSink = (uint16) cfm->sink ; 	

         SLC_DEBUG(("SLC: hci_error_role_change_not_allowed - Send Check Role again\n"));    
            /* if a SCO is open at the time a role change event is performed it will fail
               with the error message role_change_not_allowed, hence it is necessary to reschedule
               another another until it is successfull but only for the sink that failed to switch */
        MessageSendLater (&theHeadset.task , EventCheckRole , lSink , D_SEC(1) );
    }
	else
	{
		SLC_DEBUG(("SLC: Bypass role sw\n")) ;
	}        
}




/****************************************************************************
NAME    
    getScoPriorityFromHfpPriority
    
DESCRIPTION
    obtain the current sco priority level of the AG priority passed in

RETURNS
    current sco priority level, if any, may not have a sco
*/   
audio_priority getScoPriorityFromHfpPriority(hfp_link_priority priority)
{
	SLC_DEBUG(("SLC: GetScoPriority - %d=%d\n",priority,theHeadset.profile_data[PROFILE_INDEX(OTHER_PROFILE(priority))].audio.sco_priority)) ;
    return theHeadset.profile_data[PROFILE_INDEX(OTHER_PROFILE(priority))].audio.sco_priority;
}

/****************************************************************************
NAME    
    setScoPriorityFromHfpPriority
    
DESCRIPTION
    sets the current sco priority level of the AG priority passed in

RETURNS
    nothing
*/   
void setScoPriorityFromHfpPriority(hfp_link_priority priority, audio_priority level)
{
	SLC_DEBUG(("SLC: SetScoPriority - %d=%d\n",priority,level)) ;
    if(priority != hfp_invalid_link)    
        theHeadset.profile_data[PROFILE_INDEX(OTHER_PROFILE(priority))].audio.sco_priority = level;
}




/****************************************************************************
NAME    
    slcGetNextListID
    
DESCRIPTION
    selects the next available ListID for connection based on list id passed in and
    the type of profile requested, this could be 'not fussed', 'hfp' or 'a2dp', the 
    funtion will also check for the end of the pdl and wrap to the beggining if that 
    feature is enabled

RETURNS
    true or false success status
*/   
bool slcGetNextListID(void)
{
#ifdef BHC612
	SLC_DEBUG(("slcGetNextListID\n"));
#endif

    /* determine reconnection action, last or list */
    if(slcDetermineConnectAction() == AR_List)
    {
   		SLC_DEBUG(("SLC: slcGetNextListID - LIST - rem att = %d\n",theHeadset.conf->NoOfReconnectionAttempts)) ;

	#ifdef BHC612
			/* move to next item in list */
        		gSlcData.gListID++;
	#else
        /* move to next item in list */
        gSlcData.gListID++;
	#endif
	
#ifdef BHC612
		SLC_DEBUG(("[**gListID = %d]\n", gSlcData.gListID));
#endif
		
        /* check whether the ID is available, if not and at end of PDL, consider wrapping */
        if((!slcIsListIdAvailable(gSlcData.gListID))&&(gSlcData.gListID == gSlcData.gPdlSize)&&(gSlcData.gPdlSize>1))
        {
            /* At end of PDL, is PDL wrapping available ? */           
            if(theHeadset.conf->timeouts.ReconnectionAttempts && theHeadset.conf->NoOfReconnectionAttempts)
            {
           		SLC_DEBUG(("SLC: slcGetNextListID = %x - End of PDL - Wrap to 0\n",gSlcData.gListID)) ;
                /* wrapping available, go to start of PDL */
                gSlcData.gListID = 0;

#ifdef BHC612
		SLC_DEBUG(("[***gListID = %d]\n", gSlcData.gListID));
#endif
                /* check ListID 0 is available */
                if(!slcIsListIdAvailable(gSlcData.gListID))
                {
                    gSlcData.gListID = 1;
#ifdef BHC612
		SLC_DEBUG(("[****gListID = %d]\n", gSlcData.gListID));
#endif					
                    SLC_DEBUG(("SLC: slcGetNextListID = ID 0 not available use ID %x\n",gSlcData.gListID)) ;
                }
                /* success */
                return TRUE;
            }
            /* PDL wrapping not allowed so stop here */
            else
           	{
           		SLC_DEBUG(("SLC: slcGetNextListID = %x - End of PDL - No Wrapping\n",gSlcData.gListID)) ;
				#ifdef Rubidium
					if (!stateManagerIsConnected() && theHeadset.MultipointEnable)
					{
						SLC_DEBUG(("ReconnectFailed\n"));
						MessageSend(&theHeadset.task, EventReconnectFailed, 0);
					}
				#endif
                return FALSE;
            }
        }
        /* also check if ID not allowed due to being already connected, in which case try the next one */
        else if(!slcIsListIdAvailable(gSlcData.gListID))
        {
            /* move to next ID and check that */
            gSlcData.gListID++;
#ifdef BHC612
		SLC_DEBUG(("[*****gListID = %d]\n", gSlcData.gListID));
#endif			
            /* this should be available, if not it is end of PDL in which case stop as only
               allow two connections to be made */
            if(slcIsListIdAvailable(gSlcData.gListID))
           	{
                SLC_DEBUG(("SLC: slcGetNextListID = %x - Already Connected - try next\n",gSlcData.gListID)) ;
                return TRUE;
            }
            /* not available, there is only one device in PDL so return failed status */
            else
            {
           		SLC_DEBUG(("SLC: slcGetNextListID = %x - Already Connected - no next so stop\n",gSlcData.gListID)) ;
                return FALSE;
            }
        }
        /* new ListId is available for trying a connection */
        else
        {
       		SLC_DEBUG(("SLC: slcGetNextListID = %x - OK\n",gSlcData.gListID)) ;
            return TRUE;
        }
    }
    /* LAST reconnection type, headset will connect to first two devices in PDL */
    else
    {       
       /* if no profiles are connected and multipoint is not enabled give up */
       if((!theHeadset.MultipointEnable)&&(!theHeadset.conf->no_of_profiles_connected))
       {
            return FALSE;
       }
       /* if multipoint is enabled then try to connect to the second device for one attempt and then
          give up */
       else if((theHeadset.MultipointEnable)&&(!gSlcData.gListID))
       {
            /* the ListID should be 0 indicating the connection to the first device
               has been attempted */
            /* move to next device in PDL */
            gSlcData.gListID++;
#ifdef BHC612
		SLC_DEBUG(("[******gListID = %d]\n", gSlcData.gListID));
#endif
            SLC_DEBUG(("SLC: slcGetNextListID - LAST\n")) ;
            /* ensure device is available and return TRUE or FALSE status */
            return slcIsListIdAvailable(gSlcData.gListID);
        }
        /* ListID is wrong, should be zero wanting to try the second connection */
        else
            return FALSE;
    }
}

/****************************************************************************
NAME    
    slcIsListIdAvailable
    
DESCRIPTION
    determine whether the ListID passed in is available in the PDL, the ID
    passed in could be out of range of the PDL so checks for that also

RETURNS
    true or false success status
*/   
bool slcIsListIdAvailable(uint8 ListID)
{
    
    /* check ListID is a valid value within the range of the available devices in the PDL 
       and that it is not already connected */
    if((ListID < gSlcData.gPdlSize)&&(isPdlEntryAvailable(ListID)))
    {
        SLC_DEBUG(("SLC: slcIsListIdAvailable - TRUE - ListID = %x\n",ListID)) ;
        return TRUE;        
    }
    /* out of range value or already connected return failed status */
    else
    {
        SLC_DEBUG(("SLC: slcIsListIdAvailable - FALSE - ListID = %x\n",ListID)) ;
        return FALSE;
    }
}

/****************************************************************************
NAME    
    isPdlEntryAvailable
    
DESCRIPTION
    looks to see if passed in pdl index is already connected 

RETURNS
    TRUE or FALSE
*/
bool isPdlEntryAvailable( uint8 Id )
{      
    uint8 i;
    
    /* scan hfp and hsp profile instances looking for a match */
    for(i=0;i<MAX_PROFILES;i++)        
    {
        /* look for hfp profile that is connected and check its pdl listID */
        if(theHeadset.profile_data[i].status.list_id == Id)
           return FALSE;
    }
    /* no matches found so ID is available for use */
    SLC_DEBUG(("SLC: isPdlEntryAvailable for ID=%d is TRUE\n",Id ));    
    return TRUE;   
}

/****************************************************************************
NAME    
    isBdaddrAlreadyConnected
    
DESCRIPTION
    compare passed in bdaddr against those of the current connected devices
    , if a match is found returns true

RETURNS
    TRUE or FALSE
*/
uint8 isBdaddrAlreadyConnected(const bdaddr * bd_addr)
{
    Sink sinks[(MAX_MULTIPOINT_CONNECTIONS+MAX_A2DP_CONNECTIONS)];    
    bdaddr device_bdaddr;
    uint16 i;
    uint8 lResult = 0;
    
    /* obtain the sinks to any hfp instances */
    HfpLinkGetSlcSink(hfp_primary_link, &sinks[0]);
    HfpLinkGetSlcSink(hfp_secondary_link, &sinks[1]);

    /* if a2dp is enabled, get the signalling channel sink of any active connections
       , from this the bdaddr may be found and compared to that of the incoming request */
    if(theHeadset.features.EnableA2dpStreaming)
    {
        sinks[2] = A2dpSignallingGetSink(a2dp_primary);            
        sinks[3] = A2dpSignallingGetSink(a2dp_secondary);
    }
   
    /* cycle through the available sinks looking for a device bdaddr match */
    for(i=0;i<(MAX_MULTIPOINT_CONNECTIONS+MAX_A2DP_CONNECTIONS);i++)
    {        
        /* if a2dp connected */    
        if(sinks[i])
        {
           /* if A2dp device present is it an AV only source or part of an AG/phone */
           if(SinkGetBdAddr(sinks[i], &device_bdaddr))
           {    
                /* check whether A2DP is part of hfp device */
                if(BdaddrIsSame(bd_addr, &device_bdaddr))
                {
                    /* result is a bitmask of connections, bits 0&1 are hfp, 2&3 are a2dp */                    
                    lResult |= (1<<i);
                }
            }
        }
    }
    
    /* no matches found, this is a new device */     
    return lResult;
    
}

/****************************************************************************
NAME    
    GetNumberOfConnectedDevices
    
DESCRIPTION
   determines the number of different connected devices, a device may connected
   both hfp and a2dp or only one of each
RETURNS
    number of connected devices
*/
uint8 GetNumberOfConnectedDevices(void)
{
    Sink sink;
    bdaddr device_addr;
    uint8 NoOfDevices = 0;
    
    /* caclulate the number of devices connected */
    if((HfpLinkGetSlcSink(hfp_primary_link, &sink))&&(sink))
    {
        NoOfDevices++;
	/*SLC_DEBUG(("+1 "));*/
    }
    /* check hfp secondary */
    if((HfpLinkGetSlcSink(hfp_secondary_link, &sink))&&(sink))
    {
        NoOfDevices++;
	/*SLC_DEBUG(("+2 "));*/
    }
    /* check primary a2dp */        
    if((sink = A2dpSignallingGetSink(a2dp_primary))&&(SinkGetBdAddr(sink, &device_addr)))
    {
        /* check whether this address is is already connected to a hfp profile, if it is then
           this is the same device, no need to increment no of devices, if not then this is a 
           new device, increment number of devices */
        if(!(isBdaddrAlreadyConnected(&device_addr) & 0x03))
        {
            NoOfDevices++;   
		/*SLC_DEBUG(("+3 "));*/
        }
    }
    /* check secondary a2dp */        
    if((sink = A2dpSignallingGetSink(a2dp_secondary))&&(SinkGetBdAddr(sink, &device_addr)))
    {
        /* check whether this address is is already connected to a hfp profile, if it is then
           this is the same device, no need to increment no of devices, if not then this is a 
           new device, increment number of devices */
        if(!(isBdaddrAlreadyConnected(&device_addr) & 0x03))
        {
            NoOfDevices++;
		/*SLC_DEBUG(("+4 "));*/
        }
    }

    SLC_DEBUG(("SLC: GetNumberOfConnectedDevices=%d \n",NoOfDevices ));    
    
    return NoOfDevices;
}
        
        
        
