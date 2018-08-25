/*      Octopus - Bioelectromagnetic Source Localization System 0.9.5       *\
 *                     (>) GPL 2007-2009 Barkin Ilhan                       *
 *      Hacettepe University, Medical Faculty, Biophysics Department        *
\*                barkin@turk.net, barkin@hacettepe.edu.tr                  */

#include "octopus_loc_master.h"
#include "octopus_localizer.h"

int main(int argc,char** argv) {
 QApplication app(argc,argv); LocMaster *p=new LocMaster(&app);
 if (!p->initSuccess) {
  qDebug("Octopus-Localizer: Initialization failed.. exiting.."); return -1;
 } else { Localizer localizer(p); localizer.show(); return app.exec(); }
}
