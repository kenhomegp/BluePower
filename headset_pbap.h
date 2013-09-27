/****************************************************************************

DESCRIPTION
	Interface definition for handling PBAP library messages and functionality

	
FILE
	headset_pbap.h
*/


#ifndef HEADSET_PBAP_H
#define HEADSET_PBAP_H

#ifdef ENABLE_PBAP

#include <message.h>
#include <pbapc.h>

/* PBAP Client Data */
#define PBAP_DEF_PACKET 255
#define PBAPC_MAX_REMOTE_DEVICES    2       /* Maximum number of separate remote devices supported */

/*!
    @brief Pbap link priority is used to identify different pbapc links to
    AG devices using the order in which the devices were connected.
*/
typedef enum
{
    /*! no pbapc dial. */
    pbapc_no_dial,
    /*! pbapc dialling. */
    pbapc_dialling,    
    /*! pbapc get the phone number and dialled. */
    pbapc_dialled
} pbapc_dial_state;

/*!
    @brief Pbap link priority is used to identify different pbapc links to
    AG devices using the order in which the devices were connected.
*/
typedef enum
{
    /*! The link that was connected first. */
    pbapc_primary_link,
    /*! The link that was connected second. */
    pbapc_secondary_link,    
    /*! Invalid Link. */
    pbapc_invalid_link
} pbapc_link_priority;


/****************************************************************************
NAME	
	handlePbapMessages
DESCRIPTION
    PBAP Server Message Handler
    
PARAMS
    task        associated task
    pId         message id           
    pMessage    message
    
RETURNS
	void
*/
void handlePbapMessages(Task task, MessageId pId, Message pMessage);


/****************************************************************************
NAME	
	initPbap

DESCRIPTION
    Initialise the PBAP System
    
PARAMS
    void
    
RETURNS
	void
*/
void initPbap(void);


/****************************************************************************
NAME	
	pbapConnect

DESCRIPTION
    Connect to PBAP Server
    
PARAMS
    pAddr    bd addr to connect pbap
    
RETURNS
    void
*/
void pbapConnect(const bdaddr pAddr);


/****************************************************************************
NAME	
	pbapDisconnect

DESCRIPTION
    Disconnect from PBAP Server
    
PARAMS
    void
    
RETURNS
	void
*/
void pbapDisconnect(void);


/****************************************************************************
NAME	
	pbapDialPhoneBook

DESCRIPTION
    Dial the first entry in the phonebook
    
PARAMS
    void
    
RETURNS
	void
*/
void pbapDialPhoneBook( uint8 phonebook );


#endif /*ENABLE_PBAP*/
								
#endif /* HEADSET_PBAP_H */
