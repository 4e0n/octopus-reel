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

#pragma once

#include <QHostInfo>
#include <QFile>
#include <QColor>
#include "confparam.h"
#include "headmodel.h"

const int MAX_LINE_SIZE=160; // chars

class ConfigParser {
 public:
  ConfigParser(QString cp) { cfgPath=cp; cfgFile.setFileName(cfgPath); }

  bool parse(ConfParam *conf) {
   QTextStream cfgStream; QString commResponse;
   QStringList cfgLines,opts,opts2,sList,sList2,netSection,chnSection;

   QStringList bufSection,pltSection,evtSection,guiSection,headSection;

   if (!cfgFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qDebug() << "node_compute: <ConfigParser> ERROR: Cannot load" << cfgPath;
    return true;
   } else {
    cfgStream.setDevice(&cfgFile);

    while (!cfgStream.atEnd()) { // Populate cfgLines with valid lines
     cfgLine=cfgStream.readLine(MAX_LINE_SIZE); // Should not start with #, should contain "|"
     if (!(cfgLine.at(0)=='#') && cfgLine.contains('|')) cfgLines.append(cfgLine);
    }
    cfgFile.close();

    // Separate AMP, NET, CHN, GUI parameter lines
    for (auto& cl:cfgLines) {
     opts=cl.split("#"); opts=opts[0].split("|"); // Get rid off any following comment.
          if (opts[0].trimmed()=="NET") netSection.append(opts[1]);
     else if (opts[0].trimmed()=="BUF") bufSection.append(opts[1]);
     else if (opts[0].trimmed()=="PLT") pltSection.append(opts[1]);
     else if (opts[0].trimmed()=="EVT") evtSection.append(opts[1]);
     else if (opts[0].trimmed()=="CHN") chnSection.append(opts[1]); // AVG merged in CHN
     else if (opts[0].trimmed()=="GUI") guiSection.append(opts[1]);
     else if (opts[0].trimmed()=="HEAD") headSection.append(opts[1]);
     else {
      qDebug() << "node_compute: <ConfigParser> ERROR: Unknown section in config file!";
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
       if (opts2.size()==4) {
        QHostInfo acqHostInfo=QHostInfo::fromName(opts2[0].trimmed());
        conf->ipAddr=acqHostInfo.addresses().first().toString();
        conf->commPort=opts2[1].toInt();
        conf->strmPort=opts2[2].toInt();
        conf->cmodPort=opts2[3].toInt();
        if ((!(conf->commPort >= 65000 && conf->commPort < 65500)) || // Simple port validation
            (!(conf->strmPort >= 65000 && conf->strmPort < 65500)) ||
            (!(conf->cmodPort >= 65000 && conf->cmodPort < 65500))) {
         qDebug() << "node_compute: <ConfigParser> <NET> ERROR: Invalid (uplink) hostname/IP/port settings!";
         return true;
        }
       } else {
        qDebug() << "node_compute: <ConfigParser> <NET> ERROR: Invalid (uplink) count of NET|IN params!";
        return true;
       }
       qDebug() << "node_compute:" << conf->ipAddr << conf->commPort << conf->strmPort << conf->cmodPort;
      } else if (opts[0].trimmed()=="OUT") {
       opts2=opts[1].split(","); // IP, command port, stream port and commonmode port are separated by ","
       if (opts2.size()==4) {
        QHostInfo acqHostInfo=QHostInfo::fromName(opts2[0].trimmed());
        conf->svrIpAddr=acqHostInfo.addresses().first().toString();
        conf->svrCommPort=opts2[1].toInt();
        conf->svrStrmPort=opts2[2].toInt();
        conf->svrCmodPort=opts2[3].toInt();
        if ((!(conf->svrCommPort >= 65000 && conf->svrCommPort < 65500)) || // Simple port validation
            (!(conf->svrStrmPort >= 65000 && conf->svrStrmPort < 65500)) ||
            (!(conf->svrCmodPort >= 65000 && conf->svrCmodPort < 65500))) {
         qDebug() << "node_compute: <ConfigParser> <NET> ERROR: Invalid (downlink) hostname/IP/port settings!";
         return true;
        }
       } else {
        qDebug() << "node_compute: <ConfigParser> <NET> ERROR: Invalid (downlink) count of NET|OUT params!";
        return true;
       }
       qDebug() << "node_compute:" << conf->svrIpAddr << conf->svrCommPort << conf->svrStrmPort << conf->svrCmodPort;
      } else {
       qDebug() << "node_compute: <ConfigParser> <NET> ERROR: Invalid hostname/IP(v4) address!";
       return true;
      }
     }
    } else {
     qDebug() << "node_compute: <ConfigParser> <NET> ERROR: No parameters in section!";
     return true;
    }

    // BUF section
    if (bufSection.size()>0) {
     for (const auto& sect:bufSection) {
      opts=sect.split("=");
      if (opts[0].trimmed()=="PAST") {
       conf->tcpBufSize=opts[1].toInt();
       if (!(conf->tcpBufSize>=5 && conf->tcpBufSize<=20)) {
        qDebug() << "node_compute: <ConfigParser> <BUF> ERROR: PAST not within inclusive (5,20) seconds range!";
        return true;
       }
      } else {
       qDebug() << "node_compute: <ConfigParser> <BUF> ERROR: Invalid section command!";
       return true;
      }
     }
    } else {
     qDebug() << "node_compute: <ConfigParser> <BUF> ERROR: No parameters in section!";
     return true;
    }

    // ------------------------------------------------------------------

    // Setup command socket
    conf->commSocket->connectToHost(conf->ipAddr,conf->commPort); conf->commSocket->waitForConnected();

    // Get more crucial info from master node
    commResponse=conf->commandToDaemon(CMD_ACQD_GETCONF);
    if (!commResponse.isEmpty()) qDebug() << "node_compute: <ConfigParser> Daemon replied:" << commResponse;
    else qDebug() << "node_compute: <ConfigParser> (Timeout) No response from master node!";
    sList=commResponse.split(",");

    conf->initMultiAmp(sList[0].toInt()); // (ACTUAL) AMPCOUNT
    conf->sampleRate=sList[1].toInt();    // SAMPLERATE

    conf->tcpBufSize*=conf->sampleRate;       // TCPBUFSIZE (in SAMPLE#)
    conf->halfTcpBufSize=conf->tcpBufSize/2;  // (for fast-checks of population)
    conf->tcpBuffer.resize(conf->tcpBufSize);

    conf->refChnCount=sList[2].toInt();
    conf->bipChnCount=sList[3].toInt();
    conf->refGain=sList[4].toFloat();
    conf->bipGain=sList[5].toFloat();
    conf->eegProbeMsecs=sList[6].toInt(); // This determines the maximum data feed rate together with sampleRate

    conf->eegSweepDivider=conf->eegSweepCoeff[0];
    conf->eegSweepFrameTimeMs=conf->sampleRate/conf->eegSweepRefreshRate; // (1000sps/50Hz)=20ms=20samples
    // # of data to wait for, to be available for screen plot/sweeper
    conf->scrAvailableSamples=conf->eegSweepFrameTimeMs*(conf->sampleRate/1000); // 20ms -> 20 sample

    // CHANNELS

    commResponse=conf->commandToDaemon(CMD_ACQD_GETCHAN);
    if (!commResponse.isEmpty()) qDebug() << "node_compute: <Config> Daemon replied:" << commResponse;
    else qDebug() << "node_compute: <Config> No response or timeout.";
    sList=commResponse.split("\n"); ChnInfo chn;

    for (int chnIdx=0;chnIdx<sList.size();chnIdx++) { // Individual CHANNELs information
     sList2=sList[chnIdx].split(",");
     chn.physChn=sList2[0].toInt(); chn.chnName=sList2[1];
     //chn.topoTheta=sList2[2].toFloat(); chn.topoPhi=sList2[3].toFloat();
     chn.param.x=1.0; chn.param.y=sList2[2].toFloat(); chn.param.z=sList2[3].toFloat();
     chn.topoX=sList2[4].toInt(); chn.topoY=sList2[5].toInt();
     chn.isBipolar=(bool)sList2[6].toInt();
     chn.chnViewMode.resize(conf->ampCount);
     for (int ampIdx=0;ampIdx<conf->ampCount;ampIdx++) chn.chnViewMode[ampIdx]=(unsigned int)sList2[7].toInt();
     chn.interElec.resize(0); for (int idx=0;idx<sList2[8].toInt();idx++) chn.interElec.append(sList2[8+idx].toInt());
     conf->chns.append(chn);
    }
    conf->chnCount=conf->chns.size();

    //for (int idx=0;idx<conf->chns.size();idx++) {
    // QString x="";
    // for (int j=0;j<conf->chns[idx].interElec.size();j++) x.append(QString::number(conf->chns[idx].interElec[j])+",");
    // qDebug() << x.toUtf8();
    //}

    conf->s0.resize(conf->scrAvailableSamples); // Mean+StD buffers for GUI sweeper
    conf->s0s.resize(conf->scrAvailableSamples);
    conf->s1.resize(conf->scrAvailableSamples);
    conf->s1s.resize(conf->scrAvailableSamples);
    for (unsigned int idx=0;idx<conf->scrAvailableSamples;idx++) {
     conf->s0[idx].resize(conf->chnCount); conf->s0s[idx].resize(conf->chnCount);
     conf->s1[idx].resize(conf->chnCount); conf->s1s[idx].resize(conf->chnCount);
    }

    for (auto& chn:conf->chns) qDebug() << chn.physChn << chn.chnName << chn.param.y << chn.param.z << chn.isBipolar;

    // Initialize GUI/sweeping related variables
    conf->eegSweepPending.resize(conf->ampCount);
    for (unsigned int ampIdx=0;ampIdx<conf->ampCount;ampIdx++) {
     conf->eegSweepPending[ampIdx]=false;
    }

    conf->headModel.resize(conf->ampCount);
    conf->glFrameOn.resize(conf->ampCount); conf->glGridOn.resize(conf->ampCount);
    conf->glGizmoOn.resize(conf->ampCount); conf->glElectrodesOn.resize(conf->ampCount);
    conf->glScalpOn.resize(conf->ampCount); conf->glSkullOn.resize(conf->ampCount); conf->glBrainOn.resize(conf->ampCount);
    for (unsigned int ampIdx=0;ampIdx<conf->ampCount;ampIdx++) {
     conf->headModel[ampIdx].init();
     conf->glFrameOn[ampIdx]=conf->glGridOn[ampIdx]=true;
     conf->glGizmoOn[ampIdx]=conf->glElectrodesOn[ampIdx]=true;
     conf->glScalpOn[ampIdx]=conf->glSkullOn[ampIdx]=conf->glBrainOn[ampIdx]=true;
    }

    // ------------------------------------------------------------------

    // PLT - Color palette entries
    int colR,colG,colB,colA; 
    if (pltSection.size()>0) {
     for (const auto& sect:pltSection) {
      opts=sect.split("=");
      if (opts[0].trimmed()=="ADD") {
       opts2=opts[1].split(",");
       if (opts2.size()==4) {
        colR=opts2[0].toInt(); colG=opts2[1].toInt(); colB=opts2[2].toInt(); colA=opts2[3].toInt();
        if (!(colR>=0 && colR<256) || !(colG>=0 && colG<256) || !(colB>=0 && colB<256) || !(colA>=0 && colA<256)) {
         qDebug() << "node_compute: <ConfigParser> <PLT> ERROR: Invalid RGBcolor parameter!";
	 return true;
        } else { // Add to the list of recognized colors.
	 conf->rgbPalette.append(QColor(colR,colG,colB,colA));
        }
       } else {
        qDebug() << "node_compute: <ConfigParser> <PLT> ERROR: Invalid count of RGBcolor parameters!";
	return true;
       }
      } else {
       qDebug() << "node_compute: <ConfigParser> <PLT> ERROR: Invalid section command!";
       return true;
      }
     }
    } else {
     qDebug() << "node_compute: <ConfigParser> <PLT> ERROR: No parameters in section!";
     return true;
    }

    // EVT - Event Space and Hierarchy
    int evtNo,evtColIdx; QString evtName;
    if (evtSection.size()>0) {
     for (const auto& sect:evtSection) {
      opts=sect.split("=");
      if (opts[0].trimmed()=="INTERVAL") {
       opts2=opts[1].split(",");
       if (opts2.size()==4) {
        conf->erpRejBwd=opts2[0].toFloat(); conf->erpAvgBwd=opts2[1].toFloat();
        conf->erpAvgFwd=opts2[2].toFloat(); conf->erpRejFwd=opts2[3].toFloat();
	//qDebug() << conf->erpRejBwd << conf->erpAvgBwd << conf->erpAvgFwd << conf->erpRejFwd;
        if (!(conf->erpRejBwd>=-2000 && conf->erpRejBwd<=2000 && conf->erpAvgBwd>=-2000 && conf->erpAvgBwd<=2000 &&
              conf->erpAvgFwd>=-2000 && conf->erpAvgFwd<=2000 && conf->erpRejFwd>=-2000 && conf->erpRejFwd<=2000) ||
            conf->erpRejBwd>conf->erpAvgBwd || conf->erpAvgBwd>conf->erpAvgFwd || conf->erpAvgFwd>conf->erpRejFwd) {
         qDebug() << "node_compute: <ConfigParser> <EVT> ERROR: Invalid INTERVAL parameter!";
         return true;
        }
       } else {
        qDebug() << "node_compute: <ConfigParser> <EVT> ERROR: Invalid count of INTERVAL parameters!";
        return true;
       }
      } else if (opts[0].trimmed()=="ADD") {
       opts2=opts[1].split(",");
       evtNo=opts2[0].toInt(); evtName=opts2[1].trimmed(); evtColIdx=opts2[2].toInt();
       if (opts2.size()==3) {
        if (!(evtNo>0 && evtNo<256) || (evtName.size()>100) || // max 100 chars for name should be more than enough?
            !(evtColIdx>=0 && evtColIdx<conf->rgbPalette.size())) { // should be within range of present color entries
         qDebug() << "node_compute: <ConfigParser> <EVT> ERROR: Invalid EventNo parameter!";
         return true;
        } else { // Add to the list of pre-recognized events.
	 conf->events.append(Event(evtNo,evtName,evtColIdx));
        }
       } else {
        qDebug() << "node_compute: <ConfigParser> <EVT> ERROR: Invalid count of STIM parameters!";
        return true;
       }
      } else {
       qDebug() << "node_compute: <ConfigParser> <EVT> ERROR: Invalid section command!";
       return true;
      }
     }
    } else {
     qDebug() << "node_compute: <ConfigParser> <EVT> ERROR: No parameters in section!";
     return true;
    }

    // HEAD section
    if (headSection.size()>0) {
     for (const auto& sect:headSection) {
      opts=sect.split("=");
      if (opts[0].trimmed()=="GIZMO") {
       opts2=opts[1].split(",");
       if (opts2.size()==1) {
        if (opts2[0].size()) {
         conf->loadGizmo(opts2[0].trimmed());

	 //for (int g=0;g<conf->glGizmo.size();g++) {
         // for (int s=0;s<conf->glGizmo[g].seq.size();s++) std::cout << conf->glGizmo[g].seq[s] << " ";
	 // std::cout << "\n";
         // for (int s=0;s<conf->glGizmo[g].tri.size();s++)
         //  std::cout << "(" << conf->glGizmo[g].tri[s][0] << "," << conf->glGizmo[g].tri[s][1] << "," <<  conf->glGizmo[g].tri[s][2] << ")";
	 // std::cout << "\n";
         //}

        } else {
         qDebug() << "node_compute: <ConfigParser> <HEAD> <GIZMO> ERROR: filename error!";
         return true;
        }
       } else {
        qDebug() << "node_compute: <ConfigParser> <HEAD> <GIZMO> ERROR: while parsing parameters!";
        return true;
       }
      } else if (opts[0].trimmed()=="SCALP" || opts[0].trimmed()=="SKULL" || opts[0].trimmed()=="BRAIN") {
       opts2=opts[1].split(",");
       if (opts2.size()==2) {
        int headNo=opts2[0].toInt()-1; 
        if (!(headNo>=0 && headNo<(int)conf->ampCount)) { // ampCount should be determined by now.
         qDebug() << "node_compute: <ConfigParser> <HEAD> <SCALP|SKULL|BRAIN> ERROR: head# overflow!";
         return true;
        } else {
         if (opts[0].trimmed()=="SCALP") {
          conf->headModel[headNo].loadScalp(opts2[1].trimmed());
         } else if (opts[0].trimmed()=="SKULL") {
          conf->headModel[headNo].loadSkull(opts2[1].trimmed());
         } else if (opts[0].trimmed()=="BRAIN") {
          conf->headModel[headNo].loadBrain(opts2[1].trimmed());
         } else {
          qDebug() << "node_compute: <ConfigParser> <HEAD> <SCALP|SKULL|BRAIN> ERROR: while parsing parameters!";
          return true;
         }
        }
       }
      }
     }
    }

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
         qDebug() << "node_compute: <ConfigParser> <GUI> <CTRL> ERROR: Window size settings not in appropriate range!";
         return true;
        }
       } else {
        qDebug() << "node_compute: <ConfigParser> <GUI> ERROR: Invalid count of parameters!";
        return true;
       }
      } else if (opts[0].trimmed()=="STRM") {
       opts2=opts[1].split(",");
       if (opts2.size()==4) {
        conf->guiAmpX=opts2[0].toInt(); conf->guiAmpY=opts2[1].toInt();
        conf->guiAmpW=opts2[2].toInt(); conf->guiAmpH=opts2[3].toInt();
        if ((!(conf->guiAmpX >= -4000 && conf->guiAmpX <= 4000)) ||
            (!(conf->guiAmpY >= -3000 && conf->guiAmpY <= 3000)) ||
            (!(conf->guiAmpW >=   400 && conf->guiAmpW <= 4000)) ||
            (!(conf->guiAmpH >=   800 && conf->guiAmpH <= 4000))) {
         qDebug() << "node_compute: <ConfigParser> <GUI> <STRM> ERROR: Window size settings not in appropriate range!";
	 return true;
        }
       } else {
        qDebug() << "node_compute: <ConfigParser> <GUI> ERROR: Invalid count of parameters!";
	return true;
       }
      }
     }
    } else {
     qDebug() << "node_compute: <ConfigParser> <GUI> ERROR: No parameters in section!";
     return true;
    }

   } // File open
   return false;
  }

 private:
  QString cfgPath,cfgLine; QFile cfgFile;
};
