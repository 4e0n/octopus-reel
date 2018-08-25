/*      Octopus - Bioelectromagnetic Source Localization System 0.9.5       *\
 *                     (>) GPL 2007-2009 Barkin Ilhan                       *
 *      Hacettepe University, Medical Faculty, Biophysics Department        *
\*                barkin@turk.net, barkin@hacettepe.edu.tr                  */

// These are the global parameters valid for all channels..

#ifndef CHANNEL_PARAMS_H
#define CHANNEL_PARAMS_H

#include <QVector>

typedef struct _channel_params {
 int rejBwd,avgBwd,avgFwd,rejFwd,cntPastSize,cntPastIndex,
     avgCount,rejCount,postRejCount,bwCount; // All in terms of milliseconds..
} channel_params;

#endif
