#ifndef _TCPSAMPLE_H
#define _TCPSAMPLE_H

#include "../acqglobals.h"

#include "sample.h"

typedef struct _tcpsample {
 sample amp[EE_AMPCOUNT];
 unsigned int trigger;
} tcpsample;

#endif
