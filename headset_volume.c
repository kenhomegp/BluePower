/****************************************************************************

FILE NAME
    headset_volume.c
    
DESCRIPTION
    module responsible for Vol control 
    
*/
#include "headset_statemanager.h"
#include "headset_volume.h"
#include "headset_tones.h"
#include "headset_pio.h"
#include "headset_slc.h"
#include "headset_audio.h"

#ifdef ENABLE_AVRCP
#include "headset_avrcp.h"    
#endif 

#include <audio.h>
#include <stddef.h>


#ifdef DEBUG_VOLUME
#define VOL_DEBUG(x) DEBUG(x)
#else
#define VOL_DEBUG(x) 
#endif


/****************************************************************************
NAME 
 CheckVolumeA2dp

DESCRIPTION
 check whether any a2dp connections are present and if these are currently active
 and routing audio to the headset, if that is the case adjust the volume up or down
 as appropriate

RETURNS
 void
    
*/
bool CheckVolumeA2dp(volume_direction dir)
{
    uint8 index;
 
    /* check both possible instances of a2dp connection */
    for(index = a2dp_primary; index < (a2dp_secondary+1); index++)
    {
        /* is a2dp connected? */
        if(theHeadset.a2dp_link_data->connected[index])
        {
            /* check whether the a2dp connection is present and streaming data and that the audio is routed */
            if(theHeadset.sco_sink && (theHeadset.sco_sink == A2dpMediaGetSink(theHeadset.a2dp_link_data->device_id[index], theHeadset.a2dp_link_data->stream_id[index])))
            {
                /* get current volume for this profile */
                uint16 lOldVol = theHeadset.a2dp_link_data->gAvVolumeLevel[index];
                
                /* change up or down */
                if(dir == increase_volume)
                {
                    /* increase volume by one level up to maximum */
                    theHeadset.a2dp_link_data->gAvVolumeLevel[index] = theHeadset.audioData.gVolMaps[lOldVol].IncVol ;
                    /* limit to maximum and send notification event when at max level */
                    if(theHeadset.a2dp_link_data->gAvVolumeLevel[index] >= VOLUME_A2DP_MAX_LEVEL)
                    {
                        theHeadset.a2dp_link_data->gAvVolumeLevel[index] = VOLUME_A2DP_MAX_LEVEL;
                        MessageSend ( &theHeadset.task , EventVolumeMax , 0 );
                    }
                }
                /* decrease volume */
                else
                {
                    /* decrease volume by one level down to minimum */
                    theHeadset.a2dp_link_data->gAvVolumeLevel[index] = theHeadset.audioData.gVolMaps[lOldVol].DecVol ;
                    /* limit to minimum and send notification event when at min level */
                    if(theHeadset.a2dp_link_data->gAvVolumeLevel[index] == VOLUME_A2DP_MIN_LEVEL)
                    {                        
                        MessageSend ( &theHeadset.task , EventVolumeMin , 0 );                                      
                    }
                }                
#ifdef ENABLE_AVRCP    
                {
                    uint16 vol_step_change = 0;
                    if (lOldVol > theHeadset.a2dp_link_data->gAvVolumeLevel[index])
                        vol_step_change = lOldVol - theHeadset.a2dp_link_data->gAvVolumeLevel[index];
                    else
                        vol_step_change = theHeadset.a2dp_link_data->gAvVolumeLevel[index] - lOldVol;
                    headsetAvrcpVolumeStepChange(dir, vol_step_change);                
                }
#endif          
				#if 1
				VolumeSetA2dp(index, lOldVol, dir);
				#else
                VolumeSetA2dp(index, lOldVol);
				#endif
                /* volume adjusted for a A2DP media stream */
                return TRUE;
            }
        }
    }
    /* no routed a2dp media streams found */
    return FALSE;
}

/****************************************************************************
NAME 
 VolumeUp

DESCRIPTION
 Increments voulme

RETURNS
 void
    
*/
void VolumeUp ( void )
{
    uint16 lOldVol = 0;
    uint16 lNewVol = 0;

    /* check for a2dp streaming before checking for hpf profiles */
    if(!CheckVolumeA2dp(increase_volume) && theHeadset.conf->no_of_profiles_connected)
    {
        /* Get the link to change volume on */
        hfp_link_priority priority = audioGetLinkPriority(TRUE);
        
        /* get current volume for this profile */
        lOldVol = theHeadset.profile_data[PROFILE_INDEX(priority)].audio.gSMVolumeLevel;
        /* obtain new volume level */
        lNewVol = theHeadset.audioData.gVolMaps[lOldVol].IncVol ;
        /* send, set and store new volume level */

		#if 1
		VolumeSendAndSetHeadsetVolume ( lNewVol ,TRUE , priority, increase_volume) ;
		#else
        VolumeSendAndSetHeadsetVolume ( lNewVol ,TRUE , priority) ;
        #endif
		
       	VOL_DEBUG(("VOL: VolUp[%d][%d] to AG%d\n",lOldVol, lNewVol,priority))  ;
    }
}

/****************************************************************************
NAME 
 VolumeDown

DESCRIPTION
 Decrements volume - sends a response to the AG

RETURNS
 void
    
*/
void VolumeDown ( void )
{
    uint16 lOldVol = 0;
    uint16 lNewVol = 0;
  
    /* check for a2dp streaming before checking for hpf profiles */
    if(!CheckVolumeA2dp(decrease_volume) && theHeadset.conf->no_of_profiles_connected)
    {
        /* Get the link to change volume on */
        hfp_link_priority priority = audioGetLinkPriority(TRUE);

        /* get current volume for this profile */
        lOldVol = theHeadset.profile_data[PROFILE_INDEX(priority)].audio.gSMVolumeLevel;
        /* obtain new volume level */
        lNewVol = theHeadset.audioData.gVolMaps[lOldVol].DecVol ;
        /* send, set and store new volume level */

		#if 1
		VolumeSendAndSetHeadsetVolume ( lNewVol ,TRUE ,priority,decrease_volume) ;
		#else
        VolumeSendAndSetHeadsetVolume ( lNewVol ,TRUE ,priority) ;
        #endif
		
       	VOL_DEBUG(("VOL: VolDwn[%d][%d]to AG%d\n",lOldVol, lNewVol, priority))  ;
    }      
}

/****************************************************************************
NAME 
 VolumeToggleMute

DESCRIPTION
 Toggles the mute state

RETURNS
 void
    
*/
void VolumeToggleMute ( void )
{
    VOL_DEBUG(("VOL: Mute T [%c]\n" , (theHeadset.gMuted ? 'F':'T') )) ;
	
    /*if then old state was muted*/
    if (theHeadset.gMuted )
    {
        MessageSend( &theHeadset.task , EventMuteOff , 0 ) ;
    }
    else
    {
        MessageSend( &theHeadset.task , EventMuteOn , 0 ) ;
    }
}

/****************************************************************************
NAME 
 VolumeMuteOn

DESCRIPTION
 Enables Mute

RETURNS
 void
    
*/
void VolumeMuteOn ( void )
{
    VOL_DEBUG(("VOL: Mute\n")) ;        
    
	/* If headset wants to mute on or receive +VGM = 0 command from one AG, 
	   Send command to both AGs and mute itself 
	*/
	if(theHeadset.features.EnableSyncMuteMicophones)
	{
		VOL_DEBUG(("VOL: Send AT+VGM = 0 to AG1 if AG1 does not require to mute Mic\n")) ;
		if( !theHeadset.profile_data[PROFILE_INDEX(hfp_primary_link)].audio.gAgSyncMuteFlag )
			HfpVolumeSyncMicrophoneGainRequest(hfp_primary_link, 0);
		else
			theHeadset.profile_data[PROFILE_INDEX(hfp_primary_link)].audio.gAgSyncMuteFlag = 0;
			
		/* if AG2 is connected, send a message to AG2 */
   		if((theHeadset.conf->no_of_profiles_connected > 1))
   		{
			VOL_DEBUG(("VOL: Send AT+VGM = 0 to AG2 if AG2 does not require to mute Mic\n")) ;
			if( !theHeadset.profile_data[PROFILE_INDEX(hfp_secondary_link)].audio.gAgSyncMuteFlag )
				HfpVolumeSyncMicrophoneGainRequest(hfp_secondary_link, 0);
			else
				theHeadset.profile_data[PROFILE_INDEX(hfp_secondary_link)].audio.gAgSyncMuteFlag = 0;
		}     
	}

    if (theHeadset.features.MuteSpeakerAndMic)
        AudioSetMode ( AUDIO_MODE_MUTE_BOTH , NULL ) ;
    else
        AudioSetMode ( AUDIO_MODE_MUTE_MIC , NULL) ;

    PioSetMicrophoneBias ( FALSE ) ; 
 
    /*update the mutestate*/    
    theHeadset.gMuted = TRUE ;
	
	if(theHeadset.conf->timeouts.MuteRemindTime_s !=0)
    	MessageSendLater( &theHeadset.task , EventMuteReminder , 0 ,D_SEC(theHeadset.conf->timeouts.MuteRemindTime_s ) ) ;
}

/****************************************************************************
NAME 
 VolumeMuteOff

DESCRIPTION
 Disables Mute

RETURNS
 void
    
*/
void VolumeMuteOff ( void )
{
	/*update the mutestate*/    
    VOL_DEBUG(("VOL: UnMute\n")) ;        
    theHeadset.gMuted = FALSE ;
    
	/* If headset wants to mute off, detect which AG has active sco and sent mute off */
		/* If headset wants to mute on or receive +VGM = 0 command from one AG, 
	   Send command to both AG mute itself 
	*/
	if(theHeadset.features.EnableSyncMuteMicophones)
	{
		VOL_DEBUG(("VOL: Send AT+VGM = 1 to AG1 if AG1 does not require to unmute Mic\n")) ; 
		if( !theHeadset.profile_data[PROFILE_INDEX(hfp_primary_link)].audio.gAgSyncMuteFlag )
            HfpVolumeSyncMicrophoneGainRequest(hfp_primary_link, 0);
		else
			theHeadset.profile_data[PROFILE_INDEX(hfp_primary_link)].audio.gAgSyncMuteFlag = 0;
			
		/* if AG2 is connected, send a message to AG2 */
 		if((theHeadset.conf->no_of_profiles_connected > 1) )
   		{
    		VOL_DEBUG(("VOL: Send AT+VGM = 1 to AG2 if AG2 does not require to unmute Mic\n")) ; 
			if( !theHeadset.profile_data[PROFILE_INDEX(hfp_secondary_link)].audio.gAgSyncMuteFlag )
				HfpVolumeSyncMicrophoneGainRequest(hfp_secondary_link, 0);
			else
				theHeadset.profile_data[PROFILE_INDEX(hfp_secondary_link)].audio.gAgSyncMuteFlag = 0;
		}     
	}

	/*cancel any mute reminders*/
    MessageCancelAll( &theHeadset.task , EventMuteReminder ) ;
    
    AudioSetMode ( AUDIO_MODE_CONNECTED , NULL ) ;  
    
	if (theHeadset.sco_sink)
	{
    	    /*set the mic bias*/
	    PioSetMicrophoneBias ( TRUE ) ;
	}
}



/****************************************************************************
DESCRIPTION
    sets the vol to the level corresponding to the phone volume level
    In addition - send a response to the AG indicating new volume level
    
*/
#if 1
void VolumeSendAndSetHeadsetVolume( uint16 pNewVolume , bool pPlayTone, hfp_link_priority priority, volume_direction dir ) 
{
    bool lOverideMute = theHeadset.features.OverideMute ;
    
    /* ensure profile is connected before changing volume */
    if(priority)
    {
        /*if there is a hfp attached - then send the vol change, but only if not
          muted and mute overide is not in effect*/
        if ( (stateManagerIsConnected() && (!theHeadset.gMuted ))||
             ( lOverideMute ))
        {     
            HfpVolumeSyncSpeakerGainRequest ( priority , (uint8*)&pNewVolume ) ;
        }
        VOL_DEBUG(("VOL: SEND and %x",(unsigned int) priority)) ;

		#if 1
		VolumeSetHeadsetVolume( pNewVolume , pPlayTone, priority, TRUE, dir );
		#else
        VolumeSetHeadsetVolume( pNewVolume , pPlayTone, priority, TRUE );
		#endif
    }
}
#else
void VolumeSendAndSetHeadsetVolume( uint16 pNewVolume , bool pPlayTone, hfp_link_priority priority ) 
{
    bool lOverideMute = theHeadset.features.OverideMute ;
    
    /* ensure profile is connected before changing volume */
    if(priority)
    {
        /*if there is a hfp attached - then send the vol change, but only if not
          muted and mute overide is not in effect*/
        if ( (stateManagerIsConnected() && (!theHeadset.gMuted ))||
             ( lOverideMute ))
        {     
            HfpVolumeSyncSpeakerGainRequest ( priority , (uint8*)&pNewVolume ) ;
        }
        VOL_DEBUG(("VOL: SEND and %x",(unsigned int) priority)) ;

		#if 1
		VolumeSetHeadsetVolume( pNewVolume , pPlayTone, priority, TRUE, dir );
		#else
        VolumeSetHeadsetVolume( pNewVolume , pPlayTone, priority, TRUE );
		#endif
    }
}
#endif


/****************************************************************************
DESCRIPTION
 sets the internal speaker gain to the level corresponding to the phone volume level
 
*/
#if 1
void VolumeSetHeadsetVolume( uint16 pNewVolume , bool pPlayTone, hfp_link_priority priority, bool set_gain, volume_direction dir ) 
{      
    bool lPlayTone = FALSE ;
    bool lOverideMute = theHeadset.features.OverideMute ;
    bool lMuteLocalVolAction = theHeadset.features.MuteLocalVolAction ;

    VOL_DEBUG(("SetVol [%x] [%d][%d][%d]\n " ,pNewVolume, theHeadset.gMuted , lOverideMute , lMuteLocalVolAction)) ;
    
	/* this should only occur if we are not muted or if we are muted but wish to overide */
    if ( (!theHeadset.gMuted ) || ( lOverideMute ) || (lMuteLocalVolAction))
    {
        /*set the local volume only*/
        if ((theHeadset.gMuted) && (!lMuteLocalVolAction))
			MessageSend( &theHeadset.task , EventMuteOff , 0 ) ;
       
        /* the tone needs to be played so set flag */
        lPlayTone = TRUE ;     
        
        /* set new volume */
        theHeadset.profile_data[PROFILE_INDEX(priority)].audio.gSMVolumeLevel = pNewVolume ; 
        
        if(set_gain)
            AudioSetVolume ( theHeadset.audioData.gVolMaps[ pNewVolume ].VolGain , theHeadset.codec_task ) ;
    }
    
    /* ensure there is a valid tone (non zero) to be played */
    if( pPlayTone && lPlayTone && theHeadset.audioData.gVolMaps[pNewVolume].Tone )
    {   /*only attempt to play the tone if it has not yet been played*/

		/*VOL_DEBUG(("VOL: VolTone[%x]\n" , (int)theHeadset.audioData.gVolMaps[pNewVolume].Tone)) ;*/

#if 1
		if(theHeadset.TTS_ASR_Playing == false)
		{
			VOL_DEBUG(("VOL: VolTone[%x]\n" , (int)theHeadset.audioData.gVolMaps[pNewVolume].Tone)) ;
			#ifdef DifferentToneForVolumeButton
			if((stateManagerGetState() == headsetActiveCallSCO || stateManagerGetState() == headsetActiveCallNoSCO) && (theHeadset.sco_sink != 0))
			{
				if(dir)
				{
					TonesPlayTone(0x42 ,theHeadset.features.QueueVolumeTones, FALSE);
				}
				else
				{
	        		TonesPlayTone(theHeadset.audioData.gVolMaps[pNewVolume].Tone ,theHeadset.features.QueueVolumeTones, FALSE);
				}
			}
			#else
			/*TonesPlayTone(theHeadset.audioData.gVolMaps[pNewVolume].Tone ,theHeadset.features.QueueVolumeTones, FALSE);*/
			#endif
		}
#endif
    }    
}

#else
void VolumeSetHeadsetVolume( uint16 pNewVolume , bool pPlayTone, hfp_link_priority priority, bool set_gain ) 
{      
    bool lPlayTone = FALSE ;
    bool lOverideMute = theHeadset.features.OverideMute ;
    bool lMuteLocalVolAction = theHeadset.features.MuteLocalVolAction ;

    VOL_DEBUG(("SetVol [%x] [%d][%d][%d]\n " ,pNewVolume, theHeadset.gMuted , lOverideMute , lMuteLocalVolAction)) ;
    
	/* this should only occur if we are not muted or if we are muted but wish to overide */
    if ( (!theHeadset.gMuted ) || ( lOverideMute ) || (lMuteLocalVolAction))
    {
        /*set the local volume only*/
        if ((theHeadset.gMuted) && (!lMuteLocalVolAction))
			MessageSend( &theHeadset.task , EventMuteOff , 0 ) ;
       
        /* the tone needs to be played so set flag */
        lPlayTone = TRUE ;     
        
        /* set new volume */
        theHeadset.profile_data[PROFILE_INDEX(priority)].audio.gSMVolumeLevel = pNewVolume ; 
        
        if(set_gain)
            AudioSetVolume ( theHeadset.audioData.gVolMaps[ pNewVolume ].VolGain , theHeadset.codec_task ) ;
    }
    
    /* ensure there is a valid tone (non zero) to be played */
    if( pPlayTone && lPlayTone && theHeadset.audioData.gVolMaps[pNewVolume].Tone )
    {   /*only attempt to play the tone if it has not yet been played*/

		/*VOL_DEBUG(("VOL: VolTone[%x]\n" , (int)theHeadset.audioData.gVolMaps[pNewVolume].Tone)) ;*/

#if 1
		if(theHeadset.TTS_ASR_Playing == false)
		{
			VOL_DEBUG(("VOL: VolTone[%x]\n" , (int)theHeadset.audioData.gVolMaps[pNewVolume].Tone)) ;
        	TonesPlayTone(theHeadset.audioData.gVolMaps[pNewVolume].Tone ,theHeadset.features.QueueVolumeTones, FALSE);
		}
#endif
    }    
}
#endif
/****************************************************************************
DESCRIPTION
    sends the current microphone volume to the AG on connection
    
*/
void VolumeSendMicrophoneVolume( uint16 pVolume ) 
{
uint8 i;

    /*if there is a hfp attached - then send the vol change*/
    if ( stateManagerIsConnected() )
    {
        /* scan all available profiles and send mic volume if connected */
        for(i=0;i<MAX_PROFILES;i++)
        {
            /* send mic volume to AG */
            HfpVolumeSyncMicrophoneGainRequest ( (i + hfp_primary_link) , (uint8*)&pVolume ) ;
        }
    }
}

#if 1
/****************************************************************************
DESCRIPTION
    sets the current A2dp volume
    
*/
void VolumeSetA2dp(uint16 index, uint16 oldVolume, volume_direction dir)
{               
    if(theHeadset.audioData.gVolMaps[ theHeadset.a2dp_link_data->gAvVolumeLevel[index] ].A2dpGain == VOLUME_A2DP_MUTE_GAIN)
    {                   
        /* if actual mute enabled, activate it now */
        if(theHeadset.audioData.gVolMaps[ oldVolume ].A2dpGain != VOLUME_A2DP_MUTE_GAIN)
        {
            VOL_DEBUG(("VOL: A2dp mute\n"));
            AudioSetMode(AUDIO_MODE_MUTE_SPEAKER, NULL);
        }
    }                
    else
    {
        VOL_DEBUG(("VOL: A2dp set vol [%d][%d]\n", theHeadset.a2dp_link_data->gAvVolumeLevel[index], theHeadset.audioData.gVolMaps[ theHeadset.a2dp_link_data->gAvVolumeLevel[index] ].A2dpGain - 1));
        AudioSetVolume ( theHeadset.audioData.gVolMaps[ theHeadset.a2dp_link_data->gAvVolumeLevel[index] ].A2dpGain - 1 , theHeadset.codec_task ) ;
    }
    /* set volume level in audio plugin */
    if ((theHeadset.audioData.gVolMaps[ theHeadset.a2dp_link_data->gAvVolumeLevel[index] ].A2dpGain != VOLUME_A2DP_MUTE_GAIN) && (theHeadset.audioData.gVolMaps[ oldVolume ].A2dpGain == VOLUME_A2DP_MUTE_GAIN))
    {
        /* the audio was muted but now should be un-muted as above minimum volume */   
        VOL_DEBUG(("VOL: A2dp unmute\n"));
        AudioSetMode(AUDIO_MODE_CONNECTED, NULL);
    }         
    /* play tone if applicable */
#if 1
    if(theHeadset.audioData.gVolMaps[theHeadset.a2dp_link_data->gAvVolumeLevel[index]].Tone)
    {
    	if(stateManagerGetState() == headsetA2DPStreaming)
		{
			#ifdef DifferentToneForVolumeButton	
				if(dir)
				{
				VOL_DEBUG(("**Vol down tone = 0x42\n"));
	        		TonesPlayTone(0x42 ,theHeadset.features.QueueVolumeTones, FALSE);
				}
				else
				{
				VOL_DEBUG(("**Vol up tone = %d\n",theHeadset.audioData.gVolMaps[theHeadset.a2dp_link_data->gAvVolumeLevel[index]].Tone));
	        		TonesPlayTone(theHeadset.audioData.gVolMaps[theHeadset.a2dp_link_data->gAvVolumeLevel[index]].Tone ,
						theHeadset.features.QueueVolumeTones, FALSE);
				}							
			#else
			/*TonesPlayTone(theHeadset.audioData.gVolMaps[theHeadset.a2dp_link_data->gAvVolumeLevel[index]].Tone ,theHeadset.features.QueueVolumeTones, FALSE);*/
			#endif
    	}
    }
#endif
}
#else
/****************************************************************************
DESCRIPTION
    sets the current A2dp volume
    
*/
void VolumeSetA2dp(uint16 index, uint16 oldVolume)
{               
    if(theHeadset.audioData.gVolMaps[ theHeadset.a2dp_link_data->gAvVolumeLevel[index] ].A2dpGain == VOLUME_A2DP_MUTE_GAIN)
    {                   
        /* if actual mute enabled, activate it now */
        if(theHeadset.audioData.gVolMaps[ oldVolume ].A2dpGain != VOLUME_A2DP_MUTE_GAIN)
        {
            VOL_DEBUG(("VOL: A2dp mute\n"));
            AudioSetMode(AUDIO_MODE_MUTE_SPEAKER, NULL);
        }
    }                
    else
    {
        VOL_DEBUG(("VOL: A2dp set vol [%d][%d]\n", theHeadset.a2dp_link_data->gAvVolumeLevel[index], theHeadset.audioData.gVolMaps[ theHeadset.a2dp_link_data->gAvVolumeLevel[index] ].A2dpGain - 1));
        AudioSetVolume ( theHeadset.audioData.gVolMaps[ theHeadset.a2dp_link_data->gAvVolumeLevel[index] ].A2dpGain - 1 , theHeadset.codec_task ) ;
    }
    /* set volume level in audio plugin */
    if ((theHeadset.audioData.gVolMaps[ theHeadset.a2dp_link_data->gAvVolumeLevel[index] ].A2dpGain != VOLUME_A2DP_MUTE_GAIN) && (theHeadset.audioData.gVolMaps[ oldVolume ].A2dpGain == VOLUME_A2DP_MUTE_GAIN))
    {
        /* the audio was muted but now should be un-muted as above minimum volume */   
        VOL_DEBUG(("VOL: A2dp unmute\n"));
        AudioSetMode(AUDIO_MODE_CONNECTED, NULL);
    }         
    /* play tone if applicable */
#if 1
    if(theHeadset.audioData.gVolMaps[theHeadset.a2dp_link_data->gAvVolumeLevel[index]].Tone)
    {
    	if(stateManagerGetState() == headsetA2DPStreaming)
		{
        	TonesPlayTone(theHeadset.audioData.gVolMaps[theHeadset.a2dp_link_data->gAvVolumeLevel[index]].Tone ,theHeadset.features.QueueVolumeTones, FALSE);
    	}
    }
#endif
}
#endif
