/*
Octopus-ReEL - Realtime Encephalography Laboratory Network
   Copyright (C) 2007-2026 Barkin Ilhan

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <https://www.gnu.org/licenses/>.

 Contact info:
 E-Mail:  barkin@unrlabs.org
 Website: http://icon.unrlabs.org/staff/barkin/
 Repo:    https://github.com/4e0n/
*/

#pragma once

#include <QFile>
#include <QDataStream>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMutex>
#include <vector>
#include "acqchninfo.h"
#include "../common/tcpsample.h"

struct ConfParam {
 ConfParam() {}
/*
 void generateElecLists() {
  {
   QMutexLocker locker(&chnInterMutex);
   // Truncate the lists
   //qDebug() << interList.size() << offList.size();
   for (int ampIdx=0;ampIdx<interList.size();ampIdx++) interList[ampIdx].resize(0);
   for (int ampIdx=0;ampIdx<offList.size();ampIdx++) offList[ampIdx].resize(0);
   for (int chnIdx=0;chnIdx<chnInfo.size();chnIdx++) {
    if (chnInfo[chnIdx].type==0) {
     for (int ampIdx=0;ampIdx<chnInfo[chnIdx].interMode.size();ampIdx++) {
      int m=chnInfo[chnIdx].interMode[ampIdx];
//      qDebug() << m << chnIdx << chnInfo[chnIdx].physChn;
      if (m==0) offList[ampIdx].append(chnIdx);
      else if (m==2) interList[ampIdx].append(chnIdx);
     }
    }
   }
  }
//  for (int chnIdx=0;chnIdx<chnInfo.size();chnIdx++) {
//   if (chnInfo[chnIdx].type==0) {
//    printf("%d %d ",chnInfo[chnIdx].interMode[0],chnInfo[chnIdx].interMode[1]);
//   } else {
//    printf("- - ");
//   }
//  }
//  printf("\n");
  qDebug() << "Electrode interpolation and switch-off lists are (re)generated.";
 }
*/
 unsigned int ampCount,eegRate,tcpBufSize,eegProbeMsecs;
 float refGain,bipGain;
 QString acqIpAddr; unsigned int acqCommPort,acqStrmPort;
 unsigned int refChnCount,bipChnCount,physChnCount,metaChnCount; // refChnCount+bipChnCount
 unsigned int refChnMaxCount,bipChnMaxCount,physChnMaxCount;
 unsigned int totalChnCount; // refChnCount+bipChnCount+2
 unsigned int totalCount; // Chncount among all connected amplifiers, i.e. [ampCount x totalChnCount]
 unsigned int eegSamplesInTick;
 QFile hEEGFile; QDataStream hEEGStream;
 bool dumpRaw,syncOngoing,syncPerformed;
 QVector<QTcpSocket*> strmClients;
 QMutex mutex,chnInterMutex;
 quint64 tcpBufHead,tcpBufTail;
 QVector<TcpSample> tcpBuffer;
 QVector<AcqChnInfo> chnInfo;
// QVector<QVector<unsigned int>> interList,offList; // List of interpolated and switched-off electrodes for all amps
 QTcpServer commServer,strmServer;
};
