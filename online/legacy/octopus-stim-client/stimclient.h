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
   application=app;

   // Parse system config file for variables
   QStringList cfgValidLines,opts,opts2,netSection; QFile cfgFile; QTextStream cfgStream;
   QString cfgLine; QStringList cfgLines; cfgFile.setFileName("/etc/octopus_stim_client.conf");
   if (!cfgFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qDebug() << "octopus_stim_client: <.conf> cannot load /etc/octopus_stim_client.conf.";
    app->quit();
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
     else { qDebug() << "octopus_stim_client: <.conf> Unknown section in .conf file!";
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
        qDebug() << "octopus_stim_client: <.conf> (this) Host IP is" << confHost;
        confCommP=opts2[1].toInt(); confDataP=opts2[2].toInt();
        // Simple port validation..
        if ((!(confCommP >= 1024 && confCommP <= 65535)) ||
            (!(confDataP >= 1024 && confDataP <= 65535))) {
         qDebug() << "octopus_stim_client: <.conf> Error in Hostname/IP and/or port settings!";
         app->quit();
        } else {
         qDebug() << "octopus_stim_client: <.conf> CommPort ->" << confCommP << "DataPort ->" << confDataP;
	}
       }
      } else {
       qDebug() << "octopus_stim_client: <.conf> Parse error in Hostname/IP(v4) Address!";
       app->quit();
      }
     }
    } else {
     qDebug() << "octopus_stim_client: <.conf> Parse error in NET section!";
     app->quit();
    }
   }

   guiW=400; guiH=160; guiX=100; guiY=100;

   stimCommandSocket=new QTcpSocket(this); stimDataSocket=new QTcpSocket(this);
   connect(stimCommandSocket,SIGNAL(error(QAbstractSocket::SocketError)),
           this,SLOT(slotStimCommandError(QAbstractSocket::SocketError)));
   connect(stimDataSocket,SIGNAL(error(QAbstractSocket::SocketError)),
           this,SLOT(slotStimDataError(QAbstractSocket::SocketError)));

   // *** INITIAL VALUES OF RUNTIME VARIABLES ***
   stimulation=trigger=false;

   // *** POST SETUP ***
   //stimSendCommand(CS_STIM_STOP,0,0,0); // Failsafe stop ongoing stim task..
   setGeometry(guiX,guiY,guiW,guiH); setFixedSize(guiW,guiH);

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
   rebootAction=new QAction("&Reboot Server",this);
   shutdownAction=new QAction("&Shutdown Server",this);
   aboutAction=new QAction("&About..",this);
   quitAction=new QAction("&Quit",this);
   rebootAction->setStatusTip("Reboot STIM server");
   shutdownAction->setStatusTip("Shutdown STIM server");
   aboutAction->setStatusTip("About Octopus-STIM-Client..");
   quitAction->setStatusTip("Quit Octopus-STIM-Client");
   connect(rebootAction,SIGNAL(triggered()),this,SLOT(slotReboot()));
   connect(shutdownAction,SIGNAL(triggered()),this,SLOT(slotShutdown()));
   connect(aboutAction,SIGNAL(triggered()),this,SLOT(slotAbout()));
   connect(quitAction,SIGNAL(triggered()),this,SLOT(slotQuit()));
   sysMenu->addAction(rebootAction); sysMenu->addAction(shutdownAction);
   sysMenu->addSeparator();
   sysMenu->addAction(aboutAction);
   sysMenu->addSeparator();
   sysMenu->addAction(quitAction);

   // Test Menu
   testSCAction=new QAction("Sine-Cosine Test",this);
   testSquareAction=new QAction("Square-wave Test",this);
   testSCAction->setStatusTip("Sine-Cosine dual-event Jitter Test");
   testSquareAction->setStatusTip("SquareWave single-Event Jitter Test");
   connect(testSCAction,SIGNAL(triggered()),this,SLOT(slotTestSineCosine()));
   connect(testSquareAction,SIGNAL(triggered()),this,SLOT(slotTestSquare()));
   testMenu->addAction(testSCAction); testMenu->addAction(testSquareAction);

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
   paraITDPipRandAction=new QAction("ITD Pip vs. CTrain - 400-800ms(rnd) Adapter-Probe",this);
   para0021aAction=new QAction("ITD Para 0021a (150ms-250ms AP-randomized-gap)",this);
   para0021a1Action=new QAction("ITD Para 0021a1 (150ms AP-gap) Adapter-Probe",this);
   para0021a2Action=new QAction("ITD Para 0021a2 (200ms AP-gap) Adapter-Probe",this);
   para0021a3Action=new QAction("ITD Para 0021a3 (250ms AP-gap) Adapter-Probe",this);
   para0021bAction=new QAction("ITD Para 0021b (Single) Adapter-Probe",this);
   para0021cAction=new QAction("ITD Para 0021c (Quadruple) Adapter-Probe",this);
   para0021dAction=new QAction("ITD Para 0021d AP-equal-eccentricity (150ms-250ms AP-randomized-gap)",this);
   para0021eAction=new QAction("ITD Para 0021e",this);
   paraLoadPatAction->setStatusTip("Load precalculated STIMulus pattern..");
   paraClickAction->setStatusTip("Click of 1ms duration. SOA=1000ms");
   paraSquareBurstAction->setStatusTip("SquareWave burst of 50ms duration, with 500us period, and SOA=1000ms");
   paraIIDITDAction->setStatusTip("Dr. Ungan's specialized paradigm for Interaural Intensity vs.Time Delay");
   paraIIDITD_ML_Action->setStatusTip("Dr. Ungan's specialized paradigm for IID vs. ITD (Monaural-Left)");
   paraIIDITD_MR_Action->setStatusTip("Dr. Ungan's specialized paradigm for IID vs. ITD (Monaural-Right)");
   paraITDOppChnAction->setStatusTip("Verification/falsification of ITD Opponent Channels Model");
   paraITDOppChn2Action->setStatusTip("Verification/falsification of ITD Opponent Channels Model v2");
   paraITDLinTestAction->setStatusTip("Testing of ITD Linearity");
   paraITDLinTest2Action->setStatusTip("Testing of ITD Linearity - Exp.2");
   paraITDTompresAction->setStatusTip("Testing of ITD Linearity - Exp.3a/b");
   paraITDPipCTrainAction->setStatusTip("Testing of ITD Linearity - Exp.3c");
   paraITDPipRandAction->setStatusTip("Testing of ITD vs. Tone Adaptation - RND");
   para0021aAction->setStatusTip("ITD Para 0021a (150ms-250ms AP-randomized-gap)");
   para0021a1Action->setStatusTip("ITD Para 0021a1 (150ms AP-gap) Adapter-Probe");
   para0021a2Action->setStatusTip("ITD Para 0021a2 (200ms AP-gap) Adapter-Probe");
   para0021a3Action->setStatusTip("ITD Para 0021a3 (250ms AP-gap) Adapter-Probe");
   para0021bAction->setStatusTip("ITD Para 0021b Adapter-Probe");
   para0021cAction->setStatusTip("ITD Para 0021c (Quadruple) Adapter-Probe");
   para0021dAction->setStatusTip("ITD Para 0021d AP-equal-eccentricity (150ms-250ms AP-randomized-gap)");
   para0021eAction->setStatusTip("ITD Para 0021e");
   connect(paraLoadPatAction,SIGNAL(triggered()),this,SLOT(slotParadigmLoadPattern()));
   connect(paraClickAction,SIGNAL(triggered()),this,SLOT(slotParadigmClick()));
   connect(paraSquareBurstAction,SIGNAL(triggered()),this,SLOT(slotParadigmSquareBurst()));
   connect(paraIIDITDAction,SIGNAL(triggered()),this,SLOT(slotParadigmIIDITD()));
   connect(paraIIDITD_ML_Action,SIGNAL(triggered()),this,SLOT(slotParadigmIIDITD_MonoL()));
   connect(paraIIDITD_MR_Action,SIGNAL(triggered()),this,SLOT(slotParadigmIIDITD_MonoR()));
   connect(paraITDOppChnAction,SIGNAL(triggered()),this,SLOT(slotParadigmITDOppChn()));
   connect(paraITDOppChn2Action,SIGNAL(triggered()),this,SLOT(slotParadigmITDOppChn2()));
   connect(paraITDLinTestAction,SIGNAL(triggered()),this,SLOT(slotParadigmITDLinTest()));
   connect(paraITDLinTest2Action,SIGNAL(triggered()),this,SLOT(slotParadigmITDLinTest2()));
   connect(paraITDTompresAction,SIGNAL(triggered()),this,SLOT(slotParadigmTompres()));
   connect(paraITDPipCTrainAction,SIGNAL(triggered()),this,SLOT(slotParadigmPipCTrain()));
   connect(paraITDPipRandAction,SIGNAL(triggered()),this,SLOT(slotParadigmPipRand()));
   connect(para0021aAction,SIGNAL(triggered()),this,SLOT(slotParadigm0021a()));
   connect(para0021a1Action,SIGNAL(triggered()),this,SLOT(slotParadigm0021a1()));
   connect(para0021a2Action,SIGNAL(triggered()),this,SLOT(slotParadigm0021a2()));
   connect(para0021a3Action,SIGNAL(triggered()),this,SLOT(slotParadigm0021a3()));
   connect(para0021bAction,SIGNAL(triggered()),this,SLOT(slotParadigm0021b()));
   connect(para0021cAction,SIGNAL(triggered()),this,SLOT(slotParadigm0021c()));
   connect(para0021dAction,SIGNAL(triggered()),this,SLOT(slotParadigm0021d()));
   connect(para0021eAction,SIGNAL(triggered()),this,SLOT(slotParadigm0021e()));
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
   paraMenu->addAction(paraITDPipRandAction); paraMenu->addSeparator();
   paraMenu->addAction(para0021aAction);
   paraMenu->addAction(para0021a1Action);
   paraMenu->addAction(para0021a2Action);
   paraMenu->addAction(para0021a3Action);
   paraMenu->addAction(para0021bAction);
   paraMenu->addAction(para0021cAction);
   paraMenu->addAction(para0021dAction); paraMenu->addSeparator();
   paraMenu->addAction(para0021eAction);

   // *** BUTTONS AT THE TOP ***
   lightsOnButton=new QPushButton("Lights On",this);
   lightsOnButton->setGeometry(20,height()-124,78,20);
   lightsOnButton->setCheckable(false);
   connect(lightsOnButton,SIGNAL(clicked()),this,SLOT(slotLightsOn()));

   lightsDimmButton=new QPushButton("Lights Dimm",this);
   lightsDimmButton->setGeometry(110,height()-124,78,20);
   lightsDimmButton->setCheckable(false);
   connect(lightsDimmButton,SIGNAL(clicked()),this,SLOT(slotLightsDimm()));

   lightsOffButton=new QPushButton("Lights Off",this);
   lightsOffButton->setGeometry(200,height()-124,78,20);
   lightsOffButton->setCheckable(false);
   connect(lightsOffButton,SIGNAL(clicked()),this,SLOT(slotLightsOff()));

   startStimulationButton=new QPushButton("Start Stim",this);
   startStimulationButton->setGeometry(20,height()-84,78,20);
   startStimulationButton->setCheckable(false);
   connect(startStimulationButton,SIGNAL(clicked()),this,SLOT(slotStartStimulation()));

   stopStimulationButton=new QPushButton("Stop Stim",this);
   stopStimulationButton->setGeometry(100,height()-84,78,20);
   stopStimulationButton->setCheckable(false);
   connect(stopStimulationButton,SIGNAL(clicked()),this,SLOT(slotStopStimulation()));

   pauseStimulationButton=new QPushButton("Pause Stim",this);
   pauseStimulationButton->setGeometry(200,height()-84,98,20);
   pauseStimulationButton->setCheckable(false);
   connect(pauseStimulationButton,SIGNAL(clicked()),this,SLOT(slotPauseStimulation()));

   resumeStimulationButton=new QPushButton("Resume Stim",this);
   resumeStimulationButton->setGeometry(300,height()-84,98,20);
   resumeStimulationButton->setCheckable(false);
   connect(resumeStimulationButton,SIGNAL(clicked()),this,SLOT(slotResumeStimulation()));

   startTriggerButton=new QPushButton("Start Trig",this);
   startTriggerButton->setGeometry(20,height()-54,78,20);
   startTriggerButton->setCheckable(false);
   connect(startTriggerButton,SIGNAL(clicked()),this,SLOT(slotStartTrigger()));

   stopTriggerButton=new QPushButton("Stop Trig",this);
   stopTriggerButton->setGeometry(100,height()-54,78,20);
   stopTriggerButton->setCheckable(false);
   connect(stopTriggerButton,SIGNAL(clicked()),this,SLOT(slotStopTrigger()));

   setWindowTitle("Octopus STIM Client v1.0.1");
  }

  // *** UTILITY ROUTINES ***
 
  void stimSendCommand(int command,int ip0,int ip1,int ip2) {
   stimCommandSocket->connectToHost(confHost,confCommP);
   stimCommandSocket->waitForConnected();
   QDataStream stimCommandStream(stimCommandSocket); csCmd.cmd=command;
   csCmd.iparam[0]=ip0; csCmd.iparam[1]=ip1; csCmd.iparam[2]=ip2;
   stimCommandStream.writeRawData((const char*)(&csCmd),sizeof(cs_command));
   stimCommandSocket->flush(); stimCommandSocket->disconnectFromHost();
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
                           "(c) 2007-2025 Barkin Ilhan (barkin@unrlabs.org)\n"
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
   if (!stimulation) { stimSendCommand(CS_STIM_START,0,0,0); stimulation=true; }
  }

  void slotStopStimulation() {
   if (stimulation) { stimSendCommand(CS_STIM_STOP,0,0,0); stimulation=false; }
  }

  void slotPauseStimulation() {
   if (stimulation) { stimSendCommand(CS_STIM_PAUSE,0,0,0); stimulation=false; }
  }

  void slotResumeStimulation() {
   //if (!stimulation) {
    stimSendCommand(CS_STIM_RESUME,0,0,0); stimulation=true;
   //}
  }

  void slotStartTrigger() {
   if (!trigger) { stimSendCommand(CS_TRIG_START,0,0,0); trigger=true; }
  }

  void slotStopTrigger() {
   if (trigger) { stimSendCommand(CS_TRIG_STOP,0,0,0); trigger=false; }
  }

  void slotLightsOn() { stimSendCommand(CS_STIM_LIGHTS_ON,0,0,0); }
  void slotLightsDimm() { stimSendCommand(CS_STIM_LIGHTS_DIMM,0,0,0); }
  void slotLightsOff() { stimSendCommand(CS_STIM_LIGHTS_OFF,0,0,0); }

  // *** RELATIVELY SIMPLE COMMANDS TO SERVERS ***

  void slotTestSineCosine() { stimSendCommand(CS_STIM_SET_PARADIGM,TEST_SINECOSINE,0,0); }
  void slotTestSquare() { stimSendCommand(CS_STIM_SET_PARADIGM,TEST_SQUARE,0,0); }

  void slotParadigmLoadPattern() {
   QString patFileName=QFileDialog::getOpenFileName(0,
                      "Open Pattern File",".","ASCII Pattern Files (*.arp)");
   if (!patFileName.isEmpty()) { patFile.setFileName(patFileName);
    patFile.open(QIODevice::ReadOnly); patStream.setDevice(&patFile);
    pattern=patStream.readAll(); patFile.close(); // Close pattern file.

    stimCommandSocket->connectToHost(confHost,confCommP);
    stimCommandSocket->waitForConnected();

    // We want to be ready when the answer comes..
    connect(stimCommandSocket,SIGNAL(readyRead()),(QObject *)this,SLOT(slotReadAcknowledge()));

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
    stimDataSocket->connectToHost(confHost,confDataP);
    stimDataSocket->waitForConnected();
    QDataStream stimDataStream(stimDataSocket);

    int dataCount=0; pattDatagram.magic_number=0xaabbccdd;
    for (int i=0;i<pattern.size();i++) {
     pattDatagram.data[dataCount]=pattern.at(i).toLatin1(); dataCount++;
     if (dataCount==128) { // We got 128 bytes.. Send packet and Sync.
      pattDatagram.size=dataCount; dataCount=0;
      stimDataStream.writeRawData((const char*)(&pattDatagram),sizeof(patt_datagram));
      stimDataSocket->flush();
     }
    }
    if (dataCount!=0) { // Last fragment whose size is !=0 and <128
     pattDatagram.size=dataCount; dataCount=0;
     stimDataStream.writeRawData((const char*)(&pattDatagram),sizeof(patt_datagram));
     stimDataSocket->flush();
    }
    stimDataSocket->disconnectFromHost();
    disconnect(stimCommandSocket,SIGNAL(readyRead()),this,SLOT(slotReadAcknowledge()));
   }
  }

  void slotParadigmClick() { stimSendCommand(CS_STIM_SET_PARADIGM,PARA_CLICK,0,0); }
  void slotParadigmSquareBurst() { stimSendCommand(CS_STIM_SET_PARADIGM,PARA_SQUAREBURST,0,0); }
  void slotParadigmIIDITD() { stimSendCommand(CS_STIM_SET_PARADIGM,PARA_IIDITD,0,0); }
  void slotParadigmIIDITD_MonoL() { stimSendCommand(CS_STIM_SET_PARADIGM,PARA_IIDITD,1,0); }
  void slotParadigmIIDITD_MonoR() { stimSendCommand(CS_STIM_SET_PARADIGM,PARA_IIDITD,2,0); }
  void slotParadigmITDOppChn() { stimSendCommand(CS_STIM_SET_PARADIGM,PARA_ITD_OPPCHN,0,0); }
  void slotParadigmITDOppChn2() { stimSendCommand(CS_STIM_SET_PARADIGM,PARA_ITD_OPPCHN2,0,0); }
  void slotParadigmITDLinTest() { stimSendCommand(CS_STIM_SET_PARADIGM,PARA_ITD_LINTEST,0,0); }
  void slotParadigmITDLinTest2() { stimSendCommand(CS_STIM_SET_PARADIGM,PARA_ITD_LINTEST2,0,0); }
  void slotParadigmTompres() { stimSendCommand(CS_STIM_SET_PARADIGM,PARA_ITD_TOMPRES,0,0); }
  void slotParadigmPipCTrain() { stimSendCommand(CS_STIM_SET_PARADIGM,PARA_ITD_PIP_CTRAIN,0,0); }
  void slotParadigmPipRand() { stimSendCommand(CS_STIM_SET_PARADIGM,PARA_ITD_PIP_RAND,0,0); }
  void slotParadigm0021a() { stimSendCommand(CS_STIM_SET_PARADIGM,PARA_0021A,0,0); }
  void slotParadigm0021a1() { stimSendCommand(CS_STIM_SET_PARADIGM,PARA_0021A1,0,0); }
  void slotParadigm0021a2() { stimSendCommand(CS_STIM_SET_PARADIGM,PARA_0021A2,0,0); }
  void slotParadigm0021a3() { stimSendCommand(CS_STIM_SET_PARADIGM,PARA_0021A3,0,0); }
  void slotParadigm0021b() { stimSendCommand(CS_STIM_SET_PARADIGM,PARA_0021B,0,0); }
  void slotParadigm0021c() { stimSendCommand(CS_STIM_SET_PARADIGM,PARA_0021C,0,0); }
  void slotParadigm0021d() { stimSendCommand(CS_STIM_SET_PARADIGM,PARA_0021D,0,0); }
  void slotParadigm0021e() { stimSendCommand(CS_STIM_SET_PARADIGM,PARA_0021E,0,0); }

 private:
  QApplication *application;
  QTcpSocket *stimCommandSocket,*stimDataSocket; bool stimulation,trigger;
  QFile patFile; QTextStream patStream; cs_command csCmd,csAck;

  // NET
  QString confHost; int confCommP,confDataP; QString pattern; patt_datagram pattDatagram;

  // GUI
  int guiX,guiY,guiW,guiH;

  QStatusBar *guiStatusBar; QMenuBar *menuBar;
  QAction *rebootAction,*shutdownAction,*aboutAction,*quitAction,
          *testSCAction,*testSquareAction,
          *paraLoadPatAction,*paraClickAction,*paraSquareBurstAction,
          *paraIIDITDAction,*paraIIDITD_ML_Action,*paraIIDITD_MR_Action,
	  *paraITDOppChnAction,*paraITDOppChn2Action,
	  *paraITDLinTestAction,*paraITDLinTest2Action,
	  *paraITDTompresAction,*paraITDPipCTrainAction,*paraITDPipRandAction,
	  *para0021aAction,*para0021a1Action,*para0021a2Action,*para0021a3Action,
	  *para0021bAction,*para0021cAction,*para0021dAction,*para0021eAction;

  QPushButton *lightsOnButton,*lightsDimmButton,*lightsOffButton,
	      *startStimulationButton,*stopStimulationButton,
	      *pauseStimulationButton,*resumeStimulationButton,
	      *startTriggerButton,*stopTriggerButton;
};

#endif
