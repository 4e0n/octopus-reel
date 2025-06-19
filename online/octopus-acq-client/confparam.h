#ifndef CONFPARAM_H
#define CONFPARAM_H

typedef struct _ConfParam {
 unsigned int ampCount;
 unsigned int tcpBufSize;
 unsigned int sampleRate;
 unsigned int eegProbeMsecs;
 unsigned int cmProbeMsecs;
 QString hostIP;
 unsigned int commPort;
 unsigned int dataPort;
 int acqGuiX;
 int acqGuiY;
 unsigned int cmCellSize;
} ConfParam;

#endif
