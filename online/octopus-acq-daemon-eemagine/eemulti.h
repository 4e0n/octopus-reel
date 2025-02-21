#ifndef _EEMULTI_H
#define _EEMULTI_H

#define EEMAGINE

typedef struct _eemulti {
#ifdef EEMAGINE
 eemagine::sdk::amplifier *amp;
 std::vector<eemagine::sdk::channel> chnList;
 eemagine::sdk::stream *stream;
 eemagine::sdk::buffer buffer;
#else
 unsigned int t;
#endif
 unsigned int smpCount; // data count int buffer
 //unsigned int chnCount; // channel count int buffer -- redundant, to be asserted
 unsigned int absSmpIdx; // Absolute Sample Index, as sent from the amplifier
 std::vector<float> imps;
 quint64 cBufIdx,cBufIdxP;
 std::vector<sample> cBuf;
 std::vector<sample> cBufF;
} eemulti;

#endif
