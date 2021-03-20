/*
 BEGIN_JUCE_MODULE_DECLARATION

   ID:               atec_core
   vendor:           atec
   version:          0.0.5
   name:             Simple utility classes for DSP
   description:      Simple utility classes for DSP
   dependencies:     juce_dsp

 END_JUCE_MODULE_DECLARATION
*/

#ifdef ATEC_CORE_H_INCLUDED
 /* When you add this cpp file to your project, you mustn't include it in a file where you've
    already included any other headers - just put it inside a file on its own, possibly with your config
    flags preceding it, but don't include anything else. That also includes avoiding any automatic prefix
    header files that the compiler may be using.
 */
 #error "Incorrect use of ATEC_CORE cpp file"
#endif

#include "atec_core.h"

#include "lfo/atec_LFO.cpp"
#include "buffering/atec_OlaBufferStereo.cpp"
#include "buffering/atec_RingBuffer.cpp"
#include "utilities/atec_Utilities.cpp"
