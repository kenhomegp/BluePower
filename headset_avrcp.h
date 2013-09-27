#ifdef ENABLE_AVRCP

/*!
@file    headset_avrcp.h
@brief   Interface to the AVRCP profile functionality. 
*/

#ifndef _HEADSET_AVRCP_H_
#define _HEADSET_AVRCP_H_


#include <message.h>
#include <app/message/system_message.h>

#include "headset_volume.h"

#include <avrcp.h>


#define MAX_AVRCP_CONNECTIONS 2 /* max AVRCP connections allowed */

#define DEFAULT_AVRCP_CONNECTION_DELAY 2000 /* delay in millsecs before connecting AVRCP after A2DP signalling connection */

#define AVRCP_ABS_VOL_STEP_CHANGE 8 /* absolute volume change for each local volume up/down */

#define AVRCP_MAX_ABS_VOL 127 /* the maximum value for absolute volume as defined in AVRCP spec */

#define AVRCP_MAX_PENDING_COMMANDS 4 /* maximum AVRCP commands that can be queued */

#define for_all_avrcp(idx)      for(idx = 0; idx < MAX_AVRCP_CONNECTIONS; idx++)

typedef enum
{
    AVRCP_CTRL_PAUSE_PRESS,
    AVRCP_CTRL_PAUSE_RELEASE,
    AVRCP_CTRL_PLAY_PRESS,
    AVRCP_CTRL_PLAY_RELEASE,
    AVRCP_CTRL_FORWARD_PRESS,
    AVRCP_CTRL_FORWARD_RELEASE,
    AVRCP_CTRL_BACKWARD_PRESS,
    AVRCP_CTRL_BACKWARD_RELEASE,
    AVRCP_CTRL_STOP_PRESS,
    AVRCP_CTRL_STOP_RELEASE,
    AVRCP_CTRL_FF_PRESS,
    AVRCP_CTRL_FF_RELEASE,
    AVRCP_CTRL_REW_PRESS,
    AVRCP_CTRL_REW_RELEASE
} avrcp_controls;

typedef enum
{
  AVRCP_CONTROL_SEND,
  AVRCP_CREATE_CONNECTION
} avrcp_ctrl_message;

typedef struct
{
    avrcp_controls control;
    uint16 index;
} AVRCP_CONTROL_SEND_T;

typedef struct
{
    bdaddr bd_addr;
} AVRCP_CREATE_CONNECTION_T;

typedef struct
{
    TaskData avrcp_ctrl_handler;
    unsigned active_avrcp:2;
    unsigned avrcp_manual_connect:1;
    unsigned unused:13;
    uint16 extensions[MAX_AVRCP_CONNECTIONS];
    uint16 features[MAX_AVRCP_CONNECTIONS];
    bool connected[MAX_AVRCP_CONNECTIONS];
    AVRCP *avrcp[MAX_AVRCP_CONNECTIONS];
    bool pending_cmd[MAX_AVRCP_CONNECTIONS];
    uint16 cmd_queue_size[MAX_AVRCP_CONNECTIONS];
    bdaddr bd_addr[MAX_AVRCP_CONNECTIONS];
    uint16 registered_events[MAX_AVRCP_CONNECTIONS];
    uint16 play_status[MAX_AVRCP_CONNECTIONS];
    uint16 absolute_volume[MAX_AVRCP_CONNECTIONS];
    bdaddr avrcp_play_addr;
} avrcp_data;


void headsetAvrcpInit(void);

void headsetAvrcpConnect(const bdaddr *bd_addr, uint16 delay_time);

void headsetAvrcpDisconnect(const bdaddr *bd_addr);
        
void headsetAvrcpDisconnectAll(void);

void headsetAvrcpPlay(void);

void headsetAvrcpPause(void);

void headsetAvrcpPlayPause(void);

void headsetAvrcpStop(void);

void headsetAvrcpSkipForward(void);

void headsetAvrcpSkipBackward(void);

void headsetAvrcpFastForwardPress(void);

void headsetAvrcpFastForwardRelease(void);

void headsetAvrcpRewindPress(void);

void headsetAvrcpRewindRelease(void);

void headsetAvrcpVolumeStepChange(volume_direction operation, uint16 step_change);

void headsetAvrcpSetLocalVolume(uint16 Index, uint16 a2dp_volume);

bool headsetAvrcpGetIndexFromInstance(AVRCP *avrcp, uint16 *Index);

bool headsetAvrcpGetIndexFromBdaddr(const bdaddr *bd_addr, uint16 *Index);

void headsetAvrcpSetActiveConnection(const bdaddr *bd_addr);

uint16 headsetAvrcpGetActiveConnection(void);

void headsetAvrcpUdpateActiveConnection(void);

void headsetAvrcpSetPlayStatus(const bdaddr *bd_addr, uint16 play_status);

bool headsetAvrcpCheckManualConnectReset(bdaddr *bd_addr);

void headsetAvrcpHandleMessage(Task task, MessageId id, Message message);


#endif /* _HEADSET_AVRCP_H_ */


#endif /* ENABLE_AVRCP */

