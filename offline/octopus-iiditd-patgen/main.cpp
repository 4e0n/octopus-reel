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

/* This application generates the random pattern which streams in background
   in the IIDITD protocol proposed by Dr. Pekcan Ungan

   Creation algorithm, designed by his research assistant Barkin Ilhan,
   is as follows:

   The parameter is the power of 2; the count of overall pattern in binary.

   The criteria are:
   
   1st degree prohibitions:
    i) Any subsequent 3 or more of the same bit (000,111,0000..,1111..).

   2nd degree prohibitions:
    i) Any subsequent alternating unary sequence (10101 and 01010)
   ii) Any subsequent alternating binary sequence (11001100 and 00110011)
*/   

#include <QString>
#include <QStringList>
#include <QIntValidator>
#include <QFile>
#include <QTextStream>

static int patternCount,currentEvent=2;
static QStringList pattern1,pattern2,*curPattern,*dstPattern,patt4;
static QString dummyString,dummyString2;

bool isValid(QString inputString) {
 if (!inputString.contains("000") && !inputString.contains("111") &&
     !inputString.contains("01010") && !inputString.contains("10101") &&
     !inputString.contains("00110011") && !inputString.contains("11001100"))
  return true;
 else return false;
}
 
void extend() {
 if (curPattern==&pattern1) dstPattern=&pattern2;
 else if (curPattern==&pattern2) dstPattern=&pattern1;
 else qDebug("Pointer error!");
 dstPattern->clear();
 if ((*curPattern)[0].size() < 8) {
  for (int i=0;i<curPattern->size();i++)
   for (int j=0;j<curPattern->size();j++) {
    dummyString=(*curPattern)[i]+(*curPattern)[j];
    if (isValid(dummyString))
     dstPattern->append((*curPattern)[i]+(*curPattern)[j]);
   }
 } else {
  for (int i=0;i<curPattern->size();i++)
   for (int j=0;j<curPattern->size();j++) {
    dummyString=(*curPattern)[i]+(*curPattern)[j];
    dummyString2=dummyString.remove(0,dummyString.size()/2-8);
    dummyString=dummyString2.remove(14,dummyString.size()/2-7);
    if (isValid(dummyString))
     dstPattern->append((*curPattern)[i]+(*curPattern)[j]);
   }
 }
 curPattern=dstPattern; // swap
 qDebug("Validated size (16)= %d",curPattern->size());
}

void extend20() {
 QString p1String,p2String,p3String,p4String;
 if (curPattern==&pattern1) dstPattern=&pattern2;
 else if (curPattern==&pattern2) dstPattern=&pattern1;
 else qDebug("Pointer error!");
 dstPattern->clear();
 for (int i=0;i<curPattern->size();i++) for (int j=0;j<patt4.size();j++) {
  p1String=p2String=p3String=p4String=(*curPattern)[i]+patt4[j];
  p1String.replace(0,6,"x01001"); p1String.replace(15,5,"01001");
  p2String.replace(0,6,"x01001"); p2String.replace(15,5,"01101");
  p3String.replace(0,6,"x01101"); p3String.replace(15,5,"01001");
  p4String.replace(0,6,"x01101"); p4String.replace(15,5,"01101");
  if (isValid(p1String)) dstPattern->append(p1String);
  if (isValid(p2String)) dstPattern->append(p2String);
  if (isValid(p3String)) dstPattern->append(p3String);
  if (isValid(p4String)) dstPattern->append(p4String);
 }
 curPattern=dstPattern; // swap
 qDebug("Validated size (20)= %d",curPattern->size());
}

int main(int argc,char *argv[]) {
 if (argc!=3) {
  qDebug("Usage: octopus-patt <file name> <seconds @ 50ms stim.length>");
  return -1;
 }

 QString p=QString::fromAscii(argv[2]);
 QIntValidator intValidator(30,7200,NULL); int pos=0;
 if (intValidator.validate(p,pos) != QValidator::Acceptable) {
  qDebug("Please enter a positive integer between 30 and 7200 as stim.length!");
  qDebug("Usage: octopus-patt <file name> <seconds @ 50ms stim.length>");
  return -1;
 }
 patternCount=20*p.toInt();

 QFile patternFile; QTextStream patternStream;
 patternFile.setFileName(argv[1]);
 if (!patternFile.open(QIODevice::WriteOnly)) {
  qDebug("Pattern error: Cannot generate/open pattern file for writing.");
  return -1;
 }
 patternStream.setDevice(&patternFile);

 curPattern=&pattern1;

 // Initialization
 for (int i=0;i<16;i++) {
  dummyString="";
  dummyString.append(0x30+((i&8)>>3)); dummyString.append(0x30+((i&4)>>2));
  dummyString.append(0x30+((i&2)>>1)); dummyString.append(0x30+((i&1)>>0)); 
  if (isValid(dummyString)) curPattern->append(dummyString);
 }
 patt4=*curPattern;
 extend(); extend(); extend20(); // Now widths are 20lets (50ms per stim.length)
 				 // and there are 3280 probabilities
 std::srand(time(NULL));

 int currentPatternNo; QString finalPattern; // Add random 20lets..
 while (finalPattern.size() < patternCount) {
  currentPatternNo=rand() % curPattern->size();
  dummyString=(*curPattern)[currentPatternNo];
  if (isValid(dummyString)) {
   if (finalPattern.size()==0) dummyString.replace(0,1,"0"); // 0 at first
   finalPattern.append(dummyString);
  } else qDebug("(SERIOUS:) ERROR!");
 }

 for (int i=0;i<finalPattern.size();i++) { // Scatter appropriate stimuli..
  dummyString=finalPattern.at(i);
  if (dummyString=="x") {
   finalPattern.replace(i,1,0x30+currentEvent);
   currentEvent++; if (currentEvent==6) currentEvent=2;
  }
 }

 qDebug("Outputting to file..");
 for (int i=0;i<patternCount;i++) patternStream << finalPattern[i];

 qDebug("Exiting..");
 patternStream.setDevice(0);
 patternFile.close();
 return 0;
}
