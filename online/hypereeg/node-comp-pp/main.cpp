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

/* This is the HyperEEG "Acquisition PreProcessor Daemon" a.k.a. "Level 2" Node.
 * It streams "multiple EEGs+audio data" from "Origin" Daemon (likely the acquisition node; node-acq)
 * in a fashion originally defined and set up, filters it for mains common-noise (50 Hz),
 * generates its bandpassed (2-40 Hz), and subbands (2-4,4-8,8-12,12-28 and 28-40 Hz)
 * versions and streams outward to connected clients.
 * Being a persistent system service by default, similar to node-acq,  it assumes
 * the settings at /etc/octopus/hypereeg.conf
 */

#include <QCoreApplication>
#include <QSurfaceFormat>
#include <QtGlobal>
#include <QDateTime>
#include <QString>
#include <QFile>
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
 qInfo() << "                    DETAILED CHANNELS INFO";
 qInfo() << "===============================================================";
 QString interElec;
 qInfo() << "---------------------------";
 qInfo() << "EEG (referential) channels:";
 qInfo() << "---------------------------"; if (conf->refChnCount==0) qInfo() << "None.";
 for (const auto& c:conf->refChns) {
  interElec=""; for (int i=0;i<c.interElec.size();i++) interElec.append(QString::number(c.interElec[i])+" ");
  qInfo("%d -> %s - (%2.1f,%2.2f) - [%d,%d] - Neighbors=%s",c.physChn,qUtf8Printable(c.chnName),
                                                            c.topoTheta,c.topoPhi,c.topoX,c.topoY,
                                                            interElec.toUtf8().constData());
 }
 qInfo() << "--------------------------------";
 qInfo() << "BiPolar (differential) channels:";
 qInfo() << "--------------------------------"; if (conf->bipChnCount==0) qInfo() << "None.";
 for (const auto& c:conf->bipChns) {
  interElec=""; for (int i=0;i<c.interElec.size();i++) interElec.append(QString::number(c.interElec[i])+" ");
  qInfo("%d -> %s - (%2.1f,%2.2f) - [%d,%d] - Neighbors=%s",c.physChn,qUtf8Printable(c.chnName),
                                                            c.topoTheta,c.topoPhi,c.topoX,c.topoY,
                                                            interElec.toUtf8().constData());
 }
 qInfo() << "------------------------------";
 qInfo() << "Meta (only-computed) channels:";
 qInfo() << "------------------------------"; if (conf->metaChnCount==0) qInfo() << "None.";
 for (const auto& c:conf->metaChns) {
  interElec=""; for (int i=0;i<c.interElec.size();i++) interElec.append(QString::number(c.interElec[i])+" ");
  qInfo("%d -> %s - (%2.1f,%2.2f) - [%d,%d] - Neighbors=%s",c.physChn,qUtf8Printable(c.chnName),
                                                            c.topoTheta,c.topoPhi,c.topoX,c.topoY,
                                                            interElec.toUtf8().constData());
 }
 qInfo() << "===============================================================";
 qInfo() << "                ACQUISITION PARAMETERS SUMMARY";
 qInfo() << "        (some are as relayed from origin node (node-acq)";
 qInfo() << "===============================================================";
 qInfo() << "# of amplifier(s):" << conf->ampCount;
 qInfo() << "Sample Rate->" << conf->eegRate << "sps";
 qInfo() << "TCP Ringbuffer allocated for" << conf->tcpBufSize << "seconds.";
 qInfo() << "EEG data fetched every" << conf->eegProbeMsecs << "ms.";
 qInfo("Per-amp Physical Channel#: %d (%d+%d)",conf->physChnCount,conf->refChnCount,conf->bipChnCount);
 qInfo("Per-amp All Channel (with Meta) #: %d (%d+%d+%d)",conf->chnCount,conf->refChnCount,conf->bipChnCount,conf->metaChnCount);
 qInfo() << "Per-amp Total Channel# (with Trig and Offset):" << conf->totalChnCount;
 qInfo() << "Total Channel# from all amps:" << conf->totalCount;
 qInfo() << "Referential channels gain:" << conf->refGain;
 qInfo() << "Bipolar channels gain:" << conf->bipGain;
 qInfo() << "===============================================================";
 qInfo() << "                      NETWORKING SUMMARY";
 qInfo() << "===============================================================";

 qInfo() << "<ServerIP> origin node IP is" << conf->origIpAddr;
 qInfo() << "<Comm> Downstreaming via ports (comm,strm):" << conf->origCommPort << conf->origStrmPort;
 qInfo() << "<Comm> Listening on port(comm,strm):" << conf->compCommPort << conf->compStrmPort;
}

int main(int argc,char* argv[]) {
 ConfParam conf;

 qInstallMessageHandler(qtMessageHandler);
 setvbuf(stdout,nullptr,_IOLBF,0); // Avoid buffering
 setvbuf(stderr,nullptr,_IONBF,0);

#ifdef __linux__
 // 1) Avoid swapping/pagefault stalls
 lock_memory_or_warn(); // needs CAP_IPC_LOCK (or high memlock ulimit)

 // 2) Help scheduler a bit
 set_process_nice(-10); // needs CAP_SYS_NICE
#endif

 QCoreApplication app(argc,argv);

 umask(0002);

 qInfo() << "===============================================================";
 qInfo() << "                  OPENMP INITIALIZATION STATUS";
 qInfo() << "===============================================================";
 omp_diag();

 if (conf_init_pre(&conf)) {
  qCritical("<FatalError> Failed to initialize Octopus-ReEL EEG pre-processor node.");
  return 1;
 }

 PPDaemon ppDaemon(nullptr,&conf);
 if (ppDaemon.start()) {
  qCritical("<FatalError> Failed to start Octopus-ReEL EEG data compute node.");
  return 1;
 }

 conf_info(&conf);

 qInfo() << "===============================================================";
 qInfo() << "                 THREAD RUNTIME LOOP STARTED";
 qInfo() << "===============================================================";

 return app.exec();
}
