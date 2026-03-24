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

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QString>
#include <QVector>
#include <QDir>
#include "../common/globals.h"
#include "../common/tcpsample_pp.h"
#include "../common/tcp_commands.h"
#include "confparam.h"
#include "configparser.h"
//#include "guichninfo.h"
#include "controlwindow.h"
#include "ampwindow.h"

const int EEGFRAME_REFRESH_RATE=100; // Base refresh rate

const QString cfgPathX=confPath()+"hypereeg.conf";

class GUIClient: public QObject {
 Q_OBJECT
 public:
  explicit GUIClient(QObject *parent=nullptr) : QObject(parent) {
   // Upstream (we're client)
   conf.acqCommSocket=new QTcpSocket(this);
   conf.acqCommSocket->setSocketOption(QAbstractSocket::LowDelayOption,1);
   conf.acqCommSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption,64*1024);
   // Upstream (we're client)
   conf.wavPlayCommSocket=new QTcpSocket(this);
   conf.wavPlayCommSocket->setSocketOption(QAbstractSocket::LowDelayOption,1);
   conf.wavPlayCommSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption,64*1024);
   // Upstream (we're client)
   conf.acqPPCommSocket=new QTcpSocket(this);
   conf.acqPPCommSocket->setSocketOption(QAbstractSocket::LowDelayOption,1);
   conf.acqPPCommSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption,64*1024);
   conf.acqPPStrmSocket=new QTcpSocket(this);
   conf.acqPPStrmSocket->setSocketOption(QAbstractSocket::LowDelayOption,1);
   conf.acqPPStrmSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption,64*1024);
   // Upstream (we're client)
   conf.storCommSocket=new QTcpSocket(this);
   conf.storCommSocket->setSocketOption(QAbstractSocket::LowDelayOption,1);
   conf.storCommSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption,64*1024);
  }

  bool initialize() { QString cfgPath=cfgPathX;
   if (cfgPathX=="~") cfgPath=QDir::homePath();
   else if (cfgPathX.startsWith("~/")) cfgPath=QDir::homePath()+cfgPathX.mid(1);
   if (QFile::exists(cfgPath)) { ConfigParser cfp(cfgPath);
    if (!cfp.parse(&conf)) {

     // Constants or calculated global settings upon the ones read from config file

     qInfo() << "---------------------------------------------------------------";
     qInfo() << "node-time: <ServerIP> is" << conf.acqPPIpAddr;
     qInfo() << "node-time: <Comm> listening on AcqServ ports (comm,strm):" << conf.acqPPCommPort << conf.acqPPStrmPort;
     qInfo() << "node-time: <Comm> listening on StorServ ports (comm):" << conf.storCommPort;
     qInfo() << "node-time: <GUI> Ctrl (X,Y,W,H):" << conf.guiCtrlX << conf.guiCtrlY << conf.guiCtrlW << conf.guiCtrlH;
     qInfo() << "node-time: <GUI> Amp (X,Y,W,H):" << conf.guiAmpX << conf.guiAmpY << conf.guiAmpW << conf.guiAmpH;

     controlWindow=new ControlWindow(&conf); controlWindow->show(); // Control Window

     for (unsigned int ampIdx=0;ampIdx<conf.ampCount;ampIdx++) { // Dynamic windows - one for each amp
      AmpWindow* sWin=new AmpWindow(ampIdx,&conf); ampWindows.append(sWin); sWin->show();
     }

     connect(conf.acqPPStrmSocket,&QTcpSocket::readyRead,&conf,&ConfParam::onStrmDataReady); // TCP handler for instream

     // Connect for streaming data -- only safe after handshake and receiving crucial info about streaming
     conf.acqPPStrmSocket->connectToHost(conf.acqPPIpAddr,conf.acqPPStrmPort); conf.acqPPStrmSocket->waitForConnected();

     return false;
    } else {
     qWarning() << "node-time: The config file" << cfgPath << "is corrupt!";
     return true;
    }
   } else {
    qWarning() << "node-time: The config file" << cfgPath << "does not exist!";
    return true;
   }
  }

  ConfParam conf;

 private:
  ControlWindow *controlWindow; QVector<AmpWindow*> ampWindows;
};
