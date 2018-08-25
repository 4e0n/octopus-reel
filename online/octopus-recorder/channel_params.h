
// These are the global parameters valid for all channels..

#ifndef CHANNEL_PARAMS_H
#define CHANNEL_PARAMS_H

#include <QVector>

typedef struct _channel_params {
 int rejBwd,avgBwd,avgFwd,rejFwd, // In terms of milliseconds
     cntPastSize,cntPastIndex,avgCount,rejCount,postRejCount,bwCount;
} channel_params;

#endif
