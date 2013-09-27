#ifndef APPBUILDFOR2010
#include <pcm.h>
#include <codec_.h>
#endif
#include <codec.h>
#include <stdlib.h>
#include <panic.h>
#include <stream.h>
#include <source.h>
#include <stdio.h>
#include <kalimba.h>
#include <file.h>
#include <stream.h>
#include <message.h>
#include <kalimba_standard_messages.h>
#include <pio.h>
#include <ps.h>
#include <string.h>

/* ATI Include Files */
#include "soundclear.h"
#include "soundclearPlugin.h"
#include "soundclearSpp.h"
#include "tuningVmCustom.h"

#include "headset_private.h"


/******************************************************************************
 * Types
 *****************************************************************************/
/* _SoundClearStateT: Tracks state information for the Plug-In */
typedef struct _SoundClearStateT
{
  unsigned               volume              :4;
  unsigned               toneIsPlaying       :1;
  unsigned               rapidConnected      :1;
  AUDIO_MODE_T           mode                :2;
  unsigned               connected           :1;
  /*                                  Total = 9 bits */
} SoundClearStateT;

/* _SoundClearConfigPskeyT: Format of the plug-in config PSKEY */
typedef struct _SoundClearConfigPskeyT
{
  /*** PSKEY Configuration ***/
  unsigned                      :2; /* Not Used */
  unsigned             dualSpkr :1;
  unsigned             dualMic  :1;
  unsigned                      :3; /* Not Used */
  unsigned             bypass   :1;

  /*** Kalimba Supported Features ***/
  /*** DO NOT CHANGE WITHOUT ADJUSTING SOUNDCLEARREADYMSG FROM KALIMBA */
  unsigned                      :4; /* Not Used */
  unsigned             demo     :1;
  unsigned             pskey    :1;
  unsigned             datalog  :1;
  unsigned             rapid    :1;
  /*                     Total = 16 bits */
} SoundClearConfigPskeyT;

typedef union _SoundClearConfigPskeyUnionT
{
  SoundClearConfigPskeyT structConfig;
  uint16                 uintConfig;
} SoundClearConfigPskeyUnionT;

/* _SoundClearCodecPskeyT: Format of the CODEC PSKEY*/
typedef struct _SoundClearCodecPskeyT
{
  unsigned micPreAmpEnable    :1;
  unsigned                    :2; /* Not Used */
  unsigned micGain            :5;
  unsigned                    :3; /* Not Used */
  unsigned spkrGain           :5;
  /*                   Total = 16 bits */
}SoundClearCodecPskeyT;

/* _SoundClearT: Holds object information */
typedef struct _SoundClearT
{
  /* Tracks state of plug-in */
  SoundClearStateT            state;
  /* Plug-in configuration */
  SoundClearConfigPskeyUnionT config;
  /* CODEC parameters */
  SoundClearCodecPskeyT       codecParams;
  /* The SCO sink/source handle */
  Sink                        scoSink;
  /* Task to receive msgs */
  TaskData                    soundclearTaskData;
  /* Task to receive codec msgs */
  Task                        codecTask;
}SoundClearT;

/* Structure for a single entry in the DAC gain table.*/
typedef struct _GainTableEntryT
{
  unsigned digitalGain :8;
  unsigned analogGain  :8;
  /*            Total = 16 bits */
} GainTableEntryT;

/* Define the structure for messages with Kalimba */
typedef struct _KalimbaMessageT
{
    uint16 id;/*ID for Message From Kalimba*/
    uint16 a;
    uint16 b;
    uint16 c;
    uint16 d;
} KalimbaMessageT;

/******************************************************************************
 * Private Function Prototypes
 *****************************************************************************/
static void setInternalCodec(uint16 codecInputGain,
                             uint16 codecOutputGain,
                             bool   micPreAmpEnable);
static void startKalimba(void);
static void connectAudio(void);
static void connectTxPath(void);
static void connectRxPath(void);
static void disconnectTxPath(void);
static void disconnectRxPath(void);
static void handleMessage (Task      task ,
                           MessageId id,
                           Message   message);
static void handleKalimbaMessage(Message message);
static void setCodecConfigNow(uint16 codecPskey);
static void connectRapid(Sink sppSink);
static void disconnectRapid(void);
static void connectDataLog(void);
static void disconnectDataLog(void);
static void sendCodecSettingsToKalimba(void);
static void pathConnect(Source inputSource,
                        Sink   inputSink,
                        Source outputSource,
                        Sink   outputSink);
static void pathDisconnect(Source inputSource,
                           Sink   inputSink,
                           Source outputSource,
                           Sink   outputSink);
static uint16 volume2gain(uint16 volume);
static void setMicCodec(uint16 micGain, bool preAmp);
static void setSpkrCodec(uint16 gain);
static void setAudioBusy(void);
static void clearAudioBusy(void);

/******************************************************************************
 * Defines
 *****************************************************************************/
/***********************************
 * Default/Initial Settings
 **********************************/
/* Default Configuration for the Plug-In */
#define SOUNDCLEARDEFAULTSPKRS      (TUNEDINITNUMSPEAKERS==2)
#define SOUNDCLEARDEFAULTMICS       (TUNEDINITNUMMICS==2)
#define SOUNDCLEARDEFAULTSPP        (FALSE)
#define SOUNDCLEARDEFAULTBYPASS     (FALSE)

/***********************************
 * DSP  PSKEYs
 **********************************/
/* Base address of PSKEY_DSP_0 */
#define DSPKEYBASE   0x2258
/* The key used to store the CODEC configuration */
#define DSPKEYCODEC                  (DSPKEYBASE + 0)
/* The key used to store the SoundClear Volume Table */
#define DSPKEYVOLTABLE               (DSPKEYBASE + 3)
/* The key used to store the SoundClear Library configuration */
#define DSPKEYLIBCONFIG              (DSPKEYBASE + 40)

/***********************************
 * PCM Ports
 **********************************/
#define CODECPCMPORT0                (0)
#define CODECPCMPORT1                (1)

/***********************************
 * Kalimba Ports
 **********************************/
/* Input Ports */
#define MICAKALPORT                  (0)
#define MICBKALPORT                  (1)
#define SCOINKALPORT                 (2)
#define TONEINKALPORT                (3)
#define RAPIDRPCINKALPORT            (4)

/* Output Ports */
#define SCOOUTKALPORT                (0)
#define SPKRAKALPORT                 (1)
#define SPKRBKALPORT                 (2)
#define DATALOGOUTKALPORT            (3)
#define RAPIDRPCOUTKALPORT           (4)

/***********************************
 * Message IDs
 **********************************/
/* This is a mask used to parse the SOUNDCLEARREADYMSG */
#define SOUNDCLEARREADYMSGFEATURESMASK (0x00FF)
/* This is a mask used to parse the MSG ID from SOUNDCLEARREADYMSG */
#define SOUNDCLEARREADYMSGMSGIDMASK    (0xFF00)

/*****************************************************************************
 * Message ID Format:
 *  _____________________________________________________________________
 * | 15 | 14 | 13 | 12 | 11 | 10 | 9 | 8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
 *  ---------------------------------------------------------------------
 * | R  | D  |         ID                |            Features           |
 *  ---------------------------------------------------------------------
 *
 * Where:
 *  R = Reserved - This bit is reserved by the CSR message framework.
 *  D = The direction of the message: 0x0 == Kal --> VM, 0x1 VM --> Kal
 *  ID = The ID of the message
 *  Features = Features supported (Only relevant with SOUNDCLEARREADYMSG)
 *****************************************************************************/
/* Messages IDs:  Kalimba SoundClear --> VM */
#define SOUNDCLEARREADYMSG           (0x0000) /* DSP is Ready */
#define SOUNDCLEARERRORMSG           (0x0100) /* DSP Error */
#define SOUNDCLEARSETCODECSMSG       (0x0200) /* Set CODECs command */
#define SOUNDCLEARSETGAINCFM         (0x0300) /* Set Spkr Gain status */
#define SOUNDCLEARSCCFGCFM           (0x0400) /* Set SC Config status */

/* Message IDs:  VM --> Kalimba SoundClear */
#define SOUNDCLEARSETGAIN            (0x4000) /* Set Spkr Gain command */
#define SOUNDCLEARSTATUSCODECSUPDATE (0x4100) /* Set CODECs status */
#define SOUNDCLEARSETSCCFMMSG        (0x4200) /* Set SC Config command */
#define SOUNDCLEARDEMOTOGGLEACTIVE   (0x4300) /* Toggle Demo command */

/***********************************
 * Other
 **********************************/
#define SCFAILURE                    (-1)
#define SCSUCCESS                    ( 1)

/******************************************************************************
 * Local Data
 *****************************************************************************/

/* The SoundClear Plugin Handler task instance pointer */
static SoundClearT *pSoundClear = NULL;

/* The SoundClear SPP Module task that will receive messages from the plug-in.
   This task is set by the SoundClear SPP Module when it calls
   soundClearRegisterSppTask().  This task is kept in global memory so that
   the SPP module can register even if the SoundClear Plug-In has not been
   started yet.*/
static TaskData *pSppTaskData = NULL;

#ifndef APPBUILDFOR2010
/* DAC Gain Table */
/* This table equates gain settings received from RAPID or located in
   TUNEDCODECSPKROUTGAININDEX into digital and analog gains.
   It has been designed to allow the digital gain to be lower by up to
   6 dB before analog attenuation is introduced.  This allows us to
   avoid a reputed problem with the digital component of the DAC that
   causes wrapping */
static const GainTableEntryT dacGainTable[] =
{
  /*******************************************************************/
  /*  Gain        Digital Gain        Analog Gain          Combined  */
  /* Setting         Setting            Setting              Gain    */
  /*******************************************************************/
  /*    0 */   { 8  /* -24   dB */ , 0 /* -20   dB */ }, /* -44   dB */
  /*    1 */   { 9  /* -20.5 dB */ , 0 /* -20   dB */ }, /* -40.5 dB */
  /*    2 */   { 10 /* -18   dB */ , 0 /* -20   dB */ }, /* -38   dB */
  /*    3 */   { 11 /* -14.5 dB */ , 0 /* -20   dB */ }, /* -34.5 dB */
  /*    4 */   { 12 /* -12   dB */ , 0 /* -20   dB */ }, /* -32   dB */
  /*    5 */   { 13 /* -8.5  dB */ , 0 /* -20   dB */ }, /* -28.5 dB */
  /*    6 */   { 14 /* -6    dB */ , 0 /* -20   dB */ }, /* -26   dB */
  /*    7 */   { 14 /* -6    dB */ , 1 /* -17.5 dB */ }, /* -23.5 dB */
  /*    8 */   { 14 /* -6    dB */ , 2 /* -14   dB */ }, /* -20   dB */
  /*    9 */   { 14 /* -6    dB */ , 3 /* -11.5 dB */ }, /* -17.5 dB */
  /*   10 */   { 14 /* -6    dB */ , 4 /* -8.5  dB */ }, /* -14.5 dB */
  /*   11 */   { 14 /* -6    dB */ , 5 /* -5.5  dB */ }, /* -11.5 dB */
  /*   12 */   { 14 /* -6    dB */ , 6 /* -3    dB */ }, /* -9    dB */
  /*   13 */   { 14 /* -6    dB */ , 7 /*  0    dB */ }, /* -6    dB */
  /*   14 */   { 15 /* -2.5  dB */ , 7 /*  0    dB */ }, /* -2.5  dB */
  /*   15 */   { 0  /*  0    dB */ , 7 /*  0    dB */ }, /*  0    dB */
  /*   16 */   { 1  /*  3.5  dB */ , 7 /*  0    dB */ }, /*  3.5  dB */
  /*   17 */   { 2  /*  6    dB */ , 7 /*  0    dB */ }, /*  6    dB */
  /*   18 */   { 3  /*  9.5  dB */ , 7 /*  0    dB */ }, /*  9.5  dB */
  /*   19 */   { 4  /*  12   dB */ , 7 /*  0    dB */ }, /*  12   dB */
  /*   20 */   { 5  /*  15.5 dB */ , 7 /*  0    dB */ }, /*  15.5 dB */
  /*   21 */   { 6  /*  18   dB */ , 7 /*  0    dB */ }, /*  18   dB */
  /*   22 */   { 7  /*  21.5 dB */ , 7 /*  0    dB */ }  /*  21.5 dB */
};
#endif

/******************************************************************************
 * Macros
 *****************************************************************************/
/******************************************************************************
 *
 * unMuteOrNot()
 *
 * Description:
 *  This Macro will return TRUE if we should unmute and FALSE if we should mute
 *
 * Arguments:
 *  mute
 *    [in] - TRUE if we should un-mute, FALSE if we should mute
 *
 *****************************************************************************/
#define unMuteOrNot(mode) \
  (mode == AUDIO_MODE_CONNECTED)

/******************************************************************************
 *
 * scPrintf()
 *
 * Description:
 *   This macro is a wrapper for printf().  It will output "SCAPP -
 *   soundclear: " before the pFormt string.
 *
 * Arguments:
 *   msg
 *    [in] - The string to output.
 *
 * Return Value:
 *   None
 *
 *****************************************************************************/
#ifdef APPDEBUGSOUNDCLEAR
#define scPrintf(msg) \
{ \
  printf("SCAPP - soundclear: "); \
  printf msg; \
}
#else
#define scPrintf(msg)
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
  scPrintf(msg); \
  Panic(); \
};

/******************************************************************************
 * Public Function Definitions
 *****************************************************************************/

/******************************************************************************
 *
 * soundClearConnect()
 *
 * Description:
 *  This function connects the SoundClear Plugin Handler to the stream
 *  subsystem.  It initializes the SoundClear plugins task instance and
 *  loads and starts the SoundClear library DSP software running on Kalimba.
 *
 * Arguments:
 *   Sink audioSink
 *     [in] Audio Sink for SoundClear plugin to use.
 *   AUDIO_SINK_T sinkType
 *     [in] Whether or not the sink is SCO or eSCO.
 *   Task codecTask
 *     [in] Codec task for SoundClear plugin to use for mic and speaker
 *          codec accesses.
 *   uint16 volume
 *     [in] Volume Level: valid range is from 0 to 15.
 *   uint32 rate
 *     [in] The sample rate to use with the CODECs. Not used.
 *   bool stereo
 *     [in] Flag for specifying stereo output. Not used.
 *   AUDIO_MODE_T mode
 *     [in] The initial mute mode of the plugin.
 *   AUDIO_ROUTE_T route
 *     [in] Where to route the audio.  Not used.
 *   AUDIO_POWER_T power
 *     [in] The power mode.  Not used.
 *   const void pParams
 *     [in] Plugin specific parameters. Not used.
 *   Task *appTask
 *     [in] The application task to send messages to.  Not used.
 *
 * Return Value:
 *   None
 *
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
                        Task          appTask)
#else
void soundClearConnect( Sink         audioSink ,
                        Task         codecTask ,
                        uint16       volume ,
                        bool         stereo ,
                        AUDIO_MODE_T mode  )
#endif
{
  scPrintf(("soundClearConnect \n"));

  /* Only start the plug-in if:
     1) The plug-in is not already running
     2) We have an audio (SCO) sink
     3) We have a codec task */
  if (pSoundClear == NULL &&
      audioSink != NULL &&
      codecTask != NULL)
  {
    /* Set the AUDIO_BUSY flag to keep other operations (i.e. tone playback,
       volume changes, etc).*/
    setAudioBusy();

    /* Allocate memory of the plug-in object */
    pSoundClear = PanicUnlessNew( SoundClearT);

    /* Initialize state variables */
    pSoundClear->state.volume           = volume & 0xf;
    pSoundClear->state.toneIsPlaying    = FALSE;
    pSoundClear->state.rapidConnected   = FALSE;
    pSoundClear->state.mode             = mode;
    pSoundClear->state.connected        = FALSE;

    /* Initialize the CODEC parameters */
    pSoundClear->codecParams.micPreAmpEnable = TUNEDCODECMICINPREAMPEN;
    pSoundClear->codecParams.micGain         = TUNEDCODECMICINGAININDEX;
    pSoundClear->codecParams.spkrGain        = TUNEDCODECSPKROUTGAININDEX;

    /* Initialize the rest of the SoundClear object */
    pSoundClear->scoSink                    = audioSink;
    pSoundClear->soundclearTaskData.handler = handleMessage;
    pSoundClear->codecTask                  = codecTask;

    /* Read the plug-in configuration key */
    if(PsFullRetrieve(DSPKEYLIBCONFIG,
                      &pSoundClear->config,
                      sizeof(pSoundClear->config)))
    {
      scPrintf(("Plug-in configuration success\n"));
    }
    else
    {
      scPrintf(("Configuring Plug-In from defaults\n"));
      pSoundClear->config.structConfig.dualSpkr   = SOUNDCLEARDEFAULTSPKRS;
      pSoundClear->config.structConfig.dualMic    = SOUNDCLEARDEFAULTMICS;
      pSoundClear->config.structConfig.bypass     = SOUNDCLEARDEFAULTBYPASS;
    }
    /* Mask off the Kalimba Features section of the pSoundClear->config to
       avoid un-intentional feature support */
    pSoundClear->config.uintConfig &= SOUNDCLEARREADYMSGMSGIDMASK;

    /* Load and start the kalimba */
    startKalimba();

    /* Route the PCM and connect the Audio Paths */
    connectAudio();

    /* AUDIO_BUSY will be cleared inside
       handleKalimbaMessages() when SOUNDCLEARDSPREADY is received.*/
  }
}

/******************************************************************************
 *
 * soundClearDisconnect()
 *
 * Description:
 *  This function disconnects the Kalimba DSP core from the stream subsystem.
 *  It cancel all pending messages and destroys the SoundClear plugins task
 *  instance.  It then powers off the Kalimba DSP core.
 *
 * Arguments:
 *   None
 *
 * Return Value:
 *   None
 *
 *****************************************************************************/
void soundClearDisconnect( void )
{
  scPrintf(("soundClearDisconnect \n"));

  if (pSoundClear != NULL)
  {
    /* signal that the audio is busy until the kalimba is fully shut down so
       that no other msg like restart Kalimba,tone messages etc will arrive*/
    setAudioBusy();

    /* Zero out the CODECs to mask any clicks and pops*/

	setMicCodec(0,FALSE);
    setSpkrCodec(0);

    disconnectRxPath();
    disconnectTxPath();
    disconnectRapid();
    disconnectDataLog();

    /* Let the SPP module know that we have disconnected */
    if (pSppTaskData != NULL)
    {
      MessageSend(pSppTaskData, SOUNDCLEARDISCONNECTEDMSG, NULL);
    }

    KalimbaPowerOff();

    /*Flush all queued messages */
    MessageFlushTask( (TaskData*)&(pSoundClear->soundclearTaskData));

    free( pSoundClear );
    pSoundClear = NULL;

    scPrintf(("Disconnected\n"));

    clearAudioBusy();

    /* Note that at this point the codecs are zereoed */
  }
}

/******************************************************************************
 *
 * soundClearPlayTone()
 *
 * Description:
 *  This function has the SoundClear plugin play a tone.
 *
 * Arguments:
 *   const ringtone_note *pTone
 *   const audio_note *pTone
 *     [in] Pointer to the audio note for the SoundClear plugin to use for
 *          playing the tone.
 *   uint16 toneVol
 *     [in] Tone volume level for the SoundClear plugin to use.
 *   bool  stereo
 *     [in] Not used.  Stereo audio is handled by a combination of PCM routing
 *          Kalimba processing.
 *
 * Return Value:
 *   None
 *
 *****************************************************************************/
#ifdef APPBUILDFOR2010
void soundClearPlayTone( const ringtone_note *pTone,
                         uint16               toneVol,
                         bool                 stereo )
#else
void soundClearPlayTone( const audio_note *pTone,
                         uint16            toneVol,
                         bool              stereo )
#endif
{
  Sink   toneSink = NULL;

  scPrintf(("soundClearPlayTone \n"));

  /* Only play the tone if we are not currently playing a tone */
  if( pSoundClear != NULL &&
      !pSoundClear->state.toneIsPlaying )
  {
    /*signal that the audio is busy until tone play over,will be cleared by
      soundClearToneEnd()*/
    setAudioBusy();

    scPrintf(("Play Tone\n"));

    /* Set the global flag to TRUE so that the tone volume doesn't overwrite
       the saved volume level.  This will be set to FALSE if the tone
       playback fails. */
    pSoundClear->state.toneIsPlaying = TRUE;

    scPrintf(("Set Tone Volume\n"));
    soundClearSetVolumeLevel( toneVol );

    /* If Kalimba is being bypassed then route the tone straight to the
       speaker CODEC */
    if (pSoundClear->config.structConfig.bypass)
    {
      scPrintf(("Tone Bypass Kalimba\n"));
#ifdef APPBUILDFOR2010
      toneSink = StreamAudioSink(
              AUDIO_HARDWARE_CODEC,
              AUDIO_INSTANCE_0,
              AUDIO_CHANNEL_A);
#else
      toneSink = StreamPcmSink(CODECPCMPORT0);
#endif
    }
    /* Otherwise, route the tone to Kalimba so that it will be mixed with
       SCO in audio */
    else
    {
      scPrintf(("Tone Through Kalimba\n"));
      toneSink = StreamKalimbaSink(TONEINKALPORT);
    }

#ifdef APPBUILDFOR2010
    pathConnect(StreamRingtoneSource(pTone),
                toneSink,
                0,
                0);
#else
    pathConnect(StreamAudioSource((const audio_note *)pTone),
                toneSink,
                0,
                0);
#endif

    /* Register the plug-in task so that we receive a notification when the
       tone has completed playback */
    MessageSinkTask(toneSink,
                    (TaskData*) &(pSoundClear->soundclearTaskData) );
  }
}

/******************************************************************************
 *
 * soundClearToneEnd()
 *
 * Description:
 *  This internal helper function that handles a tone has completed
 *
 * Arguments:
 *   None
 *
 * Return Value:
 *   None
 *
 *****************************************************************************/
void soundClearToneEnd( void )
{
  Sink toneSink;

  scPrintf(("soundClearToneEnd \n"));

  /* Only stop the tone if a tone is currently playing */
  if(pSoundClear != NULL &&
     pSoundClear->state.toneIsPlaying)
  {
    if (pSoundClear->config.structConfig.bypass)
    {
      connectRxPath();
#ifdef APPBUILDFOR2010
      toneSink = StreamAudioSink(
              AUDIO_HARDWARE_CODEC,
              AUDIO_INSTANCE_0,
              AUDIO_CHANNEL_A);
#else
      toneSink = StreamPcmSink(CODECPCMPORT0);
#endif
    }
    else
    {
      /* Disconnect the tone from the Kalimba */
      pathDisconnect(0, StreamKalimbaSink(TONEINKALPORT), 0, 0);
      toneSink = StreamKalimbaSink(TONEINKALPORT);
    }

    /* Un-register the plug-in task so that we don't receive any accidental
       tone end messages */
    MessageSinkTask(toneSink, NULL);

    pSoundClear->state.toneIsPlaying = FALSE;

    /*Resume the volume level for audio*/
    /* Tone volume is seperated from audio volume,so here don't need to set
       back audio volume */
    /*scPrintf(("Restoring Volume\n"));
    soundClearSetVolumeLevel(pSoundClear->state.volume);*/

    clearAudioBusy();

    scPrintf(("ToneComplete\n"));
  }
}

/******************************************************************************
 * soundClearSetMode()
 *
 * Description:
 *  This function set audio mode:
 *  AUDIO_MODE_MUTE_SPEAKER,
 *  AUDIO_MODE_CONNECTED
 *  AUDIO_MODE_MUTE_MIC
 *  AUDIO_MODE_MUTE_BOTH
 *
 * Arguments:
 *   AUDIO_MODE_T mode
 *     [in] audio mode
 *
 * Return Value:
 *   None
******************************************************************************/
void soundClearSetMode ( AUDIO_MODE_T mode )
{
  scPrintf(("soundClearSetMode \n"));

  if (pSoundClear != NULL)
  {
    scPrintf(("Set Mode [%d]\n", mode));

    switch (mode)
    {
      case AUDIO_MODE_CONNECTED:
      case AUDIO_MODE_MUTE_MIC:
      {
        pSoundClear->state.mode = mode;
        soundClearSetVolumeLevel(pSoundClear->state.volume);
        break;
      }

      /*Mode to mute speaker,SoundClear currently not supported*/
      case AUDIO_MODE_MUTE_SPEAKER:
      /*Mode to mute Mic and Speaker,SoundClear currently not supported*/
      case AUDIO_MODE_MUTE_BOTH:
      {
        scPrintf(("Mode not Supported [%x]\n",mode));
        break;
      }

      default:
      {
        scPrintf(("Set Mode Invalid [%x]\n",mode));
        break;
      }
    }
  }
}

/******************************************************************************
 *
 * soundClearSetVolumeLevel()
 *
 * Description:
 *   1.Sets SoundClear volume level if tone is not playing.
 *   2.Tell Kalimba to set the audio gain.
 *   3.Tell Kalimba to set tone gain(by setting the 4th parameter of
 *     KalimbaSendMessage() not zero) if tone is playing.
 *   4.Sends volume level to Kalimba.
 *
 * Arguments:
 *   uint16 volume
 *     [in] - Volume Level: valid range is 0 to 15
 *
 * Return Value:
 *   None.
 *
 *****************************************************************************/
void soundClearSetVolumeLevel( uint16 volume )
{
  /* use toneGain as the 3rd parameter of SOUNDCLEARSETGAIN message to kalimba,
     the value 0xffff(for toneGain) tells kalimba to ignore setting tone gain,
     the valid value for tone gain is from 0 to 255(8bits) */
  uint16 toneGain = 0xFFFF;

  scPrintf(("soundClearSetVolumeLevel \n"));

  if (pSoundClear != NULL)
  {
    volume = volume & 0xf;

    scPrintf(("VOL[%x]\n", volume));

    /* We only want to save the volume if a tone is not being played. This is
       done to avoid overwriting the currently saved volume, which is used
       later to restore the volume when the tone ends. */
    if(!pSoundClear->state.toneIsPlaying)
    {
      scPrintf(("Volume saved\n"));
      pSoundClear->state.volume = volume;
    }
    else
    {
      toneGain = volume2gain(volume);
    }

    scPrintf(("SOUNDCLEARSETGAIN to Kalimba \n"));

    if(!KalimbaSendMessage(SOUNDCLEARSETGAIN,
                           volume2gain(pSoundClear->state.volume),
                           unMuteOrNot(pSoundClear->state.mode),
                           toneGain,
                           pSoundClear->state.volume))
    {
      errorPanic(("KalimbaSendMessage Failed \n"));
    }
  }
}

/******************************************************************************
 *
 * soundClearRegisterSppTask()
 *
 * Description:
 *  This function is used by the SoundClear SPP module to set and clear its
 *  registered task with the plug-in.
 *
 * Arguments:
 *  TaskData *sppTaskDataPtr
 *    [in] The address of the SoundClear SPP task that will receive messages
 *         from the plug-in.
 *
 * Returns:
 *  None
 *
 *****************************************************************************/
void soundClearRegisterSppTask(TaskData *sppTaskDataPtr)
{
  scPrintf(("soundClearRegisterSppTask \n"));

  /* pSoundClear is not checked here since it is expected that this function
	 be called by the SoundClear SPP Module before soundclearConnect() has
	 executed */

  pSppTaskData = sppTaskDataPtr;
}

/******************************************************************************
 *
 * soundClearSetCodecConfig()
 *
 * Description:
 *  Reads the CODEC PSKEY and sets the CODECs accordingly.  Notifies Kalimba
 *  of the CODEC changes.
 *
 * Arguments:
 *   uint16 codecPskey
 *     [in] The address of the PSKEY that stores the CODEC configuration for the
 *          SoundClear DSP Software.
 *
 * Return Value:
 *   None
 *
 *****************************************************************************/
void soundClearSetCodecConfig(uint16 codecPskey)
{
  scPrintf(("soundClearSetCodecConfig\n"));

  if (pSoundClear != NULL)
  {
    /* AUDIO_BUSY is not set here since this function is synchronous and can
       not be interrupted by any other VM code */

    /* Set the CODECs using the configuration located in the PSKEY */
    setCodecConfigNow(codecPskey);

    /* Send the CODEC settings to Kalimba so they can be relayed to RAPID */
    sendCodecSettingsToKalimba();
  }
}

/******************************************************************************
 *
 * soundClearSetScConfig()
 *
 * Description:
 *   Sets the SoundClear DSP Software tuning configuration.  This function
 *   should be called after AudioConnect() and
 *   soundClearPluginSetCodecConfig().
 *
 * Arguments:
 *   uint16 scPskey
 *     [in] The address of the SoundClear DSP Software configuration PSKEY
 *
 * Return Value:
 *   None
 *
 *****************************************************************************/
void soundClearSetScConfig(uint16 scPskey)
{
  scPrintf(("soundClearSetScConfig\n"));

  if (pSoundClear != NULL &&
      pSoundClear->config.structConfig.pskey)
  {
    /* Set the AUDIO_BUSY flag so that other operatons (i.e. tone playback,
       volume changes, etc) will be queued.  This is cleared in
       handleKalimbaMessages() when SOUNDCLEARSCCFGCFM is received from
       the SoundClear DSP Software. */
    setAudioBusy();

    scPrintf(("PSKEY[%x] Config sent to Kalimba\n", scPskey));

    /* Send the PSKEY config message to Kalimba */
    if (!KalimbaSendMessage(SOUNDCLEARSETSCCFMMSG,
                            scPskey,
                            0,
                            0,
                            0))
    {
      errorPanic(("KalimbaSendMessage Failed \n"));
    }
  }
  else
  {
    errorPanic(("PSKEY Tuning not supported in Kalimba\n"));
  }
}

/******************************************************************************
 *
 * soundClearToggleActive()
 *
 * Description:
 *   Send command to enable/disable SoundClear.
 *
 * Arguments:
 *   uint16 scEnable
 *     [in] 1:Enable SoundClear; 0:Disable SoundClear.
 *
 * Return Value:
 *   None
 *
 *****************************************************************************/
void soundClearToggleActive(uint16 scEnable)
{
  scPrintf(("soundClearToggleActive: %s SoundClear\n",
           scEnable ? "Enable":"Disable"));
  if (pSoundClear != NULL &&
      pSoundClear->config.structConfig.demo)
  {
    /* Send the PSKEY config message to Kalimba */
    if (!KalimbaSendMessage(SOUNDCLEARDEMOTOGGLEACTIVE,
                            scEnable,
                            0,
                            0,
                            0))
    {
      errorPanic(("KalimbaSendMessage Failed \n"));
    }
  }
  else
  {
    errorPanic(("Demo Mode not Enabled in Kalimba\n"));
  }
}

/******************************************************************************
 *
 * soundClearReadyRcvMsg()
 *
 * Description:
 *  This function will check pSoundClear->state,connected.  This bit is set
 *  on receipt of the SOUNDCLEARREADYMSG, which means that Kalimba is prepared
 *  to receive messages from the VM.
 *
 * Arguments:
 *   None
 *
 *****************************************************************************/
bool soundClearReadyRcvMsg(void)
{
  scPrintf(("soundClearReadyRcvMsg \n"));

  return (pSoundClear != NULL &&
          pSoundClear->state.connected);
}

/******************************************************************************
 * Private Function Definitions
 *****************************************************************************/
/******************************************************************************
 *
 * setInternalCodec()
 *
 * Description: This function set CSR chip internal codec gain on both channel.
 *
 * Arguments:
 *   uint16 codecInputGain
 *     [in] codec input gain for both channel.
 *   uint16 codecOutputGain
 *     [in] codec output gain for both channel.
 *   bool micPreAmpEnable
 *     [in] enable microphone pre-amp,result an additional 20dB of input gain.
 *
 * Return Value:
 *   None.
 *
 *****************************************************************************/
static void setInternalCodec(uint16 codecInputGain,
                             uint16 codecOutputGain,
                             bool   micPreAmpEnable)
{
  scPrintf(("setInternalCodec \n"));

  setMicCodec(codecInputGain, micPreAmpEnable);
  setSpkrCodec(codecOutputGain);
}

/******************************************************************************
 *
 * startKalimba()
 *
 * Description:
 *  This function is used to start the DSP code.  The code for the DSP resides
 *  in the constant data area as a file in a file system.  Once the file is
 *  located, the file is loaded into the DSP and started.
 *
 * Arguments:
 *   None
 *
 * Return Value:
 *   None
 *
 *****************************************************************************/
static void startKalimba( void )
{
  static const char kapFileName[] = SOUNDCLEARPRODKAPNAME;

  FILE_INDEX  index;

  scPrintf(("startKalimba \n"));

  /* Find the SoundClear DSP PlugIn file in the file system,
   * do not pass terminator*/
  index = FileFind(FILE_ROOT, kapFileName, strlen(kapFileName));

  scPrintf(("Loading %s\n", kapFileName));

  if(index == FILE_NONE)
  {
    /* Error - can't find file        */
    errorPanic(("File not found \n"));
  }

  MessageCancelAll(
          (TaskData*) &(pSoundClear->soundclearTaskData),
          MESSAGE_FROM_KALIMBA);

  /* Specify Kalimba message handler */
  MessageKalimbaTask((TaskData*) &(pSoundClear->soundclearTaskData));

  /* Load the SoundClear algorithm into Kalimba*/
  if(!KalimbaLoad(index))
  {
    errorPanic(("Kalimba load fail \n"));
  }

  /* Now the kap file has been loaded, wait for the SOUNDCLEARREADYMSG
   * message from the dsp to be sent to the message_handler function. */
  scPrintf(("Kalimba loaded. Waiting for ready\n"));
}

/******************************************************************************
 *
 * connectTxPath()
 *
 * Description:
 *  This function connects the transmit path only.
 *
 * Arguments:
 *   None
 *
 * Return Value:
 *   None
 *
 *****************************************************************************/
static void connectTxPath( void )
{
  Source mic1Source = NULL;
  Source mic2Source = NULL;
  scPrintf(("connectTxPath\n"));

  /* Verify that the SCO Sink is still valid.  It is possible that the SCO
     sink might be invalid by the time we get here if the a call was started
     and then quickly ended. We'll let the app call AduioDisconnect() to
     cleanup. */
  if (!SinkIsValid(pSoundClear->scoSink))
  {
	scPrintf(("SCO Sink Invalid\n"));
	return;
  }

#ifdef APPBUILDFOR2010
    mic1Source = StreamAudioSource(
            AUDIO_HARDWARE_CODEC,
            AUDIO_INSTANCE_0,
            AUDIO_CHANNEL_A);

    PanicFalse(SourceConfigure(mic1Source, STREAM_CODEC_INPUT_RATE, 8000));

	if (pSoundClear->config.structConfig.dualMic)
    {
	  mic2Source = StreamAudioSource(
          AUDIO_HARDWARE_CODEC,
          AUDIO_INSTANCE_0,
          AUDIO_CHANNEL_B);
      PanicFalse(SourceConfigure(mic2Source, STREAM_CODEC_INPUT_RATE, 8000));
      PanicFalse(SourceSynchronise(mic1Source, mic2Source));
     }
#else
    mic1Source = StreamPcmSource(CODECPCMPORT0);
    mic2Source = StreamPcmSource(CODECPCMPORT1);
#endif

  /* Check to see if we are bypassing Kalimba */
  if (pSoundClear->config.structConfig.bypass == TRUE)
  {
    scPrintf(("TX Bypass Kalimba\n"));

    /* Connect the Mic A straight to the SCO output */
    pathConnect(mic1Source,
                pSoundClear->scoSink,
                0,
                0);
  }
  else
  {
    /* Connect Mic A to Kalimba and Kalimba to SCO Output */
    pathConnect(mic1Source,
                StreamKalimbaSink(MICAKALPORT),
                StreamKalimbaSource(SCOOUTKALPORT),
                pSoundClear->scoSink);

    /* If this platform must support dual mics, then the 2nd mic input to
       Kalimba must be connected */
    if (pSoundClear->config.structConfig.dualMic)
    {
      pathConnect(mic2Source,
                  StreamKalimbaSink(MICBKALPORT),
                  0,
                  0);
    }
  }
}

/******************************************************************************
 *
 * connectRxPath()
 *
 * Description:
 *  This function connects the receive path only.
 *
 * Arguments:
 *   None
 *
 * Return Value:
 *   None
 *
 *****************************************************************************/
static void connectRxPath( void )
{
  Sink spkrASink = NULL;
  Sink spkrBSink = NULL;

  scPrintf(("connectRxPath\n"));

  /* Verify that the SCO Sink is still valid.  It is possible that the SCO
     sink might be invalid by the time we get here if the a call was started
     and then quickly ended. We'll let the app call AduioDisconnect() to
     cleanup. */
  if (!SinkIsValid(pSoundClear->scoSink))
  {
	scPrintf(("SCO Sink Invalid\n"));
	return;
  }

#ifdef APPBUILDFOR2010
    /* If we are single mic dual speaker, than route both channel A & B */
    if (pSoundClear->config.structConfig.dualSpkr &&
        !pSoundClear->config.structConfig.dualMic)
    {
        spkrASink = StreamAudioSink(
              AUDIO_HARDWARE_CODEC,
              AUDIO_INSTANCE_0,
              AUDIO_CHANNEL_A_AND_B);
    }
    /* Otherwise, just route channel A. If we are both dual mic & dual speaker
       then the channel B speaker output will be connected below. */
    else
    {
        /* spkrASink = StreamAudioSink(
              AUDIO_HARDWARE_CODEC,
              AUDIO_INSTANCE_0,
              AUDIO_CHANNEL_A);*//*dn code*/
        spkrASink = StreamAudioSink(
              AUDIO_HARDWARE_CODEC,
              AUDIO_INSTANCE_0,
              AUDIO_CHANNEL_A_AND_B);

        spkrBSink = StreamAudioSink(
              AUDIO_HARDWARE_CODEC,
              AUDIO_INSTANCE_0,
              AUDIO_CHANNEL_B);
    }

    PanicFalse(SinkConfigure(spkrASink, STREAM_CODEC_OUTPUT_RATE, 8000));

#else
    spkrASink = StreamPcmSink(CODECPCMPORT0);
    spkrBSink = StreamPcmSink(CODECPCMPORT1);
#endif

  /* Check to see if we are bypassing Kalimba */
  if (pSoundClear->config.structConfig.bypass == TRUE)
  {
    scPrintf(("RX Bypass Kalimba\n"));
    /* Connect the SCO Input straight to Speaker A */
    pathConnect(StreamSourceFromSink(pSoundClear->scoSink),
                spkrASink,
                0,
                0);
  }
  else
  {
    /* Connect SCO Input to Kalimba  and Kalimba to Speaker A*/
    pathConnect(StreamSourceFromSink(pSoundClear->scoSink),
                StreamKalimbaSink(SCOINKALPORT),
                StreamKalimbaSource(SPKRAKALPORT),
                spkrASink);

    /* If this platform must support dual speakers and dual mics, then
       the 2nd speaker output from Kalimba must be connected. */
    if (pSoundClear->config.structConfig.dualSpkr &&
        pSoundClear->config.structConfig.dualMic)
    {
#ifdef APPBUILDFOR2010
      PanicFalse(SinkConfigure(spkrBSink, STREAM_CODEC_OUTPUT_RATE, 8000));
      PanicFalse(SinkSynchronise(spkrASink, spkrBSink));
#endif

      pathConnect(StreamKalimbaSource(SPKRBKALPORT),
                  spkrBSink,
                  0,
                  0);
    }
  }
}

/******************************************************************************
 *
 * disconnectTxPath()
 *
 * Description:
 *  This function disconnects the transmit path only
 *
 * Arguments:
 *   None
 *
 * Return Value:
 *   None
 *
 *****************************************************************************/
static void disconnectTxPath( void )
{
  Source mic1Source = NULL;
  Source mic2Source = NULL;

  scPrintf(("disconnectTxPath\n"));

#ifdef APPBUILDFOR2010
    mic1Source = StreamAudioSource(
            AUDIO_HARDWARE_CODEC,
            AUDIO_INSTANCE_0,
            AUDIO_CHANNEL_A);
#else
    mic1Source = StreamPcmSource(CODECPCMPORT0);
    mic2Source = StreamPcmSource(CODECPCMPORT1);
#endif

  /* Disconnect Mic A to Kalimba and Kalimba to SCO Output*/
  pathDisconnect(mic1Source,
                 StreamKalimbaSink(MICAKALPORT),
                 StreamKalimbaSource(SCOOUTKALPORT),
                 pSoundClear->scoSink);

#ifdef APPBUILDFOR2010
  PanicFalse(SourceClose(mic1Source));
#endif
  if (pSoundClear->config.structConfig.dualMic)
  {
#ifdef APPBUILDFOR2010
    mic2Source = StreamAudioSource(
            AUDIO_HARDWARE_CODEC,
            AUDIO_INSTANCE_0,
            AUDIO_CHANNEL_B);
#endif

    /* Disconnect Mic B -> Kalimba */
    pathDisconnect(mic2Source,
                   StreamKalimbaSink(MICBKALPORT),
                   0,
                   0);
#ifdef APPBUILDFOR2010
    PanicFalse(SourceClose(mic2Source));
#endif
  }
}


/******************************************************************************
 *
 * disconnectRxPath()
 *
 * Description:
 *  This function disconnects the receive path only
 *
 * Arguments:
 *   None
 *
 * Return Value:
 *   None
 *
 *****************************************************************************/
static void disconnectRxPath( void )
{
  Sink spkrASink = NULL;
  Sink spkrBSink = NULL;

  scPrintf(("disconnectRxPath\n"));

#ifdef APPBUILDFOR2010
    /* If we are single mic dual speaker, than route both channel A & B */
    if (pSoundClear->config.structConfig.dualSpkr &&
        !pSoundClear->config.structConfig.dualMic)
    {
        spkrASink = StreamAudioSink(
              AUDIO_HARDWARE_CODEC,
              AUDIO_INSTANCE_0,
              AUDIO_CHANNEL_A_AND_B);
    }
    /* Otherwise, just route channel A. If we are both dual mic & dual speaker
      then the channel B speaker output will be connected below. */
    else
    {
       /* spkrASink = StreamAudioSink(
              AUDIO_HARDWARE_CODEC,
              AUDIO_INSTANCE_0,
              AUDIO_CHANNEL_A);*//*dn code*/
        spkrASink = StreamAudioSink(
              AUDIO_HARDWARE_CODEC,
              AUDIO_INSTANCE_0,
              AUDIO_CHANNEL_A_AND_B);

        spkrBSink = StreamAudioSink(
              AUDIO_HARDWARE_CODEC,
              AUDIO_INSTANCE_0,
              AUDIO_CHANNEL_B);
    }

#else
    spkrASink = StreamPcmSink(CODECPCMPORT0);
    spkrBSink = StreamPcmSink(CODECPCMPORT1);
#endif

  /* Disconnect SCO Input from Kalimba and Kalimba from Speaker A */
  pathDisconnect(StreamSourceFromSink(pSoundClear->scoSink),
                 StreamKalimbaSink(SCOINKALPORT),
                 StreamKalimbaSource(SPKRAKALPORT),
                 spkrASink);

  PanicFalse(SinkClose(spkrASink));

  if (pSoundClear->config.structConfig.dualSpkr &&
        pSoundClear->config.structConfig.dualMic)
  {
    /* Disconnect Kalimba from Speaker B */
    pathDisconnect(StreamKalimbaSource(SPKRBKALPORT),
                   spkrBSink,
                   0,
                   0);

    PanicFalse(SinkClose(spkrBSink));
  }
}


/******************************************************************************
 *
 * connectAudio()
 *
 * Description:
 *  This function routes the PCM to the CODECs and then calls the path connect
 *  functions that will make the connections between the PCM ports, SCO, and
 *  Kalimba.
 *
 * Arguments:
 *   None
 *
 * Return Value:
 *   None
 *
 *****************************************************************************/
static void connectAudio(void)
{
#ifndef APPBUILDFOR2010
  /* Associate PCM Port 0 with Mic A and Speaker A by default.  This is the
     single microphone case. */
  vm_pcm_io pcmPort0Routing = VM_PCM_INTERNAL_A;
#endif

  scPrintf(("connectAudio:\n"));

#ifndef APPBUILDFOR2010
  /* Determine if PCM port 0 should be routed to just channel A or both
     channels.  This associates PCM port 0 with either Mic A and Speaker A
     or Mic A and both speakers.*/
  if ((!pSoundClear->config.structConfig.dualMic &&
        pSoundClear->config.structConfig.dualSpkr) ||
      (pSoundClear->config.structConfig.bypass))
  {
    pcmPort0Routing = VM_PCM_INTERNAL_A_AND_B;
  }

  PcmClearAllRouting();

  /* Route PCM port 0 to the appropriate channel */
  PcmRateAndRoute( CODECPCMPORT0,
                   PCM_NO_SYNC,
                   TUNEDINITBWFS,
                   TUNEDINITBWFS,
                   pcmPort0Routing);

  /* If necessary, route PCM port 1 to channel B.  This associates both Mic B
     and Speaker B to PCM Port 1.  This is only done if we are supporting two
     mics and bypass is not set. We don't care how many speaker there are
     since we have to use both channels to support dual mics anyway.*/
  if (pSoundClear->config.structConfig.dualMic &&
      !pSoundClear->config.structConfig.bypass)
  {
    PcmRateAndRoute( CODECPCMPORT1,
                     CODECPCMPORT0, /* Synchronize PCM Port 0 to Port 1 */
                     TUNEDINITBWFS,
                     TUNEDINITBWFS,
                     VM_PCM_INTERNAL_B);
  }
#endif

  /* Zero out the CODECs to mask out any clicks and pops that
     might arise from the stream connects */
  setMicCodec(0,FALSE);
  setSpkrCodec(0);

  /* Connect the receive and transmit paths */
  connectRxPath();
  connectTxPath();

}

/****************************************************************************
 *
 * handleMessage()
 *
 * Description:
 *  This handles all message received by the plug-in.
 *
 * Arguments:
 *   Task task
 *     [in] task for Internal Message handler.
 *   MessageId id
 *     [in] Internal Message Id,Currently only message(MESSAGE_FROM_KALIMBA)
 *          is processed.
 *   Message message
 *     [in] Internal Message,Currently only one message(MESSAGE_FROM_KALIMBA)
 *          is processed.
 *
 * Return Value:
 *   None
******************************************************************************/
static void handleMessage(Task task , MessageId id, Message message)
{
  scPrintf(("handleMessage \n"));

  switch (id)
  {
    /* We've received a message from Kalimba, so handle it. */
    case MESSAGE_FROM_KALIMBA:
    {
      scPrintf(("Message from Kalimba\n"));
      handleKalimbaMessage(message);
      break;
    }

    /* SPP has been connected, so connect the SPP port to the RAPID Kalimba
       command port. */
    case SOUNDCLEARSPPCONNECTCFM:
    {
      scPrintf(("SPP Connect Confirmation\n"));
      if (pSoundClear->config.structConfig.rapid)
      {
        connectRapid(((SoundClearSppConnectCfmT *)message)->sppSink);
      }
      break;
    }

    /* SPP has been disconnected so disconnect the RAPID command port. */
    case SOUNDCLEARSPPDISCONNECTIND:
    {
      scPrintf(("SPP Disconnect Indication\n"));
      /* Connect RAPID via UART */
      disconnectRapid();
      break;
    }

    /* This message indicates that the tone has completed playing */
    case MESSAGE_STREAM_DISCONNECT:
    {
      scPrintf(("Tone Completed\n"));
      soundClearToneEnd();
      break;
    }

    default:
      scPrintf(("Unknown message from system [0x%4x] \n", id));
      break;

  }
}

/****************************************************************************
 *
 * handleKalimbaMessage()
 *
 * Description:
 *  Kalimba messages are handled here
 *
 * Arguments:
 *   Message message
 *     [in] The Kalimba message that needs to be handled.
 *
 * Return Value:
 *   None
******************************************************************************/
static void handleKalimbaMessage(Message message)
{
  const KalimbaMessageT *m = (const KalimbaMessageT *)message;
  SoundClearConnectMsgT *pSoundClearConnectMsg = NULL;

  switch ( m->id & SOUNDCLEARREADYMSGMSGIDMASK)
  {
    /* Our Kalimba image has indicated that it is ready to process audio.
       Finalize our connection process by setting the intial volume, setting
       the CODECs and connecting datalogging and RAPID if necessary. */
    case (SOUNDCLEARREADYMSG):
    {
      scPrintf(("Kalimba is Ready \n"));

      /* Set the configuration of the plug-in based on what features are
         supported by Kalimba */
      pSoundClear->config.uintConfig |= m->id & SOUNDCLEARREADYMSGFEATURESMASK;

      scPrintf(("Kalimba Supported Features [0x%4x]\n",
                pSoundClear->config.uintConfig));

      /* Set the initial volume */
      soundClearSetVolumeLevel(pSoundClear->state.volume);

      /* Set the CODECs */
      setCodecConfigNow(DSPKEYCODEC);

      /* Connect Data Logging if needed */
      if (pSoundClear->config.structConfig.datalog)
      {
        connectDataLog();
      }

      /* Connect RAPID if needed */
      if (pSoundClear->config.structConfig.rapid)
      {
        /* If we are using SPP for RAPID and the SPP module has registered then
           tell the SoundClear SPP Module that the plug-in is up and running */
        if (pSppTaskData != NULL)
        {
          pSoundClearConnectMsg
                  = (SoundClearConnectMsgT *)PanicUnlessNew(
                          SoundClearConnectMsgT);

          pSoundClearConnectMsg->pSoundClearTaskData
                  = &pSoundClear->soundclearTaskData;

          MessageSend(pSppTaskData,
                      SOUNDCLEARCONNECTEDMSG,
                      pSoundClearConnectMsg);
        }
        /* Otherwise, connect RAPID via UART as long as we are not data
           logging */
        else if (!pSoundClear->config.structConfig.datalog)
        {
          /* Connect RAPID via UART */
          connectRapid(0);
        }
      }

      /* Indicate that the plug-In is connected */
      pSoundClear->state.connected = TRUE;

      /* soundClearConnect() originally set AUDIO_BUSY.  We can now clear it
         since the connection process has been completed. */
      clearAudioBusy();
      break;
    }

    /* RAPID via Kalimba has indicated a CODEC gain change.  Set the CODECs
       accordingly. This message will only be received if RAPID is
       supported in Kalimba. */
    case SOUNDCLEARSETCODECSMSG:
    {
      scPrintf(("Set CODECs from RAPID \n"));
      setInternalCodec(m->a,m->b,m->c);
      break;
    }

    /* Kalimba has indicated that it has completed the set gain command. */
    case SOUNDCLEARSETGAINCFM:
    {
      scPrintf(("Set Gain Confirmation: \n status = 0x%x\n",m->a));
      break;
    }

    /* Kalimba has indicated that it completed PSKEY Tuning.  If there is an
       error, than print a message and panic.  Otherwise, clear the AUDIO_BUSY
       flag. This mesage will only be received if PSKEY tuning is support in
       Kalimba. */
    case SOUNDCLEARSCCFGCFM:
    {
      if (m->a != SCSUCCESS)
      {
        errorPanic(("Set SC Config failed\n"));
      }
      scPrintf(("Set SC Config success\n"));
      /* Clear the Audio Busy flag set by soundClearSetScConfig() function */
      clearAudioBusy();
      break;
    }

    /* Kalimba has indicated some error message. Print and error message
       and panic. */
    case SOUNDCLEARERRORMSG:
    {
      errorPanic(("Kalimba error message [%d]\n",m->a));
      break;
    }

    default:
    {
      errorPanic(("Unknown Message [0x%4x]\n", m->id));
      break;
    }  /* endof m->id  switch */
  }
}

/******************************************************************************
 *
 * connectRapid()
 *
 * Description:
 *  This function configures UART for Rapid RPC communications between a Host
 *  PC and Kalimba.
 *
 * Arguments:
 *   Sink sppSink
 *     [in] The SPP Sink.  This must be non-zero if connecting RAPID to SPP.
 *
 * Return Value:
 *   None
 *
 *****************************************************************************/
static void connectRapid(Sink sppSink)
{
  Sink   serialSink   = StreamUartSink();
  Source serialSource = StreamUartSource();
  Sink   kalSink      = StreamKalimbaSink(RAPIDRPCINKALPORT);
  Source kalSource    = StreamKalimbaSource(RAPIDRPCOUTKALPORT);

  scPrintf(("connectRapid \n"));

  /* Connect RAPID via SPP*/
  if (pSppTaskData != NULL)
  {
    scPrintf(("Connecting RAPID via SPP\n"));

    pathConnect(StreamSourceFromSink(sppSink),
                kalSink,
                kalSource,
                sppSink);
  }
  /* Otherwise, connect RAPID via UART */
  else
  {
    scPrintf(("Connecting RAPID via UART\n"));

    /* Make sure that we have a valid UART sink.  If we don't it is most likely
       because PSKEY_HOST_INTERFACE is not set to "VM Access to UART" (0x4).
       This key is also set using the "Transport" option in the VM project
       properties (set it to RAW).*/
    if (serialSink == NULL)
    {
      scPrintf(("No UART Interface. Check PSKEY_HOST_INTERFACE\n"));
    }
    else
    {
      /* Dynamically configure the UART settings.
       *   void StreamUartConfigure(vm_uart_rate rate,
       *                            vm_uart_stop stop,
       *                            vm_uart_parity parity); */
      /* Configure the UART:  115200 baud, 1 stop bit, and No Parity */
      StreamUartConfigure(VM_UART_RATE_115K2,
                          VM_UART_STOP_ONE,
                          VM_UART_PARITY_NONE);

      pathConnect(serialSource,
                  kalSink,
                  kalSource,
                  serialSink);
    }
  }

  pSoundClear->state.rapidConnected = TRUE;

  sendCodecSettingsToKalimba();
}

/******************************************************************************
 *
 * disconnectRapid()
 *
 * Description:
 *  This function will disconnect RAPID.
 *
 * Arguments:
 *   None
 *
 * Return Value:
 *   None
 *
 *****************************************************************************/
static void disconnectRapid()
{
  Sink   kalSink      = StreamKalimbaSink(RAPIDRPCINKALPORT);
  Source kalSource    = StreamKalimbaSource(RAPIDRPCOUTKALPORT);

  scPrintf(("disconnectRapid \n"));

  /* Disconnect RAPID if it is connected */
  if (pSoundClear->state.rapidConnected)
  {
    pathDisconnect(0,
                   kalSink,
                   kalSource,
                   0);
  }

  pSoundClear->state.rapidConnected = FALSE;
}

/******************************************************************************
 *
 * connectDataLog()
 *
 * Description:
 *   Configures UART, then connects Kalimba data logging port to UART sink.
 *
 * Arguments:
 *   None.
 *
 * Return Value:
 *   None.
 *
 *****************************************************************************/
static void connectDataLog(void)
{
  Sink   serialSink   = StreamUartSink();

  scPrintf(("connectDataLog \n"));

  /* Make sure that we have a valid UART sink.  If we don't it is most likely
     because PSKEY_HOST_INTERFACE is not set to "VM Access to UART" (0x4).
     This key is also set using the "Transport" option in the VM project
     properties (set it to RAW).*/
  if (serialSink == NULL)
  {
    scPrintf(("No UART Interface. Check PSKEY_HOST_INTERFACE\n"));
  }
  else
  {
    /* Configure UART (For setting see:
     * BlueLab\tools\include\app\uart\uart_if.h)  */
    StreamUartConfigure(VM_UART_RATE_460K8,
                        VM_UART_STOP_ONE,
                        VM_UART_PARITY_EVEN);

    /* Connect Kalimba data logging port to UART sink. */
    pathConnect(StreamKalimbaSource(DATALOGOUTKALPORT),
                serialSink,
                0,
                0);
  }
}

/******************************************************************************
 *
 * disconnectDataLog()
 *
 * Description:
 *   Configures UART, then connects Kalimba data logging port to UART sink.
 *
 * Arguments:
 *   None.
 *
 * Return Value:
 *   None.
 *
 *****************************************************************************/
static void disconnectDataLog(void)
{
  scPrintf(("disconnectDataLog \n"));

  if (pSoundClear->config.structConfig.datalog)
  {
    /* Connect Kalimba data logging port to UART sink. */
    pathDisconnect(StreamKalimbaSource(DATALOGOUTKALPORT),
                   StreamUartSink(),
                   0,
                   0);
  }
}

/******************************************************************************
 *
 * setCodecConfigNow()
 *
 * Description:
 *  Reads the CODEC PSKEY and sets the CODECs accordingly without benefit of
 *  using AudioBusy semaphore. Uses TunedParameters if no key present.
 *
 * Arguments:
 *   uint16 codecPskey
 *     [in] The address of the PSKEY that stores the CODEC configuration for the
 *          SoundClear DSP Software.
 *
 * Return Value:
 *   None
 *
 *****************************************************************************/
void setCodecConfigNow(uint16 codecPskey)
{
  scPrintf(("setCodecConfigNow\n"));

  if(PsFullRetrieve(codecPskey,
                    &pSoundClear->codecParams,
                    sizeof(pSoundClear->codecParams)))
  {
    scPrintf(("Tune CODECs from PSKEY \n \
              micPreAmpEnable:[%d], \n \
              micGain:[%d], \n \
              spkrGain:[%d]\n",
              pSoundClear->codecParams.micPreAmpEnable,
              pSoundClear->codecParams.micGain,
              pSoundClear->codecParams.spkrGain));

  }
  /* The PSKEY retrieval fails then the init values will be used */

  /* Set the CODECs */
  setInternalCodec(pSoundClear->codecParams.micGain,
                   pSoundClear->codecParams.spkrGain,
                   pSoundClear->codecParams.micPreAmpEnable);

}

/******************************************************************************
 *
 * sendCodecSettingsToKalimba()
 *
 * Description:
 *  This function will send the current CODEC settings to  Kalimba if RAPID is
 *  connected.  This is done to keep RAPID informed of changes to the CODECs
 *  made by the app.
 *
 * Arguments:
 *   None
 *
 * Return Value:
 *   None
 *
 *****************************************************************************/
static void sendCodecSettingsToKalimba()
{
  scPrintf(("sendCodecSettingsToKalimba \n"));

  if (pSoundClear->state.rapidConnected)
  {
    /* Tell Kalimba of the new codec settings */
    if (!KalimbaSendMessage(SOUNDCLEARSTATUSCODECSUPDATE,
                            pSoundClear->codecParams.micGain,
                            pSoundClear->codecParams.spkrGain,
                            pSoundClear->codecParams.micPreAmpEnable,
                            0))
    {
      errorPanic(("KalimbaSendMessage Failed \n"));
    }
  }
}

/******************************************************************************
 *
 * pathConnect()
 *
 * Description:
 *  This function can be used to connect input sources to input sinks (e.g
 *  connecting the SCO input to the Kalimba Line In port) and output sources
 *  to output sinks (e.g. connecting the Kalimba Line Output port to SCO
 *  output).  All sources and sinks will be disconnected from any current
 *  connections.
 *
 * Arguments:
 *   Source inputSource
 *     [in] The address of the input source
 *   Sink   inputSink
 *     [in] The address of the input sink
 *   Source outputSource
 *     [in] The address of the output source
 *   Sink   outputSink
 *     [in] The address of the output sink
 *
 * Return Value:
 *   None
 *
 *****************************************************************************/
static void pathConnect(Source inputSource,
                        Sink   inputSink,
                        Source outputSource,
                        Sink   outputSink)
{
  /*pathDisconnect(inputSource,
                 inputSink,
                 outputSource,
                 outputSink);*/

  scPrintf(("pathConnect [%x] [%x] [%x] [%x] \n",
            (uint16)inputSource,
            (uint16)inputSink,
            (uint16)outputSource,
            (uint16)outputSink));

  if (inputSource != NULL && inputSink != NULL)
  {
    PanicFalse(StreamConnect(inputSource, inputSink));
  }

  if (outputSource != NULL && outputSink != NULL)
  {
    PanicFalse(StreamConnect(outputSource, outputSink));
  }
}

/******************************************************************************
 *
 * pathDisconnect()
 *
 * Description:
 *  This function can be used to disconnect input sources from input sinks and
 *  output sources from output sinks.
 *
 * Arguments:
 *   Source inputSource
 *     [in] The address of the input source
 *   Sink   inputSink
 *     [in] The address of the input sink
 *   Source outputSource
 *     [in] The address of the output source
 *   Sink   outputSink
 *     [in] The address of the output sink
 *
 * Return Value:
 *   None
 *
 *****************************************************************************/
static void pathDisconnect(Source inputSource,
                           Sink   inputSink,
                           Source outputSource,
                           Sink   outputSink)
{
  scPrintf(("pathDisconnect [%x] [%x] [%x] [%x] \n",
            (int)inputSource,
            (int)inputSink,
            (int)outputSource,
            (int)outputSink));

  StreamDisconnect(inputSource, inputSink);
  StreamDisconnect(outputSource, outputSink);
}

/******************************************************************************
 *
 * volume2gain()
 *
 * Description:
 *  This function will retrieve the speaker gain from the volume gain table
 *  PSKEY.  The size of the gain table is 8 words. The volume index is 16 steps
 *  from 0 to 15, so the table is arranged as shown below:
 *
 *  Word[0] high 8 bits - Volume index 0
 *  Word[0] low  8 bits - Volume index 1
 *  Word[1] high 8 bits - Volume index 2
 *  Word[1] low  8 bits - Volume index 3
 *  .               .                  .
 *  .               .                  .
 *  .               .                  .
 *  Word[7] high 8 bits - Volume index 14
 *  Word[7] low  8 bits - Volume index 15
 *
 * Arguments:
 *   uint16 volume
 *     [in]: the Volume ranging from 0 to 15 by HFP spec.
 *
 * Return Value:
 *   the gain[8 bits] ranging between 0(not inclusive) and 0xff(inclusive)
 *****************************************************************************/
static uint16 volume2gain(uint16 volume)
{
  uint16 volGainTab[8];
  uint16 volGain = 0xff;

  scPrintf(("getGainFromVol \n"));

  if(PsFullRetrieve(DSPKEYVOLTABLE,volGainTab,sizeof(volGainTab)))
  {
    scPrintf(("volGainTab:\n \
              [%x],[%x],[%x],[%x],[%x],[%x],[%x],[%x] \n",
              volGainTab[0],
              volGainTab[1],
              volGainTab[2],
              volGainTab[3],
              volGainTab[4],
              volGainTab[5],
              volGainTab[6],
              volGainTab[7]));

    volGain = ((volume%2) ?
                  (volGainTab[volume/2] & 0xff) :
                  ((volGainTab[volume/2] & 0xff00)>>8));

    scPrintf(("gain to be sent: [%x]\n",volGain));

  }
  else
  {
    errorPanic(("PsFullRetrieve Failed \n"));
  }

  return volGain;
}

/******************************************************************************
 *
 * setMicCodec()
 *
 * Description:
 *  This function will set the mic codecs (A & B) and both mic pre-amps
 *
 * Arguments:
 *   uint16 micGain
 *     [in] The gain need to be applied to both ADC channel of internal CODEC
 *          Valid range is 0 - 22.
 *   bool preAmp
 *     [in] TRUE if the pre-amp should be enabled, FALSE otherwise.
 *
 * Returns:
 *   None
 *
 *****************************************************************************/
static void setMicCodec(uint16 micGain, bool preAmp)
{
#ifdef APPBUILDFOR2010
  Source mic1Source = StreamAudioSource(
          AUDIO_HARDWARE_CODEC,
          AUDIO_INSTANCE_0,
          AUDIO_CHANNEL_A);
  SourceConfigure(mic1Source,
                  STREAM_CODEC_MIC_INPUT_GAIN_ENABLE,
                  preAmp);

  if (pSoundClear->config.structConfig.dualMic)
  {
     Source mic2Source = StreamAudioSource(
            AUDIO_HARDWARE_CODEC,
            AUDIO_INSTANCE_0,
            AUDIO_CHANNEL_B);
     SourceConfigure(mic2Source,
                     STREAM_CODEC_MIC_INPUT_GAIN_ENABLE,
                     preAmp);
  }
#else
  CodecEnableMicInputGainA(preAmp);
  CodecEnableMicInputGainB(preAmp);
#endif

  CodecSetInputGainNow(pSoundClear->codecTask, micGain, left_and_right_ch);

}

/******************************************************************************
 *
 * setSpkrCodec()
 *
 * Description:
 *  This function will set set speaker gain to both left and right internal
 *  CODEC channel
 *
 * Arguments:
 *   gain
 *     [in] - The index into the dacGainTable.  Valid range is 0 - 22.
 *
 * Returns:
 *   None
 *
 *****************************************************************************/
static void setSpkrCodec(uint16 gain)
{
#ifdef APPBUILDFOR2010
  CodecSetOutputGainNow(pSoundClear->codecTask, gain, left_and_right_ch);
#else
  CodecSetRawOutputGainA(dacGainTable[gain].analogGain,
                         dacGainTable[gain].digitalGain);
  CodecSetRawOutputGainB(dacGainTable[gain].analogGain,
                         dacGainTable[gain].digitalGain);
#endif
}

/******************************************************************************
 *
 * setAudioBusy()
 *
 * Description:
 *  This function will set the AUDIO_BUSY flag
 *
 * Arguments:
 *   None
 *
 * Return Value:
 *   None
 *
 *****************************************************************************/
static void setAudioBusy()
{
  scPrintf(("setAudioBusy \n"));

#if defined(APPBUILDSCPLUGINFORLEGACY)
  AUDIO_BUSY = TRUE;
#else
  AUDIO_BUSY = (TaskData*)&soundclearPlugin;
#endif
}

/******************************************************************************
 *
 * clearAudioBusy()
 *
 * Description:
 *  This function will clear the AUDIO_BUSY flag
 *
 * Arguments:
 *   None
 *
 * Return Value:
 *   None
 *
 *****************************************************************************/
static void clearAudioBusy()
{
  scPrintf(("clearAudioBusy \n"));

#if defined(APPBUILDSCPLUGINFORLEGACY)
  AUDIO_BUSY = FALSE;
#else
  AUDIO_BUSY = NULL;
#endif
}
