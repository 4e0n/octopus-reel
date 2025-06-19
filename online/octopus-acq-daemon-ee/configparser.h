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
#include "confparam.h"
#include "chninfo.h"

const int MAX_LINE_SIZE=160; // chars

class ConfigParser {
 public:
  ConfigParser(QApplication *app,QString cp) {
   application=app; cfgPath=cp; cfgFile.setFileName(cfgPath);
  }

  void parse(ConfParam *conf,QVector<ChnInfo> *chnTopo) {
   QTextStream cfgStream;
   QStringList cfgLines,opts,opts2,ampSection,netSection,chnSection,guiSection;
   ChnInfo dummyChnInfo;
   if (!cfgFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qDebug() << "octopus_acqd: <ConfigParser> ERROR: Cannot load" << cfgPath;
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
          if (opts[0].trimmed()=="AMP") ampSection.append(opts[1]);
     else if (opts[0].trimmed()=="NET") netSection.append(opts[1]);
     else if (opts[0].trimmed()=="CHN") chnSection.append(opts[1]);
     else if (opts[0].trimmed()=="GUI") guiSection.append(opts[1]);
     else {
      qDebug() << "octopus_acqd: <ConfigParser> ERROR: Unknown section in config file!";
      application->quit();
     }
    }

    // --- Extract variables from separate sections ---

    // AMP section
    if (ampSection.size()>0) {
     for (int i=0;i<ampSection.size();i++) {
      opts=ampSection[i].split("=");
      if (opts[0].trimmed()=="COUNT") {
       conf->ampCount=opts[1].toInt();
       if (conf->ampCount > EE_MAX_AMPCOUNT) {
        qDebug() << "octopus_acqd: <ConfigParser> <AMP> ERROR: Currently more than" \
		 << EE_MAX_AMPCOUNT \
                 << "simultaneously connected amplifiers is not supported!";
        application->quit();
       }
      } else if (opts[0].trimmed()=="BUFPAST") {
       conf->tcpBufSize=opts[1].toInt();
       if (!(conf->tcpBufSize >= 2 && conf->tcpBufSize <= 50)) {
        qDebug() << "octopus_acqd: <ConfigParser> <AMP> ERROR: BUFPAST not within [2,50] seconds range!";
        application->quit();
       }
      } else if (opts[0].trimmed()=="SAMPLERATE") {
       conf->sampleRate=opts[1].toInt();
       if (!(conf->sampleRate == 500 || conf->sampleRate == 1000)) {
        qDebug() << "octopus_acqd: <ConfigParser> <AMP> ERROR: SAMPLERATE not among {500,1000}!";
        application->quit();
       }
      } else if (opts[0].trimmed()=="EEGPROBEMS") {
       conf->eegProbeMsecs=opts[1].toInt();
       if (!(conf->eegProbeMsecs >= 20 && conf->eegProbeMsecs <= 1000)) {
        qDebug() << "octopus_acqd: <ConfigParser> <AMP> ERROR: EEGPROBEMS not within [20,1000] msecs range!";
        application->quit();
       }
      } else if (opts[0].trimmed()=="CMPROBEMS") {
       conf->cmProbeMsecs=opts[1].toInt();
       if (!(conf->cmProbeMsecs >= 500 && conf->cmProbeMsecs <= 2000)) {
        qDebug() << "octopus_acqd: <ConfigParser> <AMP> ERROR: CMPROBEMS not within [500,2000] msecs range!";
        application->quit();
       }
      } else {
       qDebug() << "octopus_acqd: <ConfigParser> <AMP> ERROR: Unknown subsection in AMP section!";
       application->quit();
      }
     }
    } else {
     qDebug() << "octopus_acqd: <ConfigParser> <AMP> ERROR: No parameters in section!";
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
        conf->hostIP=acqHostInfo.addresses().first().toString();
        qDebug() << "octopus_acqd: <ConfigParser> <NET> Host IP is" << conf->hostIP;
        conf->commPort=opts2[1].toInt();
	conf->dataPort=opts2[2].toInt();
        if ((!(conf->commPort >= 1024 && conf->commPort <= 65535)) || // Simple port validation
            (!(conf->dataPort >= 1024 && conf->dataPort <= 65535))) {
         qDebug() << "octopus_acqd: <ConfigParser> <NET> ERROR: Invalid hostname/IP/port settings!";
         application->quit();
        } else {
         qDebug() << "octopus_acqd: <ConfigParser> <NET> CommPort ->" \
                  << conf->commPort << "DataPort ->" << conf->dataPort;
	}
       }
      } else {
       qDebug() << "octopus_acqd: <ConfigParser> <NET> ERROR: Invalid hostname/IP(v4) address!";
       application->quit();
      }
     }
    } else {
     qDebug() << "octopus_acqd: <ConfigParser> <NET> ERROR: No parameters in section!";
     application->quit();
    }

    // CHN section
    if (chnSection.size()>0) {
     for (int i=0;i<chnSection.size();i++) {
      opts=chnSection[i].split("=");
      if (opts[0].trimmed()=="APPEND") {
       opts2=opts[1].split(",");
       if (opts2.size()==6) {
        opts2[1]=opts2[1].trimmed(); // Chn name - Trim wspcs
        if ((!((unsigned)opts2[0].toInt()>0  && (unsigned)opts2[0].toInt()<=512)) || // Channel#
            (!(opts2[1].size()>0 && opts2[1].size()<=3)) || // Channel name must be 1 to 3 chars..
            (!((float)opts2[2].toInt()>=0. && (float)opts2[2].toInt()<360.)) || // TopoThetaPhi - Theta
            (!((float)opts2[3].toInt()>=0. && (float)opts2[3].toInt()<360.)) || // TopoThetaPhi - Phi
            (!((unsigned)opts2[4].toInt()>=1 && (unsigned)opts2[4].toInt()<=11)) || // TopoXY - X
            (!((unsigned)opts2[5].toInt()>=1 && (unsigned)opts2[5].toInt()<=11))) { // TopoXY - Y
         qDebug() << "octopus_acqd: <ConfigParser> <CHN> ERROR: Invalid parameter!";
	 application->quit();
        } else { // Set and append new channel..
	 dummyChnInfo.physChn=opts2[0].toInt();         // Physical channel
	 dummyChnInfo.chnName=opts2[1];                 // Channel name
	 dummyChnInfo.topoTheta=opts2[2].toFloat();     // TopoThetaPhi - Theta
	 dummyChnInfo.topoPhi=opts2[3].toFloat();       // TopoThetaPhi - Phi
	 dummyChnInfo.topoX=opts2[4].toInt();           // TopoXY - X
	 dummyChnInfo.topoY=opts2[5].toInt();           // TopoXY - Y
	 for (unsigned int ii=0;ii<conf->ampCount;ii++) // Reset CM Level
	  dummyChnInfo.cmLevel[ii]=128.0;
         chnTopo->append(dummyChnInfo); // add channel to info table
        }
       } else {
        qDebug() << "octopus_acqd: <ConfigParser> <CHN> ERROR: Invalid count of APPEND parameters!";
	application->quit();
       }
      }
     }
    } else {
     qDebug() << "octopus_acqd: <ConfigParser> <CHN> ERROR: No parameters in section!";
     application->quit();
    }

    //for (int i=0;i<chnTopo.size();i++)
    // qDebug() << chnTopo[i].physChn << chnTopo[i].chnName << chnTopo[i].topoX << chnTopo[i].topoY;

    // GUI section
    if (guiSection.size()>0) {
     for (int i=0;i<guiSection.size();i++) {
      opts=guiSection[i].split("=");
      if (opts[0].trimmed()=="ACQ") {
       opts2=opts[1].split(",");
       if (opts2.size()==3) {
        conf->acqGuiX=opts2[0].toInt();
	conf->acqGuiY=opts2[1].toInt();
	conf->cmCellSize=opts2[2].toInt();
        if ((!(conf->acqGuiX >= -4000 && conf->acqGuiX <= 4000)) ||
            (!(conf->acqGuiY >= -3000 && conf->acqGuiY <= 3000)) ||
            (!(conf->cmCellSize >= 40 && conf->cmCellSize <= 80))) {
         qDebug() << "octopus_acqd: <ConfigParser> <GUI> ERROR: Window size settings not in appropriate range!";
         application->quit();
        }
       } else {
        qDebug() << "octopus_acqd: <ConfigParser> <GUI> ERROR: Invalid count of parameters!";
	application->quit();
       }
      }
     }
    } else {
     qDebug() << "octopus_acqd: <ConfigParser> <GUI> ERROR: No parameters in section!";
     application->quit();
    }
   } // File open
  }

 private:
  QApplication *application;
  QString cfgPath,cfgLine; QFile cfgFile;
};

#endif
