/****************************************************************************

FILE NAME
    headset_scan.c

DESCRIPTION
    

NOTES

*/


/****************************************************************************
    Header files
*/
#include "headset_private.h"
#include "headset_scan.h"

#include <string.h>
#include <stdlib.h>
#include <connection.h>
#include <hfp.h>

#if defined(APPINCLSOUNDCLEAR) && defined(APPENABLESPPRAPID)
    #include "soundclearSpp.h"
#endif

/****************************************************************************
    Definitions used in EIR data setup
*/
/* Macro to acquire byte n from a multi-byte word w */
#define GET_BYTE(w, n) (((w) >> ((n) * 8)) & 0xFF)

/* EIR tags */
#define EIR_TYPE_LOCAL_NAME_COMPLETE        (0x09)
#define EIR_TYPE_LOCAL_NAME_SHORTENED       (0x08)
#define EIR_TYPE_UUID16_COMPLETE            (0x03)
#define EIR_TYPE_INQUIRY_TX					(0x0A)

/* Device UUIDs */
#define BT_UUID_SERVICE_CLASS_HFP           (0x111E)
#define BT_UUID_SERVICE_CLASS_HSP           (0x1108)
#define BT_UUID_SERVICE_CLASS_AUDIO_SINK	(0x110B)
#define BT_UUID_SERVICE_CLASS_A2DP			(0x110D)


/* Packet size defines */
#define MAX_PACKET_SIZE_DH1 (27)
#define EIR_MAX_SIZE        (MAX_PACKET_SIZE_DH1)
#define EIR_DATA_SIZE		(2)

/* UUIDs EIR data, not including length field */
static const uint8 eir_uuids[] = {
        EIR_TYPE_UUID16_COMPLETE,
        	GET_BYTE(BT_UUID_SERVICE_CLASS_HFP, 0),
        	GET_BYTE(BT_UUID_SERVICE_CLASS_HFP, 1),
			GET_BYTE(BT_UUID_SERVICE_CLASS_HSP, 0),
			GET_BYTE(BT_UUID_SERVICE_CLASS_HSP, 1),
			GET_BYTE(BT_UUID_SERVICE_CLASS_A2DP, 0),
			GET_BYTE(BT_UUID_SERVICE_CLASS_A2DP, 1),
			GET_BYTE(BT_UUID_SERVICE_CLASS_AUDIO_SINK, 0),
			GET_BYTE(BT_UUID_SERVICE_CLASS_AUDIO_SINK, 1)
        };

/****************************************************************************
NAME    
    headsetWriteEirData
    
DESCRIPTION
    Writes the local name and device UUIDs into device EIR data, local name 
	is shortened to fit into a DH1 packet if necessary

RETURNS
    void
*/
void headsetWriteEirData( CL_DM_LOCAL_NAME_COMPLETE_T *message )
{
/* Length of EIR data with all fields complete */
#define EIR_DATA_SIZE_FULL (message->size_local_name + 2 + sizeof(eir_uuids) + 1 + 1 + EIR_DATA_SIZE + 1)

/* Whether the EIR data is shortened or not. */
#define EIR_DATA_SHORTENED (EIR_DATA_SIZE_FULL > EIR_MAX_SIZE)

/* Maximum length the local name can be to fit EIR data into DH1 */
#define EIR_NAME_MAX_SIZE (EIR_MAX_SIZE - (2 + sizeof(eir_uuids) + 1 + 1 + EIR_DATA_SIZE + 1))

/* Actual length of the local name put into the EIR data */
#define EIR_NAME_SIZE (EIR_DATA_SHORTENED ? EIR_NAME_MAX_SIZE : message->size_local_name)

        /* Determine length of EIR data */
        uint16 size = EIR_DATA_SHORTENED ? EIR_MAX_SIZE : EIR_DATA_SIZE_FULL;
    
        /* Just enough for the UUID16, Inquiry Tx and name fields and null termination */
        uint8 *const eir = (uint8 *)mallocPanic(size * sizeof(uint8));
        uint8 *p = eir;
		
        *p++ = EIR_NAME_SIZE + 1;  /* Device Name Length Field */
        *p++ = EIR_DATA_SHORTENED ? EIR_TYPE_LOCAL_NAME_SHORTENED : EIR_TYPE_LOCAL_NAME_COMPLETE;
        /*memcpy(p, message->local_name, EIR_NAME_SIZE);*/
		memmove(p, message->local_name, EIR_NAME_SIZE);
        p += EIR_NAME_SIZE;
		
		*p++ = EIR_DATA_SIZE;	  /* Inquiry Tx Length Field */
		*p++ = EIR_TYPE_INQUIRY_TX;
		*p++ = theHeadset.inquiry_tx;
        
		*p++ = sizeof(eir_uuids); /* UUIDs length field */
        /*memcpy(p, eir_uuids, sizeof(eir_uuids));*/
		memmove(p, eir_uuids, sizeof(eir_uuids));
        p += sizeof(eir_uuids);
        *p++ = 0x00; /* Termination. p ends up pointing one off the end */

        ConnectionWriteEirData(FALSE, size, eir);
		
		/* Free the EIR data */
		free(eir);
}


/****************************************************************************
NAME    
    headsetEnableConnectable
    
DESCRIPTION
    Make the device connectable 

RETURNS
    void
*/
void headsetEnableConnectable( void )
{
    hci_scan_enable scan = hci_scan_enable_off;
	

    /* Set the page scan params */
    ConnectionWritePagescanActivity(theHeadset.radio->page_scan_interval, theHeadset.radio->page_scan_window);

    /* Make sure that if we're inquiry scanning we don't disable it */
    if (theHeadset.inquiry_scan_enabled)
        scan = hci_scan_enable_inq_and_page;
    else
        scan = hci_scan_enable_page;

#if defined(APPINCLSOUNDCLEAR) && defined(APPENABLESPPRAPID)
		/* This is just a reference example code to give customer some idea on
		   integrating soundclear plug in and soundclear SPP wireless RAPID.
		   Not for production use...*/
		scan = hci_scan_enable_inq_and_page;
#endif

    /* Enable scan mode */
    ConnectionWriteScanEnable(scan);

    /* Set the flag to indicate we're page scanning */
    theHeadset.page_scan_enabled = TRUE;
}


/****************************************************************************
NAME    
    headsetDisableConnectable
    
DESCRIPTION
    Take device out of connectable mode.

RETURNS
    void
*/
void headsetDisableConnectable( void )
{
    hci_scan_enable scan;

    /* Make sure that if we're inquiry scanning we don't disable it */
    if (theHeadset.inquiry_scan_enabled)
        scan = hci_scan_enable_inq;
    else
        scan = hci_scan_enable_off;

#if defined(APPINCLSOUNDCLEAR) && defined(APPENABLESPPRAPID)
		/* This is just a reference example code to give customer some idea on
		   integrating soundclear plug in and soundclear SPP wireless RAPID.
		   Not for production use...*/
		scan = hci_scan_enable_inq_and_page;
#endif

    /* Enable scan mode */
    ConnectionWriteScanEnable(scan);


#if defined(APPINCLSOUNDCLEAR) && defined(APPENABLESPPRAPID)
		theHeadset.page_scan_enabled = TRUE;
#else
		theHeadset.page_scan_enabled = FALSE;
#endif
		/* Set the flag to indicate we're page scanning */
}


/****************************************************************************
NAME    
    headsetEnableDiscoverable
    
DESCRIPTION
    Make the device discoverable. 

RETURNS
    void
*/
void headsetEnableDiscoverable( void )
{
    hci_scan_enable scan = hci_scan_enable_off;

    /* Set the inquiry scan params */
    ConnectionWriteInquiryscanActivity(theHeadset.radio->inquiry_scan_interval, theHeadset.radio->inquiry_scan_window);

    /* Make sure that if we're page scanning we don't disable it */
    if (theHeadset.page_scan_enabled)
        scan = hci_scan_enable_inq_and_page;
    else
        scan = hci_scan_enable_inq;

#if defined(APPINCLSOUNDCLEAR) && defined(APPENABLESPPRAPID)
		/* This is just a reference example code to give customer some idea on
		   integrating soundclear plug in and soundclear SPP wireless RAPID.
		   Not for production use...*/
		scan = hci_scan_enable_inq_and_page;
#endif

    /* Enable scan mode */
    ConnectionWriteScanEnable(scan);

    /* Set the flag to indicate we're page scanning */
    theHeadset.inquiry_scan_enabled = TRUE;
}


/****************************************************************************
NAME    
    headsetDisableDiscoverable
    
DESCRIPTION
    Make the device non-discoverable. 

RETURNS
    void
*/
void headsetDisableDiscoverable( void )
{
    hci_scan_enable scan;
    
    /* Make sure that if we're page scanning we don't disable it */
    if (theHeadset.page_scan_enabled)
        scan = hci_scan_enable_page;
    else
        scan = hci_scan_enable_off;

#if defined(APPINCLSOUNDCLEAR) && defined(APPENABLESPPRAPID)
		/* This is just a reference example code to give customer some idea on
		   integrating soundclear plug in and soundclear SPP wireless RAPID.
		   Not for production use...*/
		scan = hci_scan_enable_inq_and_page;
#endif

    /* Enable scan mode */
    ConnectionWriteScanEnable(scan);

    /* Set the flag to indicate we're page scanning */
    theHeadset.inquiry_scan_enabled = FALSE;
}
