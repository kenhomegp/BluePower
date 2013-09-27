/****************************************************************************

FILE NAME
    headset_volume.h
    
DESCRIPTION

    
*/

#ifndef HEADSET_VOLUME_H
#define HEADSET_VOLUME_H


#include "headset_volume.h"

#include <hfp.h>


#define VOL_NUM_VOL_SETTINGS     (16)


typedef enum  
{
    increase_volume,
    decrease_volume
}volume_direction;

#define VOLUME_A2DP_MAX_LEVEL 15
#define VOLUME_A2DP_MIN_LEVEL 0
#define VOLUME_A2DP_MUTE_GAIN 0


/****************************************************************************
NAME 
 CheckVolumeA2dp

DESCRIPTION
 check whether any a2dp connections are present and if these are currently active
 and routing audio to the headset, if that is the case adjust the volume up or down
 as appropriate

RETURNS
 void
    
*/
bool CheckVolumeA2dp(volume_direction dir);

/****************************************************************************
NAME 
 VolumeUp

DESCRIPTION
 Increments volume

RETURNS
 void
    
*/
void VolumeUp( void );

/****************************************************************************
NAME 
 VolumeDown

DESCRIPTION
 Decrements volume - sends a response to the AG

RETURNS
 void
    
*/
void VolumeDown( void );

/****************************************************************************
NAME 
 VolumeToggleMute

DESCRIPTION
 Toggles the mute state

RETURNS
 void
    
*/
void VolumeToggleMute( void );

/****************************************************************************
NAME 
 VolumeMuteOn

DESCRIPTION
 Enables Mute

RETURNS
 void
    
*/
void VolumeMuteOn( void );

/****************************************************************************
NAME 
 VolumeMuteOff

DESCRIPTION
 Disables Mute

RETURNS
 void
    
*/
void VolumeMuteOff( void );


/****************************************************************************
NAME 
 VolumeSendAndSetHeadsetVolume

DESCRIPTION
    sets the vol to the level corresponding to the phone volume level
    In addition - send a response to the AG indicating new volume level

RETURNS
 void
    
*/
#if 1
void VolumeSendAndSetHeadsetVolume( uint16 pNewVolume , bool pPlayTone, hfp_link_priority priority, volume_direction dir ); 
#else
void VolumeSendAndSetHeadsetVolume( uint16 pNewVolume , bool pPlayTone , hfp_link_priority priority );
#endif

/****************************************************************************
NAME 
 VolumeSetHeadsetVolume

DESCRIPTION
 sets the internal speaker gain to the level corresponding to the phone volume level
    
RETURNS
 void
    
*/
#if 1
void VolumeSetHeadsetVolume( uint16 pNewVolume , bool pPlayTone, hfp_link_priority priority, bool set_gain, volume_direction dir );
#else
void VolumeSetHeadsetVolume( uint16 pNewVolume , bool pPlayTone, hfp_link_priority priority, bool set_gain );
#endif

/****************************************************************************
NAME 
 VolumeSetMicVolume
DESCRIPTION
    sends the current microphone volume to the AG on connection
    
RETURNS
 void
    
*/
void VolumeSendMicrophoneVolume( uint16 pVolume );

#if 1
/****************************************************************************
NAME 
 VolumeSetA2dp
DESCRIPTION
    sets the current A2dp volume
    
RETURNS
 void
    
*/
void VolumeSetA2dp(uint16 index, uint16 oldVolume, volume_direction dir);
#else
void VolumeSetA2dp(uint16 index, uint16 oldVolume);
#endif        

#endif

