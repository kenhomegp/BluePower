#include "headset_private.h"
#include "av_temperature_monitor.h"
#include "headset_pio.h" 
#include "headset_led_manager.h"

#include <pio.h>
#include <psu.h>

#include <string.h>

#include "headset_powermanager.h"
#include "headset_statemanager.h"
#include "headset_dut.h"
#include "headset_leds.h"
#include "headset_events.h"

#include <sink.h>

#ifdef DEBUG_TempMonitor
#define TEMPM_DEBUG(x) DEBUG(x)
#else
#define TEMPM_DEBUG(x)
#endif

/* BHC302 LED */
#define LED_WHITE		9
#define LED_BAT_RED		10

/* BHC302 */
#define PHONE_DET		4

#define VREG_PIN1    (24)
#define CHG_PIN1     (25)

#define Time_interval	1000

/*the mask values for the charger pins*/
#define VREG_PIN_MASK1 ((uint32)1 << VREG_PIN1)
#define CHG_PIN_MASK1 ((uint32)1 << CHG_PIN1)

/*the mask values for the charger pin events*/
#define CHARGER_VREG_VALUE1 ( (uint32)((PsuGetVregEn()) ? VREG_PIN_MASK1:0 ) )

static void appTempMVoltageHandler(TempMTaskData *tempM, uint16 voltage)
{
#ifdef BHC612
	POWER_BATTERY_LEVEL_IND_T BHC612_batt_volt;
#endif

    if (tempM->initial_reading)
    {
        /* Clear initial reading flag */
    	tempM->initial_reading = FALSE;
        /* Re-schedule periodic readings */
		
        /* BatteryInit(&tempM->tempm_state, &tempM->task, TEMP_M_INPUT, TEMP_M_PERIOD); */

		#if 0
		BatteryInit(&tempM->tempm_state, &tempM->task, TEMP_M_INPUT, Time_interval);
		#else
		BatteryInit(&tempM->tempm_state, &tempM->task, TEMP_M_INPUT, 2000);
		#endif
		
		/*TEMPM_DEBUG(("Re-schedule periodic readings\n"));*/
    }
	
#ifdef BHC612
	if(tempM->RunMode != 0)
	{
		#if 0
		PowerGetVoltage(&BHC612_batt_volt);
		TEMPM_DEBUG(("Batt_V = %d\n",BHC612_batt_volt.voltage));
		#endif
	}
	else
	{	
		if(ChargerIsChargerConnected())
		{
			if(theHeadset.BHC612_PowerInitComplete == 1)
			{
				if(ChargerStatus() == STANDBY)
				{
					TEMPM_DEBUG(("Charger standby!!\n"));
				}
				else
				{
					PowerGetVoltage(&BHC612_batt_volt);
					/*
					TEMPM_DEBUG(("Batt_V = %d\n",BHC612_batt_volt.voltage));
					*/
				}
					
				if(theHeadset.batt_level >= Battery_ChargeStop)
				{
					/*PowerGetVoltage(&BHC612_batt_volt);*/
					/*TEMPM_DEBUG(("Batt_V = %d\n",BHC612_batt_volt.voltage));*/

					#ifdef NewChargeMMI
					if((PsuGetVregEn() != 0) && (ChargerStatus() == STANDBY))	
					#else
					if(ChargerStatus() == STANDBY)
					#endif	
					{
						/*TEMPM_DEBUG(("Charger: standby,VBatt = %d\n",BHC612_batt_volt.voltage));*/
						theHeadset.BHC612_Chargefull =1;	
						PioSetPio(battery_low_io , PowerBlu_Disconnect);
						theHeadset.BHC612_BatteryLowState = PowerBlu_Disconnect;
						TEMPM_DEBUG(("### Battery low : High , Charge stop!"));
					}
				}
				else if(theHeadset.batt_level < Battery_Recharge)
				{
					if(theHeadset.BHC612_Chargefull)
						theHeadset.BHC612_Chargefull = 0;
				}
			}
		}

		if(theHeadset.BHC612_DockMonitor == 1)
		{
			#if 0	/*#ifdef NewPSBlueLEDx*/
			checkPowerBluConn();

			if(ChargerIsChargerConnected() == FALSE)
			{			
				/*TEMPM_DEBUG(("@"));*/
				if(theHeadset.BHC612_PowerBluConn == BlueLDE_Detect_Disconnect)
				{
					if(theHeadset.theLEDTask->gLEDSEnabled == FALSE)
					{
						theHeadset.theLEDTask->gLEDSEnabled = TRUE;
					}
				
					theHeadset.BHC612_DockMonitor = 0;
					PioSetPio(battery_low_io , PowerBlu_Connect);
					theHeadset.BHC612_Chargefull =0;	
					TEMPM_DEBUG(("Clear chargefull flag.\n"));

					theHeadset.BHC612_UNDOCKMMI = 1;
					theHeadset.BHC612_DOCKMMI = 1;
					theHeadset.DockLED = 0;
					LEDManagerIndicateState(stateManagerGetState());
				}
				else
				{
					#ifdef FullChargeLED		
						if((theHeadset.batt_level == POWER_BATT_LEVEL3) && (stateManagerGetState() == headsetConnectable))
						{
							if((theHeadset.BHC612_DOCKMMI == 0) && (theHeadset.BHC612_UNDOCKMMI == 1) && (theHeadset.DockLED == 0))
							{
								PowerGetVoltage(&BHC612_batt_volt);
								TEMPM_DEBUG(("*Batt_V = %d\n",BHC612_batt_volt.voltage));

								if((theHeadset.BHC612_TEMP > 0) && (theHeadset.BHC612_TEMP < 3))
								{	
									if(theHeadset.BHC612_TEMP == 2)
									{																						
										PioSetLedPin( BHC612_LED_0 , LED_ON );
										PioSetLedPin( 15 , LED_ON );
										PioSetPio(14 , LED_ON);
										PioSetPio(15 , LED_ON);
										/*TEMPM_DEBUG(("4 LED ON!"));*/
									}
									else
									{																
										PioSetLedPin( BHC612_LED_0 , LED_OFF );
										PioSetLedPin( 15 , LED_OFF );
										PioSetPio(14 , LED_OFF);
										PioSetPio(15 , LED_OFF);
										/*TEMPM_DEBUG(("4 LED OFF!"));*/
									}
									
									theHeadset.BHC612_TEMP--;
								}
							}
							else
							{
								/*
								TEMPM_DEBUG(("%d,%d,%d\n",theHeadset.BHC612_DOCKMMI,theHeadset.BHC612_UNDOCKMMI,theHeadset.DockLED));
								*/
								theHeadset.BHC612_DOCKMMI = 0;
								theHeadset.BHC612_UNDOCKMMI = 1;
								theHeadset.DockLED = 0;
							}
						}
						else
						{
							PowerGetVoltage(&BHC612_batt_volt);
							TEMPM_DEBUG(("$Batt_V = %d\n",BHC612_batt_volt.voltage));
							/*TEMPM_DEBUG(("$"));*/
						}
					#endif
				}
			}
			else
			{
				/*TEMPM_DEBUG(("#"));*/
			}
			#endif
		}

		/*Chgrging LED for 30 sec*/
		if(theHeadset.BHC612_DOCKMMI == 1 && theHeadset.BHC612_UNDOCKMMI == 0)
		{
			/*if(ChargerIsChargerConnected() == FALSE)*/
			if(ChargerIsChargerConnected() == FALSE && theHeadset.BHC612_DockMonitor == 0)
			{
				theHeadset.BHC612_UNDOCKMMI = 0;
				theHeadset.BHC612_DOCKMMI = 0;
				theHeadset.DockLED = 0;
				LEDManagerIndicateState(stateManagerGetState());
			}
		}

		#if 1
		if((theHeadset.BHC612_StopChargingLED == 1) && (theHeadset.BHC612_DOCKMMI == 0) && (theHeadset.BHC612_UNDOCKMMI == 0))
		{
			theHeadset.BHC612_StopChargingLED = 0;
			LEDManagerIndicateState(stateManagerGetState());
			TEMPM_DEBUG(("###Reset state LED!\n"));
		}
		#endif

		 #ifdef New_MMI
		 if(theHeadset.BHC612_PowerInitComplete)
		 {
			 #ifdef HW_DVT2
				 #ifdef NewChargeMMI
				 	/*
				 	PowerGetVoltage(&BHC612_batt_volt);
					TEMPM_DEBUG(("*Batt_V = %d\n",BHC612_batt_volt.voltage));
					*/

				 	#if 1
				 	if(theHeadset.batt_level < Battery_Recharge)
					{
						PioSetPio(battery_low_io , PowerBlu_Connect);
						theHeadset.BHC612_BatteryLowState = PowerBlu_Connect;
					}
					else if(theHeadset.batt_level >= Battery_ChargeStop)
					{
						/*(if(PsuGetVregEn())*/
						/*if((PsuGetVregEn() == TRUE) && (ChargerIsChargerConnected() == TRUE))*/

						#if 0
						if((PsuGetVregEn() == TRUE) && (ChargerStatus() == FAST_CHARGE))
						{
							/*
							TEMPM_DEBUG(("*** Batt level 3\n"));
							PioSetPio(battery_low_io , PowerBlu_Disconnect);
							*/
							PioSetPio(battery_low_io , PowerBlu_Connect);
						}
						else
						{
							PioSetPio(battery_low_io , PowerBlu_Connect);
						}
						#endif
					}
					#endif
			 	 #endif
			 	 #if 0	/*#ifdef NewChargeMMIx*/
				 if(theHeadset.BHC612_BTCONNECT == 0 && theHeadset.BHC612_BTINCCALL == 0)/*headset state = headsetConnectable,headsetLimbo*/
				 {	
					 	if((ChargerIsChargerConnected() == FALSE) && (theHeadset.BHC612_DockMonitor == 1))
					 	{
							if(theHeadset.batt_level != POWER_BATT_LEVEL3)
								PioSetPio(battery_low_io , PowerBlu_Connect);
					 	}
				 }
				 else
				 {
						if(theHeadset.batt_level != POWER_BATT_LEVEL3)
							PioSetPio(battery_low_io , PowerBlu_Connect);
				 }

				 if(ChargerStatus() == DISABLED_ERROR)
				 {
				 	TEMPM_DEBUG(("Charger : Disable_Error\n"));
					PioSetPio(battery_low_io , PowerBlu_Disconnect);
				 }
				 #endif

			 #else		 
				PioSetPio(battery_low_io , PowerBlu_Connect);
			 	theHeadset.BHC612_BatteryLowState = PowerBlu_Connect;
			 #endif
		 }

		 if (stateManagerGetState() == headsetConnected)
		 {
			 if(theHeadset.BHC612_BTCONNECT == 1 && theHeadset.theLEDTask->gLEDSEnabled == TRUE)
			 {
				/*if(theHeadset.BHC612_DOCKMMI == 1)*/
				if(theHeadset.BHC612_DOCKMMI == 1 && theHeadset.BHC612_UNDOCKMMI == 0)
				{
					/*#ifdef NewChargeMMI*/
					#if 0
						tempM->Counter++;
						#ifdef PSBlueLED
						if(theHeadset.BHC612_BLUELED)
						{
							if(tempM->Counter == 2)
							{
								theHeadset.BHC612_BLUELED = 0;														
								PioSetPio(PS_blue_led, PS_blue_led_on);
							}
						}
						else
						{
							if(tempM->Counter == 4)
							{
								theHeadset.BHC612_BLUELED = 1;
								PioSetPio(PS_blue_led, PS_blue_led_off);
								tempM->Counter = 0;
							}
						}
						#endif
					#else						
						#ifdef PSBlueLED
							if(theHeadset.BHC612_BLUELED)
							{
								theHeadset.BHC612_BLUELED = 0;	
														
								PioSetPio(PS_blue_led, PS_blue_led_on);
							}
							else
							{
								theHeadset.BHC612_BLUELED = 1;
								
								PioSetPio(PS_blue_led, PS_blue_led_off);
							}
						#endif	
					#endif
				}			
				else
				{
					#ifdef PSBlueLED
						PioSetPio(PS_blue_led, PS_blue_led_off);/*PowerBlu LED OFF*/
					#endif
				}
			 }
		 }
		 else
		 {
			if(stateManagerGetState() != headsetIncomingCallEstablish)
			{
				#ifdef PSBlueLED
				PioSetPio(PS_blue_led, PS_blue_led_off);
				#endif
			}
		 }

		 if (stateManagerGetState() == headsetLimbo )
		 {		
			checkPowerBluConn();

			#ifdef NewPSBlueLED
			if(ChargerIsChargerConnected() == FALSE)
			#else
			if((theHeadset.BHC612_PowerBluConn == BlueLDE_Detect_Disconnect) || (ChargerIsChargerConnected() == FALSE))
			#endif
			{
				if(theHeadset.PowerOffIsEnabled == false)
					theHeadset.PowerOffIsEnabled = true;

				#if 0
				PioSetPio(PS_blue_led, PS_blue_led_on);
				PioSetPio(battery_low_io , PowerBlu_Connect);
				#endif
										
				MessageSend ( &theHeadset.task , EventPowerOff , 0) ;
			}	
		 }
		 #endif
	}
#endif
}


static void appTempMHandleMessage(Task task, MessageId id, Message message)
{
	TempMTaskData *theTempM = (TempMTaskData *)task;
    
    switch (id)
    {
	   case BATTERY_READING_MESSAGE:
	       appTempMVoltageHandler(theTempM, *(uint32 *)message);
		   /*TEMPM_DEBUG(("BATTERY_READING_MESSAGE..\n"));*/
		   break;
	   default:
           break;
    }
}

void appTempMInit(TempMTaskData *theTempM , uint16 Runmode)
{
#ifdef DEBUG_TempMonitorx
	TEMPM_DEBUG(("appTempMInit,Runmode = %d\n",Runmode));
#endif
    /* Set up task handler */
	theTempM->task.handler = appTempMHandleMessage;

    /* Initialise state variables */
	theTempM->initial_reading = TRUE;
    theTempM->charger_configured = FALSE;
	theTempM->RunMode = Runmode;
	theTempM->Counter = 0;

    /* Start immediate battery reading */
    BatteryInit(&theTempM->tempm_state, &theTempM->task, TEMP_M_INPUT, 0);
}

