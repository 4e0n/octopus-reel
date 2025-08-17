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
   QStringList cfgLines,opts,opts2,netSection,bufSection;

   if (!cfgFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qDebug() << "hnode_compute_hempow: <ConfigParser> ERROR: Cannot load" << cfgPath;
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
     else {
      qDebug() << "hnode_compute_hempow: <ConfigParser> ERROR: Unknown section in config file!";
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
        if ((!(conf->commPort >= 65000 && conf->commPort < 65500)) || // Simple port validation
            (!(conf->strmPort >= 65000 && conf->strmPort < 65500))) {
         qDebug() << "hnode_compute_hempow: <ConfigParser> <NET> ERROR: Invalid (uplink) hostname/IP/port settings!";
         return true;
        }
       } else {
        qDebug() << "hnode_compute_hempow: <ConfigParser> <NET> ERROR: Invalid (uplink) count of NET|IN params!";
        return true;
       }
       qDebug() << "hnode_compute_hempow:" << conf->ipAddr << conf->commPort << conf->strmPort;
      } else if (opts[0].trimmed()=="OUT") {
       opts2=opts[1].split(","); // IP, command port, stream port and commonmode port are separated by ","
       if (opts2.size()==4) {
        QHostInfo acqHostInfo=QHostInfo::fromName(opts2[0].trimmed());
        conf->svrIpAddr=acqHostInfo.addresses().first().toString();
        conf->svrCommPort=opts2[1].toInt();
        conf->svrStrmPort=opts2[2].toInt();
        if ((!(conf->svrCommPort >= 65000 && conf->svrCommPort < 65500)) || // Simple port validation
            (!(conf->svrStrmPort >= 65000 && conf->svrStrmPort < 65500))) {
         qDebug() << "hnode_compute_hempow: <ConfigParser> <NET> ERROR: Invalid (downlink) hostname/IP/port settings!";
         return true;
        }
       } else {
        qDebug() << "hnode_compute_hempow: <ConfigParser> <NET> ERROR: Invalid (downlink) count of NET|OUT params!";
        return true;
       }
       qDebug() << "hnode_compute_hempow:" << conf->svrIpAddr << conf->svrCommPort << conf->svrStrmPort;
      } else {
       qDebug() << "hnode_compute_hempow: <ConfigParser> <NET> ERROR: Invalid hostname/IP(v4) address!";
       return true;
      }
     }
    } else {
     qDebug() << "hnode_compute_hempow: <ConfigParser> <NET> ERROR: No parameters in section!";
     return true;
    }

    // BUF section
    if (bufSection.size()>0) {
     for (const auto& sect:bufSection) {
      opts=sect.split("=");
      if (opts[0].trimmed()=="PAST") {
       conf->tcpBufSize=opts[1].toInt();
       if (!(conf->tcpBufSize>=5 && conf->tcpBufSize<=20)) {
        qDebug() << "hnode_compute_hempow: <ConfigParser> <BUF> ERROR: PAST not within inclusive (5,20) seconds range!";
        return true;
       }
      } else {
       qDebug() << "hnode_compute_hempow: <ConfigParser> <BUF> ERROR: Invalid section command!";
       return true;
      }
     }
    } else {
     qDebug() << "hnode_compute_hempow: <ConfigParser> <BUF> ERROR: No parameters in section!";
     return true;
    }

   } // File open
   return false;
  }

 private:
  QString cfgPath,cfgLine; QFile cfgFile;
};

#endif
