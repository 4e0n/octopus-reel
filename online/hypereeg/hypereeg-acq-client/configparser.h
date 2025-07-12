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

#include <QFile>
#include <QColor>
#include <QHostInfo>
#include "confparam.h"

const int MAX_LINE_SIZE=160; // chars

class ConfigParser {
 public:
  ConfigParser(QString cp) {
   cfgPath=cp; cfgFile.setFileName(cfgPath);
  }

  bool parse(ConfParam *conf) {
   QTextStream cfgStream;
   QStringList cfgLines,opts,opts2,netSection,bufSection,pltSection,evtSection,chnSection,guiSection,modSection;
   if (!cfgFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qDebug() << "octopus_hacq_client: <ConfigParser> ERROR: Cannot load" << cfgPath;
    return true;
   } else {
    cfgStream.setDevice(&cfgFile);

    while (!cfgStream.atEnd()) { // Populate cfgLines with valid lines
     cfgLine=cfgStream.readLine(MAX_LINE_SIZE); // Should not start with #, should contain "|"
     if (!(cfgLine.at(0)=='#') && cfgLine.contains('|')) cfgLines.append(cfgLine);
    }
    cfgFile.close();

    // Separate AMP, NET, CHN, GUI parameter lines
    for (int i=0;i<cfgLines.size();i++) {
     opts=cfgLines[i].split("#"); opts=opts[0].split("|"); // Get rid off any following comment.
          if (opts[0].trimmed()=="NET") netSection.append(opts[1]);
     else if (opts[0].trimmed()=="BUF") bufSection.append(opts[1]);
     else if (opts[0].trimmed()=="PLT") pltSection.append(opts[1]);
     else if (opts[0].trimmed()=="EVT") evtSection.append(opts[1]);
     else if (opts[0].trimmed()=="CHN") chnSection.append(opts[1]); // AVG merged in CHN
     else if (opts[0].trimmed()=="GUI") guiSection.append(opts[1]);
     else if (opts[0].trimmed()=="MOD") modSection.append(opts[1]);
     //else if (opts[0].trimmed()=="DIG") digSection.append(opts[1]);
     else {
      qDebug() << "octopus_hacq_client: <ConfigParser> ERROR: Unknown section in config file!";
      return true;
     }
    }

    // --- Extract variables from separate sections ---

    // NET section
    if (netSection.size()>0) {
     for (int i=0;i<netSection.size();i++) {
      opts=netSection[i].split("=");
      if (opts[0].trimmed()=="ACQ") {
       opts2=opts[1].split(","); // IP, ConfigPort and DataPort are separated by ","
       if (opts2.size()==3) {
        QHostInfo acqHostInfo=QHostInfo::fromName(opts2[0].trimmed());
        conf->ipAddr=acqHostInfo.addresses().first().toString();
        conf->commPort=opts2[1].toInt();
        conf->dataPort=opts2[2].toInt();
        if ((!(conf->commPort >= 1024 && conf->commPort <= 65535)) || // Simple port validation
            (!(conf->dataPort >= 1024 && conf->dataPort <= 65535))) {
         qDebug() << "octopus_hacq_client: <ConfigParser> <NET> ERROR: Invalid hostname/IP/port settings!";
         return true;
        }
       } else {
        qDebug() << "octopus_hacq_client: <ConfigParser> <NET> ERROR: Invalid count of ACQ params!";
        return true;
       }
       qDebug() << "octopus_hacq_client:" << conf->ipAddr << conf->commPort << conf->dataPort;
      } else {
       qDebug() << "octopus_hacq_client: <ConfigParser> <NET> ERROR: Invalid section command!";
       return true;
      }
     }
    } else {
     qDebug() << "octopus_hacq_client: <ConfigParser> <NET> ERROR: No parameters in section!";
     return true;
    }

    // BUF section
    if (bufSection.size()>0) {
     for (int i=0;i<bufSection.size();i++) {
      opts=bufSection[i].split("=");
      if (opts[0].trimmed()=="PAST") {
       conf->tcpBufSize=opts[1].toInt();
       if (!(conf->tcpBufSize >= 1000 && conf->tcpBufSize <= 20000)) {
        qDebug() << "octopus_hacq_client: <ConfigParser> <BUF> ERROR: PAST not within inclusive (1000,20000) range!";
        return true;
       } else {
        conf->tcpBuffer.resize(conf->tcpBufSize);
       }
      } else {
       qDebug() << "octopus_hacq_client: <ConfigParser> <BUF> ERROR: Invalid section command!";
       return true;
      }
     }
    } else {
     qDebug() << "octopus_hacq_client: <ConfigParser> <BUF> ERROR: No parameters in section!";
     return true;
    }

    // PLT - Color palette entries
    int colR,colG,colB,colA; 
    if (pltSection.size()>0) {
     for (int i=0;i<pltSection.size();i++) {
      opts=pltSection[i].split("=");
      if (opts[0].trimmed()=="ADD") {
       opts2=opts[1].split(",");
       if (opts2.size()==4) {
        colR=opts2[0].toInt(); colG=opts2[1].toInt(); colB=opts2[2].toInt(); colA=opts2[3].toInt();
        if (!(colR>=0 && colR<256) || !(colG>=0 && colG<256) || !(colB>=0 && colB<256) || !(colA>=0 && colA<256)) {
         qDebug() << "octopus_acq_client: <ConfigParser> <PLT> ERROR: Invalid RGBcolor parameter!";
	 return true;
        } else { // Add to the list of recognized colors.
	 conf->rgbPalette.append(QColor(colR,colG,colB,colA));
        }
       } else {
        qDebug() << "octopus_acq_client: <ConfigParser> <PLT> ERROR: Invalid count of RGBcolor parameters!";
	return true;
       }
      } else {
       qDebug() << "octopus_hacq_client: <ConfigParser> <PLT> ERROR: Invalid section command!";
       return true;
      }
     }
    } else {
     qDebug() << "octopus_acq_client: <ConfigParser> <PLT> ERROR: No parameters in section!";
     return true;
    }
    //for (int i=0;i<conf->rgbPalette.size();i++) qDebug() << conf->rgbPalette[i];

    // EVT - Event Space and Hierarchy
    int evtNo,evtColIdx; QString evtName;
    if (evtSection.size()>0) {
     for (int i=0;i<evtSection.size();i++) {
      opts=evtSection[i].split("=");
      if (opts[0].trimmed()=="ADD") {
       opts2=opts[1].split(",");
       evtNo=opts2[0].toInt(); evtName=opts2[1].trimmed(); evtColIdx=opts2[2].toInt();
       if (opts2.size()==3) {
        if (!(evtNo>0 && evtNo<256) || (evtName.size()>100) || // max 100 chars for name should be more than enough?
            !(evtColIdx>=0 && evtColIdx<conf->rgbPalette.size())) { // should be within range of present color entries
         qDebug() << "octopus_acq_client: <ConfigParser> <EVT> ERROR: Invalid Event parameter!";
         return true;
        } else { // Add to the list of pre-recognized events.
	 conf->events.append(Event(evtNo,evtName,evtColIdx));
        }
       } else {
        qDebug() << "octopus_acq_client: <ConfigParser> <EVT> ERROR: Invalid count of STIM parameters!";
        return true;
       }
      } else {
       qDebug() << "octopus_hacq_client: <ConfigParser> <EVT> ERROR: Invalid section command!";
       return true;
      }
     }
    } else {
     qDebug() << "octopus_acq_client: <ConfigParser> <EVT> ERROR: No parameters in section!";
     return true;
    }
/*
    // CHN section
    if (chnSection.size()>0) {
     for (int i=0;i<chnSection.size();i++) {
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
        // qDebug() << "octopus_acq_client: <ConfigParser> <CHN> ERROR: Invalid APPEND parameter!";
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
         qDebug() << "octopus_acq_client: <ConfigParser> <CHN> ERROR: Invalid INTERVAL parameter!";
         return true;
        }
       } else {
        qDebug() << "octopus_acq_client: <ConfigParser> <CHN> ERROR: Invalid count of INTERVAL parameters!";
        return true;
       }
      } else {
       qDebug() << "octopus_hacq_client: <ConfigParser> <CHN> ERROR: Invalid section command!";
       return true;
      }
     }
    } else {
     qDebug() << "octopus_acq_client: <ConfigParser> <CHN> ERROR: No parameters in section!";
     return true;
    }

    // MOD section
    if (modSection.size()>0) {
     for (int i=0;i<modSection.size();i++) {
      opts=modSection[i].split("=");
      if (opts[0].trimmed()=="GIZMO") {
       opts2=opts[1].split(",");
       if (opts2.size()==1) {
	       ;
        //if (opts2[0].size()) headModel->loadGizmo_OgzFile(opts2[0].trimmed());
        //else {
        // qDebug() << "octopus_acq_client: <ConfigParser> <MOD> <GIZMO> ERROR: filename error!";
        // return true;
	//}
       } else {
        qDebug() << "octopus_acq_client: <ConfigParser> <MOD> <GIZMO> ERROR: while parsing parameters!";
        return true;
       }
      } else if (opts[0].trimmed()=="SCALP" || opts[0].trimmed()=="SKULL" || opts[0].trimmed()=="BRAIN") {
       opts2=opts[1].split(",");
       if (opts2.size()==2) {
	       ;
	//int headNo=opts2[0].toInt()-1; 
        //if (!(headNo>=0 && headNo<headModel->getHeadCount())) {
        // qDebug() << "octopus_acq_client: <ConfigParser> <MOD> <HEAD> ERROR: head# out of bounds!";
        // return true;
        //} else {
        // if (opts[0].trimmed()=="SCALP") {
        //  headModel->loadScalp_ObjFile(opts2[1].trimmed(),headNo);
	// } else if (opts[0].trimmed()=="SKULL") {
        //  headModel->loadSkull_ObjFile(opts2[1].trimmed(),headNo);
	// } else if (opts[0].trimmed()=="BRAIN") {
        //  headModel->loadBrain_ObjFile(opts2[1].trimmed(),headNo);
	// } else {
        //  qDebug() << "octopus_acq_client: <ConfigParser> <MOD> <SCALP|SKULL|BRAIN> ERROR: while parsing parameters!";
        //  return true;
        // }
	 //else if (opts[0].trimmed()=="SKULL") headModel->loadSkull_ObjFile(opts2[0].trimmed(),headNo);
         //   else {
         //    qDebug() << "octopus_acq_client: <ConfigParser> <MOD> <SKULL> ERROR: while parsing parameters!";
         //    return true;
         //   } else if (opts[0].trimmed()=="BRAIN") headModel->loadBrain_ObjFile(opts2[0].trimmed(),headNo);
         // else {
         //  qDebug() << "octopus_acq_client: <ConfigParser> <MOD> <BRAIN> ERROR: while parsing parameters!";
         //  return true;
         // }
         //} else {
         // qDebug() << "octopus_acq_client: <ConfigParser> <MOD> ERROR: No parameters in section!";
         // return true;
         //}
       }
      }
     }    
    }
*/
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
         return true;
        }
       } else {
        qDebug() << "octopus_acq_client: <ConfigParser> <GUI> ERROR: Invalid count of parameters!";
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
         qDebug() << "octopus_acq_client: <ConfigParser> <GUI> <STRM> ERROR: Window size settings not in appropriate range!";
	 return true;
        }
       } else {
        qDebug() << "octopus_acq_client: <ConfigParser> <GUI> ERROR: Invalid count of parameters!";
	return true;
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
	 return true;
        }
       } else {
        qDebug() << "octopus_acq_client: <ConfigParser> <GUI> ERROR: Invalid count of parameters!";
	return true;
       }
      }
     }
    } else {
     qDebug() << "octopus_acq_client: <ConfigParser> <GUI> ERROR: No parameters in section!";
     return true;
    }

   } // File open
   return false;
  }

 private:
  QString cfgPath,cfgLine; QFile cfgFile;
};

#endif
