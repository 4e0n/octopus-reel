#ifndef CHNINFO_H
#define CHNINFO_H

#include "../acqglobals.h"

typedef struct _ChnInfo {
 unsigned int physChn;
 QString chnName;
 float topoTheta,topoPhi;
} ChnInfo;

#endif
