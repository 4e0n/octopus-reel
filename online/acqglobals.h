#ifndef GLOBALS
#define GLOBALS

//#define EEMAGINE

const unsigned int EE_AMPCOUNT=2;
const unsigned int EE_MAX_AMPCOUNT=8;

const int SAMPLE_RATE=1000;

const int REF_CHN_COUNT=0;
const int REF_CHN_MAXCOUNT=64;

const int BIP_CHN_COUNT=1; // 2
const int BIP_CHN_MAXCOUNT=24;

const int PHYS_CHN_COUNT=REF_CHN_COUNT+BIP_CHN_COUNT;
const int TOTAL_CHN_COUNT=PHYS_CHN_COUNT+2;

const float EE_REF_GAIN=1.;
const float EE_BIP_GAIN=4.;

const unsigned int CBUF_SIZE_IN_SECS=10;

#endif
