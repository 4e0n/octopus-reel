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
TARGET = hypereeg-acq-client
INCLUDEPATH += .
LIBS += -lGLU
QT += core gui widgets network multimedia opengl
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# Input
HEADERS += acqmaster.h \
           acqcontrolwindow.h \
           acqstreamwindow.h \
           headglwidget.h \
           audioframe.h \
           eegframe.h \
           eegthread.h \
           configparser.h \
           confparam.h \
           chninfo.h \
#           headmodel.h \
#           legendframe.h \
#           headglwidget.h \
           ../../../common/event.h \
           ../../../common/vec3.h \
           ../../../common/coord3d.h \
           ../tcpcommand.h
SOURCES += main.cpp
CONFIG += c++11
