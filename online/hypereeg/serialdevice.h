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

// Simple serial port definition for use in our Polhemus digitizer class.

#ifndef SERIALDEVICE_H
#define SERIALDEVICE_H

#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

class SerialDevice {
 public:
  SerialDevice() { devIsOpen=false; };
  void init() {
   if (devIsOpen) { ::close(device); devIsOpen=false; }
   devName="/dev/ttyACM0";
   baudRate=B115200;
   dataBits=CS8;
   parity=0;
   parOn=0;
   stopBit=1;
  }

  int open() {
   device=::open(devName.toLatin1().data(),O_RDWR|O_NOCTTY);
   if (device>0) {
    tcgetattr(device,&oldtio); // Save current port settings..
    bzero(&newtio,sizeof(newtio));    // Clear struct for new settings..

    // CLOCAL: Local connection, no modem control
    // CREAD:  Enable receiving chars
    newtio.c_cflag=baudRate|dataBits|parity|parOn|stopBit|CLOCAL|CREAD;
    newtio.c_iflag=IGNPAR|ICRNL; // Ignore bytes with parity errors,
                                 // map CR to NL, as it will terminate the
                                 // input for canonical mode..

    newtio.c_oflag=0;            // Raw Output

    newtio.c_lflag=ICANON;       // Enable Canonical Input

    tcflush(device,TCIFLUSH);    // Clear line and activate settings..
    tcsetattr(device,TCSANOW,&newtio);
    sleep(1);
    return 1;
   } else return -1;
  }

  void close() {
   // Restore old serial context..
   tcsetattr(device,TCSANOW,&oldtio);
   ::close(device);
  }

  void write(unsigned char trigger) {
   unsigned char trig=trigger; ::write(device,&trig,sizeof(unsigned char));
  }

  bool devIsOpen;
 private:
  struct termios oldtio,newtio; // place for old & new serial port settings..
  QString devName; int device,baudRate,dataBits,parity,parOn,stopBit;
};

#endif
