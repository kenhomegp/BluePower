/****************************************************************************

FILE NAME
    headset_energy_filter.h
    
DESCRIPTION
    Basic demonstration of the capabilities to set dynamically the IIR filter coefficients
	based on the current energy estimation range
*/
#ifdef ENABLE_ENERGY_FILTER		

/*!
@file	headset_energy_filter.h
@brief	
*/
#ifndef _HEADSET_ENERGY_FILTER_H_
#define _HEADSET_ENERGY_FILTER_H_

/*!
 	@brief The energy filter enabler 
 */
	
/*! [Configuration]

This module is completely configurable through PSKeys USR28.

FILTER COEFFICIENTS
	This coefficients have been calculated with the Matlab script "IIR_filters.m" with the following parameters:
 	\code
		Sample Rate     : 8000Hz
		Type            : Butterworth
		Response        : High Pass
		Cutoff Frequency: 300Hz
		Gain            : 1
	\endcode
	
	The IIR coefficients have the following functionalities 
	See: "BlueCore5-Multimedia External Datasheet" - Integrated Digital Filter
	\code
		0: Gain
		1: b01
		2: b02
		3: a01
		4: a02
		5: b11
		6: b12
		7: a11
		8: a12
		9: DC Block (1 = enable, 0 = disable)
	\endcode
	
	{0x02F0, 0x0800, 0x0400, 0x099A, 0x0295, 0x0800, 0x0400, 0x08DC, 0x0358, 0x0000}
    		
VAD THRESHOLD
	The value of "energy estimation" above which we trigger a voice activity
	
ECHO REDUCTION GAIN
	The Gain value of the filter to be used in case of incoming voice activity to reduce echo
*/

/*!
	@brief Switch the filter on
*/
void Filter_On(void);
		
/*!
	@brief Switch the filter off
*/
void Filter_Off(void);
	

/*!
	@brief Handle the MESSAGE_ENERGY_CHANGED received in the AV headset application main task
	
	@param message The MESSAGE_ENERGY_CHANGED
*/	
void Filter_EnergyChanged(Message message);

#endif

#endif

