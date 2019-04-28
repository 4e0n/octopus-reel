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

#ifndef OCTOPUS_CHANNEL_H
#define OCTOPUS_CHANNEL_H

#include <QObject>
#include <QString>
#include "channel_params.h"
#include "coord3d.h"
#include "../../../common/vec3.h"

class Channel : QObject {
 public:
  Channel(int pc,QString n,int chRejLev,int chRejRef,QString cv,QString cr,
          QString av,QString ar,float th,float ph) : QObject() {
   physChn=pc-1; name=n; rejLev=(float)chRejLev; rejRef=chRejRef;
   cntVis = (cv=="T" || cv=="t") ? true : false;
   cntRec = (cr=="T" || cr=="t") ? true : false;
   avgVis = (av=="T" || av=="t") ? true : false;
   avgRec = (ar=="T" || ar=="t") ? true : false;
   param.y=th; param.z=ph; real=Vec3(1000.,0.,0.); realS.zero();
  }
  ~Channel() {}

  void setEventProfile(int eventCount,int dataCount) {
   QVector<float> *dummyAS;
   for (int i=0;i<avgData.size();i++) { delete avgData[i]; delete stdData[i]; }
   for (int i=0;i<eventCount;i++) {
    dummyAS=new QVector<float>(dataCount); avgData.append(dummyAS);
    dummyAS=new QVector<float>(dataCount); stdData.append(dummyAS);
   }
  }

  void resetEvents() {
   for (int i=0;i<avgData.size();i++) {
    for (int j=0;j<avgData[i]->size();j++)
     (*(avgData[i]))[j]=(*(stdData[i]))[j]=0.;
   }
  }

  // Continuous and Average visibility and recording flags exist as strings
  // in the constructor, which is how they are read from the config file..
  int physChn,rejRef; QString name; bool cntVis,cntRec,avgVis,avgRec;

  Coord3D param; Vec3 real,realS; // Parametric and realistic coords..

  QVector<float> pastData,pastFilt; float rejLev;
  QVector<QVector<float>* > avgData,stdData;
  float notchLevel; QColor notchColor; // Instantly computed line noise level..
};

#endif
