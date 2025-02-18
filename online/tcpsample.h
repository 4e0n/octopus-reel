#ifndef _TCPSAMPLE_H
#define _TCPSAMPLE_H

#include "sample.h"

typedef struct _tcpsample {
 float marker1;
 sample amp1;
 float marker2;
 sample amp2;
 unsigned int trigger;
} tcpsample;

#endif
