#include <codec.h>
#include <stdlib.h>
#include <stdio.h>
#include <stream.h>
#include <audio_plugin_if.h>
#include <string.h>

/* ATI Include Files */
#include "soundclearPlugin.h"
#include "soundclear.h"
#include "vmAppver.h"

#include "headset_private.h"

/*
#define APPDEBUGSOUNDCLEARPLUGIN
*/
/******************************************************************************
 * Types
 *****************************************************************************/
/* Structure of the SCPLUGINSETSCCFGMSG used internally by
   soundclearPlugin.c */
typedef struct _ScPluginSetScCfgMsgT
{
  uint16 scPskey;
} ScPluginSetScCfgMsgT;

/* Structure of the SCPLUGINSETCODECCFGMSG used internally by
   soundclearPlugin.c */
typedef struct _ScPluginSetCodecCfgMsgT
{
  uint16 codecPskey;
} ScPluginSetCodecCfgMsgT;

/* Structure of the SCPLUGINTOGGLESCACTIVEMSG used internally by'
   soundclearPlugin.c */
typedef struct
{
  uint16 scEnable;
} ScPluginToggleScActiveMsgT;

/******************************************************************************
 * Definitions
 *****************************************************************************/
#define SCPLUGINSETSCCFGMSG          (0x3000)
#define SCPLUGINSETCODECCFGMSG       (0x3001)
#define SCPLUGINTOGGLESCACTIVEMSG    (0x3002)

/******************************************************************************
 * Private Function Prototypes
 *****************************************************************************/
/*the local message handling functions for Audio Plugin*/
static void handleAudioMessage ( Task task , MessageId id, Message message );

/******************************************************************************
 * Local Data
 *****************************************************************************/
/*soundclearPlugin: the Entry point for soundclear Plugin Audio Manager
  framework*/
const TaskData soundclearPlugin = { handleAudioMessage };

/******************************************************************************
 * Macros
 *****************************************************************************/
/******************************************************************************
 *
 * scPluginPrintf()
 *
 * Description:
 *   This macro is a wrapper for printf().  It will output "SCAPP -
 *   soundclearPlugin: " before the pFormt string.
 *
 * Arguments:
 *   msg
 *    [in] - The string to output.
 *
 * Return Value:
 *   None
 *
 *****************************************************************************/
#ifdef APPDEBUGSOUNDCLEARPLUGIN
#define scPluginPrintf(msg) \
{ \
  printf("SCAPP - soundclearPlugin: "); \
  printf msg; \
}
#else
#define scPluginPrintf(msg)
#endif

/******************************************************************************
 *
 * errorPanic()
 *
 * Description:
 *  This macro will print the error message and then call Panic()
 *
 * Arguments:
 *   msg
 *     [in] The error message to print
 *
 * Return Value:
 *   None
 *
 *****************************************************************************/
#define errorPanic(msg) \
{ \
  scPluginPrintf(msg); \
  Panic(); \
};

/******************************************************************************
 * Public Function Definitions
 *****************************************************************************/

/******************************************************************************
 *
 * handleAudioMessage()
 *
 * Description:
 *  The main task message handler for Audio Plugin.  It will receive messages
 *  from the CSR Audio library or from one of the public functions in
 *  soundclearPlugin.h.  In general, the AUDIO_BUSY flag is checked for each
 *  message.  If AUDIO_BUSY is set, then the message will be queued until
 *  AUDIO_BUSY is cleared.  Otherwise, the message will be executed
 *  immediately.  The only exception is AUDIO_PLUGIN_STOP_TONE_MSG.  It is
 *  expected that AUDIO_BUSY is set when a tone is playing.
 *
 * Arguments:
 *   Task task
 *     [in] task for audio plugin.
 *   MessageId id
 *     [in] Audio Message Id,see audio_plugin_interface_message_type_t in CSR
 *          lib
 *   Message message
 *     [in] Audio Message,see audio_plugin_interface_message_type_t in CSR lib
 *
 * Return Value:
 *   None
 *
 *****************************************************************************/
static void handleAudioMessage ( Task task , MessageId id, Message message )
{
  void *    pQueuedMessage = NULL;
  MessageId queuedMsgId    = 0;
  uint16    queuedMsgSize  = 0;
  bool      queueMsg       = FALSE;

  /* AUDIO_BUSY: global flag acts as a semaphore that indicates if the audio
     plugin is busy*/
  if (AUDIO_BUSY)
  {
    queueMsg = TRUE;
  }

  switch (id)
  {
    /* This message is received when the VM application calls AudioConnect().
       It indicates that the VM app wants to starts the SoundClear Plug-In.  */
    case (AUDIO_PLUGIN_CONNECT_MSG ):
    {

		
      /* Print the version of the SoundClear Plug-In */
      scPluginPrintf(("Version %d.%d.%d\n",
                     VMAPPVERNUMBER,
                     VMAPPVERMAJORREV,
                     VMAPPVERMINORREV));
      scPluginPrintf(("AUDIO_PLUGIN_CONNECT_MSG\n"));

      if (!queueMsg)
      {
        scPluginPrintf(("Starting SoundClear Plug-In\n"));
        
        /* Start the SoundClear Plug-In */
#ifdef APPBUILDFOR2010
        soundClearConnect(((AUDIO_PLUGIN_CONNECT_MSG_T *)message)->audio_sink, 
                          ((AUDIO_PLUGIN_CONNECT_MSG_T *)message)->sink_type, 
                          ((AUDIO_PLUGIN_CONNECT_MSG_T *)message)->codec_task, 
                          ((AUDIO_PLUGIN_CONNECT_MSG_T *)message)->volume, 
                          ((AUDIO_PLUGIN_CONNECT_MSG_T *)message)->rate, 
                          ((AUDIO_PLUGIN_CONNECT_MSG_T *)message)->stereo, 
                          ((AUDIO_PLUGIN_CONNECT_MSG_T *)message)->mode, 
                          ((AUDIO_PLUGIN_CONNECT_MSG_T *)message)->route, 
                          ((AUDIO_PLUGIN_CONNECT_MSG_T *)message)->power,
                          ((AUDIO_PLUGIN_CONNECT_MSG_T *)message)->params, 
                          ((AUDIO_PLUGIN_CONNECT_MSG_T *)message)->app_task );
#else
        soundClearConnect(((AUDIO_PLUGIN_CONNECT_MSG_T *)message)->audio_sink,
                          ((AUDIO_PLUGIN_CONNECT_MSG_T *)message)->codec_task,
                          ((AUDIO_PLUGIN_CONNECT_MSG_T *)message)->volume,
                          ((AUDIO_PLUGIN_CONNECT_MSG_T *)message)->stereo,
                          ((AUDIO_PLUGIN_CONNECT_MSG_T *)message)->mode);
#endif
      }
      else
      {
        /* Get space for the message and save its ID and size in case we have
           to queue it */
        pQueuedMessage = PanicUnlessNew(AUDIO_PLUGIN_CONNECT_MSG_T);
        queuedMsgId    = AUDIO_PLUGIN_CONNECT_MSG;
        queuedMsgSize  = sizeof(AUDIO_PLUGIN_CONNECT_MSG_T);
      }
      break;
    }

    /* This message is received when the VM application calls
       AudioDisconnect().  It indicates that the VM app wants to end the
       SoundClear Plug-In */
    case (AUDIO_PLUGIN_DISCONNECT_MSG ):
    {
      scPluginPrintf(("AUDIO_PLUGIN_DISCONNECT_MSG \n"));

      if (!queueMsg)
      {
        /* End the SoundClear Plug-In */
        soundClearDisconnect() ;
      }
      else
      {
        /* Get space for the message and save its ID and size in case we have to
           queue it */
     /* pQueuedMessage = NULL; Use default */
        queuedMsgId    = AUDIO_PLUGIN_DISCONNECT_MSG;
     /* queuedMsgSize  = 0; Use default */
      }
      break;
    }

    /* This message is received when the VM application calls AudioSetMode().
       It indicates that the VM app wants to change the mute mode of the
       SoundClear Plug-In. */
    case (AUDIO_PLUGIN_SET_MODE_MSG ):
    {
      scPluginPrintf(("AUDIO_PLUGIN_SET_MODE_MSG \n"));

      if (!queueMsg)
      {
        /* Change the mute mode of the SoundClear Plug-In */
        soundClearSetMode(((AUDIO_PLUGIN_SET_MODE_MSG_T *)message)->mode);
      }
      else
      {
        /* Get space for the message and save its ID and size in case we have
           to queue it */
        pQueuedMessage = PanicUnlessNew(AUDIO_PLUGIN_SET_MODE_MSG_T);
        queuedMsgId    = AUDIO_PLUGIN_SET_MODE_MSG;
        queuedMsgSize  = sizeof(AUDIO_PLUGIN_SET_MODE_MSG_T);
      }
      break;
    }

    /* This message is received when the VM application calls AudioSetVolume().
       It indicates that the VM app wants to change the volume level of the
       speaker. */
    case (AUDIO_PLUGIN_SET_VOLUME_MSG ):
    {
      scPluginPrintf(("AUDIO_PLUGIN_SET_VOLUME_MSG \n"));

      if (!queueMsg)
      {
        /* Change the volume of the SoundClear Plug-In */
        soundClearSetVolumeLevel(
                     ((AUDIO_PLUGIN_SET_VOLUME_MSG_T *)message)->volume );
      }
      else
      {
        /* Get space for the message and save its ID and size in case we have
           to queue it */
        pQueuedMessage = PanicUnlessNew(AUDIO_PLUGIN_SET_VOLUME_MSG_T);
        queuedMsgId    = AUDIO_PLUGIN_SET_VOLUME_MSG;
        queuedMsgSize  = sizeof(AUDIO_PLUGIN_SET_VOLUME_MSG_T);
      }
      break;
    }

    /* This message is received when the VM application calls AudioPlayTone().
       It indicates that the VM app wishes to play a tone through the receive
       path. */
    case (AUDIO_PLUGIN_PLAY_TONE_MSG ):
    {
      scPluginPrintf(("AUDIO_PLUGIN_PLAY_TONE_MSG \n"));

      if (!queueMsg)
      {
        /* Play the requested tone */
        soundClearPlayTone(
                ((AUDIO_PLUGIN_PLAY_TONE_MSG_T *)message)->tone,
                ((AUDIO_PLUGIN_PLAY_TONE_MSG_T *)message)->tone_volume,
                ((AUDIO_PLUGIN_PLAY_TONE_MSG_T *)message)->stereo);
      }
      else if (((AUDIO_PLUGIN_PLAY_TONE_MSG_T *)message)->can_queue != TRUE)
      {
        /* Only allowing queuing of tone messages if required. */
        queueMsg = FALSE;
      }
      else
      {
        /* Get space for the message and save its ID and size in case we have to
           queue it.  Only do this if we can queue tones. */
        pQueuedMessage = PanicUnlessNew(AUDIO_PLUGIN_PLAY_TONE_MSG_T);
        queuedMsgId    = AUDIO_PLUGIN_PLAY_TONE_MSG;
        queuedMsgSize  = sizeof(AUDIO_PLUGIN_PLAY_TONE_MSG_T);
      }
      break;
    }

    /* This message is received when the VM application calls AudioStopTone().
       It indicates that the VM app wishes to stop a tone that is currenly
       being played */
    case (AUDIO_PLUGIN_STOP_TONE_MSG ):
    {
      scPluginPrintf(("AUDIO_PLUGIN_STOP_TONE_MSG\n"));

      /* This message is never queued since it only has relevance if a tone is
         currently playing */
      queueMsg = FALSE;

      /* End any tone currently being played.  This call will perform no action
         if a tone is not currently playing. */
      soundClearToneEnd();
      break;
    }

    /* This message is received when the VM application calls
       soundClearPluginSetCodecConfig().  It indicates that the VM app wishes
       to change the current codec configuration of the SoundClear Plug-In. */
    case (SCPLUGINSETCODECCFGMSG):
    {
      scPluginPrintf(("SCPLUGINSETCODECCFGMSG \n"));

      if (!queueMsg)
      {
        if (soundClearReadyRcvMsg())
        {
          /* Set the codec configuration for the SoundClear Plug-In */
          soundClearSetCodecConfig(
                  ((ScPluginSetCodecCfgMsgT *)message)->codecPskey);
        }
        else
        {
          errorPanic(("This function must be called after AudioConnect()"));
        }
      }
      else
      {
        /* Get space for the message and save its ID and size in case we have to
           queue it */
        pQueuedMessage = PanicUnlessNew(ScPluginSetCodecCfgMsgT);
        queuedMsgId    = SCPLUGINSETCODECCFGMSG;
        queuedMsgSize  = sizeof(ScPluginSetCodecCfgMsgT);
      }
      break;
    }

    /* This message is received when the VM application calls
       soundClearPluginSetScConfig().  It indicates that the VM app wishes
       to change the current tuning used by the SoundClear DSP Software. */
    case (SCPLUGINSETSCCFGMSG):
    {
      scPluginPrintf(("SCPLUGINSETSCCFGMSG \n"));

      if (!queueMsg)
      {
        if (soundClearReadyRcvMsg())
        {
          /* Set the tuning for the SoundClear DSP Software */
          soundClearSetScConfig(((ScPluginSetScCfgMsgT *)message)->scPskey);
        }
        else
        {
          errorPanic(("This function must be called after AudioConnect()"));
        }
      }
      else
      {
        /* Get space for the message and save its ID and size in case we have to
           queue it */
        pQueuedMessage = PanicUnlessNew(ScPluginSetScCfgMsgT);
        queuedMsgId    = SCPLUGINSETSCCFGMSG;
        queuedMsgSize  = sizeof(ScPluginSetScCfgMsgT);
      }
      break;
    }

    /* This message is received when the VM application calls
       soundClearPluginToggleActiveMsg().  It indicates that the VM app wishes
       to enable/disable the processing performed by the SoundClear DSP
       Software. */
    case SCPLUGINTOGGLESCACTIVEMSG:
    {
      scPluginPrintf(("SCPLUGINTOGGLESCACTIVEMSG \n"));

      if (!queueMsg)
      {
        if (soundClearReadyRcvMsg())
        {
          /* Toggle SoundClear processing */
          soundClearToggleActive(
                    ((ScPluginToggleScActiveMsgT *)message)->scEnable);
        }
        else
        {
          errorPanic(("This function must be called after AudioConnect()"));
        }
      }
      else
      {
        /* Get space for the message and save its ID and size in case we have to
           queue it */
        pQueuedMessage = PanicUnlessNew(ScPluginToggleScActiveMsgT);
        queuedMsgId    = SCPLUGINTOGGLESCACTIVEMSG;
        queuedMsgSize  = sizeof(ScPluginToggleScActiveMsgT);
      }
      break;
    }

    default:
    {
      scPluginPrintf(("Received unknown message\n"));
      /*Make sure we will not get here if we processed all audio messages*/
      /*Panic();*/
      break;
    }
  }

  if (queueMsg)
  {
    scPluginPrintf(("Queue Message\n"));

    /* If there is message data, then copy it into the queued message */
    if (pQueuedMessage != NULL)
    {
      memcpy(pQueuedMessage, message, queuedMsgSize);
    }

    /* Message Will be sent when AUDIO_BUSY is zero */
    MessageSendConditionally ( task,
                               queuedMsgId,
                               pQueuedMessage,
                               (const uint16 *)&AUDIO_BUSY);
  }
}

/******************************************************************************
 *
 * soundClearPluginSetCodecConfig()
 *
 * Description:
 *   Sets the CODEC gains used by the SoundClear Plug-In.  This function should
 *   not be called until after AudioConnect().
 *
 * Arguments:
 *   uint16 codecPskey
 *     [in] The absolute address of the PSKEY that stores the CODEC
 *          configuration for the SoundClear DSP Software.
 *
 * Return Value:
 *   None
 *
 *****************************************************************************/
void soundClearPluginSetCodecConfig(uint16 codecPskey)
{
  /* Send a message to handleAudioMessage so that it can handle the request
     with the Audio Manager Framework messages. */
  ScPluginSetCodecCfgMsgT *pScPluginSetCodecCfgMsg
        = PanicUnlessNew(ScPluginSetCodecCfgMsgT);

  scPluginPrintf(("soundClearPluginSetCodecConfig\n"));

  pScPluginSetCodecCfgMsg->codecPskey = codecPskey;

  /* Message Will be sent when AUDIO_BUSY is zero*/
  MessageSendConditionally((TaskData *)&soundclearPlugin,
                           SCPLUGINSETCODECCFGMSG,
                           pScPluginSetCodecCfgMsg,
                           (const uint16 *)&AUDIO_BUSY);
}

/******************************************************************************
 *
 * soundClearPluginSetScConfig()
 *
 * Description:
 *   Sets the SoundClear DSP Software tuning configuration.  This function
 *   should be called after AudioConnect() and
 *   soundClearPluginSetCodecConfig().
 *
 * Arguments:
 *   uint16 scPskey
 *     [in] The absolute address of the SoundClear DSP Software configuration
 *          PSKEY
 *
 * Return Value:
 *   None
 *
 *****************************************************************************/
void soundClearPluginSetScConfig(uint16 scPskey)
{
  /* Send a message to handleAudioMessage so that it can handle the request
     with the Audio Manager Framework messages. */
  ScPluginSetScCfgMsgT *pScPluginSetScCfgMsg
        = PanicUnlessNew(ScPluginSetScCfgMsgT);

  scPluginPrintf(("soundClearPluginSetScConfig\n"));

  pScPluginSetScCfgMsg->scPskey = scPskey;

  /* Message Will be sent when AUDIO_BUSY is zero*/
  MessageSendConditionally((TaskData *)&soundclearPlugin,
                           SCPLUGINSETSCCFGMSG,
                           pScPluginSetScCfgMsg,
                           (const uint16 *)&AUDIO_BUSY);
}

/******************************************************************************
 *
 * soundClearPluginToggleScActive()
 *
 * Description:
 *   Interface function for SoundClear plugin to enable/disable SoundClear
 *   processing.
 *
 * Arguments:
 *   uint16 scEnable
 *     [in] 1:Enable SoundClear;0:Disable SoundClear.
 *
 * Return Value:
 *   None
 *
 *****************************************************************************/
void soundClearPluginToggleScActive(uint16 scEnable)
{
  /* Send a message to handleAudioMessage so that it can handle the request
     with the Audio Manager Framework messages. */
  ScPluginToggleScActiveMsgT *pScPluginToggleScActiveMsg
        = PanicUnlessNew(ScPluginToggleScActiveMsgT);

  scPluginPrintf(("soundClearPluginToggleScActive\n"));

  pScPluginToggleScActiveMsg->scEnable = scEnable;

  /* Message Will be sent when AUDIO_BUSY is zero */
  MessageSendConditionally((TaskData *)&soundclearPlugin,
                           SCPLUGINTOGGLESCACTIVEMSG,
                           pScPluginToggleScActiveMsg,
                           (const uint16 *)&AUDIO_BUSY);
}
