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

#ifndef STIMDAEMON_H
#define STIMDAEMON_H

#include <QtNetwork>
#include <unistd.h>
#include <stdlib.h>
#include <rtai_fifos.h>
#include <rtai_shm.h>
#include "../stimglobals.h"
#include "../patt_datagram.h"
#include "../fb_command.h"
#include "../cs_command.h"
#include "../stim_test_para.h"

class StimDaemon : public QTcpServer {
 Q_OBJECT
 public:
  StimDaemon(QObject *parent,QCoreApplication *app): QTcpServer(parent) {
   application=app;

   // Parse system config file for variables
   QStringList cfgValidLines,opts,opts2,netSection;
   QFile cfgFile; QTextStream cfgStream;
   QString cfgLine; QStringList cfgLines; cfgFile.setFileName("/etc/octopus_stimd.conf");
   if (!cfgFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qDebug() << "octopus_stimd: <.conf> cannot load /etc/octopus_stimd.conf.";
    qDebug() << "octopus_stimd: <.conf> Falling back to hardcoded defaults.";
    confHost="127.0.0.1";  confCommP=65000;  confDataP=65001;
   } else { cfgStream.setDevice(&cfgFile);
    while (!cfgStream.atEnd()) { cfgLine=cfgStream.readLine(160); // Max Line Size
     cfgLines.append(cfgLine); } cfgFile.close();

    // Parse config
    for (int i=0;i<cfgLines.size();i++) { // Isolate valid lines
     if (!(cfgLines[i].at(0)=='#') &&
         cfgLines[i].contains('|')) cfgValidLines.append(cfgLines[i]); }

    for (int i=0;i<cfgValidLines.size();i++) {
     opts=cfgValidLines[i].split("|");
          if (opts[0].trimmed()=="NET") netSection.append(opts[1]);
     else { qDebug() << "octopus_stimd: <.conf> Unknown section in .conf file!";
      app->quit();
     }
    }

    // NET
    if (netSection.size()>0) {
     for (int i=0;i<netSection.size();i++) { opts=netSection[i].split("=");

      if (opts[0].trimmed()=="STIM") { opts2=opts[1].split(",");
       if (opts2.size()==3) { confHost=opts2[0].trimmed();
        QHostInfo qhiAcq=QHostInfo::fromName(confHost);
        confHost=qhiAcq.addresses().first().toString();
        qDebug() << "octopus_stimd: <.conf> (this) Host IP is" << confHost;
        confCommP=opts2[1].toInt(); confDataP=opts2[2].toInt();
        // Simple port validation..
        if ((!(confCommP >= 1024 && confCommP <= 65535)) ||
            (!(confDataP >= 1024 && confDataP <= 65535))) {
         qDebug() << "octopus_stimd: <.conf> Error in Hostname/IP and/or port settings!";
         app->quit();
        } else {
         qDebug() << "octopus_stimd: <.conf> CommPort ->" << confCommP << "DataPort ->" << confDataP;
	}
       }
      } else {
       qDebug() << "octopus_stimd: <.conf> Parse error in Hostname/IP(v4) Address!";
       app->quit();
      }
     }
    } else {
     confHost="127.0.0.1";  confCommP=65000;  confDataP=65001;
    }
   }

   // FIFOs
   if ((fbFifo=open("/dev/rtf0",O_WRONLY))>0 && (bfFifo=open("/dev/rtf1",O_RDONLY|O_NONBLOCK))>0) {
    // Send RESET Backend Command
    reset_msg.id=STIM_RST_SYN;
    write(fbFifo,&reset_msg,sizeof(fb_command)); sleep(1); reset_msg.id=0;
    read(bfFifo,&reset_msg,sizeof(fb_command));
    if (reset_msg.id!=STIM_RST_ACK) {
     qDebug() << "octopus_stimd: <KernelFIFOs> Kernel-space backend does not respond!"; app->quit();
    } else { // Backend ACKd. Everything's OK. Close and repoen FIFO in blocking mode.
     ::close(bfFifo); if ((bfFifo=open("/dev/rtf1",O_RDONLY))<0) {
      qDebug() << "octopus_stimd: <KernelFIFOs> Error during reoping fifo in blocking mode!"; app->quit();
     }
    }
   } else {
    qDebug() << "octopus_stimd: Cannot open any of the f2b or b2f FIFOs!"; app->quit();
   }

   // SHM
   if ((shmBuffer=(char *)rtai_malloc('XFER',XFERBUFSIZE))<=0) {
    qDebug() << "octopus_stimd: Kernel-space backend SHM could not be opened!"; app->quit();
   } //else {
    //qDebug("octopus_stimd: Kernel-space backend SHM addr: 0x%x",shmBuffer);
   //}

   QHostAddress hostAddress(confHost);
   //qDebug() << hostAddress;
   // Initialize Tcp Command Server
   commandServer=new QTcpServer(this);
   commandServer->setMaxPendingConnections(1);
   connect(commandServer,SIGNAL(newConnection()),this,SLOT(slotIncomingCommand()));
   connect(this,SIGNAL(newConnection()),this,SLOT(slotIncomingData()));
   setMaxPendingConnections(1);

   if (!commandServer->listen(hostAddress,confCommP) || !listen(hostAddress,confDataP)) {
    qDebug() << "octopus_stimd: Error starting command and/or data server(s)!"; app->quit();
   } else {
    qDebug() << "octopus_stimd: Daemon started successfully..";
    qDebug() << "octopus_stimd: Waiting for client connection..";
    fbWrite(STIM_SET_PARADIGM,PARA_ITD_OPPCHN2,0);
   }
   daemonRunning=true; clientConnected=false;
  }

  void fbWrite(unsigned short code,int p1,int p2) {
   fbMsg.id=code; fbMsg.iparam[0]=p1; fbMsg.iparam[1]=p2;
   write(fbFifo,&fbMsg,sizeof(fb_command));
  }

 public slots:
  void slotIncomingCommand() {
   if ((commandSocket=commandServer->nextPendingConnection()))
    connect(commandSocket,SIGNAL(readyRead()),this,SLOT(slotHandleCommand()));
  }

  void slotHandleCommand() {
   QDataStream commandStream(commandSocket);
//   commandStream.setVersion(QDataStream::Qt_4_0);
   if ((quint64)commandSocket->bytesAvailable() >= sizeof(cs_command)) {
    commandStream.readRawData((char*)(&csCmd),sizeof(cs_command));
    qDebug("octopus_stimd: Command received -> 0x%x(%d,%d)",csCmd.cmd,csCmd.iparam[0],csCmd.iparam[1]);
    switch (csCmd.cmd) {
     case CS_STIM_SET_PARADIGM:
      qDebug("octopus_stimd: Paradigm changed to %d.",csCmd.iparam[0]);
      fbWrite(STIM_SET_PARADIGM,csCmd.iparam[0],csCmd.iparam[1]);
      break;
     case CS_STIM_LOAD_PATTERN_SYN:
      fileSize=csCmd.iparam[0]; csCmd.cmd=CS_STIM_LOAD_PATTERN_ACK;
      qDebug("octopus_stimd: Starting pattern stream. FileSize=%d",fileSize);
      commandStream.writeRawData((const char*)(&csCmd),sizeof(cs_command)); commandSocket->flush();
      incomingDataSize=0;
      fbWrite(STIM_LOAD_PATTERN,fileSize,0); // Inform backend that data will come.
      break;
     case CS_STIM_START:
      qDebug() << "octopus_stimd: Received start stim command.";
      fbWrite(STIM_START,0,0);
      break;
     case CS_STIM_STOP:
      qDebug() << "octopus_stimd: Received stop stim command.";
      fbWrite(STIM_STOP,0,0);
      break;
     case CS_STIM_PAUSE:
      qDebug() << "octopus_stimd: Received pause stim command.";
      fbWrite(STIM_PAUSE,0,0);
      break;
     case CS_STIM_RESUME:
      qDebug() << "octopus_stimd: Received resume stim command.";
      fbWrite(STIM_RESUME,0,0);
      break;
     case CS_TRIG_START:
      qDebug() << "octopus_stimd: Received start trigger command.";
      fbWrite(TRIG_START,0,0);
      break;
     case CS_TRIG_STOP:
      qDebug() << "octopus_stimd: Received stop trigger command.";
      fbWrite(TRIG_STOP,0,0);
      break;
     case CS_STIM_LIGHTS_ON:
      qDebug() << "octopus_stimd: Received lights on stim command.";
      fbWrite(STIM_LIGHTS_ON,0,0);
      break;
     case CS_STIM_LIGHTS_DIMM:
      qDebug() << "octopus_stimd: Received lights dimm stim command.";
      fbWrite(STIM_LIGHTS_DIMM,0,0);
      break;
     case CS_STIM_LIGHTS_OFF:
      qDebug() << "octopus_stimd: Received lights off stim command.";
      fbWrite(STIM_LIGHTS_OFF,0,0);
      break;
     case CS_STIM_SYNTHETIC_EVENT:
      qDebug() << "octopus_stimd: Received synthetic event.";
      fbWrite(STIM_SYNTH_EVENT,csCmd.iparam[0],0);
      break;
     case CS_STIM_SET_PARAM_P1:
      fbWrite(STIM_SET_PARAM_P1,csCmd.iparam[0],0);
      break;
     case CS_STIM_SET_PARAM_P2:
      fbWrite(STIM_SET_PARAM_P2,csCmd.iparam[0],0);
      break;
     case CS_STIM_SET_PARAM_P3:
      fbWrite(STIM_SET_PARAM_P3,csCmd.iparam[0],0);
      break;
     case CS_STIM_SET_PARAM_P4:
      fbWrite(STIM_SET_PARAM_P4,csCmd.iparam[0],0);
      break;
     case CS_STIM_SET_PARAM_P5:
      fbWrite(STIM_SET_PARAM_P5,csCmd.iparam[0],0);
      break;
     case CS_REBOOT:
      qDebug() << "octopus_stimd: <privileged cmd received> System rebooting..";
      system("/sbin/shutdown -r now"); //commandSocket->close();
      break;
     case CS_SHUTDOWN:
      qDebug() << "octopus_stimd: <privileged cmd received> System shutting down..";
      system("/sbin/shutdown -h now"); //commandSocket->close();
     default:
      break;
    }
    commandSocket->close();
   }
  }

  void slotIncomingData() {
   if ((dataSocket=this->nextPendingConnection()))
    connect(dataSocket,SIGNAL(readyRead()),this,SLOT(readData()));
  }

  void readData() {
   QDataStream dataStream(dataSocket); // Incoming pattern data
//   dataStream.setVersion(QDataStream::Qt_4_0);
   while (dataSocket->bytesAvailable() >= sizeof(patt_datagram)) {
    dataStream.readRawData((char*)(&pattDatagram),sizeof(patt_datagram));
    incomingDataSize+=pattDatagram.size;
//    qDebug("octopus_stimd: MN= 0x%x, %d, %d, %d",
//           pattDatagram.magic_number,
//           pattDatagram.size,
//           incomingDataSize,
//           fileSize);
    // Put the data over SHM transfer buffer..
    for (int i=0;i<(int)pattDatagram.size;i++) shmBuffer[i]=pattDatagram.data[i];

    // Send SYN(count) -- Inform backend
    fbMsg.id=STIM_XFER_SYN; fbMsg.iparam[0]=pattDatagram.size;
    write(fbFifo,&fbMsg,sizeof(fb_command));
    // Wait for ACK
    read(bfFifo,&bfMsg,sizeof(fb_command));

    if (incomingDataSize==fileSize)
     qDebug("octopus_stimd: %d data elements transferred succesfully.",incomingDataSize);
   }
  }

 private:
  QCoreApplication *application;
  int fbFifo,bfFifo; fb_command fbMsg,bfMsg; char *shmBuffer; cs_command csCmd;
  fb_command reset_msg;
  QTcpServer *commandServer; QTcpSocket *commandSocket,*dataSocket;
  QString confHost; int confTcpBufSize,confCommP,confDataP;
  int fileSize,incomingDataSize; patt_datagram pattDatagram;
  bool daemonRunning,clientConnected;
};

#endif
