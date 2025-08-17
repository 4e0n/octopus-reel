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

#ifndef CONFIGPARSER_H
#define CONFIGPARSER_H

#include <QObject>
#include <QApplication>
#include <QWidget>
#include <QStatusBar>
#include <QLabel>
#include <QtNetwork>
#include <QThread>
#include <QVector>
#include <QMutex>
#include <unistd.h>

#include "../acqglobals.h"
#include "../serial_device.h"
#include "confparam.h"
#include "chninfo.h"
#include "headmodel.h"

const int MAX_LINE_SIZE=160; // chars

class ConfigParser {
 public:
  ConfigParser(QApplication *app) { application=app; }

  void parse(QString cfgPath,ConfParam *conf,QVector<ChnInfo> *chnTopo,serial_device *serial,HeadModel *headModel) {
   QFile cfgFile; QTextStream cfgStream; QString cfgLine;
   QStringList cfgLines,opts,opts2,bufSection,netSection,chnSection,avgSection,evtSection,guiSection,digSection,modSection;
   ChnInfo dummyChnInfo;

   cfgFile.setFileName(cfgPath);
   if (!cfgFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qDebug() << "octopus_acq_client: <ConfigParser> ERROR: Cannot load" << cfgPath;
    application->quit();
   } else {
    cfgStream.setDevice(&cfgFile);

    while (!cfgStream.atEnd()) { // Populate cfgLines with valid lines
     cfgLine=cfgStream.readLine(MAX_LINE_SIZE); // Should not start with #, should contain "|"
     if (!(cfgLine.at(0)=='#') && cfgLine.contains('|')) cfgLines.append(cfgLine);
    }
    cfgFile.close();

    // Separate AMP, NET, CHN, GUI parameter lines
    for (int i=0;i<cfgLines.size();i++) {
     opts=cfgLines[i].split("|");
          if (opts[0].trimmed()=="BUF") bufSection.append(opts[1]);
     else if (opts[0].trimmed()=="NET") netSection.append(opts[1]);
     else if (opts[0].trimmed()=="CHN") chnSection.append(opts[1]);
     else if (opts[0].trimmed()=="AVG") avgSection.append(opts[1]);
     else if (opts[0].trimmed()=="EVT") evtSection.append(opts[1]);
     else if (opts[0].trimmed()=="GUI") guiSection.append(opts[1]);
     else if (opts[0].trimmed()=="DIG") digSection.append(opts[1]);
     else if (opts[0].trimmed()=="MOD") modSection.append(opts[1]);
     else {
      qDebug() << "octopus_acq_client: <ConfigParser> ERROR: Unknown section in config file!";
      application->quit();
     }
    }

    // --- Extract variables from separate sections ---

    // BUF section
    if (bufSection.size()>0) {
     for (int i=0;i<bufSection.size();i++) {
      opts=bufSection[i].split("=");
      if (opts[0].trimmed()=="PAST") {
       conf->cntPastSize=opts[1].toInt();
       if (!(conf->cntPastSize >= 1000 && conf->cntPastSize <= 20000)) {
        qDebug() << "octopus_acq_client: <ConfigParser> <BUF> ERROR: PAST not within inclusive (1000,20000) range!";
	application->quit();
       }
      } else {
       qDebug() << "octopus_acq_client: <ConfigParser> <BUF> ERROR: Unknown subsection in BUF section!";
       application->quit();
      }
     }
    } else {
     qDebug() << "octopus_acq_client: <ConfigParser> <BUF> ERROR: No parameters in section!";
     application->quit();
    }

    // NET section
    if (netSection.size()>0) {
     for (int i=0;i<netSection.size();i++) {
      opts=netSection[i].split("=");
      if (opts[0].trimmed()=="ACQ") {
       opts2=opts[1].split(","); // IP, ConfigPort and DataPort are separated by ","
       if (opts2.size()==3) {
        QHostInfo acqHostInfo=QHostInfo::fromName(opts2[0].trimmed());
        conf->ipAddr=acqHostInfo.addresses().first().toString();
        qDebug() << "octopus_acq_client: <ConfigParser> <NET> Host IP is" << conf->ipAddr;
        conf->commPort=opts2[1].toInt();
	conf->dataPort=opts2[2].toInt();
        if ((!(conf->commPort >= 1024 && conf->commPort <= 65535)) || // Simple port validation
            (!(conf->dataPort >= 1024 && conf->dataPort <= 65535))) {
         qDebug() << "octopus_acq_client: <ConfigParser> <NET> ERROR: Invalid hostname/IP/port settings!";
         application->quit();
        } else {
         qDebug() << "octopus_acq_client: <ConfigParser> <NET> CommPort ->" \
                  << conf->commPort << "DataPort ->" << conf->dataPort;
	}
       }
      } else {
       qDebug() << "octopus_acq_client: <ConfigParser> <NET> ERROR: Invalid hostname/IP(v4) address!";
       application->quit();
      }
     }
    } else {
     qDebug() << "octopus_acq_client: <ConfigParser> <NET> ERROR: No parameters in section!";
     application->quit();
    }

    // CHN section
    if (chnSection.size()>0) {
     for (int i=0;i<chnSection.size();i++) {
      opts=chnSection[i].split("=");
      if (opts[0].trimmed()=="APPEND") {
       opts2=opts[1].split(",");
       if (opts2.size()==4) {
	dummyChnInfo.physChn=(unsigned)(opts2[0].toInt());     // Physical channel
        dummyChnInfo.chnName=opts2[1].trimmed();   // Channel name
	dummyChnInfo.topoTheta=opts2[2].toFloat(); // TopoThetaPhi - Theta
	dummyChnInfo.topoPhi=opts2[3].toFloat();   // TopoThetaPhi - Phi
        if ((!(dummyChnInfo.physChn>0  && dummyChnInfo.physChn<=66)) || // Channel# - max channels is assumed as 66
            (!(dummyChnInfo.chnName.size()>0 && dummyChnInfo.chnName.size()<=3)) || // Channel name must be 1 to 3 chars..
            (!(dummyChnInfo.topoTheta>=0. && dummyChnInfo.topoTheta<360.)) ||
            (!(dummyChnInfo.topoPhi>=0. && dummyChnInfo.topoPhi<360.))) {
         qDebug() << "octopus_acq_client: <ConfigParser> <CHN> ERROR: Invalid APPEND parameter!";
	 application->quit();
        } else { // Set and append new channel..
         chnTopo->append(dummyChnInfo); // add channel to info table
        }
       } else {
        qDebug() << "octopus_acq_client: <ConfigParser> <CHN> ERROR: Invalid count of APPEND parameters!";
	application->quit();
       }
      }
     }
    } else {
     qDebug() << "octopus_acq_client: <ConfigParser> <CHN> ERROR: No parameters in section!";
     application->quit();
    }

    //for (int i=0;i<chnTopo.size();i++)
    // qDebug() << chnTopo[i].physChn << chnTopo[i].chnName << chnTopo[i].topoX << chnTopo[i].topoY;

//         dummyChn=new Channel(opts2[0].toInt()-1,  // Physical channel (0-indexed)
//                              opts2[1].trimmed(),  // Channel Name
//                              opts2[2].toInt(),	   // Rejection Level
//                              opts2[3].toInt()-1,  // Rejection Reference Channel for that channel (0-indexed)
//                              opts2[4],opts2[5],   // Cnt Vis/Rec Flags
//                              opts2[6],opts2[7],   // Avg Vis/Rec Flags
//                              opts2[8].toFloat(),  // Electrode Cart. Coords
//                              opts2[9].toFloat()); // (Theta,Phi)
//         dummyChn->pastData.resize(cp.cntPastSize);
//         dummyChn->pastFilt.resize(cp.cntPastSize); // Line-noise component
//         dummyChn->setEventProfile(acqEvents.size(),cp.avgCount);
//         for (int amp=0;amp<acqChannels.size();amp++) acqChannels[amp].append(dummyChn); // add channel for all respective amplifiers

    // AVG section
    if (avgSection.size()>0) {
     for (int i=0;i<avgSection.size();i++) {
      opts=avgSection[i].split("=");
      if (opts[0].trimmed()=="INTERVAL") {
       opts2=opts[1].split(",");
       if (opts2.size()==4) {
        conf->rejBwd=opts2[0].toInt(); conf->avgBwd=opts2[1].toInt();
	conf->avgFwd=opts2[2].toInt(); conf->rejFwd=opts2[3].toInt();
        if ((!(conf->rejBwd>=-1000 && conf->rejBwd<=0)) || (!(conf->avgBwd>=-1000 && conf->avgBwd<=0)) ||
            (!(conf->avgFwd>=0 && conf->avgFwd<=1000)) || (!(conf->rejFwd>=0 && conf->rejFwd<=1000)) ||
            (conf->rejBwd>conf->avgBwd) || (conf->avgBwd>conf->avgFwd) || (conf->avgFwd>conf->rejFwd)) {
         qDebug() << "octopus_acq_client: <ConfigParser> <AVG> ERROR: Invalid INTERVAL parameter!";
         application->quit();
        }
       } else {
        qDebug() << "octopus_acq_client: <ConfigParser> <AVG> ERROR: Invalid count of INTERVAL parameters!";
        application->quit();
       }
      }
     }
    } else {
     qDebug() << "octopus_acq_client: <ConfigParser> <AVG> ERROR: No parameters in section!";
     application->quit();
    }

    // EVT - Event Space and Hierarchy
    if (evtSection.size()>0) {
     for (int i=0;i<evtSection.size();i++) {
      opts=evtSection[i].split("=");
      if (opts[0].trimmed()=="STIM") {
       opts2=opts[1].split(",");
       if (opts2.size()==3) {
        if ((!(opts2[0].toInt()>0  && opts2[0].toInt()<256)) ||
            (!(opts2[1].size()>0)) ||
            (!(opts2[2].toInt()>=0 && opts2[2].toInt()<100))) { // Color palette index - max of 100 RGB combinations
         qDebug() << "octopus_acq_client: <ConfigParser> <EVT> ERROR: Invalid STIM parameter!";
         application->quit();
        } else { // Add to the list of pre-recognized events.
         ;
         //dummyEvt=new Event(opts2[0].toInt(),opts2[1].trimmed(),1,QColor(opts2[2].toInt(),opts2[3].toInt(),opts2[4].toInt(),255));
         //acqEvents.append(dummyEvt);
        }
       } else {
        qDebug() << "octopus_acq_client: <ConfigParser> <EVT> ERROR: Invalid count of STIM parameters!";
        application->quit();
       }
      }
     }
    } else {
     qDebug() << "octopus_acq_client: <ConfigParser> <EVT> ERROR: No parameters in section!";
     application->quit();
    }

    // GUI section
    if (guiSection.size()>0) {
     for (int i=0;i<guiSection.size();i++) {
      opts=guiSection[i].split("=");
      if (opts[0].trimmed()=="CTRL") {
       opts2=opts[1].split(",");
       if (opts2.size()==4) {
        conf->guiCtrlX=opts2[0].toInt(); conf->guiCtrlY=opts2[1].toInt();
        conf->guiCtrlW=opts2[2].toInt(); conf->guiCtrlH=opts2[3].toInt();
        if ((!(conf->guiCtrlX >= -4000 && conf->guiCtrlX <= 4000)) ||
            (!(conf->guiCtrlY >= -3000 && conf->guiCtrlY <= 3000)) ||
            (!(conf->guiCtrlW >=   400 && conf->guiCtrlW <= 2000)) ||
            (!(conf->guiCtrlH >=    60 && conf->guiCtrlH <= 1800))) {
         qDebug() << "octopus_acq_client: <ConfigParser> <GUI> <CTRL> ERROR: Window size settings not in appropriate range!";
         application->quit();
        }
       } else {
        qDebug() << "octopus_acq_client: <ConfigParser> <GUI> ERROR: Invalid count of parameters!";
	application->quit();
       }
      } else if (opts[0].trimmed()=="STRM") {
       opts2=opts[1].split(",");
       if (opts2.size()==4) {
        conf->guiStrmX=opts2[0].toInt(); conf->guiStrmY=opts2[1].toInt();
        conf->guiStrmW=opts2[2].toInt(); conf->guiStrmH=opts2[3].toInt();
        if ((!(conf->guiStrmX >= -4000 && conf->guiStrmX <= 4000)) ||
            (!(conf->guiStrmY >= -3000 && conf->guiStrmY <= 3000)) ||
            (!(conf->guiStrmW >=   400 && conf->guiStrmW <= 4000)) ||
            (!(conf->guiStrmH >=   800 && conf->guiStrmH <= 4000))) {
         qDebug() << "octopus_acq_client: <ConfigParser> <GUI> <STRM> ERROR: Window size settings not in appropriate range!";
         application->quit();
        }
       } else {
        qDebug() << "octopus_acq_client: <ConfigParser> <GUI> ERROR: Invalid count of parameters!";
	application->quit();
       }
      } else if (opts[0].trimmed()=="HEAD") { // Head frame within STRM window
       opts2=opts[1].split(",");
       if (opts2.size()==4) {
        conf->guiHeadX=opts2[0].toInt(); conf->guiHeadY=opts2[1].toInt();
        conf->guiHeadW=opts2[2].toInt(); conf->guiHeadH=opts2[3].toInt();
        if ((!(conf->guiHeadX >= -2000 && conf->guiHeadX <= 2000)) ||
            (!(conf->guiHeadY >= -2000 && conf->guiHeadY <= 2000)) ||
            (!(conf->guiHeadW >=   400 && conf->guiHeadW <= 2000)) ||
            (!(conf->guiHeadH >=   300 && conf->guiHeadH <= 2000))) {
         qDebug() << "octopus_acq_client: <ConfigParser> <GUI> <HEAD> ERROR: Window size settings not in appropriate range!";
         application->quit();
        }
       } else {
        qDebug() << "octopus_acq_client: <ConfigParser> <GUI> ERROR: Invalid count of parameters!";
	application->quit();
       }
      }
     }
    } else {
     qDebug() << "octopus_acq_client: <ConfigParser> <GUI> ERROR: No parameters in section!";
     application->quit();
    }
 
    // DIG section
    if (digSection.size()>0) {
     for (int i=0;i<digSection.size();i++) {
      opts=digSection[i].split("=");
      if (opts[0].trimmed()=="POLHEMUS") {
       opts2=opts[1].split(",");
       if (opts2.size()==5) {
        if ((!(opts2[0].toInt()>=0 && opts2[0].toInt()<8)) || // ttyS#
            (!(opts2[1].toInt()==9600 ||
               opts2[1].toInt()==19200 ||
               opts2[1].toInt()==38400 ||
               opts2[1].toInt()==57600 ||
               opts2[1].toInt()==115200)) || // BaudRate
            (!(opts2[2].toInt()==7 || opts2[2].toInt()==8)) || // Data Bits
            (!(opts2[3].trimmed()=="E" || opts2[3].trimmed()=="O" || opts2[3].trimmed()=="N")) || // Parity
            (!(opts2[4].toInt()==0 || opts2[4].toInt()==1))) { // Stop Bit
         qDebug() << "octopus_acq_client: <ConfigParser> <DIG> ERROR: Window size settings not in appropriate range!";
	 application->quit();
        } else {
         serial->devname="/dev/ttyS"+opts2[0].trimmed();
         switch (opts2[1].toInt()) {
          case   9600: serial->baudrate=B9600;   break;
          case  19200: serial->baudrate=B19200;  break;
          case  38400: serial->baudrate=B38400;  break;
          case  57600: serial->baudrate=B57600;  break;
          case 115200: serial->baudrate=B115200; break;
         }
         switch (opts2[2].toInt()) {
          case 7: serial->databits=CS7; break;
          case 8: serial->databits=CS8; break;
         }
         if (opts2[3].trimmed()=="N") {
          serial->parity=serial->par_on=0;
	 } else if (opts2[3].trimmed()=="O") {
          serial->parity=PARODD; serial->par_on=PARENB;
         } else if (opts2[3].trimmed()=="E") {
          serial->parity=0; serial->par_on=PARENB;
         }
         switch (opts2[4].toInt()) {
          default:
          case 0: serial->stopbit=0; break;
          case 1: serial->stopbit=CSTOPB; break;
         }
        }
       } else {
        qDebug() << "octopus_acq_client: <ConfigParser> <DIG> ERROR: Invalid count of parameters!";
	application->quit();
       }
      } else {
       qDebug() << "octopus_acq_client: <ConfigParser> <DIG> ERROR: No parameters in section!";
       application->quit();
      }
     }
    }

    // MOD section
    if (modSection.size()>0) {
     for (int i=0;i<modSection.size();i++) {
      opts=modSection[i].split("=");
      if (opts[0].trimmed()=="GIZMO") {
       opts2=opts[1].split(",");
       if (opts2.size()==1) {
        if (opts2[0].size()) headModel->loadGizmo_OgzFile(opts2[0].trimmed());
        else {
         qDebug() << "octopus_acq_client: <ConfigParser> <MOD> <GIZMO> ERROR: filename error!";
         application->quit();
	}
       } else {
        qDebug() << "octopus_acq_client: <ConfigParser> <MOD> <GIZMO> ERROR: while parsing parameters!";
	application->quit();
       }
      } else if (opts[0].trimmed()=="SCALP" || opts[0].trimmed()=="SKULL" || opts[0].trimmed()=="BRAIN") {
       opts2=opts[1].split(",");
       if (opts2.size()==2) {
	int headNo=opts2[0].toInt()-1; 
        if (!(headNo>=0 && headNo<headModel->getHeadCount())) {
         qDebug() << "octopus_acq_client: <ConfigParser> <MOD> <HEAD> ERROR: head# out of bounds!";
         application->quit();
        } else {
         if (opts[0].trimmed()=="SCALP") {
          headModel->loadScalp_ObjFile(opts2[1].trimmed(),headNo);
	 } else if (opts[0].trimmed()=="SKULL") {
          headModel->loadSkull_ObjFile(opts2[1].trimmed(),headNo);
	 } else if (opts[0].trimmed()=="BRAIN") {
          headModel->loadBrain_ObjFile(opts2[1].trimmed(),headNo);
	 } else {
          qDebug() << "octopus_acq_client: <ConfigParser> <MOD> <SCALP|SKULL|BRAIN> ERROR: while parsing parameters!";
          application->quit();
         }
	 //else if (opts[0].trimmed()=="SKULL") headModel->loadSkull_ObjFile(opts2[0].trimmed(),headNo);
         //   else {
         //    qDebug() << "octopus_acq_client: <ConfigParser> <MOD> <SKULL> ERROR: while parsing parameters!";
         //    application->quit();
         //   } else if (opts[0].trimmed()=="BRAIN") headModel->loadBrain_ObjFile(opts2[0].trimmed(),headNo);
         // else {
         //  qDebug() << "octopus_acq_client: <ConfigParser> <MOD> <BRAIN> ERROR: while parsing parameters!";
         //  application->quit();
         // }
         //} else {
         // qDebug() << "octopus_acq_client: <ConfigParser> <MOD> ERROR: No parameters in section!";
         // application->quit();
         //}
        }
       }
      }
     }    
    }

   } // File open
  }

 private:
  QApplication *application;
};

#endif

/*    // AMP section
    if (ampSection.size()>0) {
     for (int i=0;i<ampSection.size();i++) {
      opts=ampSection[i].split("=");
      if (opts[0].trimmed()=="COUNT") {
       conf->ampCount=opts[1].toInt();
       if (conf->ampCount > EE_MAX_AMPCOUNT) {
        qDebug() << "octopus_acq_client: <ConfigParser> <AMP> ERROR: Currently more than" \
		 << EE_MAX_AMPCOUNT \
                 << "simultaneously connected amplifiers is not supported!";
        application->quit();
       }
      } else if (opts[0].trimmed()=="BUFPAST") {
       conf->tcpBufSize=opts[1].toInt();
       if (!(conf->tcpBufSize >= 2 && conf->tcpBufSize <= 50)) {
        qDebug() << "octopus_acq_client: <ConfigParser> <AMP> ERROR: BUFPAST not within [2,50] seconds range!";
        application->quit();
       }
      } else if (opts[0].trimmed()=="SAMPLERATE") {
       conf->sampleRate=opts[1].toInt();
       if (!(conf->sampleRate == 500 || conf->sampleRate == 1000)) {
        qDebug() << "octopus_acq_client: <ConfigParser> <AMP> ERROR: SAMPLERATE not among {500,1000}!";
        application->quit();
       }
      } else if (opts[0].trimmed()=="EEGPROBEMS") {
       conf->eegProbeMsecs=opts[1].toInt();
       if (!(conf->eegProbeMsecs >= 20 && conf->eegProbeMsecs <= 1000)) {
        qDebug() << "octopus_acq_client: <ConfigParser> <AMP> ERROR: EEGPROBEMS not within [20,1000] msecs range!";
        application->quit();
       }
      } else if (opts[0].trimmed()=="CMPROBEMS") {
       conf->cmProbeMsecs=opts[1].toInt();
       if (!(conf->cmProbeMsecs >= 500 && conf->cmProbeMsecs <= 2000)) {
        qDebug() << "octopus_acq_client: <ConfigParser> <AMP> ERROR: CMPROBEMS not within [500,2000] msecs range!";
        application->quit();
       }
      } else {
       qDebug() << "octopus_acq_client: <ConfigParser> <AMP> ERROR: Unknown subsection in AMP section!";
       application->quit();
      }
     }
    } else {
     qDebug() << "octopus_acq_client: <ConfigParser> <AMP> ERROR: No parameters in section!";
     application->quit();
    }
*/

//conf->avgCount=(cp.avgFwd-cp.avgBwd)*sampleRate/1000; cp.rejCount=(cp.rejFwd-cp.rejBwd)*sampleRate/1000;
//cp.postRejCount=(cp.rejFwd-cp.avgFwd)*sampleRate/1000; cp.bwCount=cp.rejFwd*sampleRate/1000;
//     ctrlGuiX=60; ctrlGuiY=60; ctrlGuiW=640; ctrlGuiH=480;
//     contGuiX=60; contGuiY=60; contGuiW=640; contGuiH=480;
//     gl3DGuiX=160; gl3DGuiY=160; gl3DGuiW=400; gl3DGuiH=300;
