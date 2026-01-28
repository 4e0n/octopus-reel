/*
Octopus-ReEL - Realtime Encephalography Laboratory Network
   Copyright (C) 2007-2026 Barkin Ilhan

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

/* This is the HyperEEG "Frequency Domain GUI" Node.
 * Its main purpose is to handle a graphic view including OpenGL widgets,
 * for the EEG frequency component-powers' streaming over TCP from another producer node.
 * In most native configurations, this node would be the node-acq itself.
 */

#include <QApplication>
#include <QSurfaceFormat>
#include "guiclient.h"

int main(int argc,char* argv[]) {
 QSurfaceFormat fmt;
 fmt.setRenderableType(QSurfaceFormat::OpenGL);
 fmt.setProfile(QSurfaceFormat::NoProfile);  // important for GL 2.1
 fmt.setVersion(2,1);
 fmt.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
 fmt.setSwapInterval(0);
 QSurfaceFormat::setDefaultFormat(fmt);

 QApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
 //QApplication::setAttribute(Qt::AA_UseOpenGLES);
 //QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

 QApplication app(argc,argv);
 GUIClient guiClient;

 omp_diag();

 if (guiClient.initialize()) {
  qCritical("node-freq: <FatalError> Failed to initialize Octopus-ReEL EEG HyperAcquisition Frequency Domain GUI Client.");
  return 1;
 }

 return app.exec();
}
