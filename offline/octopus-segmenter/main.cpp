/*      Octopus - Bioelectromagnetic Source Localization System 0.9.5       *\
 *                     (>) GPL 2007-2009 Barkin Ilhan                       *
 *      Hacettepe University, Medical Faculty, Biophysics Department        *
\*                barkin@turk.net, barkin@hacettepe.edu.tr                  */

// Main MRI Volume processor application for segmentation,
// tissue classification, and 3D-boundarization..

#include "octopus_seg_master.h"
#include "octopus_segmenter.h"

int main(int argc,char *argv[]) {
 QApplication app(argc,argv);
 SegMaster *p=new SegMaster(&app);
 Segmenter segmenter(p); segmenter.show(); return app.exec();
}
