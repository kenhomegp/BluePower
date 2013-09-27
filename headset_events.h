/****************************************************************************

FILE NAME
    headset_events.h
    
DESCRIPTION
    Defines Headset user events
    
*/
#ifndef HEADSET_EVENTS_H
#define HEADSET_EVENTS_H


#ifndef BC4_HS_CONFIGURATOR
    #include <connection.h>
    #include <message.h>
    #include <app/message/system_message.h>
    #include <stdio.h>
#endif


#define EVENTS_MESSAGE_BASE (0x6000)

/*This enum is used as an index in an array - do not edit - without thinking*/
typedef enum headsetEventsTag
{
/*0x00*/    EventInvalid = EVENTS_MESSAGE_BASE,    
/*0x01*/    EventPowerOn , 
/*0x02*/    EventPowerOff ,  
/*0x03*/    EventEnterPairing ,
    
/*0x04*/    EventInitateVoiceDial ,
/*0x05*/    EventLastNumberRedial ,
/*0x06*/    EventAnswer , 
/*0x07*/    EventReject , 
    
/*0x08*/    EventCancelEnd , 
/*0x09*/    EventTransferToggle ,
/*0x0A*/    EventToggleMute   ,
/*0x0B*/    EventVolumeUp  ,
    
/*0x0C*/    EventVolumeDown ,
/*0x0D*/    EventToggleVolume,
/*0x0E*/    EventThreeWayReleaseAllHeld,
/*0x0F*/    EventThreeWayAcceptWaitingReleaseActive,
    
/*0x10*/    EventThreeWayAcceptWaitingHoldActive  ,
/*0x11*/    EventThreeWayAddHeldTo3Way  ,
/*0x12*/    EventThreeWayConnect2Disconnect,  
/*0x13*/    EventEnableDisableLeds  ,
    
/*0x14*/    EventResetPairedDeviceList,
/*0x15*/    EventEnterDutMode ,
/*0x16*/    EventPairingFail,
/*0x17*/    EventPairingSuccessful,
    
/*0x18*/    EventSCOLinkOpen ,
/*0x19*/    EventSCOLinkClose,
/*0x1A*/    EventLowBattery,
/*0x1B*/    EventEndOfCall ,
    
/*0x1C*/    EventEstablishSLC ,
/*0x1D*/    EventLEDEventComplete,
/*0x1E*/    EventTrickleCharge,
/*0x1F*/    EventAutoSwitchOff,
    
/*0x20*/    EventFastCharge,
/*0x21*/    EventOkBattery,
/*0x22*/    EventChargerConnected,
/*0x23*/    EventChargerDisconnected,

/*0x24*/    EventSLCDisconnected ,
/*0x25*/    EventBatteryLevelRequest ,
/*0x26*/    EventLinkLoss ,
/*0x27*/    EventLimboTimeout ,

/*0x28*/    EventMuteOn ,
/*0x29*/    EventMuteOff ,
/*0x2a*/    EventMuteReminder ,
/*0x2b*/    EventResetComplete,

/*0x2C*/    EventEnterTXContTestMode,
/*0x2D*/    EventEnterDUTState,
/*0x2E*/    EventVolumeOrientationNormal,
/*0x2F*/    EventVolumeOrientationInvert,

/*0x30*/    EventNetworkOrServiceNotPresent,
/*0x31*/    EventNetworkOrServicePresent,
/*0x32*/    EventEnableLEDS,
/*0x33*/    EventDisableLEDS,

/*0x34*/    EventSLCConnected ,
/*0x35*/    EventError,
/*0x36*/    EventLongTimer,
/*0x37*/    EventVLongTimer,

/*0x38*/    EventEnablePowerOff,
/*0x39*/    EventChargeError,
/*0x3A*/    EventPlaceIncomingCallOnHold,
/*0x3B*/    EventAcceptHeldIncomingCall,

/*0x3C*/    EventRejectHeldIncomingCall,
/*0x3D*/	EventCancelLedIndication ,
/*0x3E*/    EventCallAnswered ,
/*0x3F*/    EventChargeErrorInIdleState,

/*0x40*/	EventReconnectFailed,
/*0x41*/    EventGasGauge0,
/*0x42*/    EventGasGauge1,
/*0x43*/    EventGasGauge2,

/*0x44*/    EventGasGauge3,
/*0x45*/    EventCheckForAudioTransfer,
/*0x46*/    EventEnterDFUMode ,
/*0x47*/	EventUnused , 
			
/*0x48*/	EventEnterServiceMode,
/*0x49*/    EventServiceModeEntered ,
/*0x4A*/	EventAudioMessage1,
/*0x4B*/	EventAudioMessage2,
			
/*0x4C*/	EventAudioMessage3,
/*0x4D*/	EventAudioMessage4,
/*0x4E*/    EventCancelHSPIncomingCall,
/*0x4f*/	EventDialStoredNumber,

/*0x50*/    EventCheckForLowBatt,
/*0x51*/    EventBattTempOutOfRange,
/*0x52*/	EventRestoreDefaults,
/*0x53*/    EventChargerGasGauge0,
			
/*0x54*/    EventChargerGasGauge1,
/*0x55*/    EventChargerGasGauge2,
/*0x56*/    EventChargerGasGauge3,
/*0x57*/    EventContinueSlcConnectRequest,
            
/*0x58*/    EventConnectableTimeout,
/*0x59*/    EventLastNumberRedial_AG2,
/*0x5A*/    EventInitateVoiceDial_AG2,
/*0x5B*/	EventConfirmationAccept,
			
/*0x5C*/	EventConfirmationReject,
/*0x5D*/	EventToggleDebugKeys,            
/*0x5E*/	EventTone1,
/*0x5F*/	EventTone2,
			
/*0x60*/	EventSelectTTSLanguageMode,
/*0x61*/	EventConfirmTTSLanguage,
/*0x62*/	EventEnableMultipoint,
/*0x63*/	EventDisableMultipoint,
            
/*0x64*/    EventSpare2,
/*0x65*/    EventSLCConnectedAfterPowerOn,
/*0x66*/    EventResetLEDTimeout,
/*0x67*/    EventStartPagingInConnState,
            
/*0x68*/    EventStopPagingInConnState,
/*0x69*/    EventMultipointCallWaiting,
/*0x6A*/	EventRefreshEncryption,
/*0x6B*/    EventSpare3,

/*0x6C*/	EventUnused1,
/*0x6D*/	EventUnused2,
/*0x6E*/	EventUnused3,
/*0x6F*/	EventUnused4,

/*0x70*/	EventUnused5,
/*0x71*/	EventUnused6,	
/*0x72*/    EventRssiPair,
/*0x73*/    EventRssiPairReminder,

/*0x74*/	EventRssiPairTimeout,
/*0x75*/    EventBoostCharge,
/*0x76*/    EventCheckRole,
/*0x77*/    EventMissedCall,

/*0x78*/	EventA2dpPlay,
/*0x79*/	EventA2dpConnected,
/*0x7a*/	EventA2dpDisconnected,
/*0x7b*/	EventEstablishA2dp,

/*0x7c*/	EventConnectA2dpLinkLoss,
/*0x7d*/	EventContinueA2dpListConn,
/*0x7e*/	EventVolumeMax,
/*0x7f*/	EventVolumeMin,

/*0x80*/	EventA2dpReconnectFailed,
/*0x81*/    EventConfirmationRequest,
/*0x82*/    EventPasskeyDisplay,
/*0x83*/    EventPinCodeRequest,

/*0x84*/    EventEnableIIR,
/*0x85*/    EventDisableIIR,             
/*0x86*/    EventPbapDialMch,
/*0x87*/    EventPbapDialIch,
            
/*0x88*/    EventPbapDialSuccessful,
/*0x89*/    EventPbapDialFail, 
/*0x8A*/    EventSetWbsCodecs,
/*0x8B*/    EventOverrideResponse,
            
/*0x8C*/    EventCreateAudioConnection,
/*0x8D*/    EventSetWbsCodecsSendBAC,
/*0x8E*/    EventUpdateStoredNumber,            
/*0x8F*/    EventEnableIntelligentPowerManagement,
            
/*0x90*/    EventDisableIntelligentPowerManagement,
/*0x91*/    EventToggleIntelligentPowerManagement,
/*0x92*/    EventEnterBootMode2

#ifdef ENABLE_AVRCP
            ,EventAvrcpPlayPause,
            EventAvrcpStop,
            EventAvrcpSkipForward,
            EventAvrcpSkipBackward,
            EventAvrcpFastForwardPress,
            EventAvrcpFastForwardRelease,
            EventAvrcpRewindPress,
            EventAvrcpRewindRelease
#endif       

} headsetEvents_t; 


#ifdef ENABLE_AVRCP
#define EVENTS_LAST_EVENT EventAvrcpRewindRelease
#else
#define EVENTS_LAST_EVENT EventEnterBootMode2
#endif


#define EVENTS_MAX_EVENTS ( (EVENTS_LAST_EVENT - EVENTS_MESSAGE_BASE) + 1 )

#endif
