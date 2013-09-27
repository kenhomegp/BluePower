/****************************************************************************

FILE NAME
    headset_dut.c

DESCRIPTION
	Place the headset into Device Under Test (DUT) mode    

NOTES
		

*/


/****************************************************************************
    Header files
*/
#include "headset_private.h"
#include "headset_dut.h"

#include <pio.h>
#include <test.h>


#include <ps.h>
#include <string.h>
#include "headset_configmanager.h"

#include "headset_pio.h" 

#include "headset_statemanager.h"

#define TX_START_TEST_MODE_LO_FREQ  (2441)
#define TX_START_TEST_MODE_LEVEL    (63)
#define TX_START_TEST_MODE_MOD_FREQ (0)

#ifdef BHC612
/*Check Headset&Powerblu Connect/Disconnect*/
void checkPowerBluConn(void)
{
#ifdef NewPSBlueLEDx
	uint8 i = 0;
	uint8 j = 0;
	uint8 delay = 0;

	PioSetPio(PS_blue_led, PS_blue_led_on);

	for(delay = 0 ; delay < 10 ; delay++)
		;

	for(i = 0; i < 5; i++)
	{
		/* Set Powerble LED pin as input state */
		PioSetDir32(1 << PS_blue_led, 0);

		if(PioGet32() & (1 << PS_blue_led))
		{
			/*theHeadset.BHC612_PowerBluConn = 1;*/
			j++;
		}

		for(delay = 0 ; delay < 10 ; delay++)
			;
	}

	if(j >= 3)
	{
		if(theHeadset.BHC612_PowerBluConn == BlueLDE_Detect_Disconnect)
			theHeadset.BHC612_PowerBluConn = BlueLDE_Detect_Connect;/*PS is connected*/
	}
	else
	{
		if(theHeadset.BHC612_PowerBluConn == BlueLDE_Detect_Connect)
			theHeadset.BHC612_PowerBluConn = BlueLDE_Detect_Disconnect;/*PS is disconnected*/
	}

	PioSetPio(PS_blue_led, PS_blue_led_off);
#endif
}

#endif


/****************************************************************************
DESCRIPTION
  	This function is called to place the headset into DUT mode
*/
void enterDutMode(void)
{
	ConnectionEnterDutMode();
}

/****************************************************************************
DESCRIPTION
  	Entera continuous transmit test mode
*/
void enterTxContinuousTestMode ( void ) 
{
    TestTxStart (TX_START_TEST_MODE_LO_FREQ, 
                 TX_START_TEST_MODE_LEVEL, 
                 TX_START_TEST_MODE_MOD_FREQ) ;
}

/****************************************************************************
DESCRIPTION
  	This function is called to determine if the headset should enter DUT mode.
*/
bool checkForDUTModeEntry( void )
{
/*#ifndef BHC612*/
	if(theHeadset.conf->input_PIO.dut_pio != INPUT_PIO_UNASSIGNED)
	{
		/* Enable the DUT mode PIO line */
		PioSetDir32(1 << theHeadset.conf->input_PIO.dut_pio, 0);

		/* If it's high, enter DUT mode */
		if(PioGet32() & (1 << theHeadset.conf->input_PIO.dut_pio))
		{
			enterDutMode();
			return TRUE;
		}
	}	
/*#endif*/

	return FALSE;
}



/****************************************************************************
DESCRIPTION
  	Enter service mode - headset changers local name and enters discoverable 
	mode
*/
void enterServiceMode( void )
{
        /* Reset pair devices list */
    configManagerReset();
            
        /* Entered service mode */
    MessageSend(&theHeadset.task, EventServiceModeEntered, 0);

        /* Power on immediately */
    MessageSend(&theHeadset.task, EventPowerOn, 0);   
		/* Ensure pairing never times out */
	theHeadset.conf->timeouts.PairModeTimeout_s = 0;		
	theHeadset.conf->timeouts.PairModeTimeoutIfPDL_s = 0;
	
    MessageSend(&theHeadset.task, EventEnterPairing, 0); 
            
    ConnectionReadLocalAddr(&theHeadset.task);
}

/****************************************************************************
DESCRIPTION
  	convert decimal to ascii
*/
static char hex(int hex_dig)
{
    if (hex_dig < 10)
        return '0' + hex_dig;
    else
        return 'A' + hex_dig - 10;
}

/****************************************************************************
DESCRIPTION
  	handle a local bdaddr request and continue to enter service mode
*/
void DutHandleLocalAddr(CL_DM_LOCAL_BD_ADDR_CFM_T *cfm)
{
	uint16 buffer[] = { 0,0,0,0 };
   	 uint16 i = 0 ;
 
	char new_name[32];
	PsRetrieve(PSKEY_SW_VERSION_NUMBER, buffer, sizeof(buffer) );
	
    new_name[i++] = hex((cfm->bd_addr.nap & 0xF000) >> 12);
    new_name[i++] = hex((cfm->bd_addr.nap & 0x0F00) >> 8);
    new_name[i++] = hex((cfm->bd_addr.nap & 0x00F0) >> 4);
    new_name[i++] = hex((cfm->bd_addr.nap & 0x000F) >> 0);
    
    new_name[i++] = hex((cfm->bd_addr.uap & 0x00F0) >> 4);
    new_name[i++] = hex((cfm->bd_addr.uap & 0x000F) >> 0);
    
    new_name[i++] = hex((cfm->bd_addr.lap & 0xF00000) >> 20);
    new_name[i++] = hex((cfm->bd_addr.lap & 0x0F0000) >> 16);
    new_name[i++] = hex((cfm->bd_addr.lap & 0x00F000) >> 12);
    new_name[i++] = hex((cfm->bd_addr.lap & 0x000F00) >> 8);
    new_name[i++] = hex((cfm->bd_addr.lap & 0x0000F0) >> 4);
    new_name[i++] = hex((cfm->bd_addr.lap & 0x00000F) >> 0);

    new_name[i++] = ' ' ;	
             
	new_name[i++] = hex((buffer[0] & 0xF000) >> 12) ;	
	new_name[i++] = hex((buffer[0] & 0x0F00) >> 8 ) ;	
	new_name[i++] = hex((buffer[0] & 0x00F0) >> 4 ) ;	
	new_name[i++] = hex((buffer[0] & 0x000F) >> 0 ) ;	
		     
	new_name[i++] = hex((buffer[1] & 0xF000) >> 12) ;	
	new_name[i++] = hex((buffer[1] & 0x0F00) >> 8 ) ;	
	new_name[i++] = ' ' ;	
	new_name[i++] = hex((buffer[1] & 0x00F0) >> 4 ) ;	
	new_name[i++] = hex((buffer[1] & 0x000F) >> 0 ) ;	
		     
	new_name[i++] = hex((buffer[2] & 0xF000) >> 12) ;	
	new_name[i++] = hex((buffer[2] & 0x0F00) >> 8 ) ;	
	new_name[i++] = hex((buffer[2] & 0x00F0) >> 4 ) ;	
	new_name[i++] = hex((buffer[2] & 0x000F) >> 0 ) ;	
	  
	/* Some customers do not use the final word in their version number
	   so omit it if not present */
	if(buffer[3] != 0)
	{
		new_name[i++] = ' ' ;	
	
		new_name[i++] = hex((buffer[3] & 0xF000) >> 12) ;	
		new_name[i++] = hex((buffer[3] & 0x0F00) >> 8 ) ;	
		new_name[i++] = hex((buffer[3] & 0x00F0) >> 4 ) ;	
		new_name[i++] = hex((buffer[3] & 0x000F) >> 0 ) ;	
	}
	
    /*terminate the string*/
    new_name[i] = 0;
   
 	ConnectionChangeLocalName(i, (uint8 *)new_name);
}
