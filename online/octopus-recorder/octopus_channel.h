/*      Octopus - Bioelectromagnetic Source Localization System 0.9.5       *\
 *                     (>) GPL 2007-2009 Barkin Ilhan                       *
 *      Hacettepe University, Medical Faculty, Biophysics Department        *
\*                barkin@turk.net, barkin@hacettepe.edu.tr                  */

#ifndef OCTOPUS_CHANNEL_H
#define OCTOPUS_CHANNEL_H

#include <QObject>
#include <QString>
#include "channel_params.h"
#include "coord3d.h"
#include "../../common/vec3.h"

class Channel : QObject {
 public:
  Channel(int pc,QString n,int chRejLev,int chRejRef,QString cv,QString cr,
          QString av,QString ar,float th,float ph) : QObject() {
   physChn=pc-1; name=n; rejLev=(float)chRejLev; rejRef=chRejRef;
   cntVis = (cv=="T" || cv=="t") ? true : false;
   cntRec = (cr=="T" || cr=="t") ? true : false;
   avgVis = (av=="T" || av=="t") ? true : false;
   avgRec = (ar=="T" || ar=="t") ? true : false;
   param.y=th; param.z=ph; real=Vec3(1000.,0.,0.); realS.zero();
  }
  ~Channel() {}

  void setEventProfile(int eventCount,int dataCount) {
   QVector<float> *dummyAS;
   for (int i=0;i<avgData.size();i++) { delete avgData[i]; delete stdData[i]; }
   for (int i=0;i<eventCount;i++) {
    dummyAS=new QVector<float>(dataCount); avgData.append(dummyAS);
    dummyAS=new QVector<float>(dataCount); stdData.append(dummyAS);
   }
  }

  void resetEvents() {
   for (int i=0;i<avgData.size();i++) {
    for (int j=0;j<avgData[i]->size();j++)
     (*(avgData[i]))[j]=(*(stdData[i]))[j]=0.;
   }
  }

  // Continuous and Average visibility and recording flags exist as strings
  // in the constructor, which is how they are read from the config file..
  int physChn,rejRef; QString name; bool cntVis,cntRec,avgVis,avgRec;

  Coord3D param; Vec3 real,realS; // Parametric and realistic coords..

  QVector<float> pastData,pastFilt; float rejLev;
  QVector<QVector<float>* > avgData,stdData;
  float notchLevel; QColor notchColor; // Instantly computed line noise level..
};

#endif
