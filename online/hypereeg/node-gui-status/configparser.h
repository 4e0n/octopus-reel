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
#include <QTextStream>
#include "confparam.h"

const int MAX_LINE_SIZE=200;
const QString NODE_TEXT="STATUS";

class ConfigParser {
 public:
  ConfigParser(QString cp) { cfgPath=cp; cfgFile.setFileName(cfgPath); }

  bool parse(ConfParam *conf) {
   QTextStream cfgStream; QStringList cfgLines,opts,opts2;
   QStringList guiSection,pollSection,monSection;

   if (!cfgFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qWarning() << "node-gui-status: <ConfigParser> ERROR: Cannot load" << cfgPath;
    return true;
   }

   cfgStream.setDevice(&cfgFile);
   while (!cfgStream.atEnd()) {
    cfgLine=cfgStream.readLine(MAX_LINE_SIZE).trimmed();
    if (!cfgLine.isEmpty() && !cfgLine.startsWith('#') && cfgLine.contains('|'))
     cfgLines.append(cfgLine);
    }
    cfgFile.close();
    int idx1=-1,idx2=cfgLines.size();
    for (int idx=0; idx<cfgLines.size(); idx++) {
     opts=cfgLines[idx].split("|");
     if (opts[0].trimmed()=="NODE" && opts[1].trimmed()==NODE_TEXT) {
      idx1=idx;
      break;
     }
    }

    if (idx1<0) {
     qWarning() << "node-gui-status: <ConfigParser> ERROR: NODE|STATUS section does not exist!";
     return true;
    }

    idx1++;
    for (int idx=idx1; idx<cfgLines.size(); idx++) {
     opts=cfgLines[idx].split("|");
     if (opts[0].trimmed()=="NODE") { idx2=idx; break; }
    }

    for (int idx=idx1; idx<idx2; idx++) {
     opts=cfgLines[idx].split("#"); opts=opts[0].split("|");
     if (opts[0].trimmed()=="GUI") guiSection.append(opts[1]);
     else if (opts[0].trimmed()=="POLL") pollSection.append(opts[1]);
     else if (opts[0].trimmed()=="MON")  monSection.append(opts[1]);
     else {
      qWarning() << "node-gui-status: <ConfigParser> ERROR: Unknown section!";
      return true;
     }
    }

    // GUI
    if (guiSection.isEmpty()) {
     qWarning() << "node-gui-status: <ConfigParser> <GUI> ERROR: No GUI section!";
     return true;
    }

    for (const auto& sect:guiSection) {
     opts=sect.split("=");
     if (opts.size()!=2) continue;

     if (opts[0].trimmed()=="MAIN") {
      opts2=opts[1].split(",");
      if (opts2.size()!=4) {
       qWarning() << "node-gui-status: <ConfigParser> <GUI> ERROR: Invalid parameter count!";
       return true;
      }
      conf->guiX=opts2[0].trimmed().toInt();
      conf->guiY=opts2[1].trimmed().toInt();
      conf->guiW=opts2[2].trimmed().toInt();
      conf->guiH=opts2[3].trimmed().toInt();
     }
    }

    // POLL
    for (const auto& sect : pollSection) {
     opts=sect.split("=");
     if (opts.size()!=2) continue;
     const QString key=opts[0].trimmed();
     const QString val=opts[1].trimmed();
     if (key=="REFRESH") conf->statusRefreshMs=val.toInt();
     else if (key=="TIMEOUT") conf->statusReplyTimeoutMs=val.toInt();
    }
    if (conf->statusRefreshMs<200 || conf->statusRefreshMs>10000) {
     qWarning() << "node-gui-status: <ConfigParser> <POLL> REFRESH out of range!";
     return true;
    }
    if (conf->statusReplyTimeoutMs<100 || conf->statusReplyTimeoutMs>5000) {
     qWarning() << "node-gui-status: <ConfigParser> <POLL> TIMEOUT out of range!";
     return true;
    }

    // MON
    conf->nodes.clear();
    for (const auto& sect:monSection) {
     opts=sect.split("=");
     if (opts.size()!=2) {
      qWarning() << "node-gui-status: <ConfigParser> <MON> ERROR: Invalid syntax!";
      return true;
     }
     if (opts[0].trimmed()!="NODE") continue;
     opts2=opts[1].split(",");
     if (opts2.size()!=3) {
      qWarning() << "node-gui-status: <ConfigParser> <MON> ERROR: Invalid node parameter count!";
      return true;
     }
     MonitoredNode n; n.name=opts2[0].trimmed();
     QHostInfo hi=QHostInfo::fromName(opts2[1].trimmed());
     if (hi.addresses().isEmpty()) {
      qWarning() << "node-gui-status: <ConfigParser> <MON> ERROR: Cannot resolve host"
                 << opts2[1].trimmed();
      return true;
     }
     n.ipAddr=hi.addresses().first().toString();
     n.commPort=opts2[2].trimmed().toUInt();
     if (!(n.commPort>=65000 && n.commPort<65999)) {
      qWarning() << "node-gui-status: <ConfigParser> <MON> ERROR: Invalid node command port!";
      return true;
     }
     conf->nodes.append(n);
    }
    if (conf->nodes.isEmpty()) {
     qWarning() << "node-gui-status: <ConfigParser> <MON> ERROR: No monitored nodes defined!";
     return true;
    }
    return false;
   }

 private:
  QString cfgPath,cfgLine;
  QFile cfgFile;
};
