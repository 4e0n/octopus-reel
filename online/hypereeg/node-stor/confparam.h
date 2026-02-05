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
  QVector<StorChnInfo> chns;

  //std::atomic<quint64> tcpBufHead,tcpBufTail;
  quint64 tcpBufHead,tcpBufTail; QVector<TcpSample> tcpBuffer; quint32 tcpBufSize,halfTcpBufSize;

  unsigned int ampCount,eegRate,refChnCount,bipChnCount,chnCount,eegProbeMsecs,eegSamplesInTick; float refGain,bipGain;

  std::atomic<bool> recordingActive{false};

  QFile hEEGFile; QDataStream hEEGStream; QString rHour,rMin,rSec;

 public slots:
  void onStrmDataReady() {
   static quint64 prodBlocks=0;
   static quint64 prodSamples=0;
   static quint64 prodDeserializeFail=0;
   static quint64 prodBlocked=0;
   static qint64 lastBlockLogMs=0;
   static qint64 lastLogMs=0;
   static quint64 lastSamples=0;
   static QByteArray buffer;
   static quint64 maxAvail=0;
   buffer.append(acqStrmSocket->readAll());

   while (buffer.size()>=4) {
    QDataStream sizeStream(buffer);
    sizeStream.setByteOrder(QDataStream::LittleEndian);

    quint32 blockSize=0; sizeStream >> blockSize;

    if ((quint32)buffer.size()<4u+blockSize) break;

    QByteArray block=buffer.mid(4,blockSize);

    TcpSample tcpS;
    if (tcpS.deserialize(block,chnCount)) {
     {
      QMutexLocker locker(&mutex);

      // NO DROP: wait for free space
      while ((tcpBufHead - tcpBufTail) >= tcpBufSize) {
       prodBlocked++;
       const qint64 nowB = QDateTime::currentMSecsSinceEpoch();
       if (nowB - lastBlockLogMs >= 1000) {
        lastBlockLogMs = nowB;
        qCritical() << "<Producer> STOR[ALARM] RING FULL: producer blocked!"
                    << "head=" << tcpBufHead
                    << "tail=" << tcpBufTail
                    << "used=" << (tcpBufHead - tcpBufTail)
                    << "bufSize=" << tcpBufSize;
       }
       spaceReady.wait(&mutex,5); // or 50, but 5 reduces event-loop freeze
      }

      tcpBuffer[tcpBufHead%tcpBufSize]=tcpS;
      tcpBufHead++;

      const quint64 availableSamples=tcpBufHead-tcpBufTail;
      if (availableSamples>maxAvail) maxAvail=availableSamples;

      if (availableSamples>=(quint64)eegSamplesInTick) dataReady.wakeOne();
     }

     prodBlocks++;
     prodSamples++;

     const qint64 now=QDateTime::currentMSecsSinceEpoch();
     if (now-lastLogMs >= 1000) {
      const quint64 sps=prodSamples-lastSamples;
      lastSamples=prodSamples;
      lastLogMs=now;

      quint64 headSnap=0,tailSnap=0,availSnap=0;
      {
       QMutexLocker locker(&mutex);
       headSnap=tcpBufHead;
       tailSnap=tcpBufTail;
       availSnap=headSnap-tailSnap;
      }

      const quint64 maxA=maxAvail;
      maxAvail=0;
      qInfo().noquote()
        << QString("<Producer> STOR[PROD] +%1 samp/s, total=%2, blocked=%3, head=%4 tail=%5 avail=%6 lastOff=%7 magic=0x%8 maxAvail=%9")
            .arg(sps)
            .arg(prodSamples)
            .arg(prodBlocked)
            .arg(headSnap)
            .arg(tailSnap)
            .arg(availSnap)
            .arg((qulonglong)tcpS.offset)
            .arg(QString::number(tcpS.MAGIC,16))
            .arg(maxA);
     }
    } else {
      prodDeserializeFail++;
      if ((prodDeserializeFail%100)==1)
        qWarning() << "<Producer> STOR[PROD] deserialize failed. count=" << prodDeserializeFail;
    }

    buffer.remove(0, 4 + blockSize);
   }
  }
};
