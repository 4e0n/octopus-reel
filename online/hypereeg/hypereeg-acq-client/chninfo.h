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

#ifndef CHNINFO_H
#define CHNINFO_H

#include "../../../common/vec3.h"
//#include "confparam.h"
//#include "coord3d.h"

class ChnInfo {
 public:
  ChnInfo() { topoR=1.0; }

  void resetEvents() { // Zero all epoch data for all event types.
   for (int i=0;i<avgData.size();i++) for (int j=0;j<avgData[i].size();j++) avgData[i][j]=0.;
   //for (int i=0;i<stdData.size();i++) for (int j=0;j<stdData[i].size();j++) stdData[i][j]=0.;
  }

  void setEvents(int eventCount,int dataCount) {
   avgData.resize(eventCount); for (int i=0;i<avgData.size();i++) avgData[i].resize(dataCount);
   stdData.resize(eventCount); for (int i=0;i<stdData.size();i++) stdData[i].resize(dataCount);
  }

  unsigned int physChn;
  QString chnName;
  float topoR,topoTheta,topoPhi;
  bool isBipolar;

  QVector<int> rejRef; // Channels as reference for rejection (mean of them)
  float rejLev;
  Vec3 real,realS; // Realistic coords - if set externally.

  QVector<QVector<float> > avgData,stdData;
  //QVector<float> pastData,pastFilt;
  float cmLevel; QColor cmColor; // Instantly computed line noise level..
// private:
//  ConfParam *confParam;

  // Continuous and Average visibility and recording flags exist as strings
  // in the constructor, which is how they are read from the config file..
};

#endif
