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

#pragma once

#include <QCoreApplication>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDir>
#include <QString>
#include <QVector>
#include <QTimer>
#include <QByteArray>
#include <cmath>
#include <limits>
#include "../common/globals.h"
#include "../common/tcpsample_pp.h"
#include "../common/tcp_commands.h"
#include "confparam.h"
#include "configparser.h"
#include "powwindow.h"
#include "powglwindow.h"
#include "powcontrolwindow.h"

const int GLPOWER_FRAME_REFRESH_RATE=1; // Base refresh rate (seconds)
const int GUI_MAX_AMP_PER_LINE=4; // If >4 then (4,2), otherwise (N,1)
const int POW_REPLY_TIMEOUT_MS=1500;

class PowClient: public QObject {
 Q_OBJECT
 public:
  explicit PowClient(QObject *parent=nullptr,ConfParam *c=nullptr) : QObject(parent) { conf=c; }

  bool start() { QString commResponse; QStringList sList,sList2;

   // We're client of node-comp-pp
   conf->compPPCommSocket=new QTcpSocket(this);
   conf->compPPCommSocket->setSocketOption(QAbstractSocket::LowDelayOption,1);
   conf->compPPCommSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption,64*1024);

   // Setup COMPPP command socket
   conf->compPPCommSocket->connectToHost(conf->compPPIpAddr,conf->compPPCommPort);
   if (!conf->compPPCommSocket->waitForConnected(2000)) {
    qCritical() << "node-gui-glpower: <ConfigParser> Cannot connect to node-comp-pp:" << conf->compPPCommSocket->errorString();
    return true;
   }
   // Get crucial info from the "acquisition" node we connect to
   commResponse=conf->commandToDaemon(conf->compPPCommSocket,CMD_ACQ_GETCONF);
   if (commResponse.isEmpty()) {
    qCritical() << "node-gui-glpower: <ConfigParser> No response from Acquisition Node!";
    return true;
   }
   sList=commResponse.split(",");
   if (sList.size()<9) { // or 13 if you expect full GETCONF payload
    qCritical() << "node-gui-glpower: <ConfigParser> Bad GETCONF reply:" << commResponse;
    return true;
   }
   conf->ampCount=sList[0].toInt(); // (ACTUAL) AMPCOUNT
   conf->refChnCount=sList[2].toInt();
   conf->bipChnCount=sList[3].toInt();
   conf->metaChnCount=sList[4].toInt();
   conf->physChnCount=sList[5].toInt();
   conf->totalChnCount=sList[6].toInt();

   conf->powerChnCount=conf->refChnCount+conf->bipChnCount+conf->metaChnCount+3; // These last are GFPs
   conf->gfpAllIdx=conf->refChnCount+conf->bipChnCount+conf->metaChnCount;
   conf->gfpLeftIdx=conf->gfpAllIdx+1;
   conf->gfpRightIdx=conf->gfpAllIdx+2; 

   // CHANNELS
   const auto refChnCount=conf->refChnCount; const auto bipChnCount=conf->bipChnCount;
   const auto metaChnCount=conf->metaChnCount; const auto physChnCount=conf->physChnCount;

   commResponse=conf->commandToDaemon(conf->compPPCommSocket,CMD_ACQ_GETCHAN);
   if (commResponse.isEmpty()) qCritical() << "node-gui-glpower: <GetChannelListFromDaemon> (TIMEOUT) No response from Node!";
   sList=commResponse.split("\n"); PowChnInfo chn;

   conf->refChns.clear();
   for (unsigned int chnIdx=0;chnIdx<refChnCount;chnIdx++) { // Individual CHANNELs information
    sList2=sList[chnIdx].split(",");
    chn.physChn=sList2[0].toInt(); chn.chnName=sList2[1];
    chn.topoTheta=sList2[2].toFloat(); chn.topoPhi=sList2[3].toFloat();
    chn.topoX=sList2[4].toInt(); chn.topoY=sList2[5].toInt();
    chn.type=sList2[6].toInt();
    conf->refChns.append(chn);
   }
   conf->bipChns.clear();
   for (unsigned int chnIdx=0;chnIdx<bipChnCount;chnIdx++) { // Individual CHANNELs information
    sList2=sList[refChnCount+chnIdx].split(",");
    chn.physChn=sList2[0].toInt(); chn.chnName=sList2[1];
    chn.topoX=sList2[4].toInt(); chn.topoY=sList2[5].toInt();
    chn.type=sList2[6].toInt();
    conf->bipChns.append(chn);
   }
   conf->metaChns.clear();
   for (unsigned int chnIdx=0;chnIdx<metaChnCount;chnIdx++) { // Individual CHANNELs information
    sList2=sList[physChnCount+chnIdx].split(",");
    chn.physChn=sList2[0].toInt(); chn.chnName=sList2[1];
    chn.topoX=sList2[4].toInt(); chn.topoY=sList2[5].toInt();
    chn.type=sList2[6].toInt();
    conf->metaChns.append(chn);
   }

   conf->guiMaxAmpPerLine=GUI_MAX_AMP_PER_LINE;

   pollTimer=new QTimer(this);
   connect(pollTimer,&QTimer::timeout,this,&PowClient::slotPoll);

   replyTimer=new QTimer(this);replyTimer->setSingleShot(true);
   connect(replyTimer,&QTimer::timeout,this,&PowClient::slotReplyTimeout);

   connect(&(conf->powCommServer),&QTcpServer::newConnection,this,&PowClient::slotNewCommClient);

   connect(conf->compPPCommSocket,&QTcpSocket::readyRead,this,&PowClient::slotReadyRead);
   connect(conf->compPPCommSocket,&QTcpSocket::disconnected,this,&PowClient::slotDisconnected);

   if (!conf->powCommServer.listen(QHostAddress::Any,conf->powCommPort)) {
    qCritical() << "node-gui-glpower: <Comm> Cannot start command server on port:" << conf->powCommPort;
    return true;
   }

   // Constants or calculated global settings upon the ones read from config file
   if (conf->ampCount<=conf->guiMaxAmpPerLine) {
    conf->frameW=(conf->guiW-60)/(conf->ampCount+1)-10;
    conf->frameH=conf->guiH-60;
    conf->cellSize=conf->frameW/11;
   } else {
    const int perRow=conf->guiMaxAmpPerLine;
    const int rows=(conf->ampCount+perRow-1)/perRow;
    conf->frameW=(conf->guiW-40-(perRow-1)*20)/perRow;
    conf->frameH=(conf->guiH-40-(rows-1)*20)/rows;
    conf->cellSize=conf->frameW/11;
   }

   conf->curPowData.resize(conf->ampCount); for (auto& c:conf->curPowData) c.resize(conf->powerChnCount);

   conf->corrChHistA.resize(conf->refChnCount); conf->corrChHistB.resize(conf->refChnCount);
   conf->curChCorr.resize(conf->refChnCount);
   for (unsigned int ch=0;ch<conf->refChnCount;++ch) {
    conf->corrChHistA[int(ch)].resize(ConfParam::POWER_BAND_COUNT);
    conf->corrChHistB[int(ch)].resize(ConfParam::POWER_BAND_COUNT);
    conf->curChCorr[int(ch)].resize(ConfParam::POWER_BAND_COUNT);
    for (int b=0;b<ConfParam::POWER_BAND_COUNT;++b) conf->curChCorr[int(ch)][b]=0.0f;
   }

   conf->corrHistA.resize(ConfParam::POWER_BAND_COUNT);
   conf->corrHistB.resize(ConfParam::POWER_BAND_COUNT);
   conf->curCorr.resize(ConfParam::POWER_BAND_COUNT);
   for (int b=0;b<ConfParam::POWER_BAND_COUNT;++b) {
    conf->curCorr[b]=0.0f;
   }

   conf->loadObj(conf->scalpObjPath,conf->scalpObj);
   conf->loadObj(conf->skullObjPath,conf->skullObj);
   conf->loadObj(conf->brainObjPath,conf->brainObj);

   powWindow=new PowWindow(conf); powWindow->show(); // Power Levels Window
   powGLWindow=new PowGLWindow(conf); powGLWindow->show(); // Power Levels GLhead Window

   powControlWindow=new PowControlWindow(conf);
   connect(powControlWindow,&PowControlWindow::parametersChanged,this,[this](bool rebuild){
    if (rebuild && powGLWindow) powGLWindow->rebuildInterpolations();
    if (powGLWindow) powGLWindow->updateGLFrames();
    if (powWindow) powWindow->updatePowFrames();
   });
   powControlWindow->show();

   conf->curSpatialCorr.resize(ConfParam::POWER_BAND_COUNT);
   for (int b=0;b<ConfParam::POWER_BAND_COUNT;++b) conf->curSpatialCorr[b]=0.0f;

   pollTimer->start(conf->powRefreshMs);

   return false;
  }

  ConfParam *conf;

 private slots:
  void slotNewCommClient() {
   while (conf->powCommServer.hasPendingConnections()) {
    QTcpSocket *client=conf->powCommServer.nextPendingConnection();
    conf->powClients.append(client);

    connect(client,&QTcpSocket::readyRead,this,[this,client]() {
     QByteArray buf=client->property("rxbuf").toByteArray();
     buf+=client->readAll();
     int nlIdx;
     while ((nlIdx=buf.indexOf('\n'))>=0) {
      QByteArray line=buf.left(nlIdx).trimmed();
      buf.remove(0,nlIdx+1);
      if (!line.isEmpty()) handleCommand(QString::fromUtf8(line),client);
     }
     client->setProperty("rxbuf",buf);
    });

    connect(client,&QTcpSocket::disconnected,this,[this,client]() {
     conf->powClients.removeAll(client);
     client->deleteLater();
    });

    qInfo() << "node-gui-glpower: <Comm> Client connected from" << client->peerAddress().toString();
   }
  }

  void handleCommand(const QString &cmd,QTcpSocket *client) {
   qInfo() << "node-gui-glpower: <Comm> Received command:" << cmd;
   if (cmd==CMD_STATUS) {
    client->write("node-gui-glpower: ready.\n");
   } else if (cmd==CMD_GUI_SHOW) {
    if (powWindow) powWindow->show();
    client->write("node-gui-glpower: gui shown.\n");
   } else if (cmd==CMD_GUI_HIDE) {
    if (powWindow) powWindow->hide();
    client->write("node-gui-glpower: gui hidden.\n");
   } else if (cmd==CMD_GUI_RAISE) {
    if (powWindow) {
     powWindow->show(); powWindow->raise(); powWindow->activateWindow();
    }
    client->write("node-gui-glpower: gui raised.\n");
   } else if (cmd==CMD_GUI_REFRESH) {
    if (powWindow) powWindow->updatePowFrames();
    if (powGLWindow) powGLWindow->updateGLFrames();
    client->write("node-gui-glpower: refreshed.\n");
   } else if (cmd==CMD_GUI_START) {
    conf->pollingActive=true;
    if (pollTimer && !pollTimer->isActive()) pollTimer->start(conf->powRefreshMs);
    client->write("node-gui-glpower: polling started.\n");
   } else if (cmd.startsWith(CMD_GUI_SETREFRESH)) {
    QStringList parts=cmd.split("=");
    if (parts.size()==2) {
     int ms=parts[1].toInt();
     if (ms>=50 && ms<=5000) {
      conf->powRefreshMs=ms;
      if (pollTimer->isActive()) {
       pollTimer->stop();
       pollTimer->start(conf->powRefreshMs);
      }
      client->write(QString("node-gui-glpower: refresh set to %1 ms\n").arg(ms).toUtf8());
     } else {
      client->write("node-gui-glpower: invalid refresh range (50–5000 ms)\n");
     }
    } else {
     client->write("node-gui-glpower: usage GUISETREFRESH=ms\n");
    }
   } else if (cmd.startsWith(CMD_GUI_PALETTE)) {
    QStringList parts=cmd.split("=");
    if (parts.size()==2) {
     QString mode=parts[1].trimmed().toUpper();
     if (mode=="MEAN" || mode=="MEDIAN") {
      if (powWindow) powWindow->setPaletteMode(mode);
      client->write(QString("node-gui-glpower: palette set to %1\n").arg(mode).toUtf8());
     } else {
      client->write("node-gui-glpower: use MEAN or MEDIAN\n");
     }
    } else {
     client->write("node-gui-glpower: usage GUIPALETTE=MEAN|MEDIAN\n");
    }
   } else if (cmd==CMD_GUI_STOP) {
    conf->pollingActive=false;
    if (pollTimer && pollTimer->isActive()) pollTimer->stop();
    client->write("node-gui-glpower: polling stopped.\n");
   } else if (cmd==CMD_QUIT) {
    client->write("node-gui-glpower: quitting.\n");
    client->flush();
    requestShutdown(); // graceful shutdown
   } else {
    client->write("node-gui-glpower: unknown command.\n");
   }
  }

  void slotPoll() {
   if (!conf->pollingActive) return;
   if (!conf->compPPCommSocket) return;
   if (conf->compPPCommSocket->state()!=QAbstractSocket::ConnectedState) return;
   if (requestPending) return;
#ifdef EEGBANDSCOMP
   conf->compPPCommSocket->write(QString(CMD_COMPPP_GETPOWER+QString("\n")).toUtf8());
#else
   conf->compPPCommSocket->write(QString(CMD_COMPPP_GETCMLEVELS+QString("\n")).toUtf8());
#endif
   requestPending=true;
   replyTimer->start(POW_REPLY_TIMEOUT_MS);
   ++pollCounter;
  }

  void slotReadyRead() {
   rxBuffer+=conf->compPPCommSocket->readAll();
   int nlIdx;
   while ((nlIdx=rxBuffer.indexOf('\n'))>=0) {
    QByteArray line=rxBuffer.left(nlIdx).trimmed();
    rxBuffer.remove(0,nlIdx+1);
    if (line.isEmpty()) continue;
    QString reply=QString::fromUtf8(line); QStringList vals=reply.split(",",Qt::SkipEmptyParts);
    const int expected=int(conf->ampCount)*int(conf->powerChnCount)*ConfParam::POWER_BAND_COUNT;
    bool parsedOK=false;
    if (vals.size()>=expected) {
     QMutexLocker lk(&conf->mutex);
     conf->curPowData.resize(conf->ampCount);
     int k=0;
     for (unsigned int ampIdx=0;ampIdx<conf->ampCount;++ampIdx) {
      conf->curPowData[int(ampIdx)].resize(conf->powerChnCount);
      for (unsigned int chnIdx=0;chnIdx<conf->powerChnCount;++chnIdx) {
       conf->curPowData[int(ampIdx)][int(chnIdx)].resize(ConfParam::POWER_BAND_COUNT);
       for (int bandIdx=0;bandIdx<ConfParam::POWER_BAND_COUNT;++bandIdx) {
        conf->curPowData[int(ampIdx)][int(chnIdx)][bandIdx]=vals[k++].toFloat();
       }
      }
     }
     // curPowData valid: compute scaling
     float mn=1e30f; float mx=-1e30f;
     for (unsigned int ampIdx=0;ampIdx<conf->ampCount;++ampIdx) {
      for (unsigned int chnIdx=0;chnIdx<conf->refChnCount;++chnIdx) {
       if (int(ampIdx)>=conf->curPowData.size()) continue;
       if (int(chnIdx)>=conf->curPowData[int(ampIdx)].size()) continue;
       if (conf->glSelectedBand>=conf->curPowData[int(ampIdx)][int(chnIdx)].size()) continue;
       const float v=conf->curPowData[int(ampIdx)][int(chnIdx)][conf->glSelectedBand];
       mn=qMin(mn,v); mx=qMax(mx,v);
      }
     }
     if (mn<mx) { conf->glSharedMapMin=mn; conf->glSharedMapMax=mx; conf->glSharedMapValid=true; }

     updateCorrelationHistory();
     updateSpatioTemporalCorrelation();
     parsedOK=true;
    }

    requestPending=false;
    if (replyTimer->isActive()) replyTimer->stop();
    if (parsedOK) {
     if (powWindow) powWindow->updatePowFrames();
     if (powGLWindow) powGLWindow->updateGLFrames();
    }
   }
  }

  void slotReplyTimeout() {
   if (!requestPending) return;
   qWarning() << "node-gui-glpower: <PollTimeout> No Powers Vector reply within timeout.";
   requestPending=false;
   // For current simple line-based protocol this is acceptable.
   rxBuffer.clear();
  }

  void slotDisconnected() {
   qWarning() << "node-gui-glpower: <Socket> Disconnected from node-comp-pp.";
   requestPending=false;
   rxBuffer.clear();
   if (replyTimer->isActive()) replyTimer->stop();
  }

 private:
  void requestShutdown() {
   qInfo() << "node-gui-glpower: <Shutdown> Requested.";
   conf->quitPending=true; conf->powCommServer.close();
   if (pollTimer && pollTimer->isActive()) pollTimer->stop(); // stop polling
   requestPending=false;
   if (replyTimer && replyTimer->isActive()) replyTimer->stop(); // stop reply timer
   if (conf->compPPCommSocket) conf->compPPCommSocket->disconnectFromHost(); // disconnect upstream
   const auto clients=conf->powClients;
   for (auto *c:clients) { // disconnect all command clients
    if (c) { c->write("node-gui-glpower: server shutting down.\n"); c->flush(); c->disconnectFromHost(); }
   }
   conf->powClients.clear();
   QTimer::singleShot(0,[](){ QCoreApplication::quit(); }); // quit event loop (safe, no qApp needed)
  }

  float pearsonCorr(const QVector<float> &a,const QVector<float> &b) {
   const int n=qMin(a.size(),b.size()); if (n<3) return 0.0f;
   double ma=0.0,mb=0.0;
   for (int i=0;i<n;++i) { ma+=a[i]; mb+=b[i]; }
   ma/=double(n); mb/=double(n);
   double num=0.0,da=0.0,db=0.0;
   for (int i=0;i<n;++i) {
    const double xa=double(a[i])-ma; const double xb=double(b[i])-mb;
    num+=xa*xb; da+=xa*xa; db+=xb*xb;
   }
   const double den=std::sqrt(da*db); if (den<1e-12) return 0.0f;
   return float(num/den);
  }

  void updateCorrelationHistory() {
   if (conf->ampCount<2) return;
   if (conf->gfpAllIdx>=conf->powerChnCount) return;
   if (conf->curPowData.size()<2) return;
   for (int b=0;b<ConfParam::POWER_BAND_COUNT;++b) {
    if (conf->curPowData[0].size()<=int(conf->gfpAllIdx)) continue;
    if (conf->curPowData[1].size()<=int(conf->gfpAllIdx)) continue;
    if (conf->curPowData[0][int(conf->gfpAllIdx)].size()<=b) continue;
    if (conf->curPowData[1][int(conf->gfpAllIdx)].size()<=b) continue;
    const float a=conf->curPowData[0][int(conf->gfpAllIdx)][b]; const float c=conf->curPowData[1][int(conf->gfpAllIdx)][b];
    conf->corrHistA[b].append(a); conf->corrHistB[b].append(c);
    while (conf->corrHistA[b].size()>conf->corrWindowUpdates) conf->corrHistA[b].removeFirst();
    while (conf->corrHistB[b].size()>conf->corrWindowUpdates) conf->corrHistB[b].removeFirst();
    conf->curCorr[b]=pearsonCorr(conf->corrHistA[b],conf->corrHistB[b]);
   }
   for (unsigned int ch=0;ch<conf->refChnCount;++ch) {
    for (int b=0;b<ConfParam::POWER_BAND_COUNT;++b) {
     if (conf->curPowData[0].size()<=int(ch)) continue;
     if (conf->curPowData[1].size()<=int(ch)) continue;
     if (conf->curPowData[0][int(ch)].size()<=b) continue;
     if (conf->curPowData[1][int(ch)].size()<=b) continue;
     const float a=conf->curPowData[0][int(ch)][b]; const float c=conf->curPowData[1][int(ch)][b];
     conf->corrChHistA[int(ch)][b].append(a); conf->corrChHistB[int(ch)][b].append(c);
     while (conf->corrChHistA[int(ch)][b].size()>conf->corrWindowUpdates) conf->corrChHistA[int(ch)][b].removeFirst();
     while (conf->corrChHistB[int(ch)][b].size()>conf->corrWindowUpdates) conf->corrChHistB[int(ch)][b].removeFirst();
     float r=pearsonCorr(conf->corrChHistA[int(ch)][b],conf->corrChHistB[int(ch)][b]);
     if (std::fabs(r)<conf->corrDeadZone) r=0.0f;
     conf->curChCorr[int(ch)][b]=r;
    }
   }
   conf->corrValuesValid=true;
  }

  void updateSpatioTemporalCorrelation() {
   if (conf->ampCount<2) return;
   for (int band=0;band<ConfParam::POWER_BAND_COUNT;++band) {
    QVector<float> topoA; QVector<float> topoB;
    topoA.reserve(conf->refChnCount); topoB.reserve(conf->refChnCount);
    for (unsigned int ch=0;ch<conf->refChnCount;++ch) {
     if (int(ch)>=conf->corrChHistA.size()) continue;
     if (int(ch)>=conf->corrChHistB.size()) continue;
     if (band>=conf->corrChHistA[int(ch)].size()) continue;
     if (band>=conf->corrChHistB[int(ch)].size()) continue;
     const auto &ha=conf->corrChHistA[int(ch)][band]; const auto &hb=conf->corrChHistB[int(ch)][band];
     const int n=qMin(conf->spatialWindowUpdates,qMin(ha.size(),hb.size()));
     if (n<3) continue;
     double ma=0.0; double mb=0.0;
     for (int i=0;i<n;++i) { ma+=ha[ha.size()-n+i]; mb+=hb[hb.size()-n+i]; }
     ma/=double(n); mb/=double(n);

     auto safeLogPower=[](double x)->float {
      if (!std::isfinite(x)) return std::numeric_limits<float>::quiet_NaN();
      x=std::fabs(x);
      if (x<1e-12) x=1e-12;
      return float(std::log(x));
     };

     const float la=safeLogPower(ma); const float lb=safeLogPower(mb);
     if (std::isfinite(la) && std::isfinite(lb)) { topoA.append(la); topoB.append(lb); }
    }

//    auto minmax=[](const QVector<float> &v) {
//     float mn=1e30f,mx=-1e30f;
//     for (float x:v) { mn=qMin(mn,x); mx=qMax(mx,x); }
//     return qMakePair(mn,mx);
//    };

//    if (!topoA.isEmpty() && !topoB.isEmpty()) {
//     auto mmA=minmax(topoA); auto mmB=minmax(topoB);
//     if (band==conf->glSelectedBand) {
//      static int dbgRange=0;
//      if ((dbgRange++%20)==0) qDebug() << "[SPATIALCORR-RANGE]" << "A" << mmA.first << mmA.second << "B" << mmB.first << mmB.second;
//     }
//    }

    float r=0.0f;
    if (topoA.size()>=8 && topoB.size()>=8) {
     r=pearsonCorr(topoA,topoB);
     if (!std::isfinite(r)) r=0.0f;
     r=qBound(-1.0f,r,1.0f);
    }
    if (!conf->spatialCorrInit[band]) {
     conf->curSpatialCorr[band]=r; conf->spatialCorrInit[band]=true;
    } else {
     const float a=conf->spatialCorrSmooth;
     conf->curSpatialCorr[band]=(1.0f-a)*conf->curSpatialCorr[band]+a*r;
    }

//    if (band==conf->glSelectedBand) {
//     static int dbg=0;
//     if ((dbg++%20)==0)
//      qDebug() << "[SPATIALCORR]" << "band=" << band << "nCh=" << topoA.size() << "r=" << r << "smooth=" << conf->curSpatialCorr[band];
//    }
   }
   conf->spatialCorrValid=true;
  }

  PowWindow *powWindow=nullptr; PowGLWindow *powGLWindow=nullptr;
  PowControlWindow *powControlWindow=nullptr;

  QTimer *pollTimer=nullptr,*replyTimer=nullptr; QByteArray rxBuffer; bool requestPending=false; quint64 pollCounter=0;
};
