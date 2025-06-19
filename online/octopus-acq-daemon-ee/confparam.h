#ifndef CONFPARAM_H
#define CONFPARAM_H

typedef struct _ConfParam {
 unsigned int ampCount;
 unsigned int tcpBufSize;
 unsigned int sampleRate;
 unsigned int eegProbeMsecs,cmProbeMsecs;
 QString hostIP;
 unsigned int commPort,dataPort;
 int acqGuiX,acqGuiY;
 unsigned int cmCellSize;
} ConfParam;

#endif
