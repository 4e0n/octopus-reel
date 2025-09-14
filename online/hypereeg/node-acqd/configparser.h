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

#pragma once

#include <QHostInfo>
#include <QFile>
#include "confparam.h"
#include "chninfo.h"

const int MAX_LINE_SIZE=160; // chars

class ConfigParser {
 public:
  ConfigParser(QString cp) { cfgPath=cp; cfgFile.setFileName(cfgPath); }

  bool parse(ConfParam *conf,QVector<ChnInfo> *chnTopo) {
   QTextStream cfgStream;
   QStringList cfgLines,opts,opts2,netSection,chnSection;

   QStringList ampSection; ChnInfo dummyChnInfo;

   if (!cfgFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qDebug() << "octopus_hacqd: <ConfigParser> ERROR: Cannot load" << cfgPath;
    return true;
   } else {
    cfgStream.setDevice(&cfgFile);

    while (!cfgStream.atEnd()) { // Populate cfgLines with valid lines
     cfgLine=cfgStream.readLine(MAX_LINE_SIZE); // Should not start with #, should contain "|"
     if (!(cfgLine.at(0)=='#') && cfgLine.contains('|')) cfgLines.append(cfgLine);
    }
    cfgFile.close();

    // Separate AMP, NET, CHN, GUI parameter lines
    for (const auto& cl:cfgLines) {
     opts=cl.split("#"); opts=opts[0].split("|"); // Get rid off any following comment.
          if (opts[0].trimmed()=="AMP") ampSection.append(opts[1]);
     else if (opts[0].trimmed()=="NET") netSection.append(opts[1]);
     else if (opts[0].trimmed()=="CHN") chnSection.append(opts[1]);
     else {
      qDebug() << "octopus_hacqd: <ConfigParser> ERROR: Unknown section in config file!";
      return true;
     }
    }

    // --- Extract variables from separate sections ---

    // AMP section
    if (ampSection.size()>0) {
     for (const auto& sect:ampSection) {
      opts=sect.split("=");
      if (opts[0].trimmed()=="COUNT") {
       conf->ampCount=opts[1].toInt();
       if (conf->ampCount > EE_MAX_AMPCOUNT) {
        qDebug() << "octopus_hacqd: <ConfigParser> <AMP> ERROR: Currently more than" \
		 << EE_MAX_AMPCOUNT \
                 << "simultaneously connected amplifiers is not supported!";
        return true;
       }
      } else if (opts[0].trimmed()=="BUFPAST") {
       conf->tcpBufSize=opts[1].toInt();
       if (!(conf->tcpBufSize >= 2 && conf->tcpBufSize <= 50)) {
        qDebug() << "octopus_hacqd: <ConfigParser> <AMP> ERROR: BUFPAST not within [2,50] seconds range!";
        return true;
       }
      } else if (opts[0].trimmed()=="EEGRATE") {
       conf->eegRate=opts[1].toInt();
       if (!(conf->eegRate == 500 || conf->eegRate == 1000)) {
        qDebug() << "octopus_hacqd: <ConfigParser> <AMP> ERROR: EEG Samplerate not among {500,1000}!";
        return true;
       }
      } else if (opts[0].trimmed()=="CMRATE") {
       conf->cmRate=opts[1].toInt();
       if (!(conf->cmRate == 1 || conf->cmRate == 2)) {
        qDebug() << "octopus_hacqd: <ConfigParser> <AMP> ERROR: CommonMode Samplerate not among {1,10}!";
        return true;
       }
      } else if (opts[0].trimmed()=="EEGPROBEMS") {
       conf->eegProbeMsecs=opts[1].toInt();
       if (!(conf->eegProbeMsecs >= 20 && conf->eegProbeMsecs <= 1000)) {
        qDebug() << "octopus_hacqd: <ConfigParser> <AMP> ERROR: EEGPROBEMS not within [20,1000] msecs range!";
        return true;
       }
      } else if (opts[0].trimmed()=="REFGAIN") {
       conf->refGain=opts[1].toFloat();
       if (!(conf->refGain==1.0 ||  conf->refGain==2.0)) {
        qDebug() << "octopus_hacqd: <ConfigParser> <AMP> ERROR: REFGAIN not among {1.0,2.0}x range!";
        return true;
       }
      } else if (opts[0].trimmed()=="BIPGAIN") {
       conf->bipGain=opts[1].toFloat();
       if (!(conf->bipGain==4.0 ||  conf->bipGain==8.0)) {
        qDebug() << "octopus_hacqd: <ConfigParser> <AMP> ERROR: BIPGAIN not among {4.0,8.0}x range!";
        return true;
       }
      } else {
       qDebug() << "octopus_hacqd: <ConfigParser> <AMP> ERROR: Unknown subsection in AMP section!";
       return true;
      }
     }
    } else {
     qDebug() << "octopus_hacqd: <ConfigParser> <AMP> ERROR: No parameters in section!";
     return true;
    }

    // NET section
    if (netSection.size()>0) {
     for (const auto& sect:netSection) {
      opts=sect.split("=");
      if (opts[0].trimmed()=="OUT") {
       opts2=opts[1].split(","); // IP, command port, stream port and commonmode port are separated by ","
       if (opts2.size()==4) {
        QHostInfo acqHostInfo=QHostInfo::fromName(opts2[0].trimmed());
        conf->ipAddr=acqHostInfo.addresses().first().toString();
        conf->commPort=opts2[1].toInt();
	conf->strmPort=opts2[2].toInt();
	conf->cmodPort=opts2[3].toInt();
        if ((!(conf->commPort >= 65000 && conf->commPort < 65500)) || // Simple port validation
            (!(conf->strmPort >= 65000 && conf->strmPort < 65500)) ||
            (!(conf->cmodPort >= 65000 && conf->cmodPort < 65500))) {
         qDebug() << "octopus_hacqd: <ConfigParser> <NET> ERROR: Invalid hostname/IP/port settings!";
         return true;
        }
       } else {
        qDebug() << "octopus_hacqd: <ConfigParser> <NET> ERROR: Invalid count of NET|OUT params!";
        return true;
       }
      } else {
       qDebug() << "octopus_hacqd: <ConfigParser> <NET> ERROR: Invalid hostname/IP(v4) address!";
       return true;
      }
     }
    } else {
     qDebug() << "octopus_hacqd: <ConfigParser> <NET> ERROR: No parameters in section!";
     return true;
    }

    // CHN section
    bool bipChn; conf->refChnCount=conf->bipChnCount=0;
    if (chnSection.size()>0) {
     for (const auto& sect:chnSection) {
      opts=sect.split("=");
      if (opts[0].trimmed()=="APPEND") {
       opts=opts[1].split(">");
       opts2=opts[0].split(",");
       if (opts2.size()==7) {
        opts2[0]=opts2[0].trimmed(); // Ref vs. Bip
        opts2[2]=opts2[2].trimmed(); // Channel name
        if ((opts2[0].size()!=1) || !(opts2[0]=="R" || opts2[0]=="B") || // Ref or Bip
            (!((unsigned)opts2[1].toInt()>0  && (unsigned)opts2[1].toInt()<=512)) || // Channel#
            (!(opts2[2].size()>0 && opts2[2].size()<=3)) || // Channel name must be 1 to 3 chars..
            (!((float)opts2[3].toFloat()>=0. && (float)opts2[3].toFloat()<360.)) || // TopoThetaPhi - Theta
            (!((float)opts2[4].toFloat()>=0. && (float)opts2[4].toFloat()<360.)) || // TopoThetaPhi - Phi
            (!((unsigned)opts2[5].toInt()>=1 && (unsigned)opts2[5].toInt()<=11)) || // TopoXY - X
            (!((unsigned)opts2[6].toInt()>=1 && (unsigned)opts2[6].toInt()<=11))) { // TopoXY - Y
         qDebug() << "octopus_hacqd: <ConfigParser> <CHN> ERROR: Invalid parameter!";
	 return true;
        } else { // Set and append new channel..
         opts2[0]=="B" ? bipChn=true : bipChn=false;
	 dummyChnInfo.physChn=opts2[1].toInt();         // Physical channel
	 dummyChnInfo.chnName=opts2[2];                 // Channel name
	 dummyChnInfo.topoTheta=opts2[3].toFloat();     // TopoThetaPhi - Theta
	 dummyChnInfo.topoPhi=opts2[4].toFloat();       // TopoThetaPhi - Phi
	 dummyChnInfo.topoX=opts2[5].toInt();           // TopoXY - X
	 dummyChnInfo.topoY=opts2[6].toInt();           // TopoXY - Y
         dummyChnInfo.isBipolar=bipChn;                 // Is bipolar?
         if (bipChn) conf->bipChnCount++; else conf->refChnCount++;
        }
       } else {
        qDebug() << "octopus_hacqd: <ConfigParser> <CHN> ERROR: Invalid count of APPEND parameters!";
	return true;
       }
       opts2=opts[1].split(","); // Interpolation electrodes
       if (opts2.size()>=1 && opts2.size()<=4) {
        dummyChnInfo.interElec.resize(0);
        if (opts2[0].toInt()==0) {
         if (!dummyChnInfo.isBipolar) dummyChnInfo.interElec.append(dummyChnInfo.physChn-1);
	 else dummyChnInfo.interElec.append(0);
	} else {
	 for (int idx=0;idx<opts2.size();idx++) dummyChnInfo.interElec.append(opts2[idx].toInt()-1);
	}
        chnTopo->append(dummyChnInfo); // add channel to info table
       } else {
        qDebug() << "octopus_hacqd: <ConfigParser> <CHN> ERROR: Invalid count of APPEND (chn interpolation) parameters!";
	return true;
       }
      }
     }
    } else {
     qDebug() << "octopus_hacqd: <ConfigParser> <CHN> ERROR: No parameters in section!";
     return true;
    }

    //for (const auto& ct:chnTopo) qDebug() << ct.physChn << ct.chnName << ct.topoX << ct.topoY;

   } // File open
   return false;
  }

 private:
  QString cfgPath,cfgLine; QFile cfgFile;
};
