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

/* This application generates the randomized pattern which streams in background
   in the ITD Opponent channels protocol
*/   

#include <QString>
#include <QStringList>
#include <QIntValidator>
#include <QFile>
#include <QTextStream>

int main(int argc,char *argv[]) {
 int patternCount,domainSize;
 QFile patternFile; QTextStream patternStream;
 QString finalPattern,triggerDomain; QChar c;

 if (argc!=3) {
  qDebug("Usage: octopus-patt <file name> <stim.length in trials>");
  return -1;
 }

 QString p=QString::fromLatin1(argv[2]);
 QIntValidator intValidator(1,1800,NULL); int pos=0;
 if (intValidator.validate(p,pos) != QValidator::Acceptable) {
  qDebug("Please enter a positive integer between 1 and 1800 as stim.length!");
  qDebug("Usage: octopus-patt <file name> <size @stim.length>");
  return -1;
 }
 patternCount=p.toInt();

 patternFile.setFileName(argv[1]);
 if (!patternFile.open(QIODevice::WriteOnly)) {
  qDebug("Pattern error: Cannot generate/open pattern file for writing.");
  return -1;
 }
 patternStream.setDevice(&patternFile);

 std::srand(time(NULL));

 triggerDomain="0ABC";
 domainSize=triggerDomain.length();

 while (finalPattern.size() < patternCount) {
  c=triggerDomain[rand()%domainSize];
  finalPattern.append(c);
 }

 qDebug("Outputting to file..");
 for (int i=0;i<patternCount;i++) patternStream << finalPattern[i];

 qDebug("Exiting..");
 patternStream.setDevice(0);
 patternFile.close();
 return 0;
}

