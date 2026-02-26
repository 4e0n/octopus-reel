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

/* This is the HyperEEG "File recording/storage operations" Node.
 * Its main purpose is to record the EEG+audio data broadcast by
 * the node-acq to disk. Starting and stopping of recording is held
 * by proper commands from node-time.
 */

#include <QCoreApplication>
#include <QSurfaceFormat>
#include <QtGlobal>
#include <QDateTime>
#include <cstdio>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <linux/ioprio.h>
#include "../common/globals.h"
#include "confparam.h"
#include "configparser.h"
#include "stordaemon.h"
#include "../common/messagehandler.h"

const QString CFGPATH="/etc/octopus/hypereeg.conf";

static void setIOPriority() {
 int prio=IOPRIO_PRIO_VALUE(IOPRIO_CLASS_BE,0); // best-effort, highest prio
 syscall(SYS_ioprio_set,IOPRIO_WHO_PROCESS,0,prio);
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
 qInfo() << "Channels Info:";
 qInfo() << "--------------";
 for (auto& chn:conf->chnInfo) qInfo() << chn.physChn << chn.chnName << chn.type;
 qInfo() << "===============================================================";
 qInfo() << "Networking Summary:";
 qInfo() << "-------------------";
 qInfo() << "<ServerIP> is" << conf->acqIpAddr;
 qInfo() << "<Comm> downstreaming on ports (comm,strm):" << conf->acqCommPort << conf->acqStrmPort;
 qInfo() << "<Comm> listening on port(comm)" << conf->storCommPort;
}

int main(int argc,char* argv[]) {
 ConfParam conf;

 setIOPriority();

 qInstallMessageHandler(qtMessageHandler);
 setvbuf(stdout,nullptr,_IOLBF,0); // Avoid buffering
 setvbuf(stderr,nullptr,_IONBF,0);

 QCoreApplication app(argc,argv);

 umask(0002);

 qInfo() << "===============================================================";
 omp_diag();
 qInfo() << "===============================================================";

 if (conf_init_pre(&conf)) {
  qCritical("<FatalError> Failed to initialize Octopus-ReEL EEG data storage daemon node.");
  return 1;
 }

 StorDaemon storDaemon(nullptr,&conf);

 if (storDaemon.start()) {
  qCritical("<FatalError> Failed to start Octopus-ReEL EEG data storage daemon node.");
  return 1;
 }

 conf_info(&conf);

 // Any (e.g. hardware probing related) intermediate checks in the future to come.

 qInfo() << "====================== Thread Runtime =========================";
 qInfo() << "===============================================================";

 return app.exec();
}
