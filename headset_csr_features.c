/****************************************************************************

FILE NAME
    headset_csr_features.c

DESCRIPTION
    handles the csr to csr features 

NOTES

*/

#include "headset_debug.h"
#include "headset_private.h"
#include "headset_events.h"
#include "headset_csr_features.h"

#include <hfp.h>

#include "headset_statemanager.h"


#ifdef DEBUG_CSR2CSR
    #define CSR2CSR_DEBUG(x) DEBUG(x)
#else
    #define CSR2CSR_DEBUG(x)
#endif     

void csr2csrHandleTxtInd (void ) 
{
    CSR2CSR_DEBUG(("CSR2CSR TXT IND\n")) ;
}

void csr2csrHandleSmsInd (void ) 
{
    CSR2CSR_DEBUG(("CSR2CSR SMS IND\n")) ;
}

void csr2csrHandleSmsCfm(void ) 
{
    CSR2CSR_DEBUG(("CSR2CSR SMS CFM\n")) ;
}

void csr2csrHandleAgBatteryRequestInd ( void ) 
{
    CSR2CSR_DEBUG(("CSR2CSR BATTERY REQUEST IND\n")) ;
	MessageSend (&theHeadset.task , EventBatteryLevelRequest , 0 );
}

void csr2csrHandleAgBatteryRequestRes(headsetEvents_t gas_gauge)
{
	#ifdef New_MMI
		uint16 idx = gas_gauge - EventGasGauge0;
		if((stateManagerGetState() == headsetConnected) && (theHeadset.BHC612_LinkLoss == false) )
		{
			switch(idx)
			{
				#ifdef iOSBatteryMeter
				case 0xff:
				{					
					/*Enable custom AT commands from a headset */
					HfpCsrFeaturesBatteryLevelRequest(hfp_primary_link, idx);
					
					if(theHeadset.MultipointEnable)
						HfpCsrFeaturesBatteryLevelRequest(hfp_secondary_link, idx);	

					CSR2CSR_DEBUG(("IniBattIcon...,"));
				}
				break;
				#ifdef ChineseTTS
				case 0x5A:
				{
					HfpCsrFeaturesBatteryLevelRequest(hfp_primary_link, idx);
				}
				break;
				#endif
				case 0x1A:
				case 0x41:
				case 0x42:
				case 0x43:
				case 0x44:
				{
					/*Reports a headset state change*/
					HfpCsrFeaturesBatteryLevelRequest(hfp_primary_link, idx);
					
					if(theHeadset.MultipointEnable)
						HfpCsrFeaturesBatteryLevelRequest(hfp_secondary_link, idx); 
									
					CSR2CSR_DEBUG(("Batt_Level\n"));
				}
				break;
				#endif
				default:
				break;
			}

			#if 0
			if(idx == 0xff)
			{	/*Enable custom AT commands from a headset */
				HfpCsrFeaturesBatteryLevelRequest(hfp_primary_link, idx);
				if(theHeadset.MultipointEnable)
					HfpCsrFeaturesBatteryLevelRequest(hfp_secondary_link, idx);	
				
				CSR2CSR_DEBUG(("Initial...,"));
			}
			else
			{	/*Reports a headset state change*/
				HfpCsrFeaturesBatteryLevelRequest(hfp_primary_link, idx);
				if(theHeadset.MultipointEnable)
					HfpCsrFeaturesBatteryLevelRequest(hfp_secondary_link, idx);	
				
				CSR2CSR_DEBUG(("Batt_Level\n"));
			}
			#endif
		}
	#endif
	
	#if 0
		const unsigned char csr2csr_battery_level_indications[4] = {2,4,7,9};
	    CSR2CSR_DEBUG(("CSR2CSR BATTERY REQUEST RES %d", idx)) ;
	    if(idx < sizeof(csr2csr_battery_level_indications))
	    {
	        uint16 batt_level = csr2csr_battery_level_indications[idx];
	        CSR2CSR_DEBUG((" [%d]", batt_level)) ;
	        /* Attempt to send indication to both AGs (HFP will block if unsupported) */
	        HfpCsrFeaturesBatteryLevelRequest(hfp_primary_link, batt_level);
	        HfpCsrFeaturesBatteryLevelRequest(hfp_secondary_link, batt_level);
	    }
	    CSR2CSR_DEBUG(("\n")) ;
	#endif

	#if 0
		uint16 idx = gas_gauge - EventGasGauge0;
		if((stateManagerGetState() == headsetConnected) && (theHeadset.BHC612_LinkLoss == false))
		{
			if(idx == 0xff)
			{	
				HfpCsrFeaturesBatteryLevelRequest(hfp_primary_link, idx);
								
				CSR2CSR_DEBUG(("AT+CSMP\n"));
			}
			/*
			else
			{
				HfpCsrFeaturesBatteryLevelRequest(hfp_primary_link, idx);
				CSR2CSR_DEBUG(("AT+CSMP\n"));
			}
			*/
		}
	#endif
}
