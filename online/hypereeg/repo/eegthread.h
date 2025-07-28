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

#ifndef EEGTHREAD_H
#define EEGTHREAD_H

#include <QThread>
#include <QMutex>
#include <QBrush>
#include <QPainter>
#include <QStaticText>
#include <QVector>
#include <thread>
#include "confparam.h"
#include "audioframe.h"
#include "../sample.h"

class EEGThread : public QThread {
 Q_OBJECT
 public:
  explicit EEGThread(ConfParam *c=nullptr,unsigned int a=0,
                     QObject *parent=nullptr) : QThread(parent) {
   conf=c; ampNo=a; threadActive=true;

   chnCount=conf->chns.size(); scrAvailSmp=conf->scrAvailableSamples;
   tcpBuffer=&conf->tcpBuffer; tcpBufSize=conf->tcpBufSize;
  }

 signals:
  void updateEEGFrame();

 protected:
  virtual void run() override {
  
   //while (true) {
   while (threadActive) { // && !QThread::currentThread()->isInterruptionRequested()) {
    {
     QMutexLocker locker(&conf->mutex);
     // Sleep until producer signals
     while (!conf->scrollPending[ampNo]) { conf->scrollWait.wait(&conf->mutex); }

     if (conf->quitPending) {
      threadActive=false; //qDebug() << "Thread got the finish signal:" << ampNo;
     } else {
      unsigned int scrUpdateSmp=conf->scrUpdateSamples; unsigned int scrUpdateCount=scrAvailSmp/scrUpdateSmp;
      for (unsigned int smpIdx=0;smpIdx<scrUpdateCount-1;smpIdx++) {
       mean((smpIdx+0)*scrUpdateSmp,&(conf->s0[smpIdx]),&(conf->s0s[smpIdx]));
       mean((smpIdx+1)*scrUpdateSmp,&(conf->s1[smpIdx]),&(conf->s1s[smpIdx]));
      }
      emit updateEEGFrame();
      //qDebug() << ampNo << "updated.";
     }
     conf->scrollPending[ampNo]=false;
     conf->scrollersUpdating--; // semaphore.. kind of..  
     // If this was the last scroller, then advance tail..
     if (conf->scrollersUpdating==0) conf->tcpBufTail+=conf->scrAvailableSamples;
    }
   }
   qDebug("octopus_hacq_client: <EEGThread> Exiting thread..");
  }

 private:
  void mean(unsigned int startIdx,std::vector<float> *mu,std::vector<float> *sigma) {
   unsigned int tcpBufTail=conf->tcpBufTail; unsigned int scrUpdateSmp=conf->scrUpdateSamples;
   const float invSmp=1.0f/static_cast<float>(scrUpdateSmp);
   if (conf->notchActive) {
    for (unsigned int chnIndex=0;chnIndex<chnCount;chnIndex++) {
     float sum=0.0f,sumSq=0.0f;
     for (unsigned int smpIndex=0;smpIndex<scrUpdateSmp;smpIndex++) {
      float data=(*tcpBuffer)[(tcpBufTail+smpIndex+startIdx)%tcpBufSize].amp[ampNo].dataF[chnIndex];
      sum+=data; sumSq+=data*data;
     }
     float m=sum*invSmp;
     (*mu)[chnIndex]=m; (*sigma)[chnIndex]=std::sqrt(sumSq*invSmp-m*m);
    }
   } else {
    for (unsigned int chnIndex=0;chnIndex<chnCount;chnIndex++) {
     float sum=0.0f,sumSq=0.0f;
     for (unsigned int smpIndex=0;smpIndex<scrUpdateSmp;smpIndex++) {
      float data=(*tcpBuffer)[(tcpBufTail+smpIndex+startIdx)%tcpBufSize].amp[ampNo].data[chnIndex];
      sum+=data; sumSq+=data*data;
     }
     float m=sum*invSmp;
     (*mu)[chnIndex]=m; (*sigma)[chnIndex]=std::sqrt(sumSq*invSmp-m*m);
    }
   }
  }

  ConfParam *conf; unsigned int ampNo; bool threadActive;
  QVector<TcpSample> *tcpBuffer; unsigned int tcpBufSize,chnCount,scrAvailSmp;
};

#endif
