/*      Octopus - Bioelectromagnetic Source Localization System 0.9.9      *
 *                   (>) GPL 2007-2009,2018- Barkin Ilhan                  *
 *            barkin@unrlabs.org, http://www.unrlabs.org/barkin            */

#ifndef ACQDAEMON_H
#define ACQDAEMON_H

#include <QtNetwork>
#include <QThread>
#include <QMutex>
#include <QVector>
#include <unistd.h>
#include "../octopus-acq.h"
#include "../cs_command.h"
#include "../fb_command.h"
#include "acqthread.h"

class AcqDaemon : public QTcpServer {
 Q_OBJECT
 public:
  AcqDaemon(QObject *parent,QCoreApplication *app,
            QString h,int comm,int data,int bs,int tc,int sr,
            int fbf,int bff,float *shmb): QTcpServer(parent) {
   application=app; acqBufSize=bs; totalCount=tc; sampleRate=sr;
   fbFifo=fbf; bfFifo=bff; shmBuffer=shmb; tcpBufSize=2*sampleRate; // 2secs.
   tcpBufIndex=tcpSampleCount=0; tcpPacketSize=1;

   // TCP Buffer for subsequent acq data.. extra one for sampleset checkmark..
   connected=false; tcpBuffer.resize(tcpBufSize*(1+totalCount));

   QHostAddress hostAddress(h);

   // Initialize Tcp Command Server
   commandServer=new QTcpServer(this);
   commandServer->setMaxPendingConnections(1);
   connect(commandServer,SIGNAL(newConnection()),
           this,SLOT(slotIncomingCommand()));
   setMaxPendingConnections(1);

   if (!commandServer->listen(hostAddress,comm) ||
       !listen(hostAddress,data)) {
    qDebug("octopus-acq-daemon: Error starting command and/or data server(s)!");
    application->quit();
   } else {
    qDebug(
     "octopus-acq-daemon: Servers started.. Waiting for client connection..");
   }
  }

  void f2bWrite(unsigned short code,int p0,int p1,int p2,int p3) {
   fbMsg.id=code; fbMsg.iparam[0]=p0; fbMsg.iparam[1]=p1;
   fbMsg.iparam[2]=p2; fbMsg.iparam[3]=p3;
   write(fbFifo,&fbMsg,sizeof(fb_command));
  }

 public slots:
  void slotIncomingCommand() {
   if ((commandSocket=commandServer->nextPendingConnection()))
    connect(commandSocket,SIGNAL(readyRead()),this,SLOT(slotHandleCommand()));
  }

  void slotHandleCommand() {
   QDataStream commandStream(commandSocket);
   if (commandSocket->bytesAvailable() >= sizeof(cs_command)) {
    commandStream.readRawData((char*)(&csCmd),sizeof(cs_command));
    qDebug("octopus-acq-daemon: Command received -> 0x%x.",csCmd.cmd);
    switch (csCmd.cmd) {
     case CS_ACQ_INFO: qDebug(
                        "octopus-acq-daemon: Sending Chn.Count & Rate ..");
                       csCmd.cmd=CS_ACQ_INFO_RESULT;
                       csCmd.iparam[0]=totalCount; csCmd.iparam[1]=sampleRate;
                       commandStream.writeRawData((const char*)(&csCmd),
                                                  sizeof(cs_command));
                       commandSocket->flush(); break;
     case CS_REBOOT:   qDebug("octopus-acq-daemon: System rebooting..");
                       system("/sbin/reboot"); commandSocket->close(); break;
     case CS_SHUTDOWN: qDebug("octopus-acq-daemon: System shutting down..");
                       system("/sbin/halt"); commandSocket->close();
     case CS_ACQ_TRIGTEST:
                       mutex.lock();
                        f2bWrite(ACQ_CMD_F2B,F2B_TRIGTEST,0,0,0);
                       mutex.unlock();
     default:          break;
    }
   }
  }

 protected:
  // Data port "connection handler"..
  void incomingConnection(int socketDescriptor) {
   if (!connected) {
    qDebug("octopus-acq-daemon: Incoming client connection..");
    if (!dataSocket.setSocketDescriptor(socketDescriptor)) {
     qDebug("octopus-acq-daemon: TCP Socket Error!"); connected=false; return;
    } else {
     qDebug("octopus-acq-daemon:  ..accepted.. connecting..");
     connect(&dataSocket,SIGNAL(disconnected()),this,SLOT(slotDisconnected()));
     dataSocket.waitForConnected(); connected=true;
     qDebug("octopus-acq-daemon: Client TCP connection established.");
     // Launch thread responsible for sending acq.data asynchronously..
     outputStream.setDevice(&dataSocket);
     acqThread=new AcqThread(this,&tcpBuffer,&tcpBufIndex,totalCount,sampleRate,
                             fbFifo,bfFifo,shmBuffer,&connected,&mutex);
     // Delete thread if communication breaks..
     connect(acqThread,SIGNAL(finished()),acqThread,SLOT(deleteLater()));
     acqThread->start(QThread::HighestPriority);
    }
   } else qDebug("octopus-acq-daemon:  ..not accepted.. already connected..");
  }

 private slots:
  void slotDisconnected() {
   connected=false; qDebug("octopus-acq-daemon: Client disconnected!");
   disconnect(&dataSocket,SIGNAL(disconnected()),this,SLOT(slotDisconnected()));
  }

  void slotSendData() {
   for (int i=0;i<(totalCount+1);i++)
    outBuffer.append(tcpBuffer[tcpBufIndex*(totalCount+1)+i]);
   if (tcpSampleCount==tcpPacketSize) { tcpSampleCount=0;
    outputStream.writeRawData((const char*)((char *)(outBuffer.data())),
                              outBuffer.size()*sizeof(float));
    dataSocket.flush(); outBuffer.resize(0);
   } tcpSampleCount++; tcpBufIndex++; tcpBufIndex%=tcpBufSize;
  }

 private:
  QCoreApplication *application; AcqThread *acqThread; QMutex mutex;
  QTcpServer *commandServer; QTcpSocket *commandSocket; QTcpSocket dataSocket;
  cs_command csCmd; fb_command fbMsg,bfMsg;
  int fbFifo,bfFifo; float *shmBuffer;
  int tcpBufSize,acqBufSize,totalCount,sampleRate;
  QVector<float> tcpBuffer; bool connected; QDataStream outputStream;
  int tcpSampleCount,tcpBufIndex,tcpPacketSize;
  QVector<float> outBuffer;
};

#endif
