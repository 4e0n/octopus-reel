#ifndef CHNTOPO_H
#define CHNTOPO_H

typedef struct ChnTopo {
 unsigned int physChn;
 QString chnName;
 unsigned int topoX;
 unsigned int topoY;
 float cmLevel[2]; // 255 is the most noisy
} _ChnTopo;

#endif
