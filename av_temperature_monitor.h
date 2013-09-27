#ifndef AV_TEMPERATURE_MONITOR_H_
#define AV_TEMPERATURE_MONITOR_H_


#include <message.h>
#include <pio.h>
#include <battery.h>

#ifndef TEMP_M_INPUT
#define TEMP_M_INPUT	 AIO0
#endif

#ifndef TEMP_M_PERIOD
#ifdef TEMPM_DEBUG_ENABLE
    #define TEMP_M_PERIOD	(D_SEC(10))
#else
    #define TEMP_M_PERIOD	(D_SEC(60))
#endif
#endif

#define TEMPERATURE_DIVIDER_R1      (470)   
#define TEMPERATURE_DIVIDER_R2      (200)  


/*Temperature to voltage. -10 C*/
#define TEMPM_VOLTAGE_M10D		1230


/*Temperature to voltage. 0 C*/
#define TEMPM_VOLTAGE_P00D		1110

/*Temperature to voltage. 10 C*/
#define TEMPM_VOLTAGE_P10D		970

/*Temperature to voltage. 45 C*/
#define TEMPM_VOLTAGE_P45D		490

/*Temperature to voltage. 60 C*/
#define TEMPM_VOLTAGE_P60D		340

/*Temperature to voltage. 75 C*/
#define TEMPM_VOLTAGE_P75D		235



#define TEPM_IS_LESS_THAN_M10(__volt)	((__volt) > TEMPM_VOLTAGE_M10D)


#define TEMPM_IS_M10_TO_P00(__volt)	(((__volt) <= TEMPM_VOLTAGE_M10D) && 	\
									((__volt) > TEMPM_VOLTAGE_P00D))


#define TEMPM_IS_P00_TO_P10(__volt)	(((__volt) <= TEMPM_VOLTAGE_P00D) && 	\
									((__volt) > TEMPM_VOLTAGE_P10D))


#define TEMPM_IS_P10_TO_P45(__volt)	(((__volt) <= TEMPM_VOLTAGE_P10D) && 	\
									((__volt) > TEMPM_VOLTAGE_P45D))


#define TEMPM_IS_P45_TO_P60(__volt)	(((__volt) <= TEMPM_VOLTAGE_P45D) && 	\
									((__volt) > TEMPM_VOLTAGE_P60D))

#define TEMPM_IS_P60_TO_P75(__volt)	(((__volt) <= TEMPM_VOLTAGE_P60D) && 	\
									((__volt) > TEMPM_VOLTAGE_P75D))

#define TEMPM_IS_OVER_P75(__volt)	((__volt) <= TEMPM_VOLTAGE_P75D)


#define appTempMSetStateChargerConfigured(__state)     appGetApp()->theTemperature.charger_configured = (__state)


/*! Temperature task structure */
typedef struct
{
    TaskData 		task;            /*!< Temperature monitor task. */
    BatteryState 	tempm_state;     /*!< Temterature reading */
    unsigned	    initial_reading:1;
    unsigned        charger_configured:1;	
    unsigned 		RunMode:2;	
    unsigned 		Counter:12;	
} TempMTaskData;


extern void appTempMInit(TempMTaskData *theTempM , uint16 Runmode);

#endif /* AV_TEMPERATURE_MONITOR_H_ */
