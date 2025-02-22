#ifndef _EEX_H
#define _EEX_H

//#define EEMAGINE
#include "../acqglobals.h"

#ifdef EEMAGINE
using namespace eemagine::sdk;
#else
using namespace eesynth;
#endif

typedef struct _eex {
 amplifier *amp;
 std::vector<channel> chnList;
 stream *str;
 buffer buf;
 unsigned int t;
 unsigned int smpCount; // data count int buffer
 //unsigned int chnCount; // channel count int buffer -- redundant, to be asserted
 unsigned int absSmpIdx; // Absolute Sample Index, as sent from the amplifier
 std::vector<float> imps;
 quint64 cBufIdx,cBufIdxP;
 std::vector<sample> cBuf;
 std::vector<sample> cBufF;
} eex;

#endif
