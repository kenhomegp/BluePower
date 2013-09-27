/****************************************************************************

FILE NAME
    headset_pio.c
    
DESCRIPTION


*/

#include "headset_pio.h"

#include "headset_private.h"
#include "headset_configmanager.h"

#include <pio.h>
#include <psu.h>

#ifdef DEBUG_PIO
#define PIO_DEBUG(x)  DEBUG (x)
#else
#define PIO_DEBUG(x) 
#endif

/****************************************************************************
NAME	
	PioSetPio

DESCRIPTION
    Fn to change a PIO
    
RETURNS
	void
*/
void PioSetPio ( uint16 pPIO , bool pOnOrOff  ) 
{
    uint16 lPinVals = 0 ;
    uint16 lWhichPin  = (1<< pPIO) ;
    
    PIO_DEBUG(("PIO : set[%d][%d] [%x]\n",pPIO, pOnOrOff ,lWhichPin)) ;
    
    if ( pOnOrOff )    
    {
        lPinVals = lWhichPin  ;
    }
    else
    {
        lPinVals = 0x0000;/*clr the corresponding bit*/
    }
    
    PIO_DEBUG(("PIO : set[%x][%x]\n",lWhichPin , lPinVals)) ;
      	/*(mask,bits) setting bit to a '1' sets the corresponding port as an output*/
    PioSetDir32( lWhichPin , lWhichPin );   
    	/*set the value of the pin*/         
    PioSet32 ( lWhichPin , lPinVals ) ;     
}

/****************************************************************************
NAME	
	PioSetPowerPin

DESCRIPTION
    controls the internal regulators to latch / remove the power on state
    
RETURNS
	void
*/
void PioSetPowerPin ( bool enable ) 
{
    if ( theHeadset.features.PowerOnSMPS )
    {
        	/*this is the power regulator PIO*/ 
        PsuConfigure(PSU_SMPS0, PSU_ENABLE, (bool)enable);
        PIO_DEBUG(("PIO : PowerOnSMPS\n")) ;      
    }

    if  ( theHeadset.features.PowerOnLDO)
    {
        PioSetMicrophoneBias( (bool)enable ) ;
        PIO_DEBUG(("PIO : PowerOn LDO\n")) ;
    }    

}



