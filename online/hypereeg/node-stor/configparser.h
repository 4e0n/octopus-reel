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

#include <QHostInfo>
#include <QFile>
#include <QColor>
#include "confparam.h"

const int MAX_LINE_SIZE=160; // chars
const QString NODE_TEXT="STOR";

class ConfigParser {
 public:
  ConfigParser(QString cp) { cfgPath=cp; cfgFile.setFileName(cfgPath); }

  bool parse(ConfParam *conf) { QString commResponse; QStringList sList,sList2;
   QTextStream cfgStream; QStringList cfgLines,opts,opts2,netSection,chnSection;
   QStringList bufSection;

   if (!cfgFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qDebug() << "node-stor: <ConfigParser> ERROR: Cannot load" << cfgPath;
    return true;
   } else {
    cfgStream.setDevice(&cfgFile);
    while (!cfgStream.atEnd()) { // Populate cfgLines with valid lines
     cfgLine=cfgStream.readLine(MAX_LINE_SIZE); // Should not start with #, should contain "|"
     if (!(cfgLine.at(0)=='#') && cfgLine.contains('|')) cfgLines.append(cfgLine);
    }
    cfgFile.close();

    int idx1=-1,idx2=cfgLines.size()-1;
    for (int idx=0;idx<cfgLines.size();idx++) { opts=cfgLines[idx].split("|");
     if (opts[0].trimmed()=="NODE" && opts[1].trimmed()==NODE_TEXT) { idx1=idx; break; }
    }
    if (idx1<0) {
     qDebug() << "node_acq: <ConfigParser> ERROR: NODE section does not exist in config file!";
     return true;
    }
    idx1++;
    for (int idx=idx1;idx<cfgLines.size();idx++) { opts=cfgLines[idx].split("|");
     if (opts[0].trimmed()=="NODE") { idx2=idx; break; }
    }

    // Separate section parameter lines
    for (int idx=idx1;idx<idx2;idx++) {
     opts=cfgLines[idx].split("#"); opts=opts[0].split("|"); // Get rid off any following comment.
          if (opts[0].trimmed()=="NET") netSection.append(opts[1]);
     else if (opts[0].trimmed()=="BUF") bufSection.append(opts[1]);
     else {
      qDebug() << "node-stor: <ConfigParser> ERROR: Unknown section in config file!";
      return true;
     }
    }

    // --- Extract variables from separate sections ---

    // NET section
    if (netSection.size()>0) {
     for (const auto& sect:netSection) {
      opts=sect.split("=");
      if (opts[0].trimmed()=="ACQ") {
       opts2=opts[1].split(","); // IP, command port, stream port and commonmode port are separated by ","
       if (opts2.size()==3) {
        QHostInfo acqHostInfo=QHostInfo::fromName(opts2[0].trimmed());
        conf->acqIpAddr=acqHostInfo.addresses().first().toString();
        conf->acqCommPort=opts2[1].toInt();
        conf->acqStrmPort=opts2[2].toInt();
        if ((!(conf->acqCommPort >= 65000 && conf->acqCommPort < 65500)) || // Simple port validation
            (!(conf->acqStrmPort >= 65000 && conf->acqStrmPort < 65500))) {
         qDebug() << "node-stor: <ConfigParser> <NET> ERROR: Invalid (serving) hostname/IP/port settings!";
         return true;
        }
       } else {
        qDebug() << "node-stor: <ConfigParser> <NET> ERROR: Invalid (serving) count of NET|IN params!";
        return true;
       }
       qDebug() << "node-stor:" << conf->acqIpAddr << conf->acqCommPort << conf->acqStrmPort;
      } else if (opts[0].trimmed()=="STOR") {
       opts2=opts[1].split(","); // IP, command port, stream port and commonmode port are separated by ","
       if (opts2.size()==2) {
        QHostInfo acqHostInfo=QHostInfo::fromName(opts2[0].trimmed());
        conf->storIpAddr=acqHostInfo.addresses().first().toString();
        conf->storCommPort=opts2[1].toInt();
        if ((!(conf->storCommPort >= 65000 && conf->storCommPort < 65500))) { // Simple port validation
         qDebug() << "node-stor: <ConfigParser> <NET> ERROR: Invalid hostname/IP/port settings!";
         return true;
        }
       } else {
        qDebug() << "node-stor: <ConfigParser> <NET> ERROR: Invalid count of NET|OUT params!";
        return true;
       }
      } else {
       qDebug() << "node-stor: <ConfigParser> <NET> ERROR: Invalid hostname/IP(v4) address!";
       return true;
      }
     }
    } else {
     qDebug() << "node-stor: <ConfigParser> <NET> ERROR: No parameters in section!";
     return true;
    }

    // BUF section
    if (bufSection.size()>0) {
     for (const auto& sect:bufSection) {
      opts=sect.split("=");
      if (opts[0].trimmed()=="PAST") {
       conf->tcpBufSize=opts[1].toInt();
       if (!(conf->tcpBufSize>=5 && conf->tcpBufSize<=20)) {
        qDebug() << "node-stor: <ConfigParser> <BUF> ERROR: PAST not within inclusive (5,20) seconds range!";
        return true;
       }
      } else {
       qDebug() << "node-stor: <ConfigParser> <BUF> ERROR: Invalid section command!";
       return true;
      }
     }
    } else {
     qDebug() << "node-stor: <ConfigParser> <BUF> ERROR: No parameters in section!";
     return true;
    }

    // ------------------------------------------------------------------

    // Setup command socket
    conf->acqCommSocket->connectToHost(conf->acqIpAddr,conf->acqCommPort); conf->acqCommSocket->waitForConnected();

    // Get crucial info from the "master" node we connect to
    commResponse=conf->commandToDaemon(conf->acqCommSocket,CMD_ACQ_GETCONF);
    if (!commResponse.isEmpty()) qDebug() << "node-stor: <ConfigParser> Daemon replied:" << commResponse;
    else qDebug() << "node-stor: <ConfigParser> (TIMEOUT) No response from master node!";
    sList=commResponse.split(",");

    conf->ampCount=sList[0].toInt(); // (ACTUAL) AMPCOUNT
    conf->eegRate=sList[1].toInt();  // EEG SAMPLERATE

    conf->tcpBufSize*=conf->eegRate;          // TCPBUFSIZE (in SAMPLE#)
    conf->halfTcpBufSize=conf->tcpBufSize/2;  // (for fast-checks of population)

    conf->refChnCount=sList[2].toInt();
    conf->bipChnCount=sList[3].toInt();
    conf->refGain=sList[4].toFloat();
    conf->bipGain=sList[5].toFloat();
    conf->eegProbeMsecs=sList[6].toInt(); // This determines the (maximum/optimal) data feed rate together with eegRate
    conf->eegSamplesInTick=conf->eegRate*conf->eegProbeMsecs/1000;

    // CHANNELS

    commResponse=conf->commandToDaemon(conf->acqCommSocket,CMD_ACQ_GETCHAN);
    if (!commResponse.isEmpty()) qDebug() << "node-stor: <Config> Daemon replied:" << commResponse;
    else qDebug() << "node-stor: <Config> No response or timeout.";
    sList=commResponse.split("\n"); StorChnInfo chn;

    for (int chnIdx=0;chnIdx<sList.size();chnIdx++) { // Individual CHANNELs information
     sList2=sList[chnIdx].split(",");
     chn.physChn=sList2[0].toInt(); chn.chnName=sList2[1];
     chn.type=(bool)sList2[6].toInt();
     conf->chns.append(chn);
    }
    conf->chnCount=conf->chns.size();

    //for (int idx=0;idx<conf->chns.size();idx++) {
    // QString x="";
    // for (int j=0;j<conf->ampCount;j++) x.append(QString::number(conf->chns[idx].interMode[j])+",");
    // qDebug() << x.toUtf8();
    //}

    for (auto& chn:conf->chns) qDebug() << chn.physChn << chn.chnName << chn.type;


   } // File open
   return false;
  }

 private:
  QString cfgPath,cfgLine; QFile cfgFile;
};
