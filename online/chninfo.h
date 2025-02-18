#ifndef _CHNINFO_H
#define _CHNINFO_H

typedef struct _chninfo {
 unsigned int sampleRate;
 unsigned int refChnCount;
 unsigned int bipChnCount;
 unsigned int physChnCount;
 unsigned int refChnMaxCount;
 unsigned int bipChnMaxCount;
 unsigned int physChnMaxCount;
 unsigned int totalChnCount;
 unsigned int totalCount; // Chncount among all connected amplifiers
 unsigned int probe_msecs;
} chninfo;

#endif
