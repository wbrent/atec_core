/*
 BEGIN_JUCE_MODULE_DECLARATION

   ID:               atec_core
   vendor:           atec
   version:          0.0.3
   name:             Simple utility classes for DSP
   description:      Simple utility classes for DSP
   dependencies:       juce_dsp

 END_JUCE_MODULE_DECLARATION
*/

#pragma once
#define ATEC_CORE_H_INCLUDED

#include <juce_dsp/juce_dsp.h>

#include "lfo/atec_LFO.h"
#include "buffering/atec_OlaBufferStereo.h"
#include "buffering/atec_RingBuffer.h"
#include "utilities/atec_Utilities.h"
