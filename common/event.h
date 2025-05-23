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

#ifndef EVENT_H
#define EVENT_H

#include <QObject>
#include <QString>

class Event : QObject {
 public:
  Event(int evtNo,QString evtName,int evtType,QColor c) : QObject() {
   no=evtNo; name=evtName; type=evtType; accepted=rejected=0; color=c;
  }
  ~Event() {}

  int no,type,accepted,rejected; QString name; QColor color;
};

#endif
