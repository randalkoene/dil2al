// controller.cc
// Randal A. Koene, 20000304

#include "dil2al.hh"

String chunk_controller(String controllercmd) {
  // Called timechunk minutes after starting a task chunk
  // (e.g. via at-jobs or a timer daemon, which could be
  // constructed as an AfterStep Wharf applet).
  // It can also be called before timechunk minutes have
  // passed if the chunk is stopped early.
  // As previous chunks are closed with stop_TL_chunk() where
  // necessary with options C,S or T, AL updating options
  // determine if and how the current AL and closed chunk's DIL
  // entry should be updated, and the corresponding update is done.

  _EXTRA_VERBOSE_REPORT_FUNCTION();
  
  // check if audio-visual flag is to be made noticable
  if (showflag) {
    cout << "\a";
    if (flagcmd!="") {
      if (System_Call(flagcmd)<0) WARN << "chunk_controller(): Unable to show visual flag. Continuing.\n";
    }
  }

  // show next AL item(s) (may involve recomputing)
  //*** recomputation of AL not yet included
#ifdef DEBUG
  cout << "dil2al: DEBUG - Recomputation of ALs in chunk_controller() not yet implemented\n";
#endif

  // offer opportunity to make a Task Log entry
  bool madenote = false;
  chunkcreated = false; // reset process variable
  if (useansi) cout << ANSI_TEXT_RED;
  ANSI_bold_on();
  bool mknt = confirmation("Make Note/Task Log entry? (y/N) ",'y',"No","Yes");
  ANSI_bold_off();
  if (!mknt) { // make Task Log Entry
    if (!make_note("")) WARN << "chunk_controller(): Unable to make note. Continuing as is.\n";
    madenote = true;
  }
  
  // offer controller options
  if (useansi) cout << ANSI_TEXT_GREEN;
  TL_chunk_access TL;
  String respchars;
  char ccstr = 0;
  while (!ccstr) {
    StringList respdescriptors;
    String posttaskguide, message, pretaskguide;
    if (read_file_into_String(basedir+RELLISTSDIR+"System/system-summary.guide-text.after-task.txt",posttaskguide,true)) {
      message = "Post-Task Guide:\n";
      message += posttaskguide;
    }
    if (!read_file_into_String(basedir+RELLISTSDIR+"System/system-summary.guide-text.before-task.txt",pretaskguide,true)) pretaskguide="";
    message += "\nOptions:\n[C]ontinue Task Chunk if Time Remaining else Start New Task Chunk\n";
    if (!chunkcreated) {
      message += "[S]tart New Task Chunk (new task context)\n";
      respchars = "cstx";
      respdescriptors.split(0,"Continue;Start New;Stop Task;Exit",';');
    } else {
      respchars = "ctx";
      respdescriptors.split(0,"Continue;Stop Task;Exit",';');
    }
    message += "S[t]op Task Chunk (pause list tasks)\nE[x]it Retaining Current State (ignore list suggestions)\n";
    ANSI_bold_on();
    if ((ccstr = auto_interactive(&controllercmd,message,respchars,respdescriptors))<0) return String("");
    ANSI_bold_off();
    if (!ccstr) continue; // This can happen if a UI_ window was closed without making a selection.
    // refresh curtime to avoid misrepresentations due to long delay before option selection
    // but don't change time if madenote to properly detect chunkid==curtime
    // if a new chunk was made during make_note()
    if (!madenote) curtime = time_stamp("%Y%m%d%H%M");
    switch (ccstr) {
    case 'c': case 'C':
      if (verbose) INFO << "Continuing according to task list chunk controller suggestions.\n";
      TL.tlinsertloc = -1;
      if (!read_file_into_String(get_TL_head(),TL.tl)) return String("");
      if (TL.tl.empty()) return TL.tl;
      if (!decide_add_TL_chunk(TL,true,false)) return String("");
      if (!pretaskguide.empty()) INFO << "Pre-Task Guide:\n" << pretaskguide;
      break;
    case 's': case 'S':
      if (!chunkcreated) {
	if (verbose) INFO << "Starting task chunk with new context\n";
	TL.tlinsertloc = -1;
	if (!read_file_into_String(get_TL_head(),TL.tl)) return String("");
	if (TL.tl.empty()) return TL.tl;
	if (!decide_add_TL_chunk(TL,false,false)) return String("");
	if (!pretaskguide.empty()) INFO << "Pre-Task Guide:\n" << pretaskguide;
      } else {
	WARN << "chunk_controller(): Unknown option (" << ccstr << ").\n";
	ccstr = 0;
      }
      break;
    case 't': case 'T':
      //*** instead of this approach, which exits once list tasks are paused
      //*** it is possible to stay in this while loop and offer exit as well
      //*** as start with comparetimes=false in the decide_add_TL_chunk() call
      if (verbose) INFO << "Stopping task chunk and pausing list tasks.\n";
      if (!read_file_into_String(get_TL_head(),TL.tl)) return String("");
      if (TL.tl.empty()) return TL.tl;
      if (!stop_TL_chunk(TL.tl)) return String("");
      if (!write_file_from_String(taskloghead,TL.tl,"Task Log")) return String("");
      TL.chunkid = "-1";
      break;
    case 'x': case 'X':
      if (verbose) INFO << "Exiting and retaining current task state.\n";
      TL.chunkid = "-2";
      break;
    default:
      WARN << "chunk_controller(): Unknown option (" << ccstr << ").\n";
      ccstr = 0;
    }
  }
  if (useansi) cout << ANSI_NORMAL;
  
  return TL.chunkid;
}

inline time_t time_chunk_remaining(String chunkid) {
  time_t tdiff = timechunksize*60;
  tdiff -= time_stamp_diff(chunkid,time_stamp("%Y%m%d%H%M"));
  if (verbose) INFO << "\nTask schedule controller: " << tdiff << " seconds remaining in time chunk\n(Current chunk initiated at " << chunkid << ")\n";
  return tdiff;
}

bool schedule_controller(String controllercmd) {
  // schedules periodic calls to chunk_controller(),
  // either via an at-job or by retaining dil2al as a daemon
  //
  // *** A real Daemon could also have a request queue and
  //     improve operating efficiency by keeping many things
  //     in memory while waiting for requests to come in.
  //     Take note of the notice about flushing the output log
  //     at the end of a request (see interface.cc), unless
  //     a longer, multi-request log is desirable.
  
  String chunkid;
  
  if (isdaemon) {
    // daemon schedules chunk_controller()
    String ccmd;
    while (1) {
      ccmd = controllercmd; // fresh copy of automatic commands to controller
      if ((chunkid=chunk_controller(ccmd))=="") return false;
      if ((chunkid=="-1") || (chunkid=="-2")) return true;
      // NOTE: Flushing here, so that error, warn and info are not saved up
      // for some unknown number of iterations.
      // *** Alternatives are to a) only flush when a new task is started, or
      //     b) flush only every N iterations.
      ERROR.flush(); WARN.flush(); INFO.flush();
      time_t tdiff = time_chunk_remaining(chunkid);
      if (tdiff>0) sleep(tdiff);
    }
    
  } else {
    // at-job schedules chunk_controller()
    if ((chunkid=chunk_controller(controllercmd))=="") return false;
    if ((chunkid=="-1") || (chunkid=="-2")) return true;
#ifdef DEBUG
    cout << "current chunkid = " << chunkid << '\n';
#endif
    time_t tdiff = time_chunk_remaining(chunkid);
#ifdef DEBUG
    cout << "seconds remaining = " << tdiff << '\n';
#endif
    if (tdiff>0) {
      tdiff /= 60;
      String atcmd = atcommand;
      if (tdiff) { atcmd += " + "; atcmd += String((long) tdiff); atcmd += " minutes"; }
      else atcmd += " + 1 minute";
      FILE * p = popen((const char *) atcmd, "w");
      if (!p) {
	ERROR << "schedule_controller(): Unable to schedule controller call with " << atcontroller << ".\n";
	return false;
      }
      fprintf(p,"%s",(const char *) atcontroller);
      if (pclose(p)==-1) {
	ERROR << "schedule_controller(): Scheduling controller call with " << atcontroller << " did not complete operations.\n";
	return false;
      }
    }
  }
  
#ifdef DEBUG
  const int LLEN = 10;
  char lbuf[LLEN];
  cout << "Scheduled controller processing completed...\n";
  cin.getline(lbuf,LLEN);
#endif
  
  return true;
}

#define TWENTYMINUTESINSECONDS (20*60)
bool first_call_page() {
  // load a page template
  String firstcallpage;
  if (!read_file_into_String(firstcallpagetemplate,firstcallpage)) return false;
  // find the current time
  time_t current_time_of_day = time_stamp_time_of_day(curtime);
  time_t current_date = time_stamp_time_date(curtime);
  time_t previous_day_end_of_day = current_date - 1;
  // find the most recent task chunk end time in the task log
  String mostrecentTL;
  if (!read_file_into_String(tasklog,mostrecentTL)) return false;
  String tmrestr; time_t t_most_recent_end = -1; int tmreloc = -1;
  if ((tmreloc=mostrecentTL.index("<!-- chunk End --><I>",-1))>=0) {
    tmrestr = mostrecentTL.at(tmreloc+21,12);
    t_most_recent_end = time_stamp_time(tmrestr,true); // date and time expressed in seconds, allow -1 return (20190127)
  } else {
    ERROR << "first_call_page(): Most recent Task Log file appears to contain no Task Chunk end tag.\n";
    return false;
  } 
  // find the most recent task chunk begin time in the task log
  String tmrbstr; time_t t_most_recent_begin = -1; int tmrbloc = -1;
  if ((tmrbloc=mostrecentTL.index("<!-- chunk Begin --><TR><TD><FONT COLOR=\"#FFFFFF\"><A NAME=\"",-1))>=0) {
    tmrbstr = mostrecentTL.at(tmrbloc+59,12);
    t_most_recent_begin = time_stamp_time(tmrbstr); // date and time expressed in seconds, noexit=false (20190127)
  } else {
    ERROR << "first_call_page(): Most recent Task Log file appears to contain no Task Chunk begin tag.\n";
    return false;
  }
  // put brackets where t<=max(t_most_recent_begin,t_most_recent_end)
  time_t t_most_recent_logged = -1; // expressed in seconds of the day (no date included)
  time_t t_date_most_recent_logged = time_stamp_time_date(tmrestr);
  if (t_most_recent_begin>t_most_recent_end) t_date_most_recent_logged = time_stamp_time_date(tmrbstr);
  if (current_date==t_date_most_recent_logged) {
    if (t_most_recent_begin>t_most_recent_end) t_most_recent_logged = time_stamp_time_of_day(tmrbstr);
    else t_most_recent_logged = time_stamp_time_of_day(tmrestr);
    if (t_most_recent_logged>current_time_of_day) t_most_recent_logged = -1; // must be from a preceding day
  } else if (current_date<t_date_most_recent_logged) { // Beware! This should not happen!
    t_most_recent_logged = 100000;
  }
  for (long table_loc = 0; table_loc < (24*3); table_loc++) {
    time_t t_compare(table_loc*TWENTYMINUTESINSECONDS); // expressed in seconds of the day (no date included)
    int in_table_loc = -1;
    String tlocstr(table_loc); if (tlocstr.length()<2) tlocstr += ' ';
    if ((in_table_loc=firstcallpage.index(">"+tlocstr+'('))>=0) {
      if (t_compare>t_most_recent_logged) { firstcallpage[in_table_loc+3] = ' '; firstcallpage[in_table_loc+88] = ' '; }
      firstcallpage[in_table_loc+1] = ' ';
      firstcallpage[in_table_loc+2] = ' ';
      // bold where t>=t_current
      if (t_compare>=current_time_of_day) {
	int fontstyle_loc = -1;
	if ((fontstyle_loc=firstcallpage.index("<I>",in_table_loc))>=0) {
	  firstcallpage[fontstyle_loc+1] = 'B';
	  if ((fontstyle_loc=firstcallpage.index("</I>",fontstyle_loc))>=0) firstcallpage[fontstyle_loc+2] = 'B';
	}
      }
    }
  }
  // fill in today's date and time
  firstcallpage.gsub("YYYYMMDD",curdate);
  firstcallpage.gsub("CURRENTDATE",curdate);
  if (current_date<t_date_most_recent_logged) {
    firstcallpage.gsub("CURRENTTIMECOMPARE",curtime+" <B>Beware: For some reason this is earlier than the last task chunk logged!</B>");
  } else {
    firstcallpage.gsub("CURRENTTIMECOMPARE",curtime);
  }
  firstcallpage.gsub("CURRENTTIME",curtime);
  firstcallpage.gsub("TASKCHUNKEND",tmrestr);
  firstcallpage.gsub("TASKCHUNKBEGIN",tmrbstr);
  firstcallpage.gsub("PREVIOUSDAYEND",time_stamp("%Y%m%d%H%M",previous_day_end_of_day));
  firstcallpage.gsub("SWITCHCALLPAGE","Second Call Page");
  firstcallpage.gsub("SWITCHPAGECGI","secondcallpage.cgi");
  // write the page
  return write_file_from_String(firstcallpagetarget,firstcallpage,"",true);
}

#define FIVEMINUTESINSECONDS (5*60)
#define SIXTYMINUTESINSECONDS (60*60)
bool second_call_page() {
  // load a page template
  String firstcallpage;
  if (!read_file_into_String(firstcallpagetemplate,firstcallpage)) return false;
  int table_start = firstcallpage.index("<!-- table start -->");
  String secondcallpage = firstcallpage.before(table_start);
  int table_end = firstcallpage.index("<!-- table end -->");
  // find the current time
  time_t current_time = time_stamp_time(curtime);
  //time_t current_time_of_day = time_stamp_time_of_day(curtime);
  time_t current_date = time_stamp_time_date(curtime);
  time_t previous_day_end_of_day = current_date - 1;
  // find the most recent task chunk end time in the task log
  String mostrecentTL;
  if (!read_file_into_String(tasklog,mostrecentTL)) return false;
  String tmrestr; time_t t_most_recent_end = -1; int tmreloc = -1;
  if ((tmreloc=mostrecentTL.index("<!-- chunk End --><I>",-1))>=0) {
    tmrestr = mostrecentTL.at(tmreloc+21,12);
    t_most_recent_end = time_stamp_time(tmrestr,true); // date and time expressed in seconds, allow -1 return (20190127)
  } else {
    ERROR << "first_call_page(): Most recent Task Log file appears to contain no Task Chunk end tag.\n";
    return false;
  } 
  // find the most recent task chunk begin time in the task log
  String tmrbstr; time_t t_most_recent_begin = -1; int tmrbloc = -1;
  if ((tmrbloc=mostrecentTL.index("<!-- chunk Begin --><TR><TD><FONT COLOR=\"#FFFFFF\"><A NAME=\"",-1))>=0) {
    tmrbstr = mostrecentTL.at(tmrbloc+59,12);
    t_most_recent_begin = time_stamp_time(tmrbstr); // date and time expressed in seconds, noexit=false (20190127)
  } else {
    ERROR << "first_call_page(): Most recent Task Log file appears to contain no Task Chunk begin tag.\n";
    return false;
  }
  // put brackets where t<=max(t_most_recent_begin,t_most_recent_end)
  time_t t_most_recent_logged = -1; // expressed in seconds of the day (no date included)
  time_t t_date_most_recent_logged = time_stamp_time_date(tmrestr);
  if (t_most_recent_begin>t_most_recent_end) t_date_most_recent_logged = time_stamp_time_date(tmrbstr);
  if (current_date==t_date_most_recent_logged) {
    if (t_most_recent_begin>t_most_recent_end) t_most_recent_logged = time_stamp_time(tmrbstr);
    else t_most_recent_logged = time_stamp_time(tmrestr);
    //if (t_most_recent_logged>current_time_of_day) t_most_recent_logged = -1; // must be from a preceding day
  } else if (current_date<t_date_most_recent_logged) { // Beware! This should not happen!
    t_most_recent_logged = 10000000000;
  }
  // build the day's time table
  secondcallpage += "<PRE>\n";
  time_t next_date = current_date + (SECONDSPERDAY/2);
  for (time_t hour_date = current_date; hour_date<next_date; hour_date += SIXTYMINUTESINSECONDS) {
    time_t t = hour_date; // AM
    secondcallpage += time_stamp(" %H:",t);
    for (int i = 0; i<12; i++) {
      if (i>0) secondcallpage += ' ';
      if (t>t_most_recent_logged) secondcallpage += "<A HREF=\"file:///cgi-bin/dil2alfromweb.cgi?T="+time_stamp("%Y%m%d%H%M",t)+"&dil2al=S\">"; // add link
      if (t>=current_time) secondcallpage += "<B>"; // add bold
      secondcallpage += time_stamp("%M",t);
      if (t>=current_time) secondcallpage += "</B>"; // close bold
      if (t>t_most_recent_logged) secondcallpage += "</A>"; // close link
      t += FIVEMINUTESINSECONDS;
    }
    t = hour_date + (12*SIXTYMINUTESINSECONDS); // PM
    secondcallpage += time_stamp(" %H:",t);
    for (int i = 0; i<12; i++) {
      if (i>0) secondcallpage += ' ';
      if (t>t_most_recent_logged) secondcallpage += "<A HREF=\"file:///cgi-bin/dil2alfromweb.cgi?T="+time_stamp("%Y%m%d%H%M",t)+"&dil2al=S\">"; // add link
      if (t>=current_time) secondcallpage += "<B>"; // add bold
      secondcallpage += time_stamp("%M",t);
      if (t>=current_time) secondcallpage += "</B>"; // close bold
      if (t>t_most_recent_logged) secondcallpage += "</A>"; // close link
      t += FIVEMINUTESINSECONDS;
    }
    secondcallpage += "\n";
  }
  secondcallpage += "</PRE>\n";
  secondcallpage += firstcallpage.from(table_end);
  // fill in today's date and time
  secondcallpage.gsub("YYYYMMDD",curdate);
  secondcallpage.gsub("CURRENTDATE",curdate);
  if (current_date<t_date_most_recent_logged) {
    secondcallpage.gsub("CURRENTTIMECOMPARE",curtime+" <B>Beware: For some reason this is earlier than the last task chunk logged!</B>");
  } else {
    secondcallpage.gsub("CURRENTTIMECOMPARE",curtime);
  }
  secondcallpage.gsub("CURRENTTIME",curtime);
  secondcallpage.gsub("TASKCHUNKEND",tmrestr);
  secondcallpage.gsub("TASKCHUNKBEGIN",tmrbstr);
  secondcallpage.gsub("PREVIOUSDAYEND",time_stamp("%Y%m%d%H%M",previous_day_end_of_day));
  // when paging back, use secondcallpage.cgi
  secondcallpage.gsub("firstcallpage.cgi","secondcallpage.cgi");
  secondcallpage.gsub("SWITCHCALLPAGE","First Call Page");
  secondcallpage.gsub("SWITCHPAGECGI","firstcallpage.cgi");
  // write the page
  return write_file_from_String(firstcallpagetarget,secondcallpage,"",true);
}
