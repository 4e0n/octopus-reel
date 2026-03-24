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
 * Its main purpose is to record the **raw** EEGs+audio data broadcast by
 * the node-acq to disk. Starting and stopping of recording is held
 * by proper remote commands (mostly via GUI clients s.a. node-time).
 */

#include <QCoreApplication>
#include <QSurfaceFormat>
#include <QtGlobal>
#include <QDateTime>
#include <QString>
#include <QFile>
#include <cstdio>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <linux/ioprio.h>
#include "../common/globals.h"
#include "../common/messagehandler.h"
#include "confparam.h"
#include "configparser.h"
#include "stordaemon.h"

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
 qInfo() << "                    DETAILED CHANNELS INFO";
 qInfo() << "===============================================================";
 qInfo() << "---------------------------";
 qInfo() << "EEG (referential) channels:";
 qInfo() << "---------------------------"; if (conf->refChnCount==0) qInfo() << "None.";
 for (const auto& c:conf->refChns) qInfo("%d -> %s",c.physChn,qUtf8Printable(c.chnName));
 qInfo() << "--------------------------------";
 qInfo() << "BiPolar (differential) channels:";
 qInfo() << "--------------------------------"; if (conf->bipChnCount==0) qInfo() << "None.";
 for (const auto& c:conf->bipChns) qInfo("%d -> %s",c.physChn,qUtf8Printable(c.chnName));
 qInfo() << "--------------------------------";
 qInfo() << ">> Please note that Chn interpolation & Meta chns not (currently) defined fornode-stor. <<";
 qInfo() << "===============================================================";
 qInfo() << "                ACQUISITION PARAMETERS SUMMARY";
 qInfo() << "              (some are as relayed from node-acq)";
 qInfo() << "===============================================================";
 qInfo() << "# of amplifier(s):" << conf->ampCount;
 qInfo() << "Sample Rate->" << conf->eegRate << "sps";
 qInfo() << "TCP Ringbuffer allocated for" << conf->tcpBufSize << "seconds.";
 qInfo() << "EEG data fetched every" << conf->eegProbeMsecs << "ms.";
 qInfo("Per-amp Physical Channel#: %d (%d+%d)",conf->physChnCount,conf->refChnCount,conf->bipChnCount);
 qInfo() << "Per-amp Total Channel# (with Trig and Offset):" << conf->totalChnCount;
 qInfo() << "Total Channel# from all amps:" << conf->totalCount;
 qInfo() << "Referential channels gain:" << conf->refGain;
 qInfo() << "Bipolar channels gain:" << conf->bipGain;
 qInfo() << "===============================================================";
 qInfo() << "                      NETWORKING SUMMARY";
 qInfo() << "===============================================================";
 qInfo() << "<ServerIP> node-acq IP is" << conf->acqIpAddr;
 qInfo() << "<Comm> Downstreaming via ports (comm,strm):" << conf->acqCommPort << conf->acqStrmPort;
 qInfo() << "<Comm> listening on port(comm)" << conf->storCommPort;
}

int main(int argc,char* argv[]) {
 ConfParam conf;

 setIOPriority();

 qInstallMessageHandler(qtMessageHandler);
 setvbuf(stdout,nullptr,_IOLBF,0); // Avoid buffering
 setvbuf(stderr,nullptr,_IONBF,0);

//#ifdef __linux__
// // 1) Avoid swapping/pagefault stalls
// lock_memory_or_warn(); // needs CAP_IPC_LOCK (or high memlock ulimit)
//
// // 2) Help scheduler a bit
// set_process_nice(-10); // needs CAP_SYS_NICE
//#endif

 QCoreApplication app(argc,argv);

 umask(0002);

 qInfo() << "===============================================================";
 qInfo() << "                  OPENMP INITIALIZATION STATUS";
 qInfo() << "===============================================================";
 omp_diag();

 if (conf_init_pre(&conf)) {
  qCritical("<FatalError> Failed to initialize Octopus-ReEL recording/storage daemon node.");
  return 1;
 }

 StorDaemon storDaemon(nullptr,&conf);

 if (storDaemon.start()) {
  qCritical("<FatalError> Failed to start Octopus-ReEL recording/storage daemon node.");
  return 1;
 }

 conf_info(&conf);

 qInfo() << "===============================================================";
 qInfo() << "                 THREAD RUNTIME LOOP STARTED";
 qInfo() << "===============================================================";

 return app.exec();
}
