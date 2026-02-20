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
#include "configparser.h"
#include "audioamp.h"
#include "serialdevice.h"
#include "confparam.h"
#include "acqdaemon.h"
#include "../common/messagehandler.h"
#include "../common/rt_bootstrap.h"

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
   conf->totalCount=conf->ampCount*conf->totalChnCount;
   conf->eegSamplesInTick=conf->eegRate*conf->eegProbeMsecs/1000;
   conf->frameBytes=TcpSample(conf->ampCount,conf->physChnCount).serialize().size();
   qDebug() << "FrameBytes=" << conf->frameBytes;
   conf->dumpRaw=false;
   // --------------------------------------------------------------------------------
   if (!conf->acqCommServer.listen(QHostAddress::Any,conf->acqCommPort)) {
    qCritical() << "Cannot start TCP server on <Comm> port:" << conf->acqCommPort;
    return true;
   }
   qInfo() << "<Comm> listening on port" << conf->acqCommPort;
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
 qInfo() << "Detailed channels info:";
 qInfo() << "-----------------------";
 QString interElec;
 for (const auto& ch:conf->chnInfo) {
  interElec=""; for (int i=0;i<ch.interElec.size();i++) interElec.append(QString::number(ch.interElec[i])+" ");
  qInfo("%d -> %s - (%2.1f,%2.2f) - [%d,%d] ChnType=%d, Inter=%s",ch.physChn,qUtf8Printable(ch.chnName),
                                                                  ch.topoTheta,ch.topoPhi,ch.topoX,ch.topoY,ch.type,
                                                                  interElec.toUtf8().constData());
 }
 qInfo() << "===============================================================";
 qInfo() << "Acquisition Parameters Summary:";
 qInfo() << "-------------------------------";
 qInfo() << "Connected # of amplifier(s):" << conf->ampCount;
 qInfo() << "Sample Rate->" << conf->eegRate << "sps";
 qInfo() << "TCP Ringbuffer allocated for" << conf->tcpBufSize << "seconds.";
 qInfo() << "EEG data fetched every" << conf->eegProbeMsecs << "ms.";
 qInfo("Per-amp Physical Channel#: %d (%d+%d)",conf->physChnCount,conf->refChnCount,conf->bipChnCount);
 qInfo() << "Per-amp Total Channel# (with Trig and Offset):" << conf->totalChnCount;
 qInfo() << "Total Channel# from all amps:" << conf->totalCount;
 qInfo() << "Referential channels gain:" << conf->refGain;
 qInfo() << "Bipolar channels gain:" << conf->bipGain;
 qInfo() << "===============================================================";
 qInfo() << "Networking Summary:";
 qInfo() << "-----------------------------";
 qInfo() << "<ServerIP> is" << conf->acqIpAddr;
 qInfo() << "<Strm> listening on port" << conf->acqStrmPort;
}

int main(int argc,char *argv[]) {
 AudioAmp audioAmp; SerialDevice serDev; ConfParam conf;
 
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

 qInfo() << "===============================================================";
 omp_diag();
 qInfo() << "===============================================================";

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
 qInfo() << "Hardware connection status...";
 qInfo() << "-----------------------------";

 qInfo() << "Audio Device...";
 qInfo() << "---------------";
 if (audio_check(&audioAmp)) {
  qCritical() << "<Audio> AudioAmp ALSA init FAILURE!";
  return 1;
 }
 qInfo() << "<Audio> AudioAmp ALSA initialized successfully.";
 qInfo() << "---------------------------------------------------------------";

 qInfo() << "EEG Amplifiers...";
 qInfo() << "-----------------";
 if (eegamps_check(&eeAmps,conf.ampCount)) {
  qCritical() << "<AmpProbe> At least one  of the amplifiers is offline!";
  return 1;
 }

 qInfo() << "<AmpProbe> Amplifiers are initialized successfully.";

 // Sort amplifiers for their serial numbers
 qInfo() << "Sorting according to their IDs...";
 std::vector<unsigned int> sUnsorted,sSorted;
 for (auto& ee:eeAmps) sUnsorted.push_back(stoi(ee->getSerialNumber()));
 sSorted=sUnsorted; std::sort(sSorted.begin(),sSorted.end());
 for (auto& sNo:sSorted) { std::size_t ampIdx=0;
  for (auto& sNoU:sUnsorted) {
   if (sNo==sNoU) eeAmpsSorted.push_back(eeAmps[ampIdx]);
   ampIdx++;
  }
 }
 qInfo() << "Before:";
 for (auto& ee:eeAmps) qInfo() << stoi(ee->getSerialNumber());
 qInfo() << "After:";
 for (auto& ee:eeAmpsSorted) qInfo() << stoi(ee->getSerialNumber());

// On/off check -- no need for synthetic amp. We know that they're always on.
#ifdef  EEMAGINE
 qInfo() << "---------------------------------------------------------------";
 qInfo() << "<AmpProbe> Checking amplifiers on/off status...";

 // Check whether the amplifiers are on and ready to stream.;
 int ampIdx=0; std::vector<bool> ampStatus;
 for (auto& amp:eeAmpsSorted) {
  try {
   stream *test=amp->OpenImpedanceStream(); delete test; ampStatus.push_back(true);
   qInfo() << "<AmpProbe> EEG Amplifier" << ampIdx << "is on for streaming.";
  } catch (const exceptions::incorrectValue& ex) {
   ampStatus.push_back(false);
   qWarning() << "<AmpProbe> EEG Amplifier" << ampIdx << "is connected but not switched on!";
  }
  ampIdx++;
 }

 for (int ampIdx=0;ampIdx<conf.ampCount;ampIdx++) {
  if (!ampStatus[ampIdx]) {
   qWarning() << "Exiting...";
   return 1;
  }
 }

 qInfo() << "---------------------------------------------------------------";
#endif

// We're not checking HyperSync if we're not on real EEG amps.
#ifdef EEMAGINE
 qInfo() << "HyperSync Serial Device...";
 qInfo() << "--------------------------";
 if (serDev.init()) {
  qCritical() << "<HyperSync> Device init FAILURE!";
  return 1;
 }
 qInfo() << "<HyperSync> Device initialized successfully.";
#endif
 
 conf_info(&conf);

 qInfo() << "====================== Thread Runtime =========================";
 qInfo() << "===============================================================";

 // Passing a nullptr for serDev if not on real EEG amps
 AcqDaemon acqDaemon(nullptr,&conf,&eeAmpsSorted,&audioAmp,&serDev);

 return app.exec();
}
