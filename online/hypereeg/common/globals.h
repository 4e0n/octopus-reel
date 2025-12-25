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

#define EEMAGINE
#define AUDIODEV

#define HACQ_VERBOSE

#define OCTO_OMP
#include "octo_omp.h"

const unsigned int EE_MAX_AMPCOUNT=8;
const unsigned int REF_CHN_MAXCOUNT=64;
const unsigned int BIP_CHN_MAXCOUNT=24;
const unsigned int TRIG_AMPSYNC=0xFF;

const QString basePath="/opt/octopus/";
const QString confPath=basePath+"conf/";
const QString dataPath=basePath+"data/";
const QString synthDataPath=dataPath+"raweeg/synth-eeg.raw";
const QString hyperConfPath=confPath+"online/hypereeg/";
