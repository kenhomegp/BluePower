/****************************************************************************

FILE NAME
    headset_init.h
    
DESCRIPTION
    
*/

#ifndef _HEADSET_INIT_H_
#define _HEADSET_INIT_H_

/*************************************************************************
NAME    
    InitUserFeatures
    
DESCRIPTION
    This function initialises all of the user features - this will result in a
    poweron message if a user event is configured correctly and the headset will 
    complete the power on

RETURNS

*/
void InitUserFeatures ( void );

/*************************************************************************
NAME    
    InitEarlyUserFeatures
    
DESCRIPTION
    This function initialises the configureation that is required early 
    on in the start-up sequence. 

RETURNS

*/
void InitEarlyUserFeatures ( void );

/****************************************************************************
NAME	
	SetupPowerTable

DESCRIPTION
	Attempts to obtain a low power table from the Ps Key store.  If no table 
	(or an incomplete one) is found	in Ps Keys then the default is used.
	
RETURNS
	void
*/
void SetupPowerTable( void );


/****************************************************************************
NAME    
    headsetHfpInit
    
DESCRIPTION
    Initialise the HFP library
	
RETURNS
    void
*/
void headsetHfpInit( void );


/****************************************************************************
NAME    
    headsetInitComplete
    
DESCRIPTION
    Headset initialisation has completed. 

RETURNS
    void
*/
void headsetInitComplete( const HFP_INIT_CFM_T *cfm );


#endif /* _HEADSET_INIT_H_ */
