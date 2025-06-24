#ifndef CONFPARAM_H
#define CONFPARAM_H

typedef struct _ConfParam {
 unsigned int ampCount;
 unsigned int tcpBufSize;
 unsigned int sampleRate;
 unsigned int eegProbeMsecs,cmProbeMsecs;
 QString ipAddr;
 unsigned int commPort,dataPort;
 int guiX,guiY;
 unsigned int cmCellSize;
} ConfParam;

#endif
