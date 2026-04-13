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

#include <QApplication>
#include <QtGlobal>
#include <QFile>
#include <QDir>
#include "../common/globals.h"
#include "../common/messagehandler.h"
#include "confparam.h"
#include "configparser.h"
#include "statusclient.h"

//const QString CFGPATH="/etc/octopus/hypereeg.conf";
const QString CFGPATH=QDir::homePath()+"/.octopus-reel/hypereeg.conf";

bool conf_init_pre(ConfParam *conf) {
 QString cfgPath=CFGPATH;
 if (QFile::exists(cfgPath)) {
  ConfigParser cfp(cfgPath);
  if (!cfp.parse(conf)) {
   return false;
  } else {
   qCritical() << "The config file" << cfgPath << "is corrupt!";
   return true;
  }
 } else {
  qCritical() << "The config file" << cfgPath << "does not exist!";
  return true;
 }
}

void conf_info(ConfParam *conf) {
 qInfo() << "===============================================================";
 qInfo() << "                  NODE-GUI-STATUS CONFIG SUMMARY";
 qInfo() << "===============================================================";
 qInfo() << "<GUI>  Coords: (X,Y,W,H):"
         << conf->guiX << conf->guiY << conf->guiW << conf->guiH;
 qInfo() << "<Poll> Refresh(ms):" << conf->statusRefreshMs;
 qInfo() << "<Poll> Timeout(ms):" << conf->statusReplyTimeoutMs;
 qInfo() << "---------------------------------------------------------------";
 qInfo() << "Monitored nodes:";
 for (const auto& n : conf->nodes) {
  qInfo() << " -" << n.name << n.ipAddr << n.commPort;
 }
 qInfo() << "===============================================================";
}

int main(int argc,char* argv[]) {
 QApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
 QApplication app(argc,argv);

 qInstallMessageHandler(qtMessageHandler);

 ConfParam conf;

 qInfo() << "===============================================================";
 qInfo() << "              NODE-GUI-STATUS INITIALIZATION";
 qInfo() << "===============================================================";
 omp_diag();

 if (conf_init_pre(&conf)) {
  qCritical() << "<FatalError> Failed to initialize node-gui-status.";
  return 1;
 }

 StatusClient statusClient(nullptr,&conf);
 if (statusClient.start()) {
  qCritical() << "<FatalError> Failed to start node-gui-status.";
  return 1;
 }

 conf_info(&conf);

 qInfo() << "===============================================================";
 qInfo() << "                 QTIMER BASED LOOP STARTED";
 qInfo() << "===============================================================";

 return app.exec();
}
