/*      Octopus - Bioelectromagnetic Source Localization System 0.9.5       *\
 *                     (>) GPL 2007-2009 Barkin Ilhan                       *
 *      Hacettepe University, Medical Faculty, Biophysics Department        *
\*                barkin@turk.net, barkin@hacettepe.edu.tr                  */

#ifndef OCTOPUS_SOURCE_H
#define OCTOPUS_SOURCE_H

#include <QObject>
#include <QString>

#include "../../common/vec3.h"

class Source : QObject {
 public:
  Source() : QObject() {
   pos.zero();
  }
  ~Source() {}

  Vec3 pos; float theta,phi;
};

#endif
