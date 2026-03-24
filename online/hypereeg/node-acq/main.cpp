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

/* This is the HyperEEG "Acquisition Daemon" Node.. a.k.a. the "Master Node".
 * It acquires from multiple (USB) EEG amps, and stereo audio@48Ksps from the
 * alsa2 audio input device designated within the Octopus HyperEEG box,
 * in a syncronized fashion, synchronizes them altogether and streams to any
 * numer of connected clients via IP:commandPort:streamPort.
 * As it is meant to be a persistent system service, it assumes the settings
 * at /etc/octopus/hypereeg.conf
 * FYI, The config file is a common one with separate sections for different
 * types of nodes, for multiple nodes running on the same system.
 */

#include <QCoreApplication>
#include "../common/globals.h"

#ifdef EEMAGINE
#include <eemagine/sdk/wrapper.cc>
#define _UNICODE
#define EEGO_SDK_BIND_DYNAMIC
#include <eemagine/sdk/factory.h>
#include <eemagine/sdk/amplifier.h>
#endif

#include <QString>
#include <QFile>
#include <QtGlobal>
#include <QDateTime>
#include <cstdio>
#include <vector>
#include <sys/stat.h>
#include "configparser.h"
#include "audioamp.h"
#include "serialdevice.h"
#include "confparam.h"
#include "acqdaemon.h"
#include "../common/messagehandler.h"
#ifdef EEMAGINE
#include "../common/rt_bootstrap.h"
#endif

const QString CFGPATH="/etc/octopus/hypereeg.conf";

bool eegamps_check(std::vector<amplifier*> *eeAmps,unsigned int ampCount) {
 if (eeAmps->size()<ampCount) return true;
 return false;
}

bool audio_check(AudioAmp *audioAmp) {
 audioAmp->init();
 if (!audioAmp->initAlsa()) return true;
 return false;
}

bool conf_init(ConfParam *conf) { QString cfgPath=CFGPATH;
 if (QFile::exists(cfgPath)) { ConfigParser cfp(cfgPath);
  if (!cfp.parse(conf)) {
   // Constants or calculated global settings upon the ones read from config file
   conf->refChnMaxCount=REF_CHN_MAXCOUNT; conf->bipChnMaxCount=BIP_CHN_MAXCOUNT;
   conf->physChnCount=conf->refChnCount+conf->bipChnCount; conf->totalChnCount=conf->physChnCount+2;
   conf->chnCount=conf->refChnCount+conf->bipChnCount+conf->metaChnCount;
   conf->totalCount=conf->ampCount*conf->totalChnCount;
   conf->eegSamplesInTick=conf->eegRate*conf->eegProbeMsecs/1000;
   conf->frameBytes=TcpSample(conf->ampCount,conf->physChnCount).serialize().size();
   //qDebug() << "FrameBytes=" << conf->frameBytes;
   conf->dumpRaw=false;
   // --------------------------------------------------------------------------------
   if (!conf->acqCommServer.listen(QHostAddress::Any,conf->acqCommPort)) {
    qCritical() << "Cannot start TCP server on <Comm> port:" << conf->acqCommPort;
    return true;
   }
   if (!conf->acqStrmServer.listen(QHostAddress::Any,conf->acqStrmPort)) {
    qCritical() << "Cannot start TCP server on <Strm> port:" << conf->acqStrmPort;
    return true;
   }
   // --------------------------------------------------------------------------------
   conf->tcpBufSize*=conf->eegRate;
   conf->tcpBuffer=QVector<TcpSample>(conf->tcpBufSize,TcpSample(conf->ampCount,conf->physChnCount));

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
 qInfo() << "===============================================================";
 qInfo() << "Connected # of amplifier(s):" << conf->ampCount;
 qInfo() << "Sample Rate->" << conf->eegRate << "sps";
 qInfo() << "TCP Ringbuffer allocated for" << conf->tcpBufSize << "seconds.";
 qInfo() << "EEG data fetched every" << conf->eegProbeMsecs << "ms.";
 qInfo("Per-amp Physical Channel#: %d (%d+%d)",conf->physChnCount,conf->refChnCount,conf->bipChnCount);
 qInfo("Per-amp All Channel (with Meta) #: %d (%d+%d+%d)",conf->chnCount,conf->refChnCount,conf->bipChnCount,conf->metaChnCount);
 qInfo() << "Per-amp Total Channel# (with Trig and Offset):" << conf->totalChnCount;
 qInfo() << "Total Channel# from all amps:" << conf->totalCount;
 qInfo() << "Referential channels gain:" << conf->refGain;
 qInfo() << "Bipolar channels gain:" << conf->bipGain;
 qInfo() << "AudioCard (analog) trigger threshold:" << conf->audTrigThr;
 qInfo() << "===============================================================";
 qInfo() << "                      NETWORKING SUMMARY";
 qInfo() << "===============================================================";
 qInfo() << "<ServerIP> is" << conf->acqIpAddr;
 qInfo() << "<Comm> listening on port" << conf->acqCommPort;
 qInfo() << "<Strm> listening on port" << conf->acqStrmPort;
}

int main(int argc,char *argv[]) {
 ConfParam conf; SerialDevice serDev;AudioAmp audioAmp(&conf);
 
 qInstallMessageHandler(qtMessageHandler);
 setvbuf(stdout,nullptr,_IOLBF,0); // Avoid buffering
 setvbuf(stderr,nullptr,_IONBF,0);
 
#ifdef __linux__
#ifdef EEMAGINE
 // 1) Avoid swapping/pagefault stalls
 lock_memory_or_warn(); // needs CAP_IPC_LOCK (or high memlock ulimit)
 // 2) Help scheduler a bit (optional)
 set_process_nice(-10); // needs CAP_SYS_NICE
#endif
#endif

 QCoreApplication app(argc,argv);

 umask(0002);

 qInfo() << "===============================================================";
 qInfo() << "                  OPENMP INITIALIZATION STATUS";
 qInfo() << "===============================================================";
 omp_diag();

 if (conf_init(&conf)) {
  qCritical("<FatalError> Failed to initialize Octopus-ReEL EEG Hyperacquisition daemon node.");
  return 1;
 }

#ifdef EEMAGINE
 using namespace eemagine::sdk;
 std::vector<amplifier*> eeAmps,eeAmpsSorted; factory eeFact("libeego-SDK.so"); eeAmps=eeFact.getAmplifiers();
#else
 using namespace eesynth;
 std::vector<amplifier*> eeAmps,eeAmpsSorted; factory eeFact(conf.ampCount); eeAmps=eeFact.getAmplifiers();
#endif

 qInfo() << "===============================================================";
 qInfo() << "                  HARDWARE CONNECTION STATUS";
 qInfo() << "===============================================================";

 qInfo() << "-------------";
 qInfo() << "Audio Device:";
 qInfo() << "-------------";
 if (audio_check(&audioAmp)) {
  qCritical() << "<Audio> AudioAmp ALSA init FAILURE!";
  return 1;
 }
 qInfo() << "<Audio> AudioAmp ALSA initialized successfully.";

 qInfo() << "---------------";
 qInfo() << "EEG Amplifiers:";
 qInfo() << "---------------";
 if (eegamps_check(&eeAmps,conf.ampCount)) {
  qCritical() << "<AmpProbe> At least one  of the amplifiers is offline!";
  return 1;
 }
 qInfo() << "<EEGs><AmpProbe> Amplifiers are initialized successfully.";

 // Sort amplifiers for their serial numbers
 qInfo() << "<EEGs> Sorting according to their IDs...";
 std::vector<unsigned int> sUnsorted,sSorted;
 for (auto& ee:eeAmps) sUnsorted.push_back(stoi(ee->getSerialNumber()));
 sSorted=sUnsorted; std::sort(sSorted.begin(),sSorted.end());
 for (auto& sNo:sSorted) { std::size_t ampIdx=0;
  for (auto& sNoU:sUnsorted) {
   if (sNo==sNoU) eeAmpsSorted.push_back(eeAmps[ampIdx]);
   ampIdx++;
  }
 }
 QString s,ss;
 s=""; for (auto& ee:eeAmps) { s.append(ss.setNum(stoi(ee->getSerialNumber()))).append(" "); } s.chop(1);
 qInfo() << "<EEGs> Before:" << s;
 s=""; for (auto& ee:eeAmpsSorted) { s.append(ss.setNum(stoi(ee->getSerialNumber()))).append(" "); } s.chop(1);
 qInfo() << "<EEGs> After:" << s;

// On/off check -- no need for synthetic amp. We know that they're always on.
#ifdef  EEMAGINE
 qInfo() << "-----------------------------------------------------";
 qInfo() << "<EEGs><AmpProbe> Checking amplifiers on/off status...";
 // Check whether the amplifiers are on and ready to stream.;
 int ampIdx=0; std::vector<bool> ampStatus;
 for (auto& amp:eeAmpsSorted) {
  try {
   stream *test=amp->OpenImpedanceStream(); delete test; ampStatus.push_back(true);
   qInfo() << "<EEGs><AmpProbe> EEG Amplifier" << ampIdx << "is on for streaming.";
  } catch (const exceptions::incorrectValue& ex) {
   ampStatus.push_back(false);
   qWarning() << "<EEGs><AmpProbe> EEG Amplifier" << ampIdx << "is connected but not switched on!";
  }
  ampIdx++;
 }

 for (unsigned int ampIdx=0;ampIdx<conf.ampCount;ampIdx++) {
  if (!ampStatus[ampIdx]) {
   qWarning() << "Exiting...";
   return 1;
  }
 }
#endif

// We're not checking HyperSync if we're not on real EEG amps.
#ifdef EEMAGINE
 qInfo() << "------------------------";
 qInfo() << "HyperSync Serial Device:";
 qInfo() << "------------------------";
 if (serDev.init()) {
  qCritical() << "<HyperSync> Device init FAILURE!";
  return 1;
 }
 qInfo() << "<HyperSync> Device initialized successfully.";
#endif
 
 conf_info(&conf);

 // Passing a nullptr for serDev if not on real EEG amps
 AcqDaemon acqDaemon(nullptr,&conf,&eeAmpsSorted,&audioAmp,&serDev);

 qInfo() << "===============================================================";
 qInfo() << "                 THREAD RUNTIME LOOP STARTED";
 qInfo() << "===============================================================";

 return app.exec();
}
