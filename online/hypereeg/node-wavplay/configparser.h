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
#include "confparam.h"
#include "../common/tcp_commands.h"

const int MAX_LINE_SIZE=160; // chars
const QString NODE_TEXT="WAVPLAY";

class ConfigParser {
 public:
  ConfigParser(QString cp) { cfgPath=cp; cfgFile.setFileName(cfgPath); }

  bool parse(ConfParam *conf) { QString commResponse; QStringList sList,sList2;
   QTextStream cfgStream; QStringList cfgLines,opts,opts2,netSection,chnSection;
   QStringList bufSection;

   if (!cfgFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qCritical() << "<ConfigParser> ERROR: Cannot load" << cfgPath;
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
     qCritical() << "node-wavplay: <ConfigParser> ERROR: NODE section does not exist in config file!";
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
     else {
      qCritical() << "<ConfigParser> ERROR: Unknown section in config file!";
      return true;
     }
    }

    // --- Extract variables from separate sections ---

    // NET section
    if (netSection.size()>0) {
     for (const auto& sect:netSection) {
      opts=sect.split("=");
      if (opts[0].trimmed()=="WAVPLAY") {
       opts2=opts[1].split(","); // IP, command port, stream port and commonmode port are separated by ","
       if (opts2.size()==2) {
        QHostInfo wavPlayHostInfo=QHostInfo::fromName(opts2[0].trimmed());
        conf->wavPlayIpAddr=wavPlayHostInfo.addresses().first().toString();
        conf->wavPlayCommPort=opts2[1].toInt();
        if ((!(conf->wavPlayCommPort >= 65000 && conf->wavPlayCommPort < 65999))) { // Simple port validation
         qCritical() << "<ConfigParser> <NET> ERROR: Invalid (serving) hostname/IP/port settings!";
         return true;
        }
       } else {
        qCritical() << "<ConfigParser> <NET> ERROR: Invalid (serving) count of NET|IN params!";
        return true;
       }
      } else {
       qCritical() << "<ConfigParser> <NET> ERROR: Invalid hostname/IP(v4) address!";
       return true;
      }
     }
    } else {
     qCritical() << "<ConfigParser> <NET> ERROR: No parameters in section!";
     return true;
    }
   } // File open
   return false;
  }

 private:
  QString cfgPath,cfgLine; QFile cfgFile;
};
