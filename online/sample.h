#ifndef _SAMPLE_H
#define _SAMPLE_H

const int SAMPLE_RATE=1000;
const int REF_CHN_COUNT=0; // 64
const int BIP_CHN_COUNT=1; // 2

const int REF_CHN_MAXCOUNT=64;
const int BIP_CHN_MAXCOUNT=24;
const int PHYS_CHN_COUNT=REF_CHN_COUNT+BIP_CHN_COUNT;
const int TOTAL_CHN_COUNT=PHYS_CHN_COUNT+2;

typedef struct _sample {
 float data[PHYS_CHN_COUNT];
 float dataF[PHYS_CHN_COUNT];
 int trigger;
 int offset;
} sample;

#endif
