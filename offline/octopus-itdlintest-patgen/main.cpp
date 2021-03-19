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

/* This application generates the pseudo-randomized pattern for the
   stimulus presentation order in the ITD Linearity test protocol
   proposed by Prof. Pekcan Ungan

   Creation algorithm, designed as follows:
	C00 -> L03(A), R03(E), R06(L), L06(I)	(P=3/12)
	L03 -> L06(B), C00(D)			(P=6/12)
	R03 -> R06(F), C00(H)			(P=6/12)
	L06 -> L03(C), C00(N), R06(J)		(P=4/12)
	R06 -> R03(G), C00(K), L06(M)		(P=4/12)

   After normalization for probabilities:
   	C00 -> AAEELLLIII
	L03 -> BBBBDDD
	R03 -> FFFFHHH
	L06 -> CCNNNJJJJ
	R06 -> GGKKKMMMM
*/   

#include <iostream>
#include <vector>
#include <QString>
#include <QStringList>
#include <QIntValidator>
#include <QFile>
#include <QTextStream>

QString createPattern(int count,int g) {
 QString base="ABCDEFGHIJKLMN";
 std::vector<int> counts;
 counts.resize(base.length());
 QString dummyString;
 std::vector<QString> state;
 state.push_back(QString("AELI"));
 state.push_back(QString("BD"));
 state.push_back(QString("FH"));
 state.push_back(QString("CNJ"));
 state.push_back(QString("GKM"));
 QString result;

 int boolSum,maxcount; QChar letter;
 int grace=int(float(count)*(100.+float(g))/100.);
 std::cout << count << " " << grace << std::endl;
  
 int iter=0;
 do {
  for (int i=0;i<base.length();i++) counts[i]=0;
  result="N"; // Start at center
  while (boolSum<base.length()) {
   letter=result[result.length()-1];
   switch (letter.toAscii()) {	  
    case 'N': case 'D': case 'K': case 'H':
     letter=state[0][rand()%state[0].length()];
     break;
    case 'A': case 'C':
     letter=state[1][rand()%state[1].length()];
     break;
    case 'E': case 'G':
     letter=state[2][rand()%state[2].length()];
     break;
    case 'B': case 'M': case 'I':
     letter=state[3][rand()%state[3].length()];
     break;
    case 'F': case 'J': case 'L':
     letter=state[4][rand()%state[4].length()];
    default: break;
   } 
   result.append(letter);
   counts[int(letter.toAscii()-'A')]++;
   boolSum=0;
   for (int i=0;i<base.length();i++)
    boolSum+=int(counts[i]>=count);
  }
  std::cout << boolSum << std::endl;
  maxcount=boolSum=0;
  for (int i=0;i<base.length();i++) {
   if (counts[i]>maxcount) maxcount=counts[i];
   std::cout << counts[i] << " ";
  }
  std::cout << maxcount << "\n";
  iter++;
 } while (maxcount>grace);

 std::cout << "Iteration count: " << iter << "\n";

 return result;
}

template <typename T>
std::vector<T>& operator +=(std::vector<T>& vector1, const std::vector<T>& vector2) {
    vector1.insert(vector1.end(), vector2.begin(), vector2.end());
    return vector1;
}

QString createPattern2(int count,int g) {
 QString result,dummyString;
 QString base="ABCDEFGHIJKLMN";
 std::vector<int> counts; counts.resize(base.length());
 std::vector<int> draw0,draw1;
 int pivot,draw_idx,boolSum=0,maxCount=0;
 QChar letter0,letter1;

 std::vector<std::vector<int> > state; state.resize(5);
 state[0].push_back(1); state[0].push_back(2); state[0].push_back(4);
 state[1].push_back(0); state[1].push_back(2);
 state[2].push_back(0); state[2].push_back(1);
 state[2].push_back(3); state[2].push_back(4);
 state[3].push_back(2); state[3].push_back(4);
 state[4].push_back(0); state[4].push_back(2); state[4].push_back(3);

 std::vector<QString> letter; letter.resize(5);
 letter[0]="XCNXJ";
 letter[1]="BXDXX";
 letter[2]="IAXEL";
 letter[3]="XXHXF";
 letter[4]="MXKGX";

 int grace=int(float(count)*(100.+float(g))/100.);
 std::cout << count << " " << grace << std::endl;
 int iter=0;

 do {
  for (int i=0;i<base.length();i++) counts[i]=0;
  result=""; pivot=1; // Initial position is center
  while (boolSum<base.length()) {
   draw0.resize(0); draw1.resize(0);
   for (int i=0;i<state[pivot].size();i++) {
    for (int j=0;j<state[ state[pivot][i] ].size();j++) {
      draw0.push_back(state[pivot][i]);
      draw1.push_back(state[ state[pivot][i] ][j]);
    }
   }

//   for (int i=0;i<draw0.size();i++)
//    std::cout << draw0[i] << " " << draw1[i] << "\n";

   draw_idx=rand()%draw0.size();
   letter0=letter[pivot][ draw0[draw_idx] ];
   letter1=letter[ draw0[draw_idx] ][ draw1[draw_idx] ];
   result.append(letter0); //result.append(letter1);
//   std::cout << letter0.toAscii() << "\n";
   counts[int(letter0.toAscii()-'A')]++;
   counts[int(letter1.toAscii()-'A')]++;
   pivot=draw1[draw_idx];
   pivot=draw0[draw_idx];
   boolSum=0;
   for (int i=0;i<base.length();i++)
    boolSum+=int(counts[i]>=count);
  }

  maxCount=boolSum=0;
  for (int i=0;i<base.length();i++) {
   if (counts[i]>maxCount) maxCount=counts[i];
   std::cout << counts[i] << " ";
  }
  std::cout << "MaxCount: " << maxCount << "\n";
  iter++;
 } while (maxCount>grace);

 std::cout << "Iteration count: " << iter << "\n";

//  std::cout << result[i].toAscii();
// std::cout << "\n";

// for (int i=0;i<draw0.size();i++)
//  std::cout << draw0[i] << " " << draw1[i] << "\n";

// std::cout << "-------\n";
// std::cout << pivot << " " << draw0[draw_idx] << " " << draw1[draw_idx] << "\n";
// std::cout << "-------\n";

 return result;
}

QString createPattern3(int count,int g) {
 QString result;
 QString base="ABCDEFGHIJKLMN";

 std::vector<int> counts; counts.resize(base.length());
 std::vector<int> draw0,draw1;
 int pivot,draw_idx,boolSum=0,maxCount=0;
 QChar letter0;

 std::vector<QString> letter; letter.resize(5);
 letter[0]="XCNXJ";
 letter[1]="BXDXX";
 letter[2]="IAXEL";
 letter[3]="XXHXF";
 letter[4]="MXKGX";

 std::vector<int> minVec; char c;

 int grace=int(float(count)*(100.+float(g))/100.);
 std::cout << count << " " << grace << std::endl;
 int iter=0;
 do {
  for (int i=0;i<base.length();i++) counts[i]=0;
  result=""; pivot=2; // Initial position is center
  while (boolSum<base.length()) {
   do {
    // Optimization
    //minVec.resize(0);
    //for (int i=0;i<letter[pivot].size();i++) {
    // c=letter[pivot][i].toAscii();
    // if (c!='X') minVec.push_back(counts[int(c-'A')]);
    //}
    //bool diff=false; int minV=count*10; int minVidx=0;
    //for (int i=0;i<minVec.size();i++) {
    // if (i>0 && minVec[i-1]!=minVec[i]) diff=true;
    // if (minV<minVec[i]) { minV=minVec[i]; minVidx=i; }
    //}
    //if (diff) {
    // for (int i=0;i<letter[pivot].length();i++) {
    //  if (letter[pivot][i].toAscii()==base[minVidx]) draw_idx=i;
    //  std::cout << letter[pivot][i].toAscii() << " ";
    //  std::cout << minVidx;
    //  std::cout << "\n";
    // }
    //} //else
     draw_idx=rand()%letter.size();
    letter0=letter[pivot][draw_idx];
   } while (letter0.toAscii()=='X');
   result.append(letter0);
   counts[int(letter0.toAscii()-'A')]++;
   pivot=draw_idx;
   boolSum=0;
   for (int i=0;i<base.length();i++)
    boolSum+=int(counts[i]>=count);
  }
  maxCount=boolSum=0;
  for (int i=0;i<base.length();i++) {
   if (counts[i]>maxCount) maxCount=counts[i];
   std::cout << counts[i] << " ";
  }
  std::cout << "MaxCount: " << maxCount << "\n";
  iter++;
 } while (maxCount>grace);

 return result;
}

int main(int argc,char *argv[]) {
 QString p; int pos;
 if (argc!=5) {
  qDebug("Usage: octopus-patt <file name> <alg#> <count from each state> <grace%>");
  return -1;
 }

 p=QString::fromAscii(argv[2]);
 QIntValidator intValidator0(1,3,NULL); pos=0;
 if (intValidator0.validate(p,pos) != QValidator::Acceptable) {
  qDebug("Please enter a positive integer of either 1 or 2 as algorithm!");
  qDebug("Usage: octopus-patt <file name> <alg#> <count from each state> <grace%>");
  return -1;
 }
 int algo=p.toInt();

 p=QString::fromAscii(argv[3]);
 QIntValidator intValidator1(100,1000,NULL); pos=0;
 if (intValidator1.validate(p,pos) != QValidator::Acceptable) {
  qDebug("Please enter a positive integer between 10 and 1000 as stim.length!");
  qDebug("Usage: octopus-patt <file name> <alg#> <count from each state> <grace%>");
  return -1;
 }
 int patternCount=p.toInt();

 p=QString::fromAscii(argv[4]);
 QIntValidator intValidator2(1,20,NULL); pos=0;
 if (intValidator2.validate(p,pos) != QValidator::Acceptable) {
  qDebug("Please enter a positive integer between 1 and 20 as grace % !");
  qDebug("Usage: octopus-patt <file name> <count from each state> <grace%>");
  return -1;
 }
 int grace=p.toInt();

 QFile patternFile; QTextStream patternStream;
 patternFile.setFileName(argv[1]);
 if (!patternFile.open(QIODevice::WriteOnly)) {
  qDebug("Pattern error: Cannot generate/open pattern file for writing.");
  return -1;
 }

 patternStream.setDevice(&patternFile);
 std::srand(time(NULL));

 QString finalPattern;
 if (algo==1) finalPattern=createPattern(patternCount,grace);
 else if (algo==2) finalPattern=createPattern2(patternCount,grace);
 else if (algo==3) finalPattern=createPattern3(patternCount,grace);

 qDebug("Outputting to file..");
 for (int i=0;i<finalPattern.length();i++) patternStream << finalPattern[i];

 qDebug("Exiting..");
 patternStream.setDevice(0);
 patternFile.close();
 return 0;
}
