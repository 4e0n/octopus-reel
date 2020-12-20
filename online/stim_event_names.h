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

/* The names of default STIM events. */

#ifndef STIM_EVENT_NAMES_H
#define STIM_EVENT_NAMES_H

#include <QString>

const QString stimEventNames[128]={
 // TESTS
 "NoEvent",	//   0
 "Trig.Test",	//   1
 "Gen.Sine",	//   2
 "Gen.Cosine",	//   3
 "SquareWave",	//   4
 "SteadyWave",	//   5
 "STIM_006",	//   6
 "STIM_007",	//   7
 "STIM_008",	//   8
 "STIM_009",	//   9
 // CONVENTIONAL EVENTS
 "Click",	//  10
 "SquareBurst",	//  11 
 "P300-1",	//  12 
 "P300-2",	//  13 
 "MMN-Standard",//  14 
 "MMN-Deviant",	//  15 
 "STIM_016",	//  16
 "STIM_017",	//  17
 "STIM_018",	//  18
 "STIM_019",	//  19
 "OppC1-A",	//  20
 "OppC1-B",	//  21
 "STIM_022",	//  22
 "STIM_023",	//  23
 "STIM_024",	//  24
 "STIM_025",	//  25
 "STIM_026",	//  26
 "STIM_027",	//  27
 "STIM_028",	//  28
 "STIM_029",	//  29
 "STIM_030",	//  30
 "STIM_031",	//  31
 "STIM_032",	//  32
 "STIM_033",	//  33
 "STIM_034",	//  34
 "STIM_035",	//  35
 "STIM_036",	//  36
 "STIM_037",	//  37
 "STIM_038",	//  38
 "STIM_039",	//  39
 "STIM_040",	//  40
 "IID Left",    //  41 
 "ITD Left",    //  42 
 "IID Right",   //  43 
 "ITD Right",   //  44 
 "STIM_045",	//  45
 "STIM_046",	//  46
 "STIM_047",	//  47
 "STIM_048",	//  48
 "STIM_049",	//  49
 "PP CENTER",   //  50 
 "PP IID -6",   //  51 
 "PP IID -5",   //  52 
 "PP IID -4",   //  53 
 "PP IID -3",   //  54 
 "PP IID -2",   //  55 
 "PP IID -1",   //  56 
 "PP IID +1",   //  57 
 "PP IID +2",   //  58 
 "PP IID +3",   //  59 
 "PP IID +4",   //  60 
 "PP IID +5",   //  61 
 "PP IID +6",   //  62 
 "PP ITD -6",   //  63 
 "PP ITD -5",   //  64 
 "PP ITD -4",   //  65 
 "PP ITD -3",   //  66 
 "PP ITD -2",   //  67 
 "PP ITD -1",   //  68 
 "PP ITD +1",   //  69 
 "PP ITD +2",   //  70 
 "PP ITD +3",   //  71 
 "PP ITD +4",   //  72 
 "PP ITD +5",   //  73 
 "PP ITD +6",   //  74 
 "STIM_075",	//  75
 "STIM_076",	//  76
 "STIM_077",	//  77
 "STIM_078",	//  78
 "STIM_079",	//  79
 "STIM_080",	//  80
 "STIM_081",	//  81
 "STIM_082",	//  82
 "STIM_083",	//  83
 "STIM_084",	//  84
 "STIM_085",	//  85
 "STIM_086",	//  86
 "STIM_087",	//  87
 "STIM_088",	//  88
 "STIM_089",	//  89
 "STIM_090",	//  90
 "STIM_091",	//  91
 "STIM_092",	//  92
 "STIM_093",	//  93
 "STIM_094",	//  94
 "STIM_095",	//  95
 "STIM_096",	//  96
 "STIM_097",	//  97
 "STIM_098",	//  98
 "STIM_099",	//  99
 "STIM_100",	// 100
 "STIM_101",	// 101
 "STIM_102",	// 102
 "STIM_103",	// 103
 "STIM_104",	// 104
 "STIM_105",	// 105
 "STIM_106",	// 106
 "STIM_107",	// 107
 "STIM_108",	// 108
 "STIM_109",	// 109
 "STIM_110",	// 110
 "STIM_111",	// 111
 "STIM_112",	// 112
 "STIM_113",	// 113
 "STIM_114",	// 114
 "STIM_115",	// 115
 "STIM_116",	// 116
 "STIM_117",	// 117
 "STIM_118",	// 118
 "STIM_119",	// 119
 "STIM_120",	// 120
 "STIM_121",	// 121
 "STIM_122",	// 122
 "STIM_123",	// 123
 "STIM_124",	// 124
 "STIM_125",	// 125
 "Pause",       // 126 
 // CALIBRATION EVENT
 "Calibration"	// 127 
};

#endif
