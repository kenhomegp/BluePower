/****************************************************************************

FILE NAME
    headset_buttons.c        

DESCRIPTION
    This is the button interpreter for BC4-Headset
    This file extracts the button messages from the PIO subsystem and figures out
    the button press type and time. It passes the information to the button manager
    which is responsible for translating the button press generated into a system event
NOTES

*/
#include "headset_private.h"
#include "headset_buttonmanager.h"
#include "headset_buttons.h"
#include "headset_powermanager.h"

#include <charger.h>
#include <csrtypes.h>
#include <panic.h>
#include <stdlib.h>
#include <pio.h>
#include <psu.h>
#include <stddef.h>

#ifdef DEBUG_BUTTONS
#define B_DEBUG(x) DEBUG(x)
#else
#define B_DEBUG(x) 
#endif


#define PIOGET PioGet32


typedef enum ButtonsIntMsgTag 
{
    B_DOUBLE_TIMER , 
    B_INTERNAL_TIMER , 
    B_REPEAT_TIMER  
}ButtonsIntMsg_t;

#define VREG_PIN    (24)
#define CHG_PIN     (25)

    /*the mask values for the charger pins*/
#define VREG_PIN_MASK ((uint32)1 << VREG_PIN)
#define CHG_PIN_MASK ((uint32)1 << CHG_PIN)

    /*the mask values for the charger pin events*/
#define CHARGER_VREG_VALUE ( (uint32)((PsuGetVregEn()) ? VREG_PIN_MASK:0 ) )
#define CHARGER_CONNECT_VALUE ( (uint32)( (ChargerIsChargerConnected())  ? CHG_PIN_MASK:0 ) )

/*
	LOCAL FUNCTION PROTOTYPES
 */
static void ButtonsMessageHandler ( Task pTask, MessageId pId, Message pMessage )   ;
static bool ButtonsWasButtonPressed ( uint32 pOldState , uint32 pNewState) ;
static uint32 ButtonsWhichButtonChanged ( uint32 pOldState , uint32 pNewState ) ;
static void ButtonsButtonDetected (  ButtonsTaskData * pButtonsTask ,uint32 pButtonMask  , ButtonsTime_t pTime  ) ;
static void ButtonsEdgeDetect ( const uint32 pState , ButtonsTaskData * pButtonsTask ) ;       
static void ButtonsLevelDetect ( const uint32 pState , ButtonsTaskData * pButtonsTask )  ;

   
/****************************************************************************
DESCRIPTION
 	Initialises the Button Module parameters
*/  
void ButtonsInit ( ButtonsTaskData *pButtonsTask )
{
    pButtonsTask->task.handler = ButtonsMessageHandler;
    
    /*connect the underlying PIO task to this task*/
    MessagePioTask(&pButtonsTask->task);

    /*connect the underlying Charger task to this task*/
    MessageChargerTask(&pButtonsTask->task);
}


/****************************************************************************
DESCRIPTION
 	Called after the configuration has been read and will trigger buttons events
    if a pio has been pressed or held whilst the configuration was still being loaded
    , i.e. the power on button press    
*/
void ButtonsCheckForChangeAfterInit(void)
{        
    uint32 pio_state;

    ButtonsTaskData * lButtonsTask = theHeadset.theButtonsTask ;
      
    lButtonsTask->gBTime = B_INVALID ; 
    
    {
        /* get 32 bit pio state for BC5 onwards chips */
        pio_state = (((PIOGET() & 0xffff) | CHARGER_VREG_VALUE | CHARGER_CONNECT_VALUE)) ;
        
        /* perform a level detect looking for transistion of recently added button definition, mask pio's 
           against level configured pios to prevent false butotn press indications */
        ButtonsLevelDetect ( (pio_state & lButtonsTask->gPerformLevelCheck) , lButtonsTask ) ;
    
        /* perform an edge detect looking for transistion of recently added button definition, mask pio's 
           against edge configured pios to prevent false button press indications */
        ButtonsEdgeDetect ( (pio_state & lButtonsTask->gPerformEdgeCheck) , lButtonsTask) ;
    }
    
    /* store current set pio state in order to be able to detect the transition of a PIO in the button handler */
    lButtonsTask->gOldPioState = pio_state;
            
    /* Debounce required PIO lines */
    pio_state = PioDebounce32((0xFFFF & lButtonsTask->gButtonLevelMask),  /* mask off upper 16 bits */
                              lButtonsTask->button_config->debounce_number, 
                              lButtonsTask->button_config->debounce_period_ms );    
    
    /* check whether it has been possible to select the use of PIOs specified, if it was not possible
       to use all the PIOs specified due to a PIO being assigned a different function by PSKEY then none 
       of the PIOs will now work so mask out the unavailable PIO and assign the PIOs that are available
       for receiving change notifications */
    if(pio_state)
    {
        /* mask out the unavailable PIOs and register those PIOs that are available for receiving change events */
        PioDebounce32((0xFFFF & lButtonsTask->gButtonLevelMask & ~pio_state),  /* mask off upper 16 bits */
                              lButtonsTask->button_config->debounce_number, 
                              lButtonsTask->button_config->debounce_period_ms );           

        B_DEBUG(("B: **** ERROR **** PIO NOT AVAILABLE = 0x%lx\n",pio_state)) ;

#ifdef DEBUG_BUTTONS
        Panic();
#endif
    }

#ifdef BHC612_
	ChargerDebounce( (CHARGER_VREG_EVENT|CHARGER_CONNECT_EVENT), 4, 250 );
#else
    ChargerDebounce( (CHARGER_VREG_EVENT|CHARGER_CONNECT_EVENT), lButtonsTask->button_config->debounce_number, lButtonsTask->button_config->debounce_period_ms );
#endif    
} 

/****************************************************************************
DESCRIPTION
 	the button event message handler - converts button events to the system events
*/

static void ButtonsMessageHandler ( Task pTask, MessageId pId, Message pMessage ) 
{   
    ButtonsTaskData * lBTask = (ButtonsTaskData*)pTask ;

    B_DEBUG(("B:Message\n")) ;
    switch ( pId )
    {
	    case MESSAGE_PIO_CHANGED : 
        {
            const MessagePioChanged * lMessage = ( const MessagePioChanged * ) (pMessage ) ;
            uint32 lNewPioState = (uint32)( lMessage->state | CHARGER_VREG_VALUE | CHARGER_CONNECT_VALUE) ;
            
            B_DEBUG(("B:BMH - PIO_CHANGE: %x %x\n",lMessage->state16to31, lMessage->state)) ;
            
        
            /* when a pio is configured for an edge detect only there is significant performance gain to be had
               by only doing an edge detect call and not a level detect. To do this use a previously set edge
               detect mask and check this against the current pio being reported. Also need to check if a previously
               set PIO has now been removed and check for the edge transition once again. */
            if((lBTask->gPerformEdgeCheck & lNewPioState) ||
               (lBTask->gPerformEdgeCheck & lBTask->gOldPioState))
            {
                /* check for a valid edge transition against current pio states masked with edge configured pios
                   and perform appropriate action */                
                ButtonsEdgeDetect  ( (uint32)( lNewPioState & lBTask->gPerformEdgeCheck), lBTask ) ;
            }          
            
            /* only do a level detect call which is vm/messaging intensive when a pio has been configured as 
               short or long or very long or very very long, i.e. not rising or falling */
            if((lBTask->gPerformLevelCheck & lNewPioState ) ||
               (lBTask->gPerformLevelCheck & lBTask->gOldPioState))
            {
                /* perform a level detection, this call uses a number of messages and is quite slow to process */
                ButtonsLevelDetect ( ((uint32)lNewPioState & lBTask->gPerformLevelCheck) , lBTask) ;            
            }         
            
            /* store current set pio state in order to be able to detect the transition of a PIO configured as edge 
               detect only */
            lBTask->gOldPioState = lNewPioState ;
        
        }
    	break ;
        
        case MESSAGE_CHARGER_CHANGED:
	    {
		    const MessageChargerChanged *m = (const MessageChargerChanged *) (pMessage ) ;			
         
            /* when a charger or vreg change event is detectecd perform both an edge and level detection
               passing in only those approriately masked pios for edge or level configured buttons */
            ButtonsLevelDetect ( (((uint32)m->vreg_en_high << VREG_PIN) | ((uint32)m->charger_connected << CHG_PIN) | PIOGET()) & lBTask->gPerformLevelCheck , lBTask) ;           
            ButtonsEdgeDetect  ( (((uint32)m->vreg_en_high << VREG_PIN) | ((uint32)m->charger_connected << CHG_PIN) | PIOGET()) & lBTask->gPerformEdgeCheck , lBTask ) ;           
	    }
        break;
   
    	case B_DOUBLE_TIMER:
		{
				/*if we have reached here, then a double timer has been received*/
         	B_DEBUG(("B:Double[%lx][%x]\n", lBTask->gBDoubleState , B_SHORT_SINGLE)) ;
    
         	lBTask->gBDoubleTap = FALSE ;
        		/*indicate that a short button was pressed and it did not become a double press */
        	ButtonsButtonDetected ( lBTask, lBTask->gBDoubleState , B_SHORT_SINGLE ); 
		} 
        break ;
    	case B_INTERNAL_TIMER:
		{
			/*if we have reached here, then the buttons have been held longer than one of the timed messages*/
	        B_DEBUG(("B:Timer\n")) ;
            
            /* an internal timer has triggered which was initiated from the level detect function call */
        	if ( lBTask->gBTime == B_VERY_LONG )
        	{
                /* update timer state flag */
                lBTask->gBTime = B_VERY_VERY_LONG ;                
        	}
			/* a long press timer event has triggered */
            else if ( lBTask->gBTime == B_LONG )
        	{
                /* don't send very very long timer message until needed, i.e. very_long timer expired */
                MessageSendLater ( &lBTask->task , B_INTERNAL_TIMER , 0 ,  (lBTask->button_config->very_very_long_press_time - lBTask->button_config->very_long_press_time ) ) ;                   
                /* update tiemr state flag */
            	lBTask->gBTime = B_VERY_LONG ;
               	/*notify the app that the timer has expired*/
            	MessageSend( &theHeadset.task , EventVLongTimer , 0 ) ;    
        	}
			/* the first timer event triggered from the level detect call */
            else
        	{
                /* only send very long message when long timer expired to save messaging.                 */
                MessageSendLater ( &lBTask->task , B_INTERNAL_TIMER , 0 , (lBTask->button_config->very_long_press_time - lBTask->button_config->long_press_time)) ;
               	/*notify the app that the timer has expired*/
            	MessageSend( &theHeadset.task , EventLongTimer , 0 ) ;    
           		lBTask->gBTime = B_LONG ;
        	}    
            	/*indicate that we have received a message */
        	ButtonsButtonDetected ( lBTask, lBTask->gBOldState , lBTask->gBTime ); 
		}         
        break ;
    	case B_REPEAT_TIMER:
		{
			/*if we have reached here, the repeat time has been reached so send a new message*/
        	B_DEBUG(("B:Repeat[%lx][%x]\n", lBTask->gBOldState , B_REPEAT  )) ;
        
        	/*send another repeat message*/
        	MessageSendLater ( &lBTask->task , B_REPEAT_TIMER , 0 ,  lBTask->button_config->repeat_time ) ; 

        	ButtonsButtonDetected ( lBTask, lBTask->gBOldState , B_REPEAT ); 
		}
        break;
    	default :
           B_DEBUG(("B:?[%x]\n",pId)) ; 
        break ;
    }
}
/****************************************************************************
DESCRIPTION
 	helper method - returns true if a button was pressed
*/  
static bool ButtonsWasButtonPressed ( uint32 pOldState , uint32 pNewState)
{
    bool lWasButtonPressed = FALSE ;
 
    uint32 lButton = ButtonsWhichButtonChanged ( pOldState , pNewState ) ;
    
    if ( ( lButton & pNewState ) != 0 )
    {
        lWasButtonPressed = TRUE ;
    }
    
    return lWasButtonPressed ;
}
/****************************************************************************
DESCRIPTION
 	helper method - returns mask ofwhich button changed
*/ 
static uint32 ButtonsWhichButtonChanged ( uint32 pOldState , uint32 pNewState )
{
    uint32 lWhichButton = 0 ;
        
    lWhichButton = (pNewState ^ pOldState ) ;
	
	return lWhichButton ;
}

/****************************************************************************  
DESCRIPTION
 	function to handle a button press - informs button manager of a change
    currently makes direct call - may want to use message handling (tasks)
*/ 
static void ButtonsButtonDetected (  ButtonsTaskData * pButtonsTask, uint32 pButtonMask  , ButtonsTime_t pTime )
{
    B_DEBUG(("B:But Det[%lx]\n", pButtonMask)) ;
    
    if( pButtonMask == 0 )
    {
     	MessageCancelAll ( &pButtonsTask->task , B_REPEAT_TIMER ) ;
    }
    else
    {
        BMButtonDetected ( pButtonMask, pTime ) ;             
    } 
}

/****************************************************************************
DESCRIPTION
 	function to detect level changes of buttons / multiple buttons. 
*/ 
static void ButtonsLevelDetect ( const uint32 pState , ButtonsTaskData * pButtonsTask ) 
{
    uint32 lNewState = 0;

    	/*we have an indication from the PIO subsytem that a PIO has changed value*/
    lNewState = (uint32) (pState & (pButtonsTask->gPerformLevelCheck) ) ;
    
    B_DEBUG(("But Lev Det|:[%lx][%lx]\n", pState , (pButtonsTask->gButtonLevelMask) )) ;
    B_DEBUG(("But Lev Det|:[%lx][%lx]\n", pButtonsTask->gBOldState , lNewState )) ;
    
    if ( ButtonsWasButtonPressed( pButtonsTask->gBOldState , lNewState )  )
    {
        /*cancel all previously timed messages*/   
        MessageCancelAll ( &pButtonsTask->task , B_INTERNAL_TIMER ) ;
        MessageCancelAll ( &pButtonsTask->task , B_REPEAT_TIMER ) ;
        MessageCancelAll ( &pButtonsTask->task , B_DOUBLE_TIMER ) ;           
        
        /* send new timed messages*/
        MessageSendLater ( &pButtonsTask->task , B_INTERNAL_TIMER , 0 ,  pButtonsTask->button_config->long_press_time ) ; 
        MessageSendLater ( &pButtonsTask->task , B_REPEAT_TIMER   , 0 ,  pButtonsTask->button_config->repeat_time ) ; 	
        
        /*having restrted the timers, reset the time*/
        pButtonsTask->gBTime = B_SHORT ;      
    }
    /*button was released or was masked out, check to make sure there is a pio bit change as vreg enable
      can generate an addition MSG without any pio's changing state */
    else if(pButtonsTask->gBOldState != lNewState )              
    {         /*it was only a released if there was a button actually pressed last time around - 
              buttons we have masked out still end up here but no state changes are made  */       
        if ( pButtonsTask->gBOldState != 0 )
        {
                 /*if we have had a double press in the required time 
                 and the button pressed was the same as this one*/
             if (  (pButtonsTask->gBDoubleTap ) && (pButtonsTask->gBOldState == pButtonsTask->gBDoubleState ) )
             {
                 pButtonsTask->gBTime = B_DOUBLE ;
                    /*reset the double state*/
                 pButtonsTask->gBDoubleState = 0x0000 ;
                 pButtonsTask->gBDoubleTap = FALSE ;
                 ButtonsButtonDetected ( pButtonsTask , pButtonsTask->gBOldState , B_DOUBLE  ); 
             }                    

             
                /*only send a message if it was a short one - long / v long /double handled elsewhere*/
             if ( (pButtonsTask->gBTime == B_SHORT ) )
             {
                 ButtonsButtonDetected ( pButtonsTask , pButtonsTask->gBOldState , B_SHORT  ); 
                 
                 /*store the double state*/
                 pButtonsTask->gBDoubleState = pButtonsTask->gBOldState ;
                 pButtonsTask->gBDoubleTap = TRUE ;
        
                    /*start the double timer - only applicable to a short press*/
                 MessageSendLater ( &pButtonsTask->task , B_DOUBLE_TIMER , 0 ,pButtonsTask->button_config->double_press_time ) ;
             }  
             else if ( (pButtonsTask->gBTime == B_LONG) )
             {
                 ButtonsButtonDetected ( pButtonsTask , pButtonsTask->gBOldState , B_LONG_RELEASE  );                  
             }
             else if ( (pButtonsTask->gBTime == B_VERY_LONG) )
             {
                 ButtonsButtonDetected ( pButtonsTask , pButtonsTask->gBOldState , B_VERY_LONG_RELEASE  ); 
             }
             else if ( (pButtonsTask->gBTime == B_VERY_VERY_LONG) )
             {
                 ButtonsButtonDetected ( pButtonsTask , pButtonsTask->gBOldState , B_VERY_VERY_LONG_RELEASE  ); 
             }
             
             if (pButtonsTask->gBTime != B_INVALID)
             {
                MessageCancelAll ( &pButtonsTask->task , B_INTERNAL_TIMER) ;
                MessageCancelAll ( &pButtonsTask->task , B_REPEAT_TIMER ) ; 
             }    
             
			 /*removing this allows all releases to generate combination presses is this right?*/
             if ( !lNewState )
             {
                 pButtonsTask->gBTime = B_INVALID ;      
             } 
         }     
    }
    pButtonsTask->gBOldState = lNewState ;
}
  


/****************************************************************************

DESCRIPTION
 	function to detect edge changes of buttons / multiple buttons. 

*/ 
static void ButtonsEdgeDetect ( const uint32 pState , ButtonsTaskData * pButtonsTask ) 
{
    uint32 lNewState = 0x0000 ;
    
    uint32 lButton = 0x0000 ;
    
    /*what has changed */
    lNewState = (uint32) ( pState & (pButtonsTask->gPerformEdgeCheck) ) ;
    
    lButton = ButtonsWhichButtonChanged( pButtonsTask->gBOldEdgeState , lNewState ) ;
    
    
    B_DEBUG(("But Edge Det: [%lx][%lx][%lx]\n", pButtonsTask->gBOldEdgeState , lNewState , pButtonsTask->gButtonLevelMask  )) ;
    B_DEBUG(("But Edge Det: [%lx][%lx][%lx]\n", lNewState , lButton , (lNewState & lButton) )) ;
    
        /*if a button has changed*/
    if ( lButton )
    {
            /*determine which edge has been received and process accordingly*/
        if ( lNewState & lButton )
        {  
            ButtonsButtonDetected ( pButtonsTask , lButton , B_LOW_TO_HIGH )   ;
        }
        else
        {
            ButtonsButtonDetected ( pButtonsTask , lButton , B_HIGH_TO_LOW  )   ;
        }
    }
        /*remember the last state*/
    pButtonsTask->gBOldEdgeState = lNewState;
}



