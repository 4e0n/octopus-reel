void parseConfig(QStringList cfgLines) {
 QStringList cfgValidLines,opts,opts2,opts3,
             bufSection,netSection,avgSection,evtSection,
             chnSection,digSection,guiSection,modSection;

 for (int i=0;i<cfgLines.size();i++) { // Isolate valid lines
  if (!(cfgLines[i].at(0)=='#') &&
      cfgLines[i].contains('|')) cfgValidLines.append(cfgLines[i]); }

 // *** CATEGORIZE SECTIONS ***

 initSuccess=true;
 for (int i=0;i<cfgValidLines.size();i++) {
  opts=cfgValidLines[i].split("|");
       if (opts[0].trimmed()=="AVG") avgSection.append(opts[1]);
  else if (opts[0].trimmed()=="CHN") chnSection.append(opts[1]);
  else if (opts[0].trimmed()=="GUI") guiSection.append(opts[1]);
  else if (opts[0].trimmed()=="MOD") modSection.append(opts[1]);
  else { qDebug("Unknown section in .cfg file!"); initSuccess=false; break; }
 } if (!initSuccess) return;

 // AVG
 if (avgSection.size()>0) {
  for (int i=0;i<avgSection.size();i++) { opts=avgSection[i].split("=");
   if (opts[0].trimmed()=="INTERVAL") { opts2=opts[1].split(",");
    if (opts2.size()==4) {
     cp.rejBwd=opts2[0].toInt(); cp.avgBwd=opts2[1].toInt();
     cp.avgFwd=opts2[2].toInt(); cp.rejFwd=opts2[3].toInt();
     if ((!(cp.rejBwd >= -1000 && cp.rejBwd <=    0)) ||
         (!(cp.avgBwd >= -1000 && cp.avgBwd <=    0)) ||
         (!(cp.avgFwd >=     0 && cp.avgFwd <= 1000)) ||
         (!(cp.rejFwd >=     0 && cp.rejFwd <= 1000)) ||
         (cp.rejBwd>cp.avgBwd) ||
         (cp.avgBwd>cp.avgFwd) || (cp.avgFwd>cp.rejFwd)) {
      qDebug(".cfg: Logic error in AVG|INTERVAL parameters!");
      initSuccess=false; break;
     } else {
      cp.avgCount=(cp.avgFwd-cp.avgBwd)*sampleRate/1000;
      cp.rejCount=(cp.rejFwd-cp.rejBwd)*sampleRate/1000;
      cp.postRejCount=(cp.rejFwd-cp.avgFwd)*sampleRate/1000;
      cp.bwCount=cp.rejFwd*sampleRate/1000;
     }
    } else {
     qDebug(".cfg: Parse error in AVG|INTERNAL parameters!");
     initSuccess=false; break;
    }
   }
  }
 } else {
  cp.rejBwd=-300; cp.avgBwd=-200; cp.avgFwd=500; cp.rejFwd=600;
  cp.avgCount=700*sampleRate/1000; cp.rejCount=900*sampleRate/1000;
  cp.postRejCount=100*sampleRate/1000; cp.bwCount=600*sampleRate/1000;
 } if (!initSuccess) return;

 // CHN
 if (chnSection.size()>0) {
  for (int i=0;i<chnSection.size();i++) { opts=chnSection[i].split("=");
   if (opts[0].trimmed()=="DEFINE") { opts2=opts[1].split(",");
    if (opts2.size()==10) {
     opts2[1]=opts2[1].trimmed();
     opts2[4]=opts2[4].trimmed(); opts2[5]=opts2[5].trimmed(); // Trim wspcs
     opts2[6]=opts2[6].trimmed(); opts2[7]=opts2[7].trimmed();
     opts2[8]=opts2[8].trimmed(); opts2[9]=opts2[9].trimmed();
     if ((!(opts2[0].toInt()>0 && opts2[0].toInt()<=128)) || // Channel#
         (!(opts2[1].size()>0)) || // Channel name must be at least 1 char..

         (!(opts2[2].toInt()>=0 && opts2[2].toInt()<1000))   || // Rej
         (!(opts2[3].toInt()>=0 && opts2[3].toInt()<=128)) || // RejRef

         (!(opts2[4]=="T" || opts2[4]=="t" ||
          opts2[4]=="F" || opts2[4]=="f")) ||
         (!(opts2[5]=="T" || opts2[5]=="t" ||
          opts2[5]=="F" || opts2[5]=="f")) ||
         (!(opts2[6]=="T" || opts2[6]=="t" ||
          opts2[6]=="F" || opts2[6]=="f")) ||
         (!(opts2[7]=="T" || opts2[7]=="t" ||
          opts2[7]=="F" || opts2[7]=="f")) ||

         (!(opts2[8].toFloat()>=0. && opts2[8].toFloat()<=360.)) || // Theta 
         (!(opts2[9].toFloat()>=0. && opts2[9].toFloat()<=360.))){  // Phi
      qDebug(".cfg: Syntax/logic Error in Channel definitions!");
      initSuccess=false; break;
     } else { // Set and append new channel..
      dummyChn=new Channel(opts2[0].toInt(),	    // Physical channel
                           opts2[1].trimmed(),	    // Channel Name
                           opts2[2].toInt(),	    // Rejection Level
                           opts2[3].toInt(),	    // Rejection Reference
                           opts2[4],opts2[5],    // Cnt Vis/Rec Flags
                           opts2[6],opts2[7],    // Avg Vis/Rec Flags
                           opts2[8].toFloat(),   // Electrode Cart. Coords
                           opts2[9].toFloat());  // (Theta,Phi)
      dummyChn->setEventProfile(acqEvents.size(),cp.avgCount);
      acqChannels.append(dummyChn);
     }
    } else {
     qDebug(".cfg: Parse error in CHN|APPEND parameters!");
     initSuccess=false; break;
    }
   }
  }
 } else {
  // Default channel settings
  QString chnString,noString;
  for (int i=0;i<128;i++) {
   chnString="Chn#"+noString.setNum(i);
   dummyChn=new Channel(i,chnString,0,0,"t","t","t","t",0.,0.);
   acqChannels.append(dummyChn);
  }
 } if (!initSuccess) return;

 // GUI
 if (guiSection.size()>0) {
  for (int i=0;i<guiSection.size();i++) { opts=guiSection[i].split("=");
    if (opts[0].trimmed()=="MAIN") { opts2=opts[1].split(",");
    if (opts2.size()==4) {
     guiX=opts2[0].toInt(); guiY=opts2[1].toInt();
     guiWidth=opts2[2].toInt(); guiHeight=opts2[3].toInt();
     if ((!(guiX      >= -2000 && guiX      <= 2000)) ||
         (!(guiY      >= -2000 && guiY      <= 2000)) ||
         (!(guiWidth  >=   640 && guiWidth  <= 2000)) ||
         (!(guiHeight >=   480 && guiHeight <= 2000))) {
      qDebug(".cfg: GUI|MAIN size settings not in appropriate range!");
      initSuccess=false; break;
     }
    } else {
     qDebug(".cfg: Parse error in GUI settings!");
     initSuccess=false; break;
    }
   } else if (opts[0].trimmed()=="HEAD") { opts2=opts[1].split(",");
    if (opts2.size()==4) {
     hwX=opts2[0].toInt(); hwY=opts2[1].toInt();
     hwWidth=opts2[2].toInt(); hwHeight=opts2[3].toInt();
     if ((!(hwX      >= -2000 && hwX      <= 2000)) ||
         (!(hwY      >= -2000 && hwY      <= 2000)) ||
         (!(hwWidth  >=   400 && hwWidth  <= 2000)) ||
         (!(hwHeight >=   300 && hwHeight <= 2000))) {
      qDebug(".cfg: GUI|HEAD size settings not in appropriate range!");
      initSuccess=false; break;
     }
    } else {
     qDebug(".cfg: Parse error in Head Widget settings!");
     initSuccess=false; break;
    }
   }
  }
 } else {
  guiX=60; guiY=60; guiWidth=640; guiHeight=480;
  hwX=160; hwY=160; hwWidth=400; hwHeight=300;
 } if (!initSuccess) return;

 // MOD
 if (modSection.size()>0) {
  for (int i=0;i<modSection.size();i++) { opts=modSection[i].split("=");
   if (opts[0].trimmed()=="GIZMO") { opts2=opts[1].split(",");
    if (opts2.size()==1) {
     if (opts2[0].size()) loadGizmo_OgzFile(opts2[0].trimmed());
     else {
      qDebug(".cfg: MOD|GIZMO filename error!");
      initSuccess=false; break;
     }
    } else {
     qDebug(".cfg: Parse error in MOD|GIZMO parameters!");
     initSuccess=false; break;
    }
   } else if (opts[0].trimmed()=="SCALP") { opts2=opts[1].split(",");
    if (opts2.size()==1) loadScalp_ObjFile(opts2[0].trimmed());
    else {
     qDebug(".cfg: Parse error in MOD|SCALP parameters!");
     initSuccess=false; break;
    }
   }
  }
 } if (!initSuccess) return;
} 
