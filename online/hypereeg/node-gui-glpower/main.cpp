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

/* This is the HyperEEG "Power Levels GUI" Node.
 * Its main purpose is to provide a simple graphic view for the current
 * frequency band power levels of GFP and selected individual electrodes
 * on each amplifier. Similar to CMlevels client, current data vector is
 * manually fetched regularly (e.g. 2/sec) from node-comp-pp, rather than streaming
 * continuously.
 */

#include <QApplication>
#include <QSurfaceFormat>
#include <QtGlobal>
#include <QDateTime>
#include <QString>
#include <QFile>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QScreen>
#include <cstdio>
#include <sys/stat.h>
#include "../common/globals.h"
#include "../common/messagehandler.h"
#include "confparam.h"
#include "configparser.h"
#include "powclient.h"

const QString CFGPATH="/opt/octopus/etc/hypereeg.conf";

static bool apply_cli_overrides(QApplication &app, ConfParam *conf) {
 QCommandLineParser parser;
 parser.setApplicationDescription("Octopus node-gui-glpower");
 parser.addHelpOption();

 QCommandLineOption portOpt(QStringList() << "p" << "port","Override local command/listen port from config file.","port");
 QCommandLineOption xOpt(QStringList() << "x","Override GUI X position.","x");
 QCommandLineOption yOpt(QStringList() << "y","Override GUI Y position.","y");
 QCommandLineOption widthOpt(QStringList() << "width","Override GUI width.","width");
 QCommandLineOption heightOpt(QStringList() << "height","Override GUI height.","height");
 QCommandLineOption screenOpt(QStringList() << "screen","Place GUI on screen index.","screen");

 parser.addOption(portOpt);
 parser.addOption(screenOpt);
 parser.addOption(xOpt);
 parser.addOption(yOpt);
 parser.addOption(widthOpt);
 parser.addOption(heightOpt);
 parser.process(app);

 auto parseIntOpt=[&](const QCommandLineOption &opt,const QString &name,int minVal,int maxVal,int &dst)->bool {
  if (!parser.isSet(opt)) return false;
  bool ok=false; int v=parser.value(opt).toInt(&ok);
  if (!ok || v<minVal || v>maxVal) {
   qCritical() << "node-gui-glpower: <CLI> Invalid" << name << "value:" << parser.value(opt)
               << QString("(expected %1..%2)").arg(minVal).arg(maxVal);
   return true;
  }
  dst=v; qInfo() << "node-gui-glpower: <CLI> Overriding" << name << "with:" << dst;
  return false;
 };

 if (parser.isSet(portOpt)) {
  bool ok=false; int p=parser.value(portOpt).toInt(&ok);

  if (!ok || p<65000 || p>=65999) {
   qCritical() << "node-gui-glpower: <CLI> Invalid --port value:" << parser.value(portOpt) << "(expected 65000..65998)";
   return true;
  }

  conf->powCommPort=quint32(p);
  qInfo() << "node-gui-glpower: <CLI> Overriding local command port with:" << conf->powCommPort;
 }

 if (parser.isSet(screenOpt)) {
  bool ok=false; int screenIdx=parser.value(screenOpt).toInt(&ok);
  const QList<QScreen*> screens=app.screens();
  if (!ok || screenIdx<0 || screenIdx>=screens.size()) {
   qCritical() << "node-gui-glpower: <CLI> Invalid --screen value:"
               << parser.value(screenOpt)
               << QString("(available screens: 0..%1)").arg(screens.size()-1);
   return true;
  }
  QRect g=screens[screenIdx]->geometry();
  conf->guiX=g.x(); conf->guiY=g.y(); conf->guiW=g.width(); conf->guiH=g.height();
  qInfo() << "node-gui-glpower: <CLI> Using screen" << screenIdx
          << "geometry:"
          << conf->guiX << conf->guiY << conf->guiW << conf->guiH;
 }

 if (parseIntOpt(xOpt,"GUI X",-4000,4000,conf->guiX)) return true;
 if (parseIntOpt(yOpt,"GUI Y",-3000,3000,conf->guiY)) return true;
 if (parseIntOpt(widthOpt,"GUI width",400,4000,conf->guiW)) return true;
 if (parseIntOpt(heightOpt,"GUI height",60,2800,conf->guiH)) return true;


 return false;
}

bool conf_init_pre(ConfParam *conf) { QString cfgPath=CFGPATH;
 if (QFile::exists(cfgPath)) { ConfigParser cfp(cfgPath);
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
 qInfo() << "                    DETAILED CHANNELS INFO";
 qInfo() << "===============================================================";
 qInfo() << "---------------------------";
 qInfo() << "EEG (referential) channels:";
 qInfo() << "---------------------------"; if (conf->refChnCount==0) qInfo() << "None.";
 for (const auto& c:conf->refChns) {
  qInfo("%d -> %s - [%d,%d]",c.physChn,qUtf8Printable(c.chnName),c.topoX,c.topoY);
 }
 qInfo() << "--------------------------------";
 qInfo() << "BiPolar (differential) channels:";
 qInfo() << "--------------------------------"; if (conf->bipChnCount==0) qInfo() << "None.";
 for (const auto& c:conf->bipChns) {
  qInfo("%d -> %s - [%d,%d]",c.physChn,qUtf8Printable(c.chnName),c.topoX,c.topoY);
 }
 qInfo() << "------------------------------";
 qInfo() << "Meta (only-computed) channels:";
 qInfo() << "------------------------------"; if (conf->metaChnCount==0) qInfo() << "None.";
 for (const auto& c:conf->metaChns) {
  qInfo("%d -> %s - [%d,%d]",c.physChn,qUtf8Printable(c.chnName),c.topoX,c.topoY);
 }
 qInfo() << "===============================================================";
 qInfo() << "                ACQUISITION PARAMETERS SUMMARY";
 qInfo() << "              (some are as relayed from node-acq)";
 qInfo() << "===============================================================";
 qInfo() << "# of amplifier(s):" << conf->ampCount;
 qInfo("Per-amp Physical Channel#: %d (%d+%d)",conf->physChnCount,conf->refChnCount,conf->bipChnCount);
 qInfo("Per-amp All Channel (with Meta)#: %d (%d+%d+%d)",conf->physChnCount,conf->refChnCount,conf->bipChnCount,conf->metaChnCount);
 qInfo() << "Per-amp Total Channel# (with Trig and Offset):" << conf->totalChnCount;
 qInfo() << "===============================================================";
 qInfo() << "                      NETWORKING SUMMARY";
 qInfo() << "===============================================================";
 qInfo() << "<ServerIP> is" << conf->compPPIpAddr;
 qInfo() << "<Comm> Connected at port (comm):" << conf->compPPCommPort;
 qInfo() << "<Comm> Listening for commands on port(comm):" << conf->powCommPort;
 qInfo() << "===============================================================";
 qInfo() << "                           GUI COORDS";
 qInfo() << "===============================================================";
 qInfo() << "<GUI> Coords: (X,Y,W,H):" << conf->guiX << conf->guiY << conf->guiW << conf->guiH;
}

int main(int argc,char* argv[]) {
 QApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
 ConfParam conf;

 qInstallMessageHandler(qtMessageHandler);
// setvbuf(stdout,nullptr,_IOLBF,0); // Avoid buffering
// setvbuf(stderr,nullptr,_IONBF,0);

//#ifdef __linux__
// // 1) Avoid swapping/pagefault stalls
// lock_memory_or_warn(); // needs CAP_IPC_LOCK (or high memlock ulimit)
//
// // 2) Help scheduler a bit
// set_process_nice(-10); // needs CAP_SYS_NICE
//#endif

 QApplication app(argc,argv);

// umask(0002);

 qInfo() << "===============================================================";
 qInfo() << "                  OPENMP INITIALIZATION STATUS";
 qInfo() << "===============================================================";
 omp_diag();

 if (conf_init_pre(&conf)) {
  qCritical("node-gui-glpower: <FatalError> Failed to initialize Octopus-ReEL Power computation node.");
  return 1;
 }

 if (apply_cli_overrides(app,&conf)) {
  qCritical("node-gui-glpower: <FatalError> Invalid command-line options.");
  return 1;
 }

 PowClient powClient(nullptr,&conf);
 if (powClient.start()) {
  qCritical("node-gui-glpower: <FatalError> Failed to initialize Octopus-ReEL Power computation node.");
  return 1;
 }

 conf_info(&conf);

 qInfo() << "===============================================================";
 qInfo() << "                 QTIMER BASED LOOP STARTED";
 qInfo() << "===============================================================";

 return app.exec();
}
