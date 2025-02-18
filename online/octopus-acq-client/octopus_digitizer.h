/*
Octopus-ReEL - Realtime Encephalography Laboratory Network
   Copyright (C) 2007 Barkin Ilhan

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If no:t, see <https://www.gnu.org/licenses/>.

 Contact info:
 E-Mail:  barkin@unrlabs.org
 Website: http://icon.unrlabs.org/staff/barkin/
 Repo:    https://github.com/4e0n/
*/

#ifndef OCTOPUS_DIGITIZER_H
#define OCTOPUS_DIGITIZER_H

#include <QThread>
#include <QMutex>
#include <QStringList>
#include <QSound>

#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <cmath>

#include "serial_device.h"
#include "../../common/vec3.h"

const int AVG_N=100;
const int SCR_REFRESH=5;

class Digitizer; static Digitizer *dst;
class Digitizer : public QThread {
 Q_OBJECT
 public:
  Digitizer(QObject *p,serial_device *s) : QThread(p) {
   dst=this; parent=p; serial=s; connected=averaging=false; scrUpdate=0;
   connect(this,SIGNAL(beep(int)),this,SLOT(slotBeep(int)));
  }

  void serialOpen() { // Open the serial device..
   QString result;
   if ((device=open(serial->devname.toLatin1().data(),O_RDWR|O_NOCTTY))>0) {
    tcgetattr(device,&oldtio);     // Save current port settings..
    bzero(&newtio,sizeof(newtio)); // Clear struct for new settings..

    // CLOCAL: Local connection, no modem control
    // CREAD:  Enable receiving chars
    newtio.c_cflag=serial->baudrate | serial->databits |
                   serial->parity   | serial->par_on   |
                   serial->stopbit  | CLOCAL |CREAD;

    newtio.c_iflag=IGNPAR | ICRNL;  // Ignore bytes with parity errors,
                                    // map CR to NL, as it will terminate the
                                    // input for canonical mode..

    newtio.c_oflag=0;               // Raw Output

    newtio.c_lflag=ICANON;          // Enable Canonical Input

    tcflush(device,TCIFLUSH); // Clear line and activate settings..
    tcsetattr(device,TCSANOW,&newtio); sleep(1);

    // Init Digitizer
//    digitizerSend(QString(25)); sleep(7); // Reset (Ctrl-Y) & Get Status..
    digitizerSend("S"); msleep(100); result=digitizerGetLine();
    QStringList k=result.split(" ",QString::SkipEmptyParts);
    if (k.size()>=3) {
     qDebug("Digitizer status:\n%s",result.toLatin1().data());
     digitizerSend("u"); msleep(100);   // Metric system..
     digitizerSend("c"); msleep(100);      // Single coord mode..
     digitizerSend("e1,0\n"); msleep(100); // Mouse Mode..
     //  - Receive X,Y,Z's of all 4 receivers on a single line..
     digitizerSend("O1,16,2,0\n"); msleep(100);
     digitizerSend("O2,2,0\n"); msleep(100);
     digitizerSend("O3,2,0\n"); msleep(100);
     digitizerSend("O4,2,0,1\n"); msleep(100);
     // Define mid-tip of the receivers as origin..
     digitizerSend("N2,0.301,0,0.313\n"); msleep(200);
     digitizerSend("N3,0.301,0,0.313\n"); msleep(200);
     digitizerSend("N4,0.301,0,0.313\n"); msleep(200);

     // Turn transmitter frame 180deg upside down for all receivers about X..
     digitizerSend("r1,0,0,180\n"); msleep(200);
     digitizerSend("r2,0,0,180\n"); msleep(200);
     digitizerSend("r3,0,0,180\n"); msleep(200);
     digitizerSend("r4,0,0,180\n"); msleep(200);
//     msleep(100); tcflush(device,TCIFLUSH); msleep(100);

     start(QThread::LowestPriority); connected=true; msleep(200);
     digitizerSend("C"); // Metric system, single mode..
    } else { qDebug("Error in digitizer status format!"); return; }
   }
  }

  void serialClose() {
   digitizerSend("c"); wait(500); terminate(); wait(500);
   tcsetattr(device,TCSANOW,&oldtio); // Restore old serial context..
   close(device);
  }

  void digitizerSend(QString command) {
   write(device,command.toLatin1().data(),command.size());
  }

  QString digitizerGetLine() {
   QString line; for (int i=0;i<250;i++) c[i]=0; // Clear string buffer
   read(device,c,249); read(device,cc,1); // Handle extra CR at the end..
   line=c; return line.trimmed();
  } 

  void digitizerReceive() { // Synchronous (canonical) receive
   QStringList c; QString receivedLine=digitizerGetLine();
   c=receivedLine.split(" ",QString::SkipEmptyParts);
   if (c.size()==17) {
    r0[0]=c[ 2].toFloat(); r0[1]=c[ 3].toFloat(); r0[2]=c[ 4].toFloat();
    r1[0]=c[ 6].toFloat(); r1[1]=c[ 7].toFloat(); r1[2]=c[ 8].toFloat();
    r2[0]=c[10].toFloat(); r2[1]=c[11].toFloat(); r2[2]=c[12].toFloat();
    r3[0]=c[14].toFloat(); r3[1]=c[15].toFloat(); r3[2]=c[16].toFloat();
    hc=Vec3((r2[0]+r3[0])/2.,(r2[1]+r3[1])/2.,(r2[2]+r3[2])/2.); // Center
    r0=r0-hc; r1=r1-hc; r2=r2-hc; r3=r3-hc; // Translation to Center
    sty=r0; yp=r1; yp.normalize(); xp=r3-r2; xp.normalize(); // Cap frame
    zp=xp; zp.cross(yp); zp.normalize();
    // Be sure that Y's point to same direction.. (Rot I)
    angleZ=-yp.sphPhi(); angleY=M_PI/2.-yp.sphTheta();
    sty.rotZ(angleZ); sty.rotY(angleY); sty.rotZ(M_PI/2.);
    yp.rotZ(angleZ); yp.rotY(angleY); yp.rotZ(M_PI/2.);
    zp.rotZ(angleZ); zp.rotY(angleY); zp.rotZ(M_PI/2.);
    xp=yp; xp.cross(zp); xp.normalize(); // Recompute orthogonal xp..
    angleY=M_PI/2.-xp.sphTheta();
    sty.rotY(angleY); xp.rotY(angleY); zp.rotY(angleY);
    xp.normalize(); yp.normalize(); zp.normalize();
    // Translation of frame (but not sty) to forward for visualization
    Vec3 trans(1.,1.,1.); yp=yp+trans; xp=xp+trans; zp=zp+trans;

    if (!scrUpdate) {
     mutex.lock(); styF=sty; xpF=xp; ypF=yp; zpF=zp; mutex.unlock();
     emit digMonitor();
    } scrUpdate++; scrUpdate%=SCR_REFRESH;

    if (!averaging) {
     if (c[1].toInt()==1) {
      emit beep(1); averaging=true; avgN=0; stylus.zero(); stylusS.zero();
      qDebug("Taking average of %d coords.. please do not move stylus.",AVG_N);
     }
    } else { // Currently averaging..
     avgN++; stylus=stylus+sty;
     stylusS=stylusS+Vec3(sty[0]*sty[0],sty[1]*sty[1],sty[2]*sty[2]);

     if (avgN==AVG_N) { averaging=false;
      stylus=stylus*(1./AVG_N); stylusS=stylusS*(1./AVG_N);
      stylusS=Vec3(std::fabs(stylusS[0]-stylus[0]*stylus[0]),
                   std::fabs(stylusS[1]-stylus[1]*stylus[1]),
                   std::fabs(stylusS[2]-stylus[2]*stylus[2]));
      mutex2.lock(); stylusF=stylus; stylusSF=stylusS; mutex2.unlock();
      emit digResult(); emit beep(0);
     }
    }
   } else {
    qDebug("Error in received coord line.. It is:\n %d %s\n",c.size(),
           receivedLine.toLatin1().data());
   }
  }

  virtual void run() { while (true) { digitizerReceive(); } }

  bool connected; QMutex mutex,mutex2; Vec3 styF,xpF,ypF,zpF,stylusF,stylusSF;

 signals:
  void digMonitor(); void digResult(); void beep(int);

 private slots:
  void slotBeep(int x) {
   if (x==1) QSound::play("../../octopus-data/SOUND/start.wav");
   else if (x==0) QSound::play("../../octopus-data/SOUND/stop.wav");
  }

 private:
  QObject *parent; serial_device *serial; int device; // Actual serial dev..
  struct termios oldtio,newtio; // place for old & new serial port settings..
  QString *resultString; char c[250],cc[2]; bool averaging;
  int avgN,scrUpdate; float angleY,angleZ;
  Vec3 r0,r1,r2,r3,hc,sty,xp,yp,zp,stylus,stylusS;
};

#endif
