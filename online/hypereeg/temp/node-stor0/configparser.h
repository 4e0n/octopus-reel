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

class ConfigParser {
 public:
  ConfigParser(QString cp) { cfgPath=cp; cfgFile.setFileName(cfgPath); }

  bool parse(ConfParam *conf) {
   QTextStream cfgStream; QString commResponse;
   QStringList cfgLines,opts,opts2,sList,sList2,netSection,bufSection,chnSection;

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
     if (opts[0].trimmed()=="NODE" && opts[1].trimmed()=="STOR") { idx1=idx; break; }
    }
    if (idx1<0) {
     qDebug() << "node_stor: <ConfigParser> ERROR: NODE|TIME section does not exist in config file!";
     return true;
    }
    idx1++;
    for (int idx=idx1;idx<cfgLines.size();idx++) { opts=cfgLines[idx].split("|");
     if (opts[0].trimmed()=="NODE") { idx2=idx; break; }
    }

    //qDebug() << "node_stor: <ConfigParser> Indices" << idx1 << idx2;
    //qDebug() << cfgLines[idx2];

    // Separate BUF, NET, CHN parameter lines
    //for (auto& cl:cfgLines) {
    for (int idx=idx1;idx<idx2;idx++) {
     opts=cfgLines[idx].split("#"); opts=opts[0].split("|"); // Get rid off any following comment.
          if (opts[0].trimmed()=="NET") netSection.append(opts[1]);
     else if (opts[0].trimmed()=="BUF") bufSection.append(opts[1]);
     else if (opts[0].trimmed()=="CHN") chnSection.append(opts[1]); // AVG merged in CHN
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
      if (opts[0].trimmed()=="IN") {
       opts2=opts[1].split(","); // IP, command port, stream port and commonmode port are separated by ","
       if (opts2.size()==3) {
        QHostInfo acqHostInfo=QHostInfo::fromName(opts2[0].trimmed());
        conf->ipAddr=acqHostInfo.addresses().first().toString();
        conf->commPort=opts2[1].toInt();
        conf->strmPort=opts2[2].toInt();
        if ((!(conf->commPort >= 65000 && conf->commPort < 65500)) || // Simple port validation
            (!(conf->strmPort >= 65000 && conf->strmPort < 65500))) {
         qDebug() << "node-stor: <ConfigParser> <NET> ERROR: Invalid (uplink) hostname/IP/port settings!";
         return true;
        }
       } else {
        qDebug() << "node-stor: <ConfigParser> <NET> ERROR: Invalid (uplink) count of NET|IN params!";
        return true;
       }
       qDebug() << "node-stor:" << conf->ipAddr << conf->commPort << conf->strmPort;
      } else if (opts[0].trimmed()=="OUT") {
       opts2=opts[1].split(","); // IP, command port, stream port and commonmode port are separated by ","
       if (opts2.size()==2) {
        QHostInfo acqHostInfo=QHostInfo::fromName(opts2[0].trimmed());
        conf->svrIpAddr=acqHostInfo.addresses().first().toString();
        conf->svrCommPort=opts2[1].toInt();
        if ((!(conf->svrCommPort >= 65000 && conf->svrCommPort < 65500))) { // Simple port validation
         qDebug() << "node-stor: <ConfigParser> <NET> ERROR: Invalid (downlink) hostname/IP/port settings!";
         return true;
        }
       } else {
        qDebug() << "node-stor: <ConfigParser> <NET> ERROR: Invalid (downlink) count of NET|OUT params!";
        return true;
       }
       qDebug() << "node-stor:" << conf->svrIpAddr << conf->svrCommPort;
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
    conf->commSocket->connectToHost(conf->ipAddr,conf->commPort); conf->commSocket->waitForConnected();

    // Get more crucial info from master node
    commResponse=conf->commandToDaemon(CMD_ACQ_GETCONF);
    if (!commResponse.isEmpty()) qDebug() << "node-stor: <ConfigParser> Daemon replied:" << commResponse;
    else qDebug() << "node-stor: <ConfigParser> (Timeout) No response from master node!";
    sList=commResponse.split(",");

    conf->ampCount=sList[0].toInt();      // (ACTUAL) AMPCOUNT
    conf->eegRate=sList[1].toInt();       // EEG SAMPLERATE

    conf->tcpBufSize*=conf->eegRate;          // TCPBUFSIZE (in SAMPLE#)
    conf->halfTcpBufSize=conf->tcpBufSize/2;  // (for fast-checks of population)
    conf->tcpBuffer.resize(conf->tcpBufSize);

    conf->refChnCount=sList[2].toInt();
    conf->bipChnCount=sList[3].toInt();
    conf->refGain=sList[4].toFloat();
    conf->bipGain=sList[5].toFloat();
    conf->eegProbeMsecs=sList[6].toInt(); // This determines the maximum data feed rate together with eegRate
    conf->eegSamplesInTick=conf->eegRate*conf->eegProbeMsecs/1000;

    // CHANNELS

    commResponse=conf->commandToDaemon(CMD_ACQ_GETCHAN);
    if (!commResponse.isEmpty()) qDebug() << "node-stor: <Config> Daemon replied:" << commResponse;
    else qDebug() << "node-stor: <Config> No response or timeout.";
    sList=commResponse.split("\n"); ChnInfo chn;

    for (int chnIdx=0;chnIdx<sList.size();chnIdx++) { // Individual CHANNELs information
     sList2=sList[chnIdx].split(",");
     chn.physChn=sList2[0].toInt(); chn.chnName=sList2[1];
     //chn.topoTheta=sList2[2].toFloat(); chn.topoPhi=sList2[3].toFloat();
     chn.param.x=1.0; chn.param.y=sList2[2].toFloat(); chn.param.z=sList2[3].toFloat();
     chn.topoX=sList2[4].toInt(); chn.topoY=sList2[5].toInt();
     chn.isBipolar=(bool)sList2[6].toInt();
     conf->chns.append(chn);
    }
    conf->chnCount=conf->chns.size();

    //for (int idx=0;idx<conf->chns.size();idx++) {
    // QString x="";
    // for (int j=0;j<conf->chns[idx].interElec.size();j++) x.append(QString::number(conf->chns[idx].interElec[j])+",");
    // qDebug() << x.toUtf8();
    //}

    for (auto& chn:conf->chns) qDebug() << chn.physChn << chn.chnName << chn.param.y << chn.param.z << chn.isBipolar;

   } // File open
   return false;
  }

 private:
  QString cfgPath,cfgLine; QFile cfgFile;
};
