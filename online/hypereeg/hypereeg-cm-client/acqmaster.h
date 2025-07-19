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

#ifndef ACQMASTER_H
#define ACQMASTER_H

#include <QObject>
#include <QTcpSocket>
#include "cmwindow.h"
#include "configparser.h"
#include "confparam.h"
#include "chninfo.h"
#include "../tcpcommand.h"
#include "../tcpcmarray.h"

const int HYPEREEG_CM_CLIENT_VER=200;

const QString cfgPath="/etc/octopus_hcm_client.conf";

class AcqMaster: public QObject {
 Q_OBJECT
 public:
  explicit AcqMaster(QApplication *app=nullptr,QObject *parent=nullptr) : QObject(parent) {
   QString commResponse; QStringList sList;
   conf.application=app;
   conf.commSocket=new QTcpSocket(this); conf.dataSocket=new QTcpSocket(this);

   // Initialize
   if (QFile::exists(cfgPath)) {
    ConfigParser cfp(cfgPath);
    if (!cfp.parse(&conf)) {
     //qInfo() << "---------------------------------------------------------------";

     // Setup command socket
     conf.commSocket->connectToHost(conf.ipAddr,conf.commPort); conf.commSocket->waitForConnected();

     commResponse=conf.commandToDaemon(CMD_GETCONF_S);
     if (commResponse.isEmpty())qDebug() << "octopus_hcm_client: <Config> No response or timeout.";
     //else qDebug() << "octopus_hcm_client: <Config> Daemon replied:" << commResponse;
     sList=commResponse.split(",");
     conf.init(sList[0].toInt()); // ampCount
     conf.refChnCount=sList[2].toInt();
     conf.bipChnCount=sList[3].toInt();
     
     conf.frameW=conf.guiCellSize*11; conf.frameH=conf.guiCellSize*12;
     conf.guiW=(conf.frameW+10)*conf.ampCount; conf.guiH=conf.frameH;
     if (conf.ampCount>GUI_MAX_AMP_PER_LINE) { conf.guiW/=2; conf.guiH*=2; }
     conf.guiW+=60; conf.guiH+=60;

     commResponse=conf.commandToDaemon(CMD_GETCHAN_S);
     if (commResponse.isEmpty()) qDebug() << "octopus_hcm_client: <Config> No response or timeout.";
     //else qDebug() << "octopus_hcm_client: <Config> Daemon replied:" << commResponse;
     sList=commResponse.split("\n");
     for (auto& s:sList) {
      ChnInfo chn; QStringList sL=s.split(",");
      chn.physChn=sL[0].toInt(); chn.chnName=sL[1];
      chn.topoX=sL[4].toInt(); chn.topoY=sL[5].toInt();
      chn.isBipolar=(bool)sL[6].toInt();
      conf.chns.append(chn);
     }
     conf.chnCount=conf.chns.size();
     for (auto& cD:conf.cmCurData) cD.resize(conf.chnCount);

     qDebug() << "octopus_hcm_client: <ChannelList>:";
     for (auto& c:conf.chns) qDebug() << c.physChn << c.chnName << c.topoX << c.topoY << c.isBipolar;

     // Generate control window and EEG streaming windows
     cmWindow=new CMWindow(&conf); cmWindow->show();

     connect(conf.dataSocket,&QTcpSocket::readyRead,&conf,&ConfParam::onDataReady);
     conf.dataSocket->connectToHost(conf.ipAddr,conf.dataPort); conf.dataSocket->waitForConnected();
    } else {
     qWarning() << "octopus_hcm_client: The config file" << cfgPath << "is corrupt!";
     return;
    }
   } else {
    qWarning() << "octopus_hcm_client: The config file" << cfgPath << "does not exist!";
    return;
   }
  }

  ConfParam conf;

 private:
  CMWindow *cmWindow;
};

#endif
