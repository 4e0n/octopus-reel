/*      Octopus - Bioelectromagnetic Source Localization System 0.9.5       *\
 *                     (>) GPL 2007-2009 Barkin Ilhan                       *
 *      Hacettepe University, Medical Faculty, Biophysics Department        *
\*                barkin@turk.net, barkin@hacettepe.edu.tr                  */

#ifndef OCTOPUS_GIZMO_H
#define OCTOPUS_GIZMO_H

#include <QString>
#include <QVector>

class Gizmo {
 public:
  Gizmo(QString nm) { name=nm; }

  QString name; QVector<int> seq; QVector<QVector<int> > tri,lin;
};

#endif
