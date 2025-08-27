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
#include "../../../common/event.h"
#include "../tcpsample.h"

const int EEG_SCROLL_REFRESH_RATE=50; // 100Hz max.

class ConfParam : public QObject {
 Q_OBJECT
 public:
  ConfParam() {};
  void init(int ampC=0) {
   ampCount=ampC; tcpBufHead=tcpBufTail=0; guiUpdating=0; ampX.resize(ampCount); threads.resize(ampCount);
   eegScrollRefreshRate=EEG_SCROLL_REFRESH_RATE; scrollMode=quitPending=false; notchActive=true;
   recCounter=0; recording=false; audFrameH=100;
  };

  QString commandToDaemon(const QString &command, int timeoutMs=1000) { // Upstream command
   if (!commSocket || commSocket->state() != QAbstractSocket::ConnectedState) return QString(); // or the error msg
   commSocket->write(command.toUtf8()+"\n");
   if (!commSocket->waitForBytesWritten(timeoutMs)) return QString(); // timeout or write error
   if (!commSocket->waitForReadyRead(timeoutMs)) return QString(); // timeout or no response
   return QString::fromUtf8(commSocket->readAll()).trimmed();
  }

  std::vector<std::vector<float>> s0,s0s,s1,s1s; // Scroll vals

  QTcpSocket *commSocket,*strmSocket,*cmodSocket; // Upstream

  //std::atomic<quint64> tcpBufHead,tcpBufTail;
  QVector<TcpSample> tcpBuffer; quint64 tcpBufHead,tcpBufTail; quint32 tcpBufSize,halfTcpBufSize;

  unsigned int ampCount,sampleRate,refChnCount,bipChnCount,chnCount,eegProbeMsecs; float refGain,bipGain;
  QString ipAddr; quint32 commPort,strmPort,cmodPort; // Uplink
  QString svrIpAddr; quint32 svrCommPort,svrStrmPort,svrCmodPort; // Downlink
  int guiCtrlX,guiCtrlY,guiCtrlW,guiCtrlH,guiStrmX,guiStrmY,guiStrmW,guiStrmH;
  int eegFrameW,eegFrameH,audFrameH;

  bool scrollMode,notchActive,quitPending;

  bool tick,event;

  unsigned int eegScrollRefreshRate,eegScrollFrameTimeMs,eegScrollDivider;

  QVector<QThread*> threads;

  const int eegScrollCoeff[5]={10,5,4,2,1};

  const float ampRange[6]={(1e6/1000.0),
                           (1e6/ 500.0),
                           (1e6/ 200.0),
                           (1e6/ 100.0),
                           (1e6/  50.0),
                           (1e6/  20.0)};

  QVector<Event*> acqEvents;
  QVector<float> ampX;
  QVector<ChnInfo> chns;
  QString curEventName;
  quint32 curEventType;
  float rejBwd,avgBwd,avgFwd,rejFwd; // In terms of milliseconds
  int cntPastSize,cntPastIndex;
  QVector<QColor> rgbPalette;
  QVector<Event> events;

  QMutex mutex; QWaitCondition guiWait;
  //std::atomic<unsigned int> guiUpdating;
  unsigned int scrAvailableSamples,scrUpdateSamples;
  unsigned int guiUpdating; QVector<bool> guiPending;

  QFile hegFile; QTextStream hegStream;
  quint64 recCounter; bool recording;
  QString rHour,rMin,rSec;
  QLabel *timeLabel;

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

     if (tcpS.offset%1000==0) {
      qint64 now=QDateTime::currentMSecsSinceEpoch(); qint64 age=now-tcpS.timestampMs;
      qInfo() << "Sample latency @onStrmDataReady:" << age << "ms";
     }

     // Push the deserialized tcpSample to circular buffer
     tcpBuffer[tcpBufHead%tcpBufSize]=tcpS;
     
     if (recording) { // .. to disk ..
      hegStream << tcpS.offset << " ";
      for (unsigned int ampIdx=0;ampIdx<ampCount;ampIdx++) {
       for (int chnIdx=0;chnIdx<chns.size();chnIdx++) hegStream << tcpS.amp[ampIdx].dataF[chnIdx]*1e6 << " ";
      }
      hegStream << tcpS.trigger << "\n";
      recCounter++; if (!(recCounter%sampleRate)) updateRecTime();
     }
     
     tcpBufHead++;
    } else {
     qWarning() << "Failed to deserialize TcpSample block.";
    }

    {
     QMutexLocker locker(&mutex);
     unsigned int availableSamples=tcpBufHead-tcpBufTail;
     if (availableSamples>scrAvailableSamples && guiUpdating==0) {
      if (availableSamples>=halfTcpBufSize) qWarning() << "[Client] Buffer overflow risk! Tail too slow.";
      //qDebug() << "[Client] 1.Min # EEG samples to fill one scrollframe has arrived, 2. All scrollers are idle.";
      // A fraction of above - i.e. one pixel corresponds to how many physical samples
      // 10:5Hz(100) 5:10Hz(200) 4:20Hz(250) 2:25Hz(500) 1:50Hz(1000)
      scrUpdateSamples=scrAvailableSamples/(eegScrollFrameTimeMs/eegScrollDivider);
      if (quitPending) {
       if (strmSocket && strmSocket->isOpen()) strmSocket->disconnectFromHost();
       if (cmodSocket && cmodSocket->isOpen()) cmodSocket->disconnectFromHost();
      } 
      guiUpdating=ampCount; { for (auto& g:guiPending) g=true; } guiWait.wakeAll();
     }
    }

    // Here the scrollings should asynchronously happen and this routine cannot enter the above region.
    // As soon as the last scroller finishes, 1. the tcpBufTail will advance, 2.guiUpdating will be 0
    // So, the region above will again be available for a single entry only.

    buffer.remove(0,4+blockSize); // Remove processed block
   }
  }
 private:
  void updateRecTime() { int s,m,h; QString dummyString; s=recCounter/sampleRate; //m=s/60; h=m/60;
   h=s/3600; s%=3600; m=s/60; s%=60;
   if (h<10) rHour="0"; else rHour=""; rHour+=dummyString.setNum(h);
   if (m<10) rMin="0"; else rMin=""; rMin+=dummyString.setNum(m);
   if (s<10) rSec="0"; else rSec=""; rSec+=dummyString.setNum(s);
   timeLabel->setText("Rec.Time: "+rHour+":"+rMin+":"+rSec);
  }
};

#endif

