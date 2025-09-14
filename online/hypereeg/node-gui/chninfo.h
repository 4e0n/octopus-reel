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

#pragma once

#include "../../../common/vec3.h"
#include "../../../common/coord3d.h"

class ChnInfo {
 public:
  ChnInfo() { param.x=1.0; cmColor=QColor(255,255,255,128); }

//  void resetEvents() { // Zero all epoch data for all event types.
//   for (int ampIdx=0;ampIdx<conf->ampCount;ampIdx++)
//    for (int evtIdx=0;evtIdx<conf->eventCodes.size();evtIdx++)
//     for (int smpIdx=0;smpIdx<conf->avgSmpCount;smpIdx++) avgData[ampIdx][evtIdx][smpIdx]=0.;
//  }

//  void setEvents(int eventCount,int dataCount) {
//   avgData.resize(eventCount); for (int i=0;i<avgData.size();i++) avgData[i].resize(dataCount);
//  }

  unsigned int physChn; QString chnName;
  int topoX,topoY; bool isBipolar;

  QVector<int> rejRef; // Channels as reference for rejection (mean of them)
  float rejLev;
  Vec3 real,realS; // Realistic coords - if set externally.
  Coord3D param;

  QVector<QVector<QVector<float>>> avgData;
  float cmLevel; QColor cmColor; // Instantly computed line noise level..

  QVector<unsigned int> interElec;

  QVector<bool> interActive; // for each amp

  // Continuous and Average visibility and recording flags exist as strings
  // in the constructor, which is how they are read from the config file..
};
