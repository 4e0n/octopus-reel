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
#include "channel_params.h"
#include "channel.h"
#include "../../common/event.h"
#include "../../common/gizmo.h"
#include "digitizer.h"
#include "coord3d.h"
#include "../cs_command.h"
#include "../sample.h"
#include "../tcpsample.h"
#include "../chninfo.h"
#include "../patt_datagram.h"
#include "../stim_event_names.h"
#include "../resp_event_names.h"

const int OCTOPUS_ACQ_CLIENT_VER=120;

class AcqMaster : QObject {
 Q_OBJECT
 public:
  AcqMaster(QApplication *app) : QObject() { application=app;

   // *** INITIAL VALUES OF RUNTIME VARIABLES ***

   clientRunning=recording=withinAvgEpoch=eventOccured=false;
   seconds=cp.cntPastIndex=avgCounter=0; cntSpeedX=4; globalCounter=scrCounter=0;
   
   notch=true; notchN=20; notchThreshold=20.;

   // *** LOAD CONFIG FILE AND READ ALL LINES ***

   ampCount=EE_AMPCOUNT;

   QString cfgLine; QStringList cfgLines; cfgFile.setFileName("/etc/octopus_acq_client.conf");
   if (!cfgFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qDebug() << "octopus_acq_client: <AcqMaster> Cannot open ./octopus_acq_client.conf for reading!.."; application->quit();
   } else { cfgStream.setDevice(&cfgFile); // Load all of the file to string
    digExists.resize(ampCount); scalpExists.resize(ampCount); skullExists.resize(ampCount); brainExists.resize(ampCount);
    for (unsigned int i=0;i<ampCount;i++)
     gizmoExists=digExists[i]=scalpExists[i]=skullExists[i]=brainExists[i]=false;
    while (!cfgStream.atEnd()) { cfgLine=cfgStream.readLine(160); cfgLines.append(cfgLine); }
    cfgFile.close();

    // *** PARSE CONFIG ***
    QStringList cfgValidLines,opts,opts2,opts3,bufSection,netSection,avgSection,evtSection,chnSection,digSection,guiSection,modSection;

    for (int i=0;i<cfgLines.size();i++) // Isolate valid lines
     if (!(cfgLines[i].at(0)=='#') && cfgLines[i].contains('|')) cfgValidLines.append(cfgLines[i]);

    // *** CATEGORIZE SECTIONS ***
    for (int i=0;i<cfgValidLines.size();i++) { opts=cfgValidLines[i].split("|");
          if (opts[0].trimmed()=="BUF") bufSection.append(opts[1]);
     else if (opts[0].trimmed()=="NET") netSection.append(opts[1]);
     else if (opts[0].trimmed()=="AVG") avgSection.append(opts[1]);
     else if (opts[0].trimmed()=="EVT") evtSection.append(opts[1]);
     else if (opts[0].trimmed()=="CHN") chnSection.append(opts[1]);
     else if (opts[0].trimmed()=="DIG") digSection.append(opts[1]);
     else if (opts[0].trimmed()=="GUI") guiSection.append(opts[1]);
     else if (opts[0].trimmed()=="MOD") modSection.append(opts[1]);
     else { qDebug() << "octopus_acq_client: <AcqMaster> <.conf> Unknown section in .conf file!"; application->quit(); }
    }

    if (bufSection.size()>0) { // BUF
     for (int i=0;i<bufSection.size();i++) { opts=bufSection[i].split("=");
      if (opts[0].trimmed()=="PAST") { cp.cntPastSize=opts[1].toInt();
       if (!(cp.cntPastSize >= 1000 && cp.cntPastSize <= 20000)) {
        qDebug() << "octopus_acq_client: <AcqMaster> <.conf> BUF|PAST not within inclusive (1000,20000) range!"; application->quit();
       }
      } else { qDebug() << "octopus_acq_client: <AcqMaster> <.conf> Parse error in BUF sections!"; application->quit(); }
     }
    }

    if (netSection.size()>0) { // NET
     for (int i=0;i<netSection.size();i++) { opts=netSection[i].split("=");
      if (opts[0].trimmed()=="ACQ") { opts2=opts[1].split(",");
       if (opts2.size()==3) { acqHost=opts2[0].trimmed();
        QHostInfo qhiAcq=QHostInfo::fromName(acqHost); acqHost=qhiAcq.addresses().first().toString();
        qDebug() << "octopus_acq_client: <AcqMaster> <.conf> AcqHost:" << acqHost;
        acqCommPort=opts2[1].toInt(); acqDataPort=opts2[2].toInt();
        if ((!(acqCommPort >= 1024 && acqCommPort <= 65535)) || (!(acqDataPort >= 1024 && acqDataPort <= 65535))) {
         qDebug() << "octopus_acq_client: <AcqMaster> <.conf> Error in ACQ IP and/or port settings!"; application->quit();
        }
       }
      } else { qDebug() << "octopus_acq_client: <AcqMaster> <.conf> Parse error in NET sections!"; application->quit(); }
     }
    }

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

    sampleRate=chnInfo.sampleRate=csCmd.iparam[0];
    chnInfo.refChnCount=csCmd.iparam[1]; chnInfo.bipChnCount=csCmd.iparam[2];
    chnInfo.physChnCount=csCmd.iparam[3]; chnInfo.refChnMaxCount=csCmd.iparam[4];
    chnInfo.bipChnMaxCount=csCmd.iparam[5]; chnInfo.physChnMaxCount=csCmd.iparam[6];
    tChns=chnInfo.totalChnCount=csCmd.iparam[7]; chnInfo.totalCount=csCmd.iparam[8];
    chnInfo.probe_eeg_msecs=csCmd.iparam[9];
    
    acqCurData.resize(chnInfo.probe_eeg_msecs);

    qDebug() << "octopus_acq_client: <AcqMaster> <.conf> ACQ server returned: Total Phys Chn#=" << 2*chnInfo.physChnCount;
    qDebug() << "octopus_acq_client: <AcqMaster> <.conf> ACQ server returned: Samplerate=" << sampleRate;

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
	if ((!(opts2[0].toInt()>0 && opts2[1].toInt()<=(int)chnInfo.physChnCount)) || // Channel#
            (!(opts2[1].size()>0)) || // Channel name must be at least 1 char..
            (!(opts2[2].toInt()>=0 && opts2[2].toInt()<1000))   || // Rej
            (!(opts2[3].toInt()>=0 && opts2[3].toInt()<=(int)chnInfo.physChnCount)) || // RejRef
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

    if (digSection.size()>0) { // DIG
     for (int i=0;i<digSection.size();i++) { opts=digSection[i].split("=");
      if (opts[0].trimmed()=="POLHEMUS") { opts2=opts[1].split(",");
       if (opts2.size()==5) {
        if ((!(opts2[0].toInt()>=0  && opts2[0].toInt()<8)) || // ttyS#
            (!(opts2[1].toInt()==9600 || opts2[1].toInt()==19200 ||
            opts2[1].toInt()==38400 || opts2[1].toInt()==57600 || opts2[1].toInt()==115200)) || // BaudRate
            (!(opts2[2].toInt()==7 || opts2[2].toInt()==8)) || // Data Bits
            (!(opts2[3].trimmed()=="E" || opts2[3].trimmed()=="O" || opts2[3].trimmed()=="N")) || // Parity
            (!(opts2[4].toInt()==0 || opts2[4].toInt()==1))) { // Stop Bit
         qDebug() << "octopus_acq_client: <AcqMaster> <.conf> Parse/logic error in DIG|POLHEMUS parameters!"; application->quit();
        } else {
         serial.devname="/dev/ttyS"+opts2[0].trimmed();
         switch (opts2[1].toInt()) {
          case   9600: serial.baudrate=B9600;   break;
          case  19200: serial.baudrate=B19200;  break;
          case  38400: serial.baudrate=B38400;  break;
          case  57600: serial.baudrate=B57600;  break;
          case 115200: serial.baudrate=B115200; break;
         }
         switch (opts2[2].toInt()) {
          case 7: serial.databits=CS7; break;
          case 8: serial.databits=CS8; break;
         }
         if (opts2[3].trimmed()=="N") { serial.parity=serial.par_on=0; }
         else if (opts2[3].trimmed()=="O") { serial.parity=PARODD; serial.par_on=PARENB; }
         else if (opts2[3].trimmed()=="E") { serial.parity=0; serial.par_on=PARENB; }
         switch (opts2[4].toInt()) {
          default:
          case 0: serial.stopbit=0; break;
          case 1: serial.stopbit=CSTOPB; break;
         }
        }
       } else { qDebug() << "octopus_acq_client: <AcqMaster> <.conf> Parse error in DIG|POLHEMUS parameters!"; application->quit(); }
      } else { qDebug() << "octopus_acq_client: <AcqMaster> <.conf> Parse error in DIG sections!"; application->quit(); }
     }
    } else { // defaults ? TODO: Is needed?
     serial.devname="/dev/ttyS0"; serial.baudrate=B115200; serial.databits=CS8; serial.parity=serial.par_on=0; serial.stopbit=1;
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
     gl3DGuiX=160; gl3DGuiY=160; gl3DGuiW=400; gl3DGuiH=300;
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

  unsigned int getAmpCount() { return ampCount; }

  // *** EXTERNAL OBJECT REGISTRY ***

  void registerScrollHandler(QObject *sh) { connect(this,SIGNAL(scrData(bool,bool)),sh,SLOT(slotScrData(bool,bool))); }
  void regRepaintGL(QObject *sh) { connect(this,SIGNAL(repaintGL(int)),sh,SLOT(slotRepaintGL(int))); }
  void regRepaintHeadWindow(QObject *sh) { connect(this,SIGNAL(repaintHeadWindow()),sh,SLOT(slotRepaint())); }
  void regRepaintLegendHandler(QObject *sh) { connect(this,SIGNAL(repaintLegend()),sh,SLOT(slotRepaintLegend())); }

  // *** UTILITY ROUTINES ***

  void acqSendCommand(int command,int ip0,int ip1,int ip2) {
   acqCommandSocket->connectToHost(acqHost,acqCommPort); acqCommandSocket->waitForConnected();
   QDataStream acqCommandStream(acqCommandSocket); csCmd.cmd=command; csCmd.iparam[0]=ip0; csCmd.iparam[1]=ip1; csCmd.iparam[2]=ip2;
   acqCommandStream.writeRawData((const char*)(&csCmd),sizeof(cs_command)); acqCommandSocket->flush(); acqCommandSocket->disconnectFromHost();
  }

  int gizFindIndex(QString s) { int idx=-1;
   for (int i=0;i<gizmo.size();i++) if (gizmo[i]->name==s) { idx=i; break; }
   return idx;
  }

  void loadGizmo_OgzFile(QString fn) {
   QFile ogzFile; QTextStream ogzStream; bool gError=false; Gizmo *dummyGizmo; QVector<int> t(3); QVector<int> ll(2);
   QString ogzLine; QStringList ogzLines,ogzValidLines,opts,opts2;

   // Delete previous
   for (int i=0;i<gizmo.size();i++) delete gizmo[i];
   gizmo.resize(0);
 
   ogzFile.setFileName(fn); ogzFile.open(QIODevice::ReadOnly|QIODevice::Text); ogzStream.setDevice(&ogzFile);

   // Read all
   while (!ogzStream.atEnd()) { ogzLine=ogzStream.readLine(160); ogzLines.append(ogzLine); } ogzFile.close();

   // Get valid lines
   for (int i=0;i<ogzLines.size();i++) if (!(ogzLines[i].at(0)=='#') && ogzLines[i].contains('|')) ogzValidLines.append(ogzLines[i]);

   // Find the essential line defining gizmo names
   for (int i=0;i<ogzValidLines.size();i++) {
    opts2=ogzValidLines[i].split(" "); opts=opts2[0].split("|"); opts2.removeFirst(); opts2=opts2[0].split(",");
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
      int k=gizFindIndex(opts[0].trimmed()); for (int j=0;j<opts2.size();j++) gizmo[k]->seq.append(opts2[j].toInt()-1);
     } else if (opts[1].trimmed()=="TRI") { int k=gizFindIndex(opts[0].trimmed());
      if (opts2.size()%3==0) {
       for (int j=0;j<opts2.size()/3;j++) {
        t[0]=opts2[j*3+0].toInt()-1; t[1]=opts2[j*3+1].toInt()-1; t[2]=opts2[j*3+2].toInt()-1; gizmo[k]->tri.append(t);
       }
      } else { gError=true;
       qDebug() << "octopus_acq_client: <AcqMaster> <LoadGizmo> <OGZ> Error in gizmo.. triangles not multiple of 3 vertices..";
      }
     } else if (opts[1].trimmed()=="LIN") { int k=gizFindIndex(opts[0].trimmed());
      if (opts2.size()%2==0) {
       for (int j=0;j<opts2.size()/2;j++) { ll[0]=opts2[j*2+0].toInt()-1; ll[1]=opts2[j*2+1].toInt()-1; gizmo[k]->lin.append(ll); }
      } else { gError=true;
       qDebug() << "octopus_acq_client: <AcqMaster> <LoadGizmo> <OGZ> Error in gizmo.. lines not multiple of 2 vertices..";
      }
     }
    } else { gError=true; qDebug() << "octopus_acq_client: <AcqMaster> <LoadGizmo> <OGZ> Error in gizmo file!"; }
   } if (!gError) gizmoExists=true;
  }

  void loadScalp_ObjFile(QString fn) {
   for (unsigned int i=0;i<ampCount;i++) scalpExists[i]=false;
   QFile scalpFile; QTextStream scalpStream; QString dummyStr; QStringList dummySL,dummySL2; Coord3D c; QVector<int> idx;

   // Reset previous
   for (int i=0;i<scalpIndex.size();i++) scalpIndex[i].resize(0);
   scalpIndex.resize(0); scalpCoord.resize(0);
 
   scalpFile.setFileName(fn); scalpFile.open(QIODevice::ReadOnly|QIODevice::Text); scalpStream.setDevice(&scalpFile);
   while (!scalpStream.atEnd()) {
    dummyStr=scalpStream.readLine(); dummySL=dummyStr.split(" ");
    if (dummySL[0]=="v") { c.x=dummySL[1].toFloat(); c.y=dummySL[2].toFloat(); c.z=dummySL[3].toFloat(); scalpCoord.append(c); }
    else if (dummySL[0]=="f") { idx.resize(0);
     dummySL2=dummySL[1].split("/"); idx.append(dummySL2[0].toInt());
     dummySL2=dummySL[2].split("/"); idx.append(dummySL2[0].toInt());
     dummySL2=dummySL[3].split("/"); idx.append(dummySL2[0].toInt());
     scalpIndex.append(idx);
    }
   } scalpStream.setDevice(0); scalpFile.close(); for (unsigned int i=0;i<ampCount;i++) scalpExists[i]=true;
  }

  void loadSkull_ObjFile(QString fn) {
   for (unsigned int i=0;i<ampCount;i++) skullExists[i]=false;
   QFile skullFile; QTextStream skullStream; QString dummyStr; QStringList dummySL,dummySL2; Coord3D c; QVector<int> idx;

   // Reset previous
   for (int i=0;i<skullIndex.size();i++) skullIndex[i].resize(0);
   skullIndex.resize(0); skullCoord.resize(0);
 
   skullFile.setFileName(fn); skullFile.open(QIODevice::ReadOnly|QIODevice::Text); skullStream.setDevice(&skullFile);
   while (!skullStream.atEnd()) {
    dummyStr=skullStream.readLine(); dummySL=dummyStr.split(" ");
    if (dummySL[0]=="v") { c.x=dummySL[1].toFloat(); c.y=dummySL[2].toFloat(); c.z=dummySL[3].toFloat(); skullCoord.append(c); }
    else if (dummySL[0]=="f") { idx.resize(0);
     dummySL2=dummySL[1].split("/"); idx.append(dummySL2[0].toInt());
     dummySL2=dummySL[2].split("/"); idx.append(dummySL2[0].toInt());
     dummySL2=dummySL[3].split("/"); idx.append(dummySL2[0].toInt());
     skullIndex.append(idx);
    }
   } skullStream.setDevice(0); skullFile.close(); for (unsigned int i=0;i<ampCount;i++) skullExists[i]=true;
  }

  void loadBrain_ObjFile(QString fn) {
   for (unsigned int i=0;i<ampCount;i++) brainExists[i]=false;
   QFile brainFile; QTextStream brainStream; QString dummyStr; QStringList dummySL,dummySL2; Coord3D c; QVector<int> idx;

   // Reset previous
   for (int i=0;i<brainIndex.size();i++) brainIndex[i].resize(0);
   brainIndex.resize(0); brainCoord.resize(0);
 
   brainFile.setFileName(fn); brainFile.open(QIODevice::ReadOnly|QIODevice::Text); brainStream.setDevice(&brainFile);
   while (!brainStream.atEnd()) {
    dummyStr=brainStream.readLine(); dummySL=dummyStr.split(" ");
    if (dummySL[0]=="v") { c.x=dummySL[1].toFloat(); c.y=dummySL[2].toFloat(); c.z=dummySL[3].toFloat(); brainCoord.append(c); }
    else if (dummySL[0]=="f") { idx.resize(0);
     dummySL2=dummySL[1].split("/"); idx.append(dummySL2[0].toInt());
     dummySL2=dummySL[2].split("/"); idx.append(dummySL2[0].toInt());
     dummySL2=dummySL[3].split("/"); idx.append(dummySL2[0].toInt());
     brainIndex.append(idx);
    }
   } brainStream.setDevice(0); brainFile.close(); for (unsigned int i=0;i<ampCount;i++) brainExists[i]=true;
  }

  void loadReal(QString fileName) {
   QString realLine; QStringList realLines,realValidLines,opts; QFile realFile(fileName); int p,c;
   realFile.open(QIODevice::ReadOnly); QTextStream realStream(&realFile);
   // Read all
   while (!realStream.atEnd()) { realLine=realStream.readLine(160); realLines.append(realLine); } realFile.close();

   // Get valid lines
   for (int i=0;i<realLines.size();i++)
    if (!(realLines[i].at(0)=='#') && realLines[i].split(" ",Qt::SkipEmptyParts).size()>4) realValidLines.append(realLines[i]);

   // Find the essential line defining gizmo names
   for (int ll=0;ll<realValidLines.size();ll++) {
    opts=realValidLines[ll].split(" ",Qt::SkipEmptyParts);
    if (opts.size()==8 && opts[0]=="v") {
     opts.removeFirst(); p=opts[0].toInt(); c=-1;
     for (int i=0;i<acqChannels.size();i++) for (int j=0;j<acqChannels[i].size();j++) {
      if (p==acqChannels[i][j]->physChn) { c=i; break; }
//    if (c!=-1) printf("%d - %d\n",p,c); else qDebug() << "octopus_acq_client: <AcqMaster> <LoadReal> Channel does not exist!";
      acqChannels[i][c]->real[0]=opts[1].toFloat();
      acqChannels[i][c]->real[1]=opts[2].toFloat();
      acqChannels[i][c]->real[2]=opts[3].toFloat();
      acqChannels[i][c]->realS[0]=opts[4].toFloat();
      acqChannels[i][c]->realS[1]=opts[5].toFloat();
      acqChannels[i][c]->realS[2]=opts[6].toFloat();
     }
    } else { qDebug() << "octopus_acq_client: <AcqMaster> <LoadReal> Erroneous real coord file.." << opts.size(); break; }
   } emit repaintGL(4); // Repaint Real coords
  }

  void saveReal(QString fileName) {
   QFile realFile(fileName); realFile.open(QIODevice::WriteOnly); QTextStream realStream(&realFile);
   realStream << "# Octopus real head coordset in headframe xyz's..\n";
   realStream << "# Generated by Octopus-recorder 0.9.5\n\n";
   realStream << "# Coord count = " << acqChannels.size() << "\n";
   for (int i=0;i<acqChannels.size();i++) for (int j=0;j<acqChannels[i].size();j++) {
    realStream << "v " << acqChannels[i][j]->physChn+1 << " "
                       << acqChannels[i][j]->real[0] << " "
                       << acqChannels[i][j]->real[1] << " "
                       << acqChannels[i][j]->real[2] << " "
                       << acqChannels[i][j]->realS[0] << " "
                       << acqChannels[i][j]->realS[1] << " "
                       << acqChannels[i][j]->realS[2] << "\n";
   } realFile.close();
  }

  int eventIndex(int no,int type) { int idx=-1;
   for (int i=0;i<acqEvents.size();i++) {
    if (no==acqEvents[i]->no && type==acqEvents[i]->type) { idx=i; break; }
   } return idx;
  }

  bool clientRunning,recording,withinAvgEpoch,eventOccured; chninfo chnInfo;

  // Non-volatile (read from and saved to octopus.cfg)

  // NET
  QString acqHost; int acqCommPort,acqDataPort;

  // CHN
  QVector<QVector<Channel*> > acqChannels; QVector<Event*> acqEvents;
  QVector<QVector<int> > cntVisChns,cntRecChns,avgVisChns,avgRecChns;

  // GUI
  int ctrlGuiX,ctrlGuiY,ctrlGuiW,ctrlGuiH, contGuiX,contGuiY,contGuiW,contGuiH, gl3DGuiX,gl3DGuiY,gl3DGuiW,gl3DGuiH,
      acqFrameW,acqFrameH,glFrameW,glFrameH;

  // Gizmo
  QVector<Gizmo* > gizmo; QVector<bool> gizmoOnReal,elecOnReal; QVector<int> currentGizmo,currentElectrode,curElecInSeq;

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

 private slots:

  void slotExportAvgs() { QString avgFN;
   QDateTime currentDT(QDateTime::currentDateTime());
   for (int i=0;i<acqEvents.size();i++) {
    if (acqEvents[i]->accepted || acqEvents[i]->rejected) { //Any event exists?
     // Generate filename using current date and time
     // add current data and time to base: trial-20071228-123012-332.oeg
     QString avgFN="Event-"+acqEvents[i]->name+currentDT.toString("yyyyMMdd-hhmmss-zzz");
     avgFile.setFileName(avgFN+".oep");
     if (!avgFile.open(QIODevice::WriteOnly)) {
      qDebug() << "octopus_acq_client: <AcqMaster> <ExportAvgs> Error: Cannot open .oep file for writing."; return;
     } avgStream.setDevice(&avgFile);
     avgStream << (int)(OCTOPUS_ACQ_CLIENT_VER); // Version
     avgStream << sampleRate;		// Sample rate
     avgStream << avgRecChns.size();	// Channel count
     avgStream << acqEvents[i]->name;   // Name of Evt - Cstyle
     avgStream << cp.avgBwd;		// Averaging Window
     avgStream << cp.avgFwd;
     avgStream << acqEvents[i]->accepted; // Accepted count
     avgStream << acqEvents[i]->rejected; // Rejected count

     for (int j=0;j<cp.avgCount;j++) {
      for (int k=0;k<avgRecChns.size();k++) avgStream << &(acqChannels[0][avgRecChns[0][k]]->avgData)[j];
      for (int k=0;k<avgRecChns.size();k++) avgStream << &(acqChannels[0][avgRecChns[0][k]]->stdData)[j];
     } avgFile.close();
    }
   }
  }

  void slotClrAvgs() {
   for (int i=0;i<acqChannels.size();i++) acqChannels[0][i]->resetEvents();
   for (int i=0;i<acqEvents.size();i++) { acqEvents[i]->accepted=acqEvents[i]->rejected=0; }
   emit repaintGL(16); emit repaintHeadWindow();
  }

  void slotAcqReadData() {
   unsigned int acqCurEvent,avgDataCount,avgStartOffset; QVector<float> *avgInChn; //,*stdInChn;
   float n1,k1,k2; unsigned int offsetC,offsetP;
   QDataStream acqDataStream(acqDataSocket);

   while (acqDataSocket->bytesAvailable() >= chnInfo.probe_eeg_msecs*(unsigned int)(sizeof(tcpsample))) {
    acqDataStream.readRawData((char*)(acqCurData.data()),chnInfo.probe_eeg_msecs*(unsigned int)(sizeof(tcpsample)));

    for (unsigned int dOffset=0;dOffset<chnInfo.probe_eeg_msecs;dOffset++) {
     // Check Sample Offset Delta for all amps
     for (unsigned int i=0;i<ampCount;i++) {
      offsetC=(unsigned int)(acqCurData[dOffset].amp[i].offset); offsetP=ampChkP[i]; ampChkP[i]=offsetC;
      if ((offsetC-offsetP)!=1)
       qDebug() << "octopus_acq_client: <AcqMaster> <AcqReadData> Offset leak!!! Amp " << i << " OffsetC->" << offsetC << " OffsetP->" << offsetP;
     }

     if (!(globalCounter%10000)) { // Sort ampChkP and print
      qDebug() << "octopus_acq_client: <AcqMaster> <AcqReadData> Interamp actual sample count Delta span ->" << abs((int)ampChkP[1]-(int)ampChkP[0]);
     } globalCounter++;

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
     float dummyAvg; int notchCount=notchN*(chnInfo.sampleRate/50); int notchStart=(cp.cntPastSize+cp.cntPastIndex-notchCount)%cp.cntPastSize; // -1 ?
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

  // *** POLHEMUS HANDLER ***

  void slotDigMonitor() {
   digitizer->mutex.lock(); sty=digitizer->styF; xp=digitizer->xpF; yp=digitizer->ypF; zp=digitizer->zpF; digitizer->mutex.unlock(); emit repaintGL(1);
  }

  void slotDigResult() {
   digitizer->mutex.lock();
    for (unsigned int i=0;i<ampCount;i++) {
     acqChannels[0][currentElectrode[i]]->real=digitizer->stylusF;
     acqChannels[0][currentElectrode[i]]->realS=digitizer->stylusSF;
    }
   digitizer->mutex.unlock();
   for (unsigned int i=0;i<ampCount;i++) curElecInSeq[i]++;
   for (unsigned int i=0;i<ampCount;i++) {
    if (curElecInSeq[i]==gizmo[currentGizmo[i]]->seq.size()) curElecInSeq[i]=0;
    for (int j=0;j<acqChannels.size();j++) if (acqChannels[i][j]->physChn==gizmo[currentGizmo[i]]->seq[curElecInSeq[i]]) { currentElectrode[i]=j; break; }
   }
   emit repaintHeadWindow(); emit repaintGL(1);
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

  void slotToggleNotch() { if (!notch) notch=true; else notch=false; }
  void slotManualTrig() { acqSendCommand(CS_ACQ_MANUAL_TRIG,AMP_SIMU_TRIG,0,0); }
  void slotManualSync() { acqSendCommand(CS_ACQ_MANUAL_SYNC,AMP_SYNC_TRIG,0,0); }

  // *** TCP HANDLERS

  void slotAcqCommandError(QAbstractSocket::SocketError socketError) {
   switch (socketError) {
    case QAbstractSocket::HostNotFoundError: qDebug() << "octopus_acq_client: <AcqMaster> <AcqCmdErr> ACQuisition command server does not exist!"; break;
    case QAbstractSocket::ConnectionRefusedError: qDebug() << "octopus_acq_client: <AcqMaster> <AcqCmdErr> ACQuisition command server refused connection!"; break;
    default: qDebug() << "octopus_acq_client: <AcqMaster> <AcqCmdErr> ACQuisition command server unknown error!"; break;
   }
  }

  void slotAcqDataError(QAbstractSocket::SocketError socketError) {
   switch (socketError) {
    case QAbstractSocket::HostNotFoundError: qDebug() << "octopus_acq_client: <AcqMaster> <AcqDataErr> ACQuisition data server does not exist!"; break;
    case QAbstractSocket::ConnectionRefusedError: qDebug() << "octopus_acq_client: <AcqMaster> <AcqDataErr> ACQuisition data server refused connection!"; break;
    default: qDebug() << "octopus_acq_client: <AcqMaster> <AcqDataErr> ACQuisition data server unknown error!"; break;
   }
  }

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
};

#endif
