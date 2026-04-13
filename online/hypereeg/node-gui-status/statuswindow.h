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

#pragma once

#include <QApplication>
#include <QMainWindow>
#include <QTableWidget>
#include <QHeaderView>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QMessageBox>
#include <QPushButton>
#include <QBrush>
#include <QColor>
#include "confparam.h"

class StatusWindow:public QMainWindow {
 Q_OBJECT
 public:
  explicit StatusWindow(ConfParam *c=nullptr,QWidget *parent=nullptr):QMainWindow(parent) {
   conf=c;

   setGeometry(conf->guiX,conf->guiY,conf->guiW,conf->guiH); setFixedSize(conf->guiW,conf->guiH);
   setWindowTitle("Octopus-HyperEEG Nodes Status");

   statusBar=new QStatusBar(this); setStatusBar(statusBar);
   menuBar=new QMenuBar(this); menuBar->setFixedWidth(width());
   QMenu *sysMenu=new QMenu("&System",menuBar); menuBar->addMenu(sysMenu); setMenuBar(menuBar);

   rebootAction=new QAction("&Reboot Servers",this);
   shutdownAction=new QAction("&Shutdown Servers",this);
   aboutAction=new QAction("&About..",this);
   quitAction=new QAction("&Quit",this);

   rebootAction->setStatusTip("Reboot all configured server nodes");
   shutdownAction->setStatusTip("Shutdown all configured server nodes");
   aboutAction->setStatusTip("About HyperEEG node status");
   quitAction->setStatusTip("Quit this GUI");

   connect(rebootAction,SIGNAL(triggered()),this,SLOT(slotReboot()));
   connect(shutdownAction,SIGNAL(triggered()),this,SLOT(slotShutdown()));
   connect(aboutAction,SIGNAL(triggered()),this,SLOT(slotAbout()));
   connect(quitAction,SIGNAL(triggered()),this,SLOT(slotQuit()));

   sysMenu->addAction(rebootAction); sysMenu->addAction(shutdownAction);
   sysMenu->addSeparator();
   sysMenu->addAction(aboutAction);
   sysMenu->addSeparator();
   sysMenu->addAction(quitAction);

   table=new QTableWidget(this);
   table->setColumnCount(5); table->setRowCount(conf->nodes.size());
   table->setHorizontalHeaderLabels({"Status","Node","IP","Port","RTT(ms)"});
   table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
   table->verticalHeader()->setVisible(false);
   table->setEditTriggers(QAbstractItemView::NoEditTriggers);
   table->setSelectionMode(QAbstractItemView::NoSelection);
   setCentralWidget(table);

   for (int i=0;i<conf->nodes.size();i++)
    for (int j=0;j<table->columnCount();j++) table->setItem(i,j,new QTableWidgetItem);

   refreshTable();
  }

  void refreshTable() {
   for (int i=0;i<conf->nodes.size();i++) {
    const auto &n=conf->nodes[i];

    auto *it0=table->item(i,0);
    auto *it1=table->item(i,1);
    auto *it2=table->item(i,2);
    auto *it3=table->item(i,3);
    auto *it4=table->item(i,4);

    if (n.checking)    { it0->setText("..."); it0->setBackground(QBrush(QColor(255,220,120)));  }
    else if (n.online) { it0->setText("OK"); it0->setBackground(QBrush(QColor(120,220,120)));   }
    else               { it0->setText("DOWN"); it0->setBackground(QBrush(QColor(230,100,100))); }

    it1->setText(n.name); it2->setText(n.ipAddr); it3->setText(QString::number(n.commPort));
    it4->setText(n.lastRttMs>=0 ? QString::number(n.lastRttMs) : "-");
   }
  }

 private slots:
  void slotReboot() {
   if (!confirmAndProceed("Reboot")) return;
   statusBar->showMessage("Sending reboot command to nodes...");
   QString summary=conf->broadcastCommand(CMD_REBOOT);
   //QMessageBox::information(this,"Reboot summary",summary);
   statusBar->showMessage("Reboot command finished",3000);
  }

  void slotShutdown() {
   if (!confirmAndProceed("Shutdown")) return;

   statusBar->showMessage("Sending shutdown command to nodes...");
   QString summary=conf->broadcastCommand(CMD_SHUTDOWN);
   //QMessageBox::information(this,"Shutdown summary",summary);
   statusBar->showMessage("Shutdown command finished",3000);
  }

  void slotAbout() {
   QMessageBox::about(this,"About Octopus-ReEL HyperEEG Network",
                      "(c) 2007-2026 Barkin Ilhan (barkin@unrlabs.org)\n"
                      "This is free software coming with\n"
                      "ABSOLUTELY NO WARRANTY; You are welcome\n"
                      "to extend/redistribute it under conditions of GPL v3.\n");
  }

  void slotQuit() {
   conf->requestQuit();
   QApplication::quit();
  }

 private:
  bool confirmAndProceed(const QString &actionWord) {
   QMessageBox msgBox(this);
   msgBox.setIcon(QMessageBox::Warning);
   msgBox.setWindowTitle(actionWord);
   msgBox.setText(actionWord+" all server nodes?");
   msgBox.setInformativeText("Normally this should be done only after today's scheduled experiments are complete.");
   QPushButton *yesBtn=msgBox.addButton(actionWord,QMessageBox::AcceptRole);
   msgBox.addButton(QMessageBox::Cancel);
   msgBox.setDefaultButton(QMessageBox::Cancel);
   msgBox.exec();
   return (msgBox.clickedButton()==yesBtn);
  }

  QStatusBar *statusBar=nullptr;
  QMenuBar *menuBar=nullptr;
  QAction *rebootAction=nullptr;
  QAction *shutdownAction=nullptr;
  QAction *aboutAction=nullptr;
  QAction *quitAction=nullptr;

  ConfParam *conf=nullptr;
  QTableWidget *table=nullptr;
};
