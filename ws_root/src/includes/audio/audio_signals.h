#ifndef __AUDIO_AUDIO_SIGNALS_H
#define __AUDIO_AUDIO_SIGNALS_H


/****************************************************************************
**
**  @(#) audio_signals.h 96/03/13 1.5
**
**  Audio Signal Types for Audio and Beep Folio
**
****************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif


/*****************************************************************************/

/* Signal Types */
typedef enum AudioSignalType {
    AUDIO_SIGNAL_TYPE_GENERIC_SIGNED,   /*  Signed generic signal (e.g. amplitude)
                                            range    : -1.0 <= x < 1.0
                                            precision: 1/32768 (approx 3.05E-5)
                                        */
    AUDIO_SIGNAL_TYPE_GENERIC_UNSIGNED, /*  Unsigned generic signal
                                            range    : 0.0 <= x < 2.0
                                            precision: 1/32768 (approx 3.05E-5)
                                        */
    AUDIO_SIGNAL_TYPE_OSC_FREQ,         /*  Oscillator frequency in Hz
                                            range    : -22050.0 Hz <= x < 22050.0 Hz *
                                            precision: 22050/32768 (approx 0.673) Hz *
                                        */
    AUDIO_SIGNAL_TYPE_LFO_FREQ,         /*  LFO frequency in Hz
                                            range    : -22050/256 Hz <= x < 22050/256 (approx 86.1) Hz *
                                            precision: 22050/(256*32768) (approx 2.63E-3) Hz *
                                        */
    AUDIO_SIGNAL_TYPE_SAMPLE_RATE,      /*  Sample rate in samples/second
                                            range    : 0.0 <= x < 88200.0 samples/second *
                                            precision: 44100/32768 (approx 1.35) samples/second *
                                        */
    AUDIO_SIGNAL_TYPE_WHOLE_NUMBER      /*  Whole numbers
                                            range    : -32768.0 <= x <= 32767.0
                                            precision: 1.0
                                        */
                                        /*  * Actual frequency ranges and precision
                                              are determined by DAC sample rate. As
                                              presented, the DAC rate is assumed to
                                              be 44100 samples/second.
                                        */
#ifndef EXTERNAL_RELEASE
    , AUDIO_SIGNAL_TYPE_MANY            /* # of signal types defined here (private) */
#define AUDIO_SIGNAL_TYPE_MAX (AUDIO_SIGNAL_TYPE_MANY-1)
#endif
} AudioSignalType;


/*****************************************************************************/
/* Other miscellaneous definitions shared by audio/audio.h and beep/beep.h */

/* Audio AIFC compression types. */
#define ID_SQD2 MAKE_ID('S','Q','D','2')    /* Opera software 2:1 */
#define ID_SQS2 MAKE_ID('S','Q','S','2')    /* M2 hardware 2:1 */
#define ID_CBD2 MAKE_ID('C','B','D','2')    /* M2 software Cubic Delta compression 2:1 */
#define ID_ADP4 MAKE_ID('A','D','P','4')    /* Intel/DVI 4:1 ADPCM */


/*****************************************************************************/


#endif /* __AUDIO_AUDIO_SIGNALS_H */
