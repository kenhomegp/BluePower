#ifndef _SOUNDCLEAR_H_
#define _SOUNDCLEAR_H_

/* CSR SDK Include Files */
#include <audio_plugin_if.h>
#include <sink.h>

/* ATI Include Files */
#include "tuningVmCustom.h"

/******************************************************************************
 * Types
 *****************************************************************************/
/* Structure of the SOUNDCLEARCONNECTMSG used between the soundclearSpp.c and
   soundclear.c */
typedef struct
{
  TaskData *pSoundClearTaskData;
} SoundClearConnectMsgT;

/* Structure of the SOUNDCLEARSPPCONNECTCFM used between the soundClearspp.c
   and soundclear.c */
typedef struct
{
  Sink sppSink;
} SoundClearSppConnectCfmT;

/******************************************************************************
 * Definitions
 *****************************************************************************/
/***********************************
 * Message IDs
 **********************************/
/* Used to notify SPP that the plug-in is connected */
#define SOUNDCLEARCONNECTEDMSG       (0x3003)
/* Used to notify SPP that the plug-in is disconnected */
#define SOUNDCLEARDISCONNECTEDMSG    (0x3004)
/* Used to notify a client task that SoundClear SPP has connected to a
   remote device. SPP streams should not be connected until this message is
   received. */
#define SOUNDCLEARSPPCONNECTCFM      (0x3005)
/* Used to notify a client task that SoundClear SPP has disconnected from
   a remote device.  The client task should tear down any SPP streams in
   response to this message. */
#define SOUNDCLEARSPPDISCONNECTIND   (0x3006)

/***********************************
 * KAP Filenames
 **********************************/
#define SOUNDCLEARPRODKAPNAME       "rtTgtApp/rtTgtApp.kap"


/******************************************************************************
 * Public Function Prototypes
 *****************************************************************************/
#ifdef APPBUILDFOR2010
void soundClearConnect( Sink          audioSink,
                        AUDIO_SINK_T  sinkType,
                        Task          codecTask,
                        uint16        volume,
                        uint32        rate,
                        bool          stereo,
                        AUDIO_MODE_T  mode,
                        AUDIO_ROUTE_T route,
                        AUDIO_POWER_T power,
                        const void   *pParams,
                        Task          appTask );

void soundClearSetRoute(AUDIO_ROUTE_T route,
                        const void   *pParams);

void soundClearPlayTone( const ringtone_note *pTone,
                         uint16               toneVol,
                         bool                 stereo );

#else
void soundClearConnect( Sink audioSink,
                        Task codecTask,
                        uint16 volume,
                        bool stereo,
                        AUDIO_MODE_T mode );

void soundClearPlayTone( const audio_note *pTone,
                         uint16 toneVol,
                         bool stereo );
#endif

void soundClearDisconnect( void );
void soundClearSetVolumeLevel( uint16 volume );
void soundClearSetMode ( AUDIO_MODE_T mode );
void soundClearToneEnd ( void ) ;

void soundClearSetCodecConfig(uint16 codecPskey);
void soundClearSetScConfig(uint16 scPskey);
void soundClearToggleActive(uint16 scEnable);
void soundClearRegisterSppTask(TaskData *sppTaskDataPtr);
bool soundClearReadyRcvMsg(void);

#endif   /* _SOUNDCLEAR_H_ */

