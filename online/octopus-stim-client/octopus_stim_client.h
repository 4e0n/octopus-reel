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

#ifndef OCTOPUS_STIM_CLIENT_H
#define OCTOPUS_STIM_CLIENT_H

#include <QApplication>
#include <QWidget>
#include <QMainWindow>
#include <QMenuBar>
#include <QPushButton>
#include <QFileDialog>
#include <QStatusBar>
#include <QMessageBox>
#include <QAction>

#include <QtNetwork>
#include <unistd.h>
#include "../cs_command.h"
#include "../patt_datagram.h"
#include "../stim_test_para.h"
#include "../stim_event_names.h"
#include "../resp_event_names.h"

class StimClient : public QMainWindow {
 Q_OBJECT
 public:
  StimClient(QApplication *app,QWidget *parent=0) : QMainWindow(parent) {
	  nChns=64;
   application=app;
   guiWidth=400; guiHeight=160; guiX=100; guiY=100;

   stimCommandSocket=new QTcpSocket(this);
   stimDataSocket=new QTcpSocket(this);
   connect(stimCommandSocket,SIGNAL(error(QAbstractSocket::SocketError)),
           this,SLOT(slotStimCommandError(QAbstractSocket::SocketError)));
   connect(stimDataSocket,SIGNAL(error(QAbstractSocket::SocketError)),
           this,SLOT(slotStimDataError(QAbstractSocket::SocketError)));

   // *** INITIAL VALUES OF RUNTIME VARIABLES ***
   stimulation=trigger=false;

   // *** LOAD CONFIG FILE AND READ ALL LINES ***
   QString cfgLine; QStringList cfgLines;
   cfgFile.setFileName("octopus-stim-client.config");
   if (!cfgFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qDebug("octopus-stim-client: "
           "Cannot open ./octopus-stim-client.config for reading!..\n"
           "Loading hardcoded values..");
    stimHost="127.0.0.1"; stimCommPort=65000; stimDataPort=65001;
   } else { cfgStream.setDevice(&cfgFile); // Load all of the file to string
    while (!cfgStream.atEnd()) { cfgLine=cfgStream.readLine(160);
     cfgLines.append(cfgLine); } cfgFile.close(); parseConfig(cfgLines);
   }
   if (!initSuccess) return;

   // *** POST SETUP ***
   //stimSendCommand(CS_STIM_STOP,0,0,0); // Failsafe stop ongoing stim task..
   setGeometry(guiX,guiY,guiWidth,guiHeight);
   setFixedSize(guiWidth,guiHeight);

   // *** STATUSBAR ***
   guiStatusBar=new QStatusBar(this);
   guiStatusBar->setGeometry(0,height()-20,width(),20);
   guiStatusBar->show(); setStatusBar(guiStatusBar);

   // *** MENUBAR ***
   menuBar=new QMenuBar(this); menuBar->setFixedWidth(width());
   QMenu *sysMenu=new QMenu("&System",menuBar);
   QMenu *testMenu=new QMenu("&Test",menuBar);
   QMenu *paraMenu=new QMenu("&Paradigm",menuBar);
   menuBar->addMenu(sysMenu); menuBar->addMenu(testMenu);
   menuBar->addMenu(paraMenu); setMenuBar(menuBar);

   // System Menu
   toggleCalAction=new QAction("&Start calibration",this);
   rebootAction=new QAction("&Reboot Server",this);
   shutdownAction=new QAction("&Shutdown Server",this);
   aboutAction=new QAction("&About..",this);
   quitAction=new QAction("&Quit",this);
   toggleCalAction->setStatusTip(
  "Start amplifier calibration procedure (Lasts approximately 45 minutes).");
   rebootAction->setStatusTip("Reboot STIM server");
   shutdownAction->setStatusTip("Shutdown STIM server");
   aboutAction->setStatusTip("About Octopus-STIM-Client..");
   quitAction->setStatusTip("Quit Octopus-STIM-Client");
   connect(toggleCalAction,SIGNAL(triggered()),
           this,SLOT(slotToggleCalibration()));
   connect(rebootAction,SIGNAL(triggered()),
           this,SLOT(slotReboot()));
   connect(shutdownAction,SIGNAL(triggered()),
           this,SLOT(slotShutdown()));
   connect(aboutAction,SIGNAL(triggered()),this,SLOT(slotAbout()));
   connect(quitAction,SIGNAL(triggered()),this,SLOT(slotQuit()));
   sysMenu->addAction(toggleCalAction); sysMenu->addSeparator();
   sysMenu->addAction(rebootAction);
   sysMenu->addAction(shutdownAction); sysMenu->addSeparator();
   sysMenu->addAction(aboutAction); sysMenu->addSeparator();
   sysMenu->addAction(quitAction);

   // Test Menu
   testSCAction=new QAction("Sine-Cosine Test",this);
   testSquareAction=new QAction("Square-wave Test",this);
   testSCAction->setStatusTip("Sine-Cosine dual-event Jitter Test");
   testSquareAction->setStatusTip("SquareWave single-Event Jitter Test");
   connect(testSCAction,SIGNAL(triggered()),
           this,SLOT(slotTestSineCosine()));
   connect(testSquareAction,SIGNAL(triggered()),
           this,SLOT(slotTestSquare()));
   testMenu->addAction(testSCAction);
   testMenu->addAction(testSquareAction);

   // Paradigm Menu
   paraLoadPatAction=new QAction("&Load STIM Pattern..",this);
   paraClickAction=new QAction("1ms Click",this);
   paraSquareBurstAction=new QAction("50ms Burst",this);
   paraIIDITDAction=new QAction("IID-ITD",this);
   paraIIDITD_ML_Action=new QAction("IID-ITD Mono-L",this);
   paraIIDITD_MR_Action=new QAction("IID-ITD Mono-R",this);
   paraITDOppChnAction=new QAction("Opponent Channels",this);
   paraITDOppChn2Action=new QAction("Opponent Channels v2",this);
   paraITDLinTestAction=new QAction("ITD Linearity Test",this);
   paraITDLinTest2Action=new QAction("ITD Linearity Exp.2",this);
   paraITDTompresAction=new QAction("ITD Optimum Adapter-Probe Offset",this);
   paraITDPipCTrainAction=new QAction("ITD Pip vs. CTrain - 500ms Adapter-Probe",this);
   paraLoadPatAction->setStatusTip(
    "Load precalculated STIMulus pattern..");
   paraClickAction->setStatusTip("Click of 1ms duration. SOA=1000ms");
   paraSquareBurstAction->setStatusTip(
    "SquareWave burst of 50ms duration, with 500us period, and SOA=1000ms");
   paraIIDITDAction->setStatusTip(
  "Dr. Ungan's specialized paradigm for Interaural Intensity vs.Time Delay");
   paraIIDITD_ML_Action->setStatusTip(
    "Dr. Ungan's specialized paradigm for IID vs. ITD (Monaural-Left)");
   paraIIDITD_MR_Action->setStatusTip(
    "Dr. Ungan's specialized paradigm for IID vs. ITD (Monaural-Right)");
   paraITDOppChnAction->setStatusTip(
    "Verification/falsification of ITD Opponent Channels Model");
   paraITDOppChn2Action->setStatusTip(
    "Verification/falsification of ITD Opponent Channels Model v2");
   paraITDLinTestAction->setStatusTip(
    "Testing of ITD Linearity");
   paraITDLinTest2Action->setStatusTip(
    "Testing of ITD Linearity - Exp.2");
   paraITDTompresAction->setStatusTip(
    "Testing of ITD Linearity - Exp.3a/b");
   paraITDPipCTrainAction->setStatusTip(
    "Testing of ITD Linearity - Exp.3c");
   connect(paraLoadPatAction,SIGNAL(triggered()),
           this,SLOT(slotParadigmLoadPattern()));
   connect(paraClickAction,SIGNAL(triggered()),
           this,SLOT(slotParadigmClick()));
   connect(paraSquareBurstAction,SIGNAL(triggered()),
           this,SLOT(slotParadigmSquareBurst()));
   connect(paraIIDITDAction,SIGNAL(triggered()),
           this,SLOT(slotParadigmIIDITD()));
   connect(paraIIDITD_ML_Action,SIGNAL(triggered()),
           this,SLOT(slotParadigmIIDITD_MonoL()));
   connect(paraIIDITD_MR_Action,SIGNAL(triggered()),
           this,SLOT(slotParadigmIIDITD_MonoR()));
   connect(paraITDOppChnAction,SIGNAL(triggered()),
           this,SLOT(slotParadigmITDOppChn()));
   connect(paraITDOppChn2Action,SIGNAL(triggered()),
           this,SLOT(slotParadigmITDOppChn2()));
   connect(paraITDLinTestAction,SIGNAL(triggered()),
           this,SLOT(slotParadigmITDLinTest()));
   connect(paraITDLinTest2Action,SIGNAL(triggered()),
           this,SLOT(slotParadigmITDLinTest2()));
   connect(paraITDTompresAction,SIGNAL(triggered()),
           this,SLOT(slotParadigmTompres()));
   connect(paraITDPipCTrainAction,SIGNAL(triggered()),
           this,SLOT(slotParadigmPipCTrain()));
   paraMenu->addAction(paraLoadPatAction); paraMenu->addSeparator();
   paraMenu->addAction(paraClickAction);
   paraMenu->addAction(paraSquareBurstAction); paraMenu->addSeparator();
   paraMenu->addAction(paraIIDITDAction);
   paraMenu->addAction(paraIIDITD_ML_Action);
   paraMenu->addAction(paraIIDITD_MR_Action);
   paraMenu->addAction(paraITDOppChnAction);
   paraMenu->addAction(paraITDOppChn2Action);
   paraMenu->addAction(paraITDLinTestAction);
   paraMenu->addAction(paraITDLinTest2Action);
   paraMenu->addAction(paraITDTompresAction);
   paraMenu->addAction(paraITDPipCTrainAction);

   // *** BUTTONS AT THE TOP ***
   lightsOnButton=new QPushButton("Lights On",this);
   lightsOnButton->setGeometry(20,height()-124,78,20);
   lightsOnButton->setCheckable(false);
   connect(lightsOnButton,SIGNAL(clicked()),
           this,SLOT(slotLightsOn()));

   lightsDimmButton=new QPushButton("Lights Dimm",this);
   lightsDimmButton->setGeometry(110,height()-124,78,20);
   lightsDimmButton->setCheckable(false);
   connect(lightsDimmButton,SIGNAL(clicked()),
           this,SLOT(slotLightsDimm()));

   lightsOffButton=new QPushButton("Lights Off",this);
   lightsOffButton->setGeometry(200,height()-124,78,20);
   lightsOffButton->setCheckable(false);
   connect(lightsOffButton,SIGNAL(clicked()),
           this,SLOT(slotLightsOff()));

   startStimulationButton=new QPushButton("Start Stim",this);
   startStimulationButton->setGeometry(20,height()-84,78,20);
   startStimulationButton->setCheckable(false);
   connect(startStimulationButton,SIGNAL(clicked()),
           this,SLOT(slotStartStimulation()));

   stopStimulationButton=new QPushButton("Stop Stim",this);
   stopStimulationButton->setGeometry(100,height()-84,78,20);
   stopStimulationButton->setCheckable(false);
   connect(stopStimulationButton,SIGNAL(clicked()),
           this,SLOT(slotStopStimulation()));

   pauseStimulationButton=new QPushButton("Pause Stim",this);
   pauseStimulationButton->setGeometry(200,height()-84,98,20);
   pauseStimulationButton->setCheckable(false);
   connect(pauseStimulationButton,SIGNAL(clicked()),
           this,SLOT(slotPauseStimulation()));

   resumeStimulationButton=new QPushButton("Resume Stim",this);
   resumeStimulationButton->setGeometry(300,height()-84,98,20);
   resumeStimulationButton->setCheckable(false);
   connect(resumeStimulationButton,SIGNAL(clicked()),
           this,SLOT(slotResumeStimulation()));

   startTriggerButton=new QPushButton("Start Trig",this);
   startTriggerButton->setGeometry(20,height()-54,78,20);
   startTriggerButton->setCheckable(false);
   connect(startTriggerButton,SIGNAL(clicked()),
           this,SLOT(slotStartTrigger()));

   stopTriggerButton=new QPushButton("Stop Trig",this);
   stopTriggerButton->setGeometry(100,height()-54,78,20);
   stopTriggerButton->setCheckable(false);
   connect(stopTriggerButton,SIGNAL(clicked()),
           this,SLOT(slotStopTrigger()));

   setWindowTitle("Octopus STIM Client v0.9.9");
  }

  // *** UTILITY ROUTINES ***
 
  void parseConfig(QStringList cfgLines) {
   QStringList cfgValidLines,opts,opts2,opts3,netSection;

   for (int i=0;i<cfgLines.size();i++) { // Isolate valid lines
    if (!(cfgLines[i].at(0)=='#') &&
        cfgLines[i].contains('|')) cfgValidLines.append(cfgLines[i]); }

   // *** CATEGORIZE SECTIONS ***

   initSuccess=true;
   for (int i=0;i<cfgValidLines.size();i++) {
    opts=cfgValidLines[i].split("|");
    if (opts[0].trimmed()=="NET") netSection.append(opts[1]);
    else {
     qDebug("Unknown section in .config file!"); initSuccess=false; break;
    }
   } if (!initSuccess) return;

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
        qDebug(".config: Error in STIM IP and/or port settings!");
        initSuccess=false; break;
       }
      } else {
       qDebug(".config: Parse error in STIM IP (v4) Address!");
       initSuccess=false; break;
      }
     }
    }
   } else {
    stimHost="127.0.0.1"; stimCommPort=65000; stimDataPort=65001;
   } if (!initSuccess) return;
  } 

  void stimSendCommand(int command,int ip0,int ip1,int ip2) {
   stimCommandSocket->connectToHost(stimHost,stimCommPort);
   stimCommandSocket->waitForConnected();
   QDataStream stimCommandStream(stimCommandSocket); csCmd.cmd=command;
   csCmd.iparam[0]=ip0; csCmd.iparam[1]=ip1; csCmd.iparam[2]=ip2;
   stimCommandStream.writeRawData((const char*)(&csCmd),sizeof(cs_command));
   stimCommandSocket->flush(); stimCommandSocket->disconnectFromHost();
  }

  void startCalibration() { calibration=true; calPts=0;
   for (int i=0;i<nChns;i++) { calA[i]=calB[i]=0.; }
   calibMsg="Calibration started.. Collecting DC";
   stimSendCommand(CS_STIM_SET_PARADIGM,TEST_CALIBRATION,0,0); usleep(250000);
   stimSendCommand(CS_STIM_START,0,0,0);
  }

  void stopCalibration() { calibration=false;
   stimSendCommand(CS_STIM_STOP,0,0,0);
   guiStatusBar->showMessage("Calibration manually stopped!");
  }

 private slots:
  // *** TCP HANDLERS

  void slotStimCommandError(QAbstractSocket::SocketError socketError) {
   switch (socketError) {
    case QAbstractSocket::HostNotFoundError:
     qDebug("octopus-stim-client: "
            "STIMulus command server does not exist!"); break;
    case QAbstractSocket::ConnectionRefusedError:
     qDebug("octopus-stim-client: "
            "STIMulus command server refused connection!"); break;
    default:
     qDebug("octopus-stim-client: "
            "STIMulus command server unknown error! %d",
            socketError); break;
   }
  }

  void slotStimDataError(QAbstractSocket::SocketError socketError) {
   switch (socketError) {
    case QAbstractSocket::HostNotFoundError:
     qDebug("octopus-stim-client: "
            "STIMulus data server does not exist!"); break;
    case QAbstractSocket::ConnectionRefusedError:
     qDebug("octopus-stim-client: "
            "STIMulus data server refused connection!"); break;
    default:
     qDebug("octopus-stim-client: "
            "STIMulus data server unknown error! %d",
            socketError); break;
   }
  }

  void slotToggleCalibration() {
  if (calibration) { stopCalibration();
    toggleCalAction->setText("&Start calibration");
   } else { startCalibration();
    toggleCalAction->setText("&Stop calibration");
   }
  }

  void slotReboot() {
   stimSendCommand(CS_REBOOT,0,0,0);
   guiStatusBar->showMessage("Stim server is rebooting..",5000);
  }

  void slotShutdown() {
   stimSendCommand(CS_SHUTDOWN,0,0,0);
   guiStatusBar->showMessage("Stim server is shutting down..",5000);
  }

  void slotQuit() {
   //stimSendCommand(CS_XXX,0,0,0);
   application->exit(0);
  }

  void slotAbout() {
   QMessageBox::about(this,"About Octopus-ReEL",
                           "STIM Client\n"
                           "(c) 2020 Barkin Ilhan (barkin@unrlabs.org)\n"
                           "This is free software coming with\n"
                           "ABSOLUTELY NO WARRANTY; You are welcome\n"
                         "to redistribute it under conditions of GPL v3.\n");
  }

  //void slotToggleStimulation() {
  // if (!stimulation) {
  //  stimSendCommand(CS_STIM_START,0,0,0); stimulation=true;
  // } else {
  //  stimSendCommand(CS_STIM_STOP,0,0,0); stimulation=false;
  // }
  //}

  void slotStartStimulation() {
   if (!stimulation) {
    stimSendCommand(CS_STIM_START,0,0,0); stimulation=true;
   }
  }

  void slotStopStimulation() {
   if (stimulation) {
    stimSendCommand(CS_STIM_STOP,0,0,0); stimulation=false;
   }
  }

  void slotPauseStimulation() {
   if (stimulation) {
    stimSendCommand(CS_STIM_PAUSE,0,0,0); stimulation=false;
   }
  }

  void slotResumeStimulation() {
   //if (!stimulation) {
    stimSendCommand(CS_STIM_RESUME,0,0,0); stimulation=true;
   //}
  }

  void slotStartTrigger() {
   if (!trigger) {
    stimSendCommand(CS_TRIG_START,0,0,0); trigger=true;
   }
  }

  void slotStopTrigger() {
   if (trigger) {
    stimSendCommand(CS_TRIG_STOP,0,0,0); trigger=false;
   }
  }

  void slotLightsOn() {
   stimSendCommand(CS_STIM_LIGHTS_ON,0,0,0);
  }
  void slotLightsDimm() {
   stimSendCommand(CS_STIM_LIGHTS_DIMM,0,0,0);
  }
  void slotLightsOff() {
   stimSendCommand(CS_STIM_LIGHTS_OFF,0,0,0);
  }

  // *** RELATIVELY SIMPLE COMMANDS TO SERVERS ***

  void slotTestSineCosine() {
   stimSendCommand(CS_STIM_SET_PARADIGM,TEST_SINECOSINE,0,0);
  }
  void slotTestSquare() {
   stimSendCommand(CS_STIM_SET_PARADIGM,TEST_SQUARE,0,0);
  }

  void slotParadigmLoadPattern() {
   QString patFileName=QFileDialog::getOpenFileName(0,
                      "Open Pattern File",".","ASCII Pattern Files (*.arp)");
   if (!patFileName.isEmpty()) { patFile.setFileName(patFileName);
    patFile.open(QIODevice::ReadOnly); patStream.setDevice(&patFile);
    pattern=patStream.readAll(); patFile.close(); // Close pattern file.

    stimCommandSocket->connectToHost(stimHost,stimCommPort);
    stimCommandSocket->waitForConnected();

    // We want to be ready when the answer comes..
    connect(stimCommandSocket,SIGNAL(readyRead()),
            (QObject *)this,SLOT(slotReadAcknowledge()));

    // Send Command SYN and then Sync.
    QDataStream stimCommandStream(stimCommandSocket);
    csCmd.cmd=CS_STIM_LOAD_PATTERN_SYN;
    csCmd.iparam[0]=patFile.size(); // File size is the only parameter
    csCmd.iparam[1]=csCmd.iparam[2]=0;
  stimCommandStream.writeRawData((const char*)(&csCmd),(sizeof(cs_command)));
    stimCommandSocket->flush();
   }
  }
  void slotReadAcknowledge() {
   QDataStream ackStream(stimCommandSocket);

   // Wait for ACK from stimulation server
   if (stimCommandSocket->bytesAvailable() >= (int)(sizeof(cs_command))) {
    ackStream.readRawData((char*)(&csAck),(int)(sizeof(cs_command)));

    if (csAck.cmd!=CS_STIM_LOAD_PATTERN_ACK) { // Something went wrong?
     qDebug("octopus-stim-client: Error in STIM daemon ACK reply!");
     return;
    }

    // Now STIM Server is waiting for the file.. Let's send over data port..
    stimDataSocket->connectToHost(stimHost,stimDataPort);
    stimDataSocket->waitForConnected();
    QDataStream stimDataStream(stimDataSocket);

    int dataCount=0; pattDatagram.magic_number=0xaabbccdd;
    for (int i=0;i<pattern.size();i++) {
     pattDatagram.data[dataCount]=pattern.at(i).toLatin1(); dataCount++;
     if (dataCount==128) { // We got 128 bytes.. Send packet and Sync.
      pattDatagram.size=dataCount; dataCount=0;
      stimDataStream.writeRawData((const char*)(&pattDatagram),
                                  sizeof(patt_datagram));
      stimDataSocket->flush();
     }
    }
    if (dataCount!=0) { // Last fragment whose size is !=0 and <128
     pattDatagram.size=dataCount; dataCount=0;
     stimDataStream.writeRawData((const char*)(&pattDatagram),
                                 sizeof(patt_datagram));
     stimDataSocket->flush();
    }
    stimDataSocket->disconnectFromHost();
    disconnect(stimCommandSocket,SIGNAL(readyRead()),
               this,SLOT(slotReadAcknowledge()));
   }
  }

  void slotParadigmClick() {
   stimSendCommand(CS_STIM_SET_PARADIGM,PARA_CLICK,0,0);
  }
  void slotParadigmSquareBurst() {
   stimSendCommand(CS_STIM_SET_PARADIGM,PARA_SQUAREBURST,0,0);
  }
  void slotParadigmIIDITD() {
   stimSendCommand(CS_STIM_SET_PARADIGM,PARA_IIDITD,0,0);
  }
  void slotParadigmIIDITD_MonoL() {
   stimSendCommand(CS_STIM_SET_PARADIGM,PARA_IIDITD,1,0);
  }
  void slotParadigmIIDITD_MonoR() {
   stimSendCommand(CS_STIM_SET_PARADIGM,PARA_IIDITD,2,0);
  }
  void slotParadigmITDOppChn() {
   stimSendCommand(CS_STIM_SET_PARADIGM,PARA_ITD_OPPCHN,0,0);
  }
  void slotParadigmITDOppChn2() {
   stimSendCommand(CS_STIM_SET_PARADIGM,PARA_ITD_OPPCHN2,0,0);
  }
  void slotParadigmITDLinTest() {
   stimSendCommand(CS_STIM_SET_PARADIGM,PARA_ITD_LINTEST,0,0);
  }
  void slotParadigmITDLinTest2() {
   stimSendCommand(CS_STIM_SET_PARADIGM,PARA_ITD_LINTEST2,0,0);
  }
  void slotParadigmTompres() {
   stimSendCommand(CS_STIM_SET_PARADIGM,PARA_ITD_TOMPRES,0,0);
  }
  void slotParadigmPipCTrain() {
   stimSendCommand(CS_STIM_SET_PARADIGM,PARA_ITD_PIP_CTRAIN,0,0);
  }

 private:
  QApplication *application;
  QTcpSocket *stimCommandSocket,*stimDataSocket;

  bool calibration,stimulation,trigger;

  QFile cfgFile,patFile; QTextStream cfgStream,patStream;

  // NET
  QString stimHost,acqHost;
  int stimCommPort,stimDataPort,acqCommPort,acqDataPort;

  bool initSuccess;

  cs_command csCmd,csAck;

  // GUI
  int guiX,guiY,guiWidth,guiHeight,hwX,hwY,hwWidth,hwHeight;

  QStatusBar *guiStatusBar;

  QMenuBar *menuBar;
  QAction *toggleCalAction,*rebootAction,*shutdownAction,
          *quitAction,*aboutAction,*testSCAction,*testSquareAction,
          *paraLoadPatAction,*paraClickAction,*paraSquareBurstAction,
          *paraIIDITDAction,*paraIIDITD_ML_Action,*paraIIDITD_MR_Action,
	  *paraITDOppChnAction,*paraITDOppChn2Action,
	  *paraITDLinTestAction,*paraITDLinTest2Action,
	  *paraITDTompresAction,*paraITDPipCTrainAction;

  // Calibration
  int calPts; QVector<float> calA,calB;
  QVector<float> calDC,calSin;

  QString pattern,calibMsg;
  patt_datagram pattDatagram;

  QPushButton *lightsOnButton,*lightsDimmButton,*lightsOffButton,
	      *startStimulationButton,*stopStimulationButton,
	      *pauseStimulationButton,*resumeStimulationButton,
	      *startTriggerButton,*stopTriggerButton;

  int nChns;
};

#endif
