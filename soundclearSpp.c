#include <stdio.h>
#include <stdlib.h>
#if defined(APPBUILDFOR2010)
#include <stream.h>
#include "spps.h"
#else
#include <spp.h>
#endif

/* ATI Include Files */
#include "soundclearSpp.h"
#include "soundclear.h"

#include "headset_private.h"

/******************************************************************************
 * Types
 *****************************************************************************/
/* The structure of the SoundClear SPP object */
typedef struct _SppData
{
#if !defined(APPBUILDFOR2010)
  SPP        *pSpp;        /* A handle to the SPP profile structure */
#endif
  Sink        sink;        /* A handle to the SPP sink */
} ScSppData;

/******************************************************************************
 * Private Function Prototypes
 ******************************************************************************/
static void initDataPtr(void);
static void sppHandleInitCfm(Message message);
static void sppHandleConnectCfm(Message message);
static void sppHandleConnectInd(Message message);
static void sppDisconnectInd(void);
static void soundClearConnectMsg(Message message);
static void soundClearDisconnectMsg(void);
static void handleSppMessages(Task task, MessageId id, Message message);

/******************************************************************************
 * Local Data
 ******************************************************************************/
/* The SoundClear SPP object.  Space is allocated for this when the
   SPP_CONNECT_IND message is received from the CSR SPP library.  This is freed
   when soundClearSppDisconnect() is called or SPP_DISCONNECT_IND is received
   from the CSR SPP library. */
static ScSppData *gSppDataPtr = NULL;

/* The SoundClear Plug-In task that will receive messages fom the SoundClear
   SPP module.  Space is allocated for this when the  SOUNDCLEARCONNECTEDMSG
   is received from the plug-in.  It is freeded when the
   SOUNDCLEARDISCONNECTEDMSG is received*/
static Task pPluginTask = NULL;

/* Internal task to receive CSR SPP lib mesages*/
static const TaskData taskData = {handleSppMessages};

/******************************************************************************
 * Macros
 ******************************************************************************/
/******************************************************************************
 *
 * scSppPrintf()
 *
 * Description:
 *   This macro is a wrapper for printf().  It will output "SCAPP -
 *   soundclearSpp: " before the pFormt string.
 *
 * Arguments:
 *   msg
 *    [in] - The string to output.
 *
 * Return Value:
 *   None
 *
 *****************************************************************************/
#ifdef APPDEBUGSCSPP
#define scSppPrintf(msg) \
{ \
  printf("SCAPP - soundclearSpp: "); \
  printf msg; \
}
#else
#define scSppPrintf(msg)
#endif

/******************************************************************************
 * Public Function Definitions
 ******************************************************************************/
/******************************************************************************
 *
 * soundClearSppInit()
 *
 * Description:
 *   This function initializes the SPP library.  This function should only be
 *   called once per power cycle and must be called prior to AudioConnect();
 *
 * Arguments:
 *   None
 *
 * Return Value:
 *   None
 *
 ******************************************************************************/
void soundClearSppInit(void)
{
  scSppPrintf(("soundClearSppInit\n"));

  if (gSppDataPtr == NULL)
  {
    /* Initialize the CSR SPP library:*/
#if defined(APPBUILDSPPFORLEGACY)
    SppInit(&taskData, /* The task that will receive CSR SPP messages */
            devB,      /* We are a Slave SPP device. */
            255);      /* The low power mode priority of SPP */
#elif defined(APPBUILDFOR2010)
    SppStartService((Task)&taskData);
#else
    {
      spp_init_params init;

      init.client_recipe = NULL;    /* This must always be NULL per CSR */
      init.size_service_record = 0; /* Use SPP lib default record */
      init.service_record = NULL;   /* Use SPP lib default record */
      init.no_service_record = 0;   /* Use SPP lib default record */

      /* It is assumed that the SPP lib default record initializes SPP as
         a slave device with a low power priority of 255. */

      /* Initialise the spp profile lib, stating that this is device B */
      SppInitLazy((Task)&taskData, /* Task that will receive SPP_INIT_CFM */
                  (Task)&taskData, /* Task that will receive other SPP msgs */
                  &init);          /* SPP parameters */
    }
#endif

    /* Register the SPP task with the SoundClear Plug-In */
    soundClearRegisterSppTask((TaskData *)&taskData);
  }
}

/******************************************************************************
 *
 * soundClearSppDisconnect()
 *
 * Description:
 *   This function will cause the SPP link to be torn down if it exists.
 *   Otherwise, no action will be taken.  This is typically used during device
 *   power down.
 *
 * Arguments:
 *   None
 *
 * Return Value:
 *   None
 *
 ******************************************************************************/
void soundClearSppDisconnect(void)
{
  scSppPrintf(("soundClearSppDisconnect\n"));

#if defined(APPBUILDFOR2010)
  if (gSppDataPtr)
#else
  if (gSppDataPtr && gSppDataPtr->pSpp != NULL)
#endif
  {
#if defined(APPBUILDFOR2010)
    SppStopService((Task)&taskData);
#else
    SppDisconnect(gSppDataPtr->pSpp);
#endif
  }
}

/******************************************************************************
 * Private Function Definitions
 ******************************************************************************/
/******************************************************************************
 *
 * initDataPtr()
 *
 * Description:
 *   This function initialize gSppDataPtr and its members to default values
 *
 * Arguments:
 *   None
 *
 * Return Value:
 *   None
 *
 ******************************************************************************/
static void initDataPtr()
{
  scSppPrintf(("initDataPtr \n"));

  if (gSppDataPtr == NULL)
  {
    gSppDataPtr = PanicUnlessMalloc(sizeof(ScSppData));

#if !defined(APPBUILDFOR2010)
    gSppDataPtr->pSpp             = NULL;
#endif
    gSppDataPtr->sink             = NULL;
  }
}

/******************************************************************************
 *
 * sppHandleInitCfm()
 *
 * Description:
 *   This function handles the SPP_INIT_CFM message from the CSR SPP library
 *   indicating that the library has been initialized.  This function will
 *   print the whether or not library initialization was successful.  If
 *   the SPP library fails to be initialized than the SPP porifle will not
 *   be listed as an available service of the device.
 *
 * Arguments:
 *   Message message
 *     [in] The message data
 *
 * Return Value:
 *   None
 *
 ******************************************************************************/
static void sppHandleInitCfm(Message message)
{
#if defined (APPBUILDFOR2010)
  SPP_START_SERVICE_CFM_T *pSppInitCfmMsg = (SPP_START_SERVICE_CFM_T *)message;
#else
  SPP_INIT_CFM_T *pSppInitCfmMsg = (SPP_INIT_CFM_T *)message;
#endif

  scSppPrintf(("sppHandleInitCfm \n"));

#if defined (APPBUILDFOR2010)
  if (pSppInitCfmMsg->status == spp_start_success)
#else
  if (pSppInitCfmMsg->status == spp_init_success)
#endif
  {
    scSppPrintf(("SPP Init successful\n"));
  }
  /* Otherwise, free the data structure since we can't perform any SPP
     operations */
  else
  {
    scSppPrintf(("SPP Init failed\n"));
  }
}

/******************************************************************************
 *
 * sppHandleConnectCfm()
 *
 * Description:
 *   This function handles the SPP_CONNECT_CFM message confirming a new SPP
 *   connection.  This function will save the sink and then notify the
 *   SoundClear Plug-In that the SPP connection has been established.
 *
 * Arguments:
 *   Message message
 *     [in] The message data
 *
 * Return Value:
 *   None
 *
 ******************************************************************************/
static void sppHandleConnectCfm(Message message)
{
#if defined(APPBUILDFOR2010)
  SPP_SERVER_CONNECT_CFM_T *pSppConnectCfmMsg 
          = (SPP_SERVER_CONNECT_CFM_T *)message;
#else
  SPP_CONNECT_CFM_T *pSppConnectCfmMsg = (SPP_CONNECT_CFM_T *)message;
#endif
  SoundClearSppConnectCfmT *pSoundClearSppConnectCfm = NULL;

  scSppPrintf(("sppHandleConnectCfm \n"));

  /* If the connect was successful, then set our state to CONNECTED, save the
     address of the sink, and notify the registerd task */
  if (pSppConnectCfmMsg->status == spp_connect_success)
  {
    scSppPrintf(("Connection successful\n"));

    gSppDataPtr->sink = pSppConnectCfmMsg->sink;

    /* If a task as registered for the connect, then send the message */
    if (pPluginTask != NULL)
    {
      pSoundClearSppConnectCfm
              = (SoundClearSppConnectCfmT *)PanicUnlessNew(
                            SoundClearSppConnectCfmT);

      pSoundClearSppConnectCfm->sppSink = gSppDataPtr->sink;

      MessageSend(pPluginTask,
                  SOUNDCLEARSPPCONNECTCFM,
                  pSoundClearSppConnectCfm);
    }
  }
  /* Otherwise, set our state to NOT CONNECTED */
  else
  {
    scSppPrintf(("Connection failed\n"));
  }
}

/******************************************************************************
 *
 * sppHandleConnectInd()
 *
 * Description:
 *   This function handles the SPP_CONNECT_IND message indicating that a new
 *   device wants to connect.  This function will save the SPP profile handle
 *   and then accept the incoming connect request.
 *
 * Arguments:
 *   Message message
 *     [in] The message data
 *
 * Return Value:
 *   None
 *
 ******************************************************************************/
static void sppHandleConnectInd(Message message)
{
  SPP_CONNECT_IND_T *pSppConnectIndMsg = (SPP_CONNECT_IND_T *)message;

  scSppPrintf(("sppHandleConnectInd \n"));

  /* Init gSppDataPtr */
  initDataPtr();

#if defined(APPBUILDFOR2010)
  SppConnectResponse(
          (Task)&taskData, 
          &pSppConnectIndMsg->addr, 
          TRUE, 
          pSppConnectIndMsg->sink, 
          pSppConnectIndMsg->server_channel, 
          0);
#else
  /* Store the handle for the SPP profile */
  gSppDataPtr->pSpp = pSppConnectIndMsg->spp;

  /* Accept the request */
  SppConnectResponse(pSppConnectIndMsg->spp, TRUE, &pSppConnectIndMsg->addr);
#endif  
}

/******************************************************************************
 *
 * sppDisconnectInd()
 *
 * Description:
 *   This function handles the SPP_DISCONNECT_IND message indicating that the
 *   SPP connection has been terminated.  This function will un-register the
 *   SPP task (taskData) and free the SPP object.
 *
 * Arguments:
 *   None
 *
 * Return Value:
 *   None
 *
 ******************************************************************************/
static void sppDisconnectInd(void)
{
  scSppPrintf(("sppDisconnectInd \n"));

  /* If a task as registered for the connect, then send the message */
  if (pPluginTask != NULL)
  {
    MessageSend(pPluginTask, SOUNDCLEARSPPDISCONNECTIND, NULL);
  }

  /* Free the SPP object since we have no need for it anymore */
  free(gSppDataPtr);
  gSppDataPtr = NULL;
}

/******************************************************************************
 *
 * soundClearConnectMsg()
 *
 * Description:
 *   This function handles the SOUNDCLEARCONNECTEDMSG message from the
 *   SoundClear Plug-In indicating that is has been connected in an active
 *   call.  This function will save the SoundClear Plug-In task that will be
 *   used to send the plug-in messages.  If an SPP connection already exists
 *   than the SPP sink will be sent to the plug-in at this time.
 *
 * Arguments:
 *   Message message
 *    [in] The message from the SoundClear Plug-In.
 *
 * Return Value:
 *   None
 *
 ******************************************************************************/
static void soundClearConnectMsg(Message message)
{
  SoundClearSppConnectCfmT *pSoundClearSppConnectCfm;

  scSppPrintf(("soundClearConnectMsg \n"));

  /* Save the handle for the plug-in task */
  pPluginTask = ((SoundClearConnectMsgT *)message)->pSoundClearTaskData;

  /* Init gSppDataPtr */
  initDataPtr();

  /* If we have a valid SPP sink, then send it to the plug-in task */
  if (gSppDataPtr->sink)
  {
    pSoundClearSppConnectCfm
            = (SoundClearSppConnectCfmT *)PanicUnlessNew(
                          SoundClearSppConnectCfmT);

    pSoundClearSppConnectCfm->sppSink = gSppDataPtr->sink;

    MessageSend(pPluginTask,
                SOUNDCLEARSPPCONNECTCFM,
                pSoundClearSppConnectCfm);
  }
}

/******************************************************************************
 *
 * soundClearDisconnectMsg()
 *
 * Description:
 *   This function handles the SOUNDCLEARDISCONNECTEDMSG message from the
 *   SoundClear Plug-In indicating that it is no longer in an active call.
 *   The local handle to the plug-in task will be cleared to avoid sending
 *   unnecessary messages.
 *
 * Arguments:
 *   None
 *
 * Return Value:
 *   None
 *
 ******************************************************************************/
static void soundClearDisconnectMsg()
{
  scSppPrintf(("soundClearDisconnectMsg \n"));

  /* Clear the SoundClear Plug-In task */
  pPluginTask = NULL;
}

/******************************************************************************
 *
 * handleSppMessages()
 *
 * Description:
 *   This function handles all SPP messages from the CSR SPP library and the
 *   SoundClear Plug-Inand calls the appropriate handler for each one.  This
 *   function is actually called from main MessageLoop() via the function
 *   pointer registered when SppInit() is called in soundClearSppInit().
 *
 * Arguments:
 *   Task task
 *     [in] The address of the task data (i.e. this method)
 *          Not used here but required as part of the function pointer type
 *   MessageId id
 *     [in] The ID of the message - see spp.h
 *   Message message
 *     [in] The message data
 *
 * Return Value:
 *   None
 *
 ******************************************************************************/
static void handleSppMessages(Task task, MessageId id, Message message)
{
  scSppPrintf(("handleMessages \n"));

  switch(id)
  {
#if defined(APPBUILDFOR2010)
    case SPP_START_SERVICE_CFM:
#else
    case SPP_INIT_CFM:
#endif
      scSppPrintf(("SPP_INIT_CFM\n"));
      sppHandleInitCfm(message);
      break;
#if defined(APPBUILDFOR2010)
    case SPP_SERVER_CONNECT_CFM:
#else
    case SPP_CONNECT_CFM:
#endif
      scSppPrintf(("SPP_CONNECT_CFM\n"));
      sppHandleConnectCfm(message);
      break;
    case SPP_CONNECT_IND:
      scSppPrintf(("SPP_CONNECT_IND\n"));
      sppHandleConnectInd(message);
      break;
    case SPP_DISCONNECT_IND:
      scSppPrintf(("SPP_DISCONNECT_IND\n"));
      sppDisconnectInd();
      break;
    case SOUNDCLEARCONNECTEDMSG:
      scSppPrintf(("SOUNDCLEARCONNECTEDMSG\n"));
      soundClearConnectMsg(message);
      break;
    case SOUNDCLEARDISCONNECTEDMSG:
      scSppPrintf(("SOUNDCLEARDISCONNECTEDMSG\n"));
      soundClearDisconnectMsg();
      break;
 /* case SPP_MESSAGE_MORE_DATA:
    case SPP_MESSAGE_MORE_SPACE: */
    default:
      scSppPrintf(("Unhandled message received [%x]\n", id));
  }
}
