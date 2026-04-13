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

#include <QMainWindow>
#include "confparam.h"
#include "cmframe.h"

class CMWindow : public QMainWindow {
 Q_OBJECT
 public:
  explicit CMWindow(ConfParam *c=nullptr,QWidget *parent=nullptr):QMainWindow(parent) {
   conf=c; setGeometry(conf->guiX,conf->guiY,conf->guiW,conf->guiH); setFixedSize(conf->guiW,conf->guiH);

   CMFrame *cmF;
   for (unsigned int ampIdx=0; ampIdx<conf->ampCount; ampIdx++) {
    cmF=new CMFrame(conf,ampIdx,this); cmF->show(); cmFrames.append(cmF);
   }

   setWindowTitle("Octopus-HyperEEG Common-Mode (Mains) Noise Levels");
  }

  ~CMWindow() override {}

  void updateCMFrames() { for (auto *f:cmFrames) if (f) f->refreshImage(); }

  void showNode() { show(); raise(); activateWindow(); }

  void hideNode() { hide(); }

  void setPaletteMode(const QString &mode) {
   CMFrame::CenterMode m=CMFrame::UseMean;
   if (mode=="MEDIAN") m=CMFrame::UseMedian;
   for (auto *f:cmFrames) { if (f) f->setCenterMode(m); }
  }

  ConfParam *conf=nullptr;

 private:
  QVector<CMFrame*> cmFrames;
};
