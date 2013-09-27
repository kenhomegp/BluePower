/*!
@file    headset_a2dp.h
@brief   Interface to the headset a2dp profile initialisation functions. 
*/

#ifndef _HEADSET_A2DP_INIT_H_
#define _HEADSET_A2DP_INIT_H_


#include "headset_private.h"
#include <a2dp.h>

/* Local stream end point codec IDs */
#define SBC_SEID                1           /*!< @brief Local Stream End Point ID for SBC codec */
#define MP3_SEID                2           /*!< @brief Local Stream End Point ID for MP3 codec */
#define AAC_SEID                3           /*!< @brief Local Stream End Point ID for AAC codec */
#define FASTSTREAM_SEID         4           /*!< @brief Local Stream End Point ID for FastStream codec */

#ifdef A2DP_EXTRA_CODECS
    #define NUM_SEPS                (FASTSTREAM_SEID)  /*!< @brief The total number of SEPs */
#else
    #define NUM_SEPS                (SBC_SEID)  /*!< @brief The total number of SEPs */
#endif

/* The bits used to enable codec support for A2DP, as read from PSKEY_CODEC_ENABLED */
#define MP3_CODEC_BIT           0           /*!< @brief Bit used to enable MP3 codec in PSKEY */
#define AAC_CODEC_BIT           1           /*!< @brief Bit used to enable AAC codec in PSKEY */
#define FASTSTREAM_CODEC_BIT    2           /*!< @brief Bit used to enable FastStream codec in PSKEY */
#define KALIMBA_RESOURCE_ID     1           /*!< @brief Resource ID for Kalimba */

#define MAX_A2DP_CONNECTIONS 2
#define primary_a2dp 0
#define secondary_a2dp 1

#define for_all_a2dp(idx)      for(idx = 0; idx < MAX_A2DP_CONNECTIONS; idx++)

#ifdef ENABLE_AVRCP
typedef enum
{
    avrcp_support_unknown,
    avrcp_support_second_attempt,
    avrcp_support_unsupported,
    avrcp_support_supported
} avrcpSupport;
#endif

typedef struct
{
    bool connected[MAX_A2DP_CONNECTIONS];
    bool SuspendState[MAX_A2DP_CONNECTIONS];
    uint16 device_id[MAX_A2DP_CONNECTIONS];
    uint16 stream_id[MAX_A2DP_CONNECTIONS];
    uint8 gAvVolumeLevel[MAX_A2DP_CONNECTIONS];
    bdaddr bd_addr[MAX_A2DP_CONNECTIONS];
    uint16 clockMismatchRate[MAX_A2DP_CONNECTIONS];
#ifdef ENABLE_AVRCP
    avrcpSupport avrcp_support[MAX_A2DP_CONNECTIONS];
#endif    

}a2dp_data;

/****************************************************************************
  FUNCTIONS
*/

/*************************************************************************
NAME    
    InitA2dp
    
DESCRIPTION
    This function initialises the A2DP library.
*/
void InitA2dp(void);

/*************************************************************************
NAME    
    getA2dpIndex
    
DESCRIPTION
    This function tries to find a device id match in the array of a2dp links 
    to that device id passed in

RETURNS
    match status of true or false
**************************************************************************/
bool getA2dpIndex(uint16 DeviceId, uint16 * Index);

/*************************************************************************
NAME    
    InitSeidConnectPriority
    
DESCRIPTION
    Retrieves a list of the preferred Stream End Points to connect with.
*/
uint16 InitSeidConnectPriority(uint8 seid, uint8 *seid_list);


/*************************************************************************
NAME    
    getA2dpPlugin
    
DESCRIPTION
    Retrieves the audio plugin for the requested SEP.
*/
Task getA2dpPlugin(uint8 seid);

/****************************************************************************
NAME    
    headsetA2dpInitComplete
    
DESCRIPTION
    Headset A2DP initialisation has completed, check for success. 

RETURNS
    void
**************************************************************************/
void headsetA2dpInitComplete(const A2DP_INIT_CFM_T *msg);

/*************************************************************************
NAME    
    handleA2DPSignallingConnected
    
DESCRIPTION
    handle a successfull confirm of a signalling channel connected

RETURNS
    
**************************************************************************/
void handleA2DPSignallingConnected(a2dp_status_code status, uint16 DeviceId, bdaddr SrcAddr);

/*************************************************************************
NAME    
    handleA2DPOpenInd
    
DESCRIPTION
    handle an indication of an media channel open request, decide whether 
    to accept or reject it

RETURNS
    
**************************************************************************/
void handleA2DPOpenInd(uint16 DeviceId);

/*************************************************************************
NAME    
    handleA2DPOpenCfm
    
DESCRIPTION
    handle a successfull confirm of a media channel open

RETURNS
    
**************************************************************************/
void handleA2DPOpenCfm(uint16 DeviceId, uint16 StreamId, a2dp_status_code status);

/*************************************************************************
NAME    
    handleA2DPSignallingDisconnected
    
DESCRIPTION
    handle the disconnection of the signalling channel
RETURNS
    
**************************************************************************/
void handleA2DPSignallingDisconnected(uint16 DeviceId, a2dp_status_code status,  bdaddr SrcAddr);

/*************************************************************************
NAME    
    handleA2DPStartStreaming
    
DESCRIPTION
    handle the indication of media start from either the ind or the cfm
RETURNS
    
**************************************************************************/
void handleA2DPStartStreaming(uint16 DeviceId, uint16 StreamId, a2dp_status_code status);

/*************************************************************************
NAME    
    handleA2DPSuspendStreaming
    
DESCRIPTION
    handle the indication of media suspend from either the ind or the cfm
RETURNS
    
**************************************************************************/
void handleA2DPSuspendStreaming(uint16 DeviceId, uint16 StreamId, a2dp_status_code status);


/*************************************************************************
NAME    
    handleA2DPStoreClockMismatchRate
    
DESCRIPTION
    handle storing the clock mismatch rate for the active stream
RETURNS
    
**************************************************************************/
void handleA2DPStoreClockMismatchRate(uint16 clockMismatchRate);

/*************************************************************************
NAME    
    SuspendA2dpStream
    
DESCRIPTION
    called when it is necessary to suspend an a2dp media stream due to 
    having to process a call from a different AG 
RETURNS
    
**************************************************************************/
void SuspendA2dpStream(uint8 index);
        


#ifdef ENABLE_AVRCP
bool getA2dpVolume(const bdaddr *bd_addr, uint16 *a2dp_volume);


bool setA2dpVolume(const bdaddr *bd_addr, uint16 a2dp_volume);
#endif


/*************************************************************************
NAME    
    handleA2DPMessage
    
DESCRIPTION
    A2DP message Handler, this function handles all messages returned
    from the A2DP library and calls the relevant functions if required

RETURNS
    
**************************************************************************/
void handleA2DPMessage( Task task, MessageId id, Message message );

#endif /* _HEADSET_A2DP_INIT_H_ */


