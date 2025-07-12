#ifndef CHNINFO_H
#define CHNINFO_H

//#include "../hacqglobals.h"

typedef struct _ChnInfo {
 unsigned int physChn;
 QString chnName;
 float topoTheta,topoPhi;
 unsigned int topoX,topoY;
 bool isBipolar;
} ChnInfo;

#endif
