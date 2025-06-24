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

#ifndef ACQDAEMON_H
#define ACQDAEMON_H

#include <QObject>
#include <QApplication>
#include <QtNetwork>
#include <QThread>
#include <QVector>
#include <QMutex>
#include <unistd.h>

#include "../acqglobals.h"

#include "../cs_command.h"
#include "../tcpsample.h"
#include "../ampinfo.h"
#include "chninfo.h"
#include "confparam.h"
#include "tcpthread.h"
#include "clienthandler.h"
#include "configparser.h"

class AcqDaemon : public QTcpServer {
 Q_OBJECT
 public:
  AcqDaemon(QApplication *app,QObject *parent=0): QTcpServer(parent) {
   application=app;

   qDebug() << "---------------------------------------------------------------";

   ConfigParser cfp(application,"/etc/octopus_acqd.conf");
   cfp.parse(&conf,&chnInfo);
/*
   qDebug() << conf.ampCount << conf.tcpBufSize << conf.sampleRate << conf.eegProbeMsecs << conf.cmProbeMsecs \
	    << conf.ipAddr << conf.commPort << conf.dataPort << conf.guiX << conf.guiY << conf.cmCellSize;
   for (int i=0;i<chnInfo.size();i++) {
    qDebug() << chnInfo[i].physChn << chnInfo[i].chnName << chnInfo[i].topoTheta << chnInfo[i].topoPhi \
	     << chnInfo[i].topoX << chnInfo[i].topoY;
    for (unsigned int ii=0;ii<EE_AMPCOUNT;ii++) qDebug("%2.2f",chnInfo[i].cmLevel[ii]);
   }
*/
   extTrig=0;

   // TODO: C/C++ conventions doesn't allow determining the structure sizes in runtime without sacrificing efficiency.
   // So, we're preserving this check to assert that we're not happy with this.. In the future I hope that
   // only the ampCount within the config file will determine the amp count.

   // Convey global information to ampInfo
   ampInfo.ampCount=conf.ampCount;
   ampInfo.sampleRate=conf.sampleRate;
   ampInfo.refChnCount=REF_CHN_COUNT; ampInfo.refChnMaxCount=REF_CHN_MAXCOUNT;
   ampInfo.bipChnCount=BIP_CHN_COUNT; ampInfo.bipChnMaxCount=BIP_CHN_MAXCOUNT;
   ampInfo.physChnCount=REF_CHN_COUNT+BIP_CHN_COUNT;
   ampInfo.totalChnCount=ampInfo.physChnCount+2;
   ampInfo.totalCount=conf.ampCount*ampInfo.totalChnCount;
   ampInfo.probe_eeg_msecs=conf.eegProbeMsecs;
   ampInfo.probe_cm_msecs=conf.cmProbeMsecs;

   if (conf.ampCount != EE_AMPCOUNT) {
    //qDebug() << "octopus_acqd: <FUTURE_ASSERTION|TODO> AmpCounts from config file and static compilation differ. Exiting..";
    application->quit();
   }

   qDebug() << "---------------------------------------------------------------";
   qDebug() << "octopus_acqd: ---> Datahandling Info <---";
   qDebug() << "octopus_acqd: Sample Rate:" << ampInfo.sampleRate;
   qDebug() << "octopus_acqd: EEG probe every (ms):" << ampInfo.probe_eeg_msecs;
   qDebug() << "octopus_acqd: CM probe every (ms):" << ampInfo.probe_cm_msecs;
   qDebug() << "octopus_acqd: Per-amp Physical Channel#:" << ampInfo.physChnCount << "(" \
            << ampInfo.refChnCount << "+" << ampInfo.bipChnCount << ")";
   qDebug() << "octopus_acqd: Per-amp Total Channel# (with Trig and Offset):" << ampInfo.totalChnCount;
   qDebug() << "octopus_acqd: Total Channel# from all amps:" << ampInfo.totalCount;
   qDebug() << "---------------------------------------------------------------";

   cmLevelFrameW=conf.cmCellSize*11; cmLevelFrameH=conf.cmCellSize*12;
   acqGuiW=(cmLevelFrameW+10)*conf.ampCount; acqGuiH=cmLevelFrameH;
   if (EE_AMPCOUNT>GUI_MAX_AMP_PER_LINE) { acqGuiW/=2; acqGuiH*=2; }
   acqGuiW+=60; acqGuiH+=60;

   tcpBuffer.resize(conf.tcpBufSize*ampInfo.sampleRate); // in seconds, after which data is lost.

   QHostAddress hostAddress(conf.ipAddr);

// --------

   // Initialize Tcp Command Server
   commandServer=new QTcpServer(this);
   commandServer->setMaxPendingConnections(1);
   connect(commandServer,SIGNAL(newConnection()),this,SLOT(slotIncomingCommand()));

   setMaxPendingConnections(1);

   if (!commandServer->listen(hostAddress,conf.commPort) || !listen(hostAddress,conf.dataPort)) {
    qDebug() << "octopus_acqd: <Daemon> ERROR: Cannot start command and/or data server!";
    application->quit();
   } else {
    qDebug() << "octopus_acqd: <Daemon> Started successfully..";
    qDebug() << "octopus_acqd: <Daemon> Waiting for client connection..";
   }

   tcpBuffer.resize(conf.tcpBufSize*ampInfo.sampleRate); tcpBufPIdx=tcpBufCIdx=0;

   daemonRunning=true; eegImpedanceMode=false; clientConnected=false;
  }
  
  ConfParam conf; AmpInfo ampInfo; QVector<ChnInfo> chnInfo; unsigned int extTrig;
  int acqGuiX,acqGuiY,acqGuiW,acqGuiH,cmLevelFrameW,cmLevelFrameH;
  QMutex tcpMutex,guiMutex; QVector<tcpsample> tcpBuffer; quint64 tcpBufPIdx;
  bool daemonRunning,eegImpedanceMode,clientConnected;

  void registerCMLevelHandler(QObject *sh) {
   connect(this,SIGNAL(cmLevelsReady(void)),sh,SLOT(slotCMLevelsReady(void)));
  }

  void registerSendTriggerHandler(QObject *sh) {
   connect(this,SIGNAL(sendTrigger(unsigned char)),sh,SLOT(sendTrigger(unsigned char)));
  }

  void registerSendSynthTriggerHandler(QObject *sh) {
   connect(this,SIGNAL(sendSynthTrigger(unsigned int)),sh,SLOT(sendSynthTrigger(unsigned int)));
  }

  void updateCMLevels() { emit cmLevelsReady(); }

 signals:
  void repaintGUI(int ampNo);
  void cmLevelsReady(void);
  void sendTrigger(unsigned char trigger);
  void sendSynthTrigger(unsigned int trigger);

 public slots:
  void slotQuit() {
   application->quit();
  }

  void slotIncomingCommand() {
   if ((commandSocket=commandServer->nextPendingConnection()))
    connect(commandSocket,SIGNAL(readyRead()),this,SLOT(slotHandleCommand()));
  }

  void slotHandleCommand() {
   QDataStream commandStream(commandSocket);
   if ((quint64)commandSocket->bytesAvailable() >= sizeof(cs_command)) {
    commandStream.readRawData((char*)(&csCmd),sizeof(cs_command));
    qDebug("octopus_acqd: Command received -> 0x%x.",csCmd.cmd);
    switch (csCmd.cmd) {
     case CS_ACQ_INFO: qDebug(
                        "octopus_acqd: <TCPcmd> Sending Chn.Info..");
                       csCmd.cmd=CS_ACQ_INFO_RESULT;
		       csCmd.iparam[0]=ampInfo.sampleRate;
                       csCmd.iparam[1]=ampInfo.refChnCount;
                       csCmd.iparam[2]=ampInfo.bipChnCount;
                       csCmd.iparam[3]=ampInfo.physChnCount;
                       csCmd.iparam[4]=ampInfo.refChnMaxCount;
                       csCmd.iparam[5]=ampInfo.bipChnMaxCount;
                       csCmd.iparam[6]=ampInfo.physChnMaxCount;
                       csCmd.iparam[7]=ampInfo.totalChnCount;
                       csCmd.iparam[8]=ampInfo.totalCount;
                       csCmd.iparam[9]=ampInfo.probe_eeg_msecs;
                       csCmd.iparam[10]=ampInfo.probe_cm_msecs;
                       commandStream.writeRawData((const char*)(&csCmd),
                                                  sizeof(cs_command));
                       //dataSocket.flush();
                       commandSocket->flush(); break;
     case CS_ACQ_SETMODE:
		       if (csCmd.iparam[0]==0) eegImpedanceMode=true;
		       else eegImpedanceMode=false;
		       break;
     case CS_ACQ_MANUAL_TRIG:
		       extTrig=csCmd.iparam[0]; emit sendSynthTrigger(extTrig);
                       qDebug() << "octopus_acqd: <TCPcmd> External Trigger acknownledged. TCode: "
                                << extTrig;
		       break;
     case CS_ACQ_MANUAL_SYNC:
		       emit sendTrigger(AMP_SYNC_TRIG);
                       qDebug() << "octopus_acqd: <TCPcmd> External SYNC acknownledged.";
		       break;
     case CS_REBOOT:   qDebug("octopus_acqd: <privileged cmd received> System rebooting..");
                       system("/sbin/shutdown -r now"); commandSocket->close(); break;
     case CS_SHUTDOWN: qDebug("octopus_acqd: <privileged cmd received> System shutting down..");
                       system("/sbin/shutdown -h now"); commandSocket->close(); break;
     case CS_ACQ_TRIGTEST:
     default:          break;
    }
   }
  }

 protected:
  // Data port "connection handler"..
  void incomingConnection(qintptr socketDescriptor) override {

   //auto handler = std::make_shared<ClientHandler>(socketDescriptor, &buffer);
   //clients.append(handler);
   //handler->start();  // Runs in its own thread or event loop

   if (!clientConnected) {
    qDebug("octopus_acqd: <TCP incoming> New client connection..");
    if (!dataSocket.setSocketDescriptor(socketDescriptor)) {
     qDebug("octopus_acqd: <TCP incoming> Socket Error!"); clientConnected=false; return;
    } else {
     qDebug("octopus_acqd: <TCP incoming> ..accepted.. connecting..");
     connect(&dataSocket,SIGNAL(disconnected()),this,SLOT(slotDisconnected()));
     if (dataSocket.waitForConnected()) {
      //qDebug() << dataSocket.peerAddress();
      clientConnected=true;
      qDebug("octopus_acqd: <TCP incoming> Client connection established.");
      // Launch thread responsible for sending acq.data asynchronously..
 
      tcpBufCIdx=tcpBufPIdx; // Previous data is assumed to be gone..

      tcpThread=new TcpThread(this,&tcpBuffer,&tcpBufPIdx,&tcpBufCIdx,&daemonRunning,&clientConnected,&tcpMutex);
      // Delete thread if communication breaks..
      connect(tcpThread,SIGNAL(finished()),tcpThread,SLOT(deleteLater()));
      tcpThread->start(QThread::HighestPriority);
     } else {
      qDebug("octopus_acqd: <TCP incoming> Cannot connect to client socket!!!");
     }
    }
   } else qDebug("octopus_acqd: <TCP incoming> Already connected, connection NOT accepted.");

  }

 private slots:
  void slotDisconnected() {
   disconnect(&dataSocket,SIGNAL(disconnected()),this,SLOT(slotDisconnected()));
   clientConnected=false;
   while (tcpThread->isRunning()); // Wait for thread termination
   dataSocket.close();
   qDebug("octopus_acqd: <TCP disconnection> Client gone!");
  }

  void slotSendData() {
   quint64 tcpBufSize=tcpBuffer.size();
   QDataStream outputStream(&dataSocket);
   //qDebug() << outputStream.status();
   tcpMutex.lock();
    quint64 tcpDataCount=tcpBufPIdx-tcpBufCIdx; outBuffer.resize(tcpDataCount);
    for (quint64 i=0;i<tcpDataCount;i++) outBuffer[i]=tcpBuffer[(tcpBufCIdx+i)%tcpBufSize];
    outputStream.writeRawData((const char*)((char *)(outBuffer.data())),outBuffer.size()*sizeof(tcpsample));
    tcpBufCIdx=tcpBufPIdx;
    dataSocket.flush();
   tcpMutex.unlock();
   //qDebug() << "octopus_acqd: <TCP datasend> Data sent.";
  }

 private:
  QApplication *application; QTcpServer *commandServer;
  QTcpSocket *commandSocket; QTcpSocket dataSocket;
  quint64 tcpBufCIdx; QVector<tcpsample> outBuffer;

  TcpThread *tcpThread; cs_command csCmd;

  QString confHost;

  // Multiple clients
  QVector<std::shared_ptr<ClientHandler> > clients;
};

#endif
