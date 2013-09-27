#ifndef _SOUNDCLEARPLUGIN_H_
#define _SOUNDCLEARPLUGIN_H_

/* CSR SDK Include Files */
#include <message.h>

/******************************************************************************
 * Globals
 *****************************************************************************/
extern const TaskData soundclearPlugin ;

/******************************************************************************
 * Public Function Prototypes
 *****************************************************************************/
void soundClearPluginSetCodecConfig(uint16 codecPskey);
void soundClearPluginSetScConfig(uint16 scPskey);
void soundClearPluginToggleScActive(uint16 scEnable);
#endif


