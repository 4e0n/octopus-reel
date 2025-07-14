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
  explicit AcqMaster(QApplication *app=nullptr,QObject *parent=nullptr) : QObject(parent) {
   //conf=ConfParam();
   QString commResponse; QStringList sList,sList2;
   conf.application=app; conf.commSocket=new QTcpSocket(this); conf.dataSocket=new QTcpSocket(this);
   conf.scrCounter=0; conf.cntSpeedX=100;

   // Initialize
   if (QFile::exists(cfgPath)) {
    ConfigParser cfp(cfgPath);
    if (!cfp.parse(&conf)) {
     qInfo() << "---------------------------------------------------------------";
     qInfo() << "octopus_hacq_client: <ServerIP> is" << conf.ipAddr;
     qInfo() << "octopus_hacq_client: <Comm> listening on ports (comm,data):" << conf.commPort << conf.dataPort;
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
     conf.refChnCount=sList[2].toInt();
     conf.bipChnCount=sList[3].toInt();
     conf.refGain=sList[4].toFloat();
     conf.bipGain=sList[5].toFloat();
     conf.eegProbeMsecs=sList[6].toInt(); // This determines the maximum data feed rate together with sampleRate

     commResponse=conf.commandToDaemon(CMD_GETCHAN_S);
     if (!commResponse.isEmpty()) qDebug() << "octopus_hacq_client: <Config> Daemon replied:" << commResponse;
     else qDebug() << "octopus_hacq_client: <Config> No response or timeout.";
     sList=commResponse.split("\n"); ChnInfo chn;
     for (int i=0;i<sList.size();i++) {
      sList2=sList[i].split(",");
      chn.physChn=sList2[0].toInt();
      chn.chnName=sList2[1];
      chn.topoTheta=sList2[2].toFloat();
      chn.topoPhi=sList2[3].toFloat();
      chn.isBipolar=(bool)sList2[4].toInt();
      conf.chns.append(chn);
     }
     conf.chnCount=conf.chns.size();

     for (unsigned int i=0;i<conf.chnCount;i++)
      qDebug() << conf.chns[i].physChn << conf.chns[i].chnName << conf.chns[i].topoTheta << conf.chns[i].topoPhi << conf.chns[i].isBipolar;

     // Generate control window and EEG streaming windows
     controlWindow=new AcqControlWindow(&conf); controlWindow->show();

     conf.updated.resize(conf.ampCount);
     conf.scrPrvData.resize(conf.ampCount); conf.scrPrvDataF.resize(conf.ampCount);
     conf.scrCurData.resize(conf.ampCount); conf.scrCurDataF.resize(conf.ampCount);
     for (unsigned int i=0;i<conf.ampCount;i++) {
      conf.updated[i]=false;
      conf.scrPrvData[i].resize(conf.chnCount); conf.scrPrvDataF[i].resize(conf.chnCount);
      conf.scrCurData[i].resize(conf.chnCount); conf.scrCurDataF[i].resize(conf.chnCount);
     }

     for (unsigned int i=0;i<conf.ampCount;i++) {
      AcqStreamWindow* streamWin=new AcqStreamWindow(i,&conf);
      streamWin->show();
      streamWindows.append(streamWin);
     }

//     frameDivider=5; // 20 
//     scrollTimer=new QTimer(this);
//     dataPerBaseFrame=conf.sampleRate/EEGFRAME_REFRESH_RATE; // 1000/200=5
//     auraSampleCount=dataPerBaseFrame*frameDivider; // 5*20=100
//     auraTcpS.resize(auraSampleCount); auraS.resize(auraSampleCount);
//     meanS.resize(chnCount); stdS.resize(chnCount);
//     //(conf.sampleRate/conf.eegProbeMsecs); // Timebase is 100fps
//     connect(scrollTimer,&QTimer::timeout,this,&AcqMaster::scrollSlot);
//     scrollTimer->start(EEGFRAME_REFRESH_RATE);  // Timebase is: 100 ms → 10 fps

     connect(conf.dataSocket,&QTcpSocket::readyRead,&conf,&ConfParam::onDataReady);

     // At this point the scrolling widgets, and everything should be initialized and ready.
     // Setup data socket -- only safe after handshake and receiving crucial info about streaming
     conf.dataSocket->connectToHost(conf.ipAddr,conf.dataPort); conf.dataSocket->waitForConnected();
    } else {
     qWarning() << "octopus_hacq_client: The config file" << cfgPath << "is corrupt!";
     return;
    }
   } else {
    qWarning() << "octopus_hacq_client: The config file" << cfgPath << "does not exist!";
    return;
   }
  }

  ConfParam conf; QVector<float> meanS,stdS;

 private:
  AcqControlWindow *controlWindow; QVector<AcqStreamWindow*> streamWindows;
//  void computeMeanStd() {
//   int N=auraS.size();
//   if (N==0) { mean=0.0f; stDev=0.0f; return; }
//   float sum=0.0f,sumSq=0.0f;
//   for (float v:auraS) { sum+=v; sumSq+=v*v; }
//   mean=sum/N; stDev=sqrtf( (sumSq/N)-(mean*mean)  ); // std::max Clamp to avoid negative variance due to FP errors
//  }
//  QVector<TcpSample> auraTcpS; QVector<float> auraS;
//  QTimer *scrollTimer;
//  quint32 auraSampleCount;
//  float mean,stDev;
//  int frameDivider,dataPerBaseFrame;
};

#endif
