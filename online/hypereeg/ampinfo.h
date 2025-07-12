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

#ifndef _AMPINFO_H
#define _AMPINFO_H

typedef struct _AmpInfo {
 unsigned int ampCount;
 unsigned int sampleRate;
 unsigned int tcpBufSize;
 unsigned int refChnCount,bipChnCount;
 unsigned int physChnCount; // refChnCount+bipChnCount
 unsigned int refChnMaxCount,bipChnMaxCount;
 unsigned int physChnMaxCount;
 unsigned int totalChnCount; // refChnCount+bipChnCount+2
 unsigned int totalCount; // Chncount among all connected amplifiers, i.e. ampCount*totalChnCount
 float refGain,bipGain;
 unsigned int eegProbeMsecs;
} AmpInfo;

#endif
