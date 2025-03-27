/*
Octopus-ReEL - Realtime Encephalography Laboratory Network
   Copyright (C) 2007-2025 Barkin Ilhan

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

#include "stimclient.h"

int main(int argc,char** argv) {
 QString confHost; int confCommP,confDataP;

 confHost="127.0.0.1"; confCommP=65000; confDataP=65001;

 // Parse system config file for variables
 QStringList cfgValidLines,opts,opts2,netSection;
 QFile cfgFile; QTextStream cfgStream;
 QString cfgLine; QStringList cfgLines; cfgFile.setFileName("/etc/octopus_stim_client.conf");
 if (!cfgFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
  qDebug() << "octopus_stim_client: <.conf> cannot load /etc/octopus_stim_client.conf.";
  exit(-1);
 } else { cfgStream.setDevice(&cfgFile);
  while (!cfgStream.atEnd()) { cfgLine=cfgStream.readLine(160); // Max Line Size
   cfgLines.append(cfgLine); } cfgFile.close();

  // Parse config
  for (int i=0;i<cfgLines.size();i++) { // Isolate valid lines
   if (!(cfgLines[i].at(0)=='#') &&
       cfgLines[i].contains('|')) cfgValidLines.append(cfgLines[i]); }

  for (int i=0;i<cfgValidLines.size();i++) {
   opts=cfgValidLines[i].split("|");
        if (opts[0].trimmed()=="NET") netSection.append(opts[1]);
   else { qDebug() << "octopus_stim_client: <.conf> Unknown section in .conf file!";
    exit(-1);
   }
  }

  // NET
  if (netSection.size()>0) {
   for (int i=0;i<netSection.size();i++) { opts=netSection[i].split("=");

    if (opts[0].trimmed()=="STIM") { opts2=opts[1].split(",");
     if (opts2.size()==3) { confHost=opts2[0].trimmed();
      QHostInfo qhiAcq=QHostInfo::fromName(confHost);
      confHost=qhiAcq.addresses().first().toString();
      qDebug() << "octopus_stim_client: <.conf> Connected host IP is" << confHost;
      confCommP=opts2[1].toInt(); confDataP=opts2[2].toInt();
      // Simple port validation..
      if ((!(confCommP >= 1024 && confCommP <= 65535)) ||
          (!(confDataP >= 1024 && confDataP <= 65535))) {
       qDebug() << "octopus_stim_client: <.conf> Error in Hostname/IP and/or port settings!";
       exit(-1);
      } else {
       qDebug() << "octopus_stim_client: <.conf> CommPort ->" << confCommP << "DataPort ->" << confDataP;
      }
     }
    } else {
     qDebug() << "octopus_stim_client: <.conf> Parse error in Hostname/IP(v4) Address!";
     exit(-1);
    }
   }
  } else {
   exit(-1);
  }
 }

 QApplication app(argc,argv);
 StimClient stimClient(&app,confHost,confCommP,confDataP);
 stimClient.show();
 return app.exec();
 return 0;
}
