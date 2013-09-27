/****************************************************************************

DESCRIPTION
	Implementation for handling PBAP library messages and functionality
	
FILE
	headset_pbap.c
	
*/

/****************************************************************************
    Header files
*/


#include <connection.h>
#include <hfp.h>
#include <print.h>
#include <panic.h>
#include <stdlib.h>
#include <bdaddr.h>
#include <stream.h>
#include <string.h>
#include <sink.h>

#ifdef ENABLE_PBAP

#include <pbapc.h>
#include <pbap_common.h>
#include <md5.h>


#include "headset_pbap.h"
#include "headset_private.h"
#include "headset_init.h"
#include "headset_slc.h"
#include "headset_statemanager.h"
#include "headset_callmanager.h"

#ifdef DEBUG_PBAP
    #define PBAP_DEBUG(x) {printf x;}
#else
    #define PBAP_DEBUG(x) 
#endif

#ifdef Rubidium
#include "headset_tts.h"
#include <rubidium_text_to_speech_plugin.h> 
extern char	TTS_text[50];
#endif

#define VCARD_TEL     "TEL"

/* Message Handler Prototypes */

static void handlePbapInitCfm(PBAPC_INIT_CFM_T *pMsg);
static void handlePbapConnectCfm(PBAPC_CONNECT_CFM_T *pMsg);
static void handlePbapDisconnectInd(PBAPC_DISCONNECT_IND_T *pMsg);

static void handlePbapSetPhonebookCfm(PBAPC_SET_PHONEBOOK_CFM_T *pMsg);

static void handlePullVCardEntryStartInd(PBAPC_PULL_VCARD_ENTRY_START_IND_T *pMsg);
static void handlePullVCardEntryDataInd(PBAPC_PULL_VCARD_ENTRY_DATA_IND_T *pMsg);
static void handlePullVCardEntryCompleteInd(PBAPC_PULL_VCARD_ENTRY_COMPLETE_IND_T *pMsg);

static void handlePullPhonebookStartInd(PBAPC_PULL_PHONEBOOK_START_IND_T *pMsg);
static void handlePullPhonebookDataInd(PBAPC_PULL_PHONEBOOK_DATA_IND_T *pMsg);
static void handlePullPhonebookCompleteInd(PBAPC_PULL_PHONEBOOK_COMPLETE_IND_T *pMsg);
static void handleAuthRequestInd(PBAPC_AUTH_REQUEST_IND_T *pMsg);

static uint8 VcardGetFirstTel(const char* pVcard, const uint8 vcardLen, char** pTel);
static bool handlePbapDialDate(const char* pVcard, const uint8 vcardLen);
static void handlePbapPhonebookDate(uint16 device_id, const uint8 *lSource, const uint8 vcardLen, bool more);
static void handlePbapvCardEntryDate(uint16 device_id, const uint8 *lSource, const uint8 vcardLen, bool more);

static void pbapDial(uint8 phonebook);

/* Interface Functions */
/*
  Initialise the PBAP System
*/
void initPbap(void)
{      
    PBAP_DEBUG(("initPbap\n"));
  
    /* initialise defaults */
    theHeadset.pbap_ready       = FALSE;
    theHeadset.pbap_dial_state  = pbapc_no_dial;
    theHeadset.pbap_active_link = pbapc_invalid_link;
    theHeadset.pbap_active_pb   = pbap_b_unknown;

    /* Initialise the PBAP library */
	PbapcInit(&theHeadset.task, 1);
}

/*
	Connect to PBAP Server
*/
void pbapConnect(const bdaddr pAddr)
{
    if(!BdaddrIsZero(&pAddr))
    {
        PBAP_DEBUG(("Connecting Pbap profile, Addr %x,%x,%lx\n", pAddr.nap, pAddr.uap, pAddr.lap ));
        PbapcConnect(&pAddr, PBAP_DEF_PACKET);
    }
}

/*
	Disconnect from PBAP Server
*/
void pbapDisconnect( void )
{
    uint16 index = 0;
    
    PBAP_DEBUG(("Disconnect all Pbap connections\n"));
    
    for(index = 0; index < PBAPC_MAX_REMOTE_DEVICES; index++)
    {
        PbapcDisconnect( index );
    }
}

/*
	Dial the first entry in the specified phonebook
*/
static void pbapDial(uint8 phonebook)
{  
    /* attempt to dial the first entry in the AG missed call history */
    if(theHeadset.pbap_ready)
    {
        if(theHeadset.pbap_active_link != pbapc_invalid_link)
        {
            /* the Pbap profile of the primary HFP device has been connected */ 
            /* set the phonebook and dial */
            PBAP_DEBUG(("Pbap dial, set the phonebook first\n"));
            PbapcSetPhonebook(theHeadset.pbap_active_link, pbap_local, phonebook);
        }
        else
        {
            /* Otherwise, try to connect Pbap profile of the primary HFP device before dialling */ 
            bdaddr ag_addr;
            Sink sink;
            
            if( HfpLinkGetSlcSink(hfp_primary_link, &sink) && SinkGetBdAddr(sink, &ag_addr) )
            {
                PBAP_DEBUG(("Pbap dial, connect the Pbap profile first\n"));
                pbapConnect(ag_addr);
            }
        }        
        
        theHeadset.pbap_active_pb = phonebook;
    }
    else
    {
        PBAP_DEBUG(("PBAPC profile was not initialised\n"));
        
        MessageSend ( &theHeadset.task , EventPbapDialFail , 0 ) ;
        
        theHeadset.pbap_dial_state = pbapc_no_dial;
    }
}

/*
	Dial the first entry in the phonebook
*/
void pbapDialPhoneBook( uint8 phonebook )
{
    if(theHeadset.pbap_dial_state != pbapc_dialling)
    {
        if (!stateManagerIsConnected() )
        {	
#ifdef ENABLE_AVRCP
            headsetAvrcpCheckManualConnectReset(NULL);
#endif            
            PBAP_DEBUG(("Pbap dial, Connect the HFP profile first\n"));
            MessageSend ( &theHeadset.task , EventEstablishSLC , 0 ) ;
            
            switch(phonebook)
            {
                case pbap_ich:
                    headsetQueueEvent( EventPbapDialIch ) ;
                break;
                case pbap_mch:
                    headsetQueueEvent( EventPbapDialMch ) ;
                break;
                default:
                break;
            }
        }
        else
        {   
        	#ifdef ActiveRubiASR
				#if 0
				if(strcmp(TTS_text , ""))
				{
					PBAP_DEBUG(("Pbap : Rubi > UnloadRubiEngine***\n"));	

					TTSTerminate();

					AudioDisconnect();
					/* clear sco_sink value to indicate no routed audio */
					theHeadset.sco_sink = 0;

					UnloadRubidiumEngine();
					memset(TTS_text, 0, sizeof(TTS_text));  
				}
				#endif
				if(theHeadset.TTS_ASR_Playing)
				{
					PBAP_DEBUG(("Pbap : Rubi > UnloadRubiEngine***\n"));	

					TTSTerminate();

					AudioDisconnect();
					/* clear sco_sink value to indicate no routed audio */
					theHeadset.sco_sink = 0;

					UnloadRubidiumEngine();
					
					theHeadset.TTS_ASR_Playing = false;
				}
			#endif
			
            pbapDial(phonebook);
        }    
        
        theHeadset.pbap_dial_state = pbapc_dialling;
    }
}


/*
    Message Handler:
	    Process the Pbapc profile message from library
*/
void handlePbapMessages(Task task, MessageId pId, Message pMessage)
{
	switch (pId)
	{
	case PBAPC_INIT_CFM:
		handlePbapInitCfm((PBAPC_INIT_CFM_T *)pMessage);
		break;
	case PBAPC_CONNECT_CFM:
		handlePbapConnectCfm((PBAPC_CONNECT_CFM_T *)pMessage);
		break;
	case PBAPC_DISCONNECT_IND:
		handlePbapDisconnectInd((PBAPC_DISCONNECT_IND_T *)pMessage);
		break;
	case PBAPC_SET_PHONEBOOK_CFM:
		handlePbapSetPhonebookCfm((PBAPC_SET_PHONEBOOK_CFM_T *)pMessage);
		break;

	case PBAPC_PULL_VCARD_LIST_START_IND:
		break;
	case PBAPC_PULL_VCARD_LIST_DATA_IND:
   	    break;
	case PBAPC_PULL_VCARD_LIST_COMPLETE_IND:
		break;
		
	case PBAPC_PULL_VCARD_ENTRY_START_IND:
		handlePullVCardEntryStartInd((PBAPC_PULL_VCARD_ENTRY_START_IND_T *)pMessage);
		break;
	case PBAPC_PULL_VCARD_ENTRY_DATA_IND:
		handlePullVCardEntryDataInd((PBAPC_PULL_VCARD_ENTRY_DATA_IND_T *)pMessage);
		break;
	case PBAPC_PULL_VCARD_ENTRY_COMPLETE_IND:
		handlePullVCardEntryCompleteInd((PBAPC_PULL_VCARD_ENTRY_COMPLETE_IND_T *)pMessage);
		break;
		
	case PBAPC_PULL_PHONEBOOK_START_IND:
		handlePullPhonebookStartInd((PBAPC_PULL_PHONEBOOK_START_IND_T *)pMessage);
		break;
	case PBAPC_PULL_PHONEBOOK_DATA_IND:
		handlePullPhonebookDataInd((PBAPC_PULL_PHONEBOOK_DATA_IND_T *)pMessage);
		break;
	case PBAPC_PULL_PHONEBOOK_COMPLETE_IND:
		handlePullPhonebookCompleteInd((PBAPC_PULL_PHONEBOOK_COMPLETE_IND_T *)pMessage);
		break;
		
    case PBAPC_AUTH_REQUEST_IND :
        handleAuthRequestInd((PBAPC_AUTH_REQUEST_IND_T *)pMessage);
        break;
	case PBAPC_SERVER_PROPERTIES_COMPLETE_IND :
        break;
        
	default:
        PBAP_DEBUG(("PBAPC Unhandled message : 0x%X\n",pId));
        break;
	}
}


/* Message Handlers */

static void handlePbapInitCfm( PBAPC_INIT_CFM_T *pMsg)
{
	PBAP_DEBUG(("PBAPC_INIT_CFM : "));
	if (pMsg->status == pbapc_success)
	{
		PBAP_DEBUG(("success\n"));
        theHeadset.pbap_ready = TRUE;
        
        /*Register PBAP with SDP record*/
        PbapcRegisterSDP(pbapc_primary_link,   PBAP_SUP_DOWNLOAD | PBAP_SUP_BROWSE);
        PbapcRegisterSDP(pbapc_secondary_link, PBAP_SUP_DOWNLOAD | PBAP_SUP_BROWSE);        

        /* start initialising the configurable parameters*/
    	InitUserFeatures() ; 
	}
	else
	{ /* Failed to initialise PBAPC */
		PBAP_DEBUG(("PBAP init failed   Status : %d\n", pMsg->status));	
		Panic();
	}
}

static void handlePbapConnectCfm(PBAPC_CONNECT_CFM_T *pMsg)
{	
	PBAP_DEBUG(("PBAPC_CONNECT_CFM, Status : %d, packet size:[%d]\n", pMsg->status, pMsg->packetSize));
	
    if(pMsg->status == pbapc_success)
    {
        /* If the Pbap of primary HFP device has been connected, save its device_id as the active link */
        bdaddr ag_addr;
        Sink sink;
            
        if( HfpLinkGetSlcSink(hfp_primary_link, &sink) && SinkGetBdAddr(sink, &ag_addr) )
        {
            if(BdaddrIsSame(&ag_addr, &(pMsg->bdAddr)))
            {
                theHeadset.pbap_active_link = pMsg->device_id;
                PBAP_DEBUG(("PBAPC_CONNECT_CFM, Set the active Pbap link as [%d]\n", theHeadset.pbap_active_link));
		#ifdef BHC612
		PBAP_DEBUG(("[BT Addr: nap %04x uap %02x lap %08lx]\n",ag_addr.nap,ag_addr.uap,ag_addr.lap));
		#endif
            }
        }

        /* if we are making Pbapc dialing now */
        if(theHeadset.pbap_dial_state == pbapc_dialling)
        {
            PBAP_DEBUG(("PBAPC_CONNECT_CFM, Pbap dialling, set the phonebook\n"));
            /* Set required phonebook */
            PbapcSetPhonebook(pMsg->device_id, pbap_local, theHeadset.pbap_active_pb); 
        }
    }
    else    
    {
        /* pbapc profile connection failure */
        if(theHeadset.pbap_dial_state == pbapc_dialling)
        {
            MessageSend ( &theHeadset.task , EventPbapDialFail , 0 ) ;
            theHeadset.pbap_dial_state = pbapc_no_dial;
        }            
    }
}

static void handlePbapDisconnectInd(PBAPC_DISCONNECT_IND_T *pMsg)
{
    PBAP_DEBUG(("PBAPC_DISCONNECT_IND, number of pbapc link is [%d], ", pMsg->num_of_pbapc_links));
    if(theHeadset.pbap_active_link == pMsg->device_id)
    {
        /* The primary HFP device has been disconnected */
        if(pMsg->num_of_pbapc_links == 1)
            theHeadset.pbap_active_link = pbapc_secondary_link - pMsg->device_id;
        else
            theHeadset.pbap_active_link = pbapc_invalid_link;
        
        PBAP_DEBUG(("change the active pbap link id [%d]\n", theHeadset.pbap_active_link));
    }
}

static void handlePbapSetPhonebookCfm(PBAPC_SET_PHONEBOOK_CFM_T *pMsg)
{
	PBAP_DEBUG(("PBAPC_SET_PHONEBOOK_CFM, Status : %d\n", pMsg->status));
    
    switch(pMsg->status)
    {
        case pbapc_success:
        {
            PBAP_DEBUG(("PBAPC_SET_PHONEBOOK_CFM, Start pulling vcard Entry\n"));
            
            /* Successfully set the phonebook, pull first entry from the phone book */	        
            PbapcPullvCardEntryStart(pMsg->device_id, 0, 0x00000010, (uint32)0, pbap_format_21);  /*does not work for LG phone*/
        }
        break;
        
        case pbapc_spb_unauthorised:
            /* access to this phonebook denied by PBAP server */
            PBAP_DEBUG(("PBAP access to phonebook unauthorised\n")); 
            
            MessageSend ( &theHeadset.task , EventPbapDialFail , 0 ) ;
            theHeadset.pbap_dial_state = pbapc_no_dial;
        break;
        
        default:
            /* other error */
            PBAP_DEBUG(("PBAP failed to set phonebook\n")); 
            
            MessageSend ( &theHeadset.task , EventPbapDialFail , 0 ) ; 
            theHeadset.pbap_dial_state = pbapc_no_dial;
        break;
    }
}

static void handlePullVCardEntryStartInd(PBAPC_PULL_VCARD_ENTRY_START_IND_T *pMsg)
{
    const uint8 *lSource = SourceMap(pMsg->src);
  
    PBAP_DEBUG(("PBAPC_PULL_VCARD_ENTRY_START_IND\n"));
    
    lSource += pMsg->dataOffset;
    
    handlePbapvCardEntryDate(pMsg->device_id, lSource, pMsg->dataLen, pMsg->moreData);
}

static void handlePullVCardEntryDataInd(PBAPC_PULL_VCARD_ENTRY_DATA_IND_T *pMsg)
{
    const uint8 *lSource = SourceMap(pMsg->src);
    PBAP_DEBUG(("PBAPC_PULL_VCARD_ENTRY_DATA_IND\n"));	
    
    lSource += pMsg->dataOffset;
    
    handlePbapvCardEntryDate(pMsg->device_id, lSource, pMsg->dataLen, pMsg->moreData);
}

static void handlePullVCardEntryCompleteInd(PBAPC_PULL_VCARD_ENTRY_COMPLETE_IND_T *pMsg)
{
    /* Completed pull of the VCARD, successfully but it is still dialling state, try phone nook */
    if(theHeadset.pbap_dial_state == pbapc_dialling)
    {
        PBAP_DEBUG(("PBAPC_PULL_VCARD_ENTRY_COMPLETE_IND, Pbap dial fail, try pulling phonebook.\n"));
        
        /* It seems that Vcard entry failed, try pull phone book */
        PbapcPullPhonebookStart(pMsg->device_id, pbap_local, theHeadset.pbap_active_pb,
							 	0x00000010, 0, pbap_format_21, 1000, 0);
    }
    else
    {
       	PBAP_DEBUG(("PBAPC_PULL_VCARD_ENTRY_COMPLETE_IND, Pbap dial finished\n"));
        /* Have finished Pbap dial */
        theHeadset.pbap_dial_state = pbapc_no_dial;
    }
}

static void handlePullPhonebookStartInd(PBAPC_PULL_PHONEBOOK_START_IND_T *pMsg)
{
    const uint8 *lSource = SourceMap(pMsg->src);
    
	PBAP_DEBUG(("PBAPC_PULL_PHONEBOOK_START_IND, len [%d] book size [%d] total length [%ld]\n", 
                pMsg->dataLen, pMsg->pbook_size, pMsg->totalLength));
    
    lSource += pMsg->dataOffset;
    
    handlePbapPhonebookDate(pMsg->device_id, lSource, pMsg->dataLen, pMsg->moreData);
}

static void handlePullPhonebookDataInd(PBAPC_PULL_PHONEBOOK_DATA_IND_T *pMsg)
{
    const uint8 *lSource = SourceMap(pMsg->src);
    
	PBAP_DEBUG(("PBAPC_PULL_PHONEBOOK_DATA_IND, len: [%d]\n", pMsg->dataLen));
    
    lSource += pMsg->dataOffset;
    
    handlePbapPhonebookDate(pMsg->device_id, lSource, pMsg->dataLen, pMsg->moreData);
}

static void handlePullPhonebookCompleteInd(PBAPC_PULL_PHONEBOOK_COMPLETE_IND_T *pMsg)
{
	PBAP_DEBUG(("PBAPC_PULL_PHONEBOOK_COMPLETE_IND\n"));
        
    /* It seems that Phonebook entry failed, give up Pbap dialling*/
    theHeadset.pbap_dial_state = pbapc_no_dial;
}

static void handleAuthRequestInd(PBAPC_AUTH_REQUEST_IND_T *pMsg)
{
    uint8 digest[GOEP_SIZE_DIGEST];
    PBAP_DEBUG(("PBAPC_AUTH_REQUEST_IND\n"));

    {
        MD5_CTX context;
        /* Digest blocks */
        MD5Init(&context);
        MD5Update(&context, pMsg->nonce, strlen((char *)pMsg->nonce));
        MD5Update(&context, (uint8 *)":",1);
        MD5Update(&context, (uint8 *)"8888",4);
        MD5Final(digest,&context);
    }
    
    PbapcConnectAuthResponse(pMsg->device_id, &digest[0], 0, NULL, NULL);
}

/****************************************************************************
NAME	
	VcardGetFirstTel

DESCRIPTION
    Find the first telephone number from the supplied VCARD data
    
PARAMS
    pVcard   pointer to supplied VCARD data
    pTel     pointer to section of pVcard where the telephone number begins
    
RETURNS
	uint8    length of the found telephone number, 0 if not found
*/
static uint8 VcardGetFirstTel(const char* pVcard, const uint8 vcardLen, char** pTel)
{ 
    *pTel = (char*)pVcard;

    /* find the telephone number in the vcard*/
    while (*pTel && (*pTel < (char*)(pVcard+vcardLen)))
    {              
        /* Find the leading letter */
        *pTel = strchr((char*)(*pTel),'T');
        if(pTel)
        {
            /* first letter found now see if this is the string wanted */
            if(strncmp(*pTel,VCARD_TEL,3) ==0)
            {
                 char* pTelEnd = 0;            
                 PBAP_DEBUG(("VcardGetFirstTel:telephone number found ok\n"));
     
                 /* find telephone number after colon */
                 *pTel = strchr((char*)(*pTel),':') + 1;
             
                 /* find end of tel number */
                 pTelEnd = strchr((char*)(*pTel),'\n') - 1;
             
                 if (pTelEnd)
                 {
                     /* return the length of the telephone number */
                     return (pTelEnd - *pTel);
                 }
            }
            else
            {
                /* this is not a telephone number, move on*/ 
                *pTel = *pTel+strlen(VCARD_TEL);
            }
        }
    }
    
    /* Telephone number not found */
    PBAP_DEBUG(("VcardGetFirstTel:telephone number not found\n"));
    *pTel = NULL;
    return 0;
}


static bool handlePbapDialDate(const char* pVcard, const uint8 vcardLen)
{
    uint8 telLen = 1;
    char* pTel = 0;               
    bool success = FALSE;
    
  	/* Process Data to find telephone number*/
    telLen = VcardGetFirstTel((char*)pVcard, vcardLen, &pTel);
    
    if(telLen)
    {
        PBAP_DEBUG(("handlePbapDial:dialling from PBAP Phonebook\n"));
    
        theHeadset.pbap_dial_state = pbapc_dialled;

        HfpDialNumberRequest(hfp_primary_link, telLen, (uint8 *)pTel);
               
        success = TRUE;
   }
    else
    {
         /* error, number not found */   
         PBAP_DEBUG(("handlePbapDial:no number found to dial in VCARD\n"));
    }
    
    return(success);
}

static void handlePbapPhonebookDate(uint16 device_id, const uint8 *lSource, const uint8 vcardLen, bool more)
{
    /* Process Data to find telephone number and dial it if found*/
    if(handlePbapDialDate((char*)lSource, vcardLen))
    {
        PbapcPacketComplete(device_id);
        /* Abort the current multi-packet operation */
        if (more)
        {
            PbapcAbort(device_id);
        }
    }
    else
    {
	    PbapcPacketComplete(device_id);	
	    if (more)
	    {
		    PBAP_DEBUG(("    Requesting next Packet\n"));
		    PbapcPullPhonebookNext(device_id);
	    }
    }
}

static void handlePbapvCardEntryDate(uint16 device_id, const uint8 *lSource, const uint8 vcardLen, bool more)
{
    /* Process Data to find telephone number and dial it if found*/
    if(handlePbapDialDate((char*)lSource, vcardLen))
    {
        PbapcPacketComplete(device_id);
        /* Abort the current multi-packet operation */
        if (more)
        {
            PbapcAbort(device_id);
        }
    }
    else
    {
	    PbapcPacketComplete(device_id);	
	    if (more)
	    {
		    PBAP_DEBUG(("    Requesting next Packet\n"));
		    PbapcPullvCardEntryNext(device_id);
	    }
    }
}


#endif /*ENABLE_PBAP*/
