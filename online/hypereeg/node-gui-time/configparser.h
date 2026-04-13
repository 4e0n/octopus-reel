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
const QString NODE_TEXT="TIME";

class ConfigParser {
 public:
  ConfigParser(QString cp) { cfgPath=cp; cfgFile.setFileName(cfgPath); }

  bool parse(ConfParam *conf) { QString commResponse; QStringList sList,sList2;
   QTextStream cfgStream; QStringList cfgLines,opts,opts2,netSection,chnSection;
   QStringList bufSection,pltSection,evtSection,guiSection,headSection;

   if (!cfgFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qWarning() << "node-time: <ConfigParser> ERROR: Cannot load" << cfgPath;
    return true;
   } else {
    cfgStream.setDevice(&cfgFile);
    while (!cfgStream.atEnd()) { // Populate cfgLines with valid lines
     cfgLine=cfgStream.readLine(MAX_LINE_SIZE).trimmed(); // Should not start with #, should contain "|"
     if (!cfgLine.isEmpty() && !cfgLine.startsWith('#') && cfgLine.contains('|')) cfgLines.append(cfgLine);
    }
    cfgFile.close();

    int idx1=-1,idx2=cfgLines.size()-1;
    for (int idx=0;idx<cfgLines.size();idx++) { opts=cfgLines[idx].split("|");
     if (opts[0].trimmed()=="NODE" && opts[1].trimmed()==NODE_TEXT) { idx1=idx; break; }
    }
    if (idx1<0) {
     qWarning() << "node-time: <ConfigParser> ERROR: NODE section does not exist in config file!";
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
     else if (opts[0].trimmed()=="CHN") chnSection.append(opts[1]); // AVG merged in CHN
     else if (opts[0].trimmed()=="GUI") guiSection.append(opts[1]);
     else {
      qWarning() << "node-time: <ConfigParser> ERROR: Unknown section in config file!";
      return true;
     }
    }

    // --- Extract variables from separate sections ---

    // NET section
    if (netSection.size()>0) {
     for (const auto& sect:netSection) {
      opts=sect.split("=");

      if (opts[0].trimmed()=="ACQ") {
       opts2=opts[1].split(","); // IP and command port
       if (opts2.size()==2) {
        QHostInfo acqHostInfo=QHostInfo::fromName(opts2[0].trimmed());
        conf->acqIpAddr=acqHostInfo.addresses().first().toString();
        conf->acqCommPort=opts2[1].toInt();
        if ((!(conf->acqCommPort >= 65000 && conf->acqCommPort < 65999))) { // Simple port validation
         qWarning() << "node-time: <ConfigParser> <STRM> ERROR: Invalid (serving) hostname/IP/port settings!";
         return true;
        }
       } else {
        qWarning() << "node-time: <ConfigParser> <STRM> ERROR: Invalid (serving) count of STRM|IN params!";
        return true;
       }
       qInfo() << "node-time:" << conf->acqIpAddr << conf->acqCommPort;
      } else if (opts[0].trimmed()=="COMPPP") {
       opts2=opts[1].split(","); // IP, command port and stream port
       if (opts2.size()==3) {
        QHostInfo compPPHostInfo=QHostInfo::fromName(opts2[0].trimmed());
        conf->compPPIpAddr=compPPHostInfo.addresses().first().toString();
        conf->compPPCommPort=opts2[1].toInt();
        conf->compPPStrmPort=opts2[2].toInt();
        if ((!(conf->compPPCommPort >= 65000 && conf->compPPCommPort < 65999)) || // Simple port validation
            (!(conf->compPPStrmPort >= 65000 && conf->compPPStrmPort < 65999))) {
         qWarning() << "node-time: <ConfigParser> <STRM> ERROR: Invalid (serving) hostname/IP/port settings!";
         return true;
        }
       } else {
        qWarning() << "node-time: <ConfigParser> <STRM> ERROR: Invalid (serving) count of STRM|IN params!";
        return true;
       }
       qInfo() << "node-time:" << conf->compPPIpAddr << conf->compPPCommPort << conf->compPPStrmPort;
      } else if (opts[0].trimmed()=="WAVPLAY") {
       opts2=opts[1].split(","); // IP, command port and stream port
       if (opts2.size()==2) {
        QHostInfo wavPlayHostInfo=QHostInfo::fromName(opts2[0].trimmed());
        conf->wavPlayIpAddr=wavPlayHostInfo.addresses().first().toString();
        conf->wavPlayCommPort=opts2[1].toInt();
        if ((!(conf->wavPlayCommPort >= 65000 && conf->wavPlayCommPort < 65999))) { // Simple port validation
         qWarning() << "node-time: <ConfigParser> <STRM> ERROR: Invalid (serving) hostname/IP/port settings!";
         return true;
        }
       } else {
        qWarning() << "node-time: <ConfigParser> <STRM> ERROR: Invalid (serving) count of STRM|IN params!";
        return true;
       }
       qInfo() << "node-time:" << conf->wavPlayIpAddr << conf->wavPlayCommPort;
      } else if (opts[0].trimmed()=="STOR") {
       opts2=opts[1].split(","); // IP, command port, stream port and commonmode port are separated by ","
       if (opts2.size()==2) {
        QHostInfo storHostInfo=QHostInfo::fromName(opts2[0].trimmed());
        conf->storIpAddr=storHostInfo.addresses().first().toString();
        conf->storCommPort=opts2[1].toInt();
        if ((!(conf->storCommPort >= 65000 && conf->storCommPort < 65999))) { // Simple port validation
         qWarning() << "node-time: <ConfigParser> <STRM> ERROR: Invalid hostname/IP/port settings!";
         return true;
        }
       } else {
        qWarning() << "node-time: <ConfigParser> <STRM> ERROR: Invalid count of STRM|OUT params!";
        return true;
       }
      } else {
       qWarning() << "node-time: <ConfigParser> <STRM> ERROR: Invalid hostname/IP(v4) address!";
       return true;
      }
     }
    } else {
     qWarning() << "node-time: <ConfigParser> <STRM> ERROR: No parameters in section!";
     return true;
    }

    // BUF section
    if (bufSection.size()>0) {
     for (const auto& sect:bufSection) {
      opts=sect.split("=");
      if (opts[0].trimmed()=="PAST") {
       conf->tcpBufSize=opts[1].toInt();
       if (!(conf->tcpBufSize>=1 && conf->tcpBufSize<=200)) {
        qWarning() << "node-time: <ConfigParser> <BUF> ERROR: PAST not within inclusive (1,200) seconds range!";
        return true;
       }
      } else {
       qWarning() << "node-time: <ConfigParser> <BUF> ERROR: Invalid section command!";
       return true;
      }
     }
    } else {
     qWarning() << "node-time: <ConfigParser> <BUF> ERROR: No parameters in section!";
     return true;
    }

    // ------------------------------------------------------------------

    // Setup ACQ command socket -- For sending operator events
    conf->acqCommSocket->connectToHost(conf->acqIpAddr,conf->acqCommPort); conf->acqCommSocket->waitForConnected();

    // Setup WAVPlay command socket -- For sending operator events
    conf->wavPlayCommSocket->connectToHost(conf->wavPlayIpAddr,conf->wavPlayCommPort); conf->wavPlayCommSocket->waitForConnected();

    // Setup COMPPP command socket
    conf->compPPCommSocket->connectToHost(conf->compPPIpAddr,conf->compPPCommPort); conf->compPPCommSocket->waitForConnected();
    // Get crucial info from the "acquisition" node we connect to
    commResponse=conf->commandToDaemon(conf->compPPCommSocket,CMD_ACQ_GETCONF);
    if (!commResponse.isEmpty()) qInfo() << "node-time: <GetConfFromAcqDaemon> Reply:" << commResponse;
    else qCritical() << "node-time: <ConfigParser> (TIMEOUT) No response from Acquisition Node!";
    sList=commResponse.split(",");
    conf->ampCount=sList[0].toInt();      // (ACTUAL) AMPCOUNT
    conf->eegRate=sList[1].toInt();       // EEG SAMPLERATE
    conf->initMultiAmp();
    conf->tcpBufSize*=conf->eegRate;      // TCPBUFSIZE (in SAMPLE#)
    conf->tcpBuffer.resize(conf->tcpBufSize);

    conf->refChnCount=sList[2].toInt();
    conf->bipChnCount=sList[3].toInt();
    conf->metaChnCount=sList[4].toInt();
    conf->physChnCount=sList[5].toInt();
    conf->chnCount=sList[6].toInt();      // logical count from node-acq metadata
    conf->totalChnCount=sList[7].toInt();
    conf->totalCount=sList[8].toInt();
    conf->refGain=sList[9].toFloat();
    conf->bipGain=sList[10].toFloat();
    conf->eegProbeMsecs=sList[11].toInt(); // This determines the (maximum/optimal) data feed rate together with eegRate
    conf->eegSamplesInTick=conf->eegRate*conf->eegProbeMsecs/1000;
    conf->frameBytes=sList[12].toInt();

    // # of data to wait for, to be available for screen plot/sweeper
    // (1000sps/50Hz)/(1000/1000)=20ms=20samples
    conf->scrAvailableSamples=(conf->eegRate/conf->eegSweepRefreshRate)*(conf->eegRate/1000); // 20ms -> 20 sample
    conf->scrUpdateSamples=conf->scrAvailableSamples;
#ifdef EEGBANDSCOMP
    conf->eegBand=0;
#endif

    const unsigned int ampCount=conf->ampCount;
    const unsigned int refChnCount=conf->refChnCount; const unsigned int bipChnCount=conf->bipChnCount;
    const unsigned int physChnCount=conf->physChnCount; const unsigned int metaChnCount=conf->metaChnCount;
    const unsigned int chnCount=conf->chnCount;

    // CHANNELS

    commResponse=conf->commandToDaemon(conf->compPPCommSocket,CMD_ACQ_GETCHAN);
    if (!commResponse.isEmpty()) qInfo() << "node-time: <GetChannelListFromDaemon> ChannelList received."; // << commResponse;
    else qCritical() << "node-time: <GetChannelListFromDaemon> (TIMEOUT) No response from Node!";
    sList=commResponse.split("\n"); GUIChnInfo chn;

    chn.interMode.resize(ampCount);
    for (unsigned int chnIdx=0;chnIdx<refChnCount;chnIdx++) { // Individual CHANNELs information
     sList2=sList[chnIdx].split(",");
     chn.physChn=sList2[0].toInt(); chn.chnName=sList2[1];
     chn.topoX=sList2[4].toInt(); chn.topoY=sList2[5].toInt();
     chn.type=sList2[6].toInt();
     for (unsigned int imIdx=0;imIdx<ampCount;imIdx++) chn.interMode[imIdx]=sList2[7+imIdx].toInt();
     //chn.interMode.resize(0); for (unsigned int idx=0;idx<ampCount;idx++) chn.interMode.append(sList2[7+idx].toInt());
     conf->refChns.append(chn);
    }
    for (unsigned int chnIdx=0;chnIdx<bipChnCount;chnIdx++) { // Individual CHANNELs information
     sList2=sList[refChnCount+chnIdx].split(",");
     chn.physChn=sList2[0].toInt(); chn.chnName=sList2[1];
     chn.topoX=sList2[4].toInt(); chn.topoY=sList2[5].toInt();
     chn.type=sList2[6].toInt();
     for (unsigned int imIdx=0;imIdx<ampCount;imIdx++) chn.interMode[imIdx]=sList2[7+imIdx].toInt();
     //chn.interMode.resize(0); for (unsigned int idx=0;idx<ampCount;idx++) chn.interMode.append(sList2[7+idx].toInt());
     conf->bipChns.append(chn);
    }
    for (unsigned int chnIdx=0;chnIdx<metaChnCount;chnIdx++) { // Individual CHANNELs information
     sList2=sList[physChnCount+chnIdx].split(",");
     chn.physChn=sList2[0].toInt(); chn.chnName=sList2[1];
     chn.topoX=sList2[4].toInt(); chn.topoY=sList2[5].toInt();
     chn.type=sList2[6].toInt();
     for (unsigned int imIdx=0;imIdx<ampCount;imIdx++) chn.interMode[imIdx]=sList2[7+imIdx].toInt();
     //chn.interMode.resize(0); for (unsigned int idx=0;idx<ampCount;idx++) chn.interMode.append(sList2[7+idx].toInt());
     conf->metaChns.append(chn);
    }

    qInfo() << "[TIME] ampCount=" << ampCount << " chnCount=" << chnCount << " frameBytes=" << conf->frameBytes;

    //for (int idx=0;idx<conf->refChns.size();idx++) {
    // QString x="";
    // for (int j=0;j<conf->ampCount;j++) x.append(QString::number(conf->refChns[idx].interMode[j])+",");
    // qInfo() << x.toUtf8();
    //}

    qInfo() << "Referential Chns:";
    for (auto& chn:conf->refChns) qInfo() << chn.physChn << chn.chnName << chn.type;
    qInfo() << "Differential Chns:";
    for (auto& chn:conf->bipChns) qInfo() << chn.physChn << chn.chnName << chn.type;
    qInfo() << "Meta Chns:";
    for (auto& chn:conf->metaChns) qInfo() << chn.physChn << chn.chnName << chn.type;

    // Initialize GUI/sweeping related variables
    conf->eegSweepPending.resize(ampCount);
    for (unsigned int ampIdx=0;ampIdx<ampCount;ampIdx++) {
     conf->eegSweepPending[ampIdx]=false;
    }

    // Setup STOR command socket
    conf->storCommSocket->connectToHost(conf->storIpAddr,conf->storCommPort); conf->storCommSocket->waitForConnected();

    // Get crucial info from the "storage" node we connect to
    commResponse=conf->commandToDaemon(conf->storCommSocket,CMD_STATUS);
    if (!commResponse.isEmpty()) qInfo() << "node-time: <ConfigParser> Storage Daemon replied:" << commResponse;
    else qWarning() << "node-time: <ConfigParser> (TIMEOUT) No response from Storage Node!";

    // GUI section
    if (guiSection.size()>0) {
     for (const auto& sect:guiSection) {
      opts=sect.split("=");
      if (opts[0].trimmed()=="CTRL") {
       opts2=opts[1].split(",");
       if (opts2.size()==4) {
        conf->guiCtrlX=opts2[0].toInt(); conf->guiCtrlY=opts2[1].toInt();
        conf->guiCtrlW=opts2[2].toInt(); conf->guiCtrlH=opts2[3].toInt();
        if ((!(conf->guiCtrlX >= -4000 && conf->guiCtrlX <= 4000)) ||
            (!(conf->guiCtrlY >= -3000 && conf->guiCtrlY <= 3000)) ||
            (!(conf->guiCtrlW >=   400 && conf->guiCtrlW <= 2000)) ||
            (!(conf->guiCtrlH >=    60 && conf->guiCtrlH <= 1800))) {
         qWarning() << "node-time: <ConfigParser> <GUI> <CTRL> ERROR: Window size settings not in appropriate range!";
         return true;
        }
       } else {
        qWarning() << "node-time: <ConfigParser> <GUI> ERROR: Invalid count of parameters!";
        return true;
       }
      } else if (opts[0].trimmed()=="STRM") {
       opts2=opts[1].split(",");
       if (opts2.size()==4) {
        conf->guiAmpX=opts2[0].toInt(); conf->guiAmpY=opts2[1].toInt();
        conf->guiAmpW=opts2[2].toInt(); conf->guiAmpH=opts2[3].toInt();
        if ((!(conf->guiAmpX >= -4000 && conf->guiAmpX <= 4000)) ||
            (!(conf->guiAmpY >= -3000 && conf->guiAmpY <= 3000)) ||
            (!(conf->guiAmpW >=   200 && conf->guiAmpW <= 4000)) ||
            (!(conf->guiAmpH >=   200 && conf->guiAmpH <= 4000))) {
         qWarning() << "node-time: <ConfigParser> <GUI> <STRM> ERROR: Window size settings not in appropriate range!";
	 return true;
        }
       } else {
        qWarning() << "node-time: <ConfigParser> <GUI> ERROR: Invalid count of parameters!";
	return true;
       }
      }
     }
    } else {
     qWarning() << "node-time: <ConfigParser> <GUI> ERROR: No parameters in section!";
     return true;
    }

   } // File open
   return false;
  }

 private:
  QString cfgPath,cfgLine; QFile cfgFile;
};
