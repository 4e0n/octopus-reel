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

#define OCTO_OMP
#include "octo_omp.h"

inline constexpr int HYPEREEG_ACQ_DAEMON_VER=200;

inline constexpr int EE_MAX_AMPCOUNT=8;
inline constexpr int REF_CHN_MAXCOUNT=64;
inline constexpr int BIP_CHN_MAXCOUNT=24;
inline constexpr int TRIG_AMPSYNC=0xFF;

inline constexpr int AUDIO_N=48;

inline const QString& optPath() { static const QString v="/opt/octopus/"; return v; }
inline const QString& dataPath() { static const QString v=optPath()+"data/"; return v; }
inline const QString& synthDataPath() { static const QString v=dataPath()+"raweeg/synth-eeg.raw"; return v; }
inline const QString& confPath() { static const QString v="~/.octopus-reel/"; return v; }

//const QString optPath="/opt/octopus/";
//const QString dataPath=optPath+"data/";
//const QString synthDataPath=dataPath+"raweeg/synth-eeg.raw";

//const QString confPath="~/.octopus-reel/";
