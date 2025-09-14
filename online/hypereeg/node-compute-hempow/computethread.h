/*
Octopus-ReEL - Realtime Encephalography Laboratory Network
   Copyright (C) 2007-2025 Barkin Ilhan

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

#include <QThread>
#include <QString>
#include <QMutex>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <iostream>
#include "confparam.h"
#include "../common/globals.h"
#include "../common/tcpsample.h"

class ComputeThread : public QThread {
 Q_OBJECT
 public:
  ComputeThread(ConfParam *c,QObject *parent=nullptr) : QThread(parent) {
   conf=c;
  }

  void run() override {

   // --- Initial setup ---

   while (true) {
    // Fetch some data

    //mutex.lock();
    //quint64 tcpDataSize=cBufPivot-cBufPivotPrev; //qInfo() << cBufPivotPrev << cBufPivot;
//    for (quint64 tcpDataIdx=0;tcpDataIdx<tcpDataSize;tcpDataIdx++) {
//     // Alignment of each amplifier to the latest offset by +arrivedTrig[i]
//     for (unsigned int ampIdx=0;ampIdx<eeAmps.size();ampIdx++) {
//      tcpEEG.amp[ampIdx]=eeAmps[ampIdx].cBuf[(cBufPivotPrev+tcpDataIdx+arrivedTrig[ampIdx])%cBufSz];
//     }
//     tcpEEG.trigger=0;
//     if (nonHWTrig) { tcpEEG.trigger=nonHWTrig; nonHWTrig=0; } // Set NonHW trigger in hyperEEG data struct
//
//     tcpEEGBuffer[(tcpEEGBufHead+tcpDataIdx)%tcpEEGBufSize]=tcpEEG; // Push to Circular Buffer
//    }
 
//    tcpEEGBufHead+=tcpDataSize; // Update producer index
    //mutex.unlock();

    chnCount=conf->chnCount;

    emit sendData(); // Send computed outcome to any connected clients

   } // EEG stream

   qInfo("node_compute_hempow: <acqthread> Exiting thread..");
  }

  bool popEEGSample(TcpSample *outSample) {
   if (tcpEEGBufTail<tcpEEGBufHead) {
    *outSample=tcpEEGBuffer[tcpEEGBufTail%tcpEEGBufSize]; tcpEEGBufTail++;
    return true;
   } else return false;
  }

 signals:
  void sendData();
 
 private:
  ConfParam *conf;

  TcpSample tcpEEG;
  QVector<TcpSample> tcpEEGBuffer;
  int tcpEEGBufSize;
  quint64 tcpEEGBufHead,tcpEEGBufTail;

  unsigned int cBufSz,smpCount,chnCount;
};
