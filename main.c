/****************************************************************************

FILE NAME
    main.c        

DESCRIPTION
    This is main file for the headset application software for BC4-Headset

NOTES

*/

/****************************************************************************
    Header files
*/
#include "headset_private.h"
#include "headset_init.h"
#include "headset_auth.h"
#include "headset_scan.h"
#include "headset_slc.h" 
#include "headset_dut.h" 
#include "headset_pio.h" 
#include "headset_multipoint.h" 
#include "headset_led_manager.h"
#include "headset_buttonmanager.h"
#include "headset_configmanager.h"
#include "headset_events.h"
#include "headset_statemanager.h"
#include "headset_states.h"
#include "headset_powermanager.h"
#include "headset_callmanager.h"
#include "headset_energy_filter.h"
#include "headset_csr_features.h"

#ifdef ENABLE_PBAP
#include "headset_pbap.h"
#endif

#ifdef ENABLE_AVRCP
#include "headset_avrcp.h"
#endif

#include "headset_volume.h"
#include "headset_tones.h"
#include "headset_tts.h" 
#include "headset_energy_filter.h"

#include "headset_audio.h"
#include "vm.h"

#ifdef TEST_HARNESS
#include "test_headset.h"
#include "vm2host_connection.h"
#include "vm2host_hfp.h"
#endif

#include <bdaddr.h>
#include <connection.h>
#include <panic.h>
#include <ps.h>
#include <pio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stream.h>
#include <codec.h>
#include <boot.h>
#include <string.h>
#include <audio.h>
#include <sink.h>
#include <kalimba_standard_messages.h>
#include <audio_plugin_if.h>

#ifdef BlueCom
#include <source.h>
#include <ctype.h>
#include <led.h>
#include <test.h>
#endif

#ifdef CVC_PRODTEST
#include <kalimba.h>
#include <file.h>
#include <string.h>
#include "headset_config.h"
#endif

#ifdef Rubidium
#include <rubidium_text_to_speech_plugin.h> 
#include <kalimba.h>
/*char			TTS_text[20];*/
char			TTS_text[50];
extern uint16	Language;
extern const 	TaskData rubidium_tts_asr; 
extern /*E_RUNNING_OPTIONS*/uint16   Kalimba_mode;		/* current mode that Kalimba is running in 	*/
extern /*E_NODES*/uint16 			ASR_active_model;	/* current active menu node (.mdl file) for ASR  */    
extern bool TipCounter;
void CheckBattery_function(void);
void AnswerCall_function(void);
#ifdef RubiTTSTerminate
void AnswerCall_function1(void);
#endif
void IgnoreCall_function(void);
void Redial_function(void);
void Callback_function(void);
void PairNewDevice_function(void);
TaskData task1;
#endif

#if defined(APPINCLSOUNDCLEAR) && defined(APPENABLESPPRAPID)
	#ifdef ATI_SPP
	#include "soundclearSpp.h"
	#endif
#endif

#ifdef DEBUG_MAIN
#define MAIN_DEBUG(x) DEBUG(x)
    #define TRUE_OR_FALSE(x)  ((x) ? 'T':'F')   
#else
    #define MAIN_DEBUG(x) 
#endif

#ifdef BHC612	
	#include "headset_leds.h"
	#include <psu.h>

	POWER_BATTERY_LEVEL_IND_T BHC612_volt_level;
	
	#ifdef iOSBatteryMeter
	static uint8 BatteryCounter = 0;
	#endif
	
	#ifdef My_DEBUG
	#define MyDEBUG(x) {printf x;}
	#else
	#define MyDEBUG(x)
	#endif
	
	#ifdef PowerBlueMMI
	#define PowerBlueMMI_DEBUG(x) {printf x;}
	#else
	#define PowerBlueMMI_DEBUG(x)
	#endif

	#ifdef DEBUG_MPMAIN
		#define MP_MAIN_DEBUG(x) DEBUG(x)
	#else
    	#define MP_MAIN_DEBUG(x) 
	#endif

	#ifdef  DEBUG_RUBI_TTSASR
		#define RUBI_DEBUG(x) DEBUG(x)
	#else
		#define RUBI_DEBUG(x)
	#endif

	void iOSBatteryLevelIndication(void);

	#ifdef PSFWVer
	void Write_SW_Version(void);
	#endif
	
	void BHC612_Init(void);
#endif

/* Single instance of the Headset state */
hsTaskData theHeadset;

static void handleHFPStatusCFM ( hfp_lib_status pStatus ) ;

#ifdef CVC_PRODTEST
typedef struct
{
    uint16 id;
    uint16 a;
    uint16 b;
    uint16 c;
    uint16 d;
} DSP_REGISTER_T;
#endif

static void headsetInitCodecTask( void ) ;
/*===============================================*/
#ifdef BHC612
void BHC612_Init()
{
	theHeadset.batt_level = POWER_BATT_LEVEL3;
	theHeadset.BBHC612_PhoneDisconnect = 0;
	theHeadset.BHC612_Rubi_Lan_Changed = 0;
	theHeadset.BHC612_CallEnded = 0;
	theHeadset.BHC612_DockMonitor = 0;
	theHeadset.BHC612_UNDOCKMMI = 1;
	theHeadset.BHC612_PSConnected = 0;
	theHeadset.BHC612_BTIDLE = 0;
	theHeadset.BHC612_BTCONNECT = 0;
	theHeadset.BHC612_BTINCCALL = 0;
	theHeadset.BHC612_BTOUTCALL = 0;
	theHeadset.BHC612_DOCKMMI = 0;
	theHeadset.BHC612_BLUELED = 0;
	theHeadset.BHC612_Chargefull = 0;
	theHeadset.BHC612_PairSuccess = 0;
	theHeadset.BHC612_PowerInitComplete = 0;
	theHeadset.BHC612_PAIRMODE = 0;
	theHeadset.BHC612_TEMP = 0;
	theHeadset.BHC612_StopChargingLED = 0;
	theHeadset.BHC612_VPLinkLoss = 0;
	theHeadset.DockLED = 0;
	theHeadset.BHC612_MPReconnect = false;
	theHeadset.BHC612_PhoneChanged = 0;
	theHeadset.PressCallButton = TRUE;
	theHeadset.TTS_ASR_Playing = false; 
	theHeadset.m_ChargerRealDisconnect = false;
	theHeadset.m_ChargerRealConnect = false;
	theHeadset.BHC612_LinkLoss = false;
	theHeadset.BHC612_BattMeterOK = false;
	theHeadset.BHC612_LinkLossReconnect = false;
	theHeadset.VoicePromptNotComplete = FALSE;
	theHeadset.BHC612_SelectLanguage = FALSE;
	
#ifdef Rubidium
	theHeadset.tts_language = 0;
	/*Language = AUDIO_TTS_LANGUAGE_AMERICAN_ENGLISH;*/
	/*Language = AUDIO_TTS_LANGUAGE_SPANISH_EUROPEAN;*/
	/*Language = AUDIO_TTS_LANGUAGE_FRENCH*/

	/*
	typedef enum AudioTTSLanguageTag
	{
		AUDIO_TTS_LANGUAGE_BRITISH_ENGLISH,
		AUDIO_TTS_LANGUAGE_AMERICAN_ENGLISH,
		AUDIO_TTS_LANGUAGE_FRENCH,
		AUDIO_TTS_LANGUAGE_ITALIAN,
		AUDIO_TTS_LANGUAGE_GERMAN,
		AUDIO_TTS_LANGUAGE_SPANISH_EUROPEAN,
		AUDIO_TTS_LANGUAGE_SPANISH_MEXICAN,
		AUDIO_TTS_LANGUAGE_PORTUGUESE_BRAZILIAN,
		AUDIO_TTS_LANGUAGE_CHINESE_MANDARIN,
		AUDIO_TTS_LANGUAGE_KOREAN,
		AUDIO_TTS_LANGUAGE_RUSSIAN,
		AUDIO_TTS_LANGUAGE_ARABIC,
		AUDIO_TTS_LANGUAGE_FINNISH,
		AUDIO_TTS_LANGUAGE_JAPANESE,
		AUDIO_TTS_LANGUAGE_POLISH
	}AUDIO_TTS_LANGUAGE_T ;	
	*/
#endif
	
#ifdef Rubi_TTS
	TipCounter = 1;
#endif

}

#ifdef PSFWVer
void Write_SW_Version()
{
	uint16 buffer[] = { 0,0,0,0 };
   	uint16 i = 0 ;
	 
	i = PsRetrieve(PSKEY_SW_VERSION_NUMBER, buffer, sizeof(buffer) );

	i = 0;
	i += Rel_yy << 12;
	i += BHC612_VMAPPVERMAJORREV << 8;
	i += BHC612_VMAPPVERMINORREV << 4;
	i += BHC612_TEST_Version;

	buffer[0] = i;
	i = 0;

	i += Rel_mm << 8;
	i += Rel_dd;
	buffer[1] = i;

	PsStore(PSKEY_SW_VERSION_NUMBER , buffer ,  sizeof(buffer));
}
#endif

void iOSBatteryLevelIndication()
{
	/*theHeadset.batt_level = POWER_BATT_LEVEL3;*//*debug*/
/*#ifdef iOSBatteryMeterx*/
#if 0
 		HfpAtCmdRequest(hfp_primary_link, "AT+XAPL=0000-0000-0000,7\r");
		if(theHeadset.MultipointEnable)
			HfpAtCmdRequest(hfp_secondary_link, "AT+XAPL=0000-0000-0000,7\r");
		
		switch(theHeadset.batt_level)
		{
			case POWER_BATT_LOW:				
				HfpAtCmdRequest(hfp_primary_link, "AT+IPHONEACCEV=2,1,0,2,0\r");
				if(theHeadset.MultipointEnable)
					HfpAtCmdRequest(hfp_secondary_link, "AT+IPHONEACCEV=2,1,0,2,0\r");
				MAIN_DEBUG(("###iOSBatteryLevelIndication,Batt_Low\n")) ;
				break;
			case POWER_BATT_LEVEL0:				
				HfpAtCmdRequest(hfp_primary_link, "AT+IPHONEACCEV=2,1,2,2,0\r");
				if(theHeadset.MultipointEnable)
					HfpAtCmdRequest(hfp_secondary_link, "AT+IPHONEACCEV=2,1,2,2,0\r");
				MAIN_DEBUG(("###iOSBatteryLevelIndication 0\n")) ;
				break;
			case POWER_BATT_LEVEL1:			
				HfpAtCmdRequest(hfp_primary_link, "AT+IPHONEACCEV=2,1,5,2,0\r");
				if(theHeadset.MultipointEnable)
					HfpAtCmdRequest(hfp_secondary_link, "AT+IPHONEACCEV=2,1,5,2,0\r");
				MAIN_DEBUG(("###iOSBatteryLevelIndication 1n")) ;
				break;
			case POWER_BATT_LEVEL2:
				HfpAtCmdRequest(hfp_primary_link, "AT+IPHONEACCEV=2,1,7,2,0\r");
				if(theHeadset.MultipointEnable)
					HfpAtCmdRequest(hfp_secondary_link, "AT+IPHONEACCEV=2,1,7,2,0\r");
				MAIN_DEBUG(("###iOSBatteryLevelIndication 2\n")) ;
				break;
			case POWER_BATT_LEVEL3:
				HfpAtCmdRequest(hfp_primary_link, "AT+IPHONEACCEV=2,1,9,2,0\r");
				if(theHeadset.MultipointEnable)
					HfpAtCmdRequest(hfp_secondary_link, "AT+IPHONEACCEV=2,1,9,2,0\r");
				MAIN_DEBUG(("###iOSBatteryLevelIndication 3\n")) ;
				break;
		}

		/*MAIN_DEBUG(("###iOSBatteryLevelIndication\n")) ;*/
#endif		
}
#endif

#ifdef BlueCom

#define UART_Debug
#define PIO_LED_TEST

/*#define MAX_GETINPUTLINE_LENGTH     50*/
#define MAX_GETINPUTLINE_LENGTH     20

enum FUNC_CALLS
{
    UNKNOWN = -1,
    NOTHING,
    LEDOFF,
    LEDON ,
    PIOSTATE, 
    QUERYSN ,
    READBT ,
    RESETPDL ,
    VER , 
    AUDIOLB ,
    STOP,
    DFU,
    WRITEBT ,
    STATUS ,
    HELP
};

enum
{
	UART_TESTMODE,
	UART_VBatt
};


static Source uart_source;

/* function definition */
/*void send(const char *s);*/
void sendchar(char s);
void GetInputLine(char *s);
void UpperCase(char *s);
int ReturnTokens(char *s, int *params);
void appUartInit(UARTTaskData *theTempM);
void ShowMenu(void);
void appUartHandler(Task task, MessageId id, Message message);

void send(const char *s)
{
    uint16 n = strlen(s);
    Sink uart = StreamUartSink();
    if(uart && SinkClaim(uart , n) != 0xFFFF)
    {
        memcpy(SinkMap(uart),s,n);
        (void)PanicZero(SinkFlush(uart , n));
    }
}

void sendchar(char s)
{
    Sink uart = StreamUartSink();
    if(uart && SinkClaim(uart, 1) != 0xFFFF)
    {
        memcpy(SinkMap(uart), &s, 1);
        (void) PanicZero(SinkFlush(uart, 1));
    }
}

#ifdef Testmode2
void ShowMenu(void)
{
/*
    1:LEDOFF,
    2:LEDON ,
    3:PIOSTATE, 
    4:QUERYSN ,
    5:READBT ,
    6:RESETPDL ,
    7:VER , 
    8:AUDIOLB
*/    
	send("[1] LED Off test\r\n");
	send("[2] LED On test\r\n");
	send("[3] Button test\r\n");
	send("[4] Get Serial Number\r\n");
	send("[5] Read BT Address\r\n");
	send("[6] Factory reset(Clear PDL)\r\n");
	send("[7] Get Firmware version\r\n");
	send("[8] Audio loop back test\r\n");	
}
#endif

/*static void GetInputLine(char *s)*/
void GetInputLine(char *s)
{
    int     uart_size   = 0;
    int     inp_str_len = 0;
    int     i           = 0;

    /* Get the parameter representing the source */
    Source uart_source = StreamUartSource();

    /* Routine checks for data coming into the UART and searches through the string for carriage return.
       Once found it replaces the character with ASCII 0 and returns the string.
       As data comes in, it is echoed back to the screen. */

    while (1)
    {
        /* wait for data */
        while (!(uart_size = SourceSize(uart_source)));

        /* If too much data has been sent send an error */
        if ((inp_str_len + uart_size) > MAX_GETINPUTLINE_LENGTH)
        {
            send("\n\rInput Error: Max line length exceeded.\n\r");             /* Send the error message */
            inp_str_len = 0;                                                    			/* Reset the pointer counting data removed from UART */
            SourceDrop(uart_source, MAX_GETINPUTLINE_LENGTH);           /* Remove the now defunct data from the UART */
            uart_size = SourceSize(uart_source);                                	/* Find out how much data is now in the UART */
        }

        /* Copy data from UART */
        memcpy((s + inp_str_len), SourceMap(uart_source), uart_size);

        /* Search through the characters */
        for (i = 0; i < uart_size; i++, inp_str_len++)
        {
            /*sendchar(*(s + inp_str_len));*/                                       /* Echo the key strokes to the screen */

            /* If a backspace has been entered, update screen and pointers */
            if (*(s + inp_str_len) == '\b')
            {
                sendchar(' ');
                sendchar('\b');
                inp_str_len-=2;
            }

            if (*(s + inp_str_len) == '\r')
            {
                /* We have found the carriage return
                Send the current input upto the CR, but adding a LF and end of string (0) */
                sendchar('\n');

                /* End the string - Add ascii zero in place of the carriage return */
                *(s + inp_str_len) = 0;

                /* Empty the source of the data we have used only */
                SourceDrop(uart_source, i+1);

                /* Come out of the function */
                return;
            }
        }
        /* Inspected the complete string so now remove it from the UART */
        SourceDrop(uart_source, uart_size);
    }
}

void UpperCase(char *s)                                      /* Takes a string and converts lower case to upper, otherwise leaves unchanged. */
{
    uint16 i;

    for ( i = 0; i < strlen(s); i++)
    {
        *(s+i) = toupper(*(s+i));
    }

    return;
}

int ReturnTokens(char *s, int *params)
{
    int     i           = 0;
    int     j           = 0;
    int     func        = 0;
    int     locat[3];
    int     mult;



    /* Initialise the string length */
    if (!(locat[2] = strlen(s))) return NOTHING;


    /* Read the begining of the string to determine the function */

    if ( !memcmp(s,"HELP", 4 ))                                /* Help requested */
    {
        func = HELP;
        return func;
    }
    else if ( !memcmp(s,"STOP", 4))                                
    {
        func = STOP;
        return func;
    }
    else if ( !memcmp(s,"DFU", 3))                                
    {
        func = DFU;
        return func;
    }
    else if ( !memcmp(s,"LEDOFF", 6))                                
    {
        func = LEDOFF;
        return func;
    }
	else if( !memcmp(s, "PIOSTATE" , 8))
	{
        func = PIOSTATE;
        return func;
	}
    else if ( !memcmp(s,"LEDON", 5))                                
    {
        func = LEDON;
        return func;
    }
    else if ( !memcmp(s,"QUERYSN", 7))                                
    {
        func = QUERYSN;
        return func;
    }
	else if ( !memcmp(s,"READBT", 6))                                
    {
        func = READBT;
        return func;
    }
	else if ( !memcmp(s,"VER", 3))                                
    {
        func = VER;
        return func;
    }
	else if ( !memcmp(s,"AUDIOLB", 7))                                
    {
        func = AUDIOLB;
        return func;
    }
	else if ( !memcmp(s,"WRITEBT", 7))                                
    {
        func = WRITEBT;
        return func;
    }
	else if ( !memcmp(s,"RESETPDL", 8))                                
    {
        func = RESETPDL;
        return func;
    }
	else                                                            /* Input string not recognised */
    {
        func = UNKNOWN;
        return func;
    }

    /* Search through string for location markers - Spaces */
    for ( i = 0; i < locat[2]; i++)
    {
        if ( (*(s + i) == 0x20 ) && (j < 2) )
        {
            locat[j] = i + 1;
            j++;
        }
    }

    /* Check how many spaces have been found */
    switch (j)
    {
        case 0:                                                     /* No spaces have been found status check */
        {
            return STATUS;
        }
        break;

        case 1:                                                     /* Only one parameter passed */
        {
            locat[1] = locat[2] + 1;
        }
        break;

        case 2:                                                     /* Both parameters passed */
        {
            locat[2] = locat[2] + 1;
        }
        break;
    }


    /* Convert the string data between the locators to int data */
    for (j = 0; j < 2; j++)
    {
        *(params+j) = 0;

        mult = 1;
        for (i = (locat[j+1]-2); i > (locat[j]-1); i--)
        {
            *(params+j) = *(params+j) + ((*(s+i))-48)*mult;
            mult*=10;
        }

        if (params[j] < 0)
        {
            if (j == 0)
            {
                send("Warning the first parameter you entered is too large (> 2^15).\r\n");
            }
            else
            {
                send("Warning the second parameter you entered is too large (> 2^15).\r\n");
            }

            func = UNKNOWN;
        }
    }
    return func;
}

void appUartInit(UARTTaskData *theUartM)
{
	theUartM->uart_task.handler = appUartHandler;
	theUartM->Run = false;
	theUartM->Counter = 0;

    uart_source = StreamUartSource();
    SourceEmpty(uart_source);

    /*
       send("***************************\n");
	send("Welcome to the Manufacture testing mode.\n");
	send("***************************\n");
       */

	/*MessageSend(&theHeadset.uart_M_task.uart_task , true , 0);*/
	MessageSend(&theHeadset.uart_M_task.uart_task , UART_TESTMODE , 0);
}

void appUartHandler(Task task, MessageId id, Message message)
{
#ifdef UART_Debug
	#ifdef Testmode2
	int     func        = 0;
	uint16 lPinVals = 0 ;
	char	Sendbuf[16];
	/*int     params[2];*/
	#endif
	
	char    s[MAX_GETINPUTLINE_LENGTH];
	UARTTaskData *ptr = (UARTTaskData *)task;	
	SNData* config;
    /*POWER_BATTERY_LEVEL_IND_T volt_level;*/
    
#endif	
	
#ifdef PIO_LED_TEST 
	if(ptr->Run == false && id == UART_TESTMODE)
	{
		if(ptr->Counter < 2)
		{
			if(ptr->Counter == 0)
			{
				/*ptr->Run = true;*/
				PioSetPio(14 ,0);
				PioSetPio(15 ,0);
				ptr->Counter++;
				/*send("LED OFF!\r\n");*/
			}
			else
			{
				/*ptr->Run = false;*/
				PioSetPio(14 ,1);
				PioSetPio(15 ,1);
				ptr->Counter++;
				/*send("LED ON!\r\n");*/
			}
					
			/*PioSetDir32( 0xffe0 , 0xffe0);			 
			lPinVals = PioGet32();
			sprintf(Sendbuf , "%X\n",lPinVals);
			send(Sendbuf);
			*/
			
			MessageSendLater(task , id , 0 , 1000); 
		}
		
		if(ptr->Counter == 2)	
		{
			ptr->Run = true;
			PioSetPio(14 ,0);
			PioSetPio(15 ,0);
			/*send("ok!\r\n");*/
		}
	}
#endif

#ifdef UART_Debug 
	if(ptr->Counter == 2 && id == UART_TESTMODE)	
    {        
    	if(StreamUartSink())
    	{
			ChargerConfigure(CHARGER_SUPPRESS_LED0, TRUE);

			send("*** Welcome to BHC612-100 test mode ***\r\n");

			#ifdef Testmode2
			ShowMenu();
			#endif
			
			while(ptr->Run)
			{	
	            /* Monitor the input data and return s, when carriage return entered */
	            GetInputLine(&s[0]);

	            /* Convert input to uppercase */
	            UpperCase(&s[0]);
			
				#ifdef Testmode2
					if(s[0] >= 0x31 && s[0] <= 0x38)
					{
						/*send(s);*/
						func = s[0] - 0x30;
					}
					else
						func = UNKNOWN;
				#else
			       /* Remove tokens from input string, return 3 tokens */ 
			       func = ReturnTokens(&s[0], &params[0]);
				#endif
			
		     	switch(func)
	            	{
	                case UNKNOWN:             
	                {
						send("Unknown!\r\n");

						#if 0

						send("G\r\n");
						PowerCharger(TRUE);
						send("P\r\n");
						PowerGetVoltage(&volt_level);
						
						if(ChargerStatus() != NO_POWER)
						{
							sprintf(Sendbuf , "VBat_5V = %d\r\n",volt_level.voltage);
						}
						else
						{
							sprintf(Sendbuf , "VBat = %d\r\n",volt_level.voltage);
						}
						
						send(Sendbuf);
						
						func = ChargerStatus();
						switch(func)
						{
							case 0:
								send("T\r\n");	/*Trickle charge*/
								break;
							case 1:
								send("F\r\n");	/*Fast charge*/
								break;		
							case 2:
								send("X\r\n");	/*Error*/
								break;
							case 3:
								send("S\r\n");	/*Standby*/
								break;
							case 5:
								break;
						}
						#endif

						#if 0	
						ptr->Run = false;
						ptr->Counter++;
						send("#");
						#endif
						
						#if 0
							if(ChargerStatus() != NO_POWER)
							{
								memset(s , 0 , MAX_GETINPUTLINE_LENGTH);
			                    			
								#ifdef Testmode2
									ShowMenu();
								#else
									send("Unknown command!\r\n");
								#endif
							}
						#endif
	                }
	                break;
					case AUDIOLB:
					{					
						if(ChargerStatus() != NO_POWER)
						{
							send("AUDIOLB\r\n");
							
							if(!(TestCodecStereoLb(8000,0)))
							{
								send("Set Loopback fail\r\n");
							}
							else
								send("Set Loopback successfully\r\n");
						}
					}
					break;
					case RESETPDL:
					{
						if(ChargerStatus() != NO_POWER)
						{
							ConnectionSmDeleteAllAuthDevices(ATTRIBUTE_PSKEY_BASE);
							send("Manufacture reset(Clear PDL)!\r\n");
						}
					}
					break;
					case VER:
					{
						if(ChargerStatus() != NO_POWER)
						{
							#if 0
							sprintf(Sendbuf , "FW : V%d.%d\r\n",BHC612_VMAPPVERMAJORREV,BHC612_VMAPPVERMINORREV);
							send(Sendbuf);
							#else
							sprintf(Sendbuf , "FW : V%d.%d.%d\r\n",BHC612_VMAPPVERMAJORREV,BHC612_VMAPPVERMINORREV,BHC612_TEST_Version);
							send(Sendbuf);
							#endif
							
							sprintf(Sendbuf , "ATi : kap%d,Tuning%d\r\n",ATi_Kap_ver , ATi_Tuning_ver);
							send(Sendbuf);							
							
							sprintf(Sendbuf , "Rubidium : %d/%d/%d\r\n",Rubidium_mm,Rubidium_dd,Rubidium_yy);
							send(Sendbuf);
						}
					}
					break;
					case READBT:
					{
						if(ChargerStatus() != NO_POWER)
						{
							config = (SNData*) mallocPanic(sizeof(SNData));

							if(PsFullRetrieve(0x03b7 , &lPinVals , 1))
							{
								sprintf(Sendbuf , "charger_trim = %d\r\n",lPinVals);
								send(Sendbuf);
							}
							
							if(PsFullRetrieve(0x039b , &lPinVals , 1))
							{
								sprintf(Sendbuf , "charger_curr = %d\r\n",lPinVals);
								send(Sendbuf);
							}

							if(PsFullRetrieve(0x01f6 , &lPinVals , 1))
							{
								sprintf(Sendbuf , "crystal_trim = %d\r\n",lPinVals);
								send(Sendbuf);
							}
	                        
							#if 1
							if(PsFullRetrieve(0x0001 , config , 4))
							{
								send("BT:NAP/UAP/LAP\r\n");
								sprintf(Sendbuf , "NAP : %X\r\n",config->sn_4);/*NAP*/
								send(Sendbuf);
								sprintf(Sendbuf , "UAP : %X\r\n",config->sn_3);/*UAP*/
								send(Sendbuf);
								if(config->sn_1 > 0)
								{
									sprintf(Sendbuf , "LAP : %X",config->sn_1);
									send(Sendbuf);
									sprintf(Sendbuf , "%X\r\n",config->sn_2);/*LAP*/
									send(Sendbuf);
								}
								else
								{
									sprintf(Sendbuf , "LAP : %X\r\n",config->sn_2);/*LAP*/
									send(Sendbuf);
								}
							}
							else
							{
								send("Read BT fail!\r\n");
							}
	                        #endif
							free(config) ;
						}
					}
					break;
					case WRITEBT:
					{

					}
					break;
					case QUERYSN:
					{
						if(ChargerStatus() != NO_POWER)
						{
							#if 1
							config = (SNData*) mallocPanic(sizeof(SNData));
							
							if((PsFullRetrieve(0x025e, config , 4)) == 4)
				            {
				            	send("SN : ");
				                sprintf(Sendbuf , "%X",config->sn_1);
				                send(Sendbuf);
				                sprintf(Sendbuf , "%X",config->sn_2);
				                send(Sendbuf);
							   	sprintf(Sendbuf , "%X",config->sn_3);
				                send(Sendbuf);
				                sprintf(Sendbuf , "%X\r\n",config->sn_4);
				                send(Sendbuf);
				            }
							else
								send("SN not exist!\r\n");
	                       
							free(config);
							#else
								send("V_Batt monitor..\r\n");						
								ptr->Run = false;
								MessageSendLater(task , UART_VBatt , 0 , 1000); 
							#endif
						}
					}
					break;
					case PIOSTATE:
					{
						/*send("Query BUTT_PIO\r\n");*/

						if(ChargerStatus() != NO_POWER)
						{
							PioSetDir32( 0xffe0 , 0xffe0);           

							lPinVals = PioGet32();

							#ifdef Testmode2
								switch(lPinVals&0xFF)
								{
									case 1:/*PIO 0*/
										send("Volume Up button\r\n");break;
									case 2:/*PIO 1*/
										send("Volume Down button\r\n");break;
									case 4:/*PIO 2*/
										send("MFB button\r\n");break;
									case 0:
										send("No button pressed!\r\n");break;
									default:
										send("Unknown\r\n");
									break;
								}
							#else
								sprintf(Sendbuf , "%X\r\n",(lPinVals&0xFF));
								send(Sendbuf);
							#endif
						}
					}
					break;	
				 	case LEDOFF:
				 	{
						if(ChargerStatus() != NO_POWER)
						{
					  		/*send("LED OFF\r\n");*/
							#ifdef BHC612_100_NewHW
							PioSetPio(BHC612_LED_0 , LED_OFF);
							#else
							LedConfigure(LED_0, LED_ENABLE, 0 ) ;
				            LedConfigure(LED_0, LED_DUTY_CYCLE, (0xfff));
				            LedConfigure(LED_0, LED_PERIOD, 0 );
							#endif
							LedConfigure(LED_1, LED_ENABLE, 0 ) ;
				            LedConfigure(LED_1, LED_DUTY_CYCLE, (0xfff));
				            LedConfigure(LED_1, LED_PERIOD, 0 );
							PioSetPio(14 , LED_OFF);
							PioSetPio(15 , LED_OFF);
							PioSetPio(PS_blue_led, PS_blue_led_off);
							send("LED Off\r\n");
						}
				  	}
				  	break;
				  	case LEDON:
				  	{
						if(ChargerStatus() != NO_POWER)
						{
					  		/*send("LED ON\r\n");*/
							#ifdef BHC612_100_NewHW
							PioSetPio(BHC612_LED_0 , LED_ON);
							#else
							LedConfigure(LED_0, LED_ENABLE, 1 ) ;
				            LedConfigure(LED_0, LED_DUTY_CYCLE, (0xfff));
				            LedConfigure(LED_0, LED_PERIOD, 0 );
							#endif
							LedConfigure(LED_1, LED_ENABLE, 1 ) ;
				            LedConfigure(LED_1, LED_DUTY_CYCLE, (0xfff));
				            LedConfigure(LED_1, LED_PERIOD, 0 );
							PioSetPio(14 , LED_ON);
							PioSetPio(15 , LED_ON);
							PioSetPio(PS_blue_led, PS_blue_led_on);
							send("LED On\r\n");
						}
				  	}
				  	break;
				  	case STOP:
				  	{
						if(ChargerStatus() != NO_POWER)
						{
					  		send("Close UART,Bye!\r\n");
							SourceEmpty(uart_source);
							ptr->Run = false;
						}
				  	}
				  	break;
				  	case HELP:                
	                {
						if(ChargerStatus() != NO_POWER)
						{
	                    	send("***BHC612-100 Manufacture testing mode***\r\n");
							send("Test Command : [help/ver/ledon/ledoff/piostate/querysn/resetpdl]\r\n");
						}
			  		}
			      	break;
			  		case DFU:
			  		{
			  			/*send("Enter DFU via UART\r\n");*/
			  		}
			  		break;
					default:
						break;
		     	}
	        }

			/*send("&");*/
    	}
		/*else
			send("*");*/	
    }
	else if(id == UART_VBatt)
	{
		send("V_Batt..\r\n");
		MessageSendLater(task , UART_VBatt , 0 , 4000);
	}
#endif

}
#endif
/*===============================================*/

/*************************************************************************
NAME    
    handleCLMessage
    
DESCRIPTION
    Function to handle the CL Lib messages - these are independent of state

RETURNS

*/
static void handleCLMessage ( Task task, MessageId id, Message message )
{
    MAIN_DEBUG(("CL = [%x]\n", id)) ;    
 
        /* Handle incoming message identified by its message ID */
    switch(id)
    {        
        case CL_INIT_CFM:
            MAIN_DEBUG(("CL_INIT_CFM [%d]\n" , ((CL_INIT_CFM_T*)message)->status ));
            if(((CL_INIT_CFM_T*)message)->status == success)
            {				
				/* Initialise the codec task */
				headsetInitCodecTask();

				#ifdef ATI_SPP
				soundClearSppInit();
				#endif
			}
            else
            {
                Panic();
            }
		break;
		case CL_DM_WRITE_INQUIRY_MODE_CFM:
			/* Read the local name to put in our EIR data */
			ConnectionReadInquiryTx(&theHeadset.task);
		break;
		case CL_DM_READ_INQUIRY_TX_CFM:
			theHeadset.inquiry_tx = ((CL_DM_READ_INQUIRY_TX_CFM_T*)message)->tx_power;
			ConnectionReadLocalName(&theHeadset.task);
		break;
		case CL_DM_LOCAL_NAME_COMPLETE:
		{
			MAIN_DEBUG(("CL_DM_LOCAL_NAME_COMPLETE\n"));
			/* Write EIR data and initialise the codec task */
			headsetWriteEirData((CL_DM_LOCAL_NAME_COMPLETE_T*)message);
		}
		break;
		case CL_SM_SEC_MODE_CONFIG_CFM:
			MAIN_DEBUG(("CL_SM_SEC_MODE_CONFIG_CFM\n"));
			/* Remember if debug keys are on or off */
			theHeadset.debug_keys_enabled = ((CL_SM_SEC_MODE_CONFIG_CFM_T*)message)->debug_keys;
		break;
  		case CL_SM_PIN_CODE_IND:
            MAIN_DEBUG(("CL_SM_PIN_IND\n"));
            headsetHandlePinCodeInd((CL_SM_PIN_CODE_IND_T*) message);
        break;
		case CL_SM_USER_CONFIRMATION_REQ_IND:
			MAIN_DEBUG(("CL_SM_USER_CONFIRMATION_REQ_IND\n"));
			headsetHandleUserConfirmationInd((CL_SM_USER_CONFIRMATION_REQ_IND_T*) message);
		break;
		case CL_SM_USER_PASSKEY_REQ_IND:
			MAIN_DEBUG(("CL_SM_USER_PASSKEY_REQ_IND\n"));
			headsetHandleUserPasskeyInd((CL_SM_USER_PASSKEY_REQ_IND_T*) message);
		break;
		case CL_SM_USER_PASSKEY_NOTIFICATION_IND:
			MAIN_DEBUG(("CL_SM_USER_PASSKEY_NOTIFICATION_IND\n"));
			headsetHandleUserPasskeyNotificationInd((CL_SM_USER_PASSKEY_NOTIFICATION_IND_T*) message);
		break;
		case CL_SM_KEYPRESS_NOTIFICATION_IND:
		break;
		case CL_SM_REMOTE_IO_CAPABILITY_IND:
			MAIN_DEBUG(("CL_SM_IO_CAPABILITY_IND\n"));
			headsetHandleRemoteIoCapabilityInd((CL_SM_REMOTE_IO_CAPABILITY_IND_T*)message);
		break;
		case CL_SM_IO_CAPABILITY_REQ_IND:
			MAIN_DEBUG(("CL_SM_IO_CAPABILITY_REQ_IND\n"));
			headsetHandleIoCapabilityInd((CL_SM_IO_CAPABILITY_REQ_IND_T*) message);
		break;
        case CL_SM_AUTHORISE_IND:
            MAIN_DEBUG(("CL_SM_AUTHORISE_IND\n"));
            headsetHandleAuthoriseInd((CL_SM_AUTHORISE_IND_T*) message);
			#if 1
			if((theHeadset.MultipointEnable == 0) && (theHeadset.BHC612_VPLinkLoss == 1))
			{
				theHeadset.BHC612_VPLinkLoss = 0;
			}
			#endif
        break;            
        case CL_SM_AUTHENTICATE_CFM:
            MAIN_DEBUG(("CL_SM_AUTHENTICATE_CFM\n"));
            headsetHandleAuthenticateCfm((CL_SM_AUTHENTICATE_CFM_T*) message);
        break;           
        case CL_DM_REMOTE_FEATURES_CFM:
            MAIN_DEBUG(("HS : Supported Features\n")) ;
            headsetHandleRemoteSuppFeatures((CL_DM_REMOTE_FEATURES_CFM_T *)(message));
        break ;
        case CL_DM_INQUIRE_RESULT:
            MAIN_DEBUG(("HS : Inquiry Result\n"));
            slcHandleInquiryResult((CL_DM_INQUIRE_RESULT_T*)message);
        break;
        case CL_SM_GET_ATTRIBUTE_CFM:
			MAIN_DEBUG(("HS : CL_SM_GET_ATTRIBUTE_CFM Vol:%d \n",((CL_SM_GET_ATTRIBUTE_CFM_T *)(message))->psdata[0]));
        break;
        case CL_SM_GET_INDEXED_ATTRIBUTE_CFM:
            MAIN_DEBUG(("HS: CL_SM_GET_INDEXED_ATTRIBUTE_CFM[%d]\n" , ((CL_SM_GET_INDEXED_ATTRIBUTE_CFM_T*)message)->status)) ;
        break ;    
		
		case CL_DM_LOCAL_BD_ADDR_CFM:
            DutHandleLocalAddr((CL_DM_LOCAL_BD_ADDR_CFM_T *)message);
		break ;
          
        case CL_DM_ROLE_CFM:
            slcHandleRoleConfirm((CL_DM_ROLE_CFM_T *)message);
            return;
        
            /*all unhandled connection lib messages end up here*/          
        default :
            MAIN_DEBUG(("Headset - Unhandled CL msg[%x]\n", id));
        break ;
    }
   
}

/*************************************************************************
NAME    
    handleUEMessage
    
DESCRIPTION
    handles messages from the User Events

RETURNS
    
*/
static void handleUEMessage  ( Task task, MessageId id, Message message )
{
    /* Event state control is done by the config - we will be in the right state for the message
    therefore messages need only be passed to the relative handlers unless configurable */
    headsetState lState = stateManagerGetState() ;
	
    /*if we do not want the event received to be indicated then set this to FALSE*/
    bool lIndicateEvent = TRUE ;

	#ifdef CTIA_Test
	 hfp_call_state stateAG1;
	#endif

    /* printf("UE event , %d \n",id); */    	
    /* Deal with user generated Event specific actions*/
    switch ( id )
    {   
    	/*these are the events that are not user generated and can occur at any time*/
        case EventOkBattery:
        case EventChargerDisconnected:
		case EventPairingFail:
        case EventLEDEventComplete:
        case EventTrickleCharge:
		case EventFastCharge:	
        case EventLowBattery:
        case EventPowerOff:
        case EventLinkLoss:
		case EventLimboTimeout:
        case EventSLCConnected:
        case EventSLCConnectedAfterPowerOn:
        case EventError:
        case EventChargeError:
        case EventChargeErrorInIdleState:
        case EventCancelLedIndication:
        case EventAutoSwitchOff:
		case EventReconnectFailed:
        case EventCheckForLowBatt:
        case EventChargerGasGauge0:
        case EventChargerGasGauge1:
        case EventChargerGasGauge2:
        case EventChargerGasGauge3:		
		case EventRssiPairReminder:
		case EventRssiPairTimeout:
        case EventConnectableTimeout:
        case EventRefreshEncryption:
        case EventEndOfCall:		
        case EventSCOLinkClose:
        case EventMissedCall:
        break ;
	
        default:
      		/* If we have had an event then reset the timer - if it was the event then we will switch off anyway*/
            if (theHeadset.conf->timeouts.AutoSwitchOffTime_s !=0)
            {
                /*MAIN_DEBUG(("HS: AUTOSent Ev[%x] Time[%d]\n",id , theHeadset.conf->timeouts.AutoSwitchOffTime_s ));*/
                MessageCancelAll( task , EventAutoSwitchOff ) ;
                MessageSendLater( task , EventAutoSwitchOff , 0 , D_SEC(theHeadset.conf->timeouts.AutoSwitchOffTime_s) ) ;    
				/*MAIN_DEBUG(("HS : Reset AutoSwitchOff Timer!!!\n"));*/
            }
 
            /*cancel any missed call indicator on a user event (button press)*/
           MessageCancelAll(task , EventMissedCall ) ;
 
            /* check for led timeout/re-enable */
#ifdef ROM_LEDS
			LEDManagerCheckTimeoutState();
#endif
   
        break;
    }

#ifdef BHC612
	#ifdef StandbySavingPower
	switch(id)
	{
		case EventChargerConnected:
		case EventChargerDisconnected:
		case EventEstablishSLC:	/*Establish or Check BT connection*/
		case EventInitateVoiceDial:	
		case EventUnused6:	
		case EventAvrcpPlayPause:
		case EventVolumeUp:
		case EventVolumeDown:
		case EventConfirmTTSLanguage:
		case EventUpdateStoredNumber:
		if(lState == headsetConnected)
		{
			/*Check LED state*/
			if(theHeadset.theLEDTask->gLEDSEnabled == TRUE)
			{
				MessageCancelAll(&theHeadset.task, EventDisableLEDS);
				MessageSendLater(&theHeadset.task, EventDisableLEDS,0, D_MIN(StandbyTimeout));		
			}
			else
			{
				LedManagerEnableLEDS();
			}

			#ifdef NewChargeMMI
			if(id != EventChargerConnected && id != EventChargerDisconnected)
			{
				/*Enable recharge function*/
				theHeadset.BHC612_Chargefull = 0;
				PioSetPio(battery_low_io , PowerBlu_Connect);
				theHeadset.BHC612_BatteryLowState = PowerBlu_Connect;
			}
			#endif
			
			#ifdef iOSBatteryMeter
				if((theHeadset.BHC612_BattMeterOK == false) && (id == EventEstablishSLC))
				{
					BatteryUserInitiatedRead();
				}
			#endif
		}
		else if(lState == headsetA2DPStreaming)
		{
			if((id == EventAvrcpPlayPause) && (theHeadset.VoicePromptNotComplete == TRUE))
				theHeadset.VoicePromptNotComplete = FALSE;
		}
		else
		{
			#ifdef NewChargeMMI
			if(id != EventChargerConnected && id != EventChargerDisconnected)
			{
				if(id != EventVolumeUp && id != EventVolumeDown)
				{
					/*MAIN_DEBUG(("chk2..\n"));
					MAIN_DEBUG(("#####Clear2 recharge state\n"));
					*/
					theHeadset.BHC612_Chargefull = 0;
					PioSetPio(battery_low_io , PowerBlu_Connect);
					theHeadset.BHC612_BatteryLowState = PowerBlu_Connect;
				}
			}
			#endif
		}
		break;
		case EventSpare2:
			#ifdef iOSBatteryMeter
			/*if(theHeadset.BHC612_BattMeterOK == false)*/
			if((theHeadset.BHC612_BattMeterOK == false) && (BatteryCounter < 3))	
			{
				BatteryUserInitiatedRead();
				MessageSendLater(&theHeadset.task, EventSpare2 , 0 , 3000);/*Recover Battery icon*/
				
				BatteryCounter++;
				/*MAIN_DEBUG(("EventSpare2 = %d\n",BatteryCounter));*/
			}
			else
			{
				MessageCancelAll(task , EventSpare2 ) ;
			}
			#endif
		break;	
		default:
		break;	
	}
	#endif
#endif
     
/*    MAIN_DEBUG (( "HS : UE[%x]\n", id )); */
	 			
    /* The configurable Events*/
    switch ( id )
    {   
    	#ifdef Rubidium
		case (EventUnused):
		{			
			#ifdef RubiTTSTerminate
			if((Kalimba_mode == 3) && (ASR_active_model == 0))
			{
				RUBI_DEBUG(("###Kalimba : TTS_ASR End!\n"));
				AnswerCall_function1();
				break;
			}
			else
			{
				/*TTS_ONLY_MODE*/
				UnloadRubidiumEngine();
				theHeadset.TTS_ASR_Playing = false;			
				RUBI_DEBUG((" !!!TTS End : UnloadRubidiumEngine , SCO = %d\n",(uint16)theHeadset.sco_sink));
			}
			#else
				UnloadRubidiumEngine();
				theHeadset.TTS_ASR_Playing = false;			
				RUBI_DEBUG((" !!!TTS End : UnloadRubidiumEngine , SCO = %d\n",(uint16)theHeadset.sco_sink));
			#endif
			
			if((theHeadset.VoicePromptNotComplete == TRUE) && (stateManagerGetState() == headsetConnected))
			{
				if(!(strcmp(TTS_text , "\\p=026")))	/*Call ended*/
				{
					memset(TTS_text, 0, sizeof(TTS_text));  
					theHeadset.VoicePromptNotComplete = FALSE;
				}
			}

			if(stateManagerGetState() == headsetConnected)
			{
				if(!(strcmp(TTS_text , "\\p=029")))
				{
					memset(TTS_text, 0, sizeof(TTS_text));
					GetA2DPState();
				}
			}

			#ifdef MP_Config4
			if(theHeadset.BHC612_MPReconnect)
			{
				theHeadset.BHC612_MPReconnect = false;
				
				if(!(strcmp(TTS_text , "\\p=041")))
				{
					memset(TTS_text, 0, sizeof(TTS_text));
					RUBI_DEBUG(("!!!###\n"));
					MessageSend ( &theHeadset.task , EventPowerOff , 0 );					
				}
			}
			#endif

			if((theHeadset.BHC612_TEMP == 1) && (theHeadset.PressCallButton == FALSE))
			{
				theHeadset.BHC612_TEMP = 0;
				theHeadset.PressCallButton = TRUE;
				RUBI_DEBUG(("PressCallButton : [T]\n"));
			}

			/*#ifdef MANDARIN_SUPPORT*/
			#if 1
				#ifdef ReleaseHeldCall
				if((theHeadset.BHC612_TEMP == 2) && (theHeadset.PressCallButton == FALSE))
				{
					theHeadset.BHC612_TEMP = 0;
					MpAcceptWaitingHoldActive();/*TTS End!*/
				}				
				#endif
			#endif

			#ifdef Rubi_VP_MinorChg
			if(stateManagerGetState() == headsetConnDiscoverable)
			{
				if(theHeadset.Rubi_enable == 0)
				{
					RUBI_DEBUG((" ###TTS End : %d\n",theHeadset.DockLED));
					
					if((theHeadset.DockLED >= 2) && (stateManagerGetPdlSize() == 0))
						theHeadset.DockLED++;

					if((theHeadset.DockLED == 4) || (theHeadset.DockLED == 6))
					{
						task = (TaskData *) &rubidium_tts_asr;	
						memset(TTS_text, 0, sizeof(TTS_text));

						if(theHeadset.DockLED == 4)
						{
							strcpy(TTS_text, "\\p=005\\p=006\\p=007\\p=008");
							RUBI_DEBUG(("Rubi voice prompt > \\p=005\\p=006\\p=007\\p=008\n" ));
						}
						else
						{
							strcpy(TTS_text, "\\p=005\\p=006\\p=007\\p=008\\p=047\\p=047\\p=015");
							RUBI_DEBUG(("Rubi voice prompt > \\p=005\\p=006\\p=007\\p=008\\p=047\\p=047\\p=015\n" ));
						}
								
						Kalimba_mode 		= 1; /*TTS_ONLY_MODE*/
		        
		       			ASR_active_model	= 0; /*RINGING_NODE*/

						theHeadset.TTS_ASR_Playing = true;

						AudioPlayTTS(	task, 
										(uint16)0, 								/*TTS_CALLERID_NUMBER, */
										(uint8*)TTS_text,					 	/*num_string, */
										sizeof(TTS_text), 
										Language,	/*theHeadset.tts_language, */
										FALSE,									/*FALSE*/  
										theHeadset.codec_task,					/*theHeadset.codec_task, */
										0, 										/*TonesGetToneVolume(FALSE), */
										0										/*theHeadset.features.stereo	*/
									);

						if(theHeadset.DockLED == 6)
							theHeadset.DockLED = 0;
							
					}
					
					if((theHeadset.DockLED == 3) || (theHeadset.DockLED == 5))
					{							
						MessageSendLater (&theHeadset.task , EventUnused , 0, 5000 ) ;  
					}
				}
			}
			#endif
		}
		break;
		case (EventSpare3):
		{
			/*#ifdef Three_Language*/
			#if defined(Three_Language) || defined(MANDARIN_SUPPORT)
			/*New voice prompt : "Call on hold"*/
			if(theHeadset.sco_sink == 0)
			{
				MessageCancelAll(&theHeadset.task, EventSpare3);

				#ifdef Rubi_VoicePrompt
				if(theHeadset.Rubi_enable == 0)
				{
					task = (TaskData *) &rubidium_tts_asr;	
					
					memset(TTS_text, 0, sizeof(TTS_text));

					if(theHeadset.BHC612_TEMP == 3)
					{
						strcpy(TTS_text, "\\p=068");/*Call on hold"*/
						RUBI_DEBUG(("Rubi voice prompt > \\p=068(Call on hold)\n" ));
					}
					
					if(theHeadset.BHC612_TEMP == 2)
					{
						strcpy(TTS_text, "\\p=013");/*Connected"*/	
						RUBI_DEBUG(("Rubi voice prompt > \\p=013(Connected)\n" ));
					}
					
					Kalimba_mode		= 1; /*TTS_ONLY_MODE*/
					
					ASR_active_model	= 0; /*RINGING_NODE*/
					
					theHeadset.TTS_ASR_Playing = true;
								
					AudioPlayTTS(	task, 
									(uint16)0,								/*TTS_CALLERID_NUMBER, */
									(uint8*)TTS_text,						/*num_string, */
									sizeof(TTS_text), 
									Language,								/*theHeadset.tts_language, */
									FALSE,									/*FALSE*/  
									theHeadset.codec_task,					/*theHeadset.codec_task, */
									0,										/*TonesGetToneVolume(FALSE), */
									0										/*theHeadset.features.stereo	*/
								);

				}
				#endif

				if(theHeadset.BHC612_TEMP == 3)	
				{
					theHeadset.BHC612_TEMP = 0;
					theHeadset.PressCallButton = true;
					RUBI_DEBUG(("### PressCallButton : [T]\n"));					
				}
			}
			else
			{
				MessageSendLater (&theHeadset.task , EventSpare3 , 0, 200 ) ;   
				RUBI_DEBUG((" Ru.." ));
			}
			#endif
		}
		break;
		#endif
		case (EventToggleDebugKeys):
			MAIN_DEBUG(("HS: Toggle Debug Keys\n"));
			/* If the headset has debug keys enabled then toggle on/off */
			ConnectionSmSecModeConfig(&theHeadset.task, cl_sm_wae_acl_none, !theHeadset.debug_keys_enabled, TRUE);
		break;
		case (EventPowerOn):
            MAIN_DEBUG(("HS: Power On\n" ));

			#ifdef BHC612		
				#ifdef PSToggleQuickly
					if(!(PsuGetVregEn()))	/*Return false : Vreg detect Low : PowerOff*/
					{
						lIndicateEvent = FALSE;
						MessageCancelAll(task , EventPowerOn ) ;
						MessageSend ( &theHeadset.task , EventPowerOff , 0 ) ;
						break;
					}
				#endif
			
				#ifdef EnablePowerOnLED
					if(theHeadset.BHC612_PowerInitComplete == 0)
					{
						lIndicateEvent = FALSE;
						MessageCancelAll(task , EventPowerOn ) ;
						MessageSendLater (&theHeadset.task , EventPowerOn , 0, 50 ) ;
						break;
					}
					
					MessageCancelAll(task , EventPowerOn ) ;
					
					PowerGetVoltage(&BHC612_volt_level);
					theHeadset.batt_level = BHC612_volt_level.pLevel;
					
					if( theHeadset.batt_level >= Battery_ChargeStop)
					{
						MAIN_DEBUG(("PowerOn:batt lev3\n"));		
						
						PioSetPio(battery_low_io , PowerBlu_Connect);

						theHeadset.BHC612_BatteryLowState = PowerBlu_Connect;
					}
					else
					{
						if(theHeadset.batt_level == POWER_BATT_LOW || theHeadset.batt_level == POWER_BATT_LEVEL0)
							MAIN_DEBUG(("PowerOn:batt low,lev0\n"));
						if( theHeadset.batt_level == POWER_BATT_LEVEL1)
							MAIN_DEBUG(("PowerOn:batt lev1\n"));
						if( theHeadset.batt_level == POWER_BATT_LEVEL2)
							MAIN_DEBUG(("PowerOn:batt lev2\n"));

						PioSetPio(battery_low_io , PowerBlu_Connect);

						theHeadset.BHC612_BatteryLowState = PowerBlu_Connect;
					}
				#else
					PioSetPio(battery_low_io , PowerBlu_Connect);
					theHeadset.BHC612_BatteryLowState = PowerBlu_Connect;
				#endif
				
				#ifdef HW_DVT2
					#ifdef NewPSBlueLEDx
					MAIN_DEBUG((" ##### checkPowerBluConn...\n"));
					checkPowerBluConn();

					if(theHeadset.BHC612_PowerBluConn == BlueLDE_Detect_Connect)/*Headset dock to PS*/
					{
						lIndicateEvent = FALSE;
						MessageCancelAll(task , EventPowerOn ) ;
						MAIN_DEBUG(("Cancel Power on event! , 1\n"));
						
						#ifdef CheckForDFUModeEntry
							MessageSend(task, EventEnterDFUMode, 0);
						#endif

						break;
					}
					#endif
				#endif

				#ifdef My_DVT1_Samplex
				if(ChargerStatus() != NO_POWER)
				{
					lIndicateEvent = FALSE;
					MessageCancelAll(task , EventPowerOn ) ;
					MAIN_DEBUG(("Cancel Power on event! , 2\n"));
					break;
				}
				#endif
				
				#ifdef Rubidium
					if(theHeadset.Rubi_enable == 0)
						MAIN_DEBUG(("Rubidium enable.\n"));
					
					if(Language == AUDIO_TTS_LANGUAGE_AMERICAN_ENGLISH)
						MAIN_DEBUG(("TTS Lauguage : US\n"));
					if(Language == AUDIO_TTS_LANGUAGE_SPANISH_EUROPEAN)
						MAIN_DEBUG(("TTS Lauguage : SP\n"));
					if(Language == AUDIO_TTS_LANGUAGE_FRENCH)
						MAIN_DEBUG(("TTS Lauguage : FR\n"));
					if(Language == AUDIO_TTS_LANGUAGE_CHINESE_MANDARIN)
						MAIN_DEBUG(("TTS Lauguage : Chinese_Mandarin\n"));

					MAIN_DEBUG(("%d\n",theHeadset.tts_language));
				#endif

				#ifdef T3ProductionTest
					if(theHeadset.ProductionData != 0)
						MAIN_DEBUG(("ProductionDat = T%d\n",theHeadset.ProductionData));
				#endif
				
			#endif

	            /*we have received the power on event- we have fully powered on*/
	            stateManagerPowerOn();             	
			
            /* set flag to indicate just powered up and may use different led pattern for connectable state
               if configured, flag is cleared on first successful connection */
			theHeadset.powerup_no_connection = TRUE;
            
            if(theHeadset.conf->timeouts.EncryptionRefreshTimeout_m != 0)
				MessageSendLater(&theHeadset.task, EventRefreshEncryption, 0, D_MIN(theHeadset.conf->timeouts.EncryptionRefreshTimeout_m));

		#ifdef QuicklyPowerOnandOff
			theHeadset.features.DisablePowerOffAfterPowerOn = FALSE;
		#endif
			
            if ( theHeadset.features.DisablePowerOffAfterPowerOn )
            {
                theHeadset.PowerOffIsEnabled = FALSE ;
                DEBUG(("DIS[%x]\n" , theHeadset.conf->timeouts.DisablePowerOffAfterPowerOnTime_s  )) ;

				#ifdef QuicklyPowerOnandOff
				MessageSendLater ( &theHeadset.task , EventEnablePowerOff , 0 , 100);
				#else
				MessageSendLater ( &theHeadset.task , EventEnablePowerOff , 0 , D_SEC ( theHeadset.conf->timeouts.DisablePowerOffAfterPowerOnTime_s ) ) ;
				#endif
			}
            else
            {
                theHeadset.PowerOffIsEnabled = TRUE ;
            }
			
#ifdef DEBUG_MALLOC
			printf("MAIN: Available SLOTS:[%d]\n" , VmGetAvailableAllocations() ) ;
#endif			
        break ;          
        case (EventPowerOff):    
            MAIN_DEBUG(("HS: PowerOff - En[%c]\n" , ((theHeadset.PowerOffIsEnabled) ? 'T':'F') )) ;
			
			#ifdef New_MMI
			if(lState == headsetLimbo)
				lIndicateEvent = FALSE;
			else
				lIndicateEvent = TRUE;
			#else
            /* don't indicate event if already in limbo state */
            if(lState == headsetLimbo) lIndicateEvent = FALSE ;
      		#endif
				
            if ( theHeadset.PowerOffIsEnabled )
            {
				#ifdef QuicklyPowerOnandOff
					if(lState != headsetLimbo)
					{
						if(theHeadset.theLEDTask->PatchEventColor != LED_COL_UNDEFINED)
						{
							#ifdef ROM_LEDS
			            		LedManagerResetLEDIndications ( ) ;            
							#endif
							theHeadset.theLEDTask->PatchEventColor = LED_COL_UNDEFINED;
						}
					}
				#endif

				#ifdef New_MMI
					#ifdef PSBlueLED
					PioSetPio(PS_blue_led, PS_blue_led_off);
					#endif
					PioSetPio(battery_low_io , PowerBlu_Connect);
					theHeadset.BHC612_BatteryLowState = PowerBlu_Connect;
				#endif

				#ifdef ATI_SPP
                	soundClearSppDisconnect();
				#endif

				stateManagerEnterLimboState();

				AuthResetConfirmationFlags();
				
	      		if (theHeadset.gMuted)
	        	{
	            	VolumeMuteOff();
	        	}

				headsetClearQueueudEvent();                
    
				if(theHeadset.conf->timeouts.EncryptionRefreshTimeout_m != 0)
    				MessageCancelAll ( &theHeadset.task, EventRefreshEncryption) ;
				
	       		MessageCancelAll ( &theHeadset.task , EventPairingFail) ;
	       		MessageCancelAll ( &theHeadset.task, EventCheckForLowBatt) ;

				#ifdef New_MMI
					if(ChargerIsChargerConnected())
						lIndicateEvent = FALSE ;				
				#endif
           
            }
            else
            {
                lIndicateEvent = FALSE ;
            }
        break ;
        case (EventInitateVoiceDial):

			#ifdef TTSLanguageflag
			if(theHeadset.BHC612_SelectLanguage == TRUE)
			{
				break;
			}
			#endif

                /*Toggle the voice dial behaviour depending on whether we are currently active*/
				MAIN_DEBUG(("TTS_ASR_Playing = %d\n",theHeadset.TTS_ASR_Playing));

			/*if((theHeadset.TTS_ASR_Playing == true) || (theHeadset.PressCallButton == FALSE && theHeadset.BHC612_TEMP == 1))*/
			if((theHeadset.TTS_ASR_Playing == true) || (theHeadset.PressCallButton == FALSE && theHeadset.BHC612_TEMP == 1) || (theHeadset.BHC612_LinkLoss == true))
			{
				MessageCancelAll(task , EventInitateVoiceDial );			
				MAIN_DEBUG(("HS: InitVoiceDial , Cancel\n"));
				break;
			}
				
			if ( theHeadset.PowerOffIsEnabled )
            {   
            	if(lState == headsetConnected)
            	{
					MAIN_DEBUG(("HS: InitVoiceDial [%c] [%d]\n", (theHeadset.conf->supp_features_local & HFP_VOICE_RECOGNITION ) ? 'T':'F' , theHeadset.VoiceRecognitionIsActive )) ;			 
	                if (theHeadset.VoiceRecognitionIsActive)
	                {
						#ifdef Rubidium
							if(theHeadset.TTS_ASR_Playing == false)
							{
								headsetCancelVoiceDial(hfp_primary_link) ;
	                    		lIndicateEvent = FALSE ;
								MAIN_DEBUG(("Cancel\n"));
							}
						#else
					    	headsetCancelVoiceDial(hfp_primary_link) ;
	                    	lIndicateEvent = FALSE ;
						#endif	
	                }
	                else
	                {         
						#ifdef Rubidium
							/*if(theHeadset.TTS_ASR_Playing == false)*/
							if((theHeadset.TTS_ASR_Playing == false) && (theHeadset.sco_sink == 0))
							{
								headsetInitiateVoiceDial (hfp_primary_link) ;	
								MAIN_DEBUG(("Open\n"));
							}
						#else
					     	headsetInitiateVoiceDial (hfp_primary_link) ;
						#endif
	                }
            	}
				else
				{					
					/*#ifdef Rubi_VoicePrompt*/
					#if 0
					if(theHeadset.Rubi_enable == 0)
					{
						/*if(theHeadset.TTS_ASR_Playing == false)*/
						if((theHeadset.TTS_ASR_Playing == false) && (theHeadset.tts_language == 0) && (theHeadset.BHC612_TEMP == 0))	
						{
							task = (TaskData *) &rubidium_tts_asr;	
							
							RUBI_DEBUG(("Rubi voice prompt > \\p=011\n" ));
							memset(TTS_text, 0, sizeof(TTS_text));
							
							strcpy(TTS_text, "\\p=011");	
							
							Kalimba_mode		= 1; /*TTS_ONLY_MODE*/
							
							ASR_active_model	= 0; /*RINGING_NODE*/
					
							theHeadset.TTS_ASR_Playing = true;
										
							AudioPlayTTS(	task, 
											(uint16)0,								/*TTS_CALLERID_NUMBER, */
											(uint8*)TTS_text,						/*num_string, */
											sizeof(TTS_text), 
											Language,	/*theHeadset.tts_language, */
											FALSE,									/*FALSE*/  
											theHeadset.codec_task,					/*theHeadset.codec_task, */
											0,										/*TonesGetToneVolume(FALSE), */
											0										/*theHeadset.features.stereo	*/
										);
							
							MAIN_DEBUG(("Check connection...,%d,%d\n",theHeadset.tts_language , theHeadset.BHC612_TEMP));
						}
					}
					#endif	
				}
            }
            else
            {
                lIndicateEvent = FALSE ;
            }
        break ;
        case (EventInitateVoiceDial_AG2):
            MAIN_DEBUG(("HS: InitVoiceDial AG2[%c] [%d]\n", (theHeadset.conf->supp_features_local & HFP_VOICE_RECOGNITION ) ? 'T':'F' , theHeadset.VoiceRecognitionIsActive )) ;            
                /*Toggle the voice dial behaviour depending on whether we are currently active*/
			if ( theHeadset.PowerOffIsEnabled )
            {
                
                if (theHeadset.VoiceRecognitionIsActive)
                {
                    headsetCancelVoiceDial(hfp_secondary_link) ;					
                    lIndicateEvent = FALSE ;
                }
                else
                {                
                    headsetInitiateVoiceDial(hfp_secondary_link) ;
                }            
            }
            else
            {
                lIndicateEvent = FALSE ;
            }
        break ;
        case (EventLastNumberRedial):
            MAIN_DEBUG(("HS: LNR\n" )) ;
            
            if ( theHeadset.PowerOffIsEnabled )
            {
                if (theHeadset.features.LNRCancelsVoiceDialIfActive)
                {
                    if ( theHeadset.VoiceRecognitionIsActive )
                    {
                        MessageSend(&theHeadset.task , EventInitateVoiceDial , 0 ) ;
                        lIndicateEvent = FALSE ;
                    }
                    else
                    {
                        /* LNR on AG 1 */
						headsetInitiateLNR(hfp_primary_link) ;
                    }
                }
                else
                {
                   /* LNR on AG 1 */
					headsetInitiateLNR(hfp_primary_link) ;
                }
			}
            else
            {
                lIndicateEvent = FALSE ;
            }
        break ;   
        case (EventLastNumberRedial_AG2):
            MAIN_DEBUG(("HS: LNR AG2\n" )) ;
			if ( theHeadset.PowerOffIsEnabled )
            {
                if (theHeadset.features.LNRCancelsVoiceDialIfActive)
                {
                    if ( theHeadset.VoiceRecognitionIsActive )
                    {
                        MessageSend(&theHeadset.task , EventInitateVoiceDial , 0 ) ;
                        lIndicateEvent = FALSE ;
                    }
                    else
                    {
                        /* LNR on AG 2 */
                        headsetInitiateLNR(hfp_secondary_link) ;
                    }
                }
                else
                {
                   /* LNR on AG 2 */
            	   headsetInitiateLNR(hfp_secondary_link) ;
                }
			}
            else
            {
                lIndicateEvent = FALSE ;
            }
        break ;  
        case (EventAnswer):
           /* printf("@@@ \n"); */
            MAIN_DEBUG(("HS: Answer,state=%d\n",lState)) ;
			
            /* Call the HFP lib function, this will determine the AT cmd to send
               depending on whether the profile instance is HSP or HFP compliant. */ 
            

            /* ensure a profile is connected, if not reschedule another answer attempt */
            if(!theHeadset.conf->no_of_profiles_connected)
            {
                /* not yet finished connecting, reschedule another go in 100mS */
                MessageSendLater (&theHeadset.task , EventAnswer , 0, 100 ) ;   
                /* don't indicate this event */
                lIndicateEvent = FALSE ;
            }
            /* connected so ok to answer */
            else
            {
                /* don't indicate event if not in incoming call state as answer event is used
                   for some of the multipoint three way calling operations which generate unwanted
                   tones */
                if(stateManagerGetState() != headsetIncomingCallEstablish)
                {
					lIndicateEvent = FALSE ;
                }
				else
				{
					#ifdef Rubi_TTS
						if(theHeadset.Rubi_enable == 0)
						{
							if(theHeadset.PressCallButton == TRUE)
							{
								#ifdef RubiTTSTerminate
								TTSTerminate();
								MAIN_DEBUG(("***Call TTSTerminate()\n"));
								
								/*#ifdef ChineseTTS*/
								#if 0
									MessageSend ( task , EventRestoreDefaults , 0) ;
								#endif
								
								#else
								AnswerCall_function();
								#endif
							}
							else
							{
								MAIN_DEBUG(("Answer call , PressCallButton : [T]\n"));
			                			headsetAnswerOrRejectCall( TRUE );
								theHeadset.PressCallButton = TRUE;
							}
						}
						else
						{
							headsetAnswerOrRejectCall( TRUE );
						}
					#else					
						headsetAnswerOrRejectCall( TRUE );
					#endif
            	}
            }

        break ;   
        case (EventReject):
			MAIN_DEBUG(("HS: Reject\n" )) ;
            /* Reject incoming call - only valid for instances of HFP */
			
			#ifdef Rubi_TTS
				/*if(theHeadset.Rubi_enable == 0)*/
				if((theHeadset.Rubi_enable == 0) && (lState == headsetIncomingCallEstablish))	
				{
					if(theHeadset.PressCallButton == TRUE)
					{
						/*#ifdef ChineseTTS*/
						#if 0
							MessageSend ( task , EventRestoreDefaults , 0) ;
						#endif
						IgnoreCall_function();
					}
					else
					{
						MAIN_DEBUG(("Reject call, PressCallButton : [T] \n"));						
						headsetAnswerOrRejectCall( FALSE );
						theHeadset.PressCallButton = TRUE;
					}
				}
				else
				{
					headsetAnswerOrRejectCall( FALSE );
				}
			#else
            	headsetAnswerOrRejectCall( FALSE );
			#endif
			
			break ;

        case (EventCancelEnd):
			MAIN_DEBUG(("HS: CancelEnd\n" )) ;
            /* Terminate the current ongoing call process */
            headsetHangUpCall();
        break ;
        case (EventTransferToggle):
            MAIN_DEBUG(("HS: Transfer\n" )) ;
            /* Don't indicate this event if it's an HF to AG transfer (B-48360). */
            lIndicateEvent = headsetTransferToggle(id);
        break ;
        case EventCheckForAudioTransfer :
	        MAIN_DEBUG(("HS: Check Aud Tx\n")) ;    
            headsetCheckForAudioTransfer();
			
            #ifdef BHC612
			MAIN_DEBUG(("SCO = %d\n",(uint16)theHeadset.sco_sink));
            #endif
						
	        break ;
        case (EventToggleMute):
            MAIN_DEBUG(("EventToggleMute\n")) ;
            VolumeToggleMute();
        break ;
#ifdef ENABLE_ENERGY_FILTER
        case (EventEnableIIR):
            MAIN_DEBUG(("EventEnableIIR\n")) ;
            if(!theHeadset.iir_enabled)
                Filter_On();
            theHeadset.iir_enabled = 1;
        break ;   
        case (EventDisableIIR):
            MAIN_DEBUG(("EventDisableIIR\n")) ;
            if(theHeadset.iir_enabled)
                Filter_Off();
            theHeadset.iir_enabled = 0;
        break ;
#endif
        case EventMuteOn :
            MAIN_DEBUG(("EventMuteOn\n")) ;
            VolumeMuteOn();
        break ;
        case EventMuteOff:
            MAIN_DEBUG(("EventMuteOff\n")) ;
            VolumeMuteOff();                      
        break ;
        case (EventVolumeUp):      
            /*MAIN_DEBUG(("EventVolumeUp\n")) ;*/

		#ifdef TTSLanguageflag
		if(theHeadset.BHC612_SelectLanguage == TRUE)
		{
			break;	
		}
		#endif

			
			if(theHeadset.VoicePromptNotComplete)
			{
				if(lState == headsetA2DPStreaming)
				{
					theHeadset.VoicePromptNotComplete = FALSE;
				}
				break;
			}
			
		
    		if ( theHeadset.gVolButtonsInverted )
    		{		/*do not indicate the original event*/
	 			lIndicateEvent = FALSE ;	 			 

				/*indicate the opposite event*/
        	   	LEDManagerIndicateEvent ( EventVolumeDown ) ;
                TonesPlayEvent ( EventVolumeDown ) ;
				/*change the volume in the opposite dir*/						
		       	VolumeDown();
    		}
    		else
    		{				
           		VolumeUp();
				MAIN_DEBUG(("EventVolumeUp\n")) ;

				#ifdef NewChargeMMI
				theHeadset.BHC612_Chargefull = 0;
				PioSetPio(battery_low_io , PowerBlu_Connect);
				theHeadset.BHC612_BatteryLowState = PowerBlu_Connect;
				#endif
        	}	
			
        break ;
        case (EventVolumeDown):    
			
			if(theHeadset.VoicePromptNotComplete)
			{
				if(lState == headsetA2DPStreaming)
				{
					theHeadset.VoicePromptNotComplete = FALSE;
				}
				break;
			}
			
	       	if ( theHeadset.gVolButtonsInverted )
    		{		/*do not indicate the original event*/
	 			lIndicateEvent = FALSE ;	 			 
	 				/*indicate the opposite event*/
        	 	LEDManagerIndicateEvent ( EventVolumeUp ) ;
                TonesPlayEvent ( EventVolumeUp ) ;
					/*change the volume in the opposite dir*/
		    	VolumeUp();
    		}
    		else
    		{
            	VolumeDown();

				MAIN_DEBUG(("EventVolumeDown\n")) ;

				#ifdef NewChargeMMI
				theHeadset.BHC612_Chargefull = 0;
				PioSetPio(battery_low_io , PowerBlu_Connect);
				theHeadset.BHC612_BatteryLowState = PowerBlu_Connect;
				#endif
        	}	
						
        break ;                        
        case (EventEnterPairing):
            MAIN_DEBUG(("HS: EnterPair [%d]\n" , lState )) ;

		#ifdef TTSLanguageflag
		if(theHeadset.BHC612_SelectLanguage == TRUE)
		{
			break;	
		}
		#endif

            /*go into pairing mode*/ 
            if (( lState != headsetLimbo) && (lState != headsetConnDiscoverable ))
            {
            	if(lState == headsetConnectable)
                	stateManagerEnterConnDiscoverableState();      
				
				#ifdef Rubi_VoicePrompt
					if(theHeadset.Rubi_enable == 0)
					{
						task = (TaskData *) &rubidium_tts_asr;	
						if(theHeadset.conf->no_of_profiles_connected < 2)
						{					
							memset(TTS_text, 0, sizeof(TTS_text));
							
						   	if(lState == headsetConnectable)
						   	{
								strcpy(TTS_text, "\\p=001");

								RUBI_DEBUG(("Rubi voice prompt > \\p=001\n" ));

								Kalimba_mode 		= 1; /*TTS_ONLY_MODE*/
						
								ASR_active_model	= 0; /*RINGING_NODE*/

								theHeadset.TTS_ASR_Playing = true;
									
								AudioPlayTTS(	task, 
												(uint16)0, 								/*TTS_CALLERID_NUMBER, */
												(uint8*)TTS_text,					 	/*num_string, */
												sizeof(TTS_text), 
												Language,	/*theHeadset.tts_language, */
												FALSE,									/*FALSE*/  
												theHeadset.codec_task,					/*theHeadset.codec_task, */
												0, 										/*TonesGetToneVolume(FALSE), */
												0										/*theHeadset.features.stereo	*/
											);

								if(stateManagerGetPdlSize() == 0)
									theHeadset.DockLED = 2;
								else
									theHeadset.DockLED = 0;
						   	}
						}
						else if(theHeadset.conf->no_of_profiles_connected >= 2)
						{	
							RUBI_DEBUG(("Rubi voice prompt > \\p=003\n" ));
							memset(TTS_text, 0, sizeof(TTS_text));						
						   	strcpy(TTS_text, "\\p=003\\p=047\\p=010");	

							Kalimba_mode 		= 1; /*TTS_ONLY_MODE*/
						
							ASR_active_model	= 0; /*RINGING_NODE*/

							theHeadset.TTS_ASR_Playing = true;
										
							AudioPlayTTS(	task, 
											(uint16)0, 								/*TTS_CALLERID_NUMBER, */
											(uint8*)TTS_text,					 	/*num_string, */
											sizeof(TTS_text), 
											Language,	/*theHeadset.tts_language, */
											FALSE,									/*FALSE*/  
											theHeadset.codec_task,					/*theHeadset.codec_task, */
											0, 										/*TonesGetToneVolume(FALSE), */
											0										/*theHeadset.features.stereo	*/
										);
						}
					}
				#endif	          
            }
            else
            {
                lIndicateEvent = FALSE ;
		#ifdef New_MMI
			/*Cancel pairing*/
			if(lState == headsetConnDiscoverable)
				#ifdef Rubidium
					#ifdef Rubi_VoicePrompt
					if(theHeadset.Rubi_enable == 0)
					{
						#ifdef Rubi_VP_MinorChg						
							if((theHeadset.TTS_ASR_Playing == true) || (theHeadset.DockLED != 0))
							{
								TTSTerminate();
								theHeadset.TTS_ASR_Playing = false;
							}
							theHeadset.DockLED = 0;
						#endif
					
						task = (TaskData *) &rubidium_tts_asr;							
						
						Kalimba_mode 		= 1; /*TTS_ONLY_MODE*/
						
						ASR_active_model	= 0; /*RINGING_NODE*/

						theHeadset.TTS_ASR_Playing = true;

						theHeadset.DockLED = 0;

						memset(TTS_text, 0, sizeof(TTS_text));
						strcpy(TTS_text, "\\p=010");

						RUBI_DEBUG(("Rubi voice prompt > \\p=010\n" ));
						
						AudioPlayTTS(	task, 
										(uint16)0, 								/*TTS_CALLERID_NUMBER, */
										(uint8*)TTS_text,					 	/*num_string, */
										sizeof(TTS_text), 
										Language,	/*theHeadset.tts_language, */
										FALSE,									/*FALSE*/  
										theHeadset.codec_task,					/*theHeadset.codec_task, */
										0, 										/*TonesGetToneVolume(FALSE), */
										0										/*theHeadset.features.stereo	*/
									);
					}
					#endif
					stateManagerEnterConnectableState(TRUE); 
				#else
					MessageSend ( task , EventPairingFail , 0) ;
				#endif
		#endif
            }
        break ;
        case (EventPairingFail):
            /*we have failed to pair in the alloted time - return to the connectable state*/
            MAIN_DEBUG(("HS: Pairing Fail\n")) ;
            if (lState != headsetTestMode)
			{
				#ifdef Rubi_VP_MinorChg
				if(theHeadset.DockLED != 0)
					theHeadset.DockLED = 0;
				#endif
			
				#ifdef Rubi_VoicePrompt
					if(theHeadset.Rubi_enable == 0)
					{
						task = (TaskData *) &rubidium_tts_asr;							
						
						Kalimba_mode 		= 1; /*TTS_ONLY_MODE*/
						
						ASR_active_model	= 0; /*RINGING_NODE*/

						theHeadset.TTS_ASR_Playing = true;

						memset(TTS_text, 0, sizeof(TTS_text));
						strcpy(TTS_text, "\\p=011\\p=047\\p=010");

						RUBI_DEBUG(("Rubi voice prompt > \\p=011\\p=047\\p=010\n" ));
						
						AudioPlayTTS(	task, 
										(uint16)0, 								/*TTS_CALLERID_NUMBER, */
										(uint8*)TTS_text,					 	/*num_string, */
										sizeof(TTS_text), 
										Language,	/*theHeadset.tts_language, */
										FALSE,									/*FALSE*/  
										theHeadset.codec_task,					/*theHeadset.codec_task, */
										0, 										/*TonesGetToneVolume(FALSE), */
										0										/*theHeadset.features.stereo	*/
									);
					}
				#endif
				
				switch (theHeadset.features.PowerDownOnDiscoTimeout)
				{
					case PAIRTIMEOUT_POWER_OFF:
						MessageSend ( task , EventPowerOff , 0) ;
						break;
					case PAIRTIMEOUT_POWER_OFF_IF_NO_PDL:
						/* Power off if no entries in PDL list */
						if (stateManagerGetPdlSize() == 0)
						{
							MessageSend ( task , EventPowerOff , 0) ;
						}
						else
						{
							stateManagerEnterConnectableState(TRUE); 
						}
						break;
					case PAIRTIMEOUT_CONNECTABLE:
					default:
						stateManagerEnterConnectableState(TRUE);          
				}
			}
            /* have attempted to connect following a power on and failed so clear the power up connection flag */                
            theHeadset.powerup_no_connection = FALSE;

        break ;                        
        case ( EventPairingSuccessful):
            MAIN_DEBUG(("HS: Pairing Successful\n")) ;
			#ifdef BHC612
				theHeadset.BHC612_PairSuccess = 1;
			#endif

			#ifdef Rubi_VP_MinorChg 					
				if((theHeadset.TTS_ASR_Playing == true) || (theHeadset.DockLED != 0))
				{
					TTSTerminate();
					theHeadset.TTS_ASR_Playing = false;
				}
				theHeadset.DockLED = 0;
			#endif

            if (lState == headsetConnDiscoverable)
                stateManagerEnterConnectableState(FALSE);
			
			#ifdef Rubi_VoicePrompt
			if(theHeadset.Rubi_enable == 0)
			{
				task = (TaskData *) &rubidium_tts_asr;
			
				memset(TTS_text, 0, sizeof(TTS_text));

				/*#ifdef Rubi_VP_MinorChg*/
				#if 0
				strcpy(TTS_text, "\\p=009\\p=046\\p=015");
				RUBI_DEBUG(("Rubi voice prompt > \\p=009\\p=046\\p=015\n" ));
				#else
				strcpy(TTS_text, "\\p=009");
				RUBI_DEBUG(("Rubi voice prompt > \\p=009\n" ));
				#endif
				
				Kalimba_mode 		= 1; /*TTS_ONLY_MODE*/
				
				ASR_active_model	= 0; /*RINGING_NODE*/

				theHeadset.TTS_ASR_Playing = true;
			
				AudioPlayTTS(	task, 
								(uint16)0, 								/*TTS_CALLERID_NUMBER, */
								(uint8*)TTS_text,					 	/*num_string, */
								sizeof(TTS_text), 
								Language,	/*theHeadset.tts_language, */
								FALSE,									/*FALSE*/  
								theHeadset.codec_task,					/*theHeadset.codec_task, */
								0, 										/*TonesGetToneVolume(FALSE), */
								0										/*theHeadset.features.stereo	*/
							);
			}
			#endif
        break ;
		case ( EventConfirmationAccept ):
			MAIN_DEBUG(("HS: Pairing Correct Res\n" )) ;
			headsetPairingAcceptRes();
		break;
		case ( EventConfirmationReject ):
			MAIN_DEBUG(("HS: Pairing Reject Res\n" )) ;
			headsetPairingRejectRes();
		break;
        case ( EventEstablishSLC ) :         
			MAIN_DEBUG(("HS: EventEstablishSLC,%d,%d\n", lState , (uint16)theHeadset.sco_sink));

			#ifdef TTSLanguageflag
			if(theHeadset.BHC612_SelectLanguage == TRUE)
			{
				break;
			}
			#endif
			
			#if 1
			if(theHeadset.BHC612_LinkLoss == true)
			{
				MAIN_DEBUG(("Link loss :True\n"));
			}
			else
			{
				MAIN_DEBUG(("Link loss :False\n"));
			}
			#endif
			
			/* check we are not already connecting before starting */
            {                
				/*if(lState == headsetConnected)Double press call button to check connection...*/
				if(lState == headsetConnected)/*Double press call button to check connection...*/
				{
					if(theHeadset.VoiceRecognitionIsActive == 0)
					{
						#ifdef Rubi_VoicePrompt
					 	if((theHeadset.sco_sink == 0) && (theHeadset.Rubi_enable == 0) && (theHeadset.TTS_ASR_Playing == false))
					 	{
							task = (TaskData *) &rubidium_tts_asr;	

							memset(TTS_text, 0, sizeof(TTS_text));  

							
							if(theHeadset.BHC612_LinkLoss == false)
							{
								if(theHeadset.conf->no_of_profiles_connected < 2)
								{					
									strcpy(TTS_text, "\\p=029");/*Phone conected*/
									RUBI_DEBUG(("EventEstablishSLC,Rubi voice prompt > \\p=029\n" ));
								}
								else if(theHeadset.conf->no_of_profiles_connected >= 2)
								{			
									strcpy(TTS_text, "\\p=003");/*Two devices connected*/ 	
									RUBI_DEBUG(("EventEstablishSLC,Rubi voice prompt > \\p=003\n" ));
								}
							}
							else
							{
								/*strcpy(TTS_text, "\\p=011");No phone detected*/
								strcpy(TTS_text, "\\p=015\\p=047\\p=047\\p=011");/*Searching...pause,pause,No phone detected*/	
								RUBI_DEBUG(("EventEstablishSLC,Link loss,Rubi VP > \\p=015\\p=047\\p=047\\p=011\n" ));
							}
																	   
							Kalimba_mode		= 1; /*TTS_ONLY_MODE*/
																		 
							ASR_active_model	= 0; /*RINGING_NODE*/

							theHeadset.TTS_ASR_Playing = true;
																			 
							AudioPlayTTS(	task, 
										(uint16)0, 	/*TTS_CALLERID_NUMBER, */
										(uint8*)TTS_text, /*num_string, */
										sizeof(TTS_text), 
										Language,	/*theHeadset.tts_language, */
										FALSE,									/*FALSE*/  
										theHeadset.codec_task,					/*theHeadset.codec_task, */
										0, 										/*TonesGetToneVolume(FALSE), */
										0										/*theHeadset.features.stereo	*/
										);	

				 		}
						#endif
					}
					break;
				}
				
			#ifdef Rubi_VoicePrompt
			
			/*if(theHeadset.TTS_ASR_Playing == false)*/
			/*if((theHeadset.TTS_ASR_Playing == false) && (theHeadset.Rubi_enable == 0))*/
			if((theHeadset.TTS_ASR_Playing == false) && (theHeadset.Rubi_enable == 0) && (theHeadset.tts_language == 0 && theHeadset.BHC612_TEMP == 0))	
			{
				#if 1
				task = (TaskData *) &rubidium_tts_asr;							
				
				Kalimba_mode 		= 1; /*TTS_ONLY_MODE*/
				
				ASR_active_model	= 0; /*RINGING_NODE*/

				theHeadset.TTS_ASR_Playing = true;

				memset(TTS_text, 0, sizeof(TTS_text));

				/*if(stateManagerGetPdlSize() == 0)PDL = 0*/
				if((stateManagerGetPdlSize()== 0) || (theHeadset.BHC612_LinkLoss == true))/*PDL is empty or Link loss*/
				{
					strcpy(TTS_text, "\\p=015\\p=047\\p=047\\p=011");	
					
					/*RUBI_DEBUG(("Rubi voice prompt > \\p=015\p=047\\p=047\\p=011\n"));*/

					AudioPlayTTS(	task, 
								(uint16)0, 								/*TTS_CALLERID_NUMBER, */
								(uint8*)TTS_text,					 	/*num_string, */
								sizeof(TTS_text), 
								Language,	/*theHeadset.tts_language, */
								FALSE,									/*FALSE*/  
								theHeadset.codec_task,					/*theHeadset.codec_task, */
								0, 										/*TonesGetToneVolume(FALSE), */
								0										/*theHeadset.features.stereo	*/
							);

					
					MAIN_DEBUG(("XXX,PDL is empty or Link loss \n" ));
					break;
				}
				else
				{
					strcpy(TTS_text, "\\p=015");	
					RUBI_DEBUG(("Rubi voice prompt > \\p=015\n" ));

					AudioPlayTTS(	task, 
								(uint16)0, 								/*TTS_CALLERID_NUMBER, */
								(uint8*)TTS_text,					 	/*num_string, */
								sizeof(TTS_text), 
								Language,	/*theHeadset.tts_language, */
								FALSE,									/*FALSE*/  
								theHeadset.codec_task,					/*theHeadset.codec_task, */
								0, 										/*TonesGetToneVolume(FALSE), */
								0										/*theHeadset.features.stereo	*/
							);	
				}
				
				#endif
			}
			else
			{
				MAIN_DEBUG(("Error!,%d,%d,%d,%d\n",theHeadset.TTS_ASR_Playing,theHeadset.Rubi_enable,theHeadset.tts_language,theHeadset.BHC612_TEMP));

				if(theHeadset.TTS_ASR_Playing)
				{
					TTSTerminate();
					UnloadRubidiumEngine();
					theHeadset.TTS_ASR_Playing = false;
				}
				theHeadset.tts_language = 0;
				theHeadset.BHC612_TEMP = 0;
				break;
			}
			#endif

			MAIN_DEBUG(("EventEstablishSLC\n")) ;
                
                /* if the scroll PDL for preset number of connection attempts before giving is set
                   		retrieve the number of connection attempts to use */
                theHeadset.conf->NoOfReconnectionAttempts = theHeadset.conf->timeouts.ReconnectionAttempts ;

		
                slcEstablishSLCRequest() ;
                
                /* don't indicate the event at first power up if the use different event at power on
	                   feature bit is enabled, this enables the establish slc event to be used for the second manual
	                   connection request */
                if(stateManagerGetState() == headsetConnectable)
                {
                    /* send message to do indicate a start of paging process when in connectable state */
                    MessageSend(&theHeadset.task, EventStartPagingInConnState ,0);  
                }   
            }  
        break ;
        case ( EventRssiPair ):
            MAIN_DEBUG(("HS: RSSI Pair\n"));
            theHeadset.rssi_action = rssi_pairing;
            slcStartInquiry();
        break;
        case ( EventRssiPairReminder ):
            MAIN_DEBUG(("HS: RSSI Pair Reminder\n"));
            MessageSendLater(&theHeadset.task, EventRssiPairReminder, 0, D_SEC(INQUIRY_REMINDER_TIMEOUT_SECS));
        break;
        case ( EventRssiPairTimeout ):
            MAIN_DEBUG(("HS: RSSI Pair Timeout\n"));
            slcStopInquiry();
        break;
		case ( EventRefreshEncryption ):
			MAIN_DEBUG(("HS: Refresh Encryption\n"));
			{
				uint8 k;
                Sink sink;
                Sink audioSink;
				/* For each profile */
				for(k=0;k<MAX_PROFILES;k++)
				{
					MAIN_DEBUG(("Profile %d: ",k));
					/* If profile is connected */
					if((HfpLinkGetSlcSink((k + 1), &sink))&&(sink))
					{
						/* If profile has no valid SCO sink associated with it */
                        HfpLinkGetAudioSink((k + hfp_primary_link), &audioSink);
						if(!SinkIsValid(audioSink))
						{
							MAIN_DEBUG(("Key Refreshed\n"));
							/* Refresh the encryption key */
							ConnectionSmEncryptionKeyRefreshSink(sink);
						}
#ifdef DEBUG_MAIN
						else
						{
							MAIN_DEBUG(("Key Not Refreshed, SCO Active\n"));
						}
					}
					else
					{
						MAIN_DEBUG(("Key Not Refreshed, SLC Not Active\n"));
#endif
					}
				}
				MessageSendLater(&theHeadset.task, EventRefreshEncryption, 0, D_MIN(theHeadset.conf->timeouts.EncryptionRefreshTimeout_m));
			}
		break;
        
        /* 60 second timer has triggered to disable connectable mode in multipoint
            connection mode */
        case ( EventConnectableTimeout ) :
            /* only disable connectable mode if at least one hfp instance is connected */
            if(theHeadset.conf->no_of_profiles_connected)
            {
                MAIN_DEBUG(("SM: disable Connectable \n" ));
                /* disable connectability */
                headsetDisableConnectable();
            }
	    break;
        
        case ( EventBatteryLevelRequest ) :
		  MAIN_DEBUG(("EventBatteryLevelRequest\n")) ;
		
          /* take an immediate reading of the battery voltage only, returning to any preset
             timed readings */
		  BatteryUserInitiatedRead();
        
        
		break;
        case ( EventLEDEventComplete ) :
            /*the message is a ptr to the event we have completed*/
            MAIN_DEBUG(("HS : LEDEvCmp[%x]\n" ,  (( LMEndMessage_t *)message)->Event  )) ;
                        
            switch ( (( LMEndMessage_t *)message)->Event )
            {
            	#ifdef Rubi_VP_MinorChg
				case (EventPowerOn):
				{
					if(stateManagerGetState() == headsetConnDiscoverable)
					{
						#ifdef PSFWVer
						Write_SW_Version();
						#endif
						
						if(theHeadset.Rubi_enable == 0)
						{
							task = (TaskData *) &rubidium_tts_asr;	
							memset(TTS_text, 0, sizeof(TTS_text));

							strcpy(TTS_text, "\\p=000\\p=047\\p=001\\p=002\\p=005\\p=006\\p=007\\p=008");

							RUBI_DEBUG(("Rubi voice prompt > \\p=000\\p=047\\p=001\\p=002\\p=005\\p=006\\p=007\\p=008\n" ));
							
							Kalimba_mode 		= 1; /*TTS_ONLY_MODE*/
	        
	       					ASR_active_model	= 0; /*RINGING_NODE*/

							theHeadset.TTS_ASR_Playing = true;

							AudioPlayTTS(	task, 
											(uint16)0, 								/*TTS_CALLERID_NUMBER, */
											(uint8*)TTS_text,					 	/*num_string, */
											sizeof(TTS_text), 
											Language,	/*theHeadset.tts_language, */
											FALSE,									/*FALSE*/  
											theHeadset.codec_task,					/*theHeadset.codec_task, */
											0, 										/*TonesGetToneVolume(FALSE), */
											0										/*theHeadset.features.stereo	*/
										);

							theHeadset.DockLED = 2;
						}
					}
				}
				break;
				#endif
                case (EventResetPairedDeviceList) :
                {      /*then the reset has been completed*/
                    MessageSend(&theHeadset.task , EventResetComplete , 0 ) ;
   
                        /*Reboot if required*/
                    if ((theHeadset.features.RebootAfterReset )&&
                        (stateManagerGetState() > headsetLimbo )) 

                    {
                        MAIN_DEBUG(("HS: Reboot After Reset\n")) ;
                        MessageSend ( &theHeadset.task , EventPowerOff , 0 ) ;
                    }
                }
                break ;            				
			  	case EventPowerOff:
			  	{
					/*allows a reset of the device for those designs which keep the chip permanently powered on*/
					if (theHeadset.features.ResetAfterPowerOffComplete)
						Panic() ;
			  	}
			  	break ;
                default: 
                break ;
            }
            
#ifdef ROM_LEDS          
            if (theHeadset.features.QueueLEDEvents )
            {
                    /*if there is a queueud event*/
                if (theHeadset.theLEDTask->Queue.Event1)
                {
                    MAIN_DEBUG(("HS : Play Q'd Ev [%x]\n", (EVENTS_MESSAGE_BASE + theHeadset.theLEDTask->Queue.Event1)  ));
                    MAIN_DEBUG(("HS : Queue [%x][%x][%x][%x]\n", theHeadset.theLEDTask->Queue.Event1,
                                                              theHeadset.theLEDTask->Queue.Event2,
                                                              theHeadset.theLEDTask->Queue.Event3,
                                                              theHeadset.theLEDTask->Queue.Event4
                                                                    
                                                                ));
                
                    LEDManagerIndicateEvent ( (EVENTS_MESSAGE_BASE + theHeadset.theLEDTask->Queue.Event1) ) ;
    
                        /*shuffle the queue*/
                    theHeadset.theLEDTask->Queue.Event1 = theHeadset.theLEDTask->Queue.Event2 ;
                    theHeadset.theLEDTask->Queue.Event2 = theHeadset.theLEDTask->Queue.Event3 ;
                    theHeadset.theLEDTask->Queue.Event3 = theHeadset.theLEDTask->Queue.Event4 ;
                    theHeadset.theLEDTask->Queue.Event4 = 0x00 ;
                }	
                else
                {
                    /* restart state indication */
                    LEDManagerIndicateState ( stateManagerGetState () ) ;
                }
            }
            else
                LEDManagerIndicateState ( stateManagerGetState () ) ;
#endif
                
        break ;   
        case (EventAutoSwitchOff):
            MAIN_DEBUG(("HS: Auto S Off[%d] sec elapsed\n" , theHeadset.conf->timeouts.AutoSwitchOffTime_s )) ;			
            switch ( lState )
            {   
                case headsetLimbo:
					#ifdef BHC612
						/*PowerSwitch is off*/
						MAIN_DEBUG(("EventPowerOff\n"));
						MAIN_DEBUG(("HS: PowerOff - En[%c]\n" , ((theHeadset.PowerOffIsEnabled) ? 'T':'F') )) ;

						if(theHeadset.PowerOffIsEnabled == false)
							theHeadset.PowerOffIsEnabled = true;

						#ifdef PSBlueLED
						PioSetPio(PS_blue_led, PS_blue_led_off);	
						#endif
						
						MessageSend ( task , EventPowerOff , 0) ;
						break;
					#endif
                case headsetConnectable:
                case headsetConnDiscoverable:				
            	break;
                case headsetConnected:
				break;
                case headsetOutgoingCallEstablish:   
                case headsetIncomingCallEstablish:   
                case headsetActiveCallSCO:            
                case headsetActiveCallNoSCO:             
                case headsetTestMode:
                    break ;
                default:
                    MAIN_DEBUG(("HS : UE ?s [%d]\n", lState));
                    break ;
            }
        break;
#ifdef New_MMI		
		case EventUnused2:
		{
			/*if(theHeadset.BHC612_DOCKMMI == 0 && theHeadset.theLEDTask->PatchEventColor == LED_COL_UNDEFINED)*/
			if((theHeadset.BHC612_DOCKMMI == 0) && (theHeadset.theLEDTask->PatchEventColor == LED_COL_UNDEFINED) && (theHeadset.DockLED == 0))	
		 	{
				 LedsIndicateNoState();
				 LED_Reconfig();				
				 theHeadset.BHC612_DOCKMMI = 1;
				 theHeadset.BHC612_UNDOCKMMI = 0;
				 theHeadset.DockLED = 0;				
				 LEDManagerIndicateState (stateManagerGetState() ) ;				
		 	}
			else
			{
				/*
				MessageSendLater(&theHeadset.task , EventUnused2 , 0 , D_SEC(1));
				MAIN_DEBUG(("===Event LED not complete!===\n" ));
				*/
				
				#ifdef ROM_LEDS
	            	LedManagerResetLEDIndications ( ) ;            
				#endif
				theHeadset.theLEDTask->PatchEventColor = LED_COL_UNDEFINED;
				theHeadset.BHC612_TEMP = 0;
				theHeadset.BHC612_DOCKMMI = 0;
				theHeadset.BHC612_UNDOCKMMI = 1;
				theHeadset.DockLED = 0;		
				MessageCancelAll ( &theHeadset.task , EventUnused2) ;
				/*MAIN_DEBUG(("### Cancel LED event LED!\n" ));*/
			}
		}
		break;
		case EventUnused3:
		{			
			/*if(ChargerIsChargerConnected())*/
			if((ChargerIsChargerConnected() == TRUE) && (theHeadset.m_ChargerRealConnect == true))				
			{
				theHeadset.m_ChargerRealConnect = false;
				
				MAIN_DEBUG(("HS: Charger Connected,Batt level = %d\n",theHeadset.batt_level));
				
				theHeadset.BHC612_PSConnected = 1;
			
				if(theHeadset.theLEDTask->gLEDSEnabled == FALSE)
				{
					MessageSend ( &theHeadset.task , EventEnableLEDS , 0 ) ; 
				}
			
	     		powerManagerChargerConnected();

		        if ( lState == headsetLimbo )
		        { 
		        	stateManagerUpdateLimboState();
		        }

				if(lState != headsetLimbo)
		 		{		
		 			/*if(theHeadset.BHC612_DOCKMMI == 0)*/
					/*if(theHeadset.BHC612_DOCKMMI == 0 && theHeadset.theLEDTask->PatchEventColor == LED_COL_UNDEFINED)*/
					if(theHeadset.BHC612_DOCKMMI == 0 && theHeadset.theLEDTask->PatchEventColor == LED_COL_UNDEFINED && theHeadset.BHC612_Chargefull == 0)
		 			{
		 				#ifdef NewChargeMMI
							if(theHeadset.tts_language < 2)
							{
								#if 1
								theHeadset.battery_low_state = FALSE;
								#endif
								theHeadset.BHC612_DOCKMMI = 1;
								theHeadset.BHC612_UNDOCKMMI = 0;
								theHeadset.DockLED = 0;				
								LEDManagerIndicateState (stateManagerGetState() ) ;	
								MAIN_DEBUG((" Charging LED!(30 sec),%d\n",theHeadset.tts_language));
								theHeadset.tts_language = 0;
							}
							else
							{
								theHeadset.tts_language = 0;
								PioSetPio(battery_low_io , PowerBlu_Disconnect);
								theHeadset.BHC612_Chargefull =1;	
								theHeadset.BHC612_BatteryLowState = PowerBlu_Disconnect;
								MAIN_DEBUG(("###222 Battery low : High , Charge stop!"));
							}
						#else
							theHeadset.BHC612_DOCKMMI = 1;
							theHeadset.BHC612_UNDOCKMMI = 0;
							theHeadset.DockLED = 0;				
							LEDManagerIndicateState (stateManagerGetState() ) ;	
							MAIN_DEBUG((" Charging LED!(30 sec)\n"));
						#endif
		 			}
					else
					{
						/*MAIN_DEBUG(("***Event LED not complete!***\n" ));*/
						
						/*MessageSendLater(&theHeadset.task , EventUnused2 , 0 , D_SEC(1));*/
						MessageSendLater(&theHeadset.task , EventUnused2 , 0 , D_SEC(3));

						theHeadset.BHC612_TEMP = 0;
						/*
						if(theHeadset.BHC612_TEMP < 4)
						{
							theHeadset.BHC612_TEMP++;
							MessageSendLater(&theHeadset.task , EventUnused2 , 0 , D_SEC(1));
						}
						else
						{
							#ifdef ROM_LEDS
	            				LedManagerResetLEDIndications ( ) ;            
							#endif
							theHeadset.theLEDTask->PatchEventColor = LED_COL_UNDEFINED;
							theHeadset.BHC612_TEMP = 0;
							theHeadset.BHC612_DOCKMMI = 0;
							theHeadset.BHC612_UNDOCKMMI = 1;
							theHeadset.DockLED = 0;		
						}
						*/
					}

					#ifdef New_MMI
						/*MAIN_DEBUG(("Headet state = %d\n",lState));*/
						if(lState == headsetActiveCallSCO || lState == headsetActiveCallNoSCO)
						{
							/*MessageSendLater(&theHeadset.task , EventTransferToggle, 0 , 500);
							MAIN_DEBUG((" Transfer Audio\n" ));*/

							/*Assembla ticket #79*/
							MessageSendLater(&theHeadset.task , EventCancelEnd, 0 , 500);
							MAIN_DEBUG(("Cancel a single active call\n" ));
						}
					#endif
					
					#ifdef ENABLE_AVRCP
						#ifdef A2DP_Dock
						/*if(GetA2DPState() == true)*/
						if((GetA2DPState() == true) && (lState == headsetA2DPStreaming))
						{
							headsetAvrcpPause();
							MAIN_DEBUG(("HS: A2DP,Charger Connected, EventAvrcpPlayPause\n"));
						}
						#endif
					#endif
				}
				else
				{
				}
        	}
			else
			{
				if ( lState == headsetLimbo )
		       	{ 
		        	stateManagerUpdateLimboState();
		    	}

				theHeadset.m_ChargerRealConnect = false;
			}
			break;
		}
#endif
        case (EventChargerConnected):
        {
#ifdef New_MMI
			if(lState == headsetLimbo)
			{
				ChargerConfigure(CHARGER_SUPPRESS_LED0, TRUE);
				#ifdef NewPSBlueLED
					PioSetPio(PS_blue_led, PS_blue_led_off);
				#endif
				powerManagerChargerConnected();
				stateManagerUpdateLimboState();
			}
			else
			{
				/*#ifdef ENABLE_AVRCP
					#ifdef A2DP_Dock
					if(GetA2DPState() == true)
					{
						#if 0
						MessageCancelAll(&theHeadset.task , EventAvrcpPlayPause);
						MessageSend( &theHeadset.task , EventAvrcpPlayPause , 0) ;
						#endif
						headsetAvrcpPause();
						MAIN_DEBUG(("HS: Charger Connected, EventAvrcpPlayPause\n"));
					}
					#endif
				#endif*/
	            /*MAIN_DEBUG(("HS: Charger Connected\n"));*/

				MessageSendLater(&theHeadset.task , EventUnused3 , 0 , ChargerConnectDebounceTime);

				theHeadset.BHC612_TEMP = 0;

				theHeadset.m_ChargerRealConnect = true;

				if(theHeadset.m_ChargerRealDisconnect == true)
				{
					MessageCancelAll(task , EventUnused4 );
					theHeadset.m_ChargerRealDisconnect = false;
					/*MAIN_DEBUG(("Cancel EventChargerDisonnected!\n"));*/
					#ifdef NewChargeMMI
					if(theHeadset.tts_language < 16)
						theHeadset.tts_language++;
					#endif
				}
			}	
#else
           	MAIN_DEBUG(("HS: Charger Connected\n"));
			powerManagerChargerConnected();
            if ( lState == headsetLimbo )
            { 
                stateManagerUpdateLimboState();
            }
#endif
        }
        break;
#ifdef New_MMI
		case EventUnused1:
		{
			#ifdef NewChargeMMI
			if(PsuGetVregEn())
				PioSetPio(battery_low_io , PowerBlu_Disconnect);
				theHeadset.BHC612_BatteryLowState = PowerBlu_Disconnect;
			#else
				PioSetPio(battery_low_io , PowerBlu_Disconnect);
				theHeadset.BHC612_BatteryLowState = PowerBlu_Disconnect;
			#endif
			
			/*MAIN_DEBUG(("Chg disconnect/PS connect\n"));*/
			theHeadset.BHC612_DockMonitor = 1;

			#ifdef FullChargeLED
				theHeadset.BHC612_TEMP = 2;
			#endif
			
			if(theHeadset.batt_level == POWER_BATT_LEVEL3)
				theHeadset.BHC612_Chargefull = 1;
		}
		break;
		case EventUnused4:	/*EventChargerDisconnected*/
		{
		     /*if(ChargerIsChargerConnected() == FALSE)*/
			 if((ChargerIsChargerConnected() == FALSE) && (theHeadset.m_ChargerRealDisconnect == true))
		     {
		     	theHeadset.m_ChargerRealDisconnect = false;
				
		     	MAIN_DEBUG(("HS: Charger Disconnected,Batt level = %d\n",theHeadset.batt_level));
					 
			    powerManagerChargerDisconnected();
				 			    
			    theHeadset.BHC612_PSConnected = 0;

				#ifdef NewChargeMMI
				if(theHeadset.tts_language != 0)
					theHeadset.tts_language = 0;
				#endif

			    #if 0
				if(theHeadset.BHC612_PowerInitComplete == 1)
				{
	   				PowerGetVoltage(&BHC612_volt_level);
					theHeadset.batt_level = BHC612_volt_level.pLevel;

					if( theHeadset.batt_level == POWER_BATT_LEVEL3)
					{
						/*MAIN_DEBUG(("Chg disconnect : batt lev3\n"));*/

						checkPowerBluConn();

						if(theHeadset.BHC612_PowerBluConn == BlueLDE_Detect_Connect)
						{
							MessageSendLater(&theHeadset.task , EventUnused1 , 0 , 1000);
						}
						else
						{
							PioSetPio(battery_low_io , PowerBlu_Connect);
							/*MAIN_DEBUG(("Chg disconnect/PS disconnect\n"));*/
						}
					}
					else
					{
						PioSetPio(battery_low_io , PowerBlu_Connect);
					}				
				}
				#endif
				 
		    	if (lState == headsetLimbo )
		    	{
		        	stateManagerUpdateLimboState();
		    	}
					
				#ifdef New_MMI
					#ifdef PSBlueLED
					PioSetPio(PS_blue_led, PS_blue_led_off);
					#endif

					#ifdef NewPSBlueLEDx
						if( theHeadset.batt_level == POWER_BATT_LEVEL3)
						{
							if(theHeadset.BHC612_PowerBluConn == BlueLDE_Detect_Disconnect)
							{
								theHeadset.BHC612_UNDOCKMMI = 1;
								theHeadset.BHC612_DOCKMMI = 1;
								theHeadset.DockLED = 0;
								LEDManagerIndicateState(stateManagerGetState());
							}
							else
							{
								theHeadset.BHC612_UNDOCKMMI = 0;
								theHeadset.BHC612_DOCKMMI = 0;
								theHeadset.DockLED = 0;
							}
						}
						else
						{
							theHeadset.BHC612_UNDOCKMMI = 1;
							theHeadset.BHC612_DOCKMMI = 1;
							theHeadset.DockLED = 0;
							LEDManagerIndicateState(stateManagerGetState());
						}
					#endif
					#ifdef NewChargeMMI
						#if 1
						/*if((theHeadset.BHC612_Chargefull == 0))*/
						if(theHeadset.BHC612_BatteryLowState == PowerBlu_Connect)	
						{
						#endif
							/*Charger disconnnect, show battery level*/
							theHeadset.BHC612_UNDOCKMMI = 1;
							theHeadset.BHC612_DOCKMMI = 1;
							theHeadset.DockLED = 0;
							LEDManagerIndicateState(stateManagerGetState());
						#if 1
						}
						#endif
					#endif
				#endif
		     }
			else
			{
				theHeadset.m_ChargerRealDisconnect = false;
			}
			break;
		}
#endif
        case (EventChargerDisconnected):			
        {
#ifdef New_MMI
			if(lState == headsetLimbo)
			{
				powerManagerChargerDisconnected();
				stateManagerUpdateLimboState();
			}
			else
			{
	            /*MAIN_DEBUG(("HS: Charger Disconnected,check again!!\n"));*/

		     	MessageSendLater(&theHeadset.task , EventUnused4 , 0 , ChargerDisconnectDebounceTime);		

				theHeadset.m_ChargerRealDisconnect = true;

				if(theHeadset.m_ChargerRealConnect == true)
				{
					MessageCancelAll(task , EventUnused3 );
					theHeadset.m_ChargerRealConnect = false;
					/*MAIN_DEBUG(("Cancel EventChargerConnected!\n"));*/
				}
			}
#else
           	MAIN_DEBUG(("HS: Charger Disconnected\n"));
            powerManagerChargerDisconnected();
 
            if (lState == headsetLimbo )
            {
                stateManagerUpdateLimboState();
            }
#endif
        }
        break;
#ifdef BHC612
		case EventEnterBootMode2:
			#ifdef BlueCom
			MAIN_DEBUG(("Event : EnterBootMode2\n")) ;
			if(ChargerIsChargerConnected())
			{
				MAIN_DEBUG(("EnterBootMode2\n")) ;
			}
			else
			{

			}
	      		BootSetMode(2);
			#endif
		break;	
		case EventUnused5:
		{
			#ifdef MP_SwapHFPLinkPriority
				/*if(theHeadset.BHC612_MPReconnect == true)*/
				if(theHeadset.BHC612_MPReconnect == true && theHeadset.TTS_ASR_Playing == false)	
				{
					MessageCancelAll(task , EventUnused5);

					theHeadset.DockLED = 0;
					
					if(lState == headsetConnectable)
					{
						/*theHeadset.BHC612_MPReconnect = false;*/

						/*MessageSend ( &theHeadset.task , EventEstablishSLC , 0 ) ;*/
                
                		/* if the scroll PDL for preset number of connection attempts before giving is set
                   				retrieve the number of connection attempts to use */
                		theHeadset.conf->NoOfReconnectionAttempts = theHeadset.conf->timeouts.ReconnectionAttempts ;

                		slcEstablishSLCRequest() ;
			                
			           	/* don't indicate the event at first power up if the use different event at power on
			                   	feature bit is enabled, this enables the establish slc event to be used for the second manual
			                   	connection request */
			                   		
			          	if(stateManagerGetState() == headsetConnectable)
			      		{
			            	/* send message to do indicate a start of paging process when in connectable state */
			             	MessageSend(&theHeadset.task, EventStartPagingInConnState ,0);  
			        	}   
					}
				}
				else
				{
					MessageSendLater ( &theHeadset.task , EventUnused5 , 0 , 2000) ;
					MAIN_DEBUG(("w.\n")) ;
				}
			#endif
			break;
		}
		case EventUnused6:
		{
			#ifdef MP_SwapHFPLinkPriority
				/*if(theHeadset.MultipointEnable)*/
				/*if((theHeadset.MultipointEnable == 1) && (theHeadset.sco_sink == 0))
				if((theHeadset.MultipointEnable == 1) && (theHeadset.sco_sink == 0) && (theHeadset.conf->no_of_profiles_connected >= 2))
				*/
				if((theHeadset.MultipointEnable == 1) && (theHeadset.sco_sink == 0) && (theHeadset.conf->no_of_profiles_connected >= 2) && (theHeadset.BHC612_MPReconnect == false))
				{
					MAIN_DEBUG(("[### Multipoint  : SLC Reconnect...]\n"));

					theHeadset.BHC612_MPReconnect = true;
					
					/*Toggle presscallbutton flag*/
					theHeadset.PressCallButton = FALSE;

					MAIN_DEBUG(("PressCallButton : [F]\n"));

					theHeadset.DockLED = 1;/*Step 1*/
				
					headsetDisconnectAllSlc();

					if(theHeadset.a2dp_link_data->connected[a2dp_primary] == TRUE)
					{
						A2dpSignallingDisconnectRequest(theHeadset.a2dp_link_data->device_id[0]);
					}
					if(theHeadset.a2dp_link_data->connected[a2dp_secondary] == TRUE)
					{
						A2dpSignallingDisconnectRequest(theHeadset.a2dp_link_data->device_id[1]);
					}
					
					MAIN_DEBUG(("Primary BT Addr: nap %04x uap %02x lap %08lx\n",theHeadset.BHC612_PrimaryBTA.nap,theHeadset.BHC612_PrimaryBTA.uap,theHeadset.BHC612_PrimaryBTA.lap));
	        		ConnectionSmUpdateMruDevice( &theHeadset.BHC612_PrimaryBTA) ;
					MAIN_DEBUG(("Secondary BT Addr: nap %04x uap %02x lap %08lx\n",theHeadset.BHC612_SecondaryBTA.nap,theHeadset.BHC612_SecondaryBTA.uap,theHeadset.BHC612_SecondaryBTA.lap));	
					ConnectionSmUpdateMruDevice( &theHeadset.BHC612_SecondaryBTA) ;

					MAIN_DEBUG(("[###]\n"));

					#ifdef Rubi_VoicePrompt
					/*#if 1*/
					if(theHeadset.Rubi_enable == 0)
					{
						task = (TaskData *) &rubidium_tts_asr;	
						memset(TTS_text, 0, sizeof(TTS_text));
						strcpy(TTS_text, "\\p=041");/*Primary phone changed*/

						Kalimba_mode		= 1; /*TTS_ONLY_MODE*/
								
						ASR_active_model	= 0; /*RINGING_NODE*/

						theHeadset.TTS_ASR_Playing = true;

						RUBI_DEBUG(("Rubi voice prompt > \\p=041\n" ));
												
						AudioPlayTTS(task, 
									(uint16)0, 								/*TTS_CALLERID_NUMBER, */
									(uint8*)TTS_text,					 	/*num_string, */
									sizeof(TTS_text), 
									Language,	/*theHeadset.tts_language, */
									FALSE,									/*FALSE*/  
									theHeadset.codec_task,					/*theHeadset.codec_task, */
									0, 										/*TonesGetToneVolume(FALSE), */
									0										/*theHeadset.features.stereo	*/
									);
					}
					#endif
					
					#ifdef MP_Config4x
					MessageSendLater ( &theHeadset.task , EventUnused6 , 0 , 1000) ;/*Timer to check All SLC disconnect*/
					#endif
				}
				else
				{
					MAIN_DEBUG(("No_of_profile_connected = %d\n",theHeadset.conf->no_of_profiles_connected));  	

					if(theHeadset.MultipointEnable)
					{
						if((theHeadset.BHC612_MPReconnect == true) && (theHeadset.DockLED == 3))
						{
							MessageCancelAll ( &theHeadset.task , EventUnused6) ;
							#ifdef MP_Config4
							MessageSend ( &theHeadset.task , EventPowerOff , 0 ) ;
							#else
							MessageSendLater ( &theHeadset.task , EventUnused5 , 0 , 100) ;
							#endif
						}
						else
						{
							MessageSendLater ( &theHeadset.task , EventUnused6 , 0 , 1000) ;/*Timer to check All SLC disconnect*/	
						}
					}
				}
			#endif
		}
		break;	
#endif
        case EventSLCDisconnected: 
        	MAIN_DEBUG(("HS: EvSLCDisconnect,SCO = %d , Stae = %d\n",(uint16)theHeadset.sco_sink,lState)) ;
            {
				#ifdef New_MMI
				if((theHeadset.BHC612_LinkLoss == true))/*If link loss timeout!*/
				{
					theHeadset.BHC612_VPLinkLoss = 0;
					theHeadset.BHC612_LinkLoss = false;	
					theHeadset.BHC612_LinkLossReconnect = false;
				}
				#endif
				
				#ifdef Rubidium
					if((lState == headsetConnectable || lState == headsetConnected) && (theHeadset.BHC612_MPReconnect == false && theHeadset.DockLED == 0))
					{
							 /*if((theHeadset.MultipointEnable == 0) || (theHeadset.MultipointEnable == 1 && theHeadset.BHC612_PhoneChanged == 0 && theHeadset.BHC612_MPReconnect == false))*/		
							 /*if(lState == headsetConnectable)*/	
							 if((lState == headsetConnectable) || ((lState == headsetConnected) && (theHeadset.MultipointEnable == 0)))		
							 {
							 	if(theHeadset.BBHC612_PhoneDisconnect == 0)
							 	{
									theHeadset.BBHC612_PhoneDisconnect = 1;
									MessageCancelAll(&theHeadset.task , EventSLCDisconnected );
									MessageSendLater ( &theHeadset.task , EventSLCDisconnected , 0 , 2000) ;
									RUBI_DEBUG(("##aa\n"));
							 	}
								else
								{									
									theHeadset.BBHC612_PhoneDisconnect = 0;
									
									#ifdef Rubi_VoicePrompt
									if(theHeadset.Rubi_enable == 0)
									{
										task = (TaskData *) &rubidium_tts_asr;	
									 	memset(TTS_text, 0, sizeof(TTS_text));  
									 	strcpy(TTS_text, "\\p=011");	/*No phone detected*/			

									 	RUBI_DEBUG(("Rubi voice prompt. > \\p=011\n" ));
										Kalimba_mode 		= 1; /*TTS_ONLY_MODE*/
															 
										ASR_active_model	= 0; /*RINGING_NODE*/

										theHeadset.TTS_ASR_Playing = true;
																		 
										AudioPlayTTS(	task, 
												(uint16)0, 	/*TTS_CALLERID_NUMBER, */
												(uint8*)TTS_text, /*num_string, */
												sizeof(TTS_text), 
												Language,	/*theHeadset.tts_language, */
												FALSE,									/*FALSE*/  
												theHeadset.codec_task,					/*theHeadset.codec_task, */
												0, 										/*TonesGetToneVolume(FALSE), */
												0										/*theHeadset.features.stereo	*/
												);
									}
									#endif
								}
							 }
						 
						 /*else if(theHeadset.MultipointEnable == 1 && theHeadset.BHC612_PhoneChanged == 1 && theHeadset.BHC612_MPReconnect == false)*/
						 if((lState == headsetConnected) && (theHeadset.MultipointEnable == 1))
						 {						 
						 	if(theHeadset.BBHC612_PhoneDisconnect == 0)
						 	{
								theHeadset.BBHC612_PhoneDisconnect = 1;
								MessageCancelAll(&theHeadset.task , EventSLCDisconnected );
								MessageSendLater ( &theHeadset.task , EventSLCDisconnected , 0 , 2000) ;
						 	}
							else
							{
								if((theHeadset.conf->no_of_profiles_connected < 2) && (theHeadset.conf->no_of_profiles_connected > 0))
								{
									theHeadset.BBHC612_PhoneDisconnect = 0;

									#ifdef Rubi_VoicePrompt
									if(theHeadset.Rubi_enable == 0)
									{
										task = (TaskData *) &rubidium_tts_asr;	
									
									 	memset(TTS_text, 0, sizeof(TTS_text));

										MAIN_DEBUG(("no_of_profile_connected = %d,%d\n",theHeadset.conf->no_of_profiles_connected , GetNumberOfConnectedDevices())) ;

										strcpy(TTS_text, "\\p=029");/*Phone connected*/				
										RUBI_DEBUG(("Rubi voice prompt > \\p=029\n" ));

										/*theHeadset.BHC612_PhoneChanged = 0;*/

										Kalimba_mode 		= 1; /*TTS_ONLY_MODE*/
																 
										ASR_active_model	= 0; /*RINGING_NODE*/

										theHeadset.TTS_ASR_Playing = true;
																			 
										AudioPlayTTS(	task, 
														(uint16)0, 	/*TTS_CALLERID_NUMBER, */
														(uint8*)TTS_text, /*num_string, */
														sizeof(TTS_text), 
														Language,	/*theHeadset.tts_language, */
														FALSE,									/*FALSE*/  
														theHeadset.codec_task,					/*theHeadset.codec_task, */
														0, 										/*TonesGetToneVolume(FALSE), */
														0										/*theHeadset.features.stereo	*/
														);	
									}
									#endif
								}
							}
						 }
					}
				#endif

				#ifdef MP_SwapHFPLinkPriority
					if((theHeadset.BHC612_MPReconnect == true) && (theHeadset.DockLED >= 1))
					{
						theHeadset.DockLED++;
						MAIN_DEBUG(("++\n")) ;
					}
				#endif
				
                theHeadset.VoiceRecognitionIsActive = FALSE ;
				
                MessageCancelAll ( &theHeadset.task , EventNetworkOrServiceNotPresent ) ;
            }
        break ;
		case EventSLCConnected:
		case EventSLCConnectedAfterPowerOn:
				theHeadset.BHC612_MPReconnect = false;
				/*Toggle presscallbutton flag*/
				theHeadset.PressCallButton = TRUE;
				/*RUBI_DEBUG(("PressCallButton : [T]\n"));*/

				#if 0
				if(theHeadset.BHC612_no_of_profile_connected != theHeadset.conf->no_of_profiles_connected)
					theHeadset.BHC612_no_of_profile_connected = theHeadset.conf->no_of_profiles_connected;
				#endif
				
				#ifdef Rubi_VP_MinorChg
				theHeadset.DockLED = 0;
				#endif

				#ifdef CTIA_Test
				if(id == EventSLCConnectedAfterPowerOn)
				{
					HfpLinkGetCallState(hfp_primary_link, &stateAG1);
					MAIN_DEBUG(("#####SLCConnectedAfterPowerOn = %d\n",stateAG1));
				}
				#endif

				#ifdef Rubidium
					#ifdef Rubi_VoicePrompt					
					#ifdef CTIA_Test
					if((theHeadset.Rubi_enable == 0) && ((id == EventSLCConnected) || (id == EventSLCConnectedAfterPowerOn && stateAG1 == 0)))
					#else
					if(theHeadset.Rubi_enable == 0)
					#endif
					{
						task = (TaskData *) &rubidium_tts_asr;	
		
						if(!theHeadset.MultipointEnable)
						{
							/*Multipoint Disable*/
							memset(TTS_text, 0, sizeof(TTS_text));
							strcpy(TTS_text, "\\p=029");	
							RUBI_DEBUG(("Rubi voice prompt > \\p=029\n" ));
						}
						else
						{
							/*Multipoint Enable*/
							if(theHeadset.conf->no_of_profiles_connected < 2)
							{
								memset(TTS_text, 0, sizeof(TTS_text));
								strcpy(TTS_text, "\\p=029");/*Phone connected*/

								RUBI_DEBUG(("Rubi voice prompt > \\p=029\n" ));
							}
							else if(theHeadset.conf->no_of_profiles_connected >= 2)
							{	
								memset(TTS_text, 0, sizeof(TTS_text));
								strcpy(TTS_text, "\\p=003");/*Two device connected*/
								
								RUBI_DEBUG(("Rubi voice prompt > \\p=003\n" ));
							}
						}
						/*MAIN_DEBUG(("Rubi> Playing TTS:%s\n", TTS_text));*/

						Kalimba_mode		= 1; /*TTS_ONLY_MODE*/
				
						ASR_active_model	= 0; /*RINGING_NODE*/
		
						theHeadset.TTS_ASR_Playing = true;
						/*RUBI_DEBUG(("PressCallButton : [T]\n"));*/
		
						AudioPlayTTS(	task, 
										(uint16)0,								/*TTS_CALLERID_NUMBER, */
										(uint8*)TTS_text,						/*num_string, */
										sizeof(TTS_text), 
										Language,	/*theHeadset.tts_language, */
										FALSE,									/*FALSE*/  
										theHeadset.codec_task,					/*theHeadset.codec_task, */
										0,										/*TonesGetToneVolume(FALSE), */
										0										/*theHeadset.features.stereo	*/
									);
					}
					#endif
				#endif
				
				MAIN_DEBUG(("HS: EventSLCConnected\n")) ;
				DEBUG(("HS: EventSLCConnected\n")) ;
				
				/*if there is a queued event - we might want to know*/				  
				headsetRecallQueuedEvent();
		
				#ifdef iOSBatteryMeter
				/*#if 0*/
					if(lState == headsetConnected)
					{
						/*BatteryUserInitiatedRead();*/
						MessageSendLater(&theHeadset.task , EventBatteryLevelRequest , 0 ,1000) ;
					}
				#endif
		break;	
        case (EventResetPairedDeviceList):
            {
		MAIN_DEBUG(("HS: --Reset PDL--")) ;        
		

                if ( stateManagerIsConnected () )
                {
                        /*then we have an SLC active*/
                   headsetDisconnectAllSlc();
                }           
				
                configManagerReset();
				
				/*#ifdef Rubidium	###Reset TTS Language###*/
				#if 0
					#ifdef MANDARIN_SUPPORT
					if((Language != AUDIO_TTS_LANGUAGE_CHINESE_MANDARIN))
					#else
					if((Language != AUDIO_TTS_LANGUAGE_AMERICAN_ENGLISH))
					#endif
					{
						theHeadset.tts_language = Language; 
						#ifdef MANDARIN_SUPPORT
						Language = AUDIO_TTS_LANGUAGE_CHINESE_MANDARIN;
						#else
						Language = AUDIO_TTS_LANGUAGE_AMERICAN_ENGLISH;
						#endif
						configManagerWriteSessionData();
						Language = theHeadset.tts_language;
					}
				#endif

				#ifdef T3ProductionTest
					if(theHeadset.ProductionData != 0)
					{
						theHeadset.ProductionData = 0;
						configManagerWriteSessionData();
					}
				#endif

				#ifdef PSFWVer
					Write_SW_Version();
				#endif

				#ifdef Rubi_VoicePrompt
				if(theHeadset.Rubi_enable == 0)
				{
					task = (TaskData *) &rubidium_tts_asr;	
					memset(TTS_text, 0, sizeof(TTS_text));
					strcpy(TTS_text, "\\p=027");

					RUBI_DEBUG(("Rubi voice prompt > \\p=027\n" ));

					Kalimba_mode		= 1; /*TTS_ONLY_MODE*/
									
					ASR_active_model	= 0; /*RINGING_NODE*/

					theHeadset.TTS_ASR_Playing = true;
									
					AudioPlayTTS(	task, 
									(uint16)0, 								/*TTS_CALLERID_NUMBER, */
									(uint8*)TTS_text,					 	/*num_string, */
									sizeof(TTS_text), 
									Language,	/*theHeadset.tts_language, */
									FALSE,									/*FALSE*/  
									theHeadset.codec_task,					/*theHeadset.codec_task, */
									0, 										/*TonesGetToneVolume(FALSE), */
									0										/*theHeadset.features.stereo	*/
								);
				}
				#endif
            }
        break ;
        case ( EventLimboTimeout ):
            {
                /*we have received a power on timeout - shutdown*/
                MAIN_DEBUG(("HS: EvLimbo TIMEOUT\n")) ;
                if (lState != headsetTestMode)
                {
                    stateManagerUpdateLimboState();
                }
            }    
        break ;
        case (EventLinkLoss):
            MAIN_DEBUG(("HS: Link Loss,State = %d,SCO = %d\n",lState,(uint16)theHeadset.sco_sink)) ;
			/*MyDEBUG(("HS: Link Loss,State = %d,SCO = %d\n",lState,(uint16)theHeadset.sco_sink)) ;*/
            {
                /* should the headset have been powered off prior to a linkloss event being
                   		generated, this can happen if a link loss has occurred within 5 seconds
                   		of power off, ensure the headset does not attempt to reconnet from limbo mode */
                if(stateManagerGetState()== headsetLimbo)
                    lIndicateEvent = FALSE;

				/*DEBUG (("Link Loss [%s]\n",__TIME__));*/
				
				#ifdef BHC612
					if(theHeadset.BHC612_LinkLossReconnect == false)
					{
						theHeadset.BHC612_LinkLoss = true;
						theHeadset.BHC612_BattMeterOK = false;
					}
					else
					{
						theHeadset.BHC612_LinkLossReconnect = false;
						MessageCancelAll(task , EventLinkLoss);
					}

					if(theHeadset.sco_sink != 0)
					{
						AudioDisconnect();
            			theHeadset.sco_sink = 0;
						MAIN_DEBUG(("A2DP Link loss,Audio Disconnect!!\n"));

						if(stateManagerGetState()== headsetA2DPStreaming)
						{
							stateManagerEnterConnectedState();
						}
					}

					if(theHeadset.BHC612_VPLinkLoss == 0)
					{
						theHeadset.BHC612_VPLinkLoss = 1;
						
						#ifdef Rubi_VoicePrompt
						
						if(theHeadset.Rubi_enable == 0)
						{
							task = (TaskData *) &rubidium_tts_asr;	
						
							memset(TTS_text, 0, sizeof(TTS_text));
							strcpy(TTS_text, "\\p=011");			
						
							MAIN_DEBUG(("Rubi voice prompt > \\p=011\n" ));
								
							Kalimba_mode		= 1; /*TTS_ONLY_MODE*/
								
							ASR_active_model	= 0; /*RINGING_NODE*/
						
							theHeadset.TTS_ASR_Playing = true;
															
							AudioPlayTTS(	task, 
											(uint16)0,								/*TTS_CALLERID_NUMBER, */
											(uint8*)TTS_text,						/*num_string, */
											sizeof(TTS_text), 
											Language,	/*theHeadset.tts_language, */
											FALSE,									/*FALSE*/  
											theHeadset.codec_task,					/*theHeadset.codec_task, */
											0,										/*TonesGetToneVolume(FALSE), */
											0										/*theHeadset.features.stereo	*/
										);	
						}
						#endif
					}
				#endif				
            }
        break ;
        case (EventMuteReminder) :        
            MAIN_DEBUG(("HS: Mute Remind\n")) ;
            /*arrange the next mute reminder tone*/
            MessageSendLater( &theHeadset.task , EventMuteReminder , 0 ,D_SEC(theHeadset.conf->timeouts.MuteRemindTime_s ) )  ;            
        break;   
        
        case EventCheckForLowBatt:
           MAIN_DEBUG(("HS: Check for Low Batt\n")) ;
 
        break;       
        case EventTrickleCharge:  
            	MAIN_DEBUG(("HS: TrickleChg\n")) ;
        break;
        case EventFastCharge:
		MAIN_DEBUG(("HS: FastChg\n")) ;		
        break;
        case EventOkBattery:
        break;   
        case EventEnterDUTState :
        {
            MAIN_DEBUG(("EnterDUTState \n")) ;
            stateManagerEnterTestModeState();                
        }
        break;        
        case EventEnterDutMode :
        {
            MAIN_DEBUG(("Enter DUT Mode \n")) ;            
            if (lState !=headsetTestMode)
            {
                MessageSend( task , EventEnterDUTState, 0 ) ;
            }
            enterDutMode () ;               
        }
        break;        
        case EventEnterTXContTestMode :
        {
            MAIN_DEBUG(("Enter TX Cont \n")) ;        
            if (lState !=headsetTestMode)
            {
                MessageSend( task , EventEnterDUTState , 0 ) ;
            }            
            enterTxContinuousTestMode() ;
        }
        break ;		
        case EventVolumeOrientationNormal:
                theHeadset.gVolButtonsInverted = FALSE ;               
                MAIN_DEBUG(("HS: VOL ORIENT NORMAL [%d]\n", theHeadset.gVolButtonsInverted)) ;
                    /*write this to the PSKEY*/                
                /* also include the led disable state as well as orientation, write this to the PSKEY*/ 
				/* also include the selected tts language  */
                configManagerWriteSessionData () ;                          
        break;
        case EventVolumeOrientationInvert:       
               theHeadset.gVolButtonsInverted = TRUE ;
               MAIN_DEBUG(("HS: VOL ORIENT INVERT[%d]\n", theHeadset.gVolButtonsInverted)) ;               
               /* also include the led disable state as well as orientation, write this to the PSKEY*/                
               /* also include the selected tts language  */
			   configManagerWriteSessionData () ;           
        break;        
        case EventToggleVolume:     
                theHeadset.gVolButtonsInverted ^=1 ;    
                MAIN_DEBUG(("HS: Toggle Volume Orientation[%d]\n", theHeadset.gVolButtonsInverted)) ;      
        break ;        
        case EventNetworkOrServiceNotPresent:
            {       /*only bother to repeat this indication if it is not 0*/
                if ( theHeadset.conf->timeouts.NetworkServiceIndicatorRepeatTime_s )
                {       /*make sure only ever one in the system*/
                    MessageCancelAll( task , EventNetworkOrServiceNotPresent) ;
                    MessageSendLater  ( task , 
                                        EventNetworkOrServiceNotPresent ,
                                        0 , 
                                        D_SEC(theHeadset.conf->timeouts.NetworkServiceIndicatorRepeatTime_s) ) ;
                }                                    
                MAIN_DEBUG(("HS: NO NETWORK [%d]\n", theHeadset.conf->timeouts.NetworkServiceIndicatorRepeatTime_s )) ;
            }                                
        break ;
        case EventNetworkOrServicePresent:
            {
                MessageCancelAll ( task , EventNetworkOrServiceNotPresent ) ;                
                MAIN_DEBUG(("HS: YES NETWORK\n")) ;
            }   
        break ;
        case EventEnableDisableLeds  :   
#ifdef ROM_LEDS            
            MAIN_DEBUG(("HS: Toggle EN_DIS LEDS ")) ;
            MAIN_DEBUG(("HS: Tog Was[%c]\n" , theHeadset.theLEDTask->gLEDSEnabled ? 'T' : 'F')) ;
            
            LedManagerToggleLEDS();
            MAIN_DEBUG(("HS: Tog Now[%c]\n" , theHeadset.theLEDTask->gLEDSEnabled ? 'T' : 'F')) ;            
#endif			
			break ;        
        case EventEnableLEDS:
			MAIN_DEBUG(("EnableLEDS , TTS_ASR_Playing = %d\n",theHeadset.TTS_ASR_Playing));
#ifdef ROM_LEDS            		
	     if(theHeadset.theLEDTask->gLEDSEnabled == FALSE)
	     {
         	LedManagerEnableLEDS ( ) ;

		     #ifdef BHC612		
			     #ifndef StandbySavingPower
		            /* also include the led disable state as well as orientation, write this to the PSKEY*/                
		            configManagerWriteSessionData ( ) ;    
			     #endif	 
		     #endif
	     }

#endif
            break ;
        case EventDisableLEDS:
#ifdef ROM_LEDS            
            MAIN_DEBUG(("HS: Disable LEDS\n")) ;            
            LedManagerDisableLEDS ( ) ;

		#ifdef BHC612
			#ifndef StandbySavingPower
                /* also include the led disable state as well as orientation, write this to the PSKEY*/                
            configManagerWriteSessionData ( ) ;
			#endif
		#endif
#endif

            break ;
		case EventCancelLedIndication:
			MAIN_DEBUG(("HS: Disable LED indication\n")) ;        
#ifdef ROM_LEDS
            LedManagerResetLEDIndications ( ) ;            
#endif
		break ;
		case EventCallAnswered:
			MAIN_DEBUG(("HS: EventCallAnswered\n")) ;
						
			#ifdef Rubi_TTS
			MAIN_DEBUG(("Rubi_TTS_ASR = %d,PressCallButton : [F]\n",theHeadset.TTS_ASR_Playing)) ;
			theHeadset.PressCallButton = FALSE;
			#endif

			/*#ifdef ChineseTTS*/
			#if 0
				MessageSend ( task , EventRestoreDefaults , 0) ;
			#endif

			#if 1
			if(theHeadset.TTS_ASR_Playing)
			{
				TTSTerminate();
				theHeadset.TTS_ASR_Playing = false;
				MAIN_DEBUG(("********EventCallAnswered!!!\n")) ;
				UnloadRubidiumEngine();
			}
			#endif
		break;      
        case EventVLongTimer:
        case EventLongTimer:
           if (lState == headsetLimbo)
           {
               lIndicateEvent = FALSE ;
           }
        break ;
        case EventChargeError:
            MAIN_DEBUG(("HS: EventChargerError \n")) ;
            {       /*use the standard event if we are connected otherwise:*/
                if (( lState != headsetActiveCallSCO ) &&
                    ( lState != headsetActiveCallNoSCO ) );
                {
                    /*we are not connected so use the idle charger error event*/
                    lIndicateEvent = FALSE ;
                    MessageSend( task , EventChargeErrorInIdleState , 0 ) ;
                }                     
            }
        break ;  
        /* triggered when charging is ceased due to batt temp being out of range */
        case EventBattTempOutOfRange:
            MAIN_DEBUG(("HS: EventBattTempOutOfRange \n")) ;            
        break;
            /*these events have no required action directly associated with them  */
             /*they are received here so that LED patterns and Tones can be assigned*/
        case EventSCOLinkOpen :        
            MAIN_DEBUG(("EventScoLinkOpen\n")) ;
        break ;
        case EventSCOLinkClose:
            MAIN_DEBUG(("EventScoLinkClose\n")) ;
			
			/*#ifdef ChineseTTS*/
			#if 0
				MessageSend ( task , EventRestoreDefaults , 0) ;
			#endif
			
			#if 1
			if(theHeadset.TTS_ASR_Playing)
			{
				TTSTerminate();
				theHeadset.TTS_ASR_Playing = false;
				MAIN_DEBUG(("***TTS_ASR runing,Stop TTS\n")) ;
				UnloadRubidiumEngine();
			}
			#endif
        break ;
        case EventEndOfCall :        
            MAIN_DEBUG(("Rubi_TTS = %d,PressCallButton : [F]\n",theHeadset.TTS_ASR_Playing)) ;
			theHeadset.PressCallButton = FALSE;
			theHeadset.BHC612_TEMP = 1;

			#if 0
				BatteryCounter = 0;
				theHeadset.BHC612_BattMeterOK = false;
				MessageSendLater(&theHeadset.task, EventSpare2 , 0 , 500);/*Recover Battery icon*/
			#endif
			
#ifdef Rubi_TTS
	    /*KalimbaPowerOff();*/
	    theHeadset.RepeatCallerIDFlag = TRUE;        

		#if 0
		if(lState == headsetA2DPStreaming)
		{
			MAIN_DEBUG(("[A2DP]Cancel event : EventEndOfCall\n" ));
			MessageCancelAll(&theHeadset.task, EventEndOfCall);
			theHeadset.VoicePromptNotComplete = TRUE;
			break;
		}
		#endif
		
		#if 0
		if(theHeadset.sco_sink != 0 || theHeadset.TTS_ASR_Playing == true || (lState != headsetConnected && lState != headsetA2DPStreaming))	
		#else
		if(theHeadset.sco_sink != 0 || theHeadset.TTS_ASR_Playing == true || lState != headsetConnected)
		#endif
		{				
			if(theHeadset.BHC612_CallEnded)
			{
				MessageCancelAll(&theHeadset.task, EventEndOfCall);
				MessageSendLater(&theHeadset.task , EventEndOfCall , 0 , 1000);
				MAIN_DEBUG(("# EventEndOfCall not complete!\n" ));
			}
		}

		if((theHeadset.sco_sink == 0) && (theHeadset.TTS_ASR_Playing == true))/*Incoming call Timeout!*/
		{
			TTSTerminate();
			UnloadRubidiumEngine();
			theHeadset.TTS_ASR_Playing = false;
			/*BHC612_Init();*/
			theHeadset.BHC612_TEMP = 0;
			theHeadset.PressCallButton = TRUE;
			MAIN_DEBUG(("@@ Incoming call Timeout!\n" ));
		}

		/*if(theHeadset.sco_sink == 0 && theHeadset.TTS_ASR_Playing == false && lState == headsetConnected)*/
		if((theHeadset.sco_sink == 0) && (theHeadset.TTS_ASR_Playing == false) && (lState == headsetConnected) && (theHeadset.BHC612_CallEnded == 1))
		{
			MessageCancelAll(&theHeadset.task, EventEndOfCall);

			if(!theHeadset.PressCallButton)
				theHeadset.PressCallButton = TRUE;
			theHeadset.BHC612_CallEnded = 0;

			MAIN_DEBUG(("PressCallButton : [T]\n"));

			#ifdef Rubi_VoicePrompt
			/*#if 0*/
			if(theHeadset.Rubi_enable == 0)
			{
				task = (TaskData *) &rubidium_tts_asr;	

				memset(TTS_text, 0, sizeof(TTS_text));
				strcpy(TTS_text, "\\p=026");			

				MAIN_DEBUG(("Rubi voice prompt > \\p=026\n" ));
					
				Kalimba_mode 		= 1; /*TTS_ONLY_MODE*/
					
				ASR_active_model	= 0; /*RINGING_NODE*/

				theHeadset.TTS_ASR_Playing = true;
				
				theHeadset.VoicePromptNotComplete = TRUE;
					
				AudioPlayTTS(	task, 
								(uint16)0, 								/*TTS_CALLERID_NUMBER, */
								(uint8*)TTS_text,					 	/*num_string, */
								sizeof(TTS_text), 
								Language,	/*theHeadset.tts_language, */
								FALSE,									/*FALSE*/  
								theHeadset.codec_task,					/*theHeadset.codec_task, */
								0, 										/*TonesGetToneVolume(FALSE), */
								0										/*theHeadset.features.stereo	*/
							);	
			}

			#ifdef StandbySavingPower
				if(theHeadset.theLEDTask->gLEDSEnabled == FALSE)
				{
					/*theHeadset.theLEDTask->gLEDSEnabled = TRUE;*/
					LedManagerEnableLEDS();
				}
			#endif

			#ifdef iOSBatteryMeter
				BatteryCounter = 0;
				theHeadset.BHC612_BattMeterOK = false;
				MessageSendLater(&theHeadset.task, EventSpare2 , 0 , 500);/*Recover Battery icon*/
			#endif
			
			#endif			
		}
#endif
        break;    
        case EventResetComplete:        
            MAIN_DEBUG(("EventResetComplete\n")) ;
        break ;
        case EventError:        
            MAIN_DEBUG(("EventError\n")) ;
        break;
		case EventChargeErrorInIdleState:        
            MAIN_DEBUG(("EventChargeErrorInIdle\n")) ;
        break; 
	    case EventReconnectFailed:        
            MAIN_DEBUG(("EventReconnectFailed\n")) ;
			
			/*if(lState != headsetConnDiscoverable)*/
			/*if(lState != headsetConnDiscoverable && theHeadset.TTS_ASR_Playing == false)*/	
			if(lState == headsetConnectable && theHeadset.TTS_ASR_Playing == false)	
			{
				#ifdef Rubi_VoicePrompt
				if(theHeadset.Rubi_enable == 0)
				{
				 	 task = (TaskData *) &rubidium_tts_asr;	

					 memset(TTS_text, 0, sizeof(TTS_text));  
					 strcpy(TTS_text, "\\p=011");				

					 RUBI_DEBUG(("Rubi voice prompt... > \\p=011\n" ));
					   
					 Kalimba_mode 		= 1; /*TTS_ONLY_MODE*/
											 
					 ASR_active_model	= 0; /*RINGING_NODE*/

					 theHeadset.TTS_ASR_Playing = true;
													 
					 AudioPlayTTS(	task, 
								(uint16)0, 	/*TTS_CALLERID_NUMBER, */
								(uint8*)TTS_text, /*num_string, */
								sizeof(TTS_text), 
								Language,	/*theHeadset.tts_language, */
								FALSE,									/*FALSE*/  
								theHeadset.codec_task,					/*theHeadset.codec_task, */
								0, 										/*TonesGetToneVolume(FALSE), */
								0										/*theHeadset.features.stereo	*/
								);		
				 }
				 #endif
			}
        break;
		
#ifdef THREE_WAY_CALLING		
        case EventThreeWayReleaseAllHeld:       
            MAIN_DEBUG(("HS3 : RELEASE ALL [%c]\n", TRUE_OR_FALSE(theHeadset.conf->supp_features_local & HFP_THREE_WAY_CALLING))) ;          
            /* release the held call */
            MpReleaseAllHeld();
        break;
        case EventThreeWayAcceptWaitingReleaseActive:    
            MAIN_DEBUG(("HS3 : ACCEPT & RELEASE [%c]\n", TRUE_OR_FALSE(theHeadset.conf->supp_features_local & HFP_THREE_WAY_CALLING))) ;
            MpAcceptWaitingReleaseActive();
    		/* if headset has been muted on, mute off now */
    		if(theHeadset.gMuted)
				VolumeMuteOff();
        break ;
        case EventThreeWayAcceptWaitingHoldActive  :
            MAIN_DEBUG(("HS3 : ACCEPT & HOLD[%c]\n", TRUE_OR_FALSE(theHeadset.conf->supp_features_local & HFP_THREE_WAY_CALLING))) ;
            /* three way calling not available in multipoint usage */

            MpAcceptWaitingHoldActive();

			#ifdef ReleaseHeldCall
				if(theHeadset.BHC612_TEMP == 2)
				{
					theHeadset.PressCallButton = FALSE;/*First time press button*/
					MAIN_DEBUG(("@@@ PressCallButton : [F]\n"));
				}
			#endif
			
    		/* if headset has been muted on, mute off now */
    		if(theHeadset.gMuted)
				VolumeMuteOff();
        break ;
        case EventThreeWayAddHeldTo3Way  :
            MAIN_DEBUG(("HS3 : ADD HELD to 3WAY[%c]\n", TRUE_OR_FALSE(theHeadset.conf->supp_features_local & HFP_THREE_WAY_CALLING))) ;
            /* check to see if a conference call can be created, more than one call must be on the same AG */            
            MpHandleConferenceCall(TRUE);
        break ;
        case EventThreeWayConnect2Disconnect:  
            MAIN_DEBUG(("HS3 : EXPLICIT TRANSFER[%c]\n", TRUE_OR_FALSE(theHeadset.conf->supp_features_local & HFP_THREE_WAY_CALLING))) ;
            /* check to see if a conference call can be created, more than one call must be on the same AG */            
            MpHandleConferenceCall(FALSE);
        break ;
#endif		
        case (EventEnablePowerOff):
        {
            MAIN_DEBUG(("HS: EventEnablePowerOff \n")) ;
            theHeadset.PowerOffIsEnabled = TRUE ;
        }
        break;        
		
        case EventPlaceIncomingCallOnHold:
#ifdef BHC612			
			MP_MAIN_DEBUG(("EventPlaceIncomingCallOnHold\n"));
#endif
			headsetPlaceIncomingCallOnHold();
        break ;        
        case EventAcceptHeldIncomingCall:
#ifdef BHC612
			MP_MAIN_DEBUG(("EventAcceptHeldIncomingCall\n"));
#endif
			headsetAcceptHeldIncomingCall();
        break ;
        case EventRejectHeldIncomingCall:
#ifdef BHC612			
			MP_MAIN_DEBUG(("EventRejectHeldIncomingCall\n"));
#endif
			headsetRejectHeldIncomingCall();
        break;
		
		case EventLowBattery:
            DEBUG(("HS: EvLowBatt\n")) ;
			if (theHeadset.features.EnableCSR2CSRBattLevel && !(theHeadset.low_battery_flag_ag))
			{
				/*PowerBlueMMI_DEBUG(("HS: EvLowBatt, output to AG\n")) ;*/
                theHeadset.low_battery_flag_ag = TRUE;
			}
            /* If charging, no tone is play */
            if(ChargerIsChargerConnected())
            {
                lIndicateEvent = FALSE ;
            }
			
			#ifdef iOSBatteryMeter
			if(lState == headsetConnected)
			{
				csr2csrHandleAgBatteryRequestRes(EventGasGauge0 + 0xff);
				csr2csrHandleAgBatteryRequestRes(EventGasGauge0 + 0x1A);
			}
			#endif

			#ifdef ENABLE_AVRCP
			if((lState == headsetA2DPStreaming) && (theHeadset.battery_low_state == FALSE))
			{
				MessageSend( &theHeadset.task , EventAvrcpPlayPause , 0) ;
			}		

			#if 1
			if((ChargerIsChargerConnected()) == FALSE)
			{
				theHeadset.battery_low_state = TRUE;  
				
				/*if((lState == headsetConnectable) && (theHeadset.BHC612_DOCKMMI == 0))*/
				if(theHeadset.BHC612_DOCKMMI == 0)	
				{
					/*Enable LEDs*/
					if(theHeadset.theLEDTask->gLEDSEnabled == FALSE)
					{
					   theHeadset.theLEDTask->gLEDSEnabled = TRUE;
					}
				
					LEDManagerIndicateState( stateManagerGetState() );
				}
			}			
			#endif
			
			#endif
        break; 
        case EventGasGauge0 :
			#ifdef iOSBatteryMeter
			if(lState == headsetConnected)
			{
				csr2csrHandleAgBatteryRequestRes(EventGasGauge0 + 0xff);
				csr2csrHandleAgBatteryRequestRes(EventGasGauge0 + 0x41);
			}
			#endif
		break;
        case EventGasGauge1 :
			#ifdef iOSBatteryMeter
			if(lState == headsetConnected)
			{
				csr2csrHandleAgBatteryRequestRes(EventGasGauge0 + 0xff);
				csr2csrHandleAgBatteryRequestRes(EventGasGauge0 + 0x42);
			}
			#endif
		break;
        case EventGasGauge2 :
			#ifdef iOSBatteryMeter
			if(lState == headsetConnected)
			{
				csr2csrHandleAgBatteryRequestRes(EventGasGauge0 + 0xff);
				csr2csrHandleAgBatteryRequestRes(EventGasGauge0 + 0x43);
			}
			#endif
		break;
        case EventGasGauge3 :
			#ifdef iOSBatteryMeter
			if(lState == headsetConnected)
			{
				csr2csrHandleAgBatteryRequestRes(EventGasGauge0 + 0xff);
				csr2csrHandleAgBatteryRequestRes(EventGasGauge0 + 0x44);
			}
			#endif
        break ;
        
        case EventEnterDFUMode:       
        {
			MAIN_DEBUG(("###	EventEnterDFUMode	###\n")) ;
		
	      	BootSetMode(0);	      	
        }
        break;

		case EventEnterServiceMode:
        {
            MAIN_DEBUG(("Enter Service Mode \n")) ;            

            enterServiceMode();
        }
        break ;
        case EventServiceModeEntered:
        {
            MAIN_DEBUG(("Service Mode!!!\n")) ; 
        }
        break;

		case EventAudioMessage1:
		case EventAudioMessage2:
		case EventAudioMessage3:
		case EventAudioMessage4:
		{
			if (&theHeadset.sco_sink)
			{
				uint16 * lParam = PanicUnlessMalloc ( sizeof(uint16)) ;
				*lParam = (id -  EventAudioMessage1) ; /*0,1,2,3*/
				AudioSetMode ( AUDIO_MODE_CONNECTED , (void *) lParam) ;
			}
		}
		break ;
        case  EventCancelHSPIncomingCall:
            MAIN_DEBUG(("EventCancelHSPIncomingCall\n")) ;     
                /*clear the incoming call flag*/
			theHeadset.HSPIncomingCallInd = FALSE;
			/* if the ring indication has timed out and the state is still incoming 
				call establish, return to connected state else the headset will 
				remain in incoming call state if the call is answered or rejected
 				on the AG */
			if ( lState == headsetIncomingCallEstablish)
			{
				MAIN_DEBUG(("HSP ring with no connect, return to connected\n")) ; 
				stateManagerEnterConnectedState();    
			}
        break ;
		
        case EventUpdateStoredNumber:
			#ifdef ActiveRubiASR
					MAIN_DEBUG(("%d,%d,SCO = %d\n",lState,theHeadset.VoiceRecognitionIsActive,(uint16)theHeadset.sco_sink));

					#ifdef TTSLanguageflag
					if(theHeadset.BHC612_SelectLanguage == TRUE)
					{
						break;
					}
					#endif

					#if 1
					if(lState == headsetA2DPStreaming)
						break;
					
					if(theHeadset.VoicePromptNotComplete)					
						break;
					
					ToneTerminate();
					#endif

					if(theHeadset.BHC612_LinkLoss == true)
					{
						MessageCancelAll(&theHeadset.task, EventUpdateStoredNumber);
						break;
					}
				
					if(((lState == headsetConnected) || (lState == headsetConnectable)) && (theHeadset.sco_sink == 0) && (theHeadset.VoiceRecognitionIsActive == FALSE))
					{
						#ifdef T3ProductionTest
						if(theHeadset.ProductionData == 0){
						#endif
							/*if(theHeadset.TTS_ASR_Playing == false){*/
							if(theHeadset.TTS_ASR_Playing == false && theHeadset.Rubi_enable == 0)
							{								
								task = (TaskData *) &rubidium_tts_asr;	
								memset(TTS_text, 0, sizeof(TTS_text));

								if(lState == headsetConnected)
								{
									strcpy(TTS_text, "\\p=031\\p=034\\p=035\\p=036");
									RUBI_DEBUG(("Rubi voice prompt > \\p=031\\p=034\\p=035\\p=036\n" ));
								}
								else
								{
									strcpy(TTS_text, "\\p=031\\p=033\\p=036");
									/*strcpy(TTS_text, "\\p=033");Testing*/
									RUBI_DEBUG(("Rubi voice prompt > \\p=031\\p=033\\p=036\n" ));
								}
								
								#ifdef Barge_In_enable
    								Kalimba_mode 		= 4;  /*TTS_ASR_BARGE_IN_MODE*/
								#else
									Kalimba_mode 		= 3; /*TTS_ASR_MODE*/
								#endif
						        
						        ASR_active_model	= 1; /*IDLE_NODE*/

								theHeadset.TTS_ASR_Playing = true;

								AudioPlayTTS(	task, 
												(uint16)0, 								/*TTS_CALLERID_NUMBER, */
												(uint8*)TTS_text,					 	/*num_string, */
												sizeof(TTS_text), 
												Language,	/*theHeadset.tts_language, */
												FALSE,									/*FALSE*/  
												theHeadset.codec_task,					/*theHeadset.codec_task, */
												0, 										/*TonesGetToneVolume(FALSE), */
												0										/*theHeadset.features.stereo	*/
											);
							}
						#ifdef T3ProductionTest
						}
						#endif
					}
			#else
            			headsetUpdateStoredNumber();
			#endif
        break;
        
		case EventDialStoredNumber:
			MAIN_DEBUG(("EventDialStoredNumber\n"));
			headsetDialStoredNumber();
		
		break;
		case EventRestoreDefaults:
			#ifdef ChineseTTS
			MAIN_DEBUG(("###Restore TTS Language!!\n"));
			#else
			MAIN_DEBUG(("EventRestoreDefaults\n"));
			#endif
			configManagerRestoreDefaults();	
		break;
		
		case EventTone1:
		case EventTone2:
			MAIN_DEBUG(("HS: EventTone[%d]\n" , (id - EventTone1 + 1) )) ;
		break;
		
		case EventSelectTTSLanguageMode:
			#ifdef Rubidium
			if(theHeadset.sco_sink == 0 && theHeadset.VoiceRecognitionIsActive == FALSE)
			{
				if(theHeadset.tts_language == 0)
					theHeadset.DockLED = Language;

				#ifdef TTSLanguageflag
				theHeadset.BHC612_SelectLanguage = TRUE;
				#endif
			
				theHeadset.tts_language++;

				if(theHeadset.tts_language < (Rubi_Language+1))
				{
					#ifdef Rubi_VoicePrompt
					if(theHeadset.Rubi_enable == 0)
					{
						if(theHeadset.tts_language == 1)
						{
							#ifdef MANDARIN_SUPPORT
							Language = AUDIO_TTS_LANGUAGE_CHINESE_MANDARIN;
							#else
							Language = AUDIO_TTS_LANGUAGE_AMERICAN_ENGLISH;
							#endif
							
						}
						else if(theHeadset.tts_language == 2)
						{
							#ifdef MANDARIN_SUPPORT
							Language = AUDIO_TTS_LANGUAGE_AMERICAN_ENGLISH;
							#else
							Language = AUDIO_TTS_LANGUAGE_SPANISH_EUROPEAN;
							#endif
						}
						else
						{
							Language = AUDIO_TTS_LANGUAGE_FRENCH;				

							
						}

						task = (TaskData *) &rubidium_tts_asr;	

						memset(TTS_text, 0, sizeof(TTS_text));  
						strcpy(TTS_text, "\\p=038\\p=047\\p=047\\p=039");				

						/*To select English,press the call button now*/
						RUBI_DEBUG(("Rubi voice prompt > \\p=038\\p=047\\p=047\\p=039\n" ));
							   
						Kalimba_mode		= 1; /*TTS_ONLY_MODE*/
						ASR_active_model	= 0; /*RINGING_NODE*/
						theHeadset.TTS_ASR_Playing = true;

						AudioPlayTTS(task, 
									(uint16)0, 								/*TTS_CALLERID_NUMBER, */
									(uint8*)TTS_text, 						/*num_string, */
									sizeof(TTS_text), 
									Language,								/*theHeadset.tts_language, */
									FALSE,									/*FALSE*/  
									theHeadset.codec_task,					/*theHeadset.codec_task, */
									0, 										/*TonesGetToneVolume(FALSE), */
									0										/*theHeadset.features.stereo	*/
									);	

						if((theHeadset.tts_language == 1) || (theHeadset.tts_language == 2) || (theHeadset.tts_language == 3))
						{
							MessageSendLater(&theHeadset.task, EventSelectTTSLanguageMode, 0, D_SEC(7));
							RUBI_DEBUG(("tts_language = %d\n",theHeadset.tts_language));
						}
					}
					#endif
				}
				else
				{
					theHeadset.tts_language = 0;
					
					RUBI_DEBUG(("Cancel language selection menu.\n"));

					if(theHeadset.DockLED != 0x0f)
					{
						Language = theHeadset.DockLED;
						RUBI_DEBUG(("Restore to original language\n"));
						theHeadset.DockLED = 0;
					}

					/*theHeadset.BHC612_TEMP++;*/

					if(theHeadset.BHC612_TEMP < Rubi_loop)
					{
						theHeadset.BHC612_TEMP++;
						RUBI_DEBUG(("No button pressed!,Loop = %d\n",theHeadset.BHC612_TEMP));
						MessageSend ( task , EventSelectTTSLanguageMode , 0) ;						
					}
					else
					{
						/*MAIN_DEBUG(("## Complete !! ## , %d\n",theHeadset.BHC612_TEMP));*/
						/*theHeadset.BHC612_TEMP = 0x00;*/
					}
					
					#ifdef Rubi_VoicePrompt
						if(theHeadset.BHC612_TEMP == Rubi_loop)/*Press talk button*/
						{
							if(theHeadset.Rubi_enable == 0)
							{
								task = (TaskData *) &rubidium_tts_asr;	

								memset(TTS_text, 0, sizeof(TTS_text));  
								strcpy(TTS_text, "\\p=000");				

								RUBI_DEBUG(("VP > \\p=000(Welcome!)\n" ));
									   
								Kalimba_mode		= 1; /*TTS_ONLY_MODE*/
																 
								ASR_active_model	= 0; /*RINGING_NODE*/

								theHeadset.TTS_ASR_Playing = true;
																	 
								AudioPlayTTS(	task, 
											(uint16)0, 	/*TTS_CALLERID_NUMBER, */
											(uint8*)TTS_text, /*num_string, */
											sizeof(TTS_text), 
											Language,	/*theHeadset.tts_language, */
											FALSE,									/*FALSE*/  
											theHeadset.codec_task,					/*theHeadset.codec_task, */
											0, 										/*TonesGetToneVolume(FALSE), */
											0										/*theHeadset.features.stereo	*/
											);	
							}
						}
					#endif

					if(theHeadset.BHC612_TEMP >= Rubi_loop)
					{						
						theHeadset.BHC612_TEMP = 0x00;
						MessageCancelAll(&theHeadset.task, EventConfirmTTSLanguage);
						MessageCancelAll(&theHeadset.task, EventSelectTTSLanguageMode);
						RUBI_DEBUG(("## Complete !! ## , %d\n",theHeadset.BHC612_TEMP));

						#ifdef TTSLanguageflag
							theHeadset.BHC612_SelectLanguage = FALSE;
						#endif

						BHC612_Init();
						
						LEDManagerIndicateState (stateManagerGetState());
					}
				}
			}
			#else
			     MAIN_DEBUG(("EventSelectTTSLanguageModes\n"));

		           	TTSSelectTTSLanguageMode();
		            MessageCancelAll(&theHeadset.task, EventConfirmTTSLanguage);
		            MessageSendLater(&theHeadset.task, EventConfirmTTSLanguage, 0, D_SEC(3));
			#endif
		break;
		
        case EventConfirmTTSLanguage:
            /* Store TTS language in PS */
		#ifdef Rubidium
			/*if(theHeadset.sco_sink == 0 && theHeadset.VoiceRecognitionIsActive == FALSE)*/
			if((theHeadset.sco_sink == 0) && (theHeadset.VoiceRecognitionIsActive == FALSE) && (theHeadset.TTS_ASR_Playing == FALSE))	
			{
				if(theHeadset.tts_language == 1)
				{
					TTSTerminate();
					theHeadset.tts_language = Rubi_Language+1;
					#ifdef MANDARIN_SUPPORT
					Language = AUDIO_TTS_LANGUAGE_CHINESE_MANDARIN;
					RUBI_DEBUG(("Select TTSLanguage : Chinese\n"));
					#else
					Language = AUDIO_TTS_LANGUAGE_AMERICAN_ENGLISH;
					RUBI_DEBUG(("Select TTSLanguage : US\n"));
					#endif
					configManagerWriteSessionData();
					theHeadset.DockLED = 0x0f;
					theHeadset.BHC612_TEMP = Rubi_loop;
					break;
				}
				else if(theHeadset.tts_language == 2)
				{
					TTSTerminate();
					theHeadset.tts_language = Rubi_Language+1;		
					#ifdef MANDARIN_SUPPORT
					Language = AUDIO_TTS_LANGUAGE_AMERICAN_ENGLISH;
					RUBI_DEBUG(("Select TTSLanguage : US\n"));
					#else
					Language = AUDIO_TTS_LANGUAGE_SPANISH_EUROPEAN;
					RUBI_DEBUG(("Select TTSLanguage : SP\n"));
					#endif
					configManagerWriteSessionData();
					theHeadset.DockLED = 0x0f;
					theHeadset.BHC612_TEMP = Rubi_loop;
					break;
				}
				else if(theHeadset.tts_language == 3)
				{
					#ifdef MANDARIN_SUPPORTx
					TTSTerminate();
					theHeadset.tts_language = Rubi_Language+1;
					Language = AUDIO_TTS_LANGUAGE_FRENCH;
					configManagerWriteSessionData();
					theHeadset.DockLED = 0x0f;
					RUBI_DEBUG(("Select TTSLanguage : FR\n"));
					theHeadset.BHC612_TEMP = Rubi_loop;
					#endif
					break;					
				}

				#ifdef Rubi_VP_MinorChg
				if(theHeadset.TTS_ASR_Playing == false)
				{
					RUBI_DEBUG(("ConfirmTTSLanguage,Check Connection!\n"));

					task = (TaskData *) &rubidium_tts_asr;							
				
					Kalimba_mode 		= 1; /*TTS_ONLY_MODE*/
				
					ASR_active_model	= 0; /*RINGING_NODE*/

					theHeadset.TTS_ASR_Playing = true;

					memset(TTS_text, 0, sizeof(TTS_text));

					strcpy(TTS_text, "\\p=011");	
					RUBI_DEBUG(("Rubi voice prompt > \\p=011\n" ));

					AudioPlayTTS(	task, 
								(uint16)0, 								/*TTS_CALLERID_NUMBER, */
								(uint8*)TTS_text,					 	/*num_string, */
								sizeof(TTS_text), 
								Language,								/*theHeadset.tts_language, */
								FALSE,									/*FALSE*/  
								theHeadset.codec_task,					/*theHeadset.codec_task, */
								0, 										/*TonesGetToneVolume(FALSE), */
								0										/*theHeadset.features.stereo	*/
							);	
				}
				#endif
			}
		#else
            configManagerWriteSessionData () ;
		#endif
        break;
        		
        /* enable multipoint functionality */
        case EventEnableMultipoint:
			#ifdef BHC612
			if(theHeadset.MultipointEnable == 0)
			{
				MAIN_DEBUG(("EventEnableMultipoint\n"));
				 /* enable multipoint operation */
				 /* deny enable if disabled because of OVAL */
				 theHeadset.MultipointEnable = 1;
				 /* and store in PS for reading at next power up */
				 configManagerWriteSessionData () ;
			}
			else
			{
				MAIN_DEBUG(("EventDisableMultipoint\n"));
				/* disable multipoint operation */
				theHeadset.MultipointEnable = 0;
				/* and store in PS for reading at next power up */
				configManagerWriteSessionData () ;	  
			}
			#else
            MAIN_DEBUG(("EventEnableMultipoint\n"));
            /* enable multipoint operation */
            /* deny enable if disabled because of OVAL */
            theHeadset.MultipointEnable = 1;
            /* and store in PS for reading at next power up */
			configManagerWriteSessionData () ;
			#endif
		break;
		
        /* disable multipoint functionality */
        case EventDisableMultipoint:
            MAIN_DEBUG(("EventDisableMultipoint\n"));
            /* disable multipoint operation */
            theHeadset.MultipointEnable = 0;
		#if 0	
            /* and store in PS for reading at next power up */
			configManagerWriteSessionData () ;           
		#endif
        break;
        
        /* disabled leds have been re-enabled by means of a button press or a specific event */
        case EventResetLEDTimeout:
#ifdef ROM_LEDS               
            MAIN_DEBUG(("EventResetLEDTimeout\n"));
#if 0
	    if(theHeadset.PowerStationOnOff != ChargerVOn)
	    {
           		LEDManagerIndicateState ( lState ) ;     
			theHeadset.theLEDTask->gLEDSStateTimeout = FALSE ;
	    }
#else
            LEDManagerIndicateState ( lState ) ;     
			theHeadset.theLEDTask->gLEDSStateTimeout = FALSE ;
#endif		
#endif            
        break;
 
        /* starting paging whilst in connectable state */
        case EventStartPagingInConnState:
            MAIN_DEBUG(("EventStartPagingInConnState\n"));
            /* set bit to indicate paging status */
            theHeadset.paging_in_progress = TRUE;
        break;
        
        /* paging stopped whilst in connectable state */
        case EventStopPagingInConnState:
            MAIN_DEBUG(("EventStartPagingInConnState\n"));
            /* set bit to indicate paging status */
            theHeadset.paging_in_progress = FALSE;
        break;
        
        /* continue the slc connection procedure, will attempt connection
           to next available device */
        case EventContinueSlcConnectRequest:
            MAIN_DEBUG(("EventContinueSlcConnectRequest\n"));
		#ifdef BHC612
			#ifdef BHC612_MultipointEnable
				if (stateManagerGetPdlSize()  > 1)
				{
					MAIN_DEBUG(("PDL size > 1 : Multipoint[Enable]\n"));
					theHeadset.MultipointEnable = 1;
				}
			#endif

			#ifdef MP_Config3
				theHeadset.MultipointEnable = 0;
			#endif
		#endif
            /* attempt next connection */
   	        slcContinueEstablishSLCRequest();
        break;
        
        /* indication of call waiting when using two AG's in multipoint mode */
        case EventMultipointCallWaiting:
            MAIN_DEBUG(("EventMultipointCallWaiting\n"));
        break;
                   
        /* kick off a check the role of the headset and make changes if appropriate by requesting a role indication */
        case EventCheckRole:
        {
            uint8 hfp_idx;
            Sink * sink_passed = (Sink*)message ;
				/*no specific sink to check, check all available - happens on the back of each hfp connect cfm */
			if (!sink_passed)
            {
	            for(hfp_idx = 0; hfp_idx < theHeadset.conf->no_of_profiles_connected; hfp_idx++)
	            {
	                Sink sink; 
                    HfpLinkGetSlcSink((hfp_idx + 1), &sink);
	                if(SinkIsValid(sink))
	        	    {
					    ConnectionGetRole(&theHeadset.task, sink);
                        MAIN_DEBUG(("GET n role[%x]\n", (int)sink));
                    }
	            }
	        }
            else /*a specific sink has been passed in - happens on a failed attempt to role switch - after 1 second*/
            {
            	if (SinkIsValid(*sink_passed) )
                {
                		/*only attempt to switch the sink that has failed to switch*/
	                ConnectionGetRole(&theHeadset.task , *sink_passed) ;
					MAIN_DEBUG(("GET 1 role[%x]\n", (int)*sink_passed));
                }
            }
		}	
        break;

        case EventMissedCall:
        {
			if(theHeadset.conf->timeouts.MissedCallIndicateTime_s != 0)
			{ 
                MessageCancelAll(task , EventMissedCall ) ;
                     
                theHeadset.MissedCallIndicated -= 1;               
                if(theHeadset.MissedCallIndicated != 0)
 			 	{
                    MessageSendLater( &theHeadset.task , EventMissedCall , 0 , D_SEC(theHeadset.conf->timeouts.MissedCallIndicateTime_s) ) ;
                }
            }	
        }
        break;
      
#ifdef ENABLE_PBAP                  
        case EventPbapDialMch:
        {         
            /* pbap dial from missed call history */
            MAIN_DEBUG(("EventPbapDialMch\n"));  
                        
            if ( theHeadset.PowerOffIsEnabled )
            {
                /* If voice dial is active, cancel the voice dial if the feature bit is set */
                if (theHeadset.features.LNRCancelsVoiceDialIfActive   && 
                    theHeadset.VoiceRecognitionIsActive)
                {
                    MessageSend(&theHeadset.task , EventInitateVoiceDial , 0 ) ;
                    lIndicateEvent = FALSE ;
                }
                else
                {					
                    pbapDialPhoneBook(pbap_mch);
                }
			}
            else
            {
                lIndicateEvent = FALSE ;
            }
        }
        break;      
        
        case EventPbapDialIch:
        {            
            /* pbap dial from incoming call history */
            MAIN_DEBUG(("EventPbapDialIch\n"));
           
            if ( theHeadset.PowerOffIsEnabled )
            {
                /* If voice dial is active, cancel the voice dial if the feature bit is set */
                if (theHeadset.features.LNRCancelsVoiceDialIfActive   && 
                    theHeadset.VoiceRecognitionIsActive)
                {
                    MessageSend(&theHeadset.task , EventInitateVoiceDial , 0 ) ;
                    lIndicateEvent = FALSE ;
                }
                else
                {					
                    pbapDialPhoneBook(pbap_ich);
                }
			}
            else
            {
                lIndicateEvent = FALSE ;
            }
        }
        break;        
#endif        
        
#ifdef WBS_TEST
        /* TEST EVENTS for WBS testing */
        case EventSetWbsCodecs:
            if(theHeadset.RenegotiateSco)
            {
                MAIN_DEBUG(("HS : AT+BAC = cvsd wbs\n")) ;
                theHeadset.RenegotiateSco = 0;
                HfpWbsSetSupportedCodecs((hfp_wbs_codec_mask_cvsd | hfp_wbs_codec_mask_msbc), FALSE);
            }
            else
            {
                MAIN_DEBUG(("HS : AT+BAC = cvsd only\n")) ;
                theHeadset.RenegotiateSco = 1;
                HfpWbsSetSupportedCodecs(hfp_wbs_codec_mask_cvsd , FALSE);           
            }
            
        break;
    
        case EventSetWbsCodecsSendBAC:
            if(theHeadset.RenegotiateSco)
            {
                MAIN_DEBUG(("HS : AT+BAC = cvsd wbs\n")) ;
                theHeadset.RenegotiateSco = 0;
                HfpWbsSetSupportedCodecs((hfp_wbs_codec_mask_cvsd | hfp_wbs_codec_mask_msbc), TRUE);
            }
           else
           {
               MAIN_DEBUG(("HS : AT+BAC = cvsd only\n")) ;
               theHeadset.RenegotiateSco = 1;
               HfpWbsSetSupportedCodecs(hfp_wbs_codec_mask_cvsd , TRUE);           
           }
           break;
 
         case EventOverrideResponse:
                   
           if(theHeadset.FailAudioNegotiation)
           {
               MAIN_DEBUG(("HS : Fail Neg = off\n")) ;
               theHeadset.FailAudioNegotiation = 0;
           }
           else
           {
               MAIN_DEBUG(("HS : Fail Neg = on\n")) ;
               theHeadset.FailAudioNegotiation = 1;
           }
       break; 
    
       case EventCreateAudioConnection:
           MAIN_DEBUG(("HS : Create Audio Connection\n")) ;
           
           CreateAudioConnection();
       break;

#endif

#ifndef T3ProductionTest
       case EventEnableIntelligentPowerManagement:
           MAIN_DEBUG(("HS : Enable LBIPM\n")) ;           
            /* enable LBIPM operation */
           /* only enable if not disabled for Oval */
           theHeadset.lbipmEnable = 1;          
           /* send plugin current power level */           
           AudioSetPower(LBIPMPowerLevel());
            /* and store in PS for reading at next power up */
		   configManagerWriteSessionData () ;     
       break;
       
       case EventDisableIntelligentPowerManagement:
           MAIN_DEBUG(("HS : Disable LBIPM\n")) ;           
            /* disable LBIPM operation */
           theHeadset.lbipmEnable = 0;
           /* notify the plugin Low power mode is no longer required */           
           AudioSetPower(LBIPMPowerLevel());
            /* and store in PS for reading at next power up */
		   configManagerWriteSessionData () ; 
       break;
       
       case EventToggleIntelligentPowerManagement:
           MAIN_DEBUG(("HS : Toggle LBIPM\n")) ;
           if(theHeadset.lbipmEnable)
           {
               MessageSend( &theHeadset.task , EventDisableIntelligentPowerManagement , 0 ) ;
           }
           else
           {
               MessageSend( &theHeadset.task , EventEnableIntelligentPowerManagement , 0 ) ;
           }
       break;  
#endif	   
#ifdef ENABLE_AVRCP       
       case EventAvrcpPlayPause:
           /*MAIN_DEBUG(("HS : EventAvrcpPlayPause\n")) ;*/

		   if(theHeadset.PressCallButton)
		   {
				MAIN_DEBUG(("PressCallButton :[T]\n"));
		   }
		   else
		   {
		   		if(lState != headsetA2DPStreaming)
		   		{
					MAIN_DEBUG(("PressCallButton :[F]\n"));
					MAIN_DEBUG(("Cancel EventAvrcpPlayPause\n"));
					MessageCancelAll(task , EventAvrcpPlayPause ) ;
					break;
		   		}
				else
				{
					if(theHeadset.PressCallButton == FALSE)
					{
						theHeadset.PressCallButton = TRUE;
						MessageSendLater( &theHeadset.task , EventEnableLEDS , 0 , 100) ;
					}	
				}
		   }
		   
		   if(lState == headsetConnected)
		   {
		   		if(theHeadset.theLEDTask->gLEDSEnabled == FALSE)
					MessageSendLater( &theHeadset.task , EventEnableLEDS , 0 , 200) ;

				if(theHeadset.VoiceRecognitionIsActive == 0)
				{
		   			#if 1
					if(GetA2DPState())
					{
						ToneTerminate();
		           		headsetAvrcpPlayPause();
						MAIN_DEBUG(("HS : #EventAvrcpPlayPause\n")) ;  
					}
					else
					{
						#if 0
						task = (TaskData *) &rubidium_tts_asr;							
								
						Kalimba_mode		= 1; /*TTS_ONLY_MODE*/
								
						ASR_active_model	= 0; /*RINGING_NODE*/
								
						theHeadset.TTS_ASR_Playing = true;
								
						memset(TTS_text, 0, sizeof(TTS_text));
								
						strcpy(TTS_text, "\\p=011");	
						RUBI_DEBUG(("Rubi voice prompt > \\p=011\n" ));
								
						AudioPlayTTS(	task, 
									(uint16)0,								/*TTS_CALLERID_NUMBER, */
									(uint8*)TTS_text,						/*num_string, */
									sizeof(TTS_text), 
									Language,								/*theHeadset.tts_language, */
									FALSE,									/*FALSE*/  
									theHeadset.codec_task,					/*theHeadset.codec_task, */
									0,										/*TonesGetToneVolume(FALSE), */
									0										/*theHeadset.features.stereo	*/
									);	
						#endif
					}
					#else
			   			ToneTerminate();
	           			headsetAvrcpPlayPause();
						MAIN_DEBUG(("HS : #EventAvrcpPlayPause\n")) ;  
					#endif
				}

		   		#if 0
		   		if(theHeadset.theLEDTask->gLEDSEnabled == FALSE)
		   		{
					MessageSendLater( &theHeadset.task , EventEnableLEDS , 0 , 200) ;

					if(theHeadset.VoiceRecognitionIsActive == 0)
			   		{
						#if 1
						if(GetA2DPState())
						{
							ToneTerminate();
	           				headsetAvrcpPlayPause();
							MAIN_DEBUG(("HS : #EventAvrcpPlayPause\n")) ;  
						}
						else
						{
							task = (TaskData *) &rubidium_tts_asr;							
							
							Kalimba_mode		= 1; /*TTS_ONLY_MODE*/
							
							ASR_active_model	= 0; /*RINGING_NODE*/
							
							theHeadset.TTS_ASR_Playing = true;
							
							memset(TTS_text, 0, sizeof(TTS_text));
							
							strcpy(TTS_text, "\\p=011");	
							RUBI_DEBUG(("Rubi voice prompt > \\p=011\n" ));
							
							AudioPlayTTS(	task, 
										(uint16)0,								/*TTS_CALLERID_NUMBER, */
										(uint8*)TTS_text,						/*num_string, */
										sizeof(TTS_text), 
										Language,								/*theHeadset.tts_language, */
										FALSE,									/*FALSE*/  
										theHeadset.codec_task,					/*theHeadset.codec_task, */
										0,										/*TonesGetToneVolume(FALSE), */
										0										/*theHeadset.features.stereo	*/
									);	

						}
						#else
			   			ToneTerminate();
	           			headsetAvrcpPlayPause();
						MAIN_DEBUG(("HS : #EventAvrcpPlayPause\n")) ;  
						#endif
			   		}					
				}
				else
				{
			   		if(theHeadset.VoiceRecognitionIsActive == 0)
			   		{
			   			ToneTerminate();
	           			headsetAvrcpPlayPause();
						MAIN_DEBUG(("HS : !EventAvrcpPlayPause\n")) ;  
			   		}
				}
				#endif
		   }
		   else	/*A2DP streaming*/
		   {
			   	if(theHeadset.theLEDTask->gLEDSEnabled == FALSE)
			   	{
					MessageSendLater( &theHeadset.task , EventEnableLEDS , 0 ,200) ;
					headsetAvrcpPlayPause();
					MAIN_DEBUG(("HS : &EventAvrcpPlayPause\n")) ;  
			   	}
				else
				{
				  	/*A2DP streaming*/
				  	headsetAvrcpPlayPause();
					MAIN_DEBUG(("HS : *EventAvrcpPlayPause\n")) ;  
				}
			}
       break;
            
       case EventAvrcpStop:
           MAIN_DEBUG(("HS : EventAvrcpStop\n")) ; 
           headsetAvrcpStop();
       break;
            
       case EventAvrcpSkipForward:
           MAIN_DEBUG(("HS : EventAvrcpSkipForward\n")) ;  
           headsetAvrcpSkipForward();
       break;
      
       case EventAvrcpSkipBackward:
           MAIN_DEBUG(("HS : EventAvrcpSkipBackward\n")) ; 
           headsetAvrcpSkipBackward();
       break;
            
       case EventAvrcpFastForwardPress:
           MAIN_DEBUG(("HS : EventAvrcpFastForwardPress\n")) ;  
           headsetAvrcpFastForwardPress();
       break;
            
       case EventAvrcpFastForwardRelease:
           MAIN_DEBUG(("HS : EventAvrcpFastForwardRelease\n")) ;
           headsetAvrcpFastForwardRelease();
       break;
            
       case EventAvrcpRewindPress:
           MAIN_DEBUG(("HS : EventAvrcpRewindPress\n")) ; 
           headsetAvrcpRewindPress();
       break;
            
       case EventAvrcpRewindRelease:
           MAIN_DEBUG(("HS : EventAvrcpRewindRelease\n")) ; 
           headsetAvrcpRewindRelease();
       break;
#endif       
       default :
           MAIN_DEBUG (( "HS : UE unhandled!! [%x]\n", id ));     
       break ;           

        
    }   
    

        /* Inform theevent indications that we have received a user event*/
        /* For all events except the end event notification as this will just end up here again calling itself...*/
    if ( lIndicateEvent )
    {
        if ( id != EventLEDEventComplete )
        {
#ifdef BHC612
			if(id == EventPairingSuccessful)
			{
				MAIN_DEBUG(("PairingSuccessful LED!\n"));
			}
#endif
            LEDManagerIndicateEvent ( id ) ;
        }
		
#ifdef New_MMI
		if(!(ChargerIsChargerConnected()))
		{
			if(id == EventPowerOn)
			{
				#ifdef BHC612
					#ifdef Rubidium
					if(stateManagerGetState() == headsetConnDiscoverable)
					{
						/*ToneTerminate();*/
						
						#ifdef Rubi_VoicePrompt
						#ifdef Rubi_VP_MinorChgx
						if(theHeadset.Rubi_enable == 0)
						{
							task = (TaskData *) &rubidium_tts_asr;	
							memset(TTS_text, 0, sizeof(TTS_text));

							strcpy(TTS_text, "\\p=000\\p=047\\p=001\\p=002\\p=005\\p=006\\p=007\\p=008");

							RUBI_DEBUG(("Rubi voice prompt > \\p=000\\p=047\\p=001\\p=002\\p=005\\p=006\\p=007\\p=008\n" ));
							
							Kalimba_mode 		= 1; /*TTS_ONLY_MODE*/
	        
	       					ASR_active_model	= 0; /*RINGING_NODE*/

							theHeadset.TTS_ASR_Playing = true;

							AudioPlayTTS(	task, 
											(uint16)0, 								/*TTS_CALLERID_NUMBER, */
											(uint8*)TTS_text,					 	/*num_string, */
											sizeof(TTS_text), 
											Language,	/*theHeadset.tts_language, */
											FALSE,									/*FALSE*/  
											theHeadset.codec_task,					/*theHeadset.codec_task, */
											0, 										/*TonesGetToneVolume(FALSE), */
											0										/*theHeadset.features.stereo	*/
										);
						}
						#endif
						#endif
					}
					else
						TonesPlayEvent ( id ) ;
					#endif
				#endif
			}
			else
			{
				#ifdef CriticalBatteryPowerOff
					if((id == EventLowBattery) && (lState == headsetLowBattery))
					{
						MessageCancelAll( task , EventPowerOff ) ;
                				MessageSendLater( task , EventPowerOff , 0 , D_SEC(60) ) ;    
					}
				#endif					

				TonesPlayEvent ( id ) ;
			}
		}
#else
        TonesPlayEvent ( id ) ;
#endif
    }
    
#ifdef TEST_HARNESS 
    vm2host_send_event(id);
#endif
    
}

#ifdef Rubidium

void PairNewDevice_function(void)
{
#if 0
	/*ASR Test code*/
	Task task;

	task = (TaskData *) &rubidium_tts_asr;	

	memset(TTS_text, 0, sizeof(TTS_text));
	strcpy(TTS_text, "\\p=001");							
					
	Kalimba_mode		= 1; /*TTS_ONLY_MODE*/
				
	ASR_active_model	= 0; /*RINGING_NODE*/

	theHeadset.TTS_ASR_Playing = true;

	RUBI_DEBUG(("Rubi voice command > Pair new device!\n" ));
							
	AudioPlayTTS(	task, 
					(uint16)0,								/*TTS_CALLERID_NUMBER, */
					(uint8*)TTS_text,						/*num_string, */
					sizeof(TTS_text), 
					Language,	/*theHeadset.tts_language, */
					FALSE,									/*FALSE*/  
					theHeadset.codec_task,					/*theHeadset.codec_task, */
					0,										/*TonesGetToneVolume(FALSE), */
					0										/*theHeadset.features.stereo	*/
				);	

#else

	if(stateManagerGetState() == headsetConnectable)
	{
		#ifdef ActiveRubiASR
		Kalimba_mode 		= 1; /*TTS_ONLY_MODE*/
	        
	    ASR_active_model	= 0; /*RINGING_NODE*/

		theHeadset.TTS_ASR_Playing = true;
		#endif
		
		MessageSendLater (&theHeadset.task , EventEnterPairing , 0, 500 );
	}
	else if(stateManagerGetState() == headsetConnected)
	{
		#if 1
		/*No "Cancel" prompt at end of ASR after talking babble to the ASR*/
		Task task;
		
		task = (TaskData *) &rubidium_tts_asr;	
		
		memset(TTS_text, 0, sizeof(TTS_text));
		strcpy(TTS_text, "\\p=010");/*Cancelled*/							
						
		Kalimba_mode		= 1; /*TTS_ONLY_MODE*/
					
		ASR_active_model	= 0; /*RINGING_NODE*/
		
		theHeadset.TTS_ASR_Playing = true;
		
		RUBI_DEBUG(("Rubi voice command > \\p=010\n" ));
								
		AudioPlayTTS(	task, 
						(uint16)0,								/*TTS_CALLERID_NUMBER, */
						(uint8*)TTS_text,						/*num_string, */
						sizeof(TTS_text), 
						Language,	/*theHeadset.tts_language, */
						FALSE,									/*FALSE*/  
						theHeadset.codec_task,					/*theHeadset.codec_task, */
						0,										/*TonesGetToneVolume(FALSE), */
						0										/*theHeadset.features.stereo	*/
					);	
		#else
		TTSTerminate();
		theHeadset.TTS_ASR_Playing = false;
		#endif
	}
#endif
}

void Callback_function(void)
{
#if 0
	/*ASR Test code*/		
	Task task;

	task = (TaskData *) &rubidium_tts_asr;	

	memset(TTS_text, 0, sizeof(TTS_text));
	strcpy(TTS_text, "\\p=028");							
					
	Kalimba_mode		= 1; /*TTS_ONLY_MODE*/
				
	ASR_active_model	= 0; /*RINGING_NODE*/

	theHeadset.TTS_ASR_Playing = true;

	RUBI_DEBUG(("Rubi voice command > Call back!\n" ));
							
	AudioPlayTTS(	task, 
					(uint16)0,								/*TTS_CALLERID_NUMBER, */
					(uint8*)TTS_text,						/*num_string, */
					sizeof(TTS_text), 
					Language,	/*theHeadset.tts_language, */
					FALSE,									/*FALSE*/  
					theHeadset.codec_task,					/*theHeadset.codec_task, */
					0,										/*TonesGetToneVolume(FALSE), */
					0										/*theHeadset.features.stereo	*/
				);	

#else
#ifdef ActiveRubiASR

	Task task;
	headsetState lState = stateManagerGetState() ;

	task = (TaskData *) &rubidium_tts_asr;	

	memset(TTS_text, 0, sizeof(TTS_text));

	if(lState == headsetConnected)
	{
		strcpy(TTS_text, "\\p=028");
		RUBI_DEBUG(("Rubi voice prompt > \\p=028\n" ));
	}
	else
	{
		strcpy(TTS_text, "\\p=011");
		RUBI_DEBUG(("Rubi voice prompt > \\p=011\n" ));
	}
					
	Kalimba_mode 		= 1; /*TTS_ONLY_MODE*/
				
	ASR_active_model	= 0; /*RINGING_NODE*/

	theHeadset.TTS_ASR_Playing = true;
					
	AudioPlayTTS(	task, 
					(uint16)0, 								/*TTS_CALLERID_NUMBER, */
					(uint8*)TTS_text,					 	/*num_string, */
					sizeof(TTS_text), 
					Language,	/*theHeadset.tts_language, */
					FALSE,									/*FALSE*/  
					theHeadset.codec_task,					/*theHeadset.codec_task, */
					0, 										/*TonesGetToneVolume(FALSE), */
					0										/*theHeadset.features.stereo	*/
				);	

	if(lState == headsetConnected)
	{
		MessageSendLater (&theHeadset.task , EventPbapDialIch , 0, 3000 ) ;  
	}
#endif	
#endif
}

void Redial_function(void)
{
#if 0
		/*ASR Test code*/
		Task task;
		
		task = (TaskData *) &rubidium_tts_asr;	
		
		memset(TTS_text, 0, sizeof(TTS_text));

		strcpy(TTS_text, "\\p=012");
		
		RUBI_DEBUG(("Rubi voice command > Redial!\n" ));
		
		Kalimba_mode		= 1; /*TTS_ONLY_MODE*/
			
		ASR_active_model	= 0; /*RINGING_NODE*/

		theHeadset.TTS_ASR_Playing = true;
		
		AudioPlayTTS(	task, 
						(uint16)0,								/*TTS_CALLERID_NUMBER, */
						(uint8*)TTS_text,						/*num_string, */
						sizeof(TTS_text), 
						Language,	/*theHeadset.tts_language, */
						FALSE,									/*FALSE*/  
						theHeadset.codec_task,					/*theHeadset.codec_task, */
						0,										/*TonesGetToneVolume(FALSE), */
						0										/*theHeadset.features.stereo	*/
					);			
#else
#ifdef ActiveRubiASR

	Task task;
	headsetState lState = stateManagerGetState() ;

	task = (TaskData *) &rubidium_tts_asr;	

	memset(TTS_text, 0, sizeof(TTS_text));

	if(lState == headsetConnected)
	{
		strcpy(TTS_text, "\\p=012");
		RUBI_DEBUG(("Rubi voice prompt > \\p=012\n" ));
	}
	else
	{
		strcpy(TTS_text, "\\p=011");
		RUBI_DEBUG(("Rubi voice prompt > \\p=011\n" ));
	}

	Kalimba_mode 		= 1; /*TTS_ONLY_MODE*/
        
    ASR_active_model	= 0; /*RINGING_NODE*/

	theHeadset.TTS_ASR_Playing = true;
	
	AudioPlayTTS(	task, 
					(uint16)0, 								/*TTS_CALLERID_NUMBER, */
					(uint8*)TTS_text,					 	/*num_string, */
					sizeof(TTS_text), 
					Language,	/*theHeadset.tts_language, */
					FALSE,									/*FALSE*/  
					theHeadset.codec_task,					/*theHeadset.codec_task, */
					0, 										/*TonesGetToneVolume(FALSE), */
					0										/*theHeadset.features.stereo	*/
				);	

	if(lState == headsetConnected)
	{
		MessageSendLater (&theHeadset.task , EventLastNumberRedial , 0, 3000 ) ;
	}
#endif

#endif
}

void CheckBattery_function(void)
{
#ifdef ActiveRubiASR

	Task task;

	task = (TaskData *) &rubidium_tts_asr;	

	if(theHeadset.batt_level == POWER_BATT_LEVEL0 || theHeadset.batt_level == POWER_BATT_LOW)
	{
		/*Battery Low*/
		memset(TTS_text, 0, sizeof(TTS_text));
		strcpy(TTS_text, "\\p=023");
		RUBI_DEBUG(("Rubi voice prompt > \\p=023\n" ));
	}
	else if(theHeadset.batt_level == POWER_BATT_LEVEL1 || theHeadset.batt_level == POWER_BATT_LEVEL2)
	{
		/*Battery Medium*/
		memset(TTS_text, 0, sizeof(TTS_text));
		strcpy(TTS_text, "\\p=024");
		RUBI_DEBUG(("Rubi voice prompt > \\p=024\n" ));
	}
	#ifdef NewChargeMMI
	else if(theHeadset.batt_level == POWER_BATT_LEVEL3 || theHeadset.BHC612_Chargefull == 1)
	{
		/*Battery High*/
		memset(TTS_text, 0, sizeof(TTS_text));
		strcpy(TTS_text, "\\p=025");
		RUBI_DEBUG(("Rubi voice prompt > \\p=025\n" ));
	}
	#else
	else
	{
		/*Battery High*/
		memset(TTS_text, 0, sizeof(TTS_text));
		strcpy(TTS_text, "\\p=025");
		RUBI_DEBUG(("Rubi voice prompt > \\p=025\n" ));
	}
	#endif

	Kalimba_mode 		= 1; /*TTS_ONLY_MODE*/
        
    ASR_active_model	= 0; /*RINGING_NODE*/

	theHeadset.TTS_ASR_Playing = true;
	
	AudioPlayTTS(	task, 
					(uint16)0, 								/*TTS_CALLERID_NUMBER, */
					(uint8*)TTS_text,					 	/*num_string, */
					sizeof(TTS_text), 
					Language,	/*theHeadset.tts_language, */
					FALSE,									/*FALSE*/  
					theHeadset.codec_task,					/*theHeadset.codec_task, */
					0, 										/*TonesGetToneVolume(FALSE), */
					0										/*theHeadset.features.stereo	*/
				);	
#endif
}
#ifdef RubiTTSTerminate
void AnswerCall_function1(void)
{
	AudioDisconnect();

	theHeadset.sco_sink = 0;

	UnloadRubidiumEngine();

	theHeadset.TTS_ASR_Playing = false;

	theHeadset.PressCallButton = FALSE;
	
    MAIN_DEBUG(("Rubi > AnswerCall_function,PressCallButton : [F]\n" )) ;	
	
	MessageSend(&theHeadset.task , EventAnswer , 0) ;  
}
#endif
void AnswerCall_function(void)
{
/*#ifdef ActiveRubiASR*/
#ifdef Rubi_TTS
		AudioDisconnect();

		theHeadset.sco_sink = 0;

		UnloadRubidiumEngine();

		theHeadset.TTS_ASR_Playing = false;

		theHeadset.PressCallButton = FALSE;
		
	    MAIN_DEBUG(("Rubi > AnswerCall_function,PressCallButton : [F]\n" )) ;	
		
		/*MessageSendLater (&theHeadset.task , EventAnswer , 0, 2000 ) ; */ 
		/*MessageSendLater (&theHeadset.task , EventAnswer , 0, 500 ) ;*/  
		MessageSend(&theHeadset.task , EventAnswer , 0) ;  
#endif
}

void IgnoreCall_function(void)
{
/*#ifdef ActiveRubiASR*/
#ifdef Rubi_TTS

	TTSTerminate();

	UnloadRubidiumEngine();

	theHeadset.TTS_ASR_Playing = false;

	theHeadset.PressCallButton = FALSE;

	MAIN_DEBUG(("Rubi > IgnoreCall_function,PressCallButton : [F]\n" )) ;
    /* Reject incoming call - only valid for instances of HFP */ 
	
	/*MessageSendLater (&theHeadset.task , EventReject , 0, 2000 ) ;*/  
	MessageSendLater (&theHeadset.task , EventReject , 0, 500 ) ;
	
#endif
}

#endif

/*************************************************************************
NAME    
    handleHFPMessage

DESCRIPTION
    handles the messages from the user events

RETURNS

*/
static void handleHFPMessage  ( Task task, MessageId id, Message message )
{   
	#ifdef BHC612
		/*uint8 batt_value = 0;*/
	#endif

	MAIN_DEBUG(("HFP = [%x]\n", id)) ;   
    
    switch(id)
    {
        /* -- Handsfree Profile Library Messages -- */
    case HFP_INIT_CFM:              
        
        /* Init configuration that is required now */
	    InitEarlyUserFeatures();

        MAIN_DEBUG(("HFP_INIT_CFM - enable streaming[%x]\n", theHeadset.features.EnableA2dpStreaming)) ;   
        if(theHeadset.features.EnableA2dpStreaming)
        {
            ConnectionWriteClassOfDevice(AUDIO_MAJOR_SERV_CLASS | AV_COD_RENDER | AV_MAJOR_DEVICE_CLASS | AV_MINOR_HEADSET);        
        }
        else
        {
            ConnectionWriteClassOfDevice(AUDIO_MAJOR_SERV_CLASS | AV_MAJOR_DEVICE_CLASS | AV_MINOR_HEADSET);    
        }

        ConnectionWriteScanEnable(hci_scan_enable_inq_and_page);
		
        if  ( stateManagerGetState() == headsetLimbo ) 
        {
            if ( ((HFP_INIT_CFM_T*)message)->status == hfp_success )
                headsetInitComplete( (HFP_INIT_CFM_T*)message );
            else
                Panic();                
        }     

		#ifdef BHC612
			#ifdef PSBlueLED
				MAIN_DEBUG(("Init battery debug message\n"));
			
				#if 1
				appTempMInit(&theHeadset.theTemperature , 0);
				#endif

				#if 0
				if(stateManagerGetState() == headsetLimbo ) 
				{
					/*Charger LED0 ON*/
					LedConfigure(LED_0, LED_ENABLE, 1 ) ;
					LedConfigure(LED_0, LED_DUTY_CYCLE, (0xfff));
					LedConfigure(LED_0, LED_PERIOD, 0 );
					MessageSendLater (&theHeadset.task , EventDisableLEDS , 0, 3000 ) ;
				}
				#endif
			#endif

			#ifdef BHC612_MultipointEnable
				/*MAIN_DEBUG(("Multipoint[Enable]\n"));
				theHeadset.MultipointEnable = 1;*/
				
				if (stateManagerGetPdlSize()  > 1)
				{
					MAIN_DEBUG(("PDL size > 1 : Multipoint[Enable]\n"));
					theHeadset.MultipointEnable = 1;
				}
				else
				{
					MAIN_DEBUG(("PDL size < 2 : Multipoint[Disable]\n"));
					theHeadset.MultipointEnable = 0;
				}
				
			#else
				MAIN_DEBUG(("Multipoint[Disable]\n"));
				theHeadset.MultipointEnable = 0;
			#endif
		#endif		
        
    break;
 
    case HFP_SLC_CONNECT_IND:
        MAIN_DEBUG(("HFP_SLC_CONNECT_IND\n"));
        if (stateManagerGetState() != headsetLimbo)
        {   
            headsetHandleSlcConnectInd((HFP_SLC_CONNECT_IND_T *) message);

			#ifdef New_MMI
			if(theHeadset.BHC612_LinkLoss == true)
			{
				theHeadset.BHC612_TEMP = 0;
				theHeadset.BHC612_LinkLoss = false;
				theHeadset.BHC612_LinkLossReconnect = true;
				theHeadset.BHC612_VPLinkLoss = 0;
				MAIN_DEBUG(("SLC:Clear BHC612_LinkLoss flag!!!\n"));
				if(theHeadset.sco_sink != 0)
				{
					AudioDisconnect();
            		theHeadset.sco_sink = 0;
				}
				MessageSendLater(&theHeadset.task, EventEstablishSLC,0, 2500);/*Check BT connection again!*/
				MessageSendLater(&theHeadset.task, EventSpare2 , 0 , 3000);/*Recover Battery icon*/
			}
			#endif
        }
    break;

    case HFP_SLC_CONNECT_CFM:
        MAIN_DEBUG(("HFP_SLC_CONNECT_CFM [%x]\n", ((HFP_SLC_CONNECT_CFM_T *) message)->status ));
        if (stateManagerGetState() == headsetLimbo)
        {
            if ( ((HFP_SLC_CONNECT_CFM_T *) message)->status == hfp_success )
            {
                /*A connection has been made and we are now logically off*/
                headsetDisconnectAllSlc();   
            }
        }
        else
        {
            headsetHandleSlcConnectCfm((HFP_SLC_CONNECT_CFM_T *) message);
        }
        break;
		
    case HFP_SLC_LINK_LOSS_IND:
    {
        HFP_SLC_LINK_LOSS_IND_T* ind = (HFP_SLC_LINK_LOSS_IND_T*)message;

		#if 0
        if(ind->status == hfp_link_loss_recovery)
            MessageSend( &theHeadset.task , EventLinkLoss , 0 ) ;
		#endif
		
		#if 1
		if(theHeadset.MultipointEnable)
		{
			MessageCancelAll(task , EventLinkLoss);
			HfpSlcDisconnectRequest(ind->priority);	
			MAIN_DEBUG(("###HFP_SLC_LINK_LOSS_IND!!!\n"));
		}
		else
		{
			if(ind->status == hfp_link_loss_recovery)
				MessageSend( &theHeadset.task , EventLinkLoss , 0 ) ;
		}
		#endif
        
        /* determine whether it is required that the headset goes connectable during link loss */
        if(theHeadset.features.GoConnectableDuringLinkLoss)
        {
            /* go connectable during link loss */
            headsetEnableConnectable();   
        }
    }    
    break;
    
    case HFP_SLC_DISCONNECT_IND:
        MAIN_DEBUG(("HFP_SLC_DISCONNECT_IND\n"));
        /*MAIN_DEBUG(("Handle Disconnect\n"));*/
        headsetHandleSlcDisconnectInd((HFP_SLC_DISCONNECT_IND_T *) message);
    break;
    case HFP_SERVICE_IND:
        MAIN_DEBUG(("HFP_SERVICE_IND [%x]\n" , ((HFP_SERVICE_IND_T*)message)->service  ));
        headsetHandleServiceIndicator ( ((HFP_SERVICE_IND_T*)message) ) ;       
    break;
    /* indication of call status information, sent whenever a change in call status 
       occurs within the hfp lib */
    case HFP_CALL_STATE_IND:
        /* the Call Handler will perform headset state changes and be
           used to determine multipoint functionality */
        /* don't process call indications if in limbo mode */
        if(stateManagerGetState()!= headsetLimbo)
            headsetHandleCallInd((HFP_CALL_STATE_IND_T*)message);            
    break;


    case HFP_VOICE_TAG_NUMBER_IND:
        MAIN_DEBUG(("HFP_VOICE_TAG_NUMBER_IND\n"));
        headsetWriteStoredNumber((HFP_VOICE_TAG_NUMBER_IND_T*)message);
    break;
    case HFP_DIAL_LAST_NUMBER_CFM:
        MAIN_DEBUG(("HFP_LAST_NUMBER_REDIAL_CFM\n"));       
        handleHFPStatusCFM (((HFP_DIAL_LAST_NUMBER_CFM_T*)message)->status ) ;
    break;      
    case HFP_DIAL_NUMBER_CFM:
        MAIN_DEBUG(("HFP_DIAL_NUMBER_CFM %d %d\n", stateManagerGetState(), ((HFP_DIAL_NUMBER_CFM_T *) message)->status));
		handleHFPStatusCFM (((HFP_DIAL_NUMBER_CFM_T*)message)->status ) ;    
	break;
    case HFP_DIAL_MEMORY_CFM:
        MAIN_DEBUG(("HFP_DIAL_MEMORY_CFM %d %d\n", stateManagerGetState(), ((HFP_DIAL_MEMORY_CFM_T *) message)->status));        
    break ;     
    case HFP_CALL_ANSWER_CFM:
        MAIN_DEBUG(("HFP_ANSWER_CALL_CFM\n"));
    break;
    case HFP_CALL_TERMINATE_CFM:
        MAIN_DEBUG(("HFP_TERMINATE_CALL_CFM %d\n", stateManagerGetState()));       
    break;
    case HFP_VOICE_RECOGNITION_IND:
        MAIN_DEBUG(("HS: HFP_VOICE_RECOGNITION_IND_T [%c]\n" ,TRUE_OR_FALSE( ((HFP_VOICE_RECOGNITION_IND_T* )message)->enable) )) ;            
            /*update the state of the voice dialling on the back of the indication*/
        theHeadset.VoiceRecognitionIsActive = ((HFP_VOICE_RECOGNITION_IND_T* ) message)->enable ;            
    break;
    case HFP_VOICE_RECOGNITION_ENABLE_CFM:
        MAIN_DEBUG(("HFP_VOICE_RECOGNITION_ENABLE_CFM s[%d] w[%d]", (((HFP_VOICE_RECOGNITION_ENABLE_CFM_T *)message)->status ) , theHeadset.VoiceRecognitionIsActive));

            /*if the cfm is in error then we did not succeed - toggle */
        if  ( (((HFP_VOICE_RECOGNITION_ENABLE_CFM_T *)message)->status ) )
            theHeadset.VoiceRecognitionIsActive = 0 ;
            
        MAIN_DEBUG(("[%d]\n", theHeadset.VoiceRecognitionIsActive));

		#ifdef Rubi_VoicePrompt
			#if 0
			if(strcmp(TTS_text , ""))/*Return zero if equal*/
			{
				MAIN_DEBUG(("HFP_VOICE : UnloadRubiEngine***\n"));	
				UnloadRubidiumEngine();
				memset(TTS_text, 0, sizeof(TTS_text));  
			}
			#endif
		#endif
        
        handleHFPStatusCFM (((HFP_VOICE_RECOGNITION_ENABLE_CFM_T *)message)->status ) ;            
    break;
    case HFP_VOLUME_SYNC_SPEAKER_GAIN_IND:
    {
        bool set_gain = FALSE;
        HFP_VOLUME_SYNC_SPEAKER_GAIN_IND_T *ind = (HFP_VOLUME_SYNC_SPEAKER_GAIN_IND_T *) message;

        MAIN_DEBUG(("HFP_VOLUME_SYNC_SPEAKER_GAIN_IND %d\n", ind->volume_gain));        

        if( theHeadset.sco_sink && stateManagerGetState() != headsetA2DPStreaming )
        {
            hfp_link_priority priority = HfpLinkPriorityFromAudioSink(theHeadset.sco_sink);
          
            /* If there's an active call with audio and IND message is from the same AG, set speaker gain   */
            set_gain = (priority == ind->priority );
        }

		#if 1
        VolumeSetHeadsetVolume ( ind->volume_gain , theHeadset.features.PlayLocalVolumeTone , ind->priority, set_gain , 0) ;
		#else
		VolumeSetHeadsetVolume ( ind->volume_gain , theHeadset.features.PlayLocalVolumeTone , ind->priority, set_gain) ;
		#endif
    }
    break;
    case HFP_VOLUME_SYNC_MICROPHONE_GAIN_IND:
		{
        	MAIN_DEBUG(("HFP_VOLUME_SYNC_MICROPHONE_GAIN_IND %d\n", ((HFP_VOLUME_SYNC_MICROPHONE_GAIN_IND_T *) message)->mic_gain));        
			/*microphone indications deliberately not handled - this allows the audio plugin to control the mic volume at all times*/
	
			if(theHeadset.features.EnableSyncMuteMicophones)
			{
				HFP_VOLUME_SYNC_MICROPHONE_GAIN_IND_T *ind = (HFP_VOLUME_SYNC_MICROPHONE_GAIN_IND_T *) message;
				
				uint16	mic_gain = ind->mic_gain;
				
       			/* However, +VGM=0 or +VGA=x may be used to mute on/off microphone */
				if(mic_gain == 0)
				{
					if(!theHeadset.gMuted)
					{
						theHeadset.profile_data[PROFILE_INDEX(ind->priority)].audio.gAgSyncMuteFlag = 1;
						VolumeMuteOn();
					}
				}
				else
				{
					/* if headset has been muted on, mute off now */
					if(theHeadset.gMuted)
					{
						theHeadset.profile_data[PROFILE_INDEX(ind->priority)].audio.gAgSyncMuteFlag = 1;
						VolumeMuteOff();
					}
				}
			}
		}
	
	break;
	case HFP_RING_IND:
        MAIN_DEBUG(("HFP_RING_IND\n"));	
		#ifdef Rubidiumx
			headsetHandleRingInd((HFP_RING_IND_T *)message);
		#endif
    break;
	case HFP_CALLER_ID_ENABLE_CFM:
        MAIN_DEBUG(("HFP_CALLER_ID_ENABLE_CFM\n"));
		DEBUG(("HFP_CALLER_ID_ENABLE_CFM\n"));
    break;
	case HFP_CALLER_ID_IND:
        {		
			#ifdef Rubi_TTS
	      		HFP_CALLER_ID_IND_T *ind = (HFP_CALLER_ID_IND_T *) message;

				if((theHeadset.RepeatCallerIDFlag) && (!theHeadset.sco_sink))
				{
					/* ensure this is not a HSP profile */
					RUBI_DEBUG(("HFP_CALLER_ID_IND number %d,%s", ind->size_number, ind->caller_info + ind->offset_number));
					RUBI_DEBUG((" name %d,%s\n", ind->size_name , ind->caller_info + ind->offset_name));
				}
			#endif

			
		#ifdef Rubi_TTS
			if(theHeadset.Rubi_enable == 0)
			{
		        if ( theHeadset.sco_sink )
				{			
		        	/* Disconnect sco - e.g Inband ringing in some phone models*/        
		        	AudioDisconnect();
		        	/* clear sco_sink value to indicate no routed audio */

					theHeadset.sco_sink = 0;                                
					
					RUBI_DEBUG(("HFP_CALLER_ID , Rubi > AudioDisconnect...\n"));
					/*DEBUG(("HFP_CALLER_ID , Rubi > AudioDisconnect...\n"));*/
					
		        }
				else
				{
					if(theHeadset.RepeatCallerIDFlag)
						RUBI_DEBUG(("HFP_CALLER_ID , Rubi > !!!\n"));
					/*DEBUG(("HFP_CALLER_ID , Rubi > !!!,sco_sink = 0\n"));*/
				}
			}
        #endif
           
		if ( !theHeadset.sco_sink )
		{
				/*DEBUG(("if !theHeadset.sco_sink , TTSPlayCallerID\n"));*/
				/*MAIN_DEBUG(("if !theHeadset.sco_sink , TTSPlayCallerID,%d\n",TipCounter));*/

				#ifdef Rubi_TTS
				if(theHeadset.Rubi_enable == 0)
				{
					#ifdef MANDARIN_SUPPORT	/*Asia SKU : Ch+Us*/
						if(Language == AUDIO_TTS_LANGUAGE_CHINESE_MANDARIN)
						{
							#ifdef ChineseTTS
								if(theHeadset.BHC612_BattMeterOK)/*IPhone */
								{
									if(!TTSPlayCallerName (ind->size_name, ind->caller_info + ind->offset_name))
									{
										/* Caller name not present or not supported, try to play number */
										TTSPlayCallerNumber(ind->size_number, ind->caller_info + ind->offset_number) ;
									}
								}
								else
								{
									/*Android phone*/
									TTSPlayCallerNumber(ind->size_number, ind->caller_info + ind->offset_number) ;
								}
							#else
								/*DEBUG(("TTSPlayCallerNumber...\n"));*/
								TTSPlayCallerNumber(ind->size_number, ind->caller_info + ind->offset_number) ;
							#endif
						}
						else
						{
							#ifdef ChineseTTS
								if(theHeadset.BHC612_BattMeterOK)/*IPhone */
								{
									if(!TTSPlayCallerName (ind->size_name, ind->caller_info + ind->offset_name))
					       			{
					       				/* Caller name not present or not supported, try to play number */
										TTSPlayCallerNumber(ind->size_number, ind->caller_info + ind->offset_number) ;
					       			}
								}
								else
								{
									/*Android phone*/
									#if 1	/*Asia SKU, TTS_Language == English , Android phone*/
									if(ind->size_name != 0)
									{
										/*HTC One*/
										if(!TTSPlayCallerName (ind->size_name, ind->caller_info + ind->offset_name))
						       			{
						       				/* Caller name not present or not supported, try to play number */
											TTSPlayCallerNumber(ind->size_number, ind->caller_info + ind->offset_number) ;
						       			}
									}
									else
									{
										/*Samsung S2,S3*/
										TTSPlayCallerNumber(ind->size_number, ind->caller_info + ind->offset_number) ;
									}
									#else
									TTSPlayCallerNumber(ind->size_number, ind->caller_info + ind->offset_number) ;
									#endif
								}
							#else
								TTSPlayCallerNumber(ind->size_number, ind->caller_info + ind->offset_number) ;
							#endif
						}
					#else
					MAIN_DEBUG(("NA_HFP_CALLER_IND\n"));	
			       	/* Attempt to play caller name */
			       	if(!TTSPlayCallerName (ind->size_name, ind->caller_info + ind->offset_name))
			       	{
			       		/* Caller name not present or not supported, try to play number */
						TTSPlayCallerNumber(ind->size_number, ind->caller_info + ind->offset_number) ;
			       	}
				   	#endif
				}
			   	#endif
		}
        }
	break;           
    
	case HFP_UNRECOGNISED_AT_CMD_IND:
        {   
            uint16 i = 0 ;    

			#ifdef iOSBatteryMeter
				char* searchString = "+XAPL=iPhone,7";
				bool found = FALSE;
	            MAIN_DEBUG(("HFP_UNRECOGNISED_AT command = %s", ((HFP_UNRECOGNISED_AT_CMD_IND_T *)message)->data));
	            
	            while(i< ((HFP_UNRECOGNISED_AT_CMD_IND_T *)message)->size_data && (found == FALSE))
	            {
	                MAIN_DEBUG(("%s\n" ,(char*)(((HFP_UNRECOGNISED_AT_CMD_IND_T *)message)->data + i) )) ;
	                if (strncmp((char*)(((HFP_UNRECOGNISED_AT_CMD_IND_T *)message)->data + i), searchString, strlen(searchString)) == 0)
	                {
	                    MAIN_DEBUG(("string found +XAPL=iPhone,7\n"));        
	                    found = TRUE;
	                }
	                i++;    
	            }
				if(found)
				{
					theHeadset.BHC612_BattMeterOK = true;
					MAIN_DEBUG(("### iOS Battery Meter OK!\n" )) ;

					#ifdef ChineseTTS
						csr2csrHandleAgBatteryRequestRes(EventGasGauge0 + 0x5A);
					#endif
				}
			#else
	            char* searchString = "AT+MICTEST";
	            bool found = FALSE;
	            
	            MAIN_DEBUG(("HFP_UNRECOGNISED_AT_CMD_IND_T\n" )) ;
	            DEBUG(("AT command = %s", ((HFP_UNRECOGNISED_AT_CMD_IND_T *)message)->data));
	            
	            while(i< ((HFP_UNRECOGNISED_AT_CMD_IND_T *)message)->size_data && (found == FALSE))
	            {
	                DEBUG(("%s\n" ,(char*)(((HFP_UNRECOGNISED_AT_CMD_IND_T *)message)->data + i) )) ;
	                if (strncmp((char*)(((HFP_UNRECOGNISED_AT_CMD_IND_T *)message)->data + i), searchString, strlen(searchString)) == 0)
	                {
	                    DEBUG(("string found AT+MICTEST\n"));
	                    audioHandleMicSwitch();        
	                    found = TRUE;
	                }
	                i++;    
	            }
            #endif
        }
    break ;
    case HFP_HS_BUTTON_PRESS_CFM:
        {
            MAIN_DEBUG(("HFP_HS_BUTTON_PRESS_CFM\n")) ;
        }
    break ;
     /*****************************************************************/

#ifdef THREE_WAY_CALLING	
    case HFP_CALL_WAITING_ENABLE_CFM :
            MP_MAIN_DEBUG(("HS3 : HFP_CALL_WAITING_ENABLE_CFM_T [%c]\n", (((HFP_CALL_WAITING_ENABLE_CFM_T * )message)->status == hfp_success) ?'T':'F' )) ;
    break ;    
    case HFP_CALL_WAITING_IND:
        {
            /*change state accordingly*/
            stateManagerEnterThreeWayCallWaitingState();
            /* pass the indication to the multipoint handler which will determine if the call waiting tone needs
               to be played, this will depend upon whether the indication has come from the AG with
               the currently routed audio */
            mpHandleCallWaitingInd((HFP_CALL_WAITING_IND_T *)message);

			MP_MAIN_DEBUG(("HFP_CALL_WAITING_IND\n"));
        }
    break;

#endif	
    case HFP_SUBSCRIBER_NUMBERS_CFM:
        MP_MAIN_DEBUG(("HS3: HFP_SUBSCRIBER_NUMBERS_CFM [%c]\n" , (((HFP_SUBSCRIBER_NUMBERS_CFM_T*)message)->status == hfp_success)  ? 'T' :'F' )) ;
    break ;
    case HFP_SUBSCRIBER_NUMBER_IND:
#ifdef DEBUG_MAIN            
    {
        uint16 i=0;
            
        MP_MAIN_DEBUG(("HS3: HFP_SUBSCRIBER_NUMBER_IND [%d]\n" , ((HFP_SUBSCRIBER_NUMBER_IND_T*)message)->service )) ;
        for (i=0;i< ((HFP_SUBSCRIBER_NUMBER_IND_T*)message)->size_number ; i++)
        {
            MP_MAIN_DEBUG(("%c", ((HFP_SUBSCRIBER_NUMBER_IND_T*)message)->number[i])) ;
        }
        MP_MAIN_DEBUG(("\n")) ;
    } 
#endif
    break ;
    case HFP_CURRENT_CALLS_CFM:
        MP_MAIN_DEBUG(("HS3: HFP_CURRENT_CALLS_CFM [%c]\n", (((HFP_CURRENT_CALLS_CFM_T*)message)->status == hfp_success)  ? 'T' :'F' )) ;
    break ;
    case HFP_CURRENT_CALLS_IND:
        MP_MAIN_DEBUG(("HS3: HFP_CURRENT_CALLS_IND id[%d] mult[%d] status[%d]\n" ,
                                        ((HFP_CURRENT_CALLS_IND_T*)message)->call_idx , 
                                        ((HFP_CURRENT_CALLS_IND_T*)message)->multiparty  , 
                                        ((HFP_CURRENT_CALLS_IND_T*)message)->status)) ;
    break;
    case HFP_AUDIO_CONNECT_IND:
        MAIN_DEBUG(("HFP_AUDIO_CONNECT_IND\n")) ;
		#ifdef Rubidium
		if(theHeadset.TTS_ASR_Playing)
		{
			TTSTerminate();
			theHeadset.TTS_ASR_Playing = false;
			UnloadRubidiumEngine();
			AudioDisconnect();
            theHeadset.sco_sink = 0;
			MAIN_DEBUG(("Stop Rubi TTS\n"));
		}
		#endif
        audioHandleSyncConnectInd( (HFP_AUDIO_CONNECT_IND_T *)message ) ;
    break ;
    case HFP_AUDIO_CONNECT_CFM:
        MAIN_DEBUG(("HFP_AUDIO_CONNECT_CFM[%x][%x][%s%s%s] r[%d]t[%d]\n", ((HFP_AUDIO_CONNECT_CFM_T *)message)->status ,
                                                      (int)((HFP_AUDIO_CONNECT_CFM_T *)message)->audio_sink ,
                                                      ((((HFP_AUDIO_CONNECT_CFM_T *)message)->link_type == sync_link_sco) ? "SCO" : "" )      ,  
                                                      ((((HFP_AUDIO_CONNECT_CFM_T *)message)->link_type == sync_link_esco) ? "eSCO" : "" )    ,
                                                      ((((HFP_AUDIO_CONNECT_CFM_T *)message)->link_type == sync_link_unknown) ? "unk?" : "" ) ,
                                                      (int)((HFP_AUDIO_CONNECT_CFM_T *)message)->rx_bandwidth ,
                                                      (int)((HFP_AUDIO_CONNECT_CFM_T *)message)->tx_bandwidth 
                                                      )) ;
        audioHandleSyncConnectCfm ( (HFP_AUDIO_CONNECT_CFM_T *)message ) ;     
    break ;
    case HFP_AUDIO_DISCONNECT_IND:
        MAIN_DEBUG(("HFP_AUDIO_DISCONNECT_IND [%x]\n", ((HFP_AUDIO_DISCONNECT_IND_T *)message)->status)) ;
        audioHandleSyncDisconnectInd ((HFP_AUDIO_DISCONNECT_IND_T *)message) ;
    break ;
	case HFP_SIGNAL_IND:
        MAIN_DEBUG(("HS: HFP_SIGNAL_IND [%d]\n", ((HFP_SIGNAL_IND_T* )message)->signal )) ; 
    break ;
	case HFP_ROAM_IND:
        MAIN_DEBUG(("HS: HFP_ROAM_IND [%d]\n", ((HFP_ROAM_IND_T* )message)->roam )) ;
    break; 
	case HFP_BATTCHG_IND:     
        MAIN_DEBUG(("HS: HFP_BATTCHG_IND [%d]\n", ((HFP_BATTCHG_IND_T* )message)->battchg )) ;
    break;
    
/*******************************************************************/
	
	case HFP_CSR_FEATURES_TEXT_IND:
		csr2csrHandleTxtInd () ;
	break ;
    
    case HFP_CSR_FEATURES_NEW_SMS_IND:
	   csr2csrHandleSmsInd () ;   
	break ;
    
	case HFP_CSR_FEATURES_GET_SMS_CFM:
	   csr2csrHandleSmsCfm() ;
	break ;
    
	case HFP_CSR_FEATURES_BATTERY_LEVEL_REQUEST_IND:
	   csr2csrHandleAgBatteryRequestInd() ;
    break ;
    
/*******************************************************************/
	
/*******************************************************************/

    /*******************************/
    
    default :
        MAIN_DEBUG(("HS :  HFP ? [%x]\n",id)) ;
    break ;
    }
}

/*************************************************************************
NAME    
    handleCodecMessage
    
DESCRIPTION
    handles the codec Messages

RETURNS
    
*/
static void handleCodecMessage  ( Task task, MessageId id, Message message )
{
    MAIN_DEBUG(("CODEC MSG received [%x]\n", id)) ;
      
    if (id == CODEC_INIT_CFM ) 
    {       /* The codec is now initialised */
    
        if ( ((CODEC_INIT_CFM_T*)message)->status == codec_success) 
        {
            MAIN_DEBUG(("CODEC_INIT_CFM\n"));   
            headsetHfpInit();
            theHeadset.codec_task = ((CODEC_INIT_CFM_T*)message)->codecTask ;                   
        }
        else
        {
            Panic();
        }
    }
}

#ifdef CVC_PRODTEST
static void handleKalimbaMessage ( Task task, MessageId id, Message message )
{
    const DSP_REGISTER_T *m = (const DSP_REGISTER_T *) message;
	MAIN_DEBUG(("CVC: msg id[%x] a[%x] b[%x] c[%x] d[%x]\n", m->id, m->a, m->b, m->c, m->d));
	
    switch (m->id)
    {
        case 0x1000:
            MAIN_DEBUG(("CVC_READY\n"));
            /*CVC_LOADPARAMS_MSG, CVC_PS_BASE*/
            KalimbaSendMessage(0x1012, 0x2280, 0, 0, 0);
            break;
            
        case 0x1006:
            MAIN_DEBUG(("CVC_CODEC_MSG\n"));
            /*MESSAGE_SCO_CONFIG, sco_encoder, sco_config*/
            KalimbaSendMessage(0x2000, 0, 3, 0, 0);
            break;
        
        case 0x100c:
            MAIN_DEBUG (("CVC_SECPASSED_MSG\n"));
            exit(CVC_PRODTEST_PASS);
            break;
            
        case 0x1013:
            MAIN_DEBUG (("CVC_SECFAILD_MSG\n"));
            exit(CVC_PRODTEST_FAIL);
            break;
            
        default:
            MAIN_DEBUG(("m->id [%x]\n", m->id));
            break;    
     }
    
}
#endif

/* Handle any audio plugin messages */
static void handleAudioPluginMessage( Task task, MessageId id, Message message )
{
	switch (id)
    {        
		case AUDIO_PLUGIN_DSP_IND:
			/* Make sure this is the clock mismatch rate, sent from the DSP via the a2dp decoder common plugin */
			if (((AUDIO_PLUGIN_DSP_IND_T*)message)->id == KALIMBA_MSG_SOURCE_CLOCK_MISMATCH_RATE)
			{
				handleA2DPStoreClockMismatchRate(((AUDIO_PLUGIN_DSP_IND_T*)message)->value);
			}
			break;
        default:
            MAIN_DEBUG(("HS :  AUDIO ? [%x]\n",id)) ;
        break ;           
	}	
}

/*************************************************************************
NAME    
    app_handler
    
DESCRIPTION
    This is the main message handler for the Headset Application.  All
    messages pass through this handler to the subsequent handlers.

RETURNS

*/
static void app_handler(Task task, MessageId id, Message message)
{
    /*MAIN_DEBUG(("MSG [%x][%x][%x]\n", (int)task , (int)id , (int)&message)) ;*/
	
#ifdef ENABLE_ENERGY_FILTER
    if (id == MESSAGE_ENERGY_CHANGED) {
		Filter_EnergyChanged(message);
		return;
	}
#endif

    /* determine the message type based on base and offset */
    if ( ( id >= EVENTS_MESSAGE_BASE ) && ( id <= EVENTS_LAST_EVENT ) )
    {
        handleUEMessage(task, id,  message);          
    }
    else  if ( (id >= CL_MESSAGE_BASE) && (id <= CL_MESSAGE_TOP) )
    {
        handleCLMessage(task, id,  message);        
    #ifdef TEST_HARNESS 
        vm2host_connection(task, id, message);
    #endif 
    }
    else if ( (id >= HFP_MESSAGE_BASE ) && (id <= HFP_MESSAGE_TOP) )
    {     
        handleHFPMessage(task, id,  message);     
    #ifdef TEST_HARNESS 
        vm2host_hfp(task, id, message);
    #endif 
    }    
    else if ( (id >= CODEC_MESSAGE_BASE ) && (id <= CODEC_MESSAGE_TOP) )
    {     
        handleCodecMessage (task, id, message) ;     
    }
    else if ( (id >= POWER_MESSAGE_BASE ) && (id <= POWER_MESSAGE_TOP) )
    {     
        handleBatteryMessage (task, id, message) ;     
    }
#ifdef ENABLE_PBAP    
    else if ( (id >= PBAPC_INIT_CFM ) && (id <= PBAPC_MESSAGE_TOP) )
    {     
        handlePbapMessages (task, id, message) ;     
    }
#endif
#ifdef ENABLE_AVRCP
    else if ( (id >= AVRCP_INIT_CFM ) && (id <= AVRCP_MESSAGE_TOP) )
    {     
        headsetAvrcpHandleMessage (task, id, message) ;     
    }
#endif    
#ifdef CVC_PRODTEST
    else if (id == MESSAGE_FROM_KALIMBA)
    {
        handleKalimbaMessage(task, id, message);
    }
#endif    
	else if ( (id >= A2DP_MESSAGE_BASE ) && (id <= A2DP_MESSAGE_TOP) )
    {     
        handleA2DPMessage(task, id,  message);
		return;
    }
	else if ( (id >= AUDIO_UPSTREAM_MESSAGE_BASE ) && (id <= AUDIO_UPSTREAM_MESSAGE_TOP) )
    {     
        handleAudioPluginMessage(task, id,  message);
		return;
    }    
    else 
    { 
        MAIN_DEBUG(("MSGTYPE ? [%x]\n", id)) ;
    }       
}


/* The Headset Application starts here...*/
int main(void)
{  
	DEBUG (("Main [%s]\n",__TIME__));

    /* Initialise headset state */
	AuthResetConfirmationFlags();
    
    /*the internal regs must be latched on (smps and LDO)*/
    /*set the feature bits - these will be overwitten on configuration*/
    theHeadset.features.PowerOnSMPS    = TRUE ;
    theHeadset.features.PowerOnLDO    = TRUE ;
	#if 0
    PioSetPowerPin ( TRUE ) ;
	#endif
	
#ifdef BHC612
	if ( BootGetMode() != 0 && BootGetMode() != 2)
	{
		#ifdef New_MMI
			BHC612_Init();
		#endif
		
		#ifdef HW_DVT2
			#ifdef NewPSBlueLEDx
			checkPowerBluConn();			
			if(theHeadset.BHC612_PowerBluConn == BlueLDE_Detect_Disconnect)
				MAIN_DEBUG(("PS[PowerBlu]  exist!\n"));

			if(theHeadset.BHC612_PowerBluConn == BlueLDE_Detect_Disconnect)
				MAIN_DEBUG(("PS[PowerBlu]  not exist!\n"));
			#endif
		#endif
		
		MAIN_DEBUG(("Power station charge enable!\n"));
		PioSetPio(battery_low_io , PowerBlu_Connect);
		theHeadset.BHC612_BatteryLowState = PowerBlu_Connect;
		
		#ifdef New_MMI
			#ifdef PSBlueLED
			PioSetPio(PS_blue_led, PS_blue_led_off);
			MAIN_DEBUG(("BT LED Off 6\n"));
			#endif
		#endif
	 
	 	/* Read BHC612 Serial umber from PS if it exists */
	 	/*PSKEY_MOD_MANUF0*/    
		#if 0
		ret_len = PsFullRetrieve(0x025e, &theHeadset.BHC612_SN , 4);
		if(ret_len == 0)
		{
			MAIN_DEBUG(("SN not exist!\n"));
		}
		else
		{
			MAIN_DEBUG(("SN_1 = %x\n",theHeadset.BHC612_SN.sn_1));
			MAIN_DEBUG(("SN_2 = %x\n",theHeadset.BHC612_SN.sn_2));
			MAIN_DEBUG(("SN_3 = %x\n",theHeadset.BHC612_SN.sn_3));
			MAIN_DEBUG(("SN_4 = %x\n",theHeadset.BHC612_SN.sn_4));		
		}        
		#endif

		/*PSKEY_MOD_MANUF1*/
		#if 0
		ret_len = PsFullRetrieve(0x025f, &theHeadset.psdat, 1);
		if(ret_len == 0)
		{
			MAIN_DEBUG(("PSKEY_MOD_MANUF1 not exist!\n"));
		}
		else
		{	
			MAIN_DEBUG(("dat = %x\n",theHeadset.psdat));	
		}
		#endif

		#ifdef Rubidium
			memset(TTS_text, 0, sizeof(TTS_text));
			task1.handler = app_handler;
		#endif
	}
#endif

#ifdef CVC_PRODTEST
    /* set boot mode to 4 to check cVc license key in production */
    if( BootGetMode() == 4) 
    {
        /* check to see if license key checking is enabled */
        uint16 * buffer = PanicUnlessMalloc( sizeof(uint16) * sizeof(feature_config_type));
        char* kap_file = NULL;
        uint16 audio_plugin;
        uint16 lConfigID = get_config_id ( PSKEY_CONFIGURATION_ID ) ;
	    
        ConfigRetrieve(lConfigID , PSKEY_FEATURE_BLOCK, buffer, sizeof(feature_config_type)) ; 	
	                
        audio_plugin = (buffer[3] >> 8) & 0x7;
        MAIN_DEBUG(("buffer[3] = [%x]\n", buffer[3]));
        MAIN_DEBUG(("audio_plugin = [%x]\n", audio_plugin));
	   
        switch (audio_plugin)
        {
            case 2:
                /* 1 mic cvc */
                /* security check is the same in 1 mic */
                /* and 1 mic wideband */
                kap_file = "cvc_headset/cvc_headset.kap";
                break;
            case 3:
                /* 2 mic cvc */
                /* security check is the same in 2 mic */
                /* and 2 mic wideband */
                kap_file = "cvc_headset_2mic/cvc_headset_2mic.kap";
                break;
            default:
                /* no dsp */
                /* pass thru */
                /* default */
                /* no cvc license key check required for these states */
                /* so exit the application now */
                free(buffer);
                exit(CVC_PRODTEST_NO_CHECK);
                break;
        }
            
        theHeadset.task.handler = app_handler;
        MessageKalimbaTask(&theHeadset.task);
            
        if (FileFind(FILE_ROOT,(const char *) kap_file ,strlen(kap_file)) == FILE_NONE)
        {
            free(buffer);
            exit(CVC_PRODTEST_FILE_NOT_FOUND);
        }
        else
        {
            DEBUG (("KalimbaLoad [%s]\n", kap_file));
            KalimbaLoad(FileFind(FILE_ROOT,(const char *) kap_file ,strlen(kap_file)));
        }
        
        DEBUG (("StreamConnect\n"));
        StreamConnect(StreamKalimbaSource(0), StreamHostSink(0));
           
        free(buffer);
               
    }
    
    /* added this code here to construct else if statement */
    /* with BootGetMode check below */
    else 
        
#endif

#ifndef BHC612
	if ( BootGetMode() != 0)
#else		
	if ( BootGetMode() != 0 && BootGetMode() != 2)
#endif
	{
            /* Set up the Application task handler */
        theHeadset.task.handler = app_handler;
 
            /* Initialise the Connection Library */
        ConnectionInit(&theHeadset.task);

#ifdef TEST_HARNESS
        test_init();
#endif
    }
#ifdef BHC612
	else if ( BootGetMode() == 2) 
	{
		MAIN_DEBUG(("Test mdoe 2...\n"));

		#if 0
		InitEarlyUserFeatures();
		MAIN_DEBUG(("PowerInit...\n"));
		#endif
		
		#ifdef BlueCom
		appUartInit(&theHeadset.uart_M_task);
		#endif
	}
#endif	
    
    /* Start the message scheduler loop */
    MessageLoop();
        /* Never get here...*/
    return 0;  
}




#ifdef DEBUG_MALLOC 

#include "vm.h"

void * MallocPANIC ( const char file[], int line , size_t pSize )
{
    static uint16 lSize = 0 ;
    static uint16 lCalls = 0 ;
    void * lResult;
 
    lCalls++ ;
    lSize += pSize ;    
	printf("%s,l[%d][%d] s[%d] t[%d] a [%d]\n",file , line ,lCalls, pSize, lSize , (uint16)VmGetAvailableAllocations() ); 
                
    lResult = malloc ( pSize ) ;
      
        /*and panic if the malloc fails*/
    if ( lResult == NULL )
    {
        printf("MA : !\n") ;
        Panic() ;
    }
    
    return lResult ; 
                
}
#endif

/*************************************************************************
NAME    
    headsetInitCodecTask
    
DESCRIPTION
    Initialises the codec task

RETURNS

*/
static void headsetInitCodecTask ( void ) 
{
	/* The Connection Library has been successfully initialised, 
       initialise the HFP library to instantiate an instance of both
       the HFP and the HSP */
       			   
	/*CodecInitWolfson (Task appTask, wolfson_init_params *init) */
        
	/*init the codec task*/		
	CodecInitCsrInternal (&theHeadset.task) ;
}

/*************************************************************************
NAME    
    handleHFPStatusCFM
    
DESCRIPTION
    Handles a status response from the HFP and sends an error message if one was received

RETURNS

*/
static void handleHFPStatusCFM ( hfp_lib_status pStatus ) 
{

    if (pStatus != hfp_success )
    {
        MAIN_DEBUG(("HS: HFP CFM Err [%d]\n" , pStatus)) ;
        MessageSend ( &theHeadset.task , EventError , 0 ) ;
#ifdef ENABLE_PBAP
        if(theHeadset.pbap_dial_state != pbapc_no_dial)
        {
            MessageSend ( &theHeadset.task , EventPbapDialFail , 0 ) ; 
        } 
#endif        
    }
    else
    {
#ifdef ENABLE_PBAP
        if(theHeadset.pbap_dial_state != pbapc_no_dial)
        {
            MessageSend ( &theHeadset.task , EventPbapDialSuccessful , 0 ) ; 
        }
#endif             
    }

#ifdef ENABLE_PBAP
    theHeadset.pbap_dial_state = pbapc_no_dial;
#endif
}

