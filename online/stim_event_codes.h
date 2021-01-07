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

/* The codes of default STIM events. */

#ifndef STIM_EVENT_CODES_H
#define STIM_EVENT_CODES_H

// TESTS
#define SEC_NO_EVENT		0	// "NoEvent"
#define SEC_TRIGTEST		1	// "Trigger Test"
#define SEC_GEN_SINE		2	// "Gen.Sine"
#define SEC_GEN_COSINE		3	// "Gen.Cosine"
#define SEC_SQUARE_WAVE		4	// "SquareWave"
#define SEC_STEADY_WAVE		5	// "SteadyWave"
#define	SEC_EVENT_6		6	// "EVENT#6"
#define	SEC_EVENT_7		7	// "EVENT#7"
#define	SEC_EVENT_8		8	// "EVENT#8"
#define	SEC_EVENT_9		9	// "EVENT#9"
// CONVENTIONAL EVENTS
#define SEC_CLICK		10	// "Click"
#define SEC_SQUARE_BURST	11	// "SquareBurst"
#define SEC_P300_1		12	// "P300-1"
#define SEC_P300_2		13	// "P300-2"
#define SEC_MMN_STD		14	// "MMN-Standard"
#define SEC_MMN_DEV		15	// "MMN-Deviant"
#define SEC_EVENT_16		16	// ""
#define SEC_EVENT_17		17	// ""
#define SEC_EVENT_18		18	// ""
#define SEC_EVENT_19		19	// ""
#define SEC_EVENT_20		20	// ""
#define SEC_EVENT_21		21	// ""
#define SEC_EVENT_22		22	// ""
#define SEC_EVENT_23		23	// ""
#define SEC_EVENT_24		24	// ""
#define SEC_EVENT_25		25	// ""
#define SEC_EVENT_26		26	// ""
#define SEC_EVENT_27		27	// ""
#define SEC_EVENT_28		28	// ""
#define SEC_EVENT_29		29	// ""
#define SEC_EVENT_30		30	// ""
#define SEC_EVENT_31		31	// ""
#define SEC_EVENT_32		32	// ""
#define SEC_EVENT_33		33	// ""
#define SEC_EVENT_34		34	// ""
#define SEC_EVENT_35		35	// ""
#define SEC_EVENT_36		36	// ""
#define SEC_EVENT_37		37	// ""
#define SEC_EVENT_38		38	// ""
#define SEC_EVENT_39		39	// ""
#define SEC_EVENT_40		40	// ""
#define SEC_IID_LEFT		41	// "IID Left" 
#define SEC_ITD_LEFT		42	// "ITD Left" 
#define SEC_IID_RIGHT		43	// "IID Right" 
#define SEC_ITD_RIGHT		44	// "ITD Right" 
#define SEC_EVENT_45		45	// ""
#define SEC_EVENT_46		46	// ""
#define SEC_EVENT_47		47	// ""
#define SEC_EVENT_48		48	// ""
#define SEC_EVENT_49		49	// ""
#define SEC_PP_CENTER		50	// "PP CENTER",
#define SEC_PP_IID_L6		51	// "PP IID -6",
#define SEC_PP_IID_L5		52	// "PP IID -5",
#define SEC_PP_IID_L4		53	// "PP IID -4",
#define SEC_PP_IID_L3		54	// "PP IID -3",
#define SEC_PP_IID_L2		55	// "PP IID -2",
#define SEC_PP_IID_L1		56	// "PP IID -1",
#define SEC_PP_IID_R1		57	// "PP IID +1",
#define SEC_PP_IID_R2		58	// "PP IID +2",
#define SEC_PP_IID_R3		59	// "PP IID +3",
#define SEC_PP_IID_R4		60	// "PP IID +4",
#define SEC_PP_IID_R5		61	// "PP IID +5",
#define SEC_PP_IID_R6		62	// "PP IID +6",
#define SEC_PP_ITD_L6		63	// "PP ITD -6",
#define SEC_PP_ITD_L5		64	// "PP ITD -5",
#define SEC_PP_ITD_L4		65	// "PP ITD -4",
#define SEC_PP_ITD_L3		66	// "PP ITD -3",
#define SEC_PP_ITD_L2		67	// "PP ITD -2",
#define SEC_PP_ITD_L1		68	// "PP ITD -1",
#define SEC_PP_ITD_R1		69	// "PP ITD +1",
#define SEC_PP_ITD_R2		70	// "PP ITD +2",
#define SEC_PP_ITD_R3		71	// "PP ITD +3",
#define SEC_PP_ITD_R4		72	// "PP ITD +4",
#define SEC_PP_ITD_R5		73	// "PP ITD +5",
#define SEC_PP_ITD_R6		74	// "PP ITD +6",
#define SEC_EVENT_75		75	// ""
#define SEC_EVENT_76		76	// ""
#define SEC_EVENT_77		77	// ""
#define SEC_EVENT_78		78	// ""
#define SEC_EVENT_79		79	// ""
#define SEC_OPPCHN_CO		80	// "OPPCHN CO"
#define SEC_OPPCHN_CC		81	// "OPPCHN CC"
#define SEC_OPPCHN_CL		82	// "OPPCHN CL"
#define SEC_OPPCHN_CR		83	// "OPPCHN CR"
#define SEC_OPPCHN_LC		84	// "OPPCHN LC"
#define SEC_OPPCHN_LL		85	// "OPPCHN LL"
#define SEC_OPPCHN_LR		86	// "OPPCHN LR"
#define SEC_OPPCHN_RC		87	// "OPPCHN RC"
#define SEC_OPPCHN_RL		88	// "OPPCHN RL"
#define SEC_OPPCHN_RR		89	// "OPPCHN RR"
#define SEC_EVENT_90		90	// ""
#define SEC_EVENT_91		91	// ""
#define SEC_EVENT_92		92	// ""
#define SEC_EVENT_93		93	// ""
#define SEC_EVENT_94		94	// ""
#define SEC_EVENT_95		95	// ""
#define SEC_EVENT_96		96	// ""
#define SEC_EVENT_97		97	// ""
#define SEC_EVENT_98		98	// ""
#define SEC_EVENT_99		99	// ""
#define SEC_EVENT_100		100	// ""
#define SEC_EVENT_101		101	// ""
#define SEC_EVENT_102		102	// ""
#define SEC_EVENT_103		103	// ""
#define SEC_EVENT_104		104	// ""
#define SEC_EVENT_105		105	// ""
#define SEC_EVENT_106		106	// ""
#define SEC_EVENT_107		107	// ""
#define SEC_EVENT_108		108	// ""
#define SEC_EVENT_109		109	// ""
#define SEC_EVENT_110		110	// ""
#define SEC_EVENT_111		111	// ""
#define SEC_EVENT_112		112	// ""
#define SEC_EVENT_113		113	// ""
#define SEC_EVENT_114		114	// ""
#define SEC_EVENT_115		115	// ""
#define SEC_EVENT_116		116	// ""
#define SEC_EVENT_117		117	// ""
#define SEC_EVENT_118		118	// ""
#define SEC_EVENT_119		119	// ""
#define SEC_EVENT_120		120	// ""
#define SEC_EVENT_121		121	// ""
#define SEC_EVENT_122		122	// ""
#define SEC_EVENT_123		123	// ""
#define SEC_EVENT_124		124	// ""
#define SEC_EVENT_125		125	// ""
#define SEC_PAUSE		126	// "Pause"
// CALIBRATION EVENT
#define SEC_CALIBRATION		127	// "Calibration"

#endif
