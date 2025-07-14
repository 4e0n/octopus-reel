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
#include <atomic>
#include <QVector>
#include "chninfo.h"
#include "../../../common/event.h"
#include "../tcpsample.h"

class ConfParam : public QObject {
 Q_OBJECT
 public:
  ConfParam() {};
  void init(int ampC=0) { ampCount=ampC; tcpBufHead=tcpBufTail=0; updateInstant=false; eegAmpX.resize(ampCount); };

  QString commandToDaemon(const QString &command, int timeoutMs=1000) {
   if (!commSocket || commSocket->state() != QAbstractSocket::ConnectedState) return QString(); // or the error msg
   commSocket->write(command.toUtf8()+"\n");
   if (!commSocket->waitForBytesWritten(timeoutMs)) return QString(); // timeout or write error
   if (!commSocket->waitForReadyRead(timeoutMs)) return QString(); // timeout or no response
   return QString::fromUtf8(commSocket->readAll()).trimmed();
  }

  QTcpSocket *commSocket,*dataSocket;
  QVector<TcpSample> tcpBuffer; quint64 tcpBufHead,tcpBufTail; quint32 tcpBufSize;

  unsigned int ampCount,sampleRate,refChnCount,bipChnCount,chnCount,eegProbeMsecs; float refGain,bipGain;
  QString ipAddr; quint32 commPort,dataPort;
  int guiCtrlX,guiCtrlY,guiCtrlW,guiCtrlH,guiStrmX,guiStrmY,guiStrmW,guiStrmH,guiHeadX,guiHeadY,guiHeadW,guiHeadH;
  int eegFrameW,eegFrameH,glFrameW,glFrameH;

  QVector<QVector<float> > scrPrvData,scrPrvDataF,scrCurData,scrCurDataF; QVector<float> cntAmpX;
  quint32 scrCounter,cntSpeedX; bool tick,event;

  QVector<Event*> acqEvents;
  QVector<float> eegAmpX;
  QVector<ChnInfo> chns;
  QString curEventName;
  quint32 curEventType;
  float rejBwd,avgBwd,avgFwd,rejFwd; // In terms of milliseconds
  int cntPastSize,cntPastIndex; //,avgCount,rejCount,postRejCount,bwCount;
  QVector<QColor> rgbPalette;
  QVector<Event> events;

  QApplication *application;

  std::atomic<bool> updateInstant; QVector<bool> updated;
  mutable QMutex mutex;

 signals:
  void scrData(bool tick,bool event);

 public slots:
  void onDataReady() {
   static QByteArray buffer; buffer.append(dataSocket->readAll());
   //qDebug() << "[Client] Incoming data size:" << buffer.size();

   while (buffer.size()>=4) {
    QDataStream sizeStream(buffer); sizeStream.setByteOrder(QDataStream::LittleEndian);
    quint32 blockSize=0; sizeStream>>blockSize;
    //qDebug() << "[Client] Expected blockSize:" << blockSize;
    if ((quint32)buffer.size()<4+blockSize) break; // Not enough data yet, wait for next readyRead()
    QByteArray block=buffer.mid(4,blockSize);
    // Deserialize into TcpSample
    QDataStream in(&block,QIODevice::ReadOnly);
    in.setByteOrder(QDataStream::LittleEndian);
    quint32 trigger=0,ampCount=0; in>>trigger>>ampCount;
    //qDebug() << "[Client] trigger:" << trigger << " ampCount:" << ampCount;
    TcpSample parsedSample; parsedSample.trigger=trigger; parsedSample.amp.resize(ampCount);
    for (quint32 i=0;i<ampCount;++i) { Sample s; in>>s.marker;
     //qDebug() << "[Client] First marker:" << s.marker;
     auto readFloatVector=[&](std::vector<float>& vec,int size) { vec.resize(size);
      for (int k = 0; k < size; ++k) { float value; in>>value; vec[k]=value; }
     };
     readFloatVector(s.data, chnCount); readFloatVector(s.dataF,chnCount);
     //qDebug() << "[Client] After deserialization: amp=o"
     //         << " raw=" << s.data[0]
     //         << " flt=" << s.dataF[0];
     parsedSample.amp[i]=s;
    }
    // Push parsedSample to ring buffer
    tcpBuffer[tcpBufHead%tcpBufSize]=parsedSample; tcpBufHead++;
    //qDebug() << "Parsed EEG block with amps:" << ampCount << " Trigger:" << trigger;
    //for (AcqStreamWindow *streamWin:streamWindows) {
    // qDebug() << "[Client] Before updateEEG: amp=0"
    //          << " raw=" << parsedSample.amp[0].data[0]
    //          << " flt=" << parsedSample.amp[0].dataF[0];
    // if (streamWin) streamWin->updateEEG(parsedSample);
    //}
    buffer.remove(0,4+blockSize);

    // Here we have a new sample
    //QMutexLocker locker(&mutex);
    //mutex.lock();
    if (!scrCounter) {
     for (unsigned int i=0;i<ampCount;i++) {
      updated[i]=false;
      for (unsigned int j=0;j<chnCount;j++) {
       scrPrvData[i][j]=scrCurData[i][j]; scrPrvDataF[i][j]=scrCurDataF[i][j];
       scrCurData[i][j]=tcpBuffer[tcpBufTail%tcpBufSize].amp[i].data[j];
       scrCurDataF[i][j]=tcpBuffer[tcpBufTail%tcpBufSize].amp[i].dataF[j];
      }
     }
     updateInstant=true;
     tick=event=false;
    } tcpBufTail++; scrCounter++; scrCounter%=2; //cntSpeedX;
    //mutex.unlock();
   }
  }

};

#endif
