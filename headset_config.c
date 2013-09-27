/****************************************************************************

FILE NAME
    headset_config.c
    
DESCRIPTION
    
*/

#include "headset_config.h"

#include <ps.h>
#include <string.h>
#include <panic.h>
#include <stdlib.h>

/****************************************************************************/
/* Reference the default configuartion definition files */

 	extern const config_type csr_pioneer_default_config;
 

	

 



/****************************************************************************/
/* Identify each default configuration, this defines an offset into the main config table */
typedef enum
{
	/* CSR Headset Pioneer */
 	csr_pioneer = (0),
	
 	entry_1 = (0),
 	entry_2 = (0),
	
 	entry_3 = (0),
	
 	entry_4 = (0),    
 	entry_5 = (0),
    entry_6 = (0),
	
    entry_7 = (0),
	
 	entry_8 = (0),
	
 	entry_9 = (0),
 
 	entry_10 = (0),

 	entry_11 = (0),

 	last_config_id = (12)
}config_id_type;
 

/****************************************************************************/
/* Table of default configurations */
const config_type* const default_configs[] = 
{
 
	/* CSR Headset Pioneer */
	 &csr_pioneer_default_config,
 
 	0,
  	0,

 	0,

 	0,
 	0,
 	0,

 	0,
	
 	0,
	
	0,

	0,

	0,


};


/****************************************************************************/
const uint8 * const default_service_records[] = 
{
		/* CSR Headset Pioneer */
 	0,
 	0,
 	0,
 	0,
 	0,
 	0,
    0,
    0,
	0,	
	0,
	0,
	0,
} ;

/****************************************************************************/
const uint16 * const default_service_record_lengths[] = 
{
		/* CSR Headset Pioneer */
 	0,
 	0,
 	0,
 	0,
 	0,
 	0,
    0,
    0,
	0,	
	0,
	0,
	0,
} ;

/****************************************************************************/
/****************************************************************************/


/****************************************************************************
NAME 
 	get_config_id

DESCRIPTION
 	This function is called to read the configuration ID
 
RETURNS
 	Defaults to config 0 if the key doesn't exist
*/
uint16 get_config_id(uint16 key)
{
 	/* Default to CSR standard configuration */
 	uint16 id = 0;
 
 	/* Read the configuration ID.  This identifies the configuration held in
       constant space */
 	if(PsRetrieve(key, &id, sizeof(uint16)))
 	{
  		if(id >= last_config_id)
  		{
   			id = 0;
  		}
 	}
 
 	return id;
}


/****************************************************************************
NAME 
 	ConfigRetrieve

DESCRIPTION
 	This function is called to read a configuration key.  If the key exists
 	in persistent store it is read from there.  If it does not exist then
 	the default is read from constant space
 
RETURNS
 	0 if no data was read otherwise the length of data
*/
uint16 ConfigRetrieve(uint16 config_id, uint16 key_id, void* data, uint16 len)
{
 	uint16 ret_len;
 
 	/* Read requested key from PS if it exists */
 	ret_len = PsRetrieve(key_id, data, len);
     
 	/* If no key exists then read the parameters from the default configuration
       held in constant space */
 	if(!ret_len)
 	{
  		/* Access the required configuration */
  		if( default_configs[ config_id ] )
  		{  
            key_type * key;
            
            /* as the default configuration structures are aligned in key_id order
               it is safe to set the pointer such that it treats the default config
               config_type structure as an array and index that via the key_id value.

               The following line casts the default_config pointer to the first entry
               in the configuration structure which will be (key_type *)&battery_config, 
               the key_id (0 to 31) is then added to the start of the config to give the
               correct offset with the structure. Key then retrieves the .length and .data
               pointer from the configuration */
            key = *((key_type**)default_configs[config_id] + key_id);
            
            /* Providing the retrieved length is not zero. */
   			if(key->length == 0)
			{
				/* This will indicate an error. */
				ret_len = 0;
			}
			else
			{
	   			if(key->length == len)
	   			{
	    			/* Copy from constant space */
					memmove(data, &key->data, len);
	    			ret_len = len;
	   			}
	   			else
	   			{
					if(key->length > len)
					{
						DEBUG(("CONF:BADLEN![%x][%x][%x]\n",key_id, key->length, len)) ;	
						Panic() ;
					}
					else
					{
		   				/* (key.length < len) && (key.length != 0) here since we're comparing unsigned numbers. */

		   				/* We have more space than the size of the key in constant space.
		   				   Just copy the data for the key.
		   				   The length returned will let the caller know about the length mismatch. */
		    			/* Copy from constant space */
						memmove(data, &key->data, key->length);
		    			ret_len = key->length;
	    			}
	   			}
   			}
  		}
 	}
    else
    {
    	switch(key_id)
    	{
			/* PS keys where it's ok for (ret_len != len) */
    		case(PSKEY_CODEC_NEGOTIATION):
    			break;
            case(PSKEY_CONFIG_TONES):
                break;
    		default:
				if (ret_len != len)
				{
					DEBUG(("CONF:BADLEN![%x][%x][%x]\n",key_id, ret_len, len)) ;	
					Panic() ;
				}		 
    			break;
    	}
    }   
 
 	return ret_len;
}

/****************************************************************************
NAME 
 	get_service_record

DESCRIPTION
 	This function is called to get a special service record associated with a 
 	given configuration
 
RETURNS
 	a pointer to the service record
*/
uint8 * get_service_record ( void ) 
{
	uint16 lIndex = get_config_id(PSKEY_CONFIGURATION_ID);
    
	uint8 * lServiceRecord = (uint8*) default_service_records[lIndex] ;
	
	DEBUG(("CONF: Service_Record[%d][%d]\n",lIndex , (int)lServiceRecord)) ;
    	
	return lServiceRecord ;
}

/****************************************************************************
NAME 
 	get_service_record_length

DESCRIPTION
 	This function is called to get the length of a special service record 
 	associated with a given configuration
 
RETURNS
 	the length of the service record
*/
uint16 get_service_record_length ( void ) 
{
	uint16 lIndex = get_config_id(PSKEY_CONFIGURATION_ID);
    
    uint16 lLength = 0 ;

    if (default_service_record_lengths[lIndex])
    {
	   lLength = *default_service_record_lengths[lIndex] ;
    }

	DEBUG(("CONF: Service Record Len = [%d]\n", lLength)) ;
	
	return lLength ;
}
