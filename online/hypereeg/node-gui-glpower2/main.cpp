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

/* This is the HyperEEG "OpenGL Head GUI" Node for colorized parameter mapping.
 * Its main purpose is to provide a 3D graphic view for the current
 * state of a precomputed parameter.
 * It receives current set of electrode noise levels
 * by command polling over node-acq-pp, with a timer (e.g. each second).
 */

#include <QApplication>
#include <QSurfaceFormat>
#include <QtGlobal>
#include <QDateTime>
#include <QString>
#include <QFile>
#include <QDir>
#include <cstdio>
#include <sys/stat.h>
#include "../common/globals.h"
#include "../common/messagehandler.h"
#include "confparam.h"
#include "configparser.h"
#include "glclient.h"

//const QString CFGPATH="/etc/octopus/hypereeg.conf";
const QString CFGPATH=QDir::homePath()+"/.octopus-reel/hypereeg.conf";

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
 qInfo("Per-amp All Channel (with Meta)#: %d (%d+%d+%d)",conf->chnCount,conf->refChnCount,conf->bipChnCount,conf->metaChnCount);
 qInfo() << "Per-amp Total Channel# (with Trig and Offset):" << conf->totalChnCount;
 qInfo() << "Total Channel# from all amps:" << conf->totalCount;
 qInfo() << "===============================================================";
 qInfo() << "                      NETWORKING SUMMARY";
 qInfo() << "===============================================================";
 qInfo() << "<ServerIP> is" << conf->compPPIpAddr;
 qInfo() << "<Comm> Connected at port (comm):" << conf->compPPCommPort;
 qInfo() << "<Comm> Listening for commands on port(comm):" << conf->glCommPort;
 qInfo() << "===============================================================";
 qInfo() << "                           GUI COORDS";
 qInfo() << "===============================================================";
 qInfo() << "<GUI> Coords: (X,Y,W,H):" << conf->guiX << conf->guiY << conf->guiW << conf->guiH;
}

int main(int argc,char* argv[]) {
 QApplication::setAttribute(Qt::AA_DisableHighDpiScaling);

 ConfParam conf; QSurfaceFormat fmt;
 fmt.setRenderableType(QSurfaceFormat::OpenGL);
 fmt.setProfile(QSurfaceFormat::NoProfile);  // important for GL 2.1
 fmt.setVersion(2,1);
 fmt.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
 fmt.setSwapInterval(0);
 QSurfaceFormat::setDefaultFormat(fmt);

 QApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
 //QApplication::setAttribute(Qt::AA_UseOpenGLES);
 //QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

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
  qCritical("node-gui-glhead: <FatalError> Failed to initialize Octopus-ReEL GLHead computation node.");
  return 1;
 }

 GLClient glClient(nullptr,&conf);
 if (glClient.start()) {
  qCritical("node-gui-glhead: <FatalError> Failed to initialize Octopus-ReEL GLHead computation node.");
  return 1;
 }

 conf_info(&conf);

 qInfo() << "===============================================================";
 qInfo() << "                 QTIMER BASED LOOP STARTED";
 qInfo() << "===============================================================";

 return app.exec();
}
