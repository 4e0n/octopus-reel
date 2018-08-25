/*      Octopus - Bioelectromagnetic Source Localization System 0.9.5       *\
 *                     (>) GPL 2007-2009 Barkin Ilhan                       *
 *      Hacettepe University, Medical Faculty, Biophysics Department        *
\*                barkin@turk.net, barkin@hacettepe.edu.tr                  */

#ifndef OCTOPUS_EVENT_H
#define OCTOPUS_EVENT_H

#include <QObject>
#include <QString>

class Event : QObject {
 public:
  Event(int evtNo,QString evtName,int evtType,QColor c) : QObject() {
   no=evtNo; name=evtName; type=evtType; accepted=rejected=0; color=c;
  }
  ~Event() {}

  int no,type,accepted,rejected; QString name; QColor color;
};

#endif
