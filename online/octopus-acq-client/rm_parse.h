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

void parseConfig(QStringList cfgLines) {
 QStringList cfgValidLines,opts,opts2,opts3,
             bufSection,netSection,avgSection,evtSection,
             chnSection,digSection,guiSection,modSection;

 for (int i=0;i<cfgLines.size();i++) { // Isolate valid lines
  if (!(cfgLines[i].at(0)=='#') &&
      cfgLines[i].contains('|')) cfgValidLines.append(cfgLines[i]); }

 // *** CATEGORIZE SECTIONS ***

 initSuccess=true;
 for (int i=0;i<cfgValidLines.size();i++) {
  opts=cfgValidLines[i].split("|");
       if (opts[0].trimmed()=="BUF") bufSection.append(opts[1]);
  else if (opts[0].trimmed()=="NET") netSection.append(opts[1]);
  else if (opts[0].trimmed()=="AVG") avgSection.append(opts[1]);
  else if (opts[0].trimmed()=="EVT") evtSection.append(opts[1]);
  else if (opts[0].trimmed()=="CHN") chnSection.append(opts[1]);
  else if (opts[0].trimmed()=="DIG") digSection.append(opts[1]);
  else if (opts[0].trimmed()=="GUI") guiSection.append(opts[1]);
  else if (opts[0].trimmed()=="MOD") modSection.append(opts[1]);
  else { qDebug("Unknown section in .cfg file!"); initSuccess=false; break; }
 } if (!initSuccess) return;

 // BUF
 if (bufSection.size()>0) {
  for (int i=0;i<bufSection.size();i++) { opts=bufSection[i].split("=");
   if (opts[0].trimmed()=="PAST") { cp.cntPastSize=opts[1].toInt();
    if (!(cp.cntPastSize >= 1000 && cp.cntPastSize <= 20000)) {
     qDebug(".cfg: BUF|PAST not within inclusive (1000,20000) range!");
     initSuccess=false; break;
    }
   }
  }
 } else { cp.cntPastSize=5000; }
 if (!initSuccess) return;

 // NET
 if (netSection.size()>0) {
  for (int i=0;i<netSection.size();i++) { opts=netSection[i].split("=");
   if (opts[0].trimmed()=="STIM") { opts2=opts[1].split(",");
    if (opts2.size()==3) { stimHost=opts2[0].trimmed();
     QHostInfo qhiStim=QHostInfo::fromName(stimHost);
     stimHost=qhiStim.addresses().first().toString();
     qDebug() << "StimHost:" << stimHost;

     stimCommPort=opts2[1].toInt(); stimDataPort=opts2[2].toInt();

     // Simple port validation..
     if ((!(stimCommPort >= 1024 && stimCommPort <= 65535)) ||
         (!(stimDataPort >= 1024 && stimDataPort <= 65535))) {
      qDebug(".cfg: Error in STIM IP and/or port settings!");
      initSuccess=false; break;
     }
    } else {
     qDebug(".cfg: Parse error in STIM IP (v4) Address!");
     initSuccess=false; break;
    }
   } else if (opts[0].trimmed()=="ACQ") { opts2=opts[1].split(",");
    if (opts2.size()==3) { acqHost=opts2[0].trimmed();
     QHostInfo qhiAcq=QHostInfo::fromName(acqHost);
     acqHost=qhiAcq.addresses().first().toString();
     qDebug() << "AcqHost:" << acqHost;

     acqCommPort=opts2[1].toInt(); acqDataPort=opts2[2].toInt();

     // Simple port validation..
     if ((!(acqCommPort >= 1024 && acqCommPort <= 65535)) ||
         (!(acqDataPort >= 1024 && acqDataPort <= 65535))) {
      qDebug(".cfg: Error in ACQ IP and/or port settings!");
      initSuccess=false; break;
     }
    }
   } else {
    qDebug(".cfg: Parse error in ACQ IP (v4) Address!");
    initSuccess=false; break;
   }
  }
 } else {
  stimHost="127.0.0.1"; stimCommPort=65000; stimDataPort=65001;
  acqHost="127.0.0.1";  acqCommPort=65002;  acqDataPort=65003;
 } if (!initSuccess) return;

 // Connect to ACQ Server and get crucial info.
 acqCommandSocket->connectToHost(acqHost,acqCommPort);
 acqCommandSocket->waitForConnected();
 QDataStream acqCommandStream(acqCommandSocket);
 csCmd.cmd=CS_ACQ_INFO;
 acqCommandStream.writeRawData((const char*)(&csCmd),sizeof(cs_command));
 acqCommandSocket->flush();
 if (!acqCommandSocket->waitForReadyRead()) {
  qDebug(".cfg: ACQ server did not return crucial info!");
  initSuccess=false; return;
 }
 acqCommandStream.readRawData((char*)(&csCmd),sizeof(cs_command));
 acqCommandSocket->disconnectFromHost();
 if (csCmd.cmd!=CS_ACQ_INFO_RESULT) {
  qDebug(".cfg: ACQ server returned nonsense crucial info!");
  initSuccess=false; return;
 }

 sampleRate=chnInfo.sampleRate=csCmd.iparam[0];
 chnInfo.refChnCount=csCmd.iparam[1];
 chnInfo.bipChnCount=csCmd.iparam[2];
 chnInfo.physChnCount=csCmd.iparam[3];
 chnInfo.refChnMaxCount=csCmd.iparam[4];
 chnInfo.bipChnMaxCount=csCmd.iparam[5];
 chnInfo.physChnMaxCount=csCmd.iparam[6];
 tChns=chnInfo.totalChnCount=csCmd.iparam[7];
 chnInfo.totalCount=csCmd.iparam[8];
 chnInfo.probe_eeg_msecs=csCmd.iparam[9];

 acqCurData.resize(chnInfo.probe_eeg_msecs);

 //for (int i=0;i<2*chnInfo.physChnCount;i++) { calM.append(1.); calN.append(0.); }
 //calA.resize(2*chnInfo.physChnCount); calB.resize(2*chnInfo.physChnCount);
 //calDC.resize(2*chnInfo.physChnCount); calSin.resize(2*chnInfo.physChnCount);

 qDebug(
  ".cfg: ACQ server returned: Total Phys Chn#=%d, Samplerate=%d",2*chnInfo.physChnCount,sampleRate);

 // AVG
 if (avgSection.size()>0) {
  for (int i=0;i<avgSection.size();i++) { opts=avgSection[i].split("=");
   if (opts[0].trimmed()=="INTERVAL") { opts2=opts[1].split(",");
    if (opts2.size()==4) {
     cp.rejBwd=opts2[0].toInt(); cp.avgBwd=opts2[1].toInt();
     cp.avgFwd=opts2[2].toInt(); cp.rejFwd=opts2[3].toInt();
     if ((!(cp.rejBwd >= -1000 && cp.rejBwd <=    0)) ||
         (!(cp.avgBwd >= -1000 && cp.avgBwd <=    0)) ||
         (!(cp.avgFwd >=     0 && cp.avgFwd <= 1000)) ||
         (!(cp.rejFwd >=     0 && cp.rejFwd <= 1000)) ||
         (cp.rejBwd>cp.avgBwd) ||
         (cp.avgBwd>cp.avgFwd) || (cp.avgFwd>cp.rejFwd)) {
      qDebug(".cfg: Logic error in AVG|INTERVAL parameters!");
      initSuccess=false; break;
     } else {
      cp.avgCount=(cp.avgFwd-cp.avgBwd)*sampleRate/1000;
      cp.rejCount=(cp.rejFwd-cp.rejBwd)*sampleRate/1000;
      cp.postRejCount=(cp.rejFwd-cp.avgFwd)*sampleRate/1000;
      cp.bwCount=cp.rejFwd*sampleRate/1000;
     }
    } else {
     qDebug(".cfg: Parse error in AVG|INTERNAL parameters!");
     initSuccess=false; break;
    }
   }
  }
 } else {
  cp.rejBwd=-300; cp.avgBwd=-200; cp.avgFwd=500; cp.rejFwd=600;
  cp.avgCount=700*sampleRate/1000; cp.rejCount=900*sampleRate/1000;
  cp.postRejCount=100*sampleRate/1000; cp.bwCount=600*sampleRate/1000;
 } if (!initSuccess) return;

 if (evtSection.size()>0) {
  for (int i=0;i<evtSection.size();i++) { opts=evtSection[i].split("=");
   if (opts[0].trimmed()=="STIM") { opts2=opts[1].split(",");
    if (opts2.size()==5) {
     if ((!(opts2[0].toInt()>0  && opts2[0].toInt()<128)) ||
         (!(opts2[1].size()>0)) ||
         (!(opts2[2].toInt()>=0 && opts2[2].toInt()<256)) ||
         (!(opts2[3].toInt()>=0 && opts2[3].toInt()<256)) ||
         (!(opts2[4].toInt()>=0 && opts2[4].toInt()<256))) {
      qDebug(".cfg: Syntax/logic error in EVT|STIM parameters!");
      initSuccess=false; break;
     } else {
      dummyEvt=new Event(opts2[0].toInt(),opts2[1].trimmed(),1,
                         QColor(opts2[2].toInt(),
                                opts2[3].toInt(),
                                opts2[4].toInt(),255)); // ..convert to enum!
      acqEvents.append(dummyEvt);
     }
    } else {
     qDebug(".cfg: Parse error in EVT|STIM parameters!");
     initSuccess=false; break;
    }
   } else if (opts[0].trimmed()=="RESP") { opts2=opts[1].split(",");
    if (opts2.size()==5) {
     if ((!(opts2[0].toInt()>0  && opts2[0].toInt()<64)) ||
         (!(opts2[1].size()>0)) ||
         (!(opts2[2].toInt()>=0 && opts2[2].toInt()<256)) ||
         (!(opts2[3].toInt()>=0 && opts2[3].toInt()<256)) ||
         (!(opts2[4].toInt()>=0 && opts2[4].toInt()<256))) {
      qDebug(".cfg: Syntax/logic error in EVT|RESP parameters!");
      initSuccess=false; break;
     } else {
      dummyEvt=new Event(opts2[0].toInt(),opts2[1].trimmed(),2,
                         QColor(opts2[2].toInt(),
                                opts2[3].toInt(),
                                opts2[4].toInt(),255)); // ..convert to enum!
      acqEvents.append(dummyEvt);
     }
    } else {
     qDebug(".cfg: Parse error in EVT|RESP parameters!");
     initSuccess=false; break;
    }
   }
  }
 } else {
  // DEFAULT EVENT SETTINGS (ALL EVENTS OF Octopus-STIM)..
  // (Just a single event pool exists for both stim and resp events)..
  for (int i=0;i<128;i++) {
   dummyString=stimEventNames[i];
   dummyEvt=new Event(i,dummyString,1,QColor(255,0,0,255));
   acqEvents.append(dummyEvt);
  }
  for (int i=0;i<64;i++) {
   dummyString=respEventNames[i];
   dummyEvt=new Event(i,dummyString,2,QColor(255,0,0,255));
   acqEvents.append(dummyEvt);
  }
 } if (!initSuccess) return;

 // CHN
 if (chnSection.size()>0) {
  for (int i=0;i<chnSection.size();i++) { opts=chnSection[i].split("=");
   if (opts[0].trimmed()=="APPEND") { opts2=opts[1].split(",");
    if (opts2.size()==11) {
     opts2[2]=opts2[2].trimmed();
     opts2[5]=opts2[5].trimmed(); opts2[6]=opts2[6].trimmed(); // Trim wspcs
     opts2[7]=opts2[7].trimmed(); opts2[8]=opts2[8].trimmed();
     opts2[9]=opts2[9].trimmed(); opts2[10]=opts2[10].trimmed();
     if ((!(opts2[0].toInt()>0 && opts2[0].toInt()<=8)) || // Amp#
	 (!(opts2[1].toInt()>0 && opts2[1].toInt()<=chnInfo.physChnCount)) || // Channel#
         (!(opts2[2].size()>0)) || // Channel name must be at least 1 char..

         (!(opts2[3].toInt()>=0 && opts2[3].toInt()<1000))   || // Rej
         (!(opts2[4].toInt()>=0 && opts2[4].toInt()<=chnInfo.physChnCount)) || // RejRef

         (!(opts2[5]=="T" || opts2[5]=="t" ||
          opts2[5]=="F" || opts2[5]=="f")) ||
         (!(opts2[6]=="T" || opts2[6]=="t" ||
          opts2[6]=="F" || opts2[6]=="f")) ||
         (!(opts2[7]=="T" || opts2[7]=="t" ||
          opts2[7]=="F" || opts2[7]=="f")) ||
         (!(opts2[8]=="T" || opts2[8]=="t" ||
          opts2[8]=="F" || opts2[8]=="f")) ||

         (!(opts2[9].toFloat()>=0. && opts2[9].toFloat()<=360.)) || // Theta 
         (!(opts2[10].toFloat()>=0. && opts2[10].toFloat()<=360.))){  // Phi
      qDebug(".cfg: Syntax/logic Error in CHN|APPEND parameters!");
      initSuccess=false; break;
     } else { // Set and append new channel..
      dummyChn=new Channel(opts2[1].toInt(),	 // Physical channel
                           opts2[2].trimmed(),	 // Channel Name
                           opts2[3].toInt(),	 // Rejection Level
                           opts2[4].toInt(),	 // Rejection Reference
                           opts2[5],opts2[6],    // Cnt Vis/Rec Flags
                           opts2[7],opts2[8],    // Avg Vis/Rec Flags
                           opts2[9].toFloat(),   // Electrode Cart. Coords
                           opts2[10].toFloat()); // (Theta,Phi)
      dummyChn->pastData.resize(cp.cntPastSize);
      dummyChn->pastFilt.resize(cp.cntPastSize); // Line-noise component
      dummyChn->setEventProfile(acqEvents.size(),cp.avgCount);
      acqChannels[opts2[0].toInt()-1].append(dummyChn); // add channel to the respective amplifier
     }
    } else {
     qDebug(".cfg: Parse error in CHN|APPEND parameters!");
     initSuccess=false; break;
    }
   } else if (opts[0].trimmed()=="CALIB") { opts2=opts[1].split(",");
    if (opts2.size()==1) loadCalib_OacFile(opts2[0].trimmed());
    else {
     qDebug(".cfg: Parse error or file not found in CHN|CALIB parameters!");
     qDebug(" Correction disabled/recalibration suggested!..");
    }
   }
  }
 } else {
  // Default channel settings
  QString chnString,noString;
  for (int i=0;i<2;i++) {
   for (int j=0;j<chnInfo.physChnCount;j++) {
    chnString="Chn#"+noString.setNum(j);
    dummyChn=new Channel(j+1,chnString,0,0,"t","t","t","t",0.,0.);
    dummyChn->pastData.resize(cp.cntPastSize);
    dummyChn->pastFilt.resize(cp.cntPastSize);
    acqChannels[i].append(dummyChn);
   }
  }
 } if (!initSuccess) return;

 // DIG
 if (digSection.size()>0) {
  for (int i=0;i<digSection.size();i++) { opts=digSection[i].split("=");
   if (opts[0].trimmed()=="POLHEMUS") { opts2=opts[1].split(",");
    if (opts2.size()==5) {
     if ((!(opts2[0].toInt()>=0  && opts2[0].toInt()<8)) ||      // ttyS#
         (!(opts2[1].toInt()==9600 ||
            opts2[1].toInt()==19200 ||
            opts2[1].toInt()==38400 ||
            opts2[1].toInt()==57600 ||
            opts2[1].toInt()==115200)) ||                        // BaudRate
         (!(opts2[2].toInt()==7 || opts2[2].toInt()==8)) ||      // Data Bits
         (!(opts2[3].trimmed()=="E" ||
            opts2[3].trimmed()=="O" ||
            opts2[3].trimmed()=="N")) || // Parity
         (!(opts2[4].toInt()==0 || opts2[4].toInt()==1))) {      // Stop Bit
      qDebug(".cfg: Parse/logic error in DIG|POLHEMUS parameters!");
      initSuccess=false; break;
     } else {
      serial.devname="/dev/ttyS"+opts2[0].trimmed();
      switch (opts2[1].toInt()) {
       case   9600: serial.baudrate=B9600;   break;
       case  19200: serial.baudrate=B19200;  break;
       case  38400: serial.baudrate=B38400;  break;
       case  57600: serial.baudrate=B57600;  break;
       case 115200: serial.baudrate=B115200; break;
      }
      switch (opts2[2].toInt()) {
       case 7: serial.databits=CS7; break;
       case 8: serial.databits=CS8; break;
      }
      if (opts2[3].trimmed()=="N") {
       serial.parity=serial.par_on=0;
      } else if (opts2[3].trimmed()=="O") {
       serial.parity=PARODD; serial.par_on=PARENB;
      } else if (opts2[3].trimmed()=="E") {
       serial.parity=0; serial.par_on=PARENB;
      }
      switch (opts2[4].toInt()) {
       default:
       case 0: serial.stopbit=0; break;
       case 1: serial.stopbit=CSTOPB; break;
      }
     }
    } else {
     qDebug(".cfg: Parse error in DIG|POLHEMUS parameters!");
     initSuccess=false; break;
    }
   }
  }
 } else {
  serial.devname="/dev/ttyS0"; serial.baudrate=B115200; serial.databits=CS8;
  serial.parity=serial.par_on=0; serial.stopbit=1;
 } if (!initSuccess) return;

 // GUI
 if (guiSection.size()>0) {
  for (int i=0;i<guiSection.size();i++) { opts=guiSection[i].split("=");
   if (opts[0].trimmed()=="CTRL") { opts2=opts[1].split(",");
    if (opts2.size()==4) {
     ctrlGuiX=opts2[0].toInt(); ctrlGuiY=opts2[1].toInt();
     ctrlGuiW=opts2[2].toInt(); ctrlGuiH=opts2[3].toInt();
     if ((!(ctrlGuiX >= -4000 && ctrlGuiX <= 4000)) ||
         (!(ctrlGuiY >= -3000 && ctrlGuiY <= 3000)) ||
         (!(ctrlGuiW >=   400 && ctrlGuiW <= 2000)) ||
         (!(ctrlGuiH >=    60 && ctrlGuiH <= 1800))) {
      qDebug(".cfg: GUI|CTRL size settings not in appropriate range!");
      initSuccess=false; break;
     }
    } else {
     qDebug(".cfg: Parse error in GUI settings!");
     initSuccess=false; break;
    }
  } else if (opts[0].trimmed()=="STRM") { opts2=opts[1].split(",");
    if (opts2.size()==4) {
     contGuiX=opts2[0].toInt(); contGuiY=opts2[1].toInt();
     contGuiW=opts2[2].toInt(); contGuiH=opts2[3].toInt();
     if ((!(contGuiX >= -4000 && contGuiX <= 4000)) ||
         (!(contGuiY >= -3000 && contGuiY <= 3000)) ||
         (!(contGuiW >=   400 && contGuiW <= 4000)) ||
         (!(contGuiH >=   800 && contGuiH <= 4000))) {
      qDebug(".cfg: GUI|STRM size settings not in appropriate range!");
      initSuccess=false; break;
     }
    } else {
     qDebug(".cfg: Parse error in GUI settings!");
     initSuccess=false; break;
    }
   } else if (opts[0].trimmed()=="HEAD") { opts2=opts[1].split(",");
    if (opts2.size()==4) {
     gl3DGuiX=opts2[0].toInt(); gl3DGuiY=opts2[1].toInt();
     gl3DGuiW=opts2[2].toInt(); gl3DGuiH=opts2[3].toInt();
     if ((!(gl3DGuiX >= -2000 && gl3DGuiX <= 2000)) ||
         (!(gl3DGuiY >= -2000 && gl3DGuiY <= 2000)) ||
         (!(gl3DGuiW >=   400 && gl3DGuiW <= 2000)) ||
         (!(gl3DGuiH >=   300 && gl3DGuiH <= 2000))) {
      qDebug(".cfg: GUI|HEAD size settings not in appropriate range!");
      initSuccess=false; break;
     }
    } else {
     qDebug(".cfg: Parse error in Head Widget settings!");
     initSuccess=false; break;
    }
   }
  }
 } else {
  ctrlGuiX=60; ctrlGuiY=60; ctrlGuiW=640; ctrlGuiH=480;
  contGuiX=60; contGuiY=60; contGuiW=640; contGuiH=480;
  gl3DGuiX=160; gl3DGuiY=160; gl3DGuiW=400; gl3DGuiH=300;
 } if (!initSuccess) return;

 // MOD
 if (modSection.size()>0) {
  for (int i=0;i<modSection.size();i++) { opts=modSection[i].split("=");
   if (opts[0].trimmed()=="GIZMO") { opts2=opts[1].split(",");
    if (opts2.size()==1) {
     if (opts2[0].size()) loadGizmo_OgzFile(opts2[0].trimmed());
     else {
      qDebug(".cfg: MOD|GIZMO filename error!");
      initSuccess=false; break;
     }
    } else {
     qDebug(".cfg: Parse error in MOD|GIZMO parameters!");
     initSuccess=false; break;
    }
   } else if (opts[0].trimmed()=="SCALP") { opts2=opts[1].split(",");
    if (opts2.size()==1) loadScalp_ObjFile(opts2[0].trimmed());
    else {
     qDebug(".cfg: Parse error in MOD|SCALP parameters!");
     initSuccess=false; break;
    }
   } else if (opts[0].trimmed()=="SKULL") { opts2=opts[1].split(",");
    if (opts2.size()==1) loadSkull_ObjFile(opts2[0].trimmed());
    else {
     qDebug(".cfg: Parse error in MOD|SKULL parameters!");
     initSuccess=false; break;
    }
   } else if (opts[0].trimmed()=="BRAIN") { opts2=opts[1].split(",");
    if (opts2.size()==1) loadBrain_ObjFile(opts2[0].trimmed());
    else {
     qDebug(".cfg: Parse error in MOD|BRAIN parameters!");
     initSuccess=false; break;
    }
   }
  }
 } if (!initSuccess) return;
} 
