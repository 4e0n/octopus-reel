/*      Octopus - Bioelectromagnetic Source Localization System 0.9.9      *
 *                   (>) GPL 2007-2009,2018- Barkin Ilhan                  *
 *            barkin@unrlabs.org, http://www.unrlabs.org/barkin            */

#ifndef ACQTHREAD_H
#define ACQTHREAD_H

#include <QThread>
#include <QMutex>
#include <QtNetwork>
#include <unistd.h>
#include "../octopus-acq.h"
#include "../fb_command.h"

class AcqThread : public QThread {
 Q_OBJECT
 public:
  AcqThread(QObject* parent,QVector<float> *tb,int *tbi,int tc,
            int sr,int fbf,int bff,float *shmb,
            bool *c,QMutex *m) : QThread(parent) {
   mutex=m; tcpBuffer=tb; tcpBufIndex=tbi;
   totalCount=tc; sampleRate=sr;
   fbFifo=fbf; bfFifo=bff; shmBuffer=shmb; connected=c;
   connect(this,SIGNAL(sendData()),parent,SLOT(slotSendData()));
  }

  void f2bWrite(unsigned short code,int p0,int p1,int p2,int p3) {
   fbMsg.id=code; fbMsg.iparam[0]=p0; fbMsg.iparam[1]=p1;
   fbMsg.iparam[2]=p2; fbMsg.iparam[3]=p3;
   write(fbFifo,&fbMsg,sizeof(fb_command));
  }

  // Get buffer size from backend..
  int f2bCommand(int command) {
   fbMsg.id=ACQ_CMD_F2B;
   fbMsg.iparam[0]=command; fbMsg.iparam[1]=fbMsg.iparam[2]=fbMsg.iparam[3]=0;
   write(fbFifo,&fbMsg,sizeof(fb_command)); sleep(1);
   read(bfFifo,&bfMsg,sizeof(fb_command)); // result..
   if (!(bfMsg.id==ACQ_CMD_B2F)) {
    qDebug("octopus-acq-daemon: Kernel-space backend does not respond!");
    return -100;
   } return bfMsg.iparam[0];
  }

  virtual void run() {
   qDebug("octopus-acq-daemon: Establishing EEG upstream..");
   mutex->lock(); f2bWrite(ACQ_START,0,0,0,0); mutex->unlock();
   while (*connected) {
    read(bfFifo,&bfMsg,sizeof(fb_command));
    if (bfMsg.id==ACQ_CMD_B2F) {
     if (bfMsg.iparam[0]==B2F_DATA_SYN) {
      shmBufIndex=bfMsg.iparam[1];
      tcpBaseX=(*tcpBufIndex)*(totalCount+1);
       (*tcpBuffer)[tcpBaseX]=3.141516; // Marker
       for (int i=0;i<totalCount;i++) // Get all instantaneous data from mem..
        (*tcpBuffer)[tcpBaseX+1+i]=shmBuffer[shmBufIndex*totalCount+i];
      // Tell the backend (ACK) that the data at index is received.
      mutex->lock();
       f2bWrite(ACQ_CMD_F2B,F2B_DATA_ACK,shmBufIndex,bfMsg.iparam[2],0);
      mutex->unlock();
      emit sendData();
     } else
      qDebug(
       "octopus-acq-daemon: Irrelevant message from backend instead of SYN!");
    }
   } // while

   // Deactivate acquisition in backend..
   mutex->lock(); f2bWrite(ACQ_STOP,0,0,0,0); mutex->unlock();
   // B2F fifo send and flush net.
   if(f2bCommand(F2B_RESET_SYN)==B2F_RESET_ACK)
    qDebug("octopus-acq-daemon: Backend reset.");
   else qDebug("octopus-acq-daemon: There's a problem resetting the backend.");
   qDebug("octopus-acq-daemon: ..exiting TCP handler thread..");
  }

 signals:
  void sendData();

 private:
  bool *connected; QVector<float> *tcpBuffer; QMutex *mutex;
  int  shmBufIndex,totalCount,sampleRate;
  fb_command fbMsg,bfMsg; int fbFifo,bfFifo; float *shmBuffer;
  int tcpBaseX; // Base offset of current datagram
  int *tcpBufIndex;
};

#endif
