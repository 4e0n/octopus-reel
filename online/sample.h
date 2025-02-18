#ifndef _SAMPLE_H
#define _SAMPLE_H

const int refChnCount=64;
const int bipChnCount=2;
const int physChnCount=refChnCount+bipChnCount;
const int totalChnCount=physChnCount+2;

typedef struct _sample {
 float data[physChnCount];
 float dataF[physChnCount];
 int trigger;
 int offset;
} sample;

#endif
