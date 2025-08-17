#ifndef CONFPARAM_H
#define CONFPARAM_H

typedef struct _ConfParam {
 //unsigned int ampCount;
 //unsigned int tcpBufSize;
 //unsigned int sampleRate;
 //unsigned int eegProbeMsecs,cmProbeMsecs;
 unsigned int cntPastSize;
 int rejBwd,avgBwd,avgFwd,rejFwd;
 int guiCtrlX,guiCtrlY,guiCtrlW,guiCtrlH;
 int guiStrmX,guiStrmY,guiStrmW,guiStrmH;
 int guiHeadX,guiHeadY,guiHeadW,guiHeadH;
 QString ipAddr;
 unsigned int commPort,dataPort;
 int guiX,guiY;
 //unsigned int cmCellSize;
} ConfParam;

#endif
