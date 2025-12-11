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
#include "headmodel.h"
#include "../common/tcpsample.h"

#include "../../../common/event.h"
#include "../../../common/coord3d.h"
#include "../../../common/gizmo.h"

const int EEG_SCROLL_REFRESH_RATE=50; // 100Hz max.

class ConfParam : public QObject {
 Q_OBJECT
 public:
  ConfParam() {
   eegSweepRefreshRate=EEG_SCROLL_REFRESH_RATE; eegSweepUpdating=0;
   eegSweepMode=quitPending=ctrlRecordingActive=false; ctrlNotchActive=false;
   tcpBufHead=tcpBufTail=0; recCounter=0; audWaveH=100;
  };

  void initMultiAmp(int ampC=0) {
   ampCount=ampC; eegAmpX.resize(ampCount); erpAmpX.resize(ampCount); threads.resize(ampCount);
  };

  QString commandToDaemon(const QString &command, int timeoutMs=1000) { // Upstream command
   if (!commSocket || commSocket->state() != QAbstractSocket::ConnectedState) return QString(); // or the error msg
   commSocket->write(command.toUtf8()+"\n");
   if (!commSocket->waitForBytesWritten(timeoutMs)) return QString(); // timeout or write error
   if (!commSocket->waitForReadyRead(timeoutMs)) return QString(); // timeout or no response
   return QString::fromUtf8(commSocket->readAll()).trimmed();
  }

  void loadGizmo(QString fn) {
   QFile file; QTextStream stream; QString line; QStringList lines,validLines,opts,opts2;
   bool gError=false; QVector<unsigned int> t(3); QVector<unsigned int> ll(2);

   // TODO: here a dry-run would fit.
 
   // Delete previous if any
   if (glGizmoLoaded) glGizmo.resize(0);
   glGizmoLoaded=false;

   file.setFileName(fn); file.open(QIODevice::ReadOnly|QIODevice::Text); stream.setDevice(&file);
   while (!stream.atEnd()) { line=stream.readLine(250); lines.append(line); }
   file.close();

   // Get valid lines
   for (int i=0;i<lines.size();i++)
    if (!(lines[i].at(0)=='#') && lines[i].contains('|')) validLines.append(lines[i]);

   // Find the essential line defining gizmo names
   for (int i=0;i<validLines.size();i++) {
    opts2=validLines[i].split(" "); opts=opts2[0].split("|"); opts2.removeFirst(); opts2=opts2[0].split(",");
    if (opts.size()==2 && opts2.size()>0) {
     // GIZMO|LIST must be prior to others or segfault will occur..
     if (opts[0].trimmed()=="GIZMO") {
      if (opts[1].trimmed()=="NAME") {
       for (int g=0;g<opts2.size();g++) { Gizmo dummyGizmo(opts2[g].trimmed()); glGizmo.append(dummyGizmo); }
      }
     } else if (opts[0].trimmed()=="SHAPE") {
      ;
     } else if (opts[1].trimmed()=="SEQ") {
      int k=gizFindIndex(opts[0].trimmed()); for (int j=0;j<opts2.size();j++) glGizmo[k].seq.append(opts2[j].toInt()-1);
     } else if (opts[1].trimmed()=="TRI") { int k=gizFindIndex(opts[0].trimmed());
      if (opts2.size()%3==0) {
       for (int j=0;j<opts2.size()/3;j++) {
        t[0]=opts2[j*3+0].toInt()-1; t[1]=opts2[j*3+1].toInt()-1; t[2]=opts2[j*3+2].toInt()-1; glGizmo[k].tri.append(t);
       }
      } else { gError=true;
       qDebug() << "node_gui: <ConfParam> <LoadGizmo> <OGZ> ERROR: Triangles not multiple of 3 vertices..";
      }
     } else if (opts[1].trimmed()=="LIN") { int k=gizFindIndex(opts[0].trimmed());
      if (opts2.size()%2==0) {
       for (int j=0;j<opts2.size()/2;j++) {
        ll[0]=opts2[j*2+0].toInt()-1; ll[1]=opts2[j*2+1].toInt()-1;
	glGizmo[k].lin.append(ll);
       }
      } else { gError=true;
       qDebug() << "node_gui: <ConfParam> <LoadGizmo> <OGZ> ERROR: Lines not multiple of 2 vertices..";
      }
     }
    } else { gError=true; qDebug() << "node_gui: <ConfParam> <LoadGizmo> <OGZ> ERROR: while parsing!"; }
   } if (!gError) glGizmoLoaded=true;
  }

  int gizFindIndex(QString s) { int idx=-1;
   for (int i=0;i<glGizmo.size();i++) if (glGizmo[i].name==s) { idx=i; break; }
   return idx;
  }

  QVector<QColor> rgbPalette;

  QString ipAddr; quint32 commPort,strmPort; // Uplink
  QString svrIpAddr; quint32 svrCommPort,svrStrmPort; // Downlink
  QTcpSocket *commSocket,*strmSocket; // Upstream

  QMutex mutex; QVector<ChnInfo> chns; QVector<QThread*> threads;

  bool eegSweepMode; unsigned int eegSweepRefreshRate,eegSweepFrameTimeMs,eegSweepDivider,eegBand; QWaitCondition eegSweepWait;
  //std::atomic<unsigned int> eegSweepUpdating;
  unsigned int eegSweepUpdating; QVector<bool> eegSweepPending;

  std::vector<std::vector<float>> s0,s0s,s1,s1s; // Sweep vals

  const int eegSweepCoeff[5]={10,5,4,2,1};

  QVector<float> eegAmpX,erpAmpX;
  const float eegAmpRange[6]={(1e6/1000.0),
                              (1e6/ 500.0),
                              (1e6/ 200.0),
                              (1e6/ 100.0),
                              (1e6/  50.0),
                              (1e6/  20.0)};
  const float erpAmpRange[6]={(1e6/1000.0), // 2,5,10,20,50,100
                              (1e6/ 500.0),
                              (1e6/ 200.0),
                              (1e6/ 100.0),
                              (1e6/  50.0),
                              (1e6/  20.0)};

  //std::atomic<quint64> tcpBufHead,tcpBufTail;
  quint64 tcpBufHead,tcpBufTail; QVector<TcpSample> tcpBuffer; quint32 tcpBufSize,halfTcpBufSize;
  int cntPastSize,cntPastIndex;

  unsigned int ampCount,sampleRate,refChnCount,bipChnCount,chnCount,eegProbeMsecs; float refGain,bipGain;

  int guiCtrlX,guiCtrlY,guiCtrlW,guiCtrlH, guiAmpX,guiAmpY,guiAmpW,guiAmpH;
  int sweepFrameW,sweepFrameH, audWaveH, gl3DFrameW,gl3DFrameH;

  bool ctrlRecordingActive,ctrlNotchActive, tick,event,quitPending;

  QVector<Event> events; QString curEventName; quint32 curEventType;
  float erpRejBwd,erpAvgBwd,erpAvgFwd,erpRejFwd; // In terms of milliseconds

  unsigned int scrAvailableSamples,scrUpdateSamples;

  QFile hegFile; QTextStream hegStream; quint64 recCounter; QString rHour,rMin,rSec; QLabel *timeLabel;

  QVector<HeadModel> headModel; bool glGizmoLoaded; QVector<Gizmo> glGizmo;
  QVector<bool> glFrameOn,glGridOn,glScalpOn,glSkullOn,glBrainOn,glGizmoOn,glElectrodesOn;

 signals:
  void glUpdate();
  void glUpdateParam(int);

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


     // Handle spatial interpolation
     for (unsigned int ampIdx=0;ampIdx<ampCount;ampIdx++) {
      std::vector<float>* basePtr;
      switch (eegBand) {
       default:
       case 0: basePtr=&(tcpS.amp[ampIdx].dataBP); break;
       case 1: basePtr=&(tcpS.amp[ampIdx].dataD); break;
       case 2: basePtr=&(tcpS.amp[ampIdx].dataT); break;
       case 3: basePtr=&(tcpS.amp[ampIdx].dataA); break;
       case 4: basePtr=&(tcpS.amp[ampIdx].dataB); break;
       case 5: basePtr=&(tcpS.amp[ampIdx].dataG); break;
      }
      for (unsigned int chnIdx=0;chnIdx<chnCount;chnIdx++) {
       float avg;
       if (chns[chnIdx].chnViewMode[ampIdx]==2) {
        avg=0.; for (int idx=0;idx<chns[chnIdx].interElec.size();idx++) avg+=(*basePtr)[ chns[chnIdx].interElec[idx] ]; //tcpS.amp[ampIdx].dataBP[ chns[chnIdx].interElec[idx] ];
        (*basePtr)[chnIdx]=avg; // tcpS.amp[ampIdx].dataBP[chnIdx]=avg;
        //avg=0.; for (int idx=0;idx<chns[chnIdx].interElec.size();idx++) avg+=tcpS.amp[ampIdx].dataN[ chns[chnIdx].interElec[idx] ];
        //tcpS.amp[ampIdx].dataN[chnIdx]=avg;
       } else if (chns[chnIdx].chnViewMode[ampIdx]==0) {
        (*basePtr)[chnIdx]=0.; //tcpS.amp[ampIdx].dataBP[chnIdx]=0.;
        //tcpS.amp[ampIdx].dataN[chnIdx]=0.;
       }
      }
     }

//     if (tcpS.offset%1000==0) qDebug("BUFFER(mod1000) RECVd! tcpS.offset-> %lld - Magic: %x",tcpS.offset,tcpS.MAGIC);

//     if (tcpS.offset%1000==0) {
//      qint64 now=QDateTime::currentMSecsSinceEpoch(); qint64 age=now-tcpS.timestampMs;
//      qInfo() << "Sample latency @onStrmDataReady:" << age << "ms";
//     }

     //if (tcpS.trigger) qDebug() << "Trigger arrived!";
 
     // Push the deserialized tcpSample to circular buffer
     tcpBuffer[tcpBufHead%tcpBufSize]=tcpS;
     
     if (ctrlRecordingActive) { // .. to disk ..
      hegStream << tcpS.offset << " ";
      for (unsigned int ampIdx=0;ampIdx<ampCount;ampIdx++) {
       for (int chnIdx=0;chnIdx<chns.size();chnIdx++) hegStream << tcpS.amp[ampIdx].dataBP[chnIdx]*1e6 << " ";
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
     if (availableSamples>scrAvailableSamples && eegSweepUpdating==0) {
      if (availableSamples>=halfTcpBufSize) qWarning() << "[Client] Buffer overflow risk! Tail too slow.";
      //qDebug() << "[Client] 1.Min # EEG samples to fill one scrollframe has arrived, 2. All scrollers are idle.";
      // A fraction of above - i.e. one pixel corresponds to how many physical samples
      // 10:5Hz(100) 5:10Hz(200) 4:20Hz(250) 2:25Hz(500) 1:50Hz(1000)
      scrUpdateSamples=scrAvailableSamples/(eegSweepFrameTimeMs/eegSweepDivider);
      if (quitPending) {
       if (strmSocket && strmSocket->isOpen()) strmSocket->disconnectFromHost();
      } 
      eegSweepUpdating=ampCount; { for (auto& s:eegSweepPending) s=true; } eegSweepWait.wakeAll();
     }
    }

    // Here the scrollings should asynchronously happen and this routine cannot enter the above region.
    // As soon as the last scroller finishes, 1. the tcpBufTail will advance, 2.eegSweepUpdating will be 0
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
