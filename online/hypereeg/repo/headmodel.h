#ifndef HEADMODEL_H
#define HEADMODEL_H

#include "../../common/gizmo.h"
#include "coord3d.h"

class HeadModel {
 public:
  HeadModel(int hCount) {
   headCount=hCount;
   gizmoExists=false; // Gizmo is common for all subjects
   digExists.resize(headCount); // These exist for each subject
   scalpExists.resize(headCount); skullExists.resize(headCount); brainExists.resize(headCount);
   for (unsigned int i=0;i<headCount;i++) scalpExists[i]=skullExists[i]=brainExists[i]=digExists[i]=false;
  }

  int getHeadCount() { return headCount; }

  void loadGizmo_OgzFile(QString fn) {
   QFile ogzFile; QTextStream ogzStream; QString ogzLine; QStringList ogzLines,ogzValidLines,opts,opts2;
   bool gError=false; Gizmo *dummyGizmo; QVector<int> t(3); QVector<int> ll(2);

   // TODO: here a dry-run would fit.
 
   // Delete previous if any
   if (gizmoExists) {
    for (int i=0;i<gizmo.size();i++) delete gizmo[i];
    gizmo.resize(0);
   }
   gizmoExists=false;

   ogzFile.setFileName(fn);
   ogzFile.open(QIODevice::ReadOnly|QIODevice::Text);
   ogzStream.setDevice(&ogzFile);
   while (!ogzStream.atEnd()) {
    ogzLine=ogzStream.readLine(250);
    ogzLines.append(ogzLine);
   }
   ogzFile.close();

   // Get valid lines
   for (int i=0;i<ogzLines.size();i++)
    if (!(ogzLines[i].at(0)=='#') && ogzLines[i].contains('|'))
     ogzValidLines.append(ogzLines[i]);

   // Find the essential line defining gizmo names
   for (int i=0;i<ogzValidLines.size();i++) {
    opts2=ogzValidLines[i].split(" "); opts=opts2[0].split("|"); opts2.removeFirst(); opts2=opts2[0].split(",");
    if (opts.size()==2 && opts2.size()>0) {
     // GIZMO|LIST must be prior to others or segfault will occur..
     if (opts[0].trimmed()=="GIZMO") {
      if (opts[1].trimmed()=="NAME") {
       for (int i=0;i<opts2.size();i++) {
        dummyGizmo=new Gizmo(opts2[i].trimmed()); gizmo.append(dummyGizmo);
       }
      }
     } else if (opts[0].trimmed()=="SHAPE") {
      ;
     } else if (opts[1].trimmed()=="SEQ") {
      int k=gizFindIndex(opts[0].trimmed()); for (int j=0;j<opts2.size();j++) gizmo[k]->seq.append(opts2[j].toInt()-1);
     } else if (opts[1].trimmed()=="TRI") { int k=gizFindIndex(opts[0].trimmed());
      if (opts2.size()%3==0) {
       for (int j=0;j<opts2.size()/3;j++) {
        t[0]=opts2[j*3+0].toInt()-1; t[1]=opts2[j*3+1].toInt()-1; t[2]=opts2[j*3+2].toInt()-1; gizmo[k]->tri.append(t);
       }
      } else { gError=true;
       qDebug() << "octopus_acq_client: <HeadModel> <LoadGizmo> <OGZ> ERROR: Triangles not multiple of 3 vertices..";
      }
     } else if (opts[1].trimmed()=="LIN") { int k=gizFindIndex(opts[0].trimmed());
      if (opts2.size()%2==0) {
       for (int j=0;j<opts2.size()/2;j++) { ll[0]=opts2[j*2+0].toInt()-1; ll[1]=opts2[j*2+1].toInt()-1; gizmo[k]->lin.append(ll); }
      } else { gError=true;
       qDebug() << "octopus_acq_client: <HeadModel> <LoadGizmo> <OGZ> ERROR: Lines not multiple of 2 vertices..";
      }
     }
    } else { gError=true; qDebug() << "octopus_acq_client: <AcqMaster> <LoadGizmo> <OGZ> ERROR: while parsing!"; }
   } if (!gError) gizmoExists=true;
  }

  void loadScalp_ObjFile(QString fn,int headNo) {
   QFile scalpFile; QTextStream scalpStream; QString dummyStr; QStringList dummySL,dummySL2; Coord3D c; QVector<int> idx;
   scalpExists[headNo]=false; // TODO: This could leave without a head loaded. Soon should be replaced with a dry-run.

   // Reset previous
   for (int i=0;i<scalpIndex.size();i++) scalpIndex[i].resize(0);
   scalpIndex.resize(0); scalpCoord.resize(0);
 
   scalpFile.setFileName(fn); scalpFile.open(QIODevice::ReadOnly|QIODevice::Text); scalpStream.setDevice(&scalpFile);
   while (!scalpStream.atEnd()) {
    dummyStr=scalpStream.readLine(); dummySL=dummyStr.split(" ");
    if (dummySL[0]=="v") { c.x=dummySL[1].toFloat(); c.y=dummySL[2].toFloat(); c.z=dummySL[3].toFloat(); scalpCoord.append(c); }
    else if (dummySL[0]=="f") { idx.resize(0);
     dummySL2=dummySL[1].split("/"); idx.append(dummySL2[0].toInt());
     dummySL2=dummySL[2].split("/"); idx.append(dummySL2[0].toInt());
     dummySL2=dummySL[3].split("/"); idx.append(dummySL2[0].toInt());
     scalpIndex.append(idx);
    }
   }
   scalpStream.setDevice(0); scalpFile.close();
   scalpExists[headNo]=true;
  }

  void loadSkull_ObjFile(QString fn,int subjectNo) { skullExists[subjectNo]=true; }

  void loadBrain_ObjFile(QString fn,int subjectNo) { brainExists[subjectNo]=true; }

  int gizFindIndex(QString s) { int idx=-1;
   for (int i=0;i<gizmo.size();i++) if (gizmo[i]->name==s) { idx=i; break; }
   return idx;
  }

 private:
  int headCount; bool gizmoExists; QVector<bool> scalpExists,skullExists,brainExists,digExists;
  QVector<Coord3D> scalpCoord,skullCoord,brainCoord;
  QVector<QVector<int> > scalpIndex,skullIndex,brainIndex;
  QVector<Gizmo* > gizmo;

  //QVector<Coord3D> paramCoord,realCoord; QVector<QVector<int> > paramIndex;
  //QVector<float> scalpParamR,scalpNasion,scalpCzAngle;
  //QVector<bool> hwFrameV,hwGridV,hwDigV,hwParamV,hwRealV,hwGizmoV,hwAvgsV,hwScalpV,hwSkullV,hwBrainV;
  // Gizmo
  //QVector<bool> gizmoOnReal,elecOnReal; QVector<int> currentGizmo,currentElectrode,curElecInSeq;
};
#endif








//  void loadSkull_ObjFile(QString fn) { ;
//   for (unsigned int i=0;i<headCount;i++) skullExists[i]=false;
//   QFile skullFile; QTextStream skullStream; QString dummyStr; QStringList dummySL,dummySL2; Coord3D c; QVector<int> idx;

   // Reset previous
//   for (int i=0;i<skullIndex.size();i++) skullIndex[i].resize(0);
//   skullIndex.resize(0); skullCoord.resize(0);
 
//   skullFile.setFileName(fn); skullFile.open(QIODevice::ReadOnly|QIODevice::Text); skullStream.setDevice(&skullFile);
//   while (!skullStream.atEnd()) {
//    dummyStr=skullStream.readLine(); dummySL=dummyStr.split(" ");
//    if (dummySL[0]=="v") { c.x=dummySL[1].toFloat(); c.y=dummySL[2].toFloat(); c.z=dummySL[3].toFloat(); skullCoord.append(c); }
//    else if (dummySL[0]=="f") { idx.resize(0);
//     dummySL2=dummySL[1].split("/"); idx.append(dummySL2[0].toInt());
//     dummySL2=dummySL[2].split("/"); idx.append(dummySL2[0].toInt());
//     dummySL2=dummySL[3].split("/"); idx.append(dummySL2[0].toInt());
//     skullIndex.append(idx);
//    }
//   } skullStream.setDevice(0); skullFile.close(); for (unsigned int i=0;i<headCount;i++) skullExists[i]=true;
//  }

//  void loadBrain_ObjFile(QString fn) { ;
//   for (unsigned int i=0;i<headCount;i++) brainExists[i]=false;
//   QFile brainFile; QTextStream brainStream; QString dummyStr; QStringList dummySL,dummySL2; Coord3D c; QVector<int> idx;

   // Reset previous
//   for (int i=0;i<brainIndex.size();i++) brainIndex[i].resize(0);
//   brainIndex.resize(0); brainCoord.resize(0);
 
//   brainFile.setFileName(fn); brainFile.open(QIODevice::ReadOnly|QIODevice::Text); brainStream.setDevice(&brainFile);
//   while (!brainStream.atEnd()) {
//    dummyStr=brainStream.readLine(); dummySL=dummyStr.split(" ");
//    if (dummySL[0]=="v") { c.x=dummySL[1].toFloat(); c.y=dummySL[2].toFloat(); c.z=dummySL[3].toFloat(); brainCoord.append(c); }
//    else if (dummySL[0]=="f") { idx.resize(0);
//     dummySL2=dummySL[1].split("/"); idx.append(dummySL2[0].toInt());
//     dummySL2=dummySL[2].split("/"); idx.append(dummySL2[0].toInt());
//     dummySL2=dummySL[3].split("/"); idx.append(dummySL2[0].toInt());
//     brainIndex.append(idx);
//    }
//   } brainStream.setDevice(0); brainFile.close(); for (unsigned int i=0;i<headCount;i++) brainExists[i]=true;
//  }

//  void loadReal(QString fileName) { ;
//   QString realLine; QStringList realLines,realValidLines,opts; QFile realFile(fileName); int p,c;
//   realFile.open(QIODevice::ReadOnly); QTextStream realStream(&realFile);
   // Read all
//   while (!realStream.atEnd()) { realLine=realStream.readLine(160); realLines.append(realLine); } realFile.close();

   // Get valid lines
//   for (int i=0;i<realLines.size();i++)
//    if (!(realLines[i].at(0)=='#') && realLines[i].split(" ",Qt::SkipEmptyParts).size()>4) realValidLines.append(realLines[i]);

   // Find the essential line defining gizmo names
//   for (int ll=0;ll<realValidLines.size();ll++) {
//    opts=realValidLines[ll].split(" ",Qt::SkipEmptyParts);
//    if (opts.size()==8 && opts[0]=="v") {
//     opts.removeFirst(); p=opts[0].toInt(); c=-1;
//     for (int i=0;i<acqChannels.size();i++) for (int j=0;j<acqChannels[i].size();j++) {
//      if (p==acqChannels[i][j]->physChn) { c=i; break; }
////    if (c!=-1) printf("%d - %d\n",p,c); else qDebug() << "octopus_acq_client: <AcqMaster> <LoadReal> Channel does not exist!";
//      acqChannels[i][c]->real[0]=opts[1].toFloat();
//      acqChannels[i][c]->real[1]=opts[2].toFloat();
//      acqChannels[i][c]->real[2]=opts[3].toFloat();
//      acqChannels[i][c]->realS[0]=opts[4].toFloat();
//      acqChannels[i][c]->realS[1]=opts[5].toFloat();
//      acqChannels[i][c]->realS[2]=opts[6].toFloat();
//     }
//    } else { qDebug() << "octopus_acq_client: <AcqMaster> <LoadReal> Erroneous real coord file.." << opts.size(); break; }
//   } emit repaintGL(4); // Repaint Real coords
//  }

//  void saveReal(QString fileName) { ;
//   QFile realFile(fileName); realFile.open(QIODevice::WriteOnly); QTextStream realStream(&realFile);
//   realStream << "# Octopus real head coordset in headframe xyz's..\n";
//   realStream << "# Generated by Octopus-recorder 0.9.5\n\n";
//   realStream << "# Coord count = " << acqChannels.size() << "\n";
//   for (int i=0;i<acqChannels.size();i++) for (int j=0;j<acqChannels[i].size();j++) {
//    realStream << "v " << acqChannels[i][j]->physChn+1 << " "
//                       << acqChannels[i][j]->real[0] << " "
//                       << acqChannels[i][j]->real[1] << " "
//                       << acqChannels[i][j]->real[2] << " "
//                       << acqChannels[i][j]->realS[0] << " "
//                       << acqChannels[i][j]->realS[1] << " "
//                       << acqChannels[i][j]->realS[2] << "\n";
//   } realFile.close();
//  }

