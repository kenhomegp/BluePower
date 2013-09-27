/****************************************************************************

FILE NAME
    headset_dut.h
    
DESCRIPTION
	Place the headset into Device Under Test (DUT) mode
    
*/

#ifndef _HEADSET_DUT_H_
#define _HEADSET_DUT_H_

void checkPowerBluConn(void);

/****************************************************************************
DESCRIPTION
  	This function is called to place the headset into DUT mode
*/
void enterDutMode(void);

/****************************************************************************
DESCRIPTION
  	This function is called to place the headset into TX continuous test mode
*/

void enterTxContinuousTestMode ( void ) ;

/****************************************************************************
DESCRIPTION
  	This function is called to determine if the headset should enter DUT mode.
*/
bool checkForDUTModeEntry( void );

/****************************************************************************
DESCRIPTION
  	Enter service mode - headset changers local name and enters discoverable 
	mode
*/
void enterServiceMode( void ) ;
 
/****************************************************************************
DESCRIPTION
  	handle a local bdaddr request and continue to enter service mode
*/
void DutHandleLocalAddr(CL_DM_LOCAL_BD_ADDR_CFM_T *cfm) ;

#endif /* _HEADSET_DUT_H_ */
