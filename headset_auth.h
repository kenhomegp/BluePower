
#include "headset_private.h"

#ifndef _HEADSET_AUTH_PRIVATE_H_
#define _HEADSET_AUTH_PRIVATE_H_

/*************************************************************************
NAME    
     headsetHandlePinCodeInd
    
DESCRIPTION
     This function is called on receipt on an CL_PIN_CODE_IND message
     being recieved.  The AV Headset default pin code is sent back.

RETURNS
     
*/
void headsetHandlePinCodeInd(const CL_SM_PIN_CODE_IND_T* ind);


/*************************************************************************
NAME    
     headsetHandleUserConfirmationInd
    
DESCRIPTION
     This function is called on receipt on an CL_SM_USER_CONFIRMATION_IND

RETURNS
     
*/
void headsetHandleUserConfirmationInd(const CL_SM_USER_CONFIRMATION_REQ_IND_T* ind);
		
		
/*************************************************************************
NAME    
     headsetHandleUserConfirmationInd
    
DESCRIPTION
     This function is called on receipt on an CL_SM_USER_CONFIRMATION_IND

RETURNS
     
*/
void headsetHandleUserPasskeyInd(const CL_SM_USER_PASSKEY_REQ_IND_T* ind);


/*************************************************************************
NAME    
     headsetHandleUserPasskeyNotificationInd
    
DESCRIPTION
     This function is called on receipt on an CL_SM_USER_PASSKEY_NOTIFICATION_IND

RETURNS
     
*/
void headsetHandleUserPasskeyNotificationInd(const CL_SM_USER_PASSKEY_NOTIFICATION_IND_T* ind);
		

/*************************************************************************
NAME    
     headsetHandleIoCapabilityInd
    
DESCRIPTION
     This function is called on receipt on an CL_SM_IO_CAPABILITY_REQ_IND

RETURNS
     
*/
void headsetHandleIoCapabilityInd(const CL_SM_IO_CAPABILITY_REQ_IND_T* ind);
		
		
/*************************************************************************
NAME    
     headsetHandleRemoteIoCapabilityInd
    
DESCRIPTION
     This function is called on receipt on an CL_SM_REMOTE_IO_CAPABILITY_IND

RETURNS
     
*/
void headsetHandleRemoteIoCapabilityInd(const CL_SM_REMOTE_IO_CAPABILITY_IND_T* ind);


/****************************************************************************
NAME    
    headsetHandleAuthoriseInd
    
DESCRIPTION
    Request to authorise access to a particular service.

RETURNS
    void
*/
void headsetHandleAuthoriseInd(const CL_SM_AUTHORISE_IND_T *ind);


/****************************************************************************
NAME    
    headsetHandleAuthenticateCfm
    
DESCRIPTION
    Indicates whether the authentication succeeded or not.

RETURNS
    void
*/
void headsetHandleAuthenticateCfm(const CL_SM_AUTHENTICATE_CFM_T *cfm);

/****************************************************************************
NAME    
    headsetPairingAcceptRes 
    
DESCRIPTION
    Respond correctly to a pairing info request ind (pin code, passkey, etc...)

RETURNS
    void
*/
void headsetPairingAcceptRes( void );

/****************************************************************************
NAME    
    headsetPairingRejectRes 
    
DESCRIPTION
    Respond reject to a pairing info request ind (pin code, passkey, etc...)

RETURNS
    void
*/
void headsetPairingRejectRes( void );

/****************************************************************************
NAME    
    AuthResetConfirmationFlags
    
DESCRIPTION
    Helper function to reset the confirmation flags and associated BT address

RETURNS
     
*/

void AuthResetConfirmationFlags ( void ) ;

#endif /* _HEADSET_AUTH_PRIVATE_H_ */
