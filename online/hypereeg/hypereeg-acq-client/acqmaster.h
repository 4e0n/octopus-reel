/*
Octopus-ReEL - Realtime Encephalography Laboratory Network
   Copyright (C) 2007-2025 Barkin Ilhan

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

/* a.k.a. "The Engine"..
    This is the predefined parameters, configuration and management class
    shared over all other classes of octopus-acq-client. */

#ifndef ACQMASTER_H
#define ACQMASTER_H

#include <QObject>
#include <QTcpSocket>
#include <QVector>
#include <QTimer>
#include "acqcontrolwindow.h"
#include "acqstreamwindow.h"
#include "configparser.h"
#include "confparam.h"
#include "chninfo.h"
#include "../tcpcommand.h"
#include "../tcpsample.h"

const int HYPEREEG_ACQ_CLIENT_VER=200;
const int EEGFRAME_REFRESH_RATE=200; // Base refresh rate

const QString cfgPath="/etc/octopus_hacq_client.conf";

class AcqMaster: public QObject {
 Q_OBJECT
 public:
  explicit AcqMaster(QObject *parent=nullptr) : QObject(parent) {
   QString commResponse; QStringList sList,sList2;
   conf.commSocket=new QTcpSocket(this);
   conf.eegDataSocket=new QTcpSocket(this); conf.cmDataSocket=new QTcpSocket(this);

   // Initialize
   if (QFile::exists(cfgPath)) {
    ConfigParser cfp(cfgPath);
    if (!cfp.parse(&conf)) {
     qInfo() << "---------------------------------------------------------------";
     qInfo() << "octopus_hacq_client: <ServerIP> is" << conf.ipAddr;
     qInfo() << "octopus_hacq_client: <Comm> listening on ports (comm,eegData,cmData):" << conf.commPort << conf.eegDataPort << conf.cmDataPort;
     qInfo() << "octopus_hacq_client: <GUI> Ctrl (X,Y,W,H):" << conf.guiCtrlX << conf.guiCtrlY << conf.guiCtrlW << conf.guiCtrlH;
     qInfo() << "octopus_hacq_client: <GUI> Strm (X,Y,W,H):" << conf.guiStrmX << conf.guiStrmY << conf.guiStrmW << conf.guiStrmH;
     qInfo() << "octopus_hacq_client: <GUI> Heam (X,Y,W,H):" << conf.guiHeadX << conf.guiHeadY << conf.guiHeadW << conf.guiHeadH;

     // Setup command socket
     conf.commSocket->connectToHost(conf.ipAddr,conf.commPort); conf.commSocket->waitForConnected();

     commResponse=conf.commandToDaemon(CMD_GETCONF_S);
     if (!commResponse.isEmpty()) qDebug() << "octopus_hacq_client: <Config> Daemon replied:" << commResponse;
     else qDebug() << "octopus_hacq_client: <Config> No response or timeout.";
     sList=commResponse.split(",");
     conf.init(sList[0].toInt()); // ampCount
     conf.sampleRate=sList[1].toInt();
     conf.tcpBufSize*=conf.sampleRate; // Convert tcpBufSize from seconds to samples
     conf.halfTcpBufSize=conf.tcpBufSize/2; // for fast-population check
     conf.tcpBuffer.resize(conf.tcpBufSize);

     conf.eegScrollDivider=conf.eegScrollCoeff[2];
     conf.eegScrollFrameTimeMs=conf.sampleRate/conf.eegScrollRefreshRate; // (1000sps/50Hz)=20ms=20samples
     // # of data to wait for, to be available for screen plot/scroller
     conf.scrAvailableSamples=conf.eegScrollFrameTimeMs*(conf.sampleRate/1000); // 20ms -> 20 sample

     conf.refChnCount=sList[2].toInt();
     conf.bipChnCount=sList[3].toInt();
     conf.refGain=sList[4].toFloat();
     conf.bipGain=sList[5].toFloat();
     conf.eegProbeMsecs=sList[6].toInt(); // This determines the maximum data feed rate together with sampleRate

     commResponse=conf.commandToDaemon(CMD_GETCHAN_S);
     if (!commResponse.isEmpty()) qDebug() << "octopus_hacq_client: <Config> Daemon replied:" << commResponse;
     else qDebug() << "octopus_hacq_client: <Config> No response or timeout.";
     sList=commResponse.split("\n"); ChnInfo chn;
     for (int chnIdx=0;chnIdx<sList.size();chnIdx++) {
      sList2=sList[chnIdx].split(",");
      chn.physChn=sList2[0].toInt();
      chn.chnName=sList2[1];
      chn.topoTheta=sList2[2].toFloat();
      chn.topoPhi=sList2[3].toFloat();
      chn.isBipolar=(bool)sList2[4].toInt();
      conf.chns.append(chn);
     }
     conf.chnCount=conf.chns.size();
     conf.s0.resize(conf.scrAvailableSamples);
     conf.s0s.resize(conf.scrAvailableSamples);
     conf.s1.resize(conf.scrAvailableSamples);
     conf.s1s.resize(conf.scrAvailableSamples);
     for (unsigned int idx=0;idx<conf.scrAvailableSamples;idx++) {
      conf.s0[idx].resize(conf.chnCount); conf.s0s[idx].resize(conf.chnCount);
      conf.s1[idx].resize(conf.chnCount); conf.s1s[idx].resize(conf.chnCount);
     }

     for (auto& chn:conf.chns) qDebug() << chn.physChn << chn.chnName << chn.topoTheta << chn.topoPhi << chn.isBipolar;

     // Generate control window and EEG streaming windows
     controlWindow=new AcqControlWindow(&conf); controlWindow->show();

     conf.scrollPending.resize(conf.ampCount);
     for (unsigned int ampIdx=0;ampIdx<conf.ampCount;ampIdx++) {
      conf.scrollPending[ampIdx]=false;
     }

     for (unsigned int ampIdx=0;ampIdx<conf.ampCount;ampIdx++) {
      AcqStreamWindow* streamWin=new AcqStreamWindow(ampIdx,&conf);
      streamWin->show();
      streamWindows.append(streamWin);
     }

     connect(conf.eegDataSocket,&QTcpSocket::readyRead,&conf,&ConfParam::onEEGDataReady);

     // At this point the scrolling widgets, and everything should be initialized and ready.
     // Setup data socket -- only safe after handshake and receiving crucial info about streaming
     conf.eegDataSocket->connectToHost(conf.ipAddr,conf.eegDataPort); conf.eegDataSocket->waitForConnected();
     conf.cmDataSocket->connectToHost(conf.ipAddr,conf.cmDataPort); conf.cmDataSocket->waitForConnected();
    } else {
     qWarning() << "octopus_hacq_client: The config file" << cfgPath << "is corrupt!";
     return;
    }
   } else {
    qWarning() << "octopus_hacq_client: The config file" << cfgPath << "does not exist!";
    return;
   }
  }

  ConfParam conf;

 private:
  AcqControlWindow *controlWindow; QVector<AcqStreamWindow*> streamWindows;
};

#endif
