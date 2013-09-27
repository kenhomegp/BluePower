/****************************************************************************

FILE NAME
    headset_configmanager.h
    
DESCRIPTION
    Configuration manager for the headset - resoponsible for extracting user information out of the 
    PSKEYs and initialising the configurable nature of the headset components
    
*/
#ifndef HEADSET_POWER_MANAGER_H
#define HEADSET_POWER_MANAGER_H

#include <csrtypes.h>
#include <power.h>

/****************************************************************************
NAME    
    powerManagerConfig
    
DESCRIPTION
  	Configure power management
    
RETURNS
    void
*/
bool powerManagerConfig(const power_type * config);


/****************************************************************************
NAME    
    powerManagerChargerConnected
    
DESCRIPTION
  	This function is called when the charger is plugged into the headset
    
RETURNS
    void
*/
void powerManagerChargerConnected(void);


/****************************************************************************
NAME    
    powerManagerChargerDisconnected
    
DESCRIPTION
  	This function is called when the charger is unplugged from the headset
    
RETURNS
    void
*/
void powerManagerChargerDisconnected(void);


/*************************************************************************
NAME    
    handleBatteryMessage
    
DESCRIPTION
    handles the Battery/Charger Monitoring Messages

RETURNS
    
*/
void handleBatteryMessage( Task task, MessageId id, Message message );


/****************************************************************************
NAME
    BatteryIsBatteryLow

DESCRIPTION
  	Call this function to check the low battery warning state

RETURNS
    TRUE or FALSE
*/
#define BatteryIsBatteryLow() (theHeadset.battery_low_state) 


/****************************************************************************
NAME    
    ChargerIsChargerConnected
    
DESCRIPTION
  	This function is called by applications to check whether the charger has been 
	plugged into the headset
    
RETURNS
    void
*/
bool ChargerIsChargerConnected(void);

/****************************************************************************
NAME    
    BatteryUserInitiatedRead
    
DESCRIPTION
  	Call this function to take an immediate battery reading and sent to AG.
    
RETURNS
    void
*/
void BatteryUserInitiatedRead( void );

/****************************************************************************
NAME    
    LBIPMPowerLevel
    
DESCRIPTION
  	Returns the Power level to use for Low Battery Intelligent Power Management (LBIPM)
    Note will always return high level if this feature is disabled.
    
RETURNS
    void
*/
power_battery_level LBIPMPowerLevel( void );

#endif 
