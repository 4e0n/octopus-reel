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

/* a.k.a. "The Engine"..
    This is the predefined parameters, configuration and management class
    shared over all other classes of octopus-acq-client. */

#ifndef ACQMASTER_H
#define ACQMASTER_H

#include <QObject>
#include <QApplication>
#include <QWidget>
#include <QStatusBar>
#include <QLabel>
#include <QtNetwork>
#include <QFile>
#include <QFileDialog>
#include <QTextStream>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QMutex>

#include <cmath>

#include "../acqglobals.h"
#include "../serial_device.h"
#include "../ampinfo.h"
#include "headmodel.h"
#include "digitizer.h"
#include "configparser.h"

//#include "channel_params.h"
//#include "channel.h"
//#include "../../common/event.h"
//#include "../../common/gizmo.h"
//#include "coord3d.h"
//#include "../cs_command.h"
//#include "../sample.h"
//#include "../tcpsample.h"
//#include "chninfo.h"
//#include "../patt_datagram.h"
//#include "../stim_event_names.h"
//#include "../resp_event_names.h"


const int OCTOPUS_ACQ_CLIENT_VER=180;

class AcqMaster : QObject {
 Q_OBJECT
 public:
  AcqMaster(QApplication *app) : QObject() { application=app;

   qDebug() << "---------------------------------------------------------------";

   ampCount=EE_AMPCOUNT;
   headModel=new HeadModel(ampCount);
   ConfigParser cfp(application);
   cfp.parse("/etc/octopus_acq_client.conf",&conf,&chnTopo,&serial,headModel);

   // *** INITIAL VALUES OF RUNTIME VARIABLES ***

//   clientRunning=recording=withinAvgEpoch=eventOccured=false;
//   seconds=cp.cntPastIndex=avgCounter=0; cntSpeedX=4; globalCounter=scrCounter=0;
//   notch=true; notchN=20; notchThreshold=20.;


   // -------------------------------------------

//   cntVisChns.resize(ampCount); cntRecChns.resize(ampCount); avgVisChns.resize(ampCount); avgRecChns.resize(ampCount);
//   scrPrvData.resize(ampCount); scrCurData.resize(ampCount); scrPrvDataF.resize(ampCount); scrCurDataF.resize(ampCount);

//   ampChkP.resize(ampCount);

   // *** POST SETUP ***

//   acqFrameH=contGuiH-90; acqFrameW=(int)(.66*(float)(contGuiW)); if (acqFrameW%2==1) acqFrameW--;
//   glFrameW=(int)(.33*(float)(contGuiW)); if (glFrameW%2==1) { glFrameW--; } glFrameH=glFrameW;
   
//   cntAmpX.resize(ampCount); avgAmpX.resize(ampCount);
//   gizmoOnReal.resize(ampCount); elecOnReal.resize(ampCount);
//   hwFrameV.resize(ampCount); hwGridV.resize(ampCount); hwDigV.resize(ampCount);
//   hwParamV.resize(ampCount); hwRealV.resize(ampCount); hwGizmoV.resize(ampCount);
//   hwAvgsV.resize(ampCount); hwScalpV.resize(ampCount); hwSkullV.resize(ampCount);
//   hwBrainV.resize(ampCount);
//   currentGizmo.resize(ampCount); currentElectrode.resize(ampCount); curElecInSeq.resize(ampCount);
//   scalpParamR.resize(ampCount); scalpNasion.resize(ampCount); scalpCzAngle.resize(ampCount);
   
//   for (unsigned int i=0;i<ampCount;i++) { gizmoOnReal[i]=elecOnReal[i]=false;
//    // Initial Visualization of Head Window
//    hwFrameV[i]=hwGridV[i]=hwDigV[i]=hwParamV[i]=hwRealV[i]=hwGizmoV[i]=hwAvgsV[i]=hwScalpV[i]=hwSkullV[i]=hwBrainV[i]=true;
//    currentGizmo[i]=currentElectrode[i]=curElecInSeq[i]=0; scalpParamR[i]=15.; scalpNasion[i]=9.; scalpCzAngle[i]=11.;
//   }

//   for (int i=0;i<acqChannels.size();i++) {
//    for (int j=0;j<acqChannels[i].size();j++) { // Channel visibitility & recording
//     if (acqChannels[i][j]->cntVis) cntVisChns[i].append(j);
//     if (acqChannels[i][j]->cntRec) cntRecChns[i].append(i);
//     if (acqChannels[i][j]->avgVis) avgVisChns[i].append(i);
//     if (acqChannels[i][j]->avgRec) avgRecChns[i].append(i);
//    }
//   }

//   for (int i=0;i<acqChannels.size();i++) {
//    scrPrvData[i].resize(cntVisChns[i].size()); scrCurData[i].resize(cntVisChns[i].size());
//    scrPrvDataF[i].resize(cntVisChns[i].size()); scrCurDataF[i].resize(cntVisChns[i].size());
//   }
   
//   digitizer=new Digitizer(this,&serial); digitizer->serialOpen();
//   if (!digitizer->connected) qDebug() << "octopus_acq_client: <AcqMaster> Cannot connect to Polhemus digitizer!.. Continuing without it..";
//   else { for (unsigned int i=0;i<ampCount;i++) digExists[i]=true;
//    connect(digitizer,SIGNAL(digMonitor()),this,SLOT(slotDigMonitor())); connect(digitizer,SIGNAL(digResult()),this,SLOT(slotDigResult()));
//   }

   // Begin retrieving continuous data
//   acqDataSocket->connectToHost(acqHost,acqDataPort,QIODevice::ReadOnly); acqDataSocket->waitForConnected();
//   connect(acqDataSocket,SIGNAL(readyRead()),this,SLOT(slotAcqReadData()));

//   clientRunning=true;
  }

//  unsigned int getAmpCount() { return ampCount; }

  // *** EXTERNAL OBJECT REGISTRY ***

//  void registerScrollHandler(QObject *sh) { connect(this,SIGNAL(scrData(bool,bool)),sh,SLOT(slotScrData(bool,bool))); }
//  void regRepaintGL(QObject *sh) { connect(this,SIGNAL(repaintGL(int)),sh,SLOT(slotRepaintGL(int))); }
//  void regRepaintHeadWindow(QObject *sh) { connect(this,SIGNAL(repaintHeadWindow()),sh,SLOT(slotRepaint())); }
//  void regRepaintLegendHandler(QObject *sh) { connect(this,SIGNAL(repaintLegend()),sh,SLOT(slotRepaintLegend())); }

  // *** UTILITY ROUTINES ***

//  void acqSendCommand(int command,int ip0,int ip1,int ip2) {
//   acqCommandSocket->connectToHost(acqHost,acqCommPort); acqCommandSocket->waitForConnected();
//   QDataStream acqCommandStream(acqCommandSocket); csCmd.cmd=command; csCmd.iparam[0]=ip0; csCmd.iparam[1]=ip1; csCmd.iparam[2]=ip2;
//   acqCommandStream.writeRawData((const char*)(&csCmd),sizeof(cs_command)); acqCommandSocket->flush(); acqCommandSocket->disconnectFromHost();
//  }

//  int eventIndex(int no,int type) { int idx=-1;
//   for (int i=0;i<acqEvents.size();i++) {
//    if (no==acqEvents[i]->no && type==acqEvents[i]->type) { idx=i; break; }
//   } return idx;
//  }

//  bool clientRunning,recording,withinAvgEpoch,eventOccured; AmpInfo ampInfo;

  // Non-volatile (read from and saved to octopus.cfg)

  // NET
//  QString acqHost; int acqCommPort,acqDataPort;

  // CHN
  //QVector<QVector<Channel*> > acqChannels;
//  QVector<Event*> acqEvents;
//  QVector<QVector<int> > cntVisChns,cntRecChns,avgVisChns,avgRecChns;

  // GUI
//  int ctrlGuiX,ctrlGuiY,ctrlGuiW,ctrlGuiH, contGuiX,contGuiY,contGuiW,contGuiH, gl3DGuiX,gl3DGuiY,gl3DGuiW,gl3DGuiH,
//      acqFrameW,acqFrameH,glFrameW,glFrameH;

  // Digitizer
//  Vec3 sty,xp,yp,zp;

  // Volatile-Runtime
  QApplication *application;
//  cs_command csCmd,csAck; QTcpSocket *acqCommandSocket,*acqDataSocket;
//  QStatusBar *guiStatusBar; QLabel *timeLabel;

//  bool notch; int notchN; float notchThreshold;

//  QVector<tcpsample> acqCurData; int eIndex; channel_params cp; int tChns,sampleRate,cntSpeedX;
//  QVector<QVector<float> > scrPrvData,scrCurData,scrPrvDataF,scrCurDataF; QVector<float> cntAmpX,avgAmpX;
//  QString curEventName; int curEventType;

  serial_device serial; Digitizer *digitizer;
  HeadModel *headModel; ConfParam conf; QVector<ChnInfo> chnTopo;

 signals:
//  void scrData(bool,bool); void repaintGL(int); void repaintHeadWindow(); void repaintLegend();

 private slots:

//  void slotExportAvgs() { QString avgFN;
//   QDateTime currentDT(QDateTime::currentDateTime());
//   for (int i=0;i<acqEvents.size();i++) {
//    if (acqEvents[i]->accepted || acqEvents[i]->rejected) { //Any event exists?
//     // Generate filename using current date and time
//     // add current data and time to base: trial-20071228-123012-332.oeg
//     QString avgFN="Event-"+acqEvents[i]->name+currentDT.toString("yyyyMMdd-hhmmss-zzz");
//     avgFile.setFileName(avgFN+".oep");
//     if (!avgFile.open(QIODevice::WriteOnly)) {
//      qDebug() << "octopus_acq_client: <AcqMaster> <ExportAvgs> Error: Cannot open .oep file for writing."; return;
//     } avgStream.setDevice(&avgFile);
//     avgStream << (int)(OCTOPUS_ACQ_CLIENT_VER); // Version
//     avgStream << sampleRate;		// Sample rate
//     avgStream << avgRecChns.size();	// Channel count
//     avgStream << acqEvents[i]->name;   // Name of Evt - Cstyle
//     avgStream << cp.avgBwd;		// Averaging Window
//     avgStream << cp.avgFwd;
//     avgStream << acqEvents[i]->accepted; // Accepted count
//     avgStream << acqEvents[i]->rejected; // Rejected count

//     for (int j=0;j<cp.avgCount;j++) {
//      for (int k=0;k<avgRecChns.size();k++) avgStream << &(acqChannels[0][avgRecChns[0][k]]->avgData)[j];
//      for (int k=0;k<avgRecChns.size();k++) avgStream << &(acqChannels[0][avgRecChns[0][k]]->stdData)[j];
//     } avgFile.close();
//    }
//   }
//  }

//  void slotClrAvgs() {
//   for (int i=0;i<acqChannels.size();i++) acqChannels[0][i]->resetEvents();
//   for (int i=0;i<acqEvents.size();i++) { acqEvents[i]->accepted=acqEvents[i]->rejected=0; }
//   emit repaintGL(16); emit repaintHeadWindow();
//  }

//  void slotAcqReadData() {
//   unsigned int acqCurEvent,avgDataCount,avgStartOffset; QVector<float> *avgInChn; //,*stdInChn;
//   float n1,k1,k2; unsigned int offsetC,offsetP;
//   QDataStream acqDataStream(acqDataSocket);

//   while (acqDataSocket->bytesAvailable() >= ampInfo.probe_eeg_msecs*(unsigned int)(sizeof(tcpsample))) {
//    acqDataStream.readRawData((char*)(acqCurData.data()),ampInfo.probe_eeg_msecs*(unsigned int)(sizeof(tcpsample)));

//    for (unsigned int dOffset=0;dOffset<ampInfo.probe_eeg_msecs;dOffset++) {
//     // Check Sample Offset Delta for all amps
//     for (unsigned int i=0;i<ampCount;i++) {
//      offsetC=(unsigned int)(acqCurData[dOffset].amp[i].offset); offsetP=ampChkP[i]; ampChkP[i]=offsetC;
//      if ((offsetC-offsetP)!=1)
//       qDebug() << "octopus_acq_client: <AcqMaster> <AcqReadData> Offset leak!!! Amp " << i << " OffsetC->" << offsetC << " OffsetP->" << offsetP;
//     }

//     if (!(globalCounter%10000)) { // Sort ampChkP and print
//      qDebug() << "octopus_acq_client: <AcqMaster> <AcqReadData> Interamp actual sample count Delta span ->" << abs((int)ampChkP[1]-(int)ampChkP[0]);
//     } globalCounter++;

     // STREAMING/RECORDING, 50Hz COMPUTATION and ONLINE AVG is enabled..

//     acqCurEvent=(int)(acqCurData[dOffset].trigger); // Event

//     if (recording) { // .. to disk ..
//      for (int i=0;i<cntRecChns.size();i++) {
//       curChn=acqChannels[0][cntRecChns[0][i]];
//       //if (curChn->ampNo==1) cntStream << acqCurData[dOffset].amp[0].data[curChn->physChn];
//       //else cntStream << acqCurData[dOffset].amp[1].data[curChn->physChn];
//      }
//      cntStream << acqCurEvent;
//      recCounter++; if (!(recCounter%sampleRate)) updateRecTime();
//     }

     // Handle backward data..
     //  Put data into suitable offset for backward online averaging..
//     float dummyAvg; int notchCount=notchN*(ampInfo.sampleRate/50); int notchStart=(cp.cntPastSize+cp.cntPastIndex-notchCount)%cp.cntPastSize; // -1 ?
//     for (int j=0;j<acqChannels.size();j++) {
//      curChn=acqChannels[0][j];
      //curChn->pastData[cp.cntPastIndex] = (curChn->ampNo==1) ? acqCurData[dOffset].amp[0].data[curChn->physChn] : acqCurData[dOffset].amp[1].data[curChn->physChn];
      //curChn->pastFilt[cp.cntPastIndex] = (curChn->ampNo==1) ? acqCurData[dOffset].amp[0].dataF[curChn->physChn] : acqCurData[dOffset].amp[1].dataF[curChn->physChn];

      // Compute Absolute "50Hz+Harmonics" Level of that channel..
//      dummyAvg=0.; for (int k=0;k<notchCount;k++) dummyAvg+=abs(curChn->pastFilt[(notchStart+k)%cp.cntPastSize]);
//      dummyAvg/=(float)notchCount; //dummyAvg/=0.6; // Level of avg of sine..
//      curChn->notchLevel=dummyAvg; // /10.; // Normalization
//      if (curChn->notchLevel < notchThreshold) curChn->notchColor=QColor(0,255,0,144); else curChn->notchColor=QColor(255,0,0,144); // Green vs. Red
//     }

     // Handle Incoming Event..
//     if (acqCurEvent) { event=true; curEventName="STIM event #"; curEventName+=dummyString.setNum(acqCurEvent);
//      int idx=eventIndex(acqCurEvent,1);
//      if (idx>=0) { eIndex=idx; curEventName=acqEvents[eIndex]->name;
//       qDebug() << "octopus_acq_client: <AcqMaster> <AcqReadData> <IncomingEvent> Avg! (Index,Name)->" << eIndex << curEventName;
//       if (withinAvgEpoch) {
//        qDebug() << "octopus_acq_client: <AcqMaster> <AcqReadData> <IncomingEvent> Event collision!.. (already within process of averaging).." << avgCounter << cp.rejCount;
//       } else { withinAvgEpoch=true; avgCounter=0; }
//      }
//     }

//     if (withinAvgEpoch) {
//      if (avgCounter==cp.bwCount) { withinAvgEpoch=false;
//       qDebug() << "octopus_acq_client: <AcqMaster> <AcqReadData> <WithinEpoch> Computing for Event! (iIndex,Name)->" << eIndex << acqEvents[eIndex]->name;
 
       // Check rejection backwards on pastdata
//       bool rejFlag=false; int rejChn=0;
//       for (int i=0;i<acqChannels.size();i++) for (int j=0;j<acqChannels[i].size();j++) {
//        if (acqChannels[i][j]->rejLev>0) {
//         for (int j=0;j<cp.rejCount;j++) { unsigned int idx=(cp.cntPastSize+cp.cntPastIndex-cp.rejCount+j)%cp.cntPastSize;
//          unsigned int ref=acqChannels[i][j]->rejRef;
//          float chRejLev=abs(acqChannels[i][j]->pastData[idx]-acqChannels[i][ref]->pastData[idx]);
//          if (chRejLev > acqChannels[i][j]->rejLev) { rejFlag=true; rejChn=i; break; }
//         }
//        } if (rejFlag==true) break;
//       }

//       if (rejFlag) { // Rejected, increment rejected count
//        acqEvents[eIndex]->rejected++;
//        qDebug() << "octopus_acq_client: <AcqMaster> <AcqReadData> <Reject> Rejected because of" << acqChannels[0][rejChn]->name << "..";
//       } else { // Not rejected: compute average and increment accepted for the event
//        acqEvents[eIndex]->accepted++; eventOccured=true;
//        qDebug() << "octopus_acq_client: <AcqMaster> <AcqReadData> <Reject> Computing average for eventIndex and updating GUI..";
 
//        for (int i=0;i<acqChannels.size();i++) for (int j=0;j<acqChannels[i].size();j++) {
//         avgStartOffset=(cp.cntPastSize+cp.cntPastIndex-cp.avgCount-cp.postRejCount)%cp.cntPastSize;
//         avgInChn=&(acqChannels[i][j]->avgData)[eIndex];
//	 //stdInChn=&(acqChannels[i][j]->stdData)[eIndex];
//         n1=(float)(acqEvents[eIndex]->accepted); //n2=1
//         avgDataCount=avgInChn->size();
//         for (unsigned int k=0;k<avgDataCount;k++) { k1=(*avgInChn)[k]; k2=acqChannels[i][j]->pastData[(avgStartOffset+j)%cp.cntPastSize]; (*avgInChn)[k]=(k1*n1+k2)/(n1+1.); }
//        } emit repaintGL(16); emit repaintHeadWindow();
//       }
//      }
//     } // averaging
     
//     avgCounter++;

//     cp.cntPastIndex++; cp.cntPastIndex%=cp.cntPastSize;

//     if (!scrCounter) {
//      for (unsigned int i=0;i<ampCount;i++) for (int j=0;j<scrCurData[i].size();j++) { curChn=acqChannels[i][j];
//       scrPrvData[i][j]=scrCurData[i][j]; scrCurData[i][j]=acqCurData[dOffset].amp[i].data[curChn->physChn];
//       scrPrvDataF[i][j]=scrCurDataF[i][j]; scrCurDataF[i][j]=acqCurData[dOffset].amp[i].dataF[curChn->physChn];
//      } emit scrData(tick,event); tick=event=false; // Update CntFrame
//     } scrCounter++; scrCounter%=cntSpeedX;
//     if (!seconds) emit repaintGL(2+4); // Update 50Hz visualization..
//     seconds++; seconds%=sampleRate; if (seconds==0) tick=true;
//    } // dOffset
//   } // bytesAvailable
//  } // acqReadData

//  void slotReboot() { acqSendCommand(CS_REBOOT,0,0,0); guiStatusBar->showMessage("ACQ server is rebooting..",5000); }
//  void slotShutdown() { acqSendCommand(CS_SHUTDOWN,0,0,0); guiStatusBar->showMessage("ACQ server is shutting down..",5000); }
  
//  void slotQuit() {
//   if (digitizer->connected) digitizer->serialClose();
//   acqDataSocket->disconnectFromHost();
//   if (acqDataSocket->state()==QAbstractSocket::UnconnectedState || acqDataSocket->waitForDisconnected(1000)) application->exit(0);
//  }

  // *** POLHEMUS HANDLER ***

//  void slotDigMonitor() {
//   digitizer->mutex.lock(); sty=digitizer->styF; xp=digitizer->xpF; yp=digitizer->ypF; zp=digitizer->zpF; digitizer->mutex.unlock(); emit repaintGL(1);
//  }

//  void slotDigResult() {
//   digitizer->mutex.lock();
//    for (unsigned int i=0;i<ampCount;i++) {
//     acqChannels[0][currentElectrode[i]]->real=digitizer->stylusF;
//     acqChannels[0][currentElectrode[i]]->realS=digitizer->stylusSF;
//    }
//   digitizer->mutex.unlock();
//   for (unsigned int i=0;i<ampCount;i++) curElecInSeq[i]++;
//   for (unsigned int i=0;i<ampCount;i++) {
//    if (curElecInSeq[i]==gizmo[currentGizmo[i]]->seq.size()) curElecInSeq[i]=0;
//    for (int j=0;j<acqChannels.size();j++) if (acqChannels[i][j]->physChn==gizmo[currentGizmo[i]]->seq[curElecInSeq[i]]) { currentElectrode[i]=j; break; }
//   }
//   emit repaintHeadWindow(); emit repaintGL(1);
//  }

  //  GUI TOP LEFT BUTTONS RELATED TO RECORDING/EVENTS/TRIGGERS

//  void slotToggleRecording() { QDateTime currentDT(QDateTime::currentDateTime());
//   if (!recording) {
//    // Generate filename using current date and time, add current data and time to base: trial-20071228-123012-332.oeg
//    QString cntFN="trial-"+currentDT.toString("yyyyMMdd-hhmmss-zzz"); cntFile.setFileName(cntFN+".oeg");
//    if (!cntFile.open(QIODevice::WriteOnly)) {
//     qDebug() << "octopus_acq_client: <AcqMaster> <ToggleRec> Error: Cannot open .occ file for writing."; return;
//    } cntStream.setDevice(&cntFile);

//    cntStream << (int)(OCTOPUS_ACQ_CLIENT_VER);	// Version
//    cntStream << sampleRate;		        // Sample rate
//    cntStream << cntRecChns.size();	        // Channel count

//    for (int i=0;i<cntRecChns.size();i++) cntStream << acqChannels[0][cntRecChns[0][i]]->name; // Channel names - Cstyle
     
//    for (int i=0;i<cntRecChns.size();i++) { // Param coords
//     cntStream << acqChannels[0][cntRecChns[0][i]]->param.y;
//     cntStream << acqChannels[0][cntRecChns[0][i]]->param.z;
//    }
//    for (int i=0;i<cntRecChns.size();i++) { // Real/measured coords
//     cntStream << acqChannels[0][cntRecChns[0][i]]->real[0];
//     cntStream << acqChannels[0][cntRecChns[0][i]]->real[1];
//     cntStream << acqChannels[0][cntRecChns[0][i]]->real[2];
//     cntStream << acqChannels[0][cntRecChns[0][i]]->realS[0];
//     cntStream << acqChannels[0][cntRecChns[0][i]]->realS[1];
//     cntStream << acqChannels[0][cntRecChns[0][i]]->realS[2];
//    }

//    cntStream << acqEvents.size();	       // Event count
//    for (int i=0;i<acqEvents.size();i++) {     // Event Info of the session
//     cntStream << acqEvents[i]->no;	       // Event #
//     cntStream << acqEvents[i]->name;          // Name - Cstyle
//     cntStream << acqEvents[i]->type;	       // STIM or RESP
//     cntStream << acqEvents[i]->color.red();   // Color
//     cntStream << acqEvents[i]->color.green();
//     cntStream << acqEvents[i]->color.blue();
//    }
    
//    // Here continuous data begins..
//    timeLabel->setText("Rec.Time: 00:00:00"); recCounter=0; recording=true;
//   } else { recording=false; cntStream.setDevice(0); cntFile.close(); }
//  }

//  void slotToggleNotch() { if (!notch) notch=true; else notch=false; }
//  void slotManualTrig() { acqSendCommand(CS_ACQ_MANUAL_TRIG,AMP_SIMU_TRIG,0,0); }
//  void slotManualSync() { acqSendCommand(CS_ACQ_MANUAL_SYNC,AMP_SYNC_TRIG,0,0); }

  // *** TCP HANDLERS

//  void slotAcqCommandError(QAbstractSocket::SocketError socketError) {
//   switch (socketError) {
//    case QAbstractSocket::HostNotFoundError: qDebug() << "octopus_acq_client: <AcqMaster> <AcqCmdErr> ACQuisition command server does not exist!"; break;
//    case QAbstractSocket::ConnectionRefusedError: qDebug() << "octopus_acq_client: <AcqMaster> <AcqCmdErr> ACQuisition command server refused connection!"; break;
//    default: qDebug() << "octopus_acq_client: <AcqMaster> <AcqCmdErr> ACQuisition command server unknown error!"; break;
//   }
//  }

//  void slotAcqDataError(QAbstractSocket::SocketError socketError) {
//   switch (socketError) {
//    case QAbstractSocket::HostNotFoundError: qDebug() << "octopus_acq_client: <AcqMaster> <AcqDataErr> ACQuisition data server does not exist!"; break;
//    case QAbstractSocket::ConnectionRefusedError: qDebug() << "octopus_acq_client: <AcqMaster> <AcqDataErr> ACQuisition data server refused connection!"; break;
//    default: qDebug() << "octopus_acq_client: <AcqMaster> <AcqDataErr> ACQuisition data server unknown error!"; break;
//   }
//  }

 private: // Used Just-In-Time..
//  void updateRecTime() { int s,m,h; s=recCounter/sampleRate; m=s/60; h=m/60;
//   if (h<10) rHour="0"; else rHour=""; rHour+=dummyString.setNum(h);
//   if (m<10) rMin="0"; else rMin=""; rMin+=dummyString.setNum(m);
//   if (s<10) rSec="0"; else rSec=""; rSec+=dummyString.setNum(s);
//   timeLabel->setText("Rec.Time: "+rHour+":"+rMin+":"+rSec);
//  }

  bool tick,event; int seconds,cntBufIndex,scrCounter,recCounter,avgCounter; QObject *recorder;
  unsigned int ampCount; QString rHour,rMin,rSec,dummyString;

  QFile cfgFile,cntFile,avgFile; QTextStream cfgStream; QDataStream cntStream,avgStream;
  //Event *dummyEvt; Channel *dummyChn,*curChn;
  QVector<unsigned int> ampChkP; quint64 globalCounter;

};

#endif
/*
    // Connect to ACQ Server and get crucial info.
    acqCommandSocket=new QTcpSocket(this); acqDataSocket=new QTcpSocket(this);
    connect(acqCommandSocket,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(slotAcqCommandError(QAbstractSocket::SocketError)));
    connect(acqDataSocket,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(slotAcqDataError(QAbstractSocket::SocketError)));
    acqCommandSocket->connectToHost(acqHost,acqCommPort); acqCommandSocket->waitForConnected();
    QDataStream acqCommandStream(acqCommandSocket);
    csCmd.cmd=CS_ACQ_INFO; acqCommandStream.writeRawData((const char*)(&csCmd),sizeof(cs_command)); acqCommandSocket->flush();
    if (!acqCommandSocket->waitForReadyRead()) { qDebug() << "octopus_acq_client: <AcqMaster> <.conf> ACQ server did not return crucial info!"; application->quit(); }
    acqCommandStream.readRawData((char*)(&csCmd),sizeof(cs_command)); acqCommandSocket->disconnectFromHost();
    if (csCmd.cmd!=CS_ACQ_INFO_RESULT) { qDebug() << "octopus_acq_client: <AcqMaster> <.conf> ACQ server returned nonsense crucial info!"; application->quit(); }

    sampleRate=ampInfo.sampleRate=csCmd.iparam[0];
    ampInfo.refChnCount=csCmd.iparam[1]; ampInfo.bipChnCount=csCmd.iparam[2];
    ampInfo.physChnCount=csCmd.iparam[3]; ampInfo.refChnMaxCount=csCmd.iparam[4];
    ampInfo.bipChnMaxCount=csCmd.iparam[5]; ampInfo.physChnMaxCount=csCmd.iparam[6];
    tChns=ampInfo.totalChnCount=csCmd.iparam[7]; ampInfo.totalCount=csCmd.iparam[8];
    ampInfo.probe_eeg_msecs=csCmd.iparam[9];
    
    acqCurData.resize(ampInfo.probe_eeg_msecs);

    qDebug() << "octopus_acq_client: <AcqMaster> <.conf> ACQ server returned: Total Phys Chn#=" << 2*ampInfo.physChnCount;
    qDebug() << "octopus_acq_client: <AcqMaster> <.conf> ACQ server returned: Samplerate=" << sampleRate;



    acqChannels.resize(ampCount);
*/
