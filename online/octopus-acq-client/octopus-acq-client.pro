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
TARGET = octopus-acq-client
INCLUDEPATH += .
LIBS += -lGLU
QT += widgets network multimedia opengl

#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# Input
HEADERS += acqmaster.h \
	   confparam.h \
	   configparser.h \
	   headmodel.h \
           digitizer.h \
           chninfo.h \
           coord3d.h \
#           acqcontrol.h \
#           acqclient.h \
#           channel.h \
#           channel_params.h \
#           cntframe.h \
#           headglwidget.h \
#           legendframe.h \
	   ../ampinfo.h \
           ../serial_device.h
SOURCES += main.cpp
