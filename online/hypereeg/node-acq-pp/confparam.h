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

#include <QColor>
#include <QTcpSocket>
#include <QMutex>
#include <QWaitCondition>
#include <QThread>
#include <atomic>
#include <QFile>
#include <QDataStream>
#include <QVector>
#include <QDateTime>
#include <QTcpServer>
#include <QQueue>
#include "ppchninfo.h"
#include "../common/tcpsample.h"
#include "../common/tcpsample_pp.h"
#include "iirfilter.h"

// Notch@50Hz (Q=30)
const std::vector<double> b_notch={0.99479124,-1.89220538,0.99479124};
const std::vector<double> a_notch={1.        ,-1.89220538,0.98958248};

// [0.1-100Hz] Band-pass
//const std::vector<double> b_01_100={0.06734199, 0.         ,-0.13468398, 0.        ,0.06734199};
//const std::vector<double> a_01_100={1.        ,-3.14315078, 3.69970174,-1.96971135,0.41316049};

// [1-40Hz] Band-pass
//const std::vector<double> b_1_40={0.01274959, 0.       ,-0.02549918, 0.        ,0.01274959};
//const std::vector<double> a_1_40={1.        ,-3.6532502, 5.01411673,-3.06801391,0.7071495 };

// [2-40Hz] Band-pass
const std::vector<double> b_2_40={0.01215196, 0.        ,-0.02430392, 0.        ,0.01215196};
const std::vector<double> a_2_40={1.        ,-3.65903703, 5.03244992,-3.08686268,0.71345829};

// Delta (2-4)
//const std::vector<double> b_delta={3.91302054e-05, 0.00000000e+00,-7.82604108e-05, 0.00000000e+00,3.91302054e-05};
//const std::vector<double> a_delta={1.            ,-3.98160009    , 5.94559129    ,-3.94637655    ,0.98238545};

// Theta (4-8)
//const std::vector<double> b_theta={0.00015515, 0.        ,-0.0003103 , 0.      ,0.00015515};
//const std::vector<double> a_theta={1.        ,-3.96195654, 5.88903994,-3.892163,0.96508117};

// Alpha (8-14)
//const std::vector<double> b_alpha={0.00034604, 0.       ,-0.00069208, 0.        ,0.00034604};
//const std::vector<double> a_alpha={1.        ,-3.9379744, 5.82427903,-3.83436731,0.94808171};

// Beta (14-28)
//const std::vector<double> b_beta={0.00182013, 0.        ,-0.00364026, 0.        ,0.00182013};
//const std::vector<double> a_beta={1.        ,-3.84577528, 5.57661047,-3.61363652,0.88302609};

// Gamma (28-40)
//const std::vector<double> b_gamma={0.00134871, 0.        ,-0.00269742, 0.        ,0.00134871};
//const std::vector<double> a_gamma={1.        ,-3.80766385, 5.52048605,-3.60983953,0.89885899};

class ConfParam : public QObject {
 Q_OBJECT
 public:
  ConfParam() { tcpBufHead=tcpBufTail=0; };

  QString commandToDaemon(QTcpSocket *socket,const QString &command, int timeoutMs=1000) {
   if (!socket || socket->state() != QAbstractSocket::ConnectedState) return QString(); // or the error msg
   socket->write(command.toUtf8()+"\n");
   if (!socket->waitForBytesWritten(timeoutMs)) return QString(); // timeout or write error
   if (!socket->waitForReadyRead(timeoutMs)) return QString(); // timeout or no response
   return QString::fromUtf8(socket->readAll()).trimmed();
  }

  void initFilters() {
   filterListBP.resize(ampCount); filterListN.resize(ampCount);
   //filterListD.resize(ampCount); filterListT.resize(ampCount);
   //filterListA.resize(ampCount); filterListB.resize(ampCount);
   //filterListG.resize(ampCount); 
   for (unsigned int ampIdx=0;ampIdx<ampCount;ampIdx++) {
    filterListBP[ampIdx].reserve(chnCount); filterListN[ampIdx].reserve(chnCount);
//    filterListD[ampIdx].reserve(chnCount); filterListT[ampIdx].reserve(chnCount);
//    filterListA[ampIdx].reserve(chnCount); filterListB[ampIdx].reserve(chnCount);
//    filterListG[ampIdx].reserve(chnCount);
    for (unsigned int chnIdx=0;chnIdx<chnCount;chnIdx++) {
//    filterListBP[ampIdx].emplace_back(b_01_100,a_01_100);
     filterListBP[ampIdx].emplace_back(b_2_40,a_2_40);
     filterListN[ampIdx].emplace_back(b_notch,a_notch);
//    filterListD[ampIdx].emplace_back(b_delta,a_delta); filterListT[ampIdx].emplace_back(b_theta,a_theta);
//    filterListA[ampIdx].emplace_back(b_alpha,a_alpha); filterListB[ampIdx].emplace_back(b_beta,a_beta);
//    filterListG[ampIdx].emplace_back(b_gamma,a_gamma);
    }
   }
  }

  QString acqIpAddr; quint32 acqCommPort,acqStrmPort; QTcpSocket *acqCommSocket,*acqStrmSocket; // We're client
  QString acqppIpAddr; quint32 acqppCommPort,acqppStrmPort; // We're server

  QMutex mutex,chnInterMutex;
  QWaitCondition dataReady;  // already exists
  QWaitCondition spaceReady; // NEW: signals that producer can push
  QVector<PPChnInfo> chnInfo;

  quint64 tcpBufHead,tcpBufTail; QVector<TcpSamplePP> tcpBuffer; quint32 tcpBufSize,halfTcpBufSize;

  unsigned int ampCount,eegRate,refChnCount,bipChnCount,chnCount,eegProbeMsecs,eegSamplesInTick;
  unsigned int physChnCount,totalChnCount,totalCount;
  float refGain,bipGain;

  QTcpServer acqppCommServer,acqppStrmServer;

  // IIR filter states and others.. for each channel of the amplifier
  std::vector<std::vector<IIRFilter>> filterListBP,filterListN; //,filterListD,filterListT,filterListA,filterListB,filterListG;

  // --- compute queue (producer: readyRead, consumer: CompThread)
  QMutex compMutex;
  QWaitCondition compReady;
  QWaitCondition compSpace;
  //QQueue<QByteArray> compQueue;
  int compQueueMax=64; // to be tuned later
  std::atomic<bool> compStop{false};

  int frameBytesIn,frameBytesOut;

  std::atomic<quint64> rxDropBlocks{0};

  struct CompBlock {
   QByteArray buf; int off=0; // bytes already consumed (0..buf.size())
  };

  QQueue<CompBlock> compQueue;

 public slots:
  void onStrmDataReady() {
   static QByteArray inbuf;
   static int rd=0;           // read cursor into inbuf
   static qint64 lastMsQ=0;
   static quint64 outerRx=0;
   static quint64 dropped=0;

   inbuf.append(acqStrmSocket->readAll());

   auto compact_if_needed=[&](){
    // compact when cursor is large to avoid unbounded growth
    if (rd > 1<<20) {               // 1MB consumed -> compact
     inbuf.remove(0,rd);
     rd=0;
    } else if (rd>0 && rd>inbuf.size()/2) {
     // optional: compact when we've consumed > half
     inbuf.remove(0,rd);
     rd=0;
    }
   };

   static constexpr quint32 MAX_BLOCK=8u*1024u*1024u;

   while ((inbuf.size()-rd)>=4) {
    const uchar* p0=reinterpret_cast<const uchar*>(inbuf.constData()+rd);
    const quint32 blockSize=(quint32)p0[0]|((quint32)p0[1]<<8)|((quint32)p0[2]<<16)|((quint32)p0[3]<<24);
    if (blockSize==0 || blockSize>MAX_BLOCK) {
     qWarning() << "[PP:RX] bad blockSize=" << blockSize << "clearing buffer";
     inbuf.clear(); rd=0;
     return;
    }

    if ((inbuf.size()-rd)<(4+(int)blockSize)) break;

    // Copy out exactly one block (one allocation+copy; no mid/remove memmove)
    QByteArray block(inbuf.constData()+rd+4,(int)blockSize);
    rd+=4+(int)blockSize;

    outerRx++;

    int qszSnap=0;
    {
      QMutexLocker lk(&compMutex);
      while (compQueue.size()>=compQueueMax) { compQueue.dequeue(); dropped++; }
      //compQueue.enqueue(std::move(block));
      CompBlock cb; cb.buf=std::move(block); cb.off=0; compQueue.enqueue(std::move(cb)); 
      qszSnap=compQueue.size();
      compReady.wakeOne();
    }

    compact_if_needed();

    // 1Hz log (call time only here)
    const qint64 now=QDateTime::currentMSecsSinceEpoch();
    if (now-lastMsQ>=1000) {
     lastMsQ=now;
     qInfo().noquote() << QString("[PP:RX] outer=%1/s compQueue=%2 inbuf=%3 rd=%4 drop=%5")
                          .arg((qulonglong)outerRx)
                          .arg(qszSnap)
                          .arg(inbuf.size())
                          .arg(rd)
                          .arg((qulonglong)dropped);
     outerRx=0;
    }
   }
   compact_if_needed();
  }
};
