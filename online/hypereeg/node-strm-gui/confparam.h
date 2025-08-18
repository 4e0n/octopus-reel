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
   audioFrameH=100;
   //gl3DGuiX=160; gl3DGuiY=160;
   gl3DGuiW=400; gl3DGuiH=300;
   recCounter=0; recording=false;
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
  int guiCtrlX,guiCtrlY,guiCtrlW,guiCtrlH,guiStrmX,guiStrmY,guiStrmW,guiStrmH,guiHeadX,guiHeadY,guiHeadW,guiHeadH;
  int eegFrameW,eegFrameH,glFrameW,glFrameH;
  int audioFrameH;
  //int gl3DGuiX,gl3DGuiY;
  int gl3DGuiW,gl3DGuiH;

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
  int cntPastSize,cntPastIndex; //,avgCount,rejCount,postRejCount,bwCount;
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
     // Push the deserialized tcpSample to circular buffer
     tcpBuffer[tcpBufHead%tcpBufSize]=tcpS;
     
     if (recording) { // .. to disk ..
      hegStream << tcpS.offset << "\n";
      for (unsigned int ampIdx=0;ampIdx<ampCount;ampIdx++) {
       for (unsigned int chnIdx=0;chnIdx<chns.size();chnIdx++) hegStream << tcpS.amp[ampIdx].dataF[chnIdx] << " ";
       hegStream << tcpS.trigger << "\n";
      }
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
  void updateRecTime() { int s,m,h; QString dummyString;
   s=recCounter/sampleRate; m=s/60; h=m/60;
   if (h<10) rHour="0"; else rHour=""; rHour+=dummyString.setNum(h);
   if (m<10) rMin="0"; else rMin=""; rMin+=dummyString.setNum(m);
   if (s<10) rSec="0"; else rSec=""; rSec+=dummyString.setNum(s);
   timeLabel->setText("Rec.Time: "+rHour+":"+rMin+":"+rSec);
  }
};

#endif




/*
   clientRunning=recording=withinAvgEpoch=eventOccured=false;
   avgCounter=0;
   
   digExists.resize(ampCount); scalpExists.resize(ampCount); skullExists.resize(ampCount); brainExists.resize(ampCount);
   gizmoExists=digExists[i]=scalpExists[i]=skullExists[i]=brainExists[i]=false;

    if (avgSection.size()>0) { // AVG
     for (int i=0;i<avgSection.size();i++) { opts=avgSection[i].split("=");
      if (opts[0].trimmed()=="INTERVAL") { opts2=opts[1].split(",");
       if (opts2.size()==4) {
        cp.rejBwd=opts2[0].toInt(); cp.avgBwd=opts2[1].toInt(); cp.avgFwd=opts2[2].toInt(); cp.rejFwd=opts2[3].toInt();
        if ((!(cp.rejBwd >= -1000 && cp.rejBwd <=    0)) || (!(cp.avgBwd >= -1000 && cp.avgBwd <=    0)) ||
            (!(cp.avgFwd >=     0 && cp.avgFwd <= 1000)) || (!(cp.rejFwd >=     0 && cp.rejFwd <= 1000)) ||
            (cp.rejBwd>cp.avgBwd) || (cp.avgBwd>cp.avgFwd) || (cp.avgFwd>cp.rejFwd)) {
         qDebug() << "octopus_acq_client: <AcqMaster> <.conf> AVG|INTERVAL parameters not within suitable range!"; application->quit();
        } else {
         cp.avgCount=(cp.avgFwd-cp.avgBwd)*sampleRate/1000; cp.rejCount=(cp.rejFwd-cp.rejBwd)*sampleRate/1000;
         cp.postRejCount=(cp.rejFwd-cp.avgFwd)*sampleRate/1000; cp.bwCount=cp.rejFwd*sampleRate/1000;
        }
       } else { qDebug() << "octopus_acq_client: <AcqMaster> <.conf> Parse error in AVG|INTERVAL parameters!"; application->quit(); }
      } else { qDebug() << "octopus_acq_client: <AcqMaster> <.conf> Parse error in AVG sections!"; application->quit(); }
     }
    }

    // TODO: REDEFINE EVENT SPACE AND HIERARCHY!
    if (evtSection.size()>0) { // STIM
     for (int i=0;i<evtSection.size();i++) { opts=evtSection[i].split("=");
      if (opts[0].trimmed()=="STIM") { opts2=opts[1].split(",");
       if (opts2.size()==5) {
        if ((!(opts2[0].toInt()>0  && opts2[0].toInt()<256)) || (!(opts2[1].size()>0)) ||
            (!(opts2[2].toInt()>=0 && opts2[2].toInt()<256)) ||
            (!(opts2[3].toInt()>=0 && opts2[3].toInt()<256)) ||
            (!(opts2[4].toInt()>=0 && opts2[4].toInt()<256))) {
         qDebug() << "octopus_acq_client: <AcqMaster> <.conf>  Syntax/logic error in EVT|STIM parameters!"; application->quit();
        } else { // Add to the list of pre-recognized events.
         dummyEvt=new Event(opts2[0].toInt(),opts2[1].trimmed(),1,QColor(opts2[2].toInt(),opts2[3].toInt(),opts2[4].toInt(),255));
         acqEvents.append(dummyEvt);
        }
       } else { qDebug() << "octopus_acq_client: <AcqMaster> <.conf>  Parse error in EVT|STIM parameters!"; application->quit(); }
      } else { qDebug() << "octopus_acq_client: <AcqMaster> <.conf> Parse error in EVT sections!"; application->quit(); }
     }
    }

    acqChannels.resize(ampCount);

    if (chnSection.size()>0) { // CHN
     for (int i=0;i<chnSection.size();i++) { opts=chnSection[i].split("=");
      if (opts[0].trimmed()=="APPEND") { opts2=opts[1].split(",");
       if (opts2.size()==10) {
        opts2[0]=opts2[0].trimmed(); opts2[1]=opts2[1].trimmed(); opts2[4]=opts2[4].trimmed(); opts2[5]=opts2[5].trimmed(); // Trim wspcs
        opts2[6]=opts2[6].trimmed(); opts2[7]=opts2[7].trimmed(); opts2[8]=opts2[8].trimmed(); opts2[9]=opts2[9].trimmed();
	if ((!(opts2[0].toInt()>0 && opts2[1].toInt()<=(int)ampInfo.physChnCount)) || // Channel#
            (!(opts2[1].size()>0)) || // Channel name must be at least 1 char..
            (!(opts2[2].toInt()>=0 && opts2[2].toInt()<1000))   || // Rej
            (!(opts2[3].toInt()>=0 && opts2[3].toInt()<=(int)ampInfo.physChnCount)) || // RejRef
            (!(opts2[4]=="T" || opts2[4]=="t" || opts2[4]=="F" || opts2[4]=="f")) ||
            (!(opts2[5]=="T" || opts2[5]=="t" || opts2[5]=="F" || opts2[5]=="f")) ||
            (!(opts2[6]=="T" || opts2[6]=="t" || opts2[6]=="F" || opts2[6]=="f")) ||
            (!(opts2[7]=="T" || opts2[7]=="t" || opts2[7]=="F" || opts2[7]=="f")) ||
            (!(opts2[8].toFloat()>=0. && opts2[8].toFloat()<=360.)) || // Theta 
            (!(opts2[9].toFloat()>=0. && opts2[9].toFloat()<=360.))){  // Phi
         qDebug() << "octopus_acq_client: <AcqMaster> <.conf> Syntax/logic Error in CHN|APPEND parameters!"; application->quit();

        } else { // Add to the list of channels
         dummyChn=new Channel(opts2[0].toInt()-1,  // Physical channel (0-indexed)
                              opts2[1].trimmed(),  // Channel Name
                              opts2[2].toInt(),	   // Rejection Level
                              opts2[3].toInt()-1,  // Rejection Reference Channel for that channel (0-indexed)
                              opts2[4],opts2[5],   // Cnt Vis/Rec Flags
                              opts2[6],opts2[7],   // Avg Vis/Rec Flags
                              opts2[8].toFloat(),  // Electrode Cart. Coords
                              opts2[9].toFloat()); // (Theta,Phi)
         dummyChn->pastData.resize(cp.cntPastSize);
         dummyChn->pastFilt.resize(cp.cntPastSize); // Line-noise component
         dummyChn->setEventProfile(acqEvents.size(),cp.avgCount);
         for (int amp=0;amp<acqChannels.size();amp++) acqChannels[amp].append(dummyChn); // add channel for all respective amplifiers
        }
       } else { qDebug() << "octopus_acq_client: <AcqMaster> <.conf> Parse error in CHN|APPEND parameters!"; application->quit(); }
      } else { qDebug() << "octopus_acq_client: <AcqMaster> <.conf> Parse error in CHN sections!"; application->quit(); }
     }
    }

    if (guiSection.size()>0) { // GUI
     for (int i=0;i<guiSection.size();i++) { opts=guiSection[i].split("=");
      if (opts[0].trimmed()=="CTRL") { opts2=opts[1].split(",");
       if (opts2.size()==4) {
        ctrlGuiX=opts2[0].toInt(); ctrlGuiY=opts2[1].toInt(); ctrlGuiW=opts2[2].toInt(); ctrlGuiH=opts2[3].toInt();
        if ((!(ctrlGuiX >= -4000 && ctrlGuiX <= 4000)) || (!(ctrlGuiY >= -3000 && ctrlGuiY <= 3000)) ||
            (!(ctrlGuiW >=   400 && ctrlGuiW <= 2000)) || (!(ctrlGuiH >=    60 && ctrlGuiH <= 1800))) {
         qDebug() << "octopus_acq_client: <AcqMaster> <.conf> GUI|CTRL size settings not in appropriate range!"; application->quit();
        }
       } else {
        qDebug() << "octopus_acq_client: <AcqMaster> <.conf> Parse error in GUI settings!"; application->quit();
       }
     } else if (opts[0].trimmed()=="STRM") { opts2=opts[1].split(",");
       if (opts2.size()==4) {
        contGuiX=opts2[0].toInt(); contGuiY=opts2[1].toInt(); contGuiW=opts2[2].toInt(); contGuiH=opts2[3].toInt();
        if ((!(contGuiX >= -4000 && contGuiX <= 4000)) || (!(contGuiY >= -3000 && contGuiY <= 3000)) ||
            (!(contGuiW >=   400 && contGuiW <= 4000)) || (!(contGuiH >=   800 && contGuiH <= 4000))) {
         qDebug() << "octopus_acq_client: <AcqMaster> <.conf> GUI|STRM size settings not in appropriate range!"; application->quit();
        }
       } else {
        qDebug() << "octopus_acq_client: <AcqMaster> <.conf> Parse error in GUI settings!"; application->quit();
       }
      } else if (opts[0].trimmed()=="HEAD") { opts2=opts[1].split(",");
       if (opts2.size()==4) {
        gl3DGuiX=opts2[0].toInt(); gl3DGuiY=opts2[1].toInt(); gl3DGuiW=opts2[2].toInt(); gl3DGuiH=opts2[3].toInt();
        if ((!(gl3DGuiX >= -2000 && gl3DGuiX <= 2000)) || (!(gl3DGuiY >= -2000 && gl3DGuiY <= 2000)) ||
            (!(gl3DGuiW >=   400 && gl3DGuiW <= 2000)) || (!(gl3DGuiH >=   300 && gl3DGuiH <= 2000))) {
         qDebug() << "octopus_acq_client: <AcqMaster> <.conf> GUI|HEAD size settings not in appropriate range!"; application->quit();
        }
       } else {
        qDebug() << "octopus_acq_client: <AcqMaster> <.conf> Parse error in GUI Head Widget settings!"; application->quit();
       }
      } else { qDebug() << "octopus_acq_client: <AcqMaster> <.conf> Parse error in GUI sections!"; application->quit(); }
     }
    } else {
     ctrlGuiX=60; ctrlGuiY=60; ctrlGuiW=640; ctrlGuiH=480;
     contGuiX=60; contGuiY=60; contGuiW=640; contGuiH=480;
    }

    if (modSection.size()>0) { // MOD
     for (int i=0;i<modSection.size();i++) { opts=modSection[i].split("=");
      if (opts[0].trimmed()=="GIZMO") { opts2=opts[1].split(",");
       if (opts2.size()==1) {
        if (opts2[0].size()) loadGizmo_OgzFile(opts2[0].trimmed());
        else { qDebug() << "octopus_acq_client: <AcqMaster> <.conf> MOD|GIZMO filename error!"; application->quit(); }
       } else { qDebug() << "octopus_acq_client: <AcqMaster> <.conf> Parse error in MOD|GIZMO parameters!"; application->quit(); }
      } else if (opts[0].trimmed()=="SCALP") { opts2=opts[1].split(",");
       if (opts2.size()==1) loadScalp_ObjFile(opts2[0].trimmed());
       else { qDebug() << "octopus_acq_client: <AcqMaster> <.conf> Parse error in MOD|SCALP parameters!"; application->quit(); }
      } else if (opts[0].trimmed()=="SKULL") { opts2=opts[1].split(",");
       if (opts2.size()==1) loadSkull_ObjFile(opts2[0].trimmed());
       else { qDebug() << "octopus_acq_client: <AcqMaster> <.conf> Parse error in MOD|SKULL parameters!"; application->quit(); }
      } else if (opts[0].trimmed()=="BRAIN") { opts2=opts[1].split(",");
       if (opts2.size()==1) loadBrain_ObjFile(opts2[0].trimmed());
       else { qDebug() << "octopus_acq_client: <AcqMaster> <.conf> Parse error in MOD|BRAIN parameters!"; application->quit(); }
      } else { qDebug() << "octopus_acq_client: <AcqMaster> <.conf> Parse error in MOD sections!"; application->quit(); }
     }
    }
   }

   cntVisChns.resize(ampCount); cntRecChns.resize(ampCount); avgVisChns.resize(ampCount); avgRecChns.resize(ampCount);
   scrPrvData.resize(ampCount); scrCurData.resize(ampCount); scrPrvDataF.resize(ampCount); scrCurDataF.resize(ampCount);

   ampChkP.resize(ampCount);

   // *** POST SETUP ***

   acqFrameH=contGuiH-90; acqFrameW=(int)(.66*(float)(contGuiW)); if (acqFrameW%2==1) acqFrameW--;
   glFrameW=(int)(.33*(float)(contGuiW)); if (glFrameW%2==1) { glFrameW--; } glFrameH=glFrameW;
   
   cntAmpX.resize(ampCount); avgAmpX.resize(ampCount);
   gizmoOnReal.resize(ampCount); elecOnReal.resize(ampCount);
   hwFrameV.resize(ampCount); hwGridV.resize(ampCount); hwDigV.resize(ampCount);
   hwParamV.resize(ampCount); hwRealV.resize(ampCount); hwGizmoV.resize(ampCount);
   hwAvgsV.resize(ampCount); hwScalpV.resize(ampCount); hwSkullV.resize(ampCount);
   hwBrainV.resize(ampCount);
   currentGizmo.resize(ampCount); currentElectrode.resize(ampCount); curElecInSeq.resize(ampCount);
   scalpParamR.resize(ampCount); scalpNasion.resize(ampCount); scalpCzAngle.resize(ampCount);
   
   for (unsigned int i=0;i<ampCount;i++) { gizmoOnReal[i]=elecOnReal[i]=false;
    // Initial Visualization of Head Window
    hwFrameV[i]=hwGridV[i]=hwDigV[i]=hwParamV[i]=hwRealV[i]=hwGizmoV[i]=hwAvgsV[i]=hwScalpV[i]=hwSkullV[i]=hwBrainV[i]=true;
    currentGizmo[i]=currentElectrode[i]=curElecInSeq[i]=0; scalpParamR[i]=15.; scalpNasion[i]=9.; scalpCzAngle[i]=11.;
   }

   for (int i=0;i<acqChannels.size();i++) {
    for (int j=0;j<acqChannels[i].size();j++) { // Channel visibitility & recording
     if (acqChannels[i][j]->cntVis) cntVisChns[i].append(j);
     if (acqChannels[i][j]->cntRec) cntRecChns[i].append(i);
     if (acqChannels[i][j]->avgVis) avgVisChns[i].append(i);
     if (acqChannels[i][j]->avgRec) avgRecChns[i].append(i);
    }
   }

   for (int i=0;i<acqChannels.size();i++) {
    scrPrvData[i].resize(cntVisChns[i].size()); scrCurData[i].resize(cntVisChns[i].size());
    scrPrvDataF[i].resize(cntVisChns[i].size()); scrCurDataF[i].resize(cntVisChns[i].size());
   }
   
   digitizer=new Digitizer(this,&serial); digitizer->serialOpen();
   if (!digitizer->connected) qDebug() << "octopus_acq_client: <AcqMaster> Cannot connect to Polhemus digitizer!.. Continuing without it..";
   else { for (unsigned int i=0;i<ampCount;i++) digExists[i]=true;
    connect(digitizer,SIGNAL(digMonitor()),this,SLOT(slotDigMonitor())); connect(digitizer,SIGNAL(digResult()),this,SLOT(slotDigResult()));
   }

   // Begin retrieving continuous data
   acqDataSocket->connectToHost(acqHost,acqDataPort,QIODevice::ReadOnly); acqDataSocket->waitForConnected();
   connect(acqDataSocket,SIGNAL(readyRead()),this,SLOT(slotAcqReadData()));

   clientRunning=true;
  }

  // *** UTILITY ROUTINES ***

  int eventIndex(int no,int type) { int idx=-1;
   for (int i=0;i<acqEvents.size();i++) {
    if (no==acqEvents[i]->no && type==acqEvents[i]->type) { idx=i; break; }
   } return idx;
  }

  bool clientRunning,recording,withinAvgEpoch,eventOccured; AmpInfo ampInfo;

  // Non-volatile (read from and saved to octopus.cfg)

  // NET
  QString acqHost; int acqCommPort,acqDataPort;

  // CHN
  QVector<QVector<Channel*> > acqChannels; QVector<Event*> acqEvents;
  QVector<QVector<int> > cntVisChns,cntRecChns,avgVisChns,avgRecChns;

  // GUI

  // Gizmo
  QVector<Gizmo* > gizmo; QVector<bool> gizmoOnReal,elecOnReal;
  QVector<int> currentGizmo;
  QVector<int> currentElectrode;
  QVector<int> curElecInSeq;

  // Digitizer
  Vec3 sty,xp,yp,zp;

  // Volatile-Runtime
  QApplication *application; cs_command csCmd,csAck; QTcpSocket *acqCommandSocket,*acqDataSocket;
  QStatusBar *guiStatusBar; QLabel *timeLabel;

  bool notch; int notchN; float notchThreshold;

  QVector<tcpsample> acqCurData; int eIndex; channel_params cp; int tChns,sampleRate,cntSpeedX;
  QVector<QVector<float> > scrPrvData,scrCurData,scrPrvDataF,scrCurDataF; QVector<float> cntAmpX,avgAmpX;
  QString curEventName; int curEventType;

  bool gizmoExists;
  QVector<bool> hwFrameV,hwGridV,hwDigV,hwParamV,hwRealV,hwGizmoV,hwAvgsV,hwScalpV,hwSkullV,hwBrainV,
                digExists,scalpExists,skullExists,brainExists;

//  QVector<Coord3D> paramCoord,realCoord; QVector<QVector<int> > paramIndex;
  QVector<Coord3D> scalpCoord,skullCoord,brainCoord; QVector<QVector<int> > scalpIndex,skullIndex,brainIndex;
  QVector<float> scalpParamR,scalpNasion,scalpCzAngle;

 signals:
  void scrData(bool,bool); void repaintGL(int); void repaintHeadWindow(); void repaintLegend();

  void slotClrAvgs() {
   for (int i=0;i<acqChannels.size();i++) acqChannels[0][i]->resetEvents();
   for (int i=0;i<acqEvents.size();i++) { acqEvents[i]->accepted=acqEvents[i]->rejected=0; }
   emit repaintGL(16); emit repaintHeadWindow();
  }

     // STREAMING/RECORDING, 50Hz COMPUTATION and ONLINE AVG is enabled..

     acqCurEvent=(int)(acqCurData[dOffset].trigger); // Event

     if (recording) { // .. to disk ..
      for (int i=0;i<cntRecChns.size();i++) {
       curChn=acqChannels[0][cntRecChns[0][i]];
       //if (curChn->ampNo==1) cntStream << acqCurData[dOffset].amp[0].data[curChn->physChn];
       //else cntStream << acqCurData[dOffset].amp[1].data[curChn->physChn];
      }
      cntStream << acqCurEvent;
      recCounter++; if (!(recCounter%sampleRate)) updateRecTime();
     }

     // Handle backward data..
     //  Put data into suitable offset for backward online averaging..
     float dummyAvg; int notchCount=notchN*(ampInfo.sampleRate/50); int notchStart=(cp.cntPastSize+cp.cntPastIndex-notchCount)%cp.cntPastSize; // -1 ?
     for (int j=0;j<acqChannels.size();j++) {
      curChn=acqChannels[0][j];
      //curChn->pastData[cp.cntPastIndex] = (curChn->ampNo==1) ? acqCurData[dOffset].amp[0].data[curChn->physChn] : acqCurData[dOffset].amp[1].data[curChn->physChn];
      //curChn->pastFilt[cp.cntPastIndex] = (curChn->ampNo==1) ? acqCurData[dOffset].amp[0].dataF[curChn->physChn] : acqCurData[dOffset].amp[1].dataF[curChn->physChn];

      // Compute Absolute "50Hz+Harmonics" Level of that channel..
      dummyAvg=0.; for (int k=0;k<notchCount;k++) dummyAvg+=abs(curChn->pastFilt[(notchStart+k)%cp.cntPastSize]);
      dummyAvg/=(float)notchCount; //dummyAvg/=0.6; // Level of avg of sine..
      curChn->notchLevel=dummyAvg; // /10.; // Normalization
      if (curChn->notchLevel < notchThreshold) curChn->notchColor=QColor(0,255,0,144); else curChn->notchColor=QColor(255,0,0,144); // Green vs. Red
     }

     // Handle Incoming Event..
     if (acqCurEvent) { event=true; curEventName="STIM event #"; curEventName+=dummyString.setNum(acqCurEvent);
      int idx=eventIndex(acqCurEvent,1);
      if (idx>=0) { eIndex=idx; curEventName=acqEvents[eIndex]->name;
       qDebug() << "octopus_acq_client: <AcqMaster> <AcqReadData> <IncomingEvent> Avg! (Index,Name)->" << eIndex << curEventName;
       if (withinAvgEpoch) {
        qDebug() << "octopus_acq_client: <AcqMaster> <AcqReadData> <IncomingEvent> Event collision!.. (already within process of averaging).." << avgCounter << cp.rejCount;
       } else { withinAvgEpoch=true; avgCounter=0; }
      }
     }

     if (withinAvgEpoch) {
      if (avgCounter==cp.bwCount) { withinAvgEpoch=false;
       qDebug() << "octopus_acq_client: <AcqMaster> <AcqReadData> <WithinEpoch> Computing for Event! (iIndex,Name)->" << eIndex << acqEvents[eIndex]->name;
 
       // Check rejection backwards on pastdata
       bool rejFlag=false; int rejChn=0;
       for (int i=0;i<acqChannels.size();i++) for (int j=0;j<acqChannels[i].size();j++) {
        if (acqChannels[i][j]->rejLev>0) {
         for (int j=0;j<cp.rejCount;j++) { unsigned int idx=(cp.cntPastSize+cp.cntPastIndex-cp.rejCount+j)%cp.cntPastSize;
          unsigned int ref=acqChannels[i][j]->rejRef;
          float chRejLev=abs(acqChannels[i][j]->pastData[idx]-acqChannels[i][ref]->pastData[idx]);
          if (chRejLev > acqChannels[i][j]->rejLev) { rejFlag=true; rejChn=i; break; }
         }
        } if (rejFlag==true) break;
       }

       if (rejFlag) { // Rejected, increment rejected count
        acqEvents[eIndex]->rejected++;
        qDebug() << "octopus_acq_client: <AcqMaster> <AcqReadData> <Reject> Rejected because of" << acqChannels[0][rejChn]->name << "..";
       } else { // Not rejected: compute average and increment accepted for the event
        acqEvents[eIndex]->accepted++; eventOccured=true;
        qDebug() << "octopus_acq_client: <AcqMaster> <AcqReadData> <Reject> Computing average for eventIndex and updating GUI..";
 
        for (int i=0;i<acqChannels.size();i++) for (int j=0;j<acqChannels[i].size();j++) {
         avgStartOffset=(cp.cntPastSize+cp.cntPastIndex-cp.avgCount-cp.postRejCount)%cp.cntPastSize;
         avgInChn=&(acqChannels[i][j]->avgData)[eIndex];
	 //stdInChn=&(acqChannels[i][j]->stdData)[eIndex];
         n1=(float)(acqEvents[eIndex]->accepted); //n2=1
         avgDataCount=avgInChn->size();
         for (unsigned int k=0;k<avgDataCount;k++) { k1=(*avgInChn)[k]; k2=acqChannels[i][j]->pastData[(avgStartOffset+j)%cp.cntPastSize]; (*avgInChn)[k]=(k1*n1+k2)/(n1+1.); }
        } emit repaintGL(16); emit repaintHeadWindow();
       }
      }
     } // averaging
     
     avgCounter++;

     cp.cntPastIndex++; cp.cntPastIndex%=cp.cntPastSize;

     if (!scrCounter) {
      for (unsigned int i=0;i<ampCount;i++) for (int j=0;j<scrCurData[i].size();j++) { curChn=acqChannels[i][j];
       scrPrvData[i][j]=scrCurData[i][j]; scrCurData[i][j]=acqCurData[dOffset].amp[i].data[curChn->physChn];
       scrPrvDataF[i][j]=scrCurDataF[i][j]; scrCurDataF[i][j]=acqCurData[dOffset].amp[i].dataF[curChn->physChn];
      } emit scrData(tick,event); tick=event=false; // Update CntFrame
     } scrCounter++; scrCounter%=cntSpeedX;
     if (!seconds) emit repaintGL(2+4); // Update 50Hz visualization..
     seconds++; seconds%=sampleRate; if (seconds==0) tick=true;
    } // dOffset
   } // bytesAvailable
  } // acqReadData

  void slotReboot() { acqSendCommand(CS_REBOOT,0,0,0); guiStatusBar->showMessage("ACQ server is rebooting..",5000); }
  void slotShutdown() { acqSendCommand(CS_SHUTDOWN,0,0,0); guiStatusBar->showMessage("ACQ server is shutting down..",5000); }
  
  void slotQuit() {
   if (digitizer->connected) digitizer->serialClose();
   acqDataSocket->disconnectFromHost();
   if (acqDataSocket->state()==QAbstractSocket::UnconnectedState || acqDataSocket->waitForDisconnected(1000)) application->exit(0);
  }

  //  GUI TOP LEFT BUTTONS RELATED TO RECORDING/EVENTS/TRIGGERS

  void slotToggleRecording() { QDateTime currentDT(QDateTime::currentDateTime());
   if (!recording) {
    // Generate filename using current date and time, add current data and time to base: trial-20071228-123012-332.oeg
    QString cntFN="trial-"+currentDT.toString("yyyyMMdd-hhmmss-zzz"); cntFile.setFileName(cntFN+".oeg");
    if (!cntFile.open(QIODevice::WriteOnly)) {
     qDebug() << "octopus_acq_client: <AcqMaster> <ToggleRec> Error: Cannot open .occ file for writing."; return;
    } cntStream.setDevice(&cntFile);

    cntStream << (int)(OCTOPUS_ACQ_CLIENT_VER);	// Version
    cntStream << sampleRate;		        // Sample rate
    cntStream << cntRecChns.size();	        // Channel count

    for (int i=0;i<cntRecChns.size();i++) cntStream << acqChannels[0][cntRecChns[0][i]]->name; // Channel names - Cstyle
     
    for (int i=0;i<cntRecChns.size();i++) { // Param coords
     cntStream << acqChannels[0][cntRecChns[0][i]]->param.y;
     cntStream << acqChannels[0][cntRecChns[0][i]]->param.z;
    }
    for (int i=0;i<cntRecChns.size();i++) { // Real/measured coords
     cntStream << acqChannels[0][cntRecChns[0][i]]->real[0];
     cntStream << acqChannels[0][cntRecChns[0][i]]->real[1];
     cntStream << acqChannels[0][cntRecChns[0][i]]->real[2];
     cntStream << acqChannels[0][cntRecChns[0][i]]->realS[0];
     cntStream << acqChannels[0][cntRecChns[0][i]]->realS[1];
     cntStream << acqChannels[0][cntRecChns[0][i]]->realS[2];
    }

    cntStream << acqEvents.size();	       // Event count
    for (int i=0;i<acqEvents.size();i++) {     // Event Info of the session
     cntStream << acqEvents[i]->no;	       // Event #
     cntStream << acqEvents[i]->name;          // Name - Cstyle
     cntStream << acqEvents[i]->type;	       // STIM or RESP
     cntStream << acqEvents[i]->color.red();   // Color
     cntStream << acqEvents[i]->color.green();
     cntStream << acqEvents[i]->color.blue();
    }
    
    // Here continuous data begins..
    timeLabel->setText("Rec.Time: 00:00:00"); recCounter=0; recording=true;
   } else { recording=false; cntStream.setDevice(0); cntFile.close(); }
  }

  void slotManualTrig() { acqSendCommand(CS_ACQ_MANUAL_TRIG,AMP_SIMU_TRIG,0,0); }
  void slotManualSync() { acqSendCommand(CS_ACQ_MANUAL_SYNC,AMP_SYNC_TRIG,0,0); }

  // *** TCP HANDLERS



 private: // Used Just-In-Time..
  void updateRecTime() { int s,m,h; s=recCounter/sampleRate; m=s/60; h=m/60;
   if (h<10) rHour="0"; else rHour=""; rHour+=dummyString.setNum(h);
   if (m<10) rMin="0"; else rMin=""; rMin+=dummyString.setNum(m);
   if (s<10) rSec="0"; else rSec=""; rSec+=dummyString.setNum(s);
   timeLabel->setText("Rec.Time: "+rHour+":"+rMin+":"+rSec);
  }

  bool tick,event; int seconds,cntBufIndex,scrCounter,recCounter,avgCounter; QObject *recorder; unsigned int ampCount; QString rHour,rMin,rSec,dummyString;

  QFile cfgFile,cntFile,avgFile; QTextStream cfgStream; QDataStream cntStream,avgStream;
  serial_device serial; Digitizer *digitizer; Event *dummyEvt; Channel *dummyChn,*curChn; QVector<unsigned int> ampChkP; quint64 globalCounter;
  */
