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

/* This is the generic "Compute Node".
 * Its mainly a buffer streaming in the EEG+Audio data, process it
 * (by default does nothing) and stream out.
 * It is mainly a skeleton to be cloned and modified for generating
 * custom compute nodes.
 */

#include <QCoreApplication>
#include <QSurfaceFormat>
#include <QtGlobal>
#include <QDateTime>
#include <cstdio>
#include <sys/stat.h>
#include "../common/globals.h"
#include "../common/messagehandler.h"
#include "confparam.h"
#include "configparser.h"
#include "ppdaemon.h"

const QString CFGPATH="/etc/octopus/hypereeg.conf";

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
 qInfo() << "<Comm> listening on port(comm,strm):" << conf->acqppCommPort << conf->acqppStrmPort;
}

int main(int argc,char* argv[]) {
 ConfParam conf;

 qInstallMessageHandler(qtMessageHandler);
 setvbuf(stdout,nullptr,_IOLBF,0); // Avoid buffering
 setvbuf(stderr,nullptr,_IONBF,0);

#ifdef __linux__
 // 1) Avoid swapping/pagefault stalls
 lock_memory_or_warn(); // needs CAP_IPC_LOCK (or high memlock ulimit)

 // 2) Help scheduler a bit (optional)
 set_process_nice(-10); // needs CAP_SYS_NICE
#endif

 QCoreApplication app(argc,argv);

 umask(0002);

 qInfo() << "===============================================================";
 omp_diag();
 qInfo() << "===============================================================";

 if (conf_init_pre(&conf)) {
  qCritical("<FatalError> Failed to initialize Octopus-ReEL EEG data compute node.");
  return 1;
 }

 PPDaemon ppDaemon(nullptr,&conf);

 if (ppDaemon.start()) {
  qCritical("<FatalError> Failed to start Octopus-ReEL EEG data compute node.");
  return 1;
 }

 conf_info(&conf);

 // Any (e.g. hardware probing related) intermediate checks in the future to come.

 qInfo() << "====================== Thread Runtime =========================";
 qInfo() << "===============================================================";

 return app.exec();
}
