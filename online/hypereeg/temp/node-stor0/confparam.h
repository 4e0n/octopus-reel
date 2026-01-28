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
#include <QTextStream>
#include <QVector>
#include <QLabel>
#include <QDateTime>

#include "chninfo.h"
#include "../common/tcpsample.h"
#include "../../../common/coord3d.h"

class ConfParam : public QObject {
 Q_OBJECT
 public:
  ConfParam() {
   recordingActive=false; tcpBufHead=tcpBufTail=0; recCounter=0;
  };

  QString commandToDaemon(const QString &command, int timeoutMs=1000) { // Upstream command
   if (!commSocket || commSocket->state() != QAbstractSocket::ConnectedState) return QString(); // or the error msg
   commSocket->write(command.toUtf8()+"\n");
   if (!commSocket->waitForBytesWritten(timeoutMs)) return QString(); // timeout or write error
   if (!commSocket->waitForReadyRead(timeoutMs)) return QString(); // timeout or no response
   return QString::fromUtf8(commSocket->readAll()).trimmed();
  }

  QString ipAddr; quint32 commPort,strmPort; // Downlink
  QString svrIpAddr; quint32 svrCommPort; // Uplink
  QTcpSocket *commSocket,*strmSocket; // Downstream

  QMutex mutex; QVector<ChnInfo> chns; //QVector<QThread*> threads;

  //std::atomic<quint64> tcpBufHead,tcpBufTail;
  quint64 tcpBufHead,tcpBufTail; QVector<TcpSample> tcpBuffer; quint32 tcpBufSize,halfTcpBufSize;
  int cntPastSize,cntPastIndex;

  unsigned int ampCount,eegRate,refChnCount,bipChnCount,chnCount,eegProbeMsecs,eegSamplesInTick; float refGain,bipGain;

  bool recordingActive;

  QFile hegFile; QTextStream hegStream; quint64 recCounter; QString rHour,rMin,rSec; QLabel *timeLabel;

 public slots:
  void onStrmDataReady() {
   static QByteArray buffer; buffer.append(strmSocket->readAll());
   //qDebug() << "[Client] Incoming data size:" << buffer.size();
   while (buffer.size()>=4) {
    QDataStream sizeStream(buffer); sizeStream.setByteOrder(QDataStream::LittleEndian);
    quint32 blockSize=0; sizeStream>>blockSize;
    //qDebug() << "[Client] Expected blockSize:" << blockSize;
    if ((quint32)buffer.size()<4+blockSize) break; // Wait for more data
    QByteArray block=buffer.mid(4,blockSize);
    // Deserialize into TcpSample
    TcpSample tcpS;
    if (tcpS.deserialize(block,chnCount)) {

     // Push the deserialized tcpSample to circular buffer
     tcpBuffer[tcpBufHead%tcpBufSize]=tcpS;
     
     tcpBufHead++;
    } else {
     qWarning() << "Failed to deserialize TcpSample block.";
    }

    // Now this is for recording thread
    {
     QMutexLocker locker(&mutex);
     unsigned int availableSamples=tcpBufHead-tcpBufTail;
    }

    buffer.remove(0,4+blockSize); // Remove processed block
   }
  }
 private:
  void updateRecTime() { int s,m,h; QString dummyString; s=recCounter/eegRate; //m=s/60; h=m/60;
   h=s/3600; s%=3600; m=s/60; s%=60;
   if (h<10) rHour="0"; else rHour=""; rHour+=dummyString.setNum(h);
   if (m<10) rMin="0"; else rMin=""; rMin+=dummyString.setNum(m);
   if (s<10) rSec="0"; else rSec=""; rSec+=dummyString.setNum(s);
   timeLabel->setText("Rec.Time: "+rHour+":"+rMin+":"+rSec);
  }
};
