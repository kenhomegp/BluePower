/****************************************************************************

FILE NAME
    headset_scan.h
    
DESCRIPTION
    
*/

#ifndef _HEADSET_SCAN_H_
#define _HEADSET_SCAN_H_


/****************************************************************************
NAME    
    headsetWriteEirData
    
DESCRIPTION
    Writes the local name and device UUIDs into device EIR data, local name 
	is shortened to fit into a DH1 packet if necessary

RETURNS
    void
*/
void headsetWriteEirData( CL_DM_LOCAL_NAME_COMPLETE_T *message );


/****************************************************************************
NAME    
    headsetEnableConnectable
    
DESCRIPTION
    Make the device connectable 

RETURNS
    void
*/
void headsetEnableConnectable( void );


/****************************************************************************
NAME    
    headsetDisableConnectable
    
DESCRIPTION
    Take device out of connectable mode.

RETURNS
    void
*/
void headsetDisableConnectable( void );


/****************************************************************************
NAME    
    headsetEnableDiscoverable
    
DESCRIPTION
    Make the device discoverable. 

RETURNS
    void
*/
void headsetEnableDiscoverable( void );


/****************************************************************************
NAME    
    headsetDisableDiscoverable
    
DESCRIPTION
    Make the device non-discoverable. 

RETURNS
    void
*/
void headsetDisableDiscoverable( void );



#endif /* _HEADSET_SCAN_H_ */
