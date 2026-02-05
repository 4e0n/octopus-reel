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

// Simple serial port definition for use in our Polhemus digitizer class.
// Then modified to be used to handle SparkFun Pro Micro in HyperEEG box.

#pragma once

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <poll.h>
#include <cerrno>

#include <QDebug>

class SerialDevice {
 public:
  SerialDevice() {};
  bool init() {
   devName="/dev/ttyACM0";
   baudRate=B115200;
   if (open()<0) return true;
   return false;
  }

  int open() {
   device=::open(devName.toLatin1().data(),O_RDWR|O_NOCTTY|O_NONBLOCK|O_CLOEXEC);
   if (device<0) { qCritical() << "node-acq: <SerialDevice><Open> Cannot open device!"; return -1; }

   ioctl(device,TIOCEXCL); // Optional: take exclusive access so ModemManager/others can’t reopen

   if (tcgetattr(device,&oldtio)!=0) { qCritical() << "node-acq: <SerialDevice><Open> Cannot get/save tio attributes!"; return -1; }
   bzero(&newtio,sizeof(newtio)); // New empty one..
   cfmakeraw(&newtio); // Fetch, make raw, and lock in 8N1+noflowctrl+nohup-on-close
   cfsetispeed(&newtio,baudRate); cfsetospeed(&newtio,baudRate);
   // Ensure receiver enabled, ignore modem control, 8 data bits
   // no HW flow ctrl, no hangup-on-close (prevents auto-reset), 8N1 default OK
   newtio.c_cflag|=(CLOCAL|CREAD|CS8); newtio.c_cflag&=~(PARENB|CSTOPB|CSIZE|CRTSCTS|HUPCL);
   newtio.c_iflag&=~(IXON|IXOFF|IXANY); // No swflowctrl (defensive; cfmakeraw should already clear)
   newtio.c_iflag=IGNPAR|ICRNL; // Ignore bytes with parity errors,
                                // map CR to NL, as it will terminate the
                                // input for canonical mode..
   newtio.c_oflag=0;            // Raw Output
   newtio.c_lflag=ICANON;       // Enable Canonical Input
   newtio.c_cc[VMIN]=0; newtio.c_cc[VTIME] = 0; // Nonblocking semantics come from open(); keep VMIN/VTIME at 0
   // Apply now; TCSAFLUSH discards queued I/O in the driver
   if (tcsetattr(device,TCSAFLUSH,&newtio)!=0) { /* handle error */ }
   //tcsetattr(device,TCSANOW,&newtio);
   // Assert DTR/RTS once and leave them set
   int m=TIOCM_DTR|TIOCM_RTS; if (ioctl(device,TIOCMBIS,&m)==-1) { /* perror or log; some stacks ignore */ }
   tcflush(device,TCIOFLUSH); // Drop any stale bytes (modem probes, boot spew)

   // --- Readiness handshake (up to ~2s total) ---
   std::string line;
//   auto until_ms=[](int ms){ return ms; }; // readability

   int elapsed=0;
   while (elapsed<2000) {
    struct pollfd p{device,POLLIN,0};
    int r=poll(&p,1,50); // 50 ms slices
    elapsed+=50;
    if (r>0 && (p.revents&POLLIN)) {
     char buf[128];
     ssize_t n=read(device,buf,sizeof(buf));
     if (n>0) {
      line.append(buf,buf+n);
      // normalize CRLF -> LF
      for (char& c:line) if (c=='\r') c='\n';
      auto pos=line.find('\n');
      if (pos!=std::string::npos) {
       std::string ident=line.substr(0,pos);
       // keep remainder (if any) for later reads
       line.erase(0, pos+1);
       if (ident.rfind("OCTOPUS-HyperEEG",0)==0 &&
           ident.find("role=SYNC") != std::string::npos &&
           ident.find("sig=OCTO-EEG-42") != std::string::npos) {
        break; // READY
       } else {
        qCritical() << "node-acq: <SerialDevice><Open> Device is different than ours.";
        // Not our device; consider closing and trying next tty
        close(); return -1;
       }
      }
     }
    }
    // else timeout slice; loop continues
   }
   // fd stays O_NONBLOCK; fine for write+optional-ACK pattern.
   sleep(1);
   return 0;
  }

  void close() {
   // Restore old serial context..
   tcsetattr(device,TCSANOW,&oldtio);
   ::close(device);
  }

  void write(unsigned char trigger) {
   unsigned char trig=trigger; ::write(device,&trig,sizeof(unsigned char));
  }

  //bool sendTrigger(uint8_t trig) {
  // if (device<0) return false;
  // ssize_t w=write(device,&trig,1);
  // if (w!=1) return false;
  // // Optional: egress timestamp bound (costs a syscall)
  // tcdrain(device);
  // clock_gettime(CLOCK_MONOTONIC_RAW, &ts_);
  // last_egress_ns_ = uint64_t(ts_.tv_sec)*1000000000ull + ts_.tv_nsec;
  // return true;
  //}

 private:
  termios oldtio{},newtio{}; // old & new serial port settings..
  QString devName; int device,baudRate,dataBits,parity,parOn,stopBit;
};
