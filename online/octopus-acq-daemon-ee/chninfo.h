#ifndef CHNINFO_H
#define CHNINFO_H

#include "../acqglobals.h"

typedef struct _ChnInfo {
 unsigned int physChn;
 QString chnName;
 float topoTheta,topoPhi;
 unsigned int topoX,topoY;
 float cmLevel[EE_AMPCOUNT]; // 255 is the most noisy
} ChnInfo;

#endif
