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
   QStringList cfgLines,opts,opts2,netSection,bufSection,guiSection;
   if (!cfgFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qDebug() << "octopus_hcm_client: <ConfigParser> ERROR: Cannot load" << cfgPath;
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
     else if (opts[0].trimmed()=="GUI") guiSection.append(opts[1]);
     else {
      qDebug() << "octopus_hcm_client: <ConfigParser> ERROR: Unknown section in config file!";
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
       if (opts2.size()==4) {
        QHostInfo acqHostInfo=QHostInfo::fromName(opts2[0].trimmed());
        conf->ipAddr=acqHostInfo.addresses().first().toString();
        conf->commPort=opts2[1].toInt(); // [2] is for eegDataPort
        conf->dataPort=opts2[3].toInt();
        if ((!(conf->commPort>=1024 && conf->commPort<=65535)) || // Simple port validation
            (!(conf->dataPort>=1024 && conf->dataPort<=65535))) {
         qDebug() << "octopus_hcm_client: <ConfigParser> <NET> ERROR: Invalid hostname/IP/port settings!";
         return true;
        }
       } else {
        qDebug() << "octopus_hcm_client: <ConfigParser> <NET> ERROR: Invalid count of ACQ params!";
        return true;
       }
       qDebug() << "octopus_hcm_client: <ConfigParser> (ServerIP,CommPort,DataPort):" << conf->ipAddr << "-" << conf->commPort << "-" << conf->dataPort;
      } else {
       qDebug() << "octopus_hcm_client: <ConfigParser> <NET> ERROR: Invalid section command!";
       return true;
      }
     }
    } else {
     qDebug() << "octopus_hcm_client: <ConfigParser> <NET> ERROR: No parameters in section!";
     return true;
    }

    // BUF section
    if (bufSection.size()>0) {
     for (int i=0;i<bufSection.size();i++) {
      opts=bufSection[i].split("=");
      if (opts[0].trimmed()=="PAST") {
       conf->tcpBufSize=opts[1].toInt();
       if (!(conf->tcpBufSize >= 1000 && conf->tcpBufSize <= 20000)) {
        qDebug() << "octopus_hcm_client: <ConfigParser> <BUF> ERROR: PAST not within inclusive (1000,20000) range!";
        return true;
       } else {
        conf->tcpBuffer.resize(conf->tcpBufSize);
       }
      } else {
       qDebug() << "octopus_hcm_client: <ConfigParser> <BUF> ERROR: Invalid section command!";
       return true;
      }
     }
    } else {
     qDebug() << "octopus_hcm_client: <ConfigParser> <BUF> ERROR: No parameters in section!";
     return true;
    }

    // GUI section
    if (guiSection.size()>0) {
     for (int i=0;i<guiSection.size();i++) {
      opts=guiSection[i].split("=");
      if (opts[0].trimmed()=="CMODE") {
       opts2=opts[1].split(",");
       if (opts2.size()==3) {
        conf->guiX=opts2[0].toInt(); conf->guiY=opts2[1].toInt();
        conf->guiCellSize=opts2[2].toInt();
        if ((!(conf->guiX >= -4000 && conf->guiX <= 4000)) ||
            (!(conf->guiY >= -3000 && conf->guiY <= 3000)) ||
            (!(conf->guiCellSize >= 40 && conf->guiCellSize <= 80))) {
         qDebug() << "octopus_hcm_client: <ConfigParser> <GUI> <CMode> ERROR: Window size settings not in appropriate range!";
         return true;
        }
       } else {
        qDebug() << "octopus_hcm_client: <ConfigParser> <GUI> ERROR: Invalid count of parameters!";
        return true;
       }
      }
     }
    } else {
     qDebug() << "octopus_hcm_client: <ConfigParser> <GUI> ERROR: No parameters in section!";
     return true;
    }

   } // File open
   return false;
  }

 private:
  QString cfgPath,cfgLine; QFile cfgFile;
};

#endif
