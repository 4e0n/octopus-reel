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

#include <QHostInfo>
#include <QFile>
#include <QColor>
#include "confparam.h"

const int MAX_LINE_SIZE=160; // chars

class ConfigParser {
 public:
  ConfigParser(QString cp) { cfgPath=cp; cfgFile.setFileName(cfgPath); }

  bool parse(ConfParam *conf) {
   QTextStream cfgStream;
   QStringList cfgLines,opts,opts2,netSection,chnSection;

   QStringList bufSection,pltSection,evtSection,guiSection,modSection;

   if (!cfgFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qDebug() << "hnode_strm_gui: <ConfigParser> ERROR: Cannot load" << cfgPath;
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
     else if (opts[0].trimmed()=="MOD") modSection.append(opts[1]);
     else {
      qDebug() << "hnode_strm_gui: <ConfigParser> ERROR: Unknown section in config file!";
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
         qDebug() << "hnode_strm_gui: <ConfigParser> <NET> ERROR: Invalid (uplink) hostname/IP/port settings!";
         return true;
        }
       } else {
        qDebug() << "hnode_strm_gui: <ConfigParser> <NET> ERROR: Invalid (uplink) count of NET|IN params!";
        return true;
       }
       qDebug() << "hnode_strm_gui:" << conf->ipAddr << conf->commPort << conf->strmPort << conf->cmodPort;
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
         qDebug() << "hnode_strm_gui: <ConfigParser> <NET> ERROR: Invalid (downlink) hostname/IP/port settings!";
         return true;
        }
       } else {
        qDebug() << "hnode_strm_gui: <ConfigParser> <NET> ERROR: Invalid (downlink) count of NET|OUT params!";
        return true;
       }
       qDebug() << "hnode_strm_gui:" << conf->svrIpAddr << conf->svrCommPort << conf->svrStrmPort << conf->svrCmodPort;
      } else {
       qDebug() << "hnode_strm_gui: <ConfigParser> <NET> ERROR: Invalid hostname/IP(v4) address!";
       return true;
      }
     }
    } else {
     qDebug() << "hnode_strm_gui: <ConfigParser> <NET> ERROR: No parameters in section!";
     return true;
    }

    // BUF section
    if (bufSection.size()>0) {
     for (const auto& sect:bufSection) {
      opts=sect.split("=");
      if (opts[0].trimmed()=="PAST") {
       conf->tcpBufSize=opts[1].toInt();
       if (!(conf->tcpBufSize>=5 && conf->tcpBufSize<=20)) {
        qDebug() << "hnode_strm_gui: <ConfigParser> <BUF> ERROR: PAST not within inclusive (5,20) seconds range!";
        return true;
       }
      } else {
       qDebug() << "hnode_strm_gui: <ConfigParser> <BUF> ERROR: Invalid section command!";
       return true;
      }
     }
    } else {
     qDebug() << "hnode_strm_gui: <ConfigParser> <BUF> ERROR: No parameters in section!";
     return true;
    }

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
         qDebug() << "hnode_strm_gui: <ConfigParser> <PLT> ERROR: Invalid RGBcolor parameter!";
	 return true;
        } else { // Add to the list of recognized colors.
	 conf->rgbPalette.append(QColor(colR,colG,colB,colA));
        }
       } else {
        qDebug() << "hnode_strm_gui: <ConfigParser> <PLT> ERROR: Invalid count of RGBcolor parameters!";
	return true;
       }
      } else {
       qDebug() << "hnode_strm_gui: <ConfigParser> <PLT> ERROR: Invalid section command!";
       return true;
      }
     }
    } else {
     qDebug() << "hnode_strm_gui: <ConfigParser> <PLT> ERROR: No parameters in section!";
     return true;
    }

    // EVT - Event Space and Hierarchy
    int evtNo,evtColIdx; QString evtName;
    if (evtSection.size()>0) {
     for (const auto& sect:evtSection) {
      opts=sect.split("=");
      if (opts[0].trimmed()=="ADD") {
       opts2=opts[1].split(",");
       evtNo=opts2[0].toInt(); evtName=opts2[1].trimmed(); evtColIdx=opts2[2].toInt();
       if (opts2.size()==3) {
        if (!(evtNo>0 && evtNo<256) || (evtName.size()>100) || // max 100 chars for name should be more than enough?
            !(evtColIdx>=0 && evtColIdx<conf->rgbPalette.size())) { // should be within range of present color entries
         qDebug() << "hnode_strm_gui: <ConfigParser> <EVT> ERROR: Invalid Event parameter!";
         return true;
        } else { // Add to the list of pre-recognized events.
	 conf->events.append(Event(evtNo,evtName,evtColIdx));
        }
       } else {
        qDebug() << "hnode_strm_gui: <ConfigParser> <EVT> ERROR: Invalid count of STIM parameters!";
        return true;
       }
      } else {
       qDebug() << "hnode_strm_gui: <ConfigParser> <EVT> ERROR: Invalid section command!";
       return true;
      }
     }
    } else {
     qDebug() << "hnode_strm_gui: <ConfigParser> <EVT> ERROR: No parameters in section!";
     return true;
    }
/*
    // CHN section
    if (chnSection.size()>0) {
     for (const auto& sect:chnSection) {
      opts=chnSection[i].split("=");
      if (opts[0].trimmed()=="SETPARAM") {
       opts2=opts[1].split(",");
       if (opts2.size()==4) { ;
        //dummyChnInfo.physChn=(unsigned)(opts2[0].toInt());     // Physical channel
        //dummyChnInfo.chnName=opts2[1].trimmed();   // Channel name
        //dummyChnInfo.topoTheta=opts2[2].toFloat(); // TopoThetaPhi - Theta
        //dummyChnInfo.topoPhi=opts2[3].toFloat();   // TopoThetaPhi - Phi
        //if ((!(dummyChnInfo.physChn>0  && dummyChnInfo.physChn<=66)) || // Channel# - max channels is assumed as 66
        //    (!(dummyChnInfo.chnName.size()>0 && dummyChnInfo.chnName.size()<=3)) || // Channel name must be 1 to 3 chars..
        //    (!(dummyChnInfo.topoTheta>=0. && dummyChnInfo.topoTheta<360.)) ||
        //    (!(dummyChnInfo.topoPhi>=0. && dummyChnInfo.topoPhi<360.))) {
        // qDebug() << "hnode_strm_gui: <ConfigParser> <CHN> ERROR: Invalid APPEND parameter!";
        // return true;
        //} else { // Set and append new channel..
        // chnTopo->append(dummyChnInfo); // add channel to info table
        //}
       }
      } else if (opts[0].trimmed()=="AVGINTERVAL") {
       opts2=opts[1].split(",");
       if (opts2.size()==4) {
        conf->rejBwd=opts2[0].toFloat(); conf->avgBwd=opts2[1].toFloat();
        conf->avgFwd=opts2[2].toFloat(); conf->rejFwd=opts2[3].toFloat();
        if ((!(conf->rejBwd>=-1000 && conf->rejBwd<=0)) || (!(conf->avgBwd>=-1000 && conf->avgBwd<=0)) ||
            (!(conf->avgFwd>=0 && conf->avgFwd<=1000)) || (!(conf->rejFwd>=0 && conf->rejFwd<=1000)) ||
            (conf->rejBwd>conf->avgBwd) || (conf->avgBwd>conf->avgFwd) || (conf->avgFwd>conf->rejFwd)) {
         qDebug() << "hnode_strm_gui: <ConfigParser> <CHN> ERROR: Invalid INTERVAL parameter!";
         return true;
        }
       } else {
        qDebug() << "hnode_strm_gui: <ConfigParser> <CHN> ERROR: Invalid count of INTERVAL parameters!";
        return true;
       }
      } else {
       qDebug() << "hnode_strm_gui: <ConfigParser> <CHN> ERROR: Invalid section command!";
       return true;
      }
     }
    } else {
     qDebug() << "hnode_strm_gui: <ConfigParser> <CHN> ERROR: No parameters in section!";
     return true;
    }

    // MOD section
    if (modSection.size()>0) {
     for (const auto& sect:modSection) {
      opts=sect.split("=");
      if (opts[0].trimmed()=="GIZMO") {
       opts2=opts[1].split(",");
       if (opts2.size()==1) {
	       ;
        //if (opts2[0].size()) headModel->loadGizmo_OgzFile(opts2[0].trimmed());
        //else {
        // qDebug() << "hnode_strm_gui: <ConfigParser> <MOD> <GIZMO> ERROR: filename error!";
        // return true;
	//}
       } else {
        qDebug() << "hnode_strm_gui: <ConfigParser> <MOD> <GIZMO> ERROR: while parsing parameters!";
        return true;
       }
      } else if (opts[0].trimmed()=="SCALP" || opts[0].trimmed()=="SKULL" || opts[0].trimmed()=="BRAIN") {
       opts2=opts[1].split(",");
       if (opts2.size()==2) {
	       ;
	//int headNo=opts2[0].toInt()-1; 
        //if (!(headNo>=0 && headNo<headModel->getHeadCount())) {
        // qDebug() << "hnode_strm_gui: <ConfigParser> <MOD> <HEAD> ERROR: head# out of bounds!";
        // return true;
        //} else {
        // if (opts[0].trimmed()=="SCALP") {
        //  headModel->loadScalp_ObjFile(opts2[1].trimmed(),headNo);
	// } else if (opts[0].trimmed()=="SKULL") {
        //  headModel->loadSkull_ObjFile(opts2[1].trimmed(),headNo);
	// } else if (opts[0].trimmed()=="BRAIN") {
        //  headModel->loadBrain_ObjFile(opts2[1].trimmed(),headNo);
	// } else {
        //  qDebug() << "hnode_strm_gui: <ConfigParser> <MOD> <SCALP|SKULL|BRAIN> ERROR: while parsing parameters!";
        //  return true;
        // }
	 //else if (opts[0].trimmed()=="SKULL") headModel->loadSkull_ObjFile(opts2[0].trimmed(),headNo);
         //   else {
         //    qDebug() << "hnode_strm_gui: <ConfigParser> <MOD> <SKULL> ERROR: while parsing parameters!";
         //    return true;
         //   } else if (opts[0].trimmed()=="BRAIN") headModel->loadBrain_ObjFile(opts2[0].trimmed(),headNo);
         // else {
         //  qDebug() << "hnode_strm_gui: <ConfigParser> <MOD> <BRAIN> ERROR: while parsing parameters!";
         //  return true;
         // }
         //} else {
         // qDebug() << "hnode_strm_gui: <ConfigParser> <MOD> ERROR: No parameters in section!";
         // return true;
         //}
       }
      }
     }    
    }
*/
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
         qDebug() << "hnode_strm_gui: <ConfigParser> <GUI> <CTRL> ERROR: Window size settings not in appropriate range!";
         return true;
        }
       } else {
        qDebug() << "hnode_strm_gui: <ConfigParser> <GUI> ERROR: Invalid count of parameters!";
        return true;
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
         qDebug() << "hnode_strm_gui: <ConfigParser> <GUI> <STRM> ERROR: Window size settings not in appropriate range!";
	 return true;
        }
       } else {
        qDebug() << "hnode_strm_gui: <ConfigParser> <GUI> ERROR: Invalid count of parameters!";
	return true;
       }
      //} else if (opts[0].trimmed()=="HEAD") { // Head frame within STRM window
      // opts2=opts[1].split(",");
      // if (opts2.size()==4) {
      //  conf->guiHeadX=opts2[0].toInt(); conf->guiHeadY=opts2[1].toInt();
      //  conf->guiHeadW=opts2[2].toInt(); conf->guiHeadH=opts2[3].toInt();
      //  if ((!(conf->guiHeadX >= -2000 && conf->guiHeadX <= 2000)) ||
      //      (!(conf->guiHeadY >= -2000 && conf->guiHeadY <= 2000)) ||
      //      (!(conf->guiHeadW >=   400 && conf->guiHeadW <= 2000)) ||
      //      (!(conf->guiHeadH >=   300 && conf->guiHeadH <= 2000))) {
      //   qDebug() << "hnode_strm_gui: <ConfigParser> <GUI> <HEAD> ERROR: Window size settings not in appropriate range!";
      //   return true;
      //  }
      // } else {
      //  qDebug() << "hnode_strm_gui: <ConfigParser> <GUI> ERROR: Invalid count of parameters!";
      //  return true;
      // }
      }
     }
    } else {
     qDebug() << "hnode_strm_gui: <ConfigParser> <GUI> ERROR: No parameters in section!";
     return true;
    }

   } // File open
   return false;
  }

 private:
  QString cfgPath,cfgLine; QFile cfgFile;
};

#endif
