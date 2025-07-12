#ifndef CONFPARAM_H
#define CONFPARAM_H

typedef struct _ConfParam {
 unsigned int ampCount;
 unsigned int tcpBufSize;
 unsigned int sampleRate;
 float refGain,bipGain;
 unsigned int eegProbeMsecs;
 QString ipAddr;
 unsigned int commPort,dataPort;
 unsigned int refChnCount,bipChnCount;
} ConfParam;

#endif
