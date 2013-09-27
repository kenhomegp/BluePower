/****************************************************************************

FILE NAME
    headset_slc.h
    
DESCRIPTION
    
*/

#ifndef _HEADSET_SLC_H_
#define _HEADSET_SLC_H_

typedef enum 
{
    profile_type_hfp,
    profile_type_a2dp,
    profile_type_both
}profile_type;

/*the action to take on the auto reconnect*/
typedef enum AutoReconnectActionTag
{
    AR_LastConnected    = 0 ,   
    AR_List             = 1 ,
    AR_Rssi             = 2 
}ARAction_t ;

/* Inquiry reminder timeout */
#define INQUIRY_REMINDER_TIMEOUT_SECS 5
#define INVALID_LIST_ID 0xFF

/* pskey user 41 to 49 AG settings */
typedef enum
{
	attribute_profiles_supported,   /* HFP and A2DP */
	attribute_hfp_volume,           /* last volume used when AG last connected */
	attribute_a2dp_volume,          /* last volume used when AG was streaming audio */
    attribute_a2dp_clock_mismatch   /* last clock mismatch value when AG was streaming audio */
} AttributesType;

/* link types the headset supports */
typedef enum
{
    headset_hfp = 0x01,
    headset_a2dp = 0x02    
}headset_link_type;

/****************************************************************************/

/****************************************************************************
NAME    
    slcStartInquiry
    
DESCRIPTION
    Kick off Inquiry
RETURNS
    void
*/
void slcStartInquiry( void );

/****************************************************************************
NAME    
    slcStopInquiry
    
DESCRIPTION
    If inquiry in progress throw away the results and cancel further CL messages
RETURNS
    void
*/
void slcStopInquiry( void );

/****************************************************************************
NAME    
    slcHandleInquiryResult
    
DESCRIPTION
    Inquiry result received
RETURNS
    void
*/
void slcHandleInquiryResult( CL_DM_INQUIRE_RESULT_T* result );


/****************************************************************************
NAME    
    slcConnectFail
    
DESCRIPTION
    SLC failed to connect
RETURNS
    void
*/
void slcConnectFail( hfp_link_priority priority );

/****************************************************************************
NAME    
    headsetHandleSlcConnectInd
    
DESCRIPTION
    Handle a request to establish an SLC from the AG.

RETURNS
    void
*/
void headsetHandleSlcConnectInd( const HFP_SLC_CONNECT_IND_T *ind );

/****************************************************************************
NAME    
    headsetHandleSlcDisconnectInd
    
DESCRIPTION
    Indication that the SLC has been released.

RETURNS
    void
*/
void headsetHandleSlcDisconnectInd( const HFP_SLC_DISCONNECT_IND_T *ind );


/****************************************************************************
NAME    
    slcEstablishSLCRequest
    
DESCRIPTION
    Request to create a connection to a remote AG.

RETURNS
    void
*/

void slcEstablishSLCRequest ( void );

/****************************************************************************
NAME    
    slcContinueEstablishSLCRequest
    
DESCRIPTION
    continue the connection request to create a connection to a remote AG.

RETURNS
    void
*/

void slcContinueEstablishSLCRequest (void);

/****************************************************************************
NAME    
    slcAttemptConnection
    
DESCRIPTION
    attemp connection to next item in pdl

RETURNS
    void
*/
void slcAttemptConnection(void);

/****************************************************************************
NAME    
    slcDetermineConnectAction
    
DESCRIPTION
    Request to determine the connection action required, 

RETURNS
    required action based on current headset state
*/

ARAction_t slcDetermineConnectAction( void );

/****************************************************************************
NAME    
    headsetHandleRemoteSuppFeatures
    
DESCRIPTION
    Supported features of the remote device contained in the message if the 
    read succeeded. 

RETURNS
    void
*/
void headsetHandleRemoteSuppFeatures( const CL_DM_REMOTE_FEATURES_CFM_T *cfm );

/****************************************************************************
NAME    
    slcConnectAfterLinkLoss
    
DESCRIPTION
    Request to create a connection to a remote AG after a link loss has occured.

RETURNS
    void
*/   
void slcConnectAfterLinkLoss ( void );
/****************************************************************************
NAME    
    headsetHandleSlcConnectCfm
    
DESCRIPTION
    Confirmation that the SLC has been established (or not).

RETURNS
    void
*/
bool headsetHandleSlcConnectCfm( const HFP_SLC_CONNECT_CFM_T *cfm );
        
/****************************************************************************
NAME    
    slcProfileConnected
    
DESCRIPTION
    Indicate that a profile has connected 

RETURNS
    void
*/
void slcProfileConnected ( hfp_link_priority priority, Sink sink );


/****************************************************************************
NAME    
    headsetDisconnectAllSlc
    
DESCRIPTION
    Disconnect all the SLC's 

RETURNS
    void
*/
void headsetDisconnectAllSlc( void );     
        
/****************************************************************************
NAME	
	slcSetA2DPLinkPolicy

DESCRIPTION
	set the link policy requirements based on current headset audio state 
	
RETURNS
	void
*/
void slcSetA2DPLinkPolicy(uint16 DeviceId, uint16 StreamId, Sink sink );

/****************************************************************************
NAME	
	slcSetLinkPolicyNormal

DESCRIPTION
	set the link policy based on no a2dp streaming or sco 
	
RETURNS
	void
*/
void slcSetLinkPolicyNormal(Sink sink);

/****************************************************************************
NAME	
	slcSetLinkPolicy

DESCRIPTION
	set the link policy requirements based on current headset audio state 
	
RETURNS
	void
*/
void slcSetLinkPolicy(hfp_link_priority priority, Sink slcSink );

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
void slcMultipointCheckConnectableState( void );

/****************************************************************************
NAME    
    getHfpFreeProfile
    
DESCRIPTION
    searches for available profile of type passed in and returns index to free
    profile
    
RETURNS
    -1 if none available
*/
int8 getHfpFreeProfile( hfp_profile pProfile );

/****************************************************************************
NAME    
    getHfpIndexFromLinkLoss
    
DESCRIPTION
    searches for link loss and returns index to free
    profile
    
RETURNS
    -1 if none available
*/
int8 getHfpIndexFromLinkLoss( void );

/****************************************************************************
NAME    
    StoreProfilePsKeyInfo
    
DESCRIPTION
    stores the current values of volume and profile data in the attribute ps key
    , determine the current device and store in appropriate key,(User 32 to 39)

RETURNS
    void
*/
void StoreProfilePsKeyInfo(headset_link_type LinkType, hfp_link_priority priority, uint16 A2dpIndex);

/****************************************************************************
NAME    
    slcReset
    
DESCRIPTION
    reset the pdl connection pointer

RETURNS
    none
*/
void slcReset(void);


/****************************************************************************
NAME    
    headsetHandleServiceIndicator
    
DESCRIPTION
    Interprets the service Indicator messages and sends the appropriate Headset message 

RETURNS
    void
*/
void headsetHandleServiceIndicator ( const HFP_SERVICE_IND_T *pInd );


/****************************************************************************
NAME    
    isPdlEntryAvailable
    
DESCRIPTION
    looks to see if passed in pdl index is already connected 

RETURNS
    TRUE or FALSE
*/
bool isPdlEntryAvailable( uint8 Id );

/****************************************************************************
NAME    
    slcHandleRoleConfirm
    
DESCRIPTION
    this is a function checks the returned role of the headset and makes the decision of
    whether to change it or not, if it  needs changing it sends a role change reuest

RETURNS
    nothing
*/
void slcHandleRoleConfirm(CL_DM_ROLE_CFM_T *cfm);

/****************************************************************************
NAME    
    getScoPriorityFromHfpPriority
    
DESCRIPTION
    obtain the current sco priority level of the AG priority passed in

RETURNS
    current sco priority level, if any, may not have a sco
*/   
audio_priority getScoPriorityFromHfpPriority(hfp_link_priority priority);

/****************************************************************************
NAME    
    setScoPriorityFromHfpPriority
    
DESCRIPTION
    sets the current sco priority level of the AG priority passed in

RETURNS
    nothing
*/   
void setScoPriorityFromHfpPriority(hfp_link_priority priority, audio_priority level);

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
bool slcGetNextListID(void);

/****************************************************************************
NAME    
    slcIsListIdAvailable
    
DESCRIPTION
    determine whether the ListID passed in is available in the PDL, the ID
    passed in could be out of range of the PDL so checks for that also

RETURNS
    true or false success status
*/   
bool slcIsListIdAvailable(uint8 ListID);

/****************************************************************************
NAME    
    isBdaddrAlreadyConnected
    
DESCRIPTION
    compare passed in bdaddr against those of the current connected devices
    , if a match is found returns true

RETURNS
    TRUE or FALSE
*/
uint8 isBdaddrAlreadyConnected(const bdaddr * bd_addr);

/****************************************************************************
NAME    
    GetNumberOfConnectedDevices
    
DESCRIPTION
   determines the number of different connected devices, a device may connected
   both hfp and a2dp or only one of each
RETURNS
    number of connected devices
*/
uint8 GetNumberOfConnectedDevices(void);

#endif /* _HEADSET_SLC_H_ */

