# Octopus-ReEL - Realtime Encephalography Laboratory Network
#       Copyright (C) 2007-2025 Barkin Ilhan
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
# Contact info:
# E-Mail:  barkin@unrlabs.org
# Website: http://icon.unrlabs.org/staff/barkin/
# Repo:    https://github.com/4e0n/

TEMPLATE = app
TARGET = octopus-segmenter
LIBS += -lGLU
INCLUDEPATH += .
QT += widgets opengl

#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# Input
HEADERS += coord3d.h \
           kmeanstb.h \
           octopus_head_glwidget.h \
           octopus_mri_slice.h \
           octopus_mri_volume.h \
           octopus_seg_master.h \
           octopus_segmenter.h \
           segwidget.h \
           sliceframe.h \
           thresholdtb.h \
           ../../common/vec3.h
SOURCES += main.cpp
