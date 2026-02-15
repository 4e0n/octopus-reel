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

#pragma once

static void qtMessageHandler(QtMsgType type,const QMessageLogContext&,const QString& msg) {
 const char* level="INFO";
 switch (type) {
  case QtDebugMsg:    level="DEBUG"; break;
  case QtInfoMsg:     level="INFO";  break;
  case QtWarningMsg:  level="WARN";  break;
  case QtCriticalMsg: level="ERROR"; break;
  case QtFatalMsg:    level="FATAL"; break;
 }
 const QByteArray timestamp=QDateTime::currentDateTime().toString(Qt::ISODateWithMs).toUtf8();
 fprintf(stderr,"%s [%s] %s\n",timestamp.constData(),level,msg.toUtf8().constData());
 if (type==QtFatalMsg) { abort(); }
}
