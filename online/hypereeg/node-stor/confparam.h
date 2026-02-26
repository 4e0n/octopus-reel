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
#include "storchninfo.h"
#include "../common/tcpsample.h"

class ConfParam : public QObject {
 Q_OBJECT
 public:
  ConfParam() { recordingActive=false; tcpBufHead=tcpBufTail=0; };

  QString commandToDaemon(QTcpSocket *socket,const QString &command, int timeoutMs=1000) {
   if (!socket || socket->state() != QAbstractSocket::ConnectedState) return QString(); // or the error msg
   socket->write(command.toUtf8()+"\n");
   if (!socket->waitForBytesWritten(timeoutMs)) return QString(); // timeout or write error
   if (!socket->waitForReadyRead(timeoutMs)) return QString(); // timeout or no response
   return QString::fromUtf8(socket->readAll()).trimmed();
  }

  QString acqIpAddr; quint32 acqCommPort,acqStrmPort; QTcpSocket *acqCommSocket,*acqStrmSocket; // We're client
  QString storIpAddr; quint32 storCommPort; // We're server

  QMutex mutex;
  QWaitCondition dataReady;   // already exists
  QWaitCondition spaceReady;  // NEW: signals that producer can push
  QVector<StorChnInfo> chnInfo;

  quint64 tcpBufHead,tcpBufTail; QVector<TcpSample> tcpBuffer; quint32 tcpBufSize,halfTcpBufSize;

  unsigned int ampCount,eegRate,refChnCount,bipChnCount,chnCount,eegProbeMsecs,eegSamplesInTick;
  unsigned int physChnCount,totalChnCount,totalCount; int frameBytesIn;
  float refGain,bipGain;

  QTcpServer storCommServer;

  QFile hEEGFile; QDataStream hEEGStream; QString rHour,rMin,rSec;
  std::atomic<bool> recordingActive{false};

  std::atomic<quint64> rxOuterBlocks{0};
  std::atomic<quint64> rxBadBlocks{0};
  std::atomic<quint64> rxFramesOk{0};
  std::atomic<quint64> rxFramesBad{0};

  unsigned int storChunkSamples=1000; // 1s at 1kHz (tune)

 public slots:

 void onStrmDataReady() {
  static QByteArray inbuf;
  static int rd = 0;
  static qint64 lastMsLog = 0;

  inbuf.append(acqStrmSocket->readAll());

  auto compact_if_needed = [&]() {
    if (rd > (1<<20) || (rd > 0 && rd > inbuf.size()/2)) {
      inbuf.remove(0, rd);
      rd = 0;
    }
  };

  static constexpr quint32 MAX_BLOCK = 8u*1024u*1024u;

  // One-frame serialized size (raw TcpSample, NOT PP)
  const int frameSz=TcpSample::serializedSizeFor(ampCount,physChnCount);

  while ((inbuf.size()-rd)>=4) {
   const uchar* p0=reinterpret_cast<const uchar*>(inbuf.constData()+rd);
   const quint32 blockSize=(quint32)p0[0]|((quint32)p0[1]<<8)|((quint32)p0[2]<<16)|((quint32)p0[3]<<24);

   if (blockSize==0 || blockSize>MAX_BLOCK) {
    qWarning() << "[STOR:RX] bad blockSize=" << blockSize << "clearing buffer";
    inbuf.clear(); rd=0;
    return;
   }

   if ((inbuf.size()-rd)<(4+(int)blockSize)) break;

   const char* blockPtr=inbuf.constData()+rd+4;
   const int blockBytes=(int)blockSize;
   rd+=4+blockBytes;

   if ((blockBytes%frameSz)!=0) {
    rxBadBlocks.fetch_add(1,std::memory_order_relaxed);
    qWarning() << "[STOR:RX] block remainder not multiple of frameSz"
               << "blockBytes=" << blockBytes << "frameSz=" << frameSz;
    compact_if_needed();
    continue;
   }

   const int frames=blockBytes/frameSz;

   TcpSample tcpS(ampCount,physChnCount);
   for (int i=0;i<frames;++i) {
    const char* one=blockPtr+i*frameSz;
    rxOuterBlocks.fetch_add(1,std::memory_order_relaxed);

    // Use the SAME low-level call style as PP uses (deserializeRaw)
    if (!tcpS.deserializeRaw(one,frameSz,physChnCount,ampCount)) {
     // count/log
     rxFramesBad.fetch_add(1,std::memory_order_relaxed);
     continue;
    }
    rxFramesOk.fetch_add(1,std::memory_order_relaxed);

    // Push into ring (NO DROP=wait)
    {
      QMutexLocker locker(&mutex);
      while ((tcpBufHead-tcpBufTail)>=tcpBufSize) spaceReady.wait(&mutex);
      const quint64 before=tcpBufHead-tcpBufTail;
      tcpBuffer[tcpBufHead%tcpBufSize]=tcpS; tcpBufHead++;
      const quint64 after=tcpBufHead-tcpBufTail;
      if ((before/storChunkSamples)!=(after/storChunkSamples)) dataReady.wakeOne();
    }
   }

   const qint64 now=QDateTime::currentMSecsSinceEpoch();
   if (now-lastMsLog>=1000) {
    lastMsLog=now;
    static quint64 lastHead=0; static quint64 lastTail=0; quint64 h,t;
    {
      QMutexLocker lk(&mutex);
      h=tcpBufHead; t=tcpBufTail;
    }
    const quint64 used=h-t; const quint64 dh=h-lastHead; const quint64 dt=t-lastTail;
    lastHead=h; lastTail=t;
    qInfo().noquote() << QString("[STOR:RATES] dHead=%1 dTail=%2 dUsed=%3 used=%4")
     .arg((qulonglong)dh)
     .arg((qulonglong)dt)
     .arg((qlonglong)dh-(qlonglong)dt)
     .arg((qulonglong)(h-t));
    qInfo().noquote() << QString("[STOR:RX] outer=%1 badBlk=%2 ok=%3 bad=%4 inbuf=%5 rd=%6 frameSz=%7 used=%8/%9 (%10%)")
     .arg((qulonglong)rxOuterBlocks.exchange(0))
     .arg((qulonglong)rxBadBlocks.load())
     .arg((qulonglong)rxFramesOk.exchange(0))
     .arg((qulonglong)rxFramesBad.exchange(0))
     .arg(inbuf.size())
     .arg(rd)
     .arg(frameSz)
     .arg((qulonglong)used)
     .arg(tcpBufSize)
     .arg(100.0*double(used)/double(tcpBufSize),0,'f',1);
   }
  }
  compact_if_needed();
  if (rd==inbuf.size()) { inbuf.clear(); rd=0; }
 }
};
