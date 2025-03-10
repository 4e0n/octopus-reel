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

#ifndef GENPAR_W_H
#define GENPAR_W_H

#include <QtGui>
#include <QString>
#include "octopus_seg_master.h"

class GenParW : public QWidget {
 Q_OBJECT
 public:
  GenParW(SegMaster *sm) : QWidget(0,Qt::SubWindow) { p=sm;
   setGeometry(540,40,500,300); setFixedSize(500,300);
   setWindowTitle("General Settings");
  }

 private slots:

 private:
  SegMaster *p; QString dummyString;
};

#endif
