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

/* STD1 Definition:
   This application generates the randomized pattern of E elements
   i.e., for a session with N blocks of M randomized trials shuffled within,
   i.e., E=N*M
*/   

#include <QString>
#include <QStringList>
#include <QIntValidator>
#include <QFile>
#include <QTextStream>
#include <QVector>
#include <stdio.h>

QVector<int> randomIndex(int length) {
 QVector<int> indices; int pivot,x;
 for (int i=0;i<length;i++) indices.append(i);
 for (int i=0;i<length-1;i++) {
  pivot=(rand()%(length-i))+i;
//  printf("%d ",pivot-i);
  x=indices[i]; indices[i]=indices[pivot]; indices[pivot]=x;
 } 
// for (int i=0;i<length;i++) printf("%d ",indices[i]);
// printf("\n");
 return indices;
}

QString shuffle(QString s) {
 QString shuffled;
 QVector<int> indices=randomIndex(s.length());
// for (int i=0;i<s.length();i++) printf("%d ",indices[i]);
// printf("\n");
 for (int i=0;i<s.length();i++) shuffled.append(s[indices[i]]);
 return shuffled;
}

int main(int argc,char *argv[]) {
 int trialCount,blockCount,domainSize,pos;
 QFile patternFile; QTextStream patternStream;
 QString p,finalPattern,triggerDomain,blockDomain; QChar c;

 if (argc!=4) {
  qDebug("Usage: octopus-patt <file name> <# of blocks> <# of trials in one block>");
  return -1;
 }

 p=QString::fromLatin1(argv[3]); pos=0;
 QIntValidator intValidator1(4,1800,NULL);
 if (intValidator1.validate(p,pos) != QValidator::Acceptable) {
  qDebug("Please enter a positive integer between 5 and 1800 as # of trials per block!");
  qDebug("Usage: octopus-patt <file name> <# of blocks> <# of trials in one block>");
  return -1;
 }
 trialCount=p.toInt();

 p=QString::fromLatin1(argv[2]); pos=0;
 QIntValidator intValidator2(1,1000,NULL);
 if (intValidator2.validate(p,pos) != QValidator::Acceptable) {
  qDebug("Please enter a positive integer between 1 and 1000 as # of blocks!");
  qDebug("Usage: octopus-patt <file name> <# of blocks> <# of trials in one block>");
  return -1;
 }
 blockCount=p.toInt();

 patternFile.setFileName(argv[1]); pos=0;
 if (!patternFile.open(QIODevice::WriteOnly)) {
  qDebug("Pattern error: Cannot generate/open pattern file for writing.");
  return -1;
 }
 patternStream.setDevice(&patternFile);

 std::srand(time(NULL));

 triggerDomain="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
 domainSize=triggerDomain.length();

 for (int i=0;i<trialCount;i++) {
  blockDomain.append(triggerDomain[i%domainSize]); 
 }

 for (int i=0;i<blockCount;i++) {
  finalPattern.append('.'); // "randomize ISI" command in between blocks
  finalPattern.append(shuffle(blockDomain));
  // if (i<blockCount-1) finalPattern.append('@');
 }

 qDebug("Outputting to file..");
 for (int i=0;i<finalPattern.length();i++) patternStream << finalPattern[i];

 qDebug("Exiting..");
 patternStream.setDevice(0);
 patternFile.close();
 return 0;
}

