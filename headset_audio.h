#ifndef _HEADSET_AUDIO_H_
#define _HEADSET_AUDIO_H_


#include "headset_private.h"

/* Linked-list structure for registering user implemented WBS codecs. */
typedef struct _audio_codec_plugin_info
{
    uint32      					bandwidth;			/* Codec audio paramters - bandwidth */
    uint16      					max_latency;		/* Codec audio paramters - max latency */
    uint16     	 					voice_settings;		/* Codec audio paramters - vioce settings */
    uint16      					retx_effort;		/* Codec audio paramters - re-transmission effort */
    uint16							packet_types;		/* Codec's supported packet types */
} audio_codec_plugin_info;

#ifdef A2DP_Dock
bool GetA2DPState(void);
#endif
 
/****************************************************************************
NAME    
    audioGetLinkPriority
    
DESCRIPTION
	Common method of getting the link we want to manipulate audio settings on

RETURNS
    
*/
hfp_link_priority audioGetLinkPriority ( bool audio );

/****************************************************************************
NAME    
    audioHandleRouting
    
DESCRIPTION
	Handle the routing of the audio connections or connection based on
    sco priority level

RETURNS
    
*/
void audioHandleRouting ( void );

/****************************************************************************
NAME    
    audioHandleSyncConnectInd
    
DESCRIPTION

RETURNS
    
*/
void audioHandleSyncConnectInd ( const HFP_AUDIO_CONNECT_IND_T *pInd );

/****************************************************************************
NAME    
    audioHandleSyncConnectInd
    
DESCRIPTION

RETURNS
    
*/
void audioHandleSyncConnectCfm ( const HFP_AUDIO_CONNECT_CFM_T * pCfm );

/****************************************************************************
NAME    
    audioHandleSyncConnectInd
    
DESCRIPTION

RETURNS
    
*/
void audioHandleSyncDisconnectInd ( const HFP_AUDIO_DISCONNECT_IND_T * pInd ) ;

/****************************************************************************
DESCRIPTION
    toggles a custom audio processing feature
*/
void audioToggleFeature1 ( void ) ;

/****************************************************************************
DESCRIPTION
    toggles a custom audio processing feature
*/
void audioToggleFeature2 ( void ) ;

/****************************************************************************
NAME    
    audioHfpConnectAudio
    
DESCRIPTION
	attempt to reconnect an audio connection from the sink value associcated 
    with the passed hfp instance

RETURNS
    
*/
void audioHfpConnectAudio ( hfp_link_priority   priority );

/****************************************************************************
NAME    
    A2dpRouteAudio
    
DESCRIPTION
	attempt to connect an audio connection from a2dp device via the passed in 
    deviceID

RETURNS
    
*/
void A2dpRouteAudio(uint8 Index, Sink sink);

/****************************************************************************
NAME    
    audioHandleMicSwitch
    
DESCRIPTION
	Handle AT+MICTEST AT command from TestAg. 
    This command swaps between the two microphones to test 2nd mic in production.

RETURNS
    
*/
void audioHandleMicSwitch( void );

void CreateAudioConnection(void);

#endif 


