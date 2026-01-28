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

/*
 * This is the HyperEEG "Common Mode" GUI Client, which is may be one of the
 * simplest client types in a Octopus-ReEL network.
 * It connects to the Hyper-acq daemon over TCP via a pair of sockets
 * (IP:commandPort:cmStreamPort) to visualize CM-noise levels.
 *
 * Even though it can live anywhere on the network, it most likely runs
 * on the same computer together with the daemon, for its GUI window being
 * visible to the EEG technician-experimenter to adjust any problematic
 * electrode connections during a session of real-time observation for
 * EEG-derived sophisticated variables computed and visualized
 * under determined conditions such as speech, or music.
 */

#pragma once

#include <QObject>
#include <QTcpSocket>
#include "../common/globals.h"
#include "../common/tcpcmarray.h"
#include "../common/tcp_commands.h"
#include "confparam.h"
#include "configparser.h"
#include "chninfo.h"
#include "cmodwindow.h"

const int HYPEREEG_CM_CLIENT_VER=200;
const int CMFRAME_REFRESH_RATE=200; // Base refresh rate

const QString cfgPath=hyperConfPath+"node-gui-cmode.conf";

class CModClient: public QObject {
 Q_OBJECT
 public:
  explicit CModClient(QObject *parent=nullptr) : QObject(parent) {
   conf.commSocket=new QTcpSocket(this); conf.cmodSocket=new QTcpSocket(this);
  }

  bool initialize() {
   QString commResponse; QStringList sList;
   if (QFile::exists(cfgPath)) {
    ConfigParser cfp(cfgPath);
    if (!cfp.parse(&conf)) {
     //qInfo() << "---------------------------------------------------------------";

     // Setup command socket
     conf.commSocket->connectToHost(conf.ipAddr,conf.commPort); conf.commSocket->waitForConnected();

     commResponse=conf.commandToDaemon(CMD_ACQD_GETCONF);
     if (commResponse.isEmpty())qDebug() << "hnode_cmod_gui: <Config> No response or timeout.";
     //else qDebug() << "hnode_cmod_gui: <Config> Daemon replied:" << commResponse;
     sList=commResponse.split(",");
     conf.init(sList[0].toInt()); // ampCount
     conf.refChnCount=sList[2].toInt();
     conf.bipChnCount=sList[3].toInt();
     conf.tcpBufSize*=10;
     conf.halfTcpBufSize=conf.tcpBufSize/2;
     conf.tcpBuffer.resize(conf.tcpBufSize); // for fast-population check
     
     conf.frameW=conf.guiCellSize*11; conf.frameH=conf.guiCellSize*12;
     conf.guiW=(conf.frameW+10)*conf.ampCount; conf.guiH=conf.frameH;
     if (conf.ampCount>GUI_MAX_AMP_PER_LINE) { conf.guiW/=2; conf.guiH*=2; }
     conf.guiW+=60; conf.guiH+=60;

     commResponse=conf.commandToDaemon(CMD_ACQD_GETCHAN);
     if (commResponse.isEmpty()) qDebug() << "hnode_cmod_gui: <Config> No response or timeout.";
     //else qDebug() << "hnode_cmod_gui: <Config> Daemon replied:" << commResponse;
     sList=commResponse.split("\n");
     for (auto& s:sList) {
      ChnInfo chn; QStringList sL=s.split(",");
      chn.physChn=sL[0].toInt(); chn.chnName=sL[1];
      chn.topoX=sL[4].toInt(); chn.topoY=sL[5].toInt();
      chn.isBipolar=(bool)sL[6].toInt();
      conf.chns.append(chn);
     }
     conf.chnCount=conf.chns.size();
     conf.cmCurData.resize(conf.ampCount);
     for (auto& cD:conf.cmCurData) cD.resize(conf.chnCount);

     qDebug() << "hnode_cmod_gui: <ChannelList>:";
     for (auto& c:conf.chns) qDebug() << c.physChn << c.chnName << c.topoX << c.topoY << c.isBipolar;

     // Generate control window and EEG streaming windows
     cmWindow=new CModWindow(&conf); cmWindow->show();

     conf.guiPending.resize(conf.ampCount);
     for (unsigned int ampIdx=0;ampIdx<conf.ampCount;ampIdx++) {
      conf.guiPending[ampIdx]=false;
     }

     connect(conf.cmodSocket,&QTcpSocket::readyRead,&conf,&ConfParam::onDataReady);
     conf.cmodSocket->connectToHost(conf.ipAddr,conf.dataPort); conf.cmodSocket->waitForConnected();

     return false;
    } else {
     qWarning() << "hnode_cmod_gui: The config file" << cfgPath << "is corrupt!";
     return true;
    }
   } else {
    qWarning() << "hnode_cmod_gui: The config file" << cfgPath << "does not exist!";
    return true;
   }
  }

  ConfParam conf;

 private:
  CModWindow *cmWindow;
};
