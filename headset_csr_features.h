/****************************************************************************

FILE NAME
    headset_csr_features.h      

DESCRIPTION
    handles the csr to csr features 

NOTES

*/
/*!

@file	headset_csr_features.h
@brief csr 2 csr specific features    
*/


#ifndef _CSR2CSRFEATURES_

void csr2csrHandleTxtInd(void);
void csr2csrHandleSmsInd(void);   
void csr2csrHandleSmsCfm(void);
void csr2csrHandleAgBatteryRequestInd(void);
void csr2csrHandleAgBatteryRequestRes(headsetEvents_t gas_gauge);
    
#endif
