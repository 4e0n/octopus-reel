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

/* This application generates the orderly pattern which streams in background
   in the ITD Opponent channels protocol
*/   

#include <QString>
#include <QStringList>
#include <QIntValidator>
#include <QFile>
#include <QTextStream>

static int patternCount;
static QString dummyString;

int main(int argc,char *argv[]) {
 if (argc!=3) {
  qDebug("Usage: octopus-patt <file name> <stim.length in trials>");
  return -1;
 }

 QString p=QString::fromAscii(argv[2]);
 QIntValidator intValidator(30,1800,NULL); int pos=0;
 if (intValidator.validate(p,pos) != QValidator::Acceptable) {
  qDebug("Please enter a positive integer between 30 and 1800 as stim.length!");
  qDebug("Usage: octopus-patt <file name> <seconds @ 50ms stim.length>");
  return -1;
 }
 patternCount=p.toInt();

 QFile patternFile; QTextStream patternStream;
 patternFile.setFileName(argv[1]);
 if (!patternFile.open(QIODevice::WriteOnly)) {
  qDebug("Pattern error: Cannot generate/open pattern file for writing.");
  return -1;
 }
 patternStream.setDevice(&patternFile);


 QString finalPattern; // Add random 20lets..
 int idx=0;
 while (finalPattern.size() < patternCount) {
  dummyString='A'+idx;
  finalPattern.append(dummyString);
  idx++; idx%=9;
 }

 qDebug("Outputting to file..");
 for (int i=0;i<patternCount;i++) patternStream << finalPattern[i];

 qDebug("Exiting..");
 patternStream.setDevice(0);
 patternFile.close();
 return 0;
}
