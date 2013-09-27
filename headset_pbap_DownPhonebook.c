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

#ifdef DEBUG_PBAPx
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

#ifdef DebugPbap
typedef struct 
{
    uint8 telLen;
    uint8 *pTel;
    uint8 nameLen;
    uint8 pName[1];    
}pbapMetaData;

static const char gpbapbegin[] = "BEGIN:VCARD"; 
static const char gpbapname[]  = "\nN";
static const char gpbaptel[]   = "TEL";
static const char gpbapend[]   = "END:VCARD";

/* PBAP Client Data */
#define PBAPC_FILTER_VERSION        (1<<0)
#define PBAPC_FILTER_N              (1<<1)
#define PBAPC_FILTER_FN             (1<<2)
#define PBAPC_FILTER_TEL            (1<<7)
#define PBAPC_FILTER_LOW            ((uint32)(PBAPC_FILTER_VERSION | PBAPC_FILTER_N | PBAPC_FILTER_FN | PBAPC_FILTER_TEL))
#define PBAPC_FILTER_HIGH           ((uint32)0)
#endif

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
#ifdef DebugPbap
static uint8 VcardGetFirstTel(const uint8* pVcard, const uint16 vcardLen, pbapMetaData **pMetaData);
static uint16 VcardFindMetaData( const uint8 *start, const uint8 *end, uint8 **metaData, const char *str, const uint16 count);
static bool handlePbapDialDate(const uint8 *pVcard, const uint16 vcardLen);
#else
static uint8 VcardGetFirstTel(const char* pVcard, const uint8 vcardLen, char** pTel);
static bool handlePbapDialDate(const char* pVcard, const uint8 vcardLen);
#endif

#ifdef DebugPbap
static void handlePbapPhonebookDate(uint16 device_id, const uint8 *lSource, const uint16 vcardLen, bool more);
static void handlePbapvCardEntryDate(uint16 device_id, const uint8 *lSource, const uint16 vcardLen, bool more);
#else
static void handlePbapPhonebookDate(uint16 device_id, const uint8 *lSource, const uint8 vcardLen, bool more);
static void handlePbapvCardEntryDate(uint16 device_id, const uint8 *lSource, const uint8 vcardLen, bool more);
#endif

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
	PBAP_DEBUG(("[**PBPA] : message id = 0x%x\n",pId));
	
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
					#ifdef DebugPbap
						/*PbapcSetPhonebook(pMsg->device_id, pbap_local, pbap_ich);	Debug code*/
						/*PbapcSetPhonebook(pMsg->device_id, pbap_local, pbap_telecom);*/
						PBAP_DEBUG(("[BT Addr: nap %04x uap %02x lap %08lx]\n",ag_addr.nap,ag_addr.uap,ag_addr.lap));
						PBAP_DEBUG(("#####Download Phonebook!\n"));
						PbapcPullPhonebookStart(pMsg->device_id, pbap_local, 1, PBAPC_FILTER_LOW, PBAPC_FILTER_HIGH, pbap_format_21, 1000, 0);
					#endif
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
            
			#ifdef DebugPbap
			PbapcPullvCardEntryStart(pMsg->device_id, 0, 0x00000010, (uint32)0, pbap_format_21);
			#else
            /* Successfully set the phonebook, pull first entry from the phone book */	        
            PbapcPullvCardEntryStart(pMsg->device_id, 0, 0x00000010, (uint32)0, pbap_format_21);  /*does not work for LG phone*/
			#endif
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
	#ifdef DebugPbap
		PbapcPullPhonebookStart(pMsg->device_id, pbap_local, theHeadset.pbap_active_pb,
							 	0x00000010, 0, pbap_format_30, 1000, 0);
	#else
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
	#endif
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
#ifdef DebugPbap
static uint8 *memstr( const uint8 *buffer, const uint16 buffer_size, const uint8 *str, const uint16 count )
{

    uint8 *p = (uint8 *)memchr(buffer, str[0], buffer_size);
	/*
	PBAP_DEBUG(("PBAP memstr\n"));
    	*/
    while (p && p < buffer + buffer_size)
    {
        if(memcmp((char *)p, (char *)str, count) == 0)
        {
            return p;
        }
        p += 1;
        p = (uint8 *)memchr(p, str[0], (uint16)(buffer+buffer_size - p));
    }
    
    return 0;
}
static uint16 VcardFindMetaData( const uint8 *start, const uint8 *end, uint8 **metaData, const char *str, const uint16 count)
{
    uint16 len;
    uint8 *p         = (uint8 *)start;
    uint8 *endstring = NULL;

	/*  
    PBAP_DEBUG(("PBAP VcardFindMetaData\n"));
	*/
	
    /* find the MetaData */
    len         = (uint16)(end - p);
    
    if((((*metaData) = (uint8 *)memstr(p, len, (uint8 *)str, strlen(str))) != NULL) &&
       (((*metaData) = (uint8 *)memchr((uint8 *)(*metaData), ':',  end - (*metaData))) != NULL))
    {
        (*metaData) += 1;
        endstring    = (uint8 *)memchr((uint8 *)(*metaData), '\n', end - (*metaData)) - 1;
    }
    else
    {
        /* There are some errors about the format of phonebook. */
        return 0;
    }
    
    return(endstring - (*metaData));
}

static uint8 VcardGetFirstTel(const uint8* pVcard, const uint16 vcardLen, pbapMetaData **pMetaData)
{ 
    uint16 len    = 0;
    uint16 telLen, nameLen = 0;
    uint8 *pTel   = NULL;
    uint8 *pName  = NULL;
    
    /* Find the start and end position of the first Vcard Entry */
    uint8 *start  = memstr(pVcard, vcardLen, (uint8 *)gpbapbegin, strlen(gpbapbegin));
    uint8 *end    = memstr(pVcard, vcardLen, (uint8 *)gpbapend,   strlen(gpbapend));
    end           = (end == NULL) ? (uint8 *)(pVcard + vcardLen - 1) : end;

#ifdef DEBUG_PBAPx    
    {
        uint16 i;
        PBAP_DEBUG(("The pVcard is: "));

        for(i = 0; i < vcardLen; i++)
            PBAP_DEBUG(("%c", *(pVcard + i))); 
        
        PBAP_DEBUG(("\n"));    
    }
#endif
    
    PBAP_DEBUG(("First entry start:[%x], end:[%x]\n", (uint16)start, (uint16)end));
    
    while(start && start < end)
    {
        start = start + strlen(gpbapbegin);

        /* find the Tel */
        telLen = VcardFindMetaData(start, end, &pTel, gpbaptel, strlen(gpbaptel));
        
        if( telLen )
        {
            PBAP_DEBUG(("VcardGetFirstTel:telephone number found ok\n"));
            
            /* find the Name */
            nameLen = VcardFindMetaData(start, end, &pName, gpbapname, strlen(gpbapname));
            
            /* allocate the memory for pMetaData structure */
            *pMetaData = (pbapMetaData *)malloc(sizeof(pbapMetaData) + nameLen);

            if(pMetaData)
            {
                (*pMetaData)->pTel    = pTel;
                (*pMetaData)->telLen  = telLen;       
                (*pMetaData)->nameLen = nameLen;
            
                PBAP_DEBUG(("CallerID pos:[%x], len:[%d]\n", (uint16)pName, nameLen));
           
                if(nameLen)
                {
                    /* This memory should be freed after pbap dial command or TTS has completed */
                    memmove(&((*pMetaData)->pName), pName, nameLen);
                    (*pMetaData)->pName[nameLen] = '\0';
                
                    /* Remove the ';' between names */
                    /* Based on PBAP spec., the name format is: 
                      		LastName;FirstName;MiddleName;Prefix;Suffix
                    			*/
                    			
                    len = nameLen;
                    while(pName < (*pMetaData)->pName + nameLen)
                    {
                        pName    = (uint8 *)memchr(pName, ';', len) ;
                        *pName++ = ' ';
                        len  = len - (pName - (*pMetaData)->pName);
                    }
                
                    PBAP_DEBUG(("VcardGetFirstTel:CallerID found ok\n"));
                }

                return(telLen);
            }
            else
            {
                PBAP_DEBUG(("VcardGetFirstTel:No memory slot to store MetaData\n"));
                return 0;
            }
        }
        
        /* If the first Vcard Entry the Tel is enmty, try next Entry. */
        /* First find the next Vcard Entry start and end positions    */
        end = end + strlen(gpbapend);
        len = (uint16)(pVcard + vcardLen - end);
        start = memstr(end, len , (uint8 *)gpbapbegin, strlen(gpbapbegin));
        end   = memstr(end, len,  (uint8 *)gpbapend,   strlen(gpbapend));
        end   = (end == NULL) ? (uint8 *)(pVcard + vcardLen - 1) : end;
        
        PBAP_DEBUG(("next start:[%x], end:[%x]\n", (uint16)start, (uint16)end));
    }
     
    /*PBAP_DEBUG(("VcardGetFirstTel:telephone number not found\n"));*/
   
    return 0;
}

#else
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
#endif

#ifdef DebugPbap
static bool handlePbapDialDate(const  uint8 *pVcard, const uint16 vcardLen)
#else
static bool handlePbapDialDate(const char* pVcard, const uint8 vcardLen)
#endif
{
#ifdef DebugPbap
	pbapMetaData *pMetaData = NULL;
	bool success = FALSE;

  	/* Process Data to find telephone number*/
    if(VcardGetFirstTel(pVcard, vcardLen, &pMetaData))
    {
    	/*
        	PBAP_DEBUG(("handlePbapDial:dialling from PBAP Phonebook\n"));
        	*/
        	
        /* Display the name of tel of pbap dial entry.*/
        /* TTS can be used to play the caller ID */
        #ifdef DEBUG_PBAPx
        {
            uint8 i = 0;
            PBAP_DEBUG(("The Name is: "));
            for(i = 0; i < pMetaData->nameLen; i++)
                PBAP_DEBUG(("%c ", *(pMetaData->pName + i)));
     
            PBAP_DEBUG(("\nThe Tel is: "));
            for(i = 0; i < pMetaData->telLen; i++)
                PBAP_DEBUG(("%c ", *(pMetaData->pTel + i)));
            PBAP_DEBUG(("\n"));
        }
        #endif
                     
        success = TRUE;
        
    }
    else
    {
         /* error, number not found */
		 /*
         	 PBAP_DEBUG(("handlePbapDial:no number found to dial.\n"));
         	 */
    }
    
    if(pMetaData)
    {
        free(pMetaData);
    }

    return(success);	
#else
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
#endif
}

#ifdef DebugPbap
static void handlePbapPhonebookDate(uint16 device_id, const uint8 *lSource, const uint16 vcardLen, bool more)
#else
static void handlePbapPhonebookDate(uint16 device_id, const uint8 *lSource, const uint8 vcardLen, bool more)
#endif
{
    /* Process Data to find telephone number and dial it if found*/

	#ifdef DebugPbap
	if(handlePbapDialDate(lSource, vcardLen))
	#else
    if(handlePbapDialDate((char*)lSource, vcardLen))
	#endif
    {
        PbapcPacketComplete(device_id);
        /* Abort the current multi-packet operation */
        if (more)
        {
        	PBAP_DEBUG(("    PbapcAbort\n"));
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

#ifdef DebugPbap
static void handlePbapvCardEntryDate(uint16 device_id, const uint8 *lSource, const uint16 vcardLen, bool more)
#else
static void handlePbapvCardEntryDate(uint16 device_id, const uint8 *lSource, const uint8 vcardLen, bool more)
#endif
{
    /* Process Data to find telephone number and dial it if found*/
	#ifdef DebugPbap
	if(handlePbapDialDate(lSource, vcardLen))
	#else
    if(handlePbapDialDate((char*)lSource, vcardLen))
	#endif
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
