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

/* This is the acquisition 'frontend' daemon of Octopus-ReEL, which has been
   customized to be interfaced with Eemagine commercial EEG amps
   (i.e. assuming two 64-channel "ANTneuro eego mylab" amps connected via USB)
   Hence this acq daemon now receives (N+M+2)-channel data
   (N: Referential EEG channel, M: Bipolar channel, +1 event chn, +1 sample order chn)
   from two separate streams within AcqThread.
   Data as a whole, is forwarded to the connected GUI client, preferably running on
   a remote and dedicated workstation over TCP/IP network.
   Each dataset is prepended with a sync-marker (constant pi)
   to assure integrity.

   The daemons connectivity with the USB-streaming thread is event driven;
   i.e. as soon as the socket connection is up, thread is instantiated and started,
   and the stream is reestablished, and data begins to be fetched from amps.
   Similarly, when the connection is down, streams are disposed of, also
   followed by the disposal of the thread object itself. */

#include <QApplication>
#include <QtCore>
#include <QIntValidator>
#include <QHostInfo>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <cstdint>

#define EEMAGINE

#ifdef EEMAGINE
//#define DIAGNOSTIC
#define _UNICODE
#define EEGO_SDK_BIND_DYNAMIC
#include <eemagine/sdk/factory.h>
#include <eemagine/sdk/wrapper.cc>
#include <eemagine/sdk/amplifier.h>
#endif

//#include <stdio.h>

// Linux headers
//#include <fcntl.h> // Contains file controls like O_RDWR
//#include <errno.h> // Error integer and strerror() function
//#include <termios.h> // Contains POSIX terminal control definitions
//#include <unistd.h> // write(), read(), close()

#include "../chninfo.h"
#include "acqdaemon.h"

#ifdef EEMAGINE

#ifdef DIAGNOSTIC
std::vector<double> getSingleImpedance(eemagine::sdk::amplifier* amp, std::vector<eemagine::sdk::channel> chnList) {
 using namespace eemagine::sdk;
 std::vector<double> impData; buffer impBuf;
 //stream* impStream=amp->OpenImpedanceStream(0xffffffffffffffff);
 stream* impStream=amp->OpenImpedanceStream(chnList);
 //std::this_thread::sleep_for(std::chrono::milliseconds(5000));
 try {
  impBuf=impStream->getData();
 } catch (const exceptions::internalError& e) {
  std::cout << "Exception:" << e.what() << std::endl;
  delete impStream;
  return impData;
 }
 delete impStream;
 for (quint64 i=0;i<impBuf.getChannelCount();i++) impData.push_back(impBuf.getSample(i,0));
 return impData;
}

std::vector<double> getSingleEeg(eemagine::sdk::amplifier* amp, std::vector<eemagine::sdk::channel> chnList) {
 using namespace eemagine::sdk;
 std::vector<double> eegData; buffer eegBuf;
 stream* eegStream=amp->OpenEegStream(500,1.,.7,chnList);
 //std::this_thread::sleep_for(std::chrono::milliseconds(100));
 try {
  eegBuf=eegStream->getData();
 } catch (const exceptions::internalError& e) {
  std::cout << "Exception:" << e.what() << std::endl;
  delete eegStream;
  return eegData;
 }
 delete eegStream;
 for (quint64 i=0;i<eegBuf.getChannelCount();i++) eegData.push_back(eegBuf.getSample(i,0));
 return eegData;
}
#endif

int initEEGOConnectivity(chninfo* chnInfo) {
 using namespace eemagine::sdk;
 factory fact("libeego-SDK.so");

#ifdef DIAGNOSTIC
 std::string elecNames[chnInfo->refChnMaxCount+chnInfo->bipChnMaxCount+2]={
  "Fp1","Fpz","Fp2","F7","F3","Fz","F4","F8",
  "FC5","FC1","FC2","FC6","M1","T7","C3","Cz",
  "C4","T8","M2","CP5","CP1","CP2","CP6","P7",
  "P3","Pz","P4","P8","POz","O1","O2","EOG",
  "AF7","AF3","AF4","AF8","F5","F1","F2","F6",
  "FC3","FCz","FC4","C5","C1","C2","C6","CP3",
  "CP4","P5","P1","P2","P6","PO5","PO3","PO4",
  "PO6","FT7","FT8","TP7","TP8","PO7","PO8","Oz",
  "BP1","BP2","BP3","BP4","BP5","BP6","BP7","BP8",
  "BP9","BP10","BP11","BP12","BP13","BP14","BP15","BP16",
  "BP17","BP18","BP19","BP20","BP21","BP22","BP23","BP24",
  "TRIG","ORDER"
 };
#endif

 std::vector<amplifier*> amplifiers; amplifier* amp;
 std::vector<std::vector<channel> > chnLists; 
 struct amplifier::power_state powerState; std::string powerStateStr;
 
 std::vector<int> serialNos,sRates; std::vector<double> refRanges,bipRanges;

 amplifiers=fact.getAmplifiers();

 if (amplifiers.size()!=2) return -1; // Cannot communicate with  both amplifiers

 for (quint64 i=0;i<amplifiers.size();i++)
  serialNos.push_back(stoi(amplifiers[i]->getSerialNumber()));

 // Sort the two amplifiers in vector for their serial numbers
 if (serialNos[0]>serialNos[1]) {
  amp=amplifiers[1]; amplifiers[1]=amplifiers[0]; amplifiers[0]=amp;
 }

 for (quint64 i=0;i<amplifiers.size();i++) {
  std::cout << "AMPLIFIER " << i+1 << ":" << std::endl;
  amp=amplifiers[i];
  powerState=amp->getPowerState();
  if (powerState.is_charging) powerStateStr="Charging ("; else powerStateStr="Not charging (";
  // ---
  std::cout << "Serial: " << amp->getSerialNumber() << " -- ";
  std::cout << "Firmware: " << amp->getFirmwareVersion() << " -- ";
  std::cout << "Model: " << amp->getType() << " -- ";
  std::cout << "Battery: " << powerStateStr << powerState.is_powered << " " << powerState.charging_level << "%)" << std::endl;
  // ---
  sRates=amp->getSamplingRatesAvailable();
  std::cout << "Possible samplerates (sps): ";
  for (quint64 i=0;i<sRates.size();i++) std::cout << sRates[i] << " ";
  std::cout << std::endl;
  // ---
  refRanges=amp->getReferenceRangesAvailable();
  std::cout << "Possible reference ranges (V): ";
  for (quint64 i=0;i<refRanges.size();i++) std::cout << refRanges[i] << " ";
  std::cout << " -- ";
  // ---
  bipRanges=amp->getBipolarRangesAvailable();
  std::cout << "Possible bipolar ranges (V): ";
  for (quint64 i=0;i<bipRanges.size();i++) std::cout << bipRanges[i] << " ";
  // ---
  std::vector<eemagine::sdk::channel> c=amp->getChannelList(0xffffffffffffffff,0x0000000000ffffff);
  chnLists.push_back(c);

  std::cout << std::endl << std::endl;
 }

#ifdef DIAGNOSTIC
 std::vector<double> impData,eegData;

 for (quint64 i=0;i<amplifiers.size();i++) {
  impData=getSingleImpedance(amplifiers[i],chnLists[i]);
  // ---
  std::cout << "AMPLIFIER " << i+1 << " IMPEDANCES (" << chnInfo->refChnMaxCount << " channels, kOhm):" << std::endl;
  for (quint64 j=0;j<chnInfo->refChnMaxCount;j++) {
   std::cout << elecNames[j] << ":" << impData[j]/1e3 << " ";
   if ((j+1)%8==0) std::cout << std::endl;
  }
  std::cout << std::endl;
 }

 for (quint64 i=0;i<amplifiers.size();i++) {
  eegData=getSingleEeg(amplifiers[i],chnLists[i]);
  // ---
  std::cout << "AMPLIFIER " << i+1 << " EEG SNAPSHOT (" << eegData.size() << " channels, uV):" << std::endl;
  for (quint64 j=0;j<eegData.size();j++) {
   if (j>=70) std::cout << elecNames[j] << ":" << eegData[j] << " ";
   else std::cout << elecNames[j] << ":" << eegData[j]*1e6 << " ";
   if ((j+1)%8==0) std::cout << std::endl;
   if (j==63 || j==87) std::cout << "-------" << std::endl;
  }
  std::cout << std::endl << std::endl;
 }
 std::cout << std::endl;
#endif

 for (quint64 i=0;i<amplifiers.size();i++) delete amplifiers[i];

 return 0;
}

#endif

int main(int argc,char *argv[]) {
 QString host,comm,data,dStr1,dStr2; int commP,dataP;

 QCoreApplication app(argc,argv);

 chninfo chnInfo;
 chnInfo.sampleRate=1000;
 chnInfo.refChnCount=refChnCount; chnInfo.refChnMaxCount=64;
 chnInfo.bipChnCount=bipChnCount; chnInfo.bipChnMaxCount=24;
 chnInfo.physChnCount=physChnCount; chnInfo.totalChnCount=totalChnCount;
 chnInfo.totalCount=2*totalChnCount;
 // TODO. Trigger should be syncronized and common
 chnInfo.probe_msecs=100; // 100ms probetime

/*
 // Open Prolific 2303
 int serialDev;
 unsigned char synctrig=0xff;
 struct termios tty;

 serialDev=open("/dev/ttyUSB0",O_RDWR);
 if (serialDev<0) {
  qDebug() << "USB Serial device cannot be opened.";
  return -1;
 }

 tty.c_cflag |= CS8;     // 8 bits per byte
 tty.c_cflag &= ~PARENB; // N No parity
 tty.c_cflag &= ~CSTOPB; // 1 stop bit
 tty.c_cflag &= ~CSIZE;   // Clear all bits that set the data size
 tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
 tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)
 tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl

 tty.c_lflag &= ~ICANON;
 tty.c_lflag &= ~ECHO; // Disable echo
 tty.c_lflag &= ~ECHOE; // Disable erasure
 tty.c_lflag &= ~ECHONL; // Disable new-line echo
 tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
 tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes

 tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
 tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
 // tty.c_oflag &= ~OXTABS; // Prevent conversion of tabs to spaces (NOT PRESENT ON LINUX)
 // tty.c_oflag &= ~ONOEOT; // Prevent removal of C-d chars (0x004) in output (NOT PRESENT ON LINUX)

 tty.c_cc[VTIME] = 10;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
 tty.c_cc[VMIN] = 0;

  // Set in/out baud rate to be 9600
 cfsetispeed(&tty, B9600);
 cfsetospeed(&tty, B9600);

 // Save tty settings, also checking for error
 if (tcsetattr(serialDev, TCSANOW, &tty) != 0) {
  qDebug() << "Error from tcsetattr";
  return 1;
 }
 for (int i=0;i<3000;i++) {
  write(serialDev,&synctrig,sizeof(unsigned char));
  sleep(1);
 }
 close(serialDev);
*/

#ifdef EEMAGINE
 // Initialize EEGO
 if (initEEGOConnectivity(&chnInfo)) {
  qDebug("octopus-acq-daemon-eemagine: Cannot startup (communication problems with the TWO EEGO amplifiers!)");
  return -1;
 }
#endif

 // *** COMMAND LINE VALIDATION ***

 if (argc!=4) {
  qDebug("Usage: octopus-acq-daemon-eemagine <hostname|ip> <comm port> <data port>");
  return -1;
 }

 //QHostInfo qhi=QHostInfo::fromName(QString::fromLatin1(argv[1]));
 //QHostInfo qhi=QHostInfo::lookupHost(QString::fromLatin1(argv[1]),&app,SLOT(lookedUp(QHostInfo)));
 //host=qhi.addresses().first().toString();
 host=QString::fromLatin1(argv[1]);
 comm=QString::fromLatin1(argv[2]);
 data=QString::fromLatin1(argv[3]);
 QIntValidator intValidator(1024,65535,NULL); int pos=0;
 if (intValidator.validate(comm,pos) != QValidator::Acceptable ||
     intValidator.validate(data,pos) != QValidator::Acceptable) {
  qDebug("Please enter a positive integer between 1024 and 65535 as ports.");
  qDebug("Usage: octopus-acq-daemon-eemagine <hostname|ip> <comm port> <data port>");
  return -1;
 }

 commP=comm.toInt(); dataP=data.toInt();
 if (commP==dataP) {
  qDebug("Communication and Data ports should be different.");
  qDebug("Usage: octopus-acq-daemon-eemagine <hostname|ip> <comm port> <data port>");
  return -1;
 }

 // Validation of also the hostname/ip would be better here..

 // *** BACKEND VALIDATION ***

 qDebug("Datahandling Info:\n");
 qDebug() << "Per-amp Physical Channel#: " << chnInfo.physChnCount << "(" \
	  << chnInfo.refChnCount << "+" << chnInfo.bipChnCount << ")";
 qDebug() << "Per-amp Total Channel# (with Trig and Offset): " << chnInfo.totalChnCount;
 qDebug() << "Total Channel# from all amps: " << chnInfo.totalCount;

 AcqDaemon acqDaemon(0,&app,host,commP,dataP,&chnInfo);
 return app.exec();
}

