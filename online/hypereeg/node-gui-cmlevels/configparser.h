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
const QString NODE_TEXT="CMODE";

class ConfigParser {
 public:
  ConfigParser(QString cp) { cfgPath=cp; cfgFile.setFileName(cfgPath); }

  bool parse(ConfParam *conf) { QString commResponse; QStringList sList,sList2;
   QTextStream cfgStream; QStringList cfgLines,opts,opts2,netSection,chnSection;
   QStringList bufSection,pltSection,evtSection,guiSection,headSection;

   if (!cfgFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qWarning() << "node-gui-cmlevels: <ConfigParser> ERROR: Cannot load" << cfgPath;
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
     qWarning() << "node-gui-cmlevels: <ConfigParser> ERROR: NODE section does not exist in config file!";
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
     else if (opts[0].trimmed()=="GUI") guiSection.append(opts[1]);
     else {
      qWarning() << "node-gui-cmlevels: <ConfigParser> ERROR: Unknown section in config file!";
      return true;
     }
    }

    // --- Extract variables from separate sections ---

    // NET section
    if (netSection.size()>0) {
     for (const auto& sect:netSection) {
      opts=sect.split("=");

      if (opts[0].trimmed()=="COMP") {
       opts2=opts[1].split(","); // IP, command port and stream port
       if (opts2.size()==2) {
        QHostInfo compPPHostInfo=QHostInfo::fromName(opts2[0].trimmed());
        conf->compPPIpAddr=compPPHostInfo.addresses().first().toString();
        conf->compPPCommPort=opts2[1].toInt();
        if ((!(conf->compPPCommPort >= 65000 && conf->compPPCommPort < 65999))) { // Simple port validation
         qWarning() << "node-gui-cmlevels: <ConfigParser> <COMP> ERROR: Invalid (serving) hostname/IP/port settings!";
         return true;
        }
       } else {
        qWarning() << "node-gui-cmlevels: <ConfigParser> <COMP> ERROR: Invalid (serving) count of STRM|IN params!";
        return true;
       }
       qInfo() << "node-gui-cmlevels:" << conf->compPPIpAddr << conf->compPPCommPort;
      } else if (opts[0].trimmed()=="CMODE") {
       opts2=opts[1].split(",");
       if (opts2.size()==2) {
        conf->cmCommPort=opts2[1].toInt();
        if (!(conf->cmCommPort >= 65000 && conf->cmCommPort < 65999)) {
         qWarning() << "node-gui-cmlevels: <ConfigParser> <COMM> ERROR: Invalid local command port!";
         return true;
        }
       } else {
        qWarning() << "node-gui-cmlevels: <ConfigParser> <COMM> ERROR: Invalid parameter count!";
        return true;
       }
       qInfo() << "node-gui-cmlevels:" << conf->compPPIpAddr << conf->compPPCommPort;
      } else {
       qWarning() << "node-gui-cmlevels: <ConfigParser> <COMP> ERROR: Invalid hostname/IP(v4) address!";
       return true;
      }
     }
    } else {
     qWarning() << "node-gui-cmlevels: <ConfigParser> <COMP> ERROR: No parameters in section!";
     return true;
    }

    // GUI section
    if (guiSection.size()>0) {
     for (const auto& sect:guiSection) {
      opts=sect.split("=");
      if (opts[0].trimmed()=="MAIN") {
       opts2=opts[1].split(",");
       if (opts2.size()==4) {
        conf->guiX=opts2[0].toInt(); conf->guiY=opts2[1].toInt();
        conf->guiW=opts2[2].toInt(); conf->guiH=opts2[3].toInt();
        if ((!(conf->guiX >= -4000 && conf->guiX <= 4000)) ||
            (!(conf->guiY >= -3000 && conf->guiY <= 3000)) ||
            (!(conf->guiW >=   400 && conf->guiW <= 4000)) ||
            (!(conf->guiH >=    60 && conf->guiH <= 2800))) {
         qWarning() << "node-gui-cmlevels: <ConfigParser> <GUI> <CTRL> ERROR: Window size settings not in appropriate range!";
         return true;
        }
       } else {
        qWarning() << "node-gui-cmlevels: <ConfigParser> <GUI> ERROR: Invalid count of parameters!";
        return true;
       }
      }
     }
    } else {
     qWarning() << "node-gui-cmlevels: <ConfigParser> <GUI> ERROR: No parameters in section!";
     return true;
    }

   } // File open
   return false;
  }

 private:
  QString cfgPath,cfgLine; QFile cfgFile;
};
