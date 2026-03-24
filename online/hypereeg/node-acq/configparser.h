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

#include <QHostInfo>
#include <QFile>
#include "confparam.h"

const int MAX_LINE_SIZE=160; // chars
const QString NODE_TEXT="ACQ";

class ConfigParser {
 public:
  ConfigParser(QString cp) { cfgPath=cp; cfgFile.setFileName(cfgPath); }

  bool parse(ConfParam *conf) {
   QVector<AcqChnInfo> *refChns=&(conf->refChns);
   QVector<AcqChnInfo> *bipChns=&(conf->bipChns);
   QVector<AcqChnInfo> *metaChns=&(conf->metaChns);
   QTextStream cfgStream; QStringList cfgLines,opts,opts2,netSection,chnSection;
   QStringList ampSection; AcqChnInfo dummyChnInfo;

   if (!cfgFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qCritical() << "<ConfigParser> ERROR: Cannot load" << cfgPath;
    return true;
   } else {
    cfgStream.setDevice(&cfgFile);
    while (!cfgStream.atEnd()) { // Populate cfgLines with valid lines
     cfgLine=cfgStream.readLine(MAX_LINE_SIZE).trimmed(); // Should not start with #, should contain "|"
     if (!cfgLine.isEmpty() && !cfgLine.startsWith('#') && cfgLine.contains('|')) cfgLines.append(cfgLine);
    }
    cfgFile.close();

    int idx1=-1,idx2=cfgLines.size()-1;
    for (int idx=0;idx<cfgLines.size();idx++) { opts=cfgLines[idx].split("|");
     if (opts[0].trimmed()=="NODE" && opts[1].trimmed()==NODE_TEXT) { idx1=idx; break; }
    }
    if (idx1<0) {
     qCritical() << "<ConfigParser> ERROR: NODE section does not exist in config file!";
     return true;
    }
    idx1++;
    for (int idx=idx1;idx<cfgLines.size();idx++) { opts=cfgLines[idx].split("|");
     if (opts[0].trimmed()=="NODE") { idx2=idx; break; }
    }

    // Separate section parameter lines
    for (int idx=idx1;idx<idx2;idx++) {
     opts=cfgLines[idx].split("#"); opts=opts[0].split("|"); // Get rid off any following comment.
          if (opts[0].trimmed()=="AMP") ampSection.append(opts[1]);
     else if (opts[0].trimmed()=="NET") netSection.append(opts[1]);
     else if (opts[0].trimmed()=="CHN") chnSection.append(opts[1]);
     else {
      qCritical() << "<ConfigParser> ERROR: Unknown section in config file!";
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
        qCritical() << "<ConfigParser> <AMP> ERROR: Currently more than" \
		    << EE_MAX_AMPCOUNT \
                    << "simultaneously connected amplifiers is not supported!";
        return true;
       }
      } else if (opts[0].trimmed()=="BUFPAST") {
       conf->tcpBufSize=opts[1].toInt();
       if (!(conf->tcpBufSize >= 1 && conf->tcpBufSize <= 200)) {
        qCritical() << "<ConfigParser> <AMP> ERROR: BUFPAST not within [1,320] seconds range!";
        return true;
       }
      } else if (opts[0].trimmed()=="EEGRATE") {
       conf->eegRate=opts[1].toInt();
       if (!(conf->eegRate == 500 || conf->eegRate == 1000)) {
        qCritical() << "<ConfigParser> <AMP> ERROR: EEG Samplerate not among {500,1000}!";
        return true;
       }
      } else if (opts[0].trimmed()=="EEGPROBEMS") {
       conf->eegProbeMsecs=opts[1].toInt();
       if (!(conf->eegProbeMsecs >= 20 && conf->eegProbeMsecs <= 1000)) {
        qCritical() << "<ConfigParser> <AMP> ERROR: EEGPROBEMS not within [20,1000] msecs range!";
        return true;
       }
      } else if (opts[0].trimmed()=="REFGAIN") {
       conf->refGain=opts[1].toFloat();
       if (!(conf->refGain==1.0 || conf->refGain==2.0)) {
        qCritical() << "<ConfigParser> <AMP> ERROR: REFGAIN not among {1.0,2.0}x range!";
        return true;
       }
      } else if (opts[0].trimmed()=="BIPGAIN") {
       conf->bipGain=opts[1].toFloat();
       if (!(conf->bipGain==4.0 || conf->bipGain==8.0)) {
        qCritical() << "<ConfigParser> <AMP> ERROR: BIPGAIN not among {4.0,8.0}x range!";
        return true;
       }
      } else if (opts[0].trimmed()=="AUDTRIGTHR") {
       conf->audTrigThr=opts[1].toInt();
       if (!(conf->audTrigThr>20 && conf->audTrigThr<32000)) {
        qCritical() << "<ConfigParser> <AMP> ERROR: AUDTRIGTHR not among {20,32000}x range!";
        return true;
       }
      } else {
       qCritical() << "<ConfigParser> <AMP> ERROR: Unknown subsection in AMP section!";
       return true;
      }
     }
    } else {
     qCritical() << "<ConfigParser> <AMP> ERROR: No parameters in section!";
     return true;
    }

    // NET section
    if (netSection.size()>0) {
     for (const auto& sect:netSection) {
      opts=sect.split("=");
      if (opts[0].trimmed()=="ACQ") {
       opts2=opts[1].split(","); // IP, command port, stream port and commonmode port are separated by ","
       if (opts2.size()==3) {
        QHostInfo acqHostInfo=QHostInfo::fromName(opts2[0].trimmed());
        conf->acqIpAddr=acqHostInfo.addresses().first().toString();
        conf->acqCommPort=opts2[1].toInt(); conf->acqStrmPort=opts2[2].toInt();
        if ((!(conf->acqCommPort >= 65000 && conf->acqCommPort < 65999)) || // Simple port validation
            (!(conf->acqStrmPort >= 65000 && conf->acqStrmPort < 65999))) {
         qCritical() << "<ConfigParser> <NET> ERROR: Invalid hostname/IP/port settings!";
         return true;
        }
       } else {
        qCritical() << "<ConfigParser> <NET> ERROR: Invalid count of NET|OUT params!";
        return true;
       }
      } else {
       qCritical() << "<ConfigParser> <NET> ERROR: Invalid hostname/IP(v4) address!";
       return true;
      }
     }
    } else {
     qCritical() << "<ConfigParser> <NET> ERROR: No parameters in section!";
     return true;
    }

    // CHN section
    conf->refChnCount=conf->bipChnCount=conf->metaChnCount=0;
    if (chnSection.size()>0) {
     for (const auto& sect:chnSection) {
      opts=sect.split("=");
      if (opts[0].trimmed()=="APPEND") {
       opts=opts[1].split(">");
       opts2=opts[0].split(",");
       if (opts2.size()==7) {
        opts2[0]=opts2[0].trimmed(); // Ref vs. Bip vs. Meta
        opts2[2]=opts2[2].trimmed(); // Channel name
        if ((opts2[0].size()!=1) || !(opts2[0]=="R" || opts2[0]=="B" || opts2[0]=="M") ||
            (!((unsigned)opts2[1].toInt()>0  && (unsigned)opts2[1].toInt()<=512)) || // Channel#
            (!(opts2[2].size()>0 && opts2[2].size()<=3)) || // Channel name must be 1 to 3 chars..
            (!((float)opts2[3].toFloat()>=0. && (float)opts2[3].toFloat()<360.)) || // TopoThetaPhi - Theta
            (!((float)opts2[4].toFloat()>=0. && (float)opts2[4].toFloat()<360.)) || // TopoThetaPhi - Phi
            (!((unsigned)opts2[5].toInt()>=1 && (unsigned)opts2[5].toInt()<=11)) || // TopoXY - X
            (!((unsigned)opts2[6].toInt()>=1 && (unsigned)opts2[6].toInt()<=11))) { // TopoXY - Y
         qCritical() << "<ConfigParser> <CHN> ERROR: Invalid parameter!";
         return true;
        } else { // Set and append new channel..
         dummyChnInfo.physChn=opts2[1].toInt();      // Physical channel
         dummyChnInfo.chnName=opts2[2];              // Channel name
         dummyChnInfo.topoTheta=opts2[3].toFloat();  // TopoThetaPhi - Theta
         dummyChnInfo.topoPhi=opts2[4].toFloat();    // TopoThetaPhi - Phi
         dummyChnInfo.topoX=opts2[5].toInt();        // TopoXY - X
         dummyChnInfo.topoY=opts2[6].toInt();        // TopoXY - Y
         if (opts2[0]=="R") dummyChnInfo.type=0; // referential
         else if (opts2[0]=="B") dummyChnInfo.type=1; // bipolar
         else if (opts2[0]=="M") dummyChnInfo.type=2; // meta
         else {
          qCritical() << "<ConfigParser> <CHN> ERROR: Invalid channel type in APPEND parameters!";
          return true;
         }
         // Intermodes are decided to be irrelevant within node-acq.
         // node-acq only predefines the neighborhood of each electrode via its config file
         // and conveys this information to node-acq-pp. Hence the recordings will only have
         // the original/not-interpolated EEG data; as node-stor relies on node-acq but not node-acq-pp.
        }
       } else {
        qCritical() << "<ConfigParser> <CHN> ERROR: Invalid count of parameters in APPEND line!";
	return true;
       }
       opts2=opts[1].split(","); // Interpolation electrodes
       if (opts2.size()>=1 && opts2.size()<=6) {
        dummyChnInfo.interElec.resize(0);
        if (opts2[0].toInt()==0) {
         if (dummyChnInfo.type==0) dummyChnInfo.interElec.append(dummyChnInfo.physChn-1); // Only for referential
	 else dummyChnInfo.interElec.append(0);
	} else {
	 for (int idx=0;idx<opts2.size();idx++) dummyChnInfo.interElec.append(opts2[idx].toInt()-1);
	}
       } else {
        qCritical() << "<ConfigParser> <CHN> ERROR: Invalid count of APPEND (chn interpolation) parameters!";
	return true;
       }
       switch (dummyChnInfo.type) { // Add channel to related info table
        case 0:
        default: refChns->append(dummyChnInfo); break;
        case 1: bipChns->append(dummyChnInfo); break;
        case 2: metaChns->append(dummyChnInfo); break;
       }
      }
     }
    } else {
     qCritical() << "<ConfigParser> <CHN> ERROR: No parameters in section!";
     return true;
    }

    conf->refChnCount=refChns->size(); conf->bipChnCount=bipChns->size(); conf->metaChnCount=metaChns->size();
    //qInfo() << "EEG channels:" << conf->refChnCount;
    //for (const auto& c:conf->refChns) qInfo() << c.physChn << c.chnName << c.topoX << c.topoY;
    //qInfo() << "BP channels:" << conf->bipChnCount;
    //for (const auto& c:conf->bipChns) qInfo() << c.physChn << c.chnName << c.topoX << c.topoY;
    //qInfo() << "META channels:" << conf->metaChnCount;
    //for (const auto& c:conf->metaChns) qInfo() << c.physChn << c.chnName << c.topoX << c.topoY;

   } // File open
   return false;
  }

 private:
  QString cfgPath,cfgLine; QFile cfgFile;
};
