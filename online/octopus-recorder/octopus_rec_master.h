/*
Octopus-ReEL - Realtime Encephalography Laboratory Network
   Copyright (C) 2007 Barkin Ilhan

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If no:t, see <https://www.gnu.org/licenses/>.

 Contact info:
 E-Mail:  barkin@unrlabs.org
 Website: http://icon.unrlabs.org/staff/barkin/
 Repo:    https://github.com/4e0n/
*/

/* a.k.a. "The Engine"..
    This is the predefined parameters, configuration and management class
    shared over all other classes of octopus-recorder. */

#ifndef OCTOPUS_REC_MASTER_H
#define OCTOPUS_REC_MASTER_H

#include <QObject>
#include <QApplication>
#include <QtGui>
#include <QtNetwork>
#include <QFile>
#include <QFileDialog>
#include <QTextStream>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QMutex>

#include <cmath>

#include "serial_device.h"
#include "channel_params.h"
#include "octopus_channel.h"
#include "../../common/octopus_event.h"
#include "../../common/octopus_gizmo.h"
#include "octopus_source.h"
#include "octopus_digitizer.h"
#include "coord3d.h"
#include "../cs_command.h"
#include "../patt_datagram.h"
#include "../stim_test_para.h"
#include "../stim_event_names.h"
#include "../resp_event_names.h"

const int OCTOPUS_VER=95;

const int CAL_DC_END=1200;
const int CAL_SINE_END=1800;

class RecMaster : QObject {
 Q_OBJECT
 public:
  RecMaster(QApplication *app) : QObject() { application=app;
   stimCommandSocket=new QTcpSocket(this); stimDataSocket=new QTcpSocket(this);
   acqCommandSocket=new QTcpSocket(this); acqDataSocket=new QTcpSocket(this);
 
   connect(stimCommandSocket,SIGNAL(error(QAbstractSocket::SocketError)),
           this,SLOT(slotStimCommandError(QAbstractSocket::SocketError)));
   connect(stimDataSocket,SIGNAL(error(QAbstractSocket::SocketError)),
           this,SLOT(slotStimDataError(QAbstractSocket::SocketError)));
   connect(acqCommandSocket,SIGNAL(error(QAbstractSocket::SocketError)),
           this,SLOT(slotAcqCommandError(QAbstractSocket::SocketError)));
   connect(acqDataSocket,SIGNAL(error(QAbstractSocket::SocketError)),
           this,SLOT(slotAcqDataError(QAbstractSocket::SocketError)));


   // *** INITIAL VALUES OF RUNTIME VARIABLES ***

   recording=calibration=stimulation=trigger=averaging=eventOccured=false;
   notchN=20; notchThreshold=5.; // 20 uV RMS
   seconds=cp.cntPastIndex=avgCounter=calPts=0;
   currentGizmo=currentElectrode=curElecInSeq=0;
   scalpParamR=15.; scalpNasion=9.; scalpCzAngle=11.;
   cntSpeedX=4; scrCounter=0;
   gizmoOnReal=elecOnReal=false;

   // Two representative sources
   Source *dummySrc;
   dummySrc=new Source();
   dummySrc->pos[0]=2.; dummySrc->pos[1]=2.; dummySrc->pos[1]=0.;
   dummySrc->theta=45.; dummySrc->phi=0.;
   source.append(dummySrc);
   dummySrc=new Source();
   dummySrc->pos[0]=-2.; dummySrc->pos[1]=2.; dummySrc->pos[1]=0.;
   dummySrc->theta=45.; dummySrc->phi=45.;
   source.append(dummySrc);
//   dummySrc=new Source(); source.append(dummySrc);

   // Initial Visualization of Head Window
   hwFrameV=hwGridV=hwDigV=hwParamV=hwRealV=hwGizmoV=hwAvgsV=
   hwScalpV=hwSkullV=hwBrainV=hwSourceV=true;


   // *** LOAD CONFIG FILE AND READ ALL LINES ***

   QString cfgLine; QStringList cfgLines; cfgFile.setFileName("octopus.cfg");
   if (!cfgFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qDebug("Octopus-Recorder: "
           "Cannot open ./octopus.cfg for reading!..\n"
           "Loading hardcoded values..");
    cp.cntPastSize=5000;
    stimHost="127.0.0.1"; stimCommPort=65000; stimDataPort=65001;
    acqHost="127.0.0.1";  acqCommPort=65002;  stimDataPort=65003;
    cp.rejBwd=-300; cp.avgBwd=-200; cp.avgFwd=500; cp.rejFwd=600;

    // ..here put default events setup..
    // ..here put default channels setup..

    serial.devname="/dev/ttyS0"; serial.baudrate=B115200;
    serial.databits=CS8; serial.parity=serial.par_on=0; serial.stopbit=1;
    guiX=hwX=guiY=hwY=0; guiWidth=800; guiHeight=600;
    hwWidth=640; hwHeight=400;
    gizmoExists=digExists=scalpExists=skullExists=brainExists=false;
   } else { cfgStream.setDevice(&cfgFile); // Load all of the file to string
    while (!cfgStream.atEnd()) { cfgLine=cfgStream.readLine(160);
     cfgLines.append(cfgLine); } cfgFile.close(); parseConfig(cfgLines);
   }

   if (!initSuccess) return;


   // *** POST SETUP ***

   slTimePt=(100-cp.avgBwd)*sampleRate/1000;

   stimSendCommand(CS_STIM_STOP,0,0,0); // Failsafe stop ongoing stim task..

   for (int i=0;i<acqChannels.size();i++) { // Channel visibitility & recording
    if (acqChannels[i]->cntVis) cntVisChns.append(i);
    if (acqChannels[i]->cntRec) cntRecChns.append(i);
    if (acqChannels[i]->avgVis) avgVisChns.append(i);
    if (acqChannels[i]->avgRec) avgRecChns.append(i);
   } scrPrvData.resize(nChns); scrCurData.resize(nChns);

   digitizer=new Digitizer(this,&serial); digitizer->serialOpen();
   if (!digitizer->connected)
    qDebug("Octopus-Recorder: "
           "Cannot connect to Polhemus digitizer!.. Continuing without it..");
   else { digExists=true;
    connect(digitizer,SIGNAL(digMonitor()),this,SLOT(slotDigMonitor()));
    connect(digitizer,SIGNAL(digResult()),this,SLOT(slotDigResult()));
   }

   // Begin retrieving continuous data
   acqDataSocket->connectToHost(acqHost,acqDataPort,QIODevice::ReadOnly);
   acqDataSocket->waitForConnected();
   connect(acqDataSocket,SIGNAL(readyRead()),this,SLOT(slotAcqReadData()));
  }


  // *** EXTERNAL OBJECT REGISTRY ***

  void registerScrollHandler(QObject *sh) {
   connect(this,SIGNAL(scrData(bool,bool)),sh,SLOT(slotScrData(bool,bool)));
  }

  void regRepaintGL(QObject *sh) {
   connect(this,SIGNAL(repaintGL(int)),sh,SLOT(slotRepaintGL(int)));
  }

  void regRepaintHeadWindow(QObject *sh) {
   connect(this,SIGNAL(repaintHeadWindow()),sh,SLOT(slotRepaint()));
  }


  // *** UTILITY ROUTINES ***

  void stimSendCommand(int command,int ip0,int ip1,int ip2) {
   stimCommandSocket->connectToHost(stimHost,stimCommPort);
   stimCommandSocket->waitForConnected();
   QDataStream stimCommandStream(stimCommandSocket); csCmd.cmd=command;
   csCmd.iparam[0]=ip0; csCmd.iparam[1]=ip1; csCmd.iparam[2]=ip2;
   stimCommandStream.writeRawData((const char*)(&csCmd),sizeof(cs_command));
   stimCommandSocket->flush(); stimCommandSocket->disconnectFromHost();
  }

  void acqSendCommand(int command,int ip0,int ip1,int ip2) {
   acqCommandSocket->connectToHost(acqHost,acqCommPort);
   acqCommandSocket->waitForConnected();
   QDataStream acqCommandStream(acqCommandSocket); csCmd.cmd=command;
   csCmd.iparam[0]=ip0; csCmd.iparam[1]=ip1; csCmd.iparam[2]=ip2;
   acqCommandStream.writeRawData((const char*)(&csCmd),sizeof(cs_command));
   acqCommandSocket->flush(); acqCommandSocket->disconnectFromHost();
  }

  int gizFindIndex(QString s) { int idx=-1;
   for (int i=0;i<gizmo.size();i++) {
    if (gizmo[i]->name==s) { idx=i; break; } } return idx;
  }

#include "rm_parse.h" // Will be transferred to external method file..

  void loadGizmo_OgzFile(QString fn) {
   QFile ogzFile; QTextStream ogzStream; bool gError=false;
   Gizmo *dummyGizmo; QVector<int> t(3); QVector<int> ll(2);
   QString ogzLine; QStringList ogzLines,ogzValidLines,opts,opts2;

   // Delete previous
   for (int i=0;i<gizmo.size();i++) delete gizmo[i]; gizmo.resize(0);
 
   ogzFile.setFileName(fn);
   ogzFile.open(QIODevice::ReadOnly|QIODevice::Text);
   ogzStream.setDevice(&ogzFile);

   // Read all
   while (!ogzStream.atEnd()) {
    ogzLine=ogzStream.readLine(160); ogzLines.append(ogzLine);
   } ogzFile.close();

   // Get valid lines
   for (int i=0;i<ogzLines.size();i++)
    if (!(ogzLines[i].at(0)=='#') && ogzLines[i].contains('|'))
     ogzValidLines.append(ogzLines[i]);

   // Find the essential line defining gizmo names
   for (int i=0;i<ogzValidLines.size();i++) {
    opts2=ogzValidLines[i].split(" "); opts=opts2[0].split("|");
    opts2.removeFirst(); opts2=opts2[0].split(",");
    if (opts.size()==2 && opts2.size()>0) {

     // GIZMO|LIST must be prior to others or segfault will occur..
     if (opts[0].trimmed()=="GIZMO") {
      if (opts[1].trimmed()=="NAME") {
       for (int i=0;i<opts2.size();i++) {
        dummyGizmo=new Gizmo(opts2[i].trimmed()); gizmo.append(dummyGizmo);
       }
      }
     } else if (opts[0].trimmed()=="SHAPE") {
      ;
     } else if (opts[1].trimmed()=="SEQ") {
      int k=gizFindIndex(opts[0].trimmed());
      for (int j=0;j<opts2.size();j++) gizmo[k]->seq.append(opts2[j].toInt());
     } else if (opts[1].trimmed()=="TRI") {
      int k=gizFindIndex(opts[0].trimmed());
      if (opts2.size()%3==0) {
       for (int j=0;j<opts2.size()/3;j++) {
        t[0]=opts2[j*3+0].toInt();
        t[1]=opts2[j*3+1].toInt();
        t[2]=opts2[j*3+2].toInt(); gizmo[k]->tri.append(t);
       }
      } else { gError=true;
       qDebug("Octopus-Recorder: "
              "Error in gizmo.. triangles not multiple of 3 vertices..");
      }
     } else if (opts[1].trimmed()=="LIN") {
      int k=gizFindIndex(opts[0].trimmed());
      if (opts2.size()%2==0) {
       for (int j=0;j<opts2.size()/2;j++) {
        ll[0]=opts2[j*2+0].toInt();
        ll[1]=opts2[j*2+1].toInt(); gizmo[k]->lin.append(ll);
       }
      } else { gError=true;
       qDebug("Octopus-Recorder: "
              "Error in gizmo.. lines not multiple of 2 vertices..");
      }
     }
    } else { gError=true;
     qDebug("Octopus-Recorder: Error in gizmo file!");
    }
   } if (!gError) gizmoExists=true;
  }

  void loadScalp_ObjFile(QString fn) {
   scalpExists=false;
   QFile scalpFile; QTextStream scalpStream;
   QString dummyStr; QStringList dummySL,dummySL2;
   Coord3D c; QVector<int> idx;

   // Reset previous
   for (int i=0;i<scalpIndex.size();i++) scalpIndex[i].resize(0);
   scalpIndex.resize(0); scalpCoord.resize(0);
 
   scalpFile.setFileName(fn);
   scalpFile.open(QIODevice::ReadOnly|QIODevice::Text);
   scalpStream.setDevice(&scalpFile);
   while (!scalpStream.atEnd()) {
    dummyStr=scalpStream.readLine();
    dummySL=dummyStr.split(" ");
    if (dummySL[0]=="v") {
     c.x=dummySL[1].toFloat(); c.y=dummySL[2].toFloat();
     c.z=dummySL[3].toFloat(); scalpCoord.append(c);
    } else if (dummySL[0]=="f") { idx.resize(0);
     dummySL2=dummySL[1].split("/"); idx.append(dummySL2[0].toInt());
     dummySL2=dummySL[2].split("/"); idx.append(dummySL2[0].toInt());
     dummySL2=dummySL[3].split("/"); idx.append(dummySL2[0].toInt());
     scalpIndex.append(idx);
    }
   } scalpStream.setDevice(0); scalpFile.close(); scalpExists=true;
  }

  void loadSkull_ObjFile(QString fn) {
   skullExists=false;
   QFile skullFile; QTextStream skullStream;
   QString dummyStr; QStringList dummySL,dummySL2;
   Coord3D c; QVector<int> idx;

   // Reset previous
   for (int i=0;i<skullIndex.size();i++) skullIndex[i].resize(0);
   skullIndex.resize(0); skullCoord.resize(0);
 
   skullFile.setFileName(fn);
   skullFile.open(QIODevice::ReadOnly|QIODevice::Text);
   skullStream.setDevice(&skullFile);
   while (!skullStream.atEnd()) {
    dummyStr=skullStream.readLine();
    dummySL=dummyStr.split(" ");
    if (dummySL[0]=="v") {
     c.x=dummySL[1].toFloat(); c.y=dummySL[2].toFloat();
     c.z=dummySL[3].toFloat(); skullCoord.append(c);
    } else if (dummySL[0]=="f") { idx.resize(0);
     dummySL2=dummySL[1].split("/"); idx.append(dummySL2[0].toInt());
     dummySL2=dummySL[2].split("/"); idx.append(dummySL2[0].toInt());
     dummySL2=dummySL[3].split("/"); idx.append(dummySL2[0].toInt());
     skullIndex.append(idx);
    }
   } skullStream.setDevice(0); skullFile.close(); skullExists=true;
  }

  void loadBrain_ObjFile(QString fn) {
   brainExists=false;
   QFile brainFile; QTextStream brainStream;
   QString dummyStr; QStringList dummySL,dummySL2;
   Coord3D c; QVector<int> idx;

   // Reset previous
   for (int i=0;i<brainIndex.size();i++) brainIndex[i].resize(0);
   brainIndex.resize(0); brainCoord.resize(0);
 
   brainFile.setFileName(fn);
   brainFile.open(QIODevice::ReadOnly|QIODevice::Text);
   brainStream.setDevice(&brainFile);
   while (!brainStream.atEnd()) {
    dummyStr=brainStream.readLine();
    dummySL=dummyStr.split(" ");
    if (dummySL[0]=="v") {
     c.x=dummySL[1].toFloat(); c.y=dummySL[2].toFloat();
     c.z=dummySL[3].toFloat(); brainCoord.append(c);
    } else if (dummySL[0]=="f") { idx.resize(0);
     dummySL2=dummySL[1].split("/"); idx.append(dummySL2[0].toInt());
     dummySL2=dummySL[2].split("/"); idx.append(dummySL2[0].toInt());
     dummySL2=dummySL[3].split("/"); idx.append(dummySL2[0].toInt());
     brainIndex.append(idx);
    }
   } brainStream.setDevice(0); brainFile.close(); brainExists=true;
  }

  void loadCalib_OacFile(QString fn) {
   QFile calibFile; QTextStream calibStream;
   QString dummyStr; QStringList dummySL;

   calM.resize(0); calN.resize(0); // Reset previous

   calibFile.setFileName(fn);
   calibFile.open(QIODevice::ReadOnly|QIODevice::Text);
   calibStream.setDevice(&calibFile);
   while (!calibStream.atEnd()) {
    dummyStr=calibStream.readLine();
    dummySL=dummyStr.split(" ",QString::SkipEmptyParts);
    calM.append(dummySL[0].toFloat()); calN.append(dummySL[1].toFloat());
   } calibStream.setDevice(0); calibFile.close();

   if (calM.size()==nChns && calN.size()==nChns)
    qDebug("Octopus-Recorder: Calibration data loaded successfully..");
   else {
    qDebug("Octopus-Recorder: "
           "There has been some problem about calibration data size!");
    qDebug(" ..disabling..");
    calM.resize(0); calN.resize(0); // Reset previous
    for (int i=0;i<nChns;i++) { calM.append(1.); calN.append(0.); }
   }
  }

  void loadReal(QString fileName) {
   QString realLine; QStringList realLines,realValidLines,opts;
   QFile realFile(fileName); int p,c;
   realFile.open(QIODevice::ReadOnly); QTextStream realStream(&realFile);
   // Read all
   while (!realStream.atEnd()) {
    realLine=realStream.readLine(160); realLines.append(realLine);
   } realFile.close();

   // Get valid lines
   for (int i=0;i<realLines.size();i++)
    if (!(realLines[i].at(0)=='#') &&
        realLines[i].split(" ",QString::SkipEmptyParts).size()>4)
     realValidLines.append(realLines[i]);

   // Find the essential line defining gizmo names
   for (int i=0;i<realValidLines.size();i++) {
    opts=realValidLines[i].split(" ",QString::SkipEmptyParts);
    if (opts.size()==8 && opts[0]=="v") {
     opts.removeFirst(); p=opts[0].toInt()-1; c=-1;
     for (int i=0;i<acqChannels.size();i++)
      if (p==acqChannels[i]->physChn) { c=i; break; }
//     if (c!=-1)
//      printf("%d - %d\n",p,c);
//     else qDebug("channel does not exist!");
     acqChannels[c]->real[0]=opts[1].toFloat();
     acqChannels[c]->real[1]=opts[2].toFloat();
     acqChannels[c]->real[2]=opts[3].toFloat();
     acqChannels[c]->realS[0]=opts[4].toFloat();
     acqChannels[c]->realS[1]=opts[5].toFloat();
     acqChannels[c]->realS[2]=opts[6].toFloat();
    } else {
     qDebug("Erroneous real coord file..%d",opts.size()); break;
    }
   } emit repaintGL(4); // Repaint Real coords
  }

  void saveReal(QString fileName) {
   QFile realFile(fileName);
   realFile.open(QIODevice::WriteOnly); QTextStream realStream(&realFile);
   realStream << "# Octopus real head coordset in headframe xyz's..\n";
   realStream << "# Generated by Octopus-recorder 0.9.5\n\n";
   realStream << "# Coord count = " << acqChannels.size() << "\n";
   for (int i=0;i<acqChannels.size();i++)
    realStream << "v " << acqChannels[i]->physChn << " "
                       << acqChannels[i]->real[0] << " "
                       << acqChannels[i]->real[1] << " "
                       << acqChannels[i]->real[2] << " "
                       << acqChannels[i]->realS[0] << " "
                       << acqChannels[i]->realS[1] << " "
                       << acqChannels[i]->realS[2] << "\n";
   realFile.close();
  }

  void startCalibration() { calibration=true; calPts=0;
   for (int i=0;i<nChns;i++) { calA[i]=calB[i]=0.; }
   calibMsg="Calibration started.. Collecting DC";
   stimSendCommand(CS_STIM_SET_PARADIGM,TEST_CALIBRATION,0,0); usleep(250000);
   stimSendCommand(CS_STIM_START,0,0,0);
  }

  void stopCalibration() { calibration=false;
   stimSendCommand(CS_STIM_STOP,0,0,0);
   guiStatusBar->showMessage("Calibration manually stopped!");
  }

  int eventIndex(int no,int type) {
   int idx=-1;
   for (int i=0;i<acqEvents.size();i++) {
    if (no==acqEvents[i]->no && type==acqEvents[i]->type) { idx=i; break; }
   } return idx;
  }

  bool initSuccess;

  // Non-volatile (read from and saved to octopus.cfg)

  // NET
  QString stimHost,acqHost; int stimCommPort,stimDataPort,acqCommPort,acqDataPort;

  // CHN
  QVector<Channel*> acqChannels; QVector<Event*> acqEvents;
  QVector<int> cntVisChns,cntRecChns,avgVisChns,avgRecChns;

  // GUI
  int guiX,guiY,guiWidth,guiHeight,hwX,hwY,hwWidth,hwHeight;

  // Gizmo
  QVector<Gizmo* > gizmo; bool gizmoOnReal,elecOnReal;
  int currentGizmo,currentElectrode,curElecInSeq;

  // Digitizer
  Vec3 sty,xp,yp,zp;

  // Volatile-Runtime
  QApplication *application; cs_command csCmd,csAck;
  QTcpSocket *stimCommandSocket,*stimDataSocket,
             *acqCommandSocket,*acqDataSocket;
  QStatusBar *guiStatusBar,*hwStatusBar; QLabel *timeLabel;

  bool recording,calibration,stimulation,trigger,averaging;
  QVector<float> acqCurData; int notchN; float notchThreshold;
  bool eventOccured; int eIndex;
  channel_params cp; int tChns,nChns,sampleRate,cntSpeedX;
  QVector<float> scrPrvData,scrCurData; float cntAmpX,avgAmpX;
  QString curEventName; int curEventType;

  bool hwFrameV,hwGridV,hwDigV,hwParamV,hwRealV,hwGizmoV,hwAvgsV,
       hwScalpV,hwSkullV,hwBrainV,hwSourceV,
       digExists,gizmoExists,scalpExists,skullExists,brainExists;

  QVector<Coord3D> paramCoord,realCoord; QVector<QVector<int> > paramIndex;
  QVector<Coord3D> scalpCoord; QVector<QVector<int> > scalpIndex;
  QVector<Coord3D> skullCoord; QVector<QVector<int> > skullIndex;
  QVector<Coord3D> brainCoord; QVector<QVector<int> > brainIndex;
  float scalpParamR,scalpNasion,scalpCzAngle;

  float slTimePt;

  QVector<Source*> source;

 signals:
  void scrData(bool,bool); void repaintGL(int); void repaintHeadWindow();

 private slots:
  void slotExportAvgs() { QString avgFN;
   QDateTime currentDT(QDateTime::currentDateTime());
   for (int i=0;i<acqEvents.size();i++) {
    if (acqEvents[i]->accepted || acqEvents[i]->rejected) { //Any event exists?
     // Generate filename using current date and time
     // add current data and time to base: trial-20071228-123012-332.oeg
     QString avgFN="Event-"+acqEvents[i]->name+
                   currentDT.toString("yyyyMMdd-hhmmss-zzz");
     avgFile.setFileName(avgFN+".oep");
     if (!avgFile.open(QIODevice::WriteOnly)) {
      qDebug("Octopus-Recorder: "
             "Error: Cannot open .oep file for writing.");
      return;
     } avgStream.setDevice(&avgFile);
     avgStream << (int)(OCTOPUS_VER);	// Version
     avgStream << sampleRate;		// Sample rate
     avgStream << avgRecChns.size();	// Channel count
     avgStream << acqEvents[i]->name.toAscii().data(); // Name of Evt - Cstyle
     avgStream << cp.avgBwd;		// Averaging Window
     avgStream << cp.avgFwd;
     avgStream << acqEvents[i]->accepted; // Accepted count
     avgStream << acqEvents[i]->rejected; // Rejected count

     for (int j=0;j<cp.avgCount;j++) {
      for (int k=0;k<avgRecChns.size();k++)
       avgStream << acqChannels[avgRecChns[k]]->avgData[j];
      for (int k=0;k<avgRecChns.size();k++)
       avgStream << acqChannels[avgRecChns[k]]->stdData[j];
     } avgFile.close();
    }
   }
  }

  void slotClrAvgs() {
   for (int i=0;i<acqChannels.size();i++) acqChannels[i]->resetEvents();
   for (int i=0;i<acqEvents.size();i++) {
    acqEvents[i]->accepted=acqEvents[i]->rejected=0;
   } emit repaintGL(16); emit repaintHeadWindow();
  }

  void slotAcqReadData() {
   int acqCurStimEvent,acqCurRespEvent,avgDataCount,avgStartOffset;
   QVector<float> *avgInChn,*stdInChn; float n1,k1,k2,z;
   QDataStream acqDataStream(acqDataSocket);

   while (acqDataSocket->bytesAvailable() >=
                                    (unsigned int)((tChns+1)*sizeof(float))) {
    acqDataStream.readRawData((char*)(acqCurData.data()),
                              (tChns+1)*sizeof(float));

    if (calibration) {
     if ((calPts%((CAL_SINE_END/100)*sampleRate))==0) // 30min-> 1800/100 = 1%.
      guiStatusBar->showMessage(calibMsg+" (Total Progress is "+
       dummyString.setNum(calPts/((CAL_SINE_END/100)*sampleRate))+"%).",0);
     if (calPts>(5*sampleRate) &&
         calPts<((CAL_DC_END-5)*sampleRate)) { // ~<20 min of DC
      n1=(float)(calPts-5*sampleRate);
      for (int j=0;j<nChns;j++) { // Averages
       k1=calB[j]; k2=acqCurData[1+j]; calB[j]=(k1*n1+k2)/(n1+1.);
      }
//      printf("%d %2.2f\n",(int)n1,calB[0]);
     } else if (calPts==(CAL_DC_END*sampleRate)) { // 20min complete (~=67%)..
      calibMsg="Calibration continues.. Collecting 12Hz Sines";
     } else if (calPts>((CAL_DC_END+5)*sampleRate) &&
                calPts<((CAL_SINE_END-5)*sampleRate)) {
      n1=(float)(calPts-(CAL_DC_END+5)*sampleRate);
      for (int j=0;j<nChns;j++) { // Averages of squares
       z=acqCurData[1+j]-calB[j]; // CD corrected sinus
       k1=calA[j]; k2=z*z;
       calA[j]=(k1*n1+k2)/(n1+1.);
      }
//      printf("%d %2.2f\n",(int)n1,calA[0]);
     } else if (calPts==(CAL_SINE_END*sampleRate)) { calibration=false;
      stimSendCommand(CS_STIM_STOP,0,0,0);

      float calAvg; // Normalization of CalA to average of amplitudes..
      for (int j=0;j<nChns;j++) calAvg+=calA[j]; calAvg=calAvg/nChns;
      for (int j=0;j<nChns;j++) calA[j]=calAvg/calA[j]; // coefficients

      QDateTime currentDT(QDateTime::currentDateTime());
      QString calFN="calib-"+currentDT.toString("yyyyMMdd-hhmmss-zzz");
      QFile calibFile(calFN+".oac"); QTextStream calibStream;
      if (!calibFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
       qDebug("Error: Cannot open calibration file for writing."); return;
      } calibStream.setDevice(&calibFile);
      for (int i=0;i<nChns;i++)
       calibStream << calA[i] << " " << calB[i] << "\n";
      calibFile.close();

      guiStatusBar->showMessage("Calibration completed successfully..",0);
     } calPts++;

    } else { // RECORDING, 50Hz COMPUTATION and ONLINE AVG is enabled..
     // Apply loaded calibration model on incoming data..
     for (int i=0;i<nChns;i++) {
      acqCurData[1+i]-=calN[i]; acqCurData[i]*=calM[i];
      acqCurData[1+nChns+i]-=calN[i]; acqCurData[i]*=calM[i];
     }

     acqCurStimEvent=(int)(acqCurData[tChns-1]); // Stim Event
     acqCurRespEvent=(int)(acqCurData[tChns]);   // Resp Event

     if (recording) { // .. to disk ..
      for (int i=0;i<cntRecChns.size();i++)
       cntStream << acqCurData[1+acqChannels[cntRecChns[i]]->physChn];
      cntStream << acqCurStimEvent; cntStream << acqCurRespEvent;
      recCounter++; if (!(recCounter%sampleRate)) updateRecTime();
     }

     // Handle backward data..
     //  Put data into suitable offset for backward online averaging..
     Channel *curChn; float dummyAvg;
     int notchCount=notchN*(sampleRate/50);
     int notchStart=(cp.cntPastSize+cp.cntPastIndex-notchCount-1)%
                                                            cp.cntPastSize;
     for (int j=0;j<acqChannels.size();j++) {
      curChn=acqChannels[j];
      curChn->pastData[cp.cntPastIndex]=acqCurData[1+curChn->physChn];
      curChn->pastFilt[cp.cntPastIndex]=acqCurData[1+nChns+curChn->physChn];

      // Compute Absolute "50Hz+Harmonics" Level of that channel..
      dummyAvg=0.;
      for (int k=0;k<notchCount;k++)
       dummyAvg+=abs(curChn->pastFilt[(notchStart+k)%cp.cntPastSize]);
      dummyAvg/=(float)notchCount; //dummyAvg/=0.6; // Level of avg of sine..
      curChn->notchLevel=dummyAvg; // /10.; // Normalization
      if (curChn->notchLevel < notchThreshold)
       curChn->notchColor=QColor(0,255,0,144);     // Green
      else curChn->notchColor=QColor(255,0,0,144); // Red
     }

     // Handle Incoming Event..
     if ((acqCurStimEvent || acqCurRespEvent) && trigger) { event=true;
      if (acqCurStimEvent) {
       curEventName="Unknown STIM event #";
       curEventName+=dummyString.setNum(acqCurStimEvent); curEventType=1;
      } else if (acqCurRespEvent) {
       curEventName="Unknown RESP event #";
       curEventName+=dummyString.setNum(acqCurRespEvent); curEventType=2;
      }

      int i1=eventIndex(acqCurStimEvent,1);
      int i2=eventIndex(acqCurRespEvent,2);
      if (i1>=0 || i2>=0) {
       if (i1>=0) eIndex=i1; else if (i2>=0) eIndex=i2;
       curEventName=acqEvents[eIndex]->name;
       curEventType=acqEvents[eIndex]->type;
       qDebug("Octopus-Recorder: Avg! -> Index=%d, Name=%s",
              eIndex,curEventName.toAscii().data());
       if (averaging) {
        qDebug("Octopus-Recorder: "
               "Event collision!.. (was already averaging).. %d %d",
               avgCounter,cp.rejCount);
       } else { averaging=true; avgCounter=0; }
      }
     }

     if (averaging) {
      if (avgCounter==cp.bwCount) { averaging=false;
       qDebug("Octopus-Recorder: Computing for Event! -> Index=%d, Name=%s",
              eIndex,acqEvents[eIndex]->name.toAscii().data());
 
       // Check rejection backwards on pastdata
       bool rejFlag=false; int rejChn;
       for (int i=0;i<acqChannels.size();i++) {
        if (acqChannels[i]->rejLev>0) {
         for (int j=0;j<cp.rejCount;j++) {
          if (abs(acqChannels[i]->pastData[
           (cp.cntPastSize+cp.cntPastIndex-cp.rejCount+j)%cp.cntPastSize])>
           acqChannels[i]->rejLev) { rejFlag=true; rejChn=i; break; }
         }
        } if (rejFlag==true) break;
       }

       if (rejFlag) {
        // Rejected, increment rejected count
        acqEvents[eIndex]->rejected++;
        qDebug("Octopus-Recorder: Rejected because of %s!",
               acqChannels[rejChn]->name.toAscii().data());
       } else {
        // Not rejected: compute average and increment accepted for the event
        acqEvents[eIndex]->accepted++; eventOccured=true;
        qDebug("Octopus-Recorder: "
               "Computing average for eventIndex and updating GUI..");

        for (int i=0;i<acqChannels.size();i++) {
         avgStartOffset=
          (cp.cntPastSize+cp.cntPastIndex-cp.avgCount-cp.postRejCount)%
                                                             cp.cntPastSize;
         avgInChn=(acqChannels[i]->avgData)[eIndex];
         stdInChn=(acqChannels[i]->stdData)[eIndex];
         n1=(float)(acqEvents[eIndex]->accepted); // n2=1
         avgDataCount=avgInChn->size();

         for (int j=0;j<avgDataCount;j++) {
          k1=(*avgInChn)[j];
          k2=acqChannels[i]->pastData[(avgStartOffset+j)%cp.cntPastSize];
          (*avgInChn)[j]=(k1*n1+k2)/(n1+1.);
         }
        } emit repaintGL(16); emit repaintHeadWindow();
       }
      }
     }
     avgCounter++;
    } // End of Calibration-or-NoCalibration..

    cp.cntPastIndex++; cp.cntPastIndex%=cp.cntPastSize;

    if (!scrCounter) {
     for (int i=0;i<nChns;i++) {
      scrPrvData[i]=scrCurData[i]; scrCurData[i]=acqCurData[1+i];
      // To see 50Hz component instead of the data itself..
//      scrPrvData[i]=scrCurData[i]; scrCurData[i]=acqCurData[1+nChns+i];
     } emit scrData(tick,event); tick=event=false; // Update CntFrame
    } scrCounter++; scrCounter%=cntSpeedX;

    if (!seconds) emit repaintGL(2+4); // Update 50Hz visualization..
    seconds++; seconds%=sampleRate; if (seconds==0) tick=true;
   }
  }

  void slotReboot() {
   stimSendCommand(CS_REBOOT,0,0,0);
   acqSendCommand(CS_REBOOT,0,0,0);
   guiStatusBar->showMessage("Servers are rebooting..",5000);
  }
  void slotShutdown() {
   stimSendCommand(CS_SHUTDOWN,0,0,0);
   acqSendCommand(CS_SHUTDOWN,0,0,0);
   guiStatusBar->showMessage("Servers are shutting down..",5000);
  }
  void slotQuit() {
   if (digitizer->connected) digitizer->serialClose();
    acqDataSocket->disconnectFromHost();
    if (acqDataSocket->state()==QAbstractSocket::UnconnectedState ||
        acqDataSocket->waitForDisconnected(1000)) {
     application->exit(0);
   }
  }

  // *** POLHEMUS HANDLER ***

  void slotDigMonitor() {
   digitizer->mutex.lock();
    sty=digitizer->styF; xp=digitizer->xpF;
    yp=digitizer->ypF; zp=digitizer->zpF;
   digitizer->mutex.unlock(); emit repaintGL(1);
  }

  void slotDigResult() {
   digitizer->mutex.lock();
    acqChannels[currentElectrode]->real=digitizer->stylusF;
    acqChannels[currentElectrode]->realS=digitizer->stylusSF;
   digitizer->mutex.unlock(); curElecInSeq++;
   if (curElecInSeq==gizmo[currentGizmo]->seq.size()) curElecInSeq=0;
   for (int i=0;i<acqChannels.size();i++)
    if (acqChannels[i]->physChn==gizmo[currentGizmo]->seq[curElecInSeq]-1)
     { currentElectrode=i; break; }
   emit repaintHeadWindow(); emit repaintGL(1);
  }

  //  GUI TOP LEFT BUTTONS RELATED TO RECORDING/EVENTS/TRIGGERS

  void slotToggleRecording() {
   QDateTime currentDT(QDateTime::currentDateTime());
   if (!recording) {
    // Generate filename using current date and time
    // add current data and time to base: trial-20071228-123012-332.oeg
    QString cntFN="trial-"+currentDT.toString("yyyyMMdd-hhmmss-zzz");
    cntFile.setFileName(cntFN+".oeg");
    if (!cntFile.open(QIODevice::WriteOnly)) {
     qDebug("Octopus-Recorder: "
            "Error: Cannot open .occ file for writing.");
     return;
    } cntStream.setDevice(&cntFile);

    cntStream << (int)(OCTOPUS_VER);	// Version
    cntStream << sampleRate;		// Sample rate
    cntStream << cntRecChns.size();	// Channel count

    for (int i=0;i<cntRecChns.size();i++) // Channel names - Cstyle
     cntStream << acqChannels[cntRecChns[i]]->name.toAscii().data();

    for (int i=0;i<cntRecChns.size();i++) { // Param coords
     cntStream << acqChannels[cntRecChns[i]]->param.y;
     cntStream << acqChannels[cntRecChns[i]]->param.z;
    }
    for (int i=0;i<cntRecChns.size();i++) { // Real/measured coords
     cntStream << acqChannels[cntRecChns[i]]->real[0];
     cntStream << acqChannels[cntRecChns[i]]->real[1];
     cntStream << acqChannels[cntRecChns[i]]->real[2];
     cntStream << acqChannels[cntRecChns[i]]->realS[0];
     cntStream << acqChannels[cntRecChns[i]]->realS[1];
     cntStream << acqChannels[cntRecChns[i]]->realS[2];
    }

    cntStream << acqEvents.size();	// Event count
    for (int i=0;i<acqEvents.size();i++) { // Event Info of the session
     cntStream << acqEvents[i]->no;	// Event #
     cntStream << acqEvents[i]->name.toAscii().data(); // Name - Cstyle
     cntStream << acqEvents[i]->type;	// STIM or RESP
     cntStream << acqEvents[i]->color.red(); // Color
     cntStream << acqEvents[i]->color.green();
     cntStream << acqEvents[i]->color.blue();
    }
    
    // Here continuous data begins..

    timeLabel->setText("Rec.Time: 00:00:00");
    recCounter=0; recording=true;
   } else { recording=false;
    if (stimulation) slotToggleStimulation();
    cntStream.setDevice(0); cntFile.close();
   }
  }

  void slotToggleStimulation() {
   if (!stimulation) { stimSendCommand(CS_STIM_START,0,0,0); stimulation=true; }
   else { stimSendCommand(CS_STIM_STOP,0,0,0); stimulation=false; }
  }
  void slotToggleTrigger() {
   if (!trigger) { stimSendCommand(CS_TRIG_START,0,0,0); trigger=true; }
   else { stimSendCommand(CS_TRIG_STOP,0,0,0); trigger=false; }
  }


  // *** TCP HANDLERS */

  void slotStimCommandError(QAbstractSocket::SocketError socketError) {
   switch (socketError) {
    case QAbstractSocket::HostNotFoundError:
     qDebug("Octopus-Recorder: "
            "STIMulus command server does not exist!"); break;
    case QAbstractSocket::ConnectionRefusedError:
     qDebug("Octopus-Recorder: "
            "STIMulus command server refused connection!"); break;
    default:
     qDebug("Octopus-Recorder: "
            "STIMulus command server unknown error! %d",
            socketError); break;
   }
  }

  void slotStimDataError(QAbstractSocket::SocketError socketError) {
   switch (socketError) {
    case QAbstractSocket::HostNotFoundError:
     qDebug("Octopus-Recorder: "
            "STIMulus data server does not exist!"); break;
    case QAbstractSocket::ConnectionRefusedError:
     qDebug("Octopus-Recorder: "
            "STIMulus data server refused connection!"); break;
    default:
     qDebug("Octopus-Recorder: "
            "STIMulus data server unknown error! %d",
            socketError); break;
   }
  }

  void slotAcqCommandError(QAbstractSocket::SocketError socketError) {
   switch (socketError) {
    case QAbstractSocket::HostNotFoundError:
     qDebug("Octopus-Recorder: "
            "ACQuisition command server does not exist!"); break;
    case QAbstractSocket::ConnectionRefusedError:
     qDebug("Octopus-Recorder: "
            "ACQuisition command server refused connection!");
     break;
    default:
     qDebug("Octopus-Recorder: "
            "ACQuisition command server unknown error!"); break;
   }
  }

  void slotAcqDataError(QAbstractSocket::SocketError socketError) {
   switch (socketError) {
    case QAbstractSocket::HostNotFoundError:
     qDebug("Octopus-Recorder: "
            "ACQuisition data server does not exist!"); break;
     break;
    case QAbstractSocket::ConnectionRefusedError:
     qDebug("Octopus-Recorder: "
            "ACQuisition data server refused connection!"); break;
    default:
     qDebug("Octopus-Recorder: "
            "ACQuisition data server unknown error!"); break;
   }
  }

  // *** RELATIVELY SIMPLE COMMANDS TO SERVERS ***

  void slotTestSineCosine() {
   stimSendCommand(CS_STIM_SET_PARADIGM,TEST_SINECOSINE,0,0);
  }
  void slotTestSquare() {
   stimSendCommand(CS_STIM_SET_PARADIGM,TEST_SQUARE,0,0);
  }

  void slotParadigmLoadPattern() {
   QString patFileName=QFileDialog::getOpenFileName(0,
                        "Open Pattern File",".","ASCII Pattern Files (*.arp)");
   if (!patFileName.isEmpty()) { patFile.setFileName(patFileName);
    patFile.open(QIODevice::ReadOnly); patStream.setDevice(&patFile);
    pattern=patStream.readAll(); patFile.close(); // Close pattern file.

    stimCommandSocket->connectToHost(stimHost,stimCommPort);
    stimCommandSocket->waitForConnected();

    // We want to be ready when the answer comes..
    connect(stimCommandSocket,SIGNAL(readyRead()),
            (QObject *)this,SLOT(slotReadAcknowledge()));

    // Send Command SYN and then Sync.
    QDataStream stimCommandStream(stimCommandSocket);
    csCmd.cmd=CS_STIM_LOAD_PATTERN_SYN;
    csCmd.iparam[0]=patFile.size(); // File size is the only parameter
    csCmd.iparam[1]=csCmd.iparam[2]=0;
    stimCommandStream.writeRawData((const char*)(&csCmd),(sizeof(cs_command)));
    stimCommandSocket->flush();
   }
  }
  void slotReadAcknowledge() {
   QDataStream ackStream(stimCommandSocket);

   // Wait for ACK from stimulation server
   if (stimCommandSocket->bytesAvailable() >= (int)(sizeof(cs_command))) {
    ackStream.readRawData((char*)(&csAck),(int)(sizeof(cs_command)));

    if (csAck.cmd!=CS_STIM_LOAD_PATTERN_ACK) { // Something went wrong?
     qDebug("Octopus GUI: Error in STIM daemon ACK reply!"); return; }

    // Now STIM Server is waiting for the file.. Let's send over data port..
    stimDataSocket->connectToHost(stimHost,stimDataPort);
    stimDataSocket->waitForConnected();
    QDataStream stimDataStream(stimDataSocket);

    int dataCount=0; pattDatagram.magic_number=0xaabbccdd;
    for (int i=0;i<pattern.size();i++) {
     pattDatagram.data[dataCount]=pattern.at(i).toAscii(); dataCount++;
     if (dataCount==128) { // We got 128 bytes.. Send packet and Sync.
      pattDatagram.size=dataCount; dataCount=0;
      stimDataStream.writeRawData((const char*)(&pattDatagram),
                                  sizeof(patt_datagram));
      stimDataSocket->flush();
     }
    }
    if (dataCount!=0) { // Last fragment whose size is !=0 and <128
     pattDatagram.size=dataCount; dataCount=0;
     stimDataStream.writeRawData((const char*)(&pattDatagram),
                                 sizeof(patt_datagram));
     stimDataSocket->flush();
    }
    stimDataSocket->disconnectFromHost();
    disconnect(stimCommandSocket,SIGNAL(readyRead()),
               this,SLOT(slotReadAcknowledge()));
   }
  }

  void slotParadigmClick() {
   stimSendCommand(CS_STIM_SET_PARADIGM,PARA_CLICK,0,0);
  }
  void slotParadigmSquareBurst() {
   stimSendCommand(CS_STIM_SET_PARADIGM,PARA_SQUAREBURST,0,0);
  }
  void slotParadigmIIDITD() {
   stimSendCommand(CS_STIM_SET_PARADIGM,PARA_IIDITD,0,0);
  }
  void slotParadigmIIDITD_MonoL() {
   stimSendCommand(CS_STIM_SET_PARADIGM,PARA_IIDITD,1,0);
  }
  void slotParadigmIIDITD_MonoR() {
   stimSendCommand(CS_STIM_SET_PARADIGM,PARA_IIDITD,2,0);
  }

 private: // Used Just-In-Time..
  void updateRecTime() { int s,m,h;
   s=recCounter/sampleRate; m=s/60; h=m/60;
   if (h<10) rHour="0"; else rHour=""; rHour+=dummyString.setNum(h);
   if (m<10) rMin="0"; else rMin=""; rMin+=dummyString.setNum(m);
   if (s<10) rSec="0"; else rSec=""; rSec+=dummyString.setNum(s);
   timeLabel->setText("Rec.Time: "+rHour+":"+rMin+":"+rSec);
  }

  QFile cfgFile,calFile,patFile,cntFile,avgFile;
  QTextStream cfgStream,calStream,patStream; QDataStream cntStream,avgStream;
  serial_device serial; Digitizer *digitizer; QVector<float> calM,calN;
  int cntBufIndex,scrCounter,seconds; bool tick,event;
  patt_datagram pattDatagram; QString pattern,dummyString;
  Event *dummyEvt; Channel *dummyChn;

  QObject *recorder; QString calibMsg;

  int recCounter,avgCounter; QString rHour,rMin,rSec;

  // Calibration
  int calPts; QVector<float> calA,calB;
  QVector<float> calDC,calSin;
};

#endif
