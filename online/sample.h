#ifndef _SAMPLE_H
#define _SAMPLE_H

#include "acqglobals.h"

typedef struct _sample {
 float marker;
 float data[PHYS_CHN_COUNT];
 float dataF[PHYS_CHN_COUNT];
 int trigger;
 int offset;
} sample;

#endif
