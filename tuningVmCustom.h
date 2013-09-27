#ifndef _TUNINGVMCUSTOM_H_
#define _TUNINGVMCUSTOM_H_

/*** Parameter Defaults *******************************************************/
/* Set the default system codec values:
 *  Codec Gain Index values range from 0 to 22, where:
 *    Index  ADC Input     DAC Output
 *           Gain (dBr)    Gain (dBr)
 *      0      -27.0         -45.0
 *      1      -23.5         -41.5
 *      2      -21.0         -39.0
 *      3      -17.5         -35.5
 *      4      -15.0         -33.0
 *      5      -11.5         -29.5
 *      6       -9.0         -27.0
 *      7       -5.5         -23.5
 *      8       -3.0         -21.0
 *      9        0.0         -18.0
 *     10        3.0         -15.0
 *     11        6.0         -12.0
 *     12        9.0          -9.0
 *     13       12.0          -6.0
 *     14       15.0          -3.0
 *     15       18.0           0.0
 *     16       21.5           3.5
 *     17       24.0           6.0
 *     18       27.5           9.5
 *     19       30.0          12.0
 *     20       33.5          15.5
 *     21       36.0          18.0
 *     22       39.5          21.5
 *  Note: These values where taken from CSR's Application Note:
 *        "My Second Kalimba DSP Application", June 2007
 */
/* micInCodec Index: */
#define TUNEDCODECMICINGAININDEX    (13)
/* spkrOutCodec Index: */
#define TUNEDCODECSPKROUTGAININDEX  (15)
/* micInCodec preAmp:  On=1, Off=0 */
#define TUNEDCODECMICINPREAMPEN     (1)

/* SoundClear Initialization Values -  */
/* number of microphones:  1 or 2 */
#define TUNEDINITNUMMICS   (2)
/* number of speakers: 1 or 2 */
#define TUNEDINITNUMSPEAKERS   (1)
/* bandwidth sampling frequency:  8000 or 16000 */
#define TUNEDINITBWFS      (8000)

#endif /* _TUNINGVMCUSTOM_H_ */

