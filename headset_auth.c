
/****************************************************************************
    Header files
*/
#include "headset_private.h"
#include "headset_tts.h"
#include "headset_stateManager.h"
#include "headset_auth.h"

#include "headset_slc.h"
#include "headset_debug.h"

#include <ps.h>
#include <bdaddr.h>
#include <stdlib.h>
#include <sink.h>

#ifdef DEBUG_AUTH
    #define AUTH_DEBUG(x) DEBUG(x)    
#else
    #define AUTH_DEBUG(x) 
#endif   

/****************************************************************************
NAME    
    AuthCanHeadsetConnect 
    
DESCRIPTION
    Helper function to indicate if connecting is allowed

RETURNS
    bool
*/

bool AuthCanHeadsetConnect ( const bdaddr * bd_addr );

/****************************************************************************
NAME    
    AuthCanHeadsetPair 
    
DESCRIPTION
    Helper function to indicate if pairing is allowed

RETURNS
    bool
*/

bool AuthCanHeadsetPair ( void ) ;

/*************************************************************************
NAME    
     headsetHandlePinCodeInd
    
DESCRIPTION
     This function is called on receipt on an CL_PIN_CODE_IND message
     being recieved.  The AV Headset default pin code is sent back.

RETURNS
     
*/
void headsetHandlePinCodeInd(const CL_SM_PIN_CODE_IND_T* ind)
{
    uint16 pin_length = 0;
    uint8 pin[16];
    
    if ( AuthCanHeadsetPair() )
    {
	    
		AUTH_DEBUG(("auth: Can Pin\n")) ;
		
   		/* Do we have a fixed pin in PS, if not reject pairing */
    	if ((pin_length = PsFullRetrieve(PSKEY_FIXED_PIN, pin, 16)) == 0 || pin_length > 16)
   		{
   	    	/* Set length to 0 indicating we're rejecting the PIN request */
        	AUTH_DEBUG(("auth : failed to get pin\n")) ;
       		pin_length = 0; 
   		}	
        else if(theHeadset.features.VoicePromptPairing)
        {
            TTSPlayEvent(EventPinCodeRequest);
            TTSPlayNumString(pin_length, pin);
        }
	} 
    /* Respond to the PIN code request */
    ConnectionSmPinCodeResponse(&ind->bd_addr, pin_length, pin); 
}

/*************************************************************************
NAME    
     headsetHandleUserConfirmationInd
    
DESCRIPTION
     This function is called on receipt on an CL_SM_USER_CONFIRMATION_IND

RETURNS
     
*/
void headsetHandleUserConfirmationInd(const CL_SM_USER_CONFIRMATION_REQ_IND_T* ind)
{
	/* Can we pair? */
	if ( AuthCanHeadsetPair() && theHeadset.features.ManInTheMiddle)
    {
        theHeadset.confirmation = TRUE;
		AUTH_DEBUG(("auth: Can Confirm %ld\n",ind->numeric_value)) ;
		/* Should use text to speech here */
		theHeadset.confirmation_addr = mallocPanic(sizeof(bdaddr));
		*theHeadset.confirmation_addr = ind->bd_addr;
        if(theHeadset.features.VoicePromptPairing)
        {
            TTSPlayEvent(EventConfirmationRequest);
            TTSPlayNumber(ind->numeric_value);
        }
	}
	else
    {
		/* Reject the Confirmation request */
		AUTH_DEBUG(("auth: Rejected Confirmation Req\n")) ;
		ConnectionSmUserConfirmationResponse(&ind->bd_addr, FALSE);
    }
}

/*************************************************************************
NAME    
     headsetHandleUserPasskeyInd
    
DESCRIPTION
     This function is called on receipt on an CL_SM_USER_PASSKEY_IND

RETURNS
     
*/
void headsetHandleUserPasskeyInd(const CL_SM_USER_PASSKEY_REQ_IND_T* ind)
{
	/* Reject the Passkey request */
	AUTH_DEBUG(("auth: Rejected Passkey Req\n")) ;
	ConnectionSmUserPasskeyResponse(&ind->bd_addr, TRUE, 0);
}


/*************************************************************************
NAME    
     headsetHandleUserPasskeyNotificationInd
    
DESCRIPTION
     This function is called on receipt on an CL_SM_USER_PASSKEY_NOTIFICATION_IND

RETURNS
     
*/
void headsetHandleUserPasskeyNotificationInd(const CL_SM_USER_PASSKEY_NOTIFICATION_IND_T* ind)
{
	AUTH_DEBUG(("Passkey: %ld \n", ind->passkey));
    if(theHeadset.features.ManInTheMiddle && theHeadset.features.VoicePromptPairing)
    {
        TTSPlayEvent(EventPasskeyDisplay);
        TTSPlayNumber(ind->passkey);
    }
	/* Should use text to speech here */
}

/*************************************************************************
NAME    
     headsetHandleIoCapabilityInd
    
DESCRIPTION
     This function is called on receipt on an CL_SM_IO_CAPABILITY_REQ_IND

RETURNS
     
*/
void headsetHandleIoCapabilityInd(const CL_SM_IO_CAPABILITY_REQ_IND_T* ind)
{	
	/* If not pairable should reject */
	if(AuthCanHeadsetPair())
	{
		cl_sm_io_capability local_io_capability = theHeadset.features.ManInTheMiddle ? cl_sm_io_cap_display_yes_no : cl_sm_io_cap_no_input_no_output;
		
		AUTH_DEBUG(("auth: Sending IO Capability \n"));
		
		/* Send Response */
		ConnectionSmIoCapabilityResponse(&ind->bd_addr,local_io_capability,theHeadset.features.ManInTheMiddle,TRUE,FALSE,0,0);
	}
	else
	{
		AUTH_DEBUG(("auth: Rejecting IO Capability Req \n"));
		ConnectionSmIoCapabilityResponse(&ind->bd_addr, cl_sm_reject_request,FALSE,FALSE,FALSE,0,0);
	}
}

/*************************************************************************
NAME    
     headsetHandleRemoteIoCapabilityInd
    
DESCRIPTION
     This function is called on receipt on an CL_SM_REMOTE_IO_CAPABILITY_IND

RETURNS
     
*/
void headsetHandleRemoteIoCapabilityInd(const CL_SM_REMOTE_IO_CAPABILITY_IND_T* ind)
{
	AUTH_DEBUG(("auth: Incoming Authentication Request\n"));
}

/****************************************************************************
NAME    
    headsetHandleAuthoriseInd
    
DESCRIPTION
    Request to authorise access to a particular service.

RETURNS
    void
*/
void headsetHandleAuthoriseInd(const CL_SM_AUTHORISE_IND_T *ind)
{
	
	bool lAuthorised = FALSE ;
	
	if ( AuthCanHeadsetConnect(&ind->bd_addr) )
	{
		lAuthorised = TRUE ;
	}
	
	AUTH_DEBUG(("auth: Authorised [%d]\n" , lAuthorised)) ;
	    
	/*complete the authentication with the authorised or not flag*/
    ConnectionSmAuthoriseResponse(&ind->bd_addr, ind->protocol_id, ind->channel, ind->incoming, lAuthorised);
}


/****************************************************************************
NAME    
    headsetHandleAuthenticateCfm
    
DESCRIPTION
    Indicates whether the authentication succeeded or not.

RETURNS
    void
*/
void headsetHandleAuthenticateCfm(const CL_SM_AUTHENTICATE_CFM_T *cfm)
{	
	/* Leave bondable mode if successful unless we got a debug key */
	if (cfm->status == auth_status_success && cfm->key_type != cl_sm_link_key_debug)
		MessageSend (&theHeadset.task , EventPairingSuccessful , 0 );
		
	/* Set up some default params and shuffle PDL */
	if(cfm->bonded)
	{
        uint8  lAttributes [attribute_a2dp_clock_mismatch + 1] ;
        /* set HFP available flag */
        lAttributes[attribute_profiles_supported] = headset_hfp;
   		/* Default volume */
		lAttributes[attribute_hfp_volume] = (uint8) theHeadset.features.DefaultVolume ;		
        /* set a2dp volume and supported codecs */        
        lAttributes[attribute_a2dp_volume] = (uint8) theHeadset.features.DefaultA2dpVolLevel;
        /* set a2dp clock mismatch */        
        lAttributes[attribute_a2dp_clock_mismatch] = 0;
		/* Write params to PS */
		ConnectionSmPutAttribute(ATTRIBUTE_PSKEY_BASE, &cfm->bd_addr, ATTRIBUTE_SIZE ,  lAttributes ); 
		
		/* Shuffle the PDL around the device */
		ConnectionSmUpdateMruDevice( &cfm->bd_addr ) ;
	}
	
	/* Reset pairing info if we timed out on confirmation */
	AuthResetConfirmationFlags();
}


/****************************************************************************
NAME    
    AuthCanHeadsetPair 
    
DESCRIPTION
    Helper function to indicate if pairing is allowed

RETURNS
    bool
*/

bool AuthCanHeadsetPair ( void )
{
	bool lCanPair = FALSE ;
	
    if (theHeadset.features.SecurePairing)
    {
	    	/*if we are in pairing mode*/
		if (stateManagerGetState() == headsetConnDiscoverable)
		{
			lCanPair = TRUE ;
			AUTH_DEBUG(("auth: is ConnDisco\n")) ;
		}	    		
    }
    else
    {
	    lCanPair = TRUE ;
    }
    return lCanPair ;
}



/****************************************************************************
NAME    
    AuthCanHeadsetConnect 
    
DESCRIPTION
    Helper function to indicate if connecting is allowed

RETURNS
    bool
*/

bool AuthCanHeadsetConnect ( const bdaddr * bd_addr )
{
	bool lCanConnect = FALSE ;
    uint8 NoOfDevices = GetNumberOfConnectedDevices();
    
    /* if device is already connected via a different profile allow this next profile to connect */
    if(isBdaddrAlreadyConnected(bd_addr))
    {
    	AUTH_DEBUG(("auth: already connected, CanConnect = TRUE\n")) ;
        lCanConnect = TRUE;
    }
    /* this bdaddr is not already connected, therefore this is a new device, ensure it is allowed 
       to connect, if not reject it */
    else
    {
        /* when multipoint is turned off, only allow one device to connect */
        if(((!theHeadset.MultipointEnable)&&(!NoOfDevices))||
           ((theHeadset.MultipointEnable)&&(NoOfDevices < MAX_MULTIPOINT_CONNECTIONS)))
        {
            /* is secure pairing enabled? */
            if (theHeadset.features.SecurePairing)
            {
    	        /* If page scan is enabled (i.e. we are either connectable/discoverable or 
    	    	connected in multi point) */
    	    	if ( theHeadset.page_scan_enabled )
    	    	{
    	    		lCanConnect = TRUE ;
    	    		AUTH_DEBUG(("auth: is connectable\n")) ;
    	    	}		
            }
            /* no secure pairing */
            else
            {
            	AUTH_DEBUG(("auth: MP CanConnect = TRUE\n")) ;
    	        lCanConnect = TRUE ;
            }
        }
    }
  
    AUTH_DEBUG(("auth:  CanConnect = %d\n",lCanConnect)) ;
  
    return lCanConnect ;
}

/****************************************************************************
NAME    
    headsetPairingAcceptRes 
    
DESCRIPTION
    Respond correctly to a pairing info request ind

RETURNS
    void
*/
void headsetPairingAcceptRes( void )
{		
    if(AuthCanHeadsetPair() && theHeadset.confirmation)
	{
		AUTH_DEBUG(("auth: Accepted Confirmation Req\n")) ;
		ConnectionSmUserConfirmationResponse(theHeadset.confirmation_addr, TRUE);
     }
	else
     {
		AUTH_DEBUG(("auth: Invalid state for confirmation\n"));
     }
}

/****************************************************************************
NAME    
    headsetPairingRejectRes 
    
DESCRIPTION
    Respond reject to a pairing info request ind

RETURNS
    void
*/
void headsetPairingRejectRes( void )
{
	if(AuthCanHeadsetPair() && theHeadset.confirmation)
	{	
		AUTH_DEBUG(("auth: Rejected Confirmation Req\n")) ;
		ConnectionSmUserConfirmationResponse(theHeadset.confirmation_addr, FALSE);
	}
	else
	{
		AUTH_DEBUG(("auth: Invalid state for confirmation\n"));
	}
}

/****************************************************************************
NAME    
    AuthResetConfirmationFlags
    
DESCRIPTION
    Helper function to reset the confirmations flag and associated BT address

RETURNS
     
*/

void AuthResetConfirmationFlags ( void )
{
	AUTH_DEBUG(("auth: Reset Confirmation Flags\n"));
	if(theHeadset.confirmation)
	{
		AUTH_DEBUG(("auth: Free Confirmation Addr\n"));
		free(theHeadset.confirmation_addr);
	}
	theHeadset.confirmation_addr = NULL;
	theHeadset.confirmation = FALSE;
}

