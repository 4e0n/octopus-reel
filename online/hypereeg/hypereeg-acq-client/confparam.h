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

#ifndef CONFPARAM_H
#define CONFPARAM_H

#include <QApplication>
#include <QColor>
#include <QTcpSocket>
#include <QMutex>
#include <QWaitCondition>
#include <QThread>
#include <atomic>
#include <QVector>
#include "chninfo.h"
#include "../../../common/event.h"
#include "../tcpsample.h"

const int EEG_SCROLL_REFRESH_RATE=100; // 100Hz max.

class ConfParam : public QObject {
 Q_OBJECT
 public:
  ConfParam() {};
  void init(int ampC=0) {
   ampCount=ampC; tcpBufHead=tcpBufTail=0; scrollersUpdating=0; ampX.resize(ampCount); threads.resize(ampCount);
   eegScrollRefreshRate=EEG_SCROLL_REFRESH_RATE; threadOn=true;
  };

  QString commandToDaemon(const QString &command, int timeoutMs=1000) {
   if (!commSocket || commSocket->state() != QAbstractSocket::ConnectedState) return QString(); // or the error msg
   commSocket->write(command.toUtf8()+"\n");
   if (!commSocket->waitForBytesWritten(timeoutMs)) return QString(); // timeout or write error
   if (!commSocket->waitForReadyRead(timeoutMs)) return QString(); // timeout or no response
   return QString::fromUtf8(commSocket->readAll()).trimmed();
  }

  QTcpSocket *commSocket,*eegDataSocket,*cmDataSocket;
  //std::atomic<quint64> tcpBufHead,tcpBufTail;
  QVector<TcpSample> tcpBuffer; quint64 tcpBufHead,tcpBufTail; quint32 tcpBufSize;

  unsigned int ampCount,sampleRate,refChnCount,bipChnCount,chnCount,eegProbeMsecs; float refGain,bipGain;
  QString ipAddr; quint32 commPort,eegDataPort,cmDataPort;
  int guiCtrlX,guiCtrlY,guiCtrlW,guiCtrlH,guiStrmX,guiStrmY,guiStrmW,guiStrmH,guiHeadX,guiHeadY,guiHeadW,guiHeadH;
  int eegFrameW,eegFrameH,glFrameW,glFrameH;

  bool tick,event; int spdX;

  bool threadOn;

  int eegScrollRefreshRate,eegScrollFrameTimeMs;

  QVector<QThread*> threads;

  const float ampRange[6]={(1e6/1000.0),
                           (1e6/ 500.0),
                           (1e6/ 200.0),
                           (1e6/ 100.0),
                           (1e6/  50.0),
                           (1e6/  20.0)};

  const float spdRange[6]={10,8,4,2,1};

  QVector<Event*> acqEvents;
  QVector<float> ampX;
  QVector<ChnInfo> chns;
  QString curEventName;
  quint32 curEventType;
  float rejBwd,avgBwd,avgFwd,rejFwd; // In terms of milliseconds
  int cntPastSize,cntPastIndex; //,avgCount,rejCount,postRejCount,bwCount;
  QVector<QColor> rgbPalette;
  QVector<Event> events;

  QApplication *application;

  QMutex mutex; QWaitCondition scrollWait;
  //std::atomic<unsigned int> scrollersUpdating;
  unsigned int scrSmpCount,scrollersUpdating; QVector<bool> scrollPending;

 public slots:
  void onEEGDataReady() {
   static QByteArray buffer; buffer.append(eegDataSocket->readAll());
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
     tcpBuffer[tcpBufHead%tcpBufSize]=tcpS; tcpBufHead++;
    } else {
     qWarning() << "Failed to deserialize TcpSample block.";
    }

    // Samples to fit in one frame of scrolling has just arrived
    {
     QMutexLocker locker(&mutex);
     if ((tcpBufHead-tcpBufTail)>scrSmpCount && scrollersUpdating==0) {

      if ((tcpBufHead-tcpBufTail) >= (tcpBufSize/2)) {
       qWarning() << "[Client] Buffer overflow risk! Tail too slow.";
      }

      //qDebug() << "[Client] 1.Min # EEG samples to fill one scrollframe has arrived, 2. All scrollers are idle.";
      scrollersUpdating=ampCount; { for (auto& scr:scrollPending) scr=true; } scrollWait.wakeAll();
     }
    }

    // Here the scrollings should asynchronously happen and this routine cannot enter the above region.
    // As soon as the last scroller finishes, 1. the tcpBufTail will advance, 2.scrollersUpdating will be 0
    // So, the region above will again be available for a single entry only.

    buffer.remove(0,4+blockSize); // Remove processed block
   }
  }
};

#endif
