// alcomp.cc
// Randal A. Koene, 20000304, 20180104

// Note 20150404:
// The main function in this source file is generate_AL(), which is
// generally called from update_DIL_to_AL() below.
// A description of the AL and its updating is included in the Formalizer
// Documentation at ~/doc/html/formalizer-doc.html#AL-update and more
// details about the generate_AL() function are described in the text
// file ~/src/dil2al/generate_AL.txt.

// Note 20180104:
// Adding to alCRT_map::reserve() to enable prioritypreferences with separate
// end-of-day scheduling proposals for variable target date tasks, based on
// their priority rating. This is based on insights during Taking Stock in
// Notes on Change (Google Doc).

// Note 20180404:
// Added alCRTepsgroupoffset in alCRT_map::reserve() so that the spacing
// preserved between EPS groups allows for easier reordering of tasks or
// inserting of new tasks.

#include "dil2al.hh"

#define READSOMETYPE char*

time_t t_ctrstop = 0;

int get_topic_keywords() {
  const int LLEN = 10240;
  char lbuf[LLEN];
  ifstream dilfile;
  for (int i=0; i<numDILs; i++) {
    dilfile.open(*(dil[i]));
    if (dilfile) {
      if (!find_line(&dilfile,"Topic Keywords",lbuf,LLEN)) {
	ERROR << "get_topic_keywords(): Missing topic keywords in " << *(dil[i]) << ".\n";
	return 0;
      }
      dilfile.close();
    }
  }
  return 1;
}

String generate_AL_full_entry(String dilid, int chunkfreq) {
  String res("<LI><A HREF=\"detailed-items-by-ID.html#"+dilid+"\">"+dilid+"</A> ("+String((long)chunkfreq)+")\n");
  return res;
}

String generate_AL_entry(String dilid, bool ishighestpriority) {
  // Note: This is currently called only when dil2al is run with the
  // command line argument -a. This function generates an entry for manual
  // modification of an AL.
  
  String idstr;
  if (!read_file_into_String(idfile,idstr)) return String("");
  
  int idindex;
  idindex = idstr.index(BigRegex("[<]TD[^>]*[>][ 	]*[<]A[ 	]+[^>]*[Nn][Aa][Mm][Ee][ 	]*=[ 	]*\""+dilid));
  if (idindex<0) {
    ERROR << "generate_AL_entry(): Unable to find " << dilid << " in " << idfile << ".\n";
    return String("");
  }
  
  String topdil;
  topdil = idstr.at(BigRegex("[<]TD[^>]*[>][ 	]*[<]A[ 	]+[^>]*[Hh][Rr][Ee][Ff][ 	]*=[ 	]*\"[^\"]+"+dilid),idindex+1);
  if (topdil=="") {
    ERROR << "generate_AL_entry(): Unable to find full reference to " << dilid << ".\n";
    return String("");
  }
  topdil = topdil.after(BigRegex("[<]TD[^>]*[>][ 	]*[<]A[ 	]+[^>]*[Hh][Rr][Ee][Ff][ 	]*=[ 	]*\""));
  
  String topdilfile = topdil;
  topdil = basedir+RELLISTSDIR+topdilfile.before("#");
  if (!read_file_into_String(topdil,idstr)) return String("");
  
  idindex = idstr.index(BigRegex("[<]TD[^>]*[>][ 	]*[<]A[ 	]+[^>]*[Nn][Aa][Mm][Ee][ 	]*=[ 	]*\""+dilid));
  if (idindex<0) {
    ERROR << "generate_AL_entry(): Unable to find " << dilid << " in " << topdil << ".\n";
    return String("");
  }
  idindex = idstr.index(BigRegex("[<]TR[^>]*[>]"),idindex+1);
  if (idindex<0) return String("");
  idindex = idstr.index(BigRegex("[<]TD[^>]*[>]"),idindex+1);
  if (idindex<0) return String("");
  topdil = idstr.at(BigRegex("[<]TD[^>]*[>]...[^\n]*"),idindex);
  if (topdil=="") return String("");
  topdil = topdil.after(BigRegex("[<]TD[^>]*[>]"));
  topdil.gsub(BigRegex("[<][^>]+[>]"),"");
  if (topdil.length()>100) {
    topdil = topdil.before(100);
    topdil = topdil.before(BigRegex("[ 	\n.,;:]"),-1);
    topdil += "...";
  }
  idstr = "<TR";
  if (ishighestpriority) idstr += " BGCOLOR=\"#3F5F00\"";
  idstr += "><TD>[<A HREF=\""+topdilfile+"\"><B>"+dilid+"</B></A>][<A HREF=\"file:///cgi-bin/dil2al?dil2al=MEI&DILID="+dilid+"\">edit</A>]\n"+topdil;
  
  return idstr;
}

#ifdef INCLUDE_PERIODIC_TASKS
Calendar_Task * Calendar_Day::get_task(DIL_entry & de) {
  PLL_LOOP_FORWARD(Calendar_Task,tasks.head(),1) if ((&de)==(e->de)) return e;
  return NULL;
}

Calendar_Task * Calendar_Day::add_task(DIL_entry & de) {
  Calendar_Task * ct = new Calendar_Task(de,numdays);
  tasks.link_before(ct);
  return ct;
}

Calendar_Task::Calendar_Task(DIL_entry & _de, unsigned int multi_days): de(&_de), earliest(SECONDSPERDAY*multi_days), tdidx(-1), numdays(multi_days) {
  numchunks = (numdays*SECONDSPERDAY)/(timechunksize*60);
  chunkdivider = numchunks/alfocusedCALcolumns;
  if (chunkdivider<1) chunkdivider=1;
  numchunks = numchunks/chunkdivider;
  allocation = new char[numchunks];
  for (int i=0; i<numchunks; i++) allocation[i] = CALTASK_EMPTY;
}

void Calendar_Task::mark_chunk(time_t tcstart, time_t daystart) {
  tcstart -= daystart;
  if ((tcstart<0) || (tcstart>=(numdays*SECONDSPERDAY))) {
    WARN << "Calendar_Task::mark_chunk(): Task chunk is not within range of " << numdays << " days at " << time_stamp("%Y%m%d%H%M",tcstart+daystart) << ".\nContinuing as is.\n";
    return;
  }
  int chunkidx = (tcstart/(timechunksize*60))/chunkdivider;
  if (chunkidx<earliest) earliest = chunkidx;
  if (de->tdexact()) allocation[chunkidx] = CALTASK_EXACT;
  else if (de->tdfixed()) allocation[chunkidx] = CALTASK_FIXED;
  else allocation[chunkidx] = CALTASK_VARIABLE;
}

void Calendar_Task::mark_target_date(time_t td, time_t daystart) {
  td -= (daystart + 1); // subtracting one more insures that tasks to be done by 16:00 are only worked in task chunks up to 15:59:59, etc.
  if ((td<0) || (td>=(numdays*SECONDSPERDAY))) {
    WARN << "Calendar_Task::mark_target_date(): Target Date is not within range of " << numdays << " days at " << time_stamp("%Y%m%d%H%M",td+daystart) << ".\nContinuing as is.\n";
    return;
  }
  int chunkidx = (td/(timechunksize*60))/chunkdivider;
  tdidx = chunkidx;
  if (allocation[chunkidx]==CALTASK_EMPTY) allocation[chunkidx] = CALTASK_TD;
  else if (de->tdexact()) allocation[chunkidx] = CALTASK_TD_EXACT;
  else if (de->tdfixed()) allocation[chunkidx] = CALTASK_TD_FIXED;
  else allocation[chunkidx] = CALTASK_TD_VARIABLE;
}

void Calendar_Task::mark_intervals() {
  if (earliest<numchunks) {
    for (int i=earliest+1; i<numchunks; i++) {
      if ((tdidx>=0) && (i>=tdidx)) break;
      if (allocation[i]==CALTASK_EMPTY) allocation[i]=CALTASK_INTERVAL;
    }
  }
}

void Calendar_Day::gather_day_tasks(AL_Day & AD) {
  // Collect tasks and their task chunks on this day (or numdays)
  AL_Day * ad = &AD;
  for (int i=0; ((i<numdays) && (ad)); i++) { // in case of multiple days
    PLL_LOOP_FORWARD(AL_TC,ad->tc.head(),1) {
      DIL_entry * de  = e->Get_DE();
      if (de) {
	Calendar_Task * ct = get_task(*de); // is it already in the list for the calendar day?
	if (!ct) ct = add_task(*de); // if not then add a task
	ct->mark_chunk(e->TCstart(),daystart); // mark the task chunk for that task
      } else WARN << "Calendar_Day::gather_day_tasks(): Missing DIL entry in array at day " << time_stamp("%Y%m%d",ad->DayDate()) << ".\nContinuing as is.\n";
    }
    // Collect tasks with target dates on this day
    PLL_LOOP_FORWARD(AL_TD,ad->TD_Head(),1) {
      DIL_entry * de = e->DE()->dep;
      Calendar_Task * ct = get_task(*de); // is it already in the list for the calendar day?
      if (!ct) ct = add_task(*de); // if not then add a task
      ct->mark_target_date(e->DE()->td,daystart); // mark the target date task chunk for that task
    }
    ad = ad->Next();
  }
  // Fill in intervening time-intervals
  PLL_LOOP_FORWARD(Calendar_Task,tasks.head(),1) e->mark_intervals();
}

String Calendar_Task::HTML_Chunks_by_Task(String & dstfile) {
  // dstfile is supplied in order to generate correct relative HREF references
  String res("<TR><TD>");
  String diltext, dilurl;
  if (de->Entry_Text()) diltext = (*de->Entry_Text());
  else {
    WARN << "Calendar_Task::HTML_Day_Chunks_by_Task(): Missing Text Content for DIL entry #" << de->chars() << ". Continuing.\n";
    diltext="";
  }
  HTML_remove_tags(diltext);
  Elipsis_At(diltext,alfocusedCALexcerptlength);
  if (de->Topics(0)) {
    dilurl = de->Topics(0)->dil.file+'#'+de->chars();
    dilurl = relurl(dstfile,dilurl);
  } else {
    WARN << "Calendar_Task::HTML_Chunks_by_Task(): Missing URL for DIL entry #" << de->chars() << ". Continuing.\n";
    dilurl='#'+de->chars();
  }
  res += HTML_put_href(dilurl,de->chars())+':';
  if (de->tdexact()) res += CALTASK_TD_EXACT;
  else if (de->tdfixed()) res += CALTASK_TD_FIXED;
  else res += CALTASK_TD_VARIABLE;
  res += ": "+diltext+"<TD>";
  for (int i=0; i<numchunks; i++)
    if ((allocation[i]==CALTASK_TD_EXACT) || (allocation[i]==CALTASK_TD_FIXED) || (allocation[i]==CALTASK_TD_VARIABLE)) {
      res += "<b>"; res += allocation[i]; res += "</b>";
    } else res += allocation[i];
  return res;
}

String Calendar_Day::HTML_Day_Chunks_by_Task(String dstfile) {
  // dstfile is supplied in order to generate correct relative HREF references
  String res("<TABLE>\n");
  time_t t_notpnpcompared = 0; // e.g. regular daily infrastructure tasks, marked urgency < 0.0
  time_t t_priority = 0; // marked urgency > 0.0
  time_t t_nonpriority = 0; // marked urgency == 0.0
  PLL_LOOP_FORWARD(Calendar_Task,tasks.head(),1) {
    res += e->HTML_Chunks_by_Task(dstfile) + '\n';
    // The following calculates a nonpriority/(priority+nonpriority) ratio
    // as used in self-evaluation for effective AL planning. See DIL#20180110074459.1.
    if (e->de->Urgency()< (float) 0.0) t_notpnpcompared += e->de->Time_Required();
    else if (e->de->Urgency()> (float) 0.0) t_priority += e->de->Time_Required();
    else t_nonpriority += e->de->Time_Required();
    // *** VOUT << "TIME CMP: " << e->de->chars() << " urg: " << e->de->Urgency() << " TR: " << e->de->Time_Required() << '\n';
  }
  res += "</TABLE>\n";
  res += "AL Day Effectiveness: priority = " + String((double) t_priority / 3600.0,"%.1f") + " hrs, non-priority = " + String((double) t_nonpriority / 3600.0,"%.1f") + " hrs (other = " + String((double) t_notpnpcompared / 3600.0,"%.1f") + " hrs), nonpriority/(priority+nonpriority) = " + String((double) 100.0 * (double) t_nonpriority / ((double) t_nonpriority + (double) t_priority),"%.1f") + "%\n<P>\n";
  return res;
}
#endif

String AL_Day_Marker(String & dstfile, AL_Day * ald) {
// Generates a table row with data about ald for inclusion in AL files
  if (!ald) return String("");
  String daytxt = "<!-- @select_TL_DIL_refs: SKIP ROW@ --><B>Day " + time_stamp("%Y%m%d",ald->DayDate())
    + "</B> (suggested start " + time_stamp_GM("%H:%M",ald->DayStart()) + ", limits "
    + time_stamp_GM("%H:%M",ald->DayStartMin()) + '-' + time_stamp_GM("%H:%M",ald->DayMaxt());
  if (ald->DayType()==1) daytxt += ", worked " + time_stamp_GM("%H:%M",alworked);
  daytxt += "), "; daytxt += String(ald->Total()); daytxt += " task chunks";
  if (ald->Expanded()) daytxt += " (expanded)";
  if (ald->TD_Head()) {
    String relidfile(relurl(dstfile,idfile));
    daytxt += ", target dates: ";
    PLL_LOOP_FORWARD(AL_TD,ald->TD_Head(),1) {
#ifdef INCLUDE_PERIODIC_TASKS
      daytxt += HTML_put_href((relidfile+'#')+e->DE()->dep->chars(),"DIL#" + String(e->DE()->dep->chars()));
      daytxt += " (" + time_stamp("%H:%M",e->DE()->td) + ')';
#else
      daytxt += HTML_put_href((relidfile+'#')+e->DE()->chars(),"DIL#" + String(e->DE()->chars()));
      daytxt += " (" + time_stamp("%H:%M",e->DE()->Target_Date()) + ')';
#endif
      if (e->Next()) daytxt += ", ";
    }
  }
  return HTML_put_table_row(DEFAULTALDAYEMPHASIS,HTML_put_table_cell("",daytxt)) + '\n';
}

String AL_Passed_Target_Dates(String & dstfile, AL_TD * atd) {
// Generates a list of passed target date DIL entries
  if (atd) {
    String diltext;
    String passedstr(HTML_put_form_input("hidden","name=\"dil2al\" value=\"MEtgroup\"")+"\n<B>Passed Target Dates:</B>\n<UL>\n"), relidfile(relurl(dstfile,idfile));
    PLL_LOOP_FORWARD(AL_TD,atd,1) {
#ifdef INCLUDE_PERIODIC_TASKS
      if (e->DE()->dep->Entry_Text()) diltext = (*e->DE()->dep->Entry_Text());
#else
      if (e->DE()->Entry_Text()) diltext = (*e->DE()->Entry_Text());
#endif
      else diltext="";
      HTML_remove_tags(diltext);
      Elipsis_At(diltext,30);
#ifdef INCLUDE_PERIODIC_TASKS
      String deidstr(e->DE()->dep->chars());
#else
      String deidstr(e->DE()->chars());
#endif
      passedstr += "<LI>" + HTML_put_href((relidfile+'#')+deidstr,"DIL#"+ deidstr)
	+ "[<A HREF=\"file:///cgi-bin/dil2al?dil2al=MEI&DILID="+deidstr+"\">edit</A>] ("
#ifdef INCLUDE_PERIODIC_TASKS
	+ time_stamp("%Y%m%d%H%M",e->DE()->td) + ") "
#else
	+ time_stamp("%Y%m%d%H%M",e->DE()->Target_Date()) + ") "
#endif
	+ HTML_put_form_checkbox(String("TDCHKBX")+deidstr,"noupdate","",true)
	+ HTML_put_form_text(String("TD")+deidstr,12) + ' ' + diltext + '\n';
    }
    passedstr += "</UL>\n"+HTML_put_form_radio("TDupdate","group","Minimize fragmentation",true)
      + ' '+HTML_put_form_radio("TDupdate","direct","Direct")+"<BR>\n"
      + HTML_put_form_submit("Update")+' '+HTML_put_form_reset()+"\n<P>\n";
    return HTML_put_form(htmlformmethod,htmlforminterface,passedstr);
  } else return String("");
}

bool Active_List::generate_focused_AL(DIL_entry * superior, time_t algenwt) {
  // create the focused list of DIL entries according to dep
  // if superior==NULL then a complete AL of all DIL entries was computed
  // algenwt is marked at the top of the AL to indicate the
  // time allotted per full day prior to any expansions
  
  // determine number of task chunks to display
  long deplen = allength;
  if (alfocused<deplen) deplen = alfocused;
  if ((generatealmaxt==0) && (deplen>generatealtcs)) deplen = generatealtcs;
  
  // AL superior information
  String dstfile(basedir+RELLISTSDIR+filestr()+".html");
  String alstr(DEFAULTALFOCUSEDHEADER);
  alstr.gsub("@SUP@",get_supstr());
  alstr.gsub("@GENERAL-WORK-TIME@",time_stamp_GM("%H:%M",algenwt));
  alstr.gsub("@PASSED@",AL_Passed_Target_Dates(dstfile,td.head()));
  // *** perhaps catch MAXTIME_T and interpret it as "none"
  if (superior) alstr.gsub("@TD@",time_stamp("%Y%m%d",superior->Target_Date())); else alstr.gsub("@TD@","none");
  
  // first day as Calendar Day with task list
  if (al.head()) {
    Calendar_Day CD(al.head()->DayDate(),alfocusedCALdays);
    CD.gather_day_tasks(*al.head());
    alstr += CD.HTML_Day_Chunks_by_Task(dstfile);
  }
  
  // mark the start of the focused AL days and task chunks section
  alstr += "<!-- @Focused-AL Days+TCs START@ -->\n";
  
  // task chunk information
  String diltext, dilurl; DIL_entry * de;
  // *** Should this have a progress indicator?
  //     If so, then add here: int progresscnt = 0; progress->start(); progress->update(0,"Focused AL: ");
  //     and in the loop add: progress->update((progresscnt*100)/allength); progresscnt++;
  //     and below the loop add: progress->stop();
  PLL_LOOP_FORWARD(AL_Day,al.head(),(deplen>0)) {
    // *** Could catch the Calendar_Day::gather_day_tasks() here instead
    alstr += AL_Day_Marker(dstfile,e);
    for (AL_TC * atc = e->tc.head(); ((atc) && (deplen>0)); atc = atc->Next()) if ((de = atc->Get_DE())!=NULL) {
	if (de->Entry_Text()) diltext = (*de->Entry_Text());
	else {
	  WARN << "generate_focused_AL(): Missing Text Content for DIL entry #" << de->chars() << ". Continuing.\n";
	  diltext="";
	}
	HTML_remove_tags(diltext);
	Elipsis_At(diltext,alfocusedexcerptlength);
	if (de->Topics(0)) {
	  dilurl = de->Topics(0)->dil.file+'#'+de->chars();
	  dilurl = relurl(dstfile,dilurl);
	} else {
	  WARN << "generate_focused_AL(): Missing URL for DIL entry #" << de->chars() << ". Continuing.\n";
	  dilurl='#'+de->chars();
	}
#ifdef INCLUDE_EXACT_TARGET_DATES
	diltext.prepend("[<A HREF=\"file:///cgi-bin/dil2al?dil2al=MEI&DILID="+de->str()+"\">"+time_stamp("%H%M",atc->TCstart())+"</A>]\n");
#else
	diltext.prepend("[<A HREF=\"file:///cgi-bin/dil2al?dil2al=MEI&DILID="+de->str()+"\">edit</A>]\n");
#endif
	if ((e==al.head()) && (atc==e->tc.head())) alstr += HTML_put_table_row(DEFAULTALFOCUSEDEMPHASIS,HTML_put_table_cell("",'['+HTML_put_href(dilurl,de->chars())+"]\n"+diltext)) + '\n';
	else alstr += HTML_put_table_row("",HTML_put_table_cell("",'['+HTML_put_href(dilurl,de->chars())+"]\n"+diltext)) + '\n';
	deplen--;
      } else WARN << "generate_focused_AL(): Missing DIL entry in array at day " << time_stamp("%Y%m%d",e->DayDate()) << ".\nContinuing as is.\n";
  }
  String altail(DEFAULTALFOCUSEDTAIL);
  altail.gsub("@SUP@",get_supstr());
  altail.gsub("@ALFILE@",filestr());
  if (alshowcumulativereq) altail.gsub("@CUMULATIVE@","<LI>"+HTML_put_href(filestr()+".ctr.html","Active List - Cumulative Time Required")+'\n');
  else altail.gsub("@CUMULATIVE@","");
  altail.gsub("@ALDATE@",time_stamp("%c"));
  alstr += altail;
  
  return write_file_from_String(dstfile,alstr,"AL Focused");
}

bool Active_List::generate_wide_AL(DIL_entry * superior) {
// create the wide list of DIL entries according to dep
// if superior==NULL then a complete AL of all DIL entries was computed
  
  // determine number of task chunks to display
  long deplen = allength;
  if (alwide<deplen) deplen = alwide;
  if ((generatealmaxt==0) && (deplen>generatealtcs)) deplen = generatealtcs;
  
  // AL superior information
  String dstfile(basedir+RELLISTSDIR+filestr()+".wide.html");
  String alstr(DEFAULTALWIDEHEADER);
  alstr.gsub("@SUP@",get_supstr());
  
  // task chunk information
  String diltext, dilurl; DIL_entry * de;
  PLL_LOOP_FORWARD(AL_Day,al.head(),(deplen>0)) {
    alstr += "<BR>" + AL_Day_Marker(dstfile,e);
    for (AL_TC * atc = e->tc.head(); ((atc) && (deplen>0)); atc = atc->Next()) if ((de = atc->Get_DE())!=NULL) {
	if (de->Entry_Text()) diltext = (*de->Entry_Text());
	else {
	  WARN << "Active_List::generate_wide_AL(): Missing Text Content for DIL entry #" << de->chars() << ". Continuing.\n";
	  diltext="";
	}
	HTML_remove_tags(diltext);
	Elipsis_At(diltext,50);
	dilurl = relurl(dstfile,idfile)+'#'+de->chars();
#ifdef INCLUDE_EXACT_TARGET_DATES
	alstr += HTML_put_list_item(HTML_put_href(dilurl,de->chars())+" ["+time_stamp("%H%M",atc->TCstart())+"] "+diltext);
#else
	alstr += HTML_put_list_item(HTML_put_href(dilurl,de->chars())+' '+diltext);
#endif
	deplen--;
      } else WARN << "Active_List::generate_wide_AL(): Missing DIL entry in array at day " << time_stamp("%Y%m%d",e->DayDate()) << ".\nContinuing as is.\n";
  }
  String altail(DEFAULTALWIDETAIL);
  altail.gsub("@SUP@",get_supstr());
  altail.gsub("@ALFILE@",filestr());
  if (alshowcumulativereq) altail.gsub("@CUMULATIVE@","<LI>"+HTML_put_href(filestr()+".ctr.html","Active List - Cumulative Time Required")+'\n');
  else altail.gsub("@CUMULATIVE@","");
  altail.gsub("@ALDATE@",time_stamp("%c"));
  alstr += altail;
  
  return write_file_from_String(dstfile,alstr,"AL Wide");
}

bool update_main_ALs(DIL_entry * superior) {
  // add or modify a link to the AL referred to by superior on
  // the main page (NULL = AL created with all DIL entries)
  
  String mainstr, tag, text, supstr, supfilestr;
  if (superior) supstr = superior->chars(); else supstr = "all";
  supfilestr = relurl(listfile,basedir+RELLISTSDIR+("active-list."+supstr+".html"));
  
  // get main lists file
  if (!read_file_into_String(listfile,mainstr)) return false;
  int mainloc = 0;
  
  // locate list of ALs
  do if ((mainloc=HTML_get_name(mainstr,mainloc,tag,text))<0) {
      ERROR << "update_main_ALs(): Unable to find HTML anchor ``AL'' in " << listfile << ".\n";
      return false;
    } while (tag!="AL");
  
  // get list of ALs
  int liststart;
  if ((mainloc=HTML_get_list(mainstr,mainloc,tag,text,&liststart))<0) {
    ERROR << "update_main_ALs(): Missing list of ALs in " << listfile << ".\n";
    return false;
  }
  
  // find list item with link to AL corresponding with superior
  int listloc = 0; String itemtext,hreftext;
  do if ((listloc=HTML_get_list_item(text,listloc,tag,itemtext))>=0) 
       if (HTML_get_href(itemtext,0,tag,hreftext)<0) WARN << "update_main_ALs(): Missing HREF to AL in " << listfile << "\nContinuing.\n";
  while ((listloc>=0) && (tag!=supfilestr));
  if (listloc>=0) return true; // link already exists
  
  // if new, add link to AL corresponding with superior
  String newmainstr(mainstr.before(liststart));
  if (superior) {
    if (superior->Entry_Text()) {
      tag = *(superior->Entry_Text());
      Elipsis_At(tag,100);
    } else {
      WARN << "update_main_ALs(): No text content available for description of AL.\nContinuing as is.\n";
      tag = "(No description available.)";
    }
  } else tag = "A list of tasks created with all DIL entries.";
  // *** perhaps don't use relaxedhtml, despite possible unexpected
  //     behaviour by older function that read the main lists page
  text += HTML_put_list_item(HTML_put_href(supfilestr,"Active List: "+capitalize(supstr))+"<BR>\n"+tag+'\n',true)+"<P>\n";
  newmainstr += HTML_put_list('U',text) + mainstr.from(mainloc);
  
  return write_file_from_String(listfile,newmainstr,"Main Lists");
}

// [Note 20150404: I think most or all of the following list of to-dos have been done.]
//...add target dates to important DIL entries
//...make more DIL entries for sub-tasks/dependencies
//...perhaps don't put target dates at midnight
//...note: can remove start time parameter
//...count errors or warnings encountered
//...clean up with HTML functions and speed up bottlenecks
//...move gesn inline functions to headers (See C++ FAQ Lite [9.4])
//...perhaps make it possible to pass NULL to HTML functions
//   in order not to retrieve some of the data
//...perhaps use the BigString readline() function for queries
//...maybe turn algenregular, algenall and allgenhpd into seconds (time_t)

// *** make this able to generate an AL for a specific DIL and
// its dependencies (that DIL can be an AL Project)
#define AL_PREPARATION_DELAY 60
inline float linear_distribution(float i, float i_limit) {
// linearly distribute TC values (see TL#200006291007.2)
// The value of i should not exceed that of i_limit, since the resulting range should be between 1.0 and 0.1.
  if (i>=i_limit) return 0.0;
  return 1.0 - (0.9*i/i_limit);
}

long * DIL_Required_Task_Chunks(DIL_entry ** dep, int deplen) {
  // returns an array with the number of task chunks
  // required to complete the DIL entries in the array
  // dep with length deplen
  
  long * req = new long[deplen];
  float completion;
  long c_size = ((long) timechunksize)*60;
  for (int i=0; i<deplen; i++) {
    req[i] = dep[i]->Time_Required();
    completion = dep[i]->Completion_State();
    if (completion<0.0) completion = 1.0; // negative completion values have special meaning but do imply that the task is no longer scheduled to be completed
    if (completion>1.0) {
      // *** THIS ONE IS TRICKY!
      //     I really want to be able to access this through a URL or something
      //     similar to immediately fix it.
      WARN << "dil2al: Completion ratio exceeds 1.0 for DIL#";
      if (calledbyforminput) WARN << "<A HREF=\"" << idfile << '#' << dep[i]->chars() << "\">";
      WARN << dep[i]->chars();
      if (calledbyforminput) WARN << "</A>";
      WARN << " in DIL_Required_Task_Chunks(), continuing\n";
      req[i] = 0;
      continue;
    }
    completion = ceil(((double) req[i])*(1.0-completion)); // determine remaining time required
    req[i] = (long) completion;
    if (req[i] % c_size) req[i] = (req[i]/c_size) + 1;
    else req[i] /= c_size;
  }
  return req;
}

void Get_Worked_Estimate() {
  // update alworked (in seconds)
  
  if (alworked<0) { // (alworked>=0) if given on command line
    time_t tod;
    switch (alcurdayworkedstrategy) {
    case ACW_TIME: // estimate according to current time
      tod = time_stamp_time_of_day(curtime);
      tod -= alworkdaystart;
      if (tod<0) alworked = 0; else alworked = tod;
      break;
    case ACW_TL: // *** obtain an estimate from task log entries
      ERROR << "Get_Worked_Estimate(): ACW_TL option not yet implemented. Setting to 0.\n";
      alworked = 0;
      break;
    case ACW_ASK: // *** ask for estimate
      ERROR << "Get_Worked_Estimate(): ACW_ASK option not yet implemented. Setting to 0.\n";
      alworked = 0;
      break;
    default: alworked = 0;
    }
  }
}

AL_TC::AL_TC(time_t _tcstart, long itc, long i_limit): de(NULL), tcstart(_tcstart) {
// create TC and set value
  
  val = linear_distribution((float) itc, (float) i_limit);
  //if (val<0.0) VOUT << "AL_TC:AL_TC val = " << val << " itc=" << itc << " i_limit=" << i_limit << '\n'; VOUT.flush();
}

AL_TC * AL_TC::next_avail_el() {
// returns the next available task chunk
  
  PLL_LOOP_FORWARD(AL_TC,this,1) if ((!e->Get_DE()) && (e->get_val()>=0.0)) return e;
  return NULL;
}

AL_TC * AL_TC::avail_el(unsigned int n) {
  // returns the nth available task chunk
  
  //  VOUT << "TC val: ";
  //  PLL_LOOP_FORWARD(AL_TC,this,1) VOUT << e->get_val() << ',';
  //  VOUT << '\n'; VOUT.flush();
  PLL_LOOP_FORWARD(AL_TC,this,1) if ((!e->Get_DE()) && (e->get_val()>=0.0)) {
    if (n==0) return e;
    n--;
  }
  return NULL;
}

long AL_TC::available_before(unsigned int n) {
// returns the number of task chunks available before the
// nth task chunk (i.e. does not include the nth task chunk)
// This is used by AL_Day::available_before().
  
  long avail = 0;
  PLL_LOOP_FORWARD(AL_TC,this,1) {
    if (n==0) return avail;
    if (!e->Get_DE()) avail++;
    n--;
  }
  return avail;
}

float AL_TC::allocate_with_value(float uval, DIL_entry * d) {
  // allocate a DIL entry to a task chunk according to
  // a random value
  // returns the value of the allocated task chunk
  // creates TCs if necessary
  //
  // NOTE A: This is only for distributed allocation, i.e. only within regular work time task chunks (identified by val >= 0.0)
  // NOTE B: This does not use d->Target_Date() and is therefore safe to use when d is set by DIL_Virtual_Matrix.dep.
  
  const char _err_msg[] = "AL_TC::allocate_with_value(): Available TC values do not add up to day value. Ccontinuing.\n";
  if ((de) || (val<0.0)) {
    if (Next()) return Next()->allocate_with_value(uval,d);
    WARN << _err_msg;
    return 0.0;
  }
  if (val<uval) {
    if (Next()) return Next()->allocate_with_value(uval-val,d);
    WARN << _err_msg;
    return 0.0;
  }
  de = d;
  uval = val;
  val = 0.0;
  return uval;
}

AL_TC * AL_TC::allocate(DIL_entry * d) {
  // allocates d to the first available task chunk
  // returns the allocated task chunk
  //
  // NOTE A: This is only for distributed allocation, i.e. only within regular work time task chunks (identified by val >= 0.0)
  // NOTE B: This does not use de->Target_Date() and is therefore safe when de is set by DIL_Virtual_Matrix.dep.
  
  if ((de) || (val<0.0)) {
    if (Next()) return Next()->allocate(d);
    return NULL;
  }
  de = d;
  val = 0.0;
  return this;
}

#ifdef INCLUDE_EXACT_TARGET_DATES
bool AL_TC::allocate_if_available(DIL_entry * d) {
  // allocates d to this specific task chunk if it is available
  
  if (de) return false;
  de = d;
  if (val>=0.0) val = 0.0;
  return true;
}
#endif

#ifdef INCLUDE_DAY_SPECIFIC_WORK_TIMES
AL_Day::AL_Day(time_t dd, time_t tcsec, time_t curt, time_t dmaxt): daytype(0), dayofweek(-1), avail(0), tot(0), daydate(dd), daystartmin(0), daymaxt(dmaxt), tcseconds(tcsec), expanded(false), cache_i_start(0), cache_i_limit(0), val(0.0) {
  // NOTE: This version does not detect "last day", but that day type is not
  // tested or used anywhere in dil2al anyway.
  
  time_t curtmindd = curt - dd;
  if ((curtmindd>=0) && (curtmindd<SECONDSPERDAY)) { // current (first) day
    daytype = 1;
    daystartmin = curtmindd;
  }
  if (alworkdaystart>=daystartmin) daystart = alworkdaystart;
  else daystart = daystartmin;
}
#else
AL_Day::AL_Day(time_t dd, int typ, time_t tcsec, time_t dsmin, time_t dmaxt): daytype(typ), avail(0), tot(0), daydate(dd), daystartmin(dsmin), daymaxt(dmaxt), tcseconds(tcsec), expanded(false), cache_i_start(0), cache_i_limit(0), val(0.0) {
  if (alworkdaystart>=daystartmin) daystart = alworkdaystart;
  else daystart = daystartmin;
}
#endif

#ifdef INCLUDE_DAY_SPECIFIC_WORK_TIMES
long AL_Day::set_avail(time_t av) {
  // The day uses daystart and the applicable wdworkendprefs[] value,
  // as well as the maximum indicated in av (seconds) to determine
  // how much work time is available. That amount is divided into
  // task chunks and stored in "avail" and "tot".
  // The smaller of wdworkendprefs[] and daymaxt is applied as a
  // limit for the day, prior to further limitation by av.
  // If av<0 then no task chunks are made available.
  // returns avail
  
  if (Prev()) { // Initialize dayofweek
    dayofweek = Prev()->DayofWeek() + 1;
    if (dayofweek>6) dayofweek = 0;
  } else dayofweek = atoi((const char *) time_stamp("%w",daydate));
  time_t dayworkend = wdworkendprefs[dayofweek];
  if (dayworkend<0) dayworkend = daymaxt;
  time_t secondsavailable = dayworkend - daystart; // *** or should this be daystartmin?
  if (secondsavailable<0) secondsavailable = 0;
  else if (secondsavailable>av) secondsavailable = av; // limited to the maximum indicated by av
  if (secondsavailable>0) avail = secondsavailable/tcseconds; // floor
  else avail = 0;
  tot = avail; // initialized with all TCs available
  return avail;
}
#else
long AL_Day::set_avail(time_t av) {
// attempt to make av seconds available through task chunks
// returns the number of task chunks made available
// the number of task chunks is limited by (daymaxt-daystartmin)
// if av<0 no task chunks will be made available
  
  time_t dtmax = daymaxt - daystartmin;
#ifdef DIAGNOSTIC_OUTPUT
  INFO << "dil2al: (DIAGNOSTICS) av = " << av << ", daymaxt = " << daymaxt << ", daystartmin = " << daystartmin << ", dtmax = " << dtmax << '\n';
#endif
  if (av>0) {
    if (av>dtmax) av = dtmax;
    avail = av/tcseconds; // floor, can't exceed limit
  } else avail = 0;
  tot = avail; // initialized with all TCs available
#ifdef DIAGNOSTIC_OUTPUT
  INFO << "dil2al: (DIAGNOSTICS) av = " << av << ", avail = " << avail << ", tot = " << tot << '\n';
#endif
  return avail;
}
#endif

long AL_Day::available_before(time_t td) {
  // returns the number of task chunks available for allocation
  // prior to a target date td
  // avail, daystart, tcseconds and Next()->DayDate() must contain valid values
  // *** might be sped up by putting a for-loop into the
  //     corresponding Active_List function

  if (Next()) if (td>=Next()->DayDate()) return avail + Next()->available_before(td); // target date is within a following day
  time_t tdiff = td - (daydate+daystart);
  if (tdiff<=0) return 0; // target date is earlier than or at work start offset
#ifdef DIAGNOSTIC_OUTPUT
  INFO << "@tdiff=" << tdiff << ", tcseconds=" << tcseconds << ", tdiff/tcseconds=" << tdiff/tcseconds << '@';
#endif
  long TCs_before = tdiff / tcseconds; // can't exceed target date
  if (TCs_before>tot) TCs_before=tot; // can't exceed work range (see TL#201001091014)
  if (TC_Head()) return TC_Head()->available_before(TCs_before); // count available TCs before target date
  return TCs_before;
}

int AL_Day::days_to(time_t td) {
  // returns the number of days to the target date, including
  // the current day and the day containing the target date
  
  if (Next()) if (td>Next()->DayDate()) return 1 + Next()->days_to(td);
  return 1;
}

void AL_Day::add_TCs(long tcs, TC_add_method m, time_t addborder) {
  // safely add task chunks to a day
  // links new task chunks if tchead!=NULL, using TAIL_REPLACE,
  // RANDOM or BEFORE_HEAD methods, TAIL_REPLACE and RANDOM
  // methods use the addborder parameter
  // (see <A HREF="al-day-expand.fig">al-day-expand.fig</A>)
  // uses tchead.length() to insure that tot will
  // equal tchead.length() if tchead!=NULL
  // [***INCOMPLETE] I need to keep this from moving exact target date task chunks.
  
#ifdef INCLUDE_EXACT_TARGET_DATES
  time_t newtcstart;
#endif
  expanded = true;
  avail += tcs;
  tot += tcs;
  if (tc.head()) {
    long linked = tc.length(), tci;
    tcs = tot - linked; // recalculated to insure at least tot TCs
    if (tcs<0) tcs = 0;
    switch (m) {
    case TAIL_REPLACE:
      // Place empty TCs at tail and move de pointers at chosen
      // random location to those TCs if de->Target_Date()>addborder
#ifdef INCLUDE_EXACT_TARGET_DATES
      newtcstart = tc.tail()->TCstart() + tcseconds;
#endif
      for (; tcs>0; tcs--) {
#ifdef INCLUDE_EXACT_TARGET_DATES
	AL_TC * altc = new AL_TC(newtcstart);
	newtcstart += tcseconds;
#else
	AL_TC * altc = new AL_TC();
#endif
	tc.link_before(altc); linked++;
	tci = (unsigned long) rint(((float) linked)*(((float) rand()) / ((float) RAND_MAX)));
	if (tc[tci]) if (tc[tci]->get_val()>=0.0) {
	  DIL_entry * de = tc[tci]->Get_DE();
	  if (de) if (de->Target_Date()>addborder) altc->Set_DE(tc[tci]->Set_DE(NULL));
	}
      }
      break;
    case RANDOM:
      // Place empty TCs at head and move de pointers at chosen
      // random location to those TCs
      if (linked>addborder) linked = (long) addborder; // number of TCs prior to tdiff
#ifdef INCLUDE_EXACT_TARGET_DATES
      newtcstart = tc.head()->TCstart() - tcseconds;
#endif
      for (; tcs>0; tcs--) {
#ifdef INCLUDE_EXACT_TARGET_DATES
	AL_TC * altc = new AL_TC(newtcstart);
	newtcstart -= tcseconds;
#else
	AL_TC * altc = new AL_TC();
#endif
	tc.link_after(altc); linked++;
	tci = (unsigned long) rint(((float) linked)*(((float) rand()) / ((float) RAND_MAX)));
	if (tc[tci]) if (tc[tci]->get_val()>=0.0) {
	  DIL_entry * de = tc[tci]->Get_DE();
	  if (de) altc->Set_DE(tc[tci]->Set_DE(NULL));
	}
      }
      break;
    case BEFORE_HEAD:
      // link tcs empty TCs before head
#ifdef INCLUDE_EXACT_TARGET_DATES
      newtcstart = tc.head()->TCstart() - tcseconds;
      for (; tcs>0; tcs--) {
	tc.link_after(new AL_TC(newtcstart));
	newtcstart -= tcseconds;
      }
#else
      for (; tcs>0; tcs--) tc.link_after(new AL_TC());
#endif
      break;
    }
  }
}

// <A NAME="AL_Day::expand"></A>
long AL_Day::expand(long expandn, time_t td) {
  // expand work time by adding task chunks prior to
  // the target date td
  // returns the number of task chunks added (-1==error)
  // (see <A HREF="al-day-expand.fig">al-day-expand.fig</A>)
  
  if (expandn<=0) return 0;
  const time_t tdiff(td - daydate); // target date limit (offset in current day)
  
  // case: TD1 in al-day-expand.fig
  if (tdiff<(daystartmin+tcseconds)) return 0;
#ifdef DETAILED_AL_DIAGNOSTIC_OUTPUT
  INFO << "E>" << time_stamp("%Y%m%d",DayDate()) << ": (" << Total() << ',' << Avail() << "), expand request = " << expandn << '\n';
#endif
  long addabove = 0;
  time_t dayend = daystart+(tot*tcseconds); // if tot==0 dayend is current time
  
  // cases: TD4, TD5 and TD6 in al-day-expand.fig
  if (tdiff>=(dayend+tcseconds)) { // TD beyond current end of day tasks
    time_t dayendmax = tdiff;
    if (tdiff>daymaxt) dayendmax = daymaxt; // day upper limit
    addabove = (dayendmax-dayend)/tcseconds; // count TCs that can be added prior to target date and daymaxt
    if (addabove>0) {
      if (addabove>expandn) addabove = expandn; // add expandn or fewer TCs above current dayend
#ifdef DETAILED_AL_DIAGNOSTIC_OUTPUT
      INFO << "Ea>" << time_stamp("%Y%m%d",DayDate()) << ": (" << Total() << ',' << Avail() << ')';
#endif
      add_TCs(addabove,TAIL_REPLACE,daydate+dayendmax); // add empty TCs by replacing allocated TCs with target dates greater than dayendmax and moving those to the tail and update tot and avail
      expandn -= addabove;
#ifdef DETAILED_AL_DIAGNOSTIC_OUTPUT
      INFO << "->(" << Total() << ',' << Avail() << ")\n";
#endif
      if (expandn<0) WARN << "AL_Day::expand(): Expansion exceeded requirement.\n";
      if (expandn<=0) return addabove;
    }
  }
  
  // case: TD3 in al-day-expand.fig
  time_t tlowerexpand = daystart;
  TC_add_method m = RANDOM;
  long totbeforetdiff = (tdiff-daystart)/tcseconds;
  
  // case: TD2 in al-day-expand.fig
  if (totbeforetdiff<=0) {
    tlowerexpand = tdiff;
    m = BEFORE_HEAD;
  }
  if (totbeforetdiff>tot) totbeforetdiff = tot;
  time_t newdaystart = tlowerexpand - (expandn*tcseconds); // attempt to add expandn TCs prior to daystart
  if (newdaystart<daystartmin) newdaystart = daystartmin; // daystart limit
  long addbelow = (tlowerexpand-newdaystart)/tcseconds; // count TCs to be added prior to daystart
  if (addbelow>0) { // add expandn or fewer TCs below min(daystart,TDtime)
    daystart = newdaystart;
#ifdef DETAILED_AL_DIAGNOSTIC_OUTPUT
    INFO << "Eab>" << time_stamp("%Y%m%d",DayDate()) << ": (" << Total() << ',' << Avail() << ')';
#endif
    add_TCs(addbelow,m,totbeforetdiff); // add empty TCs before target date and update tot and avail
#ifdef DETAILED_AL_DIAGNOSTIC_OUTPUT
    INFO << "->(" << Total() << ',' << Avail() << ")\n";
#endif
    return addabove+addbelow;
  }
#ifdef DETAILED_AL_DIAGNOSTIC_OUTPUT
  INFO << "Ex>" << time_stamp("%Y%m%d",DayDate()) << ": (" << Total() << ',' << Avail() << ")\n";
#endif
  return addabove;
}

float AL_Day::set_TC_values(long & i_start, long i_limit) {
  // set TC values within this day
  // returns sum of TC values
  
  float v;
  cache_i_start = i_start;
  cache_i_limit = i_limit;
  val = 0.0;
  for (long i=0; i<avail; i++) {
    v = linear_distribution((float) (i+i_start), (float) i_limit); // taken care of in function: if ((i+i_start)>=i_limit) v = 0.0;
    if (tc.head()) {
      AL_TC * atc = tc.head()->avail_el(i);
      if (atc) atc->set_val(v);
      else WARN << "AL_Day::set_TC_values(): Available TC #" << i << " on day " << time_stamp("%Y%m%d",daydate) << " not found. Continuing.\n";
    }
    // assignment of values to remaining TCs occurs when
    // those TCs are created as a search enters them
    val += v; // store summed values of TCs for that day
  }
  i_start += avail;
  return val;
}

void AL_Day::create_TCs(bool setvalues) {
  // creates TCs and sets values according to a random
  // distribution if setvalues==true, in which case it
  // requires valid cache_i_start and cache_i_limit values
  
  if (tc.head()) return;
  AL_TC * atc;
#ifdef INCLUDE_EXACT_TARGET_DATES
  time_t newtcstart = daydate + daystart;
#endif
  for (long i = 0; i<tot; i++) {
#ifdef INCLUDE_EXACT_TARGET_DATES
    // This may only be allowed to create postive values for TCs that are within the range prior to a
    // task's target date. Beyond that, the values should be zero.
    // I.e. tot may be greater than cache_i_limit - cache_i_start.
    if (setvalues) atc = new AL_TC(newtcstart,i+cache_i_start,cache_i_limit);
    else atc = new AL_TC(newtcstart);
    newtcstart += tcseconds;
#else
    if (setvalues) atc = new AL_TC(i+cache_i_start,cache_i_limit);
    else atc = new AL_TC();
#endif
    tc.link_before(atc);
  }
}

float AL_Day::allocate_with_value(float uval, DIL_entry * de) {
  // allocate a DIL entry to a task chunk according to
  // a random value
  // returns the value of the allocated task chunk
  // creates TCs if necessary
  //
  // NOTE: This does not use de->Target_Date() and is therefore safe to use when de is set by DIL_Virtual_Matrix.dep.
  
  if (val<uval) {
    if (Next()) return Next()->allocate_with_value(uval-val,de);
    return 0.0; // (error) no more days, value was beyond sum
  }
#ifdef DETAILED_AL_DIAGNOSTIC_OUTPUT
  INFO << "V>" << time_stamp("%Y%m%d",DayDate()) << ": (" << Total() << ',' << Avail() << ')';
#endif
  create_TCs(true); // create TCs if necessary
  float v = tc.head()->allocate_with_value(uval,de);
  if (v>0.0) {
    val -= v;
    avail--;
  }
#ifdef DETAILED_AL_DIAGNOSTIC_OUTPUT
  INFO << "->(" << Total() << ',' << Avail() << ")\n";
#endif
  return v;
}

#ifdef INCLUDE_EXACT_TARGET_DATES
AL_TC * AL_Day::create_TCs_to_date(time_t tctdate, long req) {
  // Insures that TCs exist up to a specific target date.
  // Should only be called for the day within which tctdate appears.
  // returns a pointer to the AL_TC object representing the first
  // task time chunk preceding tctdate
  // Generally, call create_TCs() before this function!
  
  if (!tc.tail()) { // there are no regular TCs available during this day
    time_t newtcstart = tctdate-tcseconds;
    if (newtcstart<daystartmin) newtcstart = daystartmin;
    for (; ((req>0) && (newtcstart>=daystartmin)); req--) { // add up to req TCs that precede tctdate
	AL_TC * newtc = new AL_TC(newtcstart);
	newtc->set_val(-1.0); // off-limits to regular task chunk distribution
	tc.link_after(newtc); // newtc becomes the new head of the list
        newtcstart -= tcseconds;
    }
    return tc.tail();
  }
  if ((tc.tail()->TCstart()+tcseconds)<tctdate) { // tctdate is later than the end of the current list
    time_t newtcstart = tc.tail()->TCstart()+tcseconds;
    while (newtcstart<tctdate) { // add TCs up to tctdate and form a link
      AL_TC * newtc = new AL_TC(newtcstart);
      newtc->set_val(-1.0); // off-limits to regular task chunk distribution
      tc.link_before(newtc); // newtc becomes the new tail of the list
      newtcstart += tcseconds;
    }
    return tc.tail();
  } else if (tc.head()->TCstart()>=tctdate) { // tctdate is earlier than the beginning of the current list
    time_t newtcstart = tc.head()->TCstart();
    while (newtcstart>tctdate) { // add TCs beyond tctdate that form a link
      newtcstart -= tcseconds;
      AL_TC * newtc = new AL_TC(newtcstart);
      newtc->set_val(-1.0); // off-limits to regular task chunk distribution
      tc.link_after(newtc); // newtc becomes the new head of the list
    }
    AL_TC * res = tc.head();
    for (; ((req>0) && (newtcstart>=daydate)); req--) { // add up to req TCs that precede tctdate
      newtcstart -= tcseconds;
      AL_TC * newtc = new AL_TC(newtcstart);
      newtc->set_val(-1.0); // off-limits to regular task chunk distribution
      tc.link_after(newtc); // newtc becomes the new head of the list
    }
    return res;
  }
  AL_TC * res = tc.tail();
  while ((res->TCstart()>=tctdate) && (res->Prev())) res = res->Prev();
  return res;
}

long AL_Day::allocate_with_date(DIL_entry * de, time_t tctdate, long req, long & regularworktimechunks) {
  // allocate req task chunks for DIL entry de exactly prior to its target date
  // task chunks that coincide with available task chunks during regular work
  // time are reported in regularworktimechunks
  // Task chunks allocated as consecutive as much as possible. If some task
  // chunks within the consecutive block are not available, those lead to
  // task chunks that could not be allocated.
  // returns the number of task chunks that could not be allocated in a set of
  // consecutive task chunks exactly preceding tctdate
  // (The calling function insures that this day is the day that includes tctdate.)
  // tctdate is used instead of de->Target_Date(), so that recursive calls can be
  // used to allocate additional task chunks on preceding days.
  //
  // NOTE: This function and all the functions it calls use only tctdate, not de->Target_Date().
  // It is therefore safe to use with a de set by DIL_Virtual_Matrix.dep.
  
  create_TCs(false); // create TCs if necessary
  AL_TC * date_tc = create_TCs_to_date(tctdate,req); // insure that date specific TCs exist
  long nonconsecutive = 0;
  for (; ((req>0) && (date_tc!=NULL)); req--) {
    if (date_tc->allocate_if_available(de)) {
      if (date_tc->get_val()>=0.0) {
	regularworktimechunks++;
	avail--;
      }
    } else nonconsecutive++;
    date_tc = date_tc->Prev();
  }
  // if this day did not contain enough consecutive task chunks preceding tctdate then try the preceding day
  if (req>0) {
    if (Prev()) nonconsecutive += Prev()->allocate_with_date(de,daydate,req,regularworktimechunks);
    else nonconsecutive += req;
  }
  return nonconsecutive;
}
#endif

AL_TC * AL_Day::Get_Avail_TC(unsigned long n) {
  // returns the nth available task chunk in the AL
  // creates task chunks if necessary where tchead==NULL
  // note that n loses some of its value capacity by conversion to
  // signed in the comparison below
  
  if (avail<=(long) n) {
    if (Next()) return Next()->Get_Avail_TC(n-avail);
    return NULL;
  }
  create_TCs();
  return tc.head()->avail_el(n);
}

AL_TC * AL_Day::allocate(DIL_entry * de) {
  // allocates de to the first available task
  // chunk in this day or following days
  // returns the allocated task chunk
  // creates task chunks if necessary where tchead==NULL
  //
  // NOTE: This does not use de->Target_Date() and is therefore safe when de is set by DIL_Virtual_Matrix.dep.
  
  if (avail<=0) {
    if (Next()) return Next()->allocate(de);
    return NULL;
  }
#ifdef DETAILED_AL_DIAGNOSTIC_OUTPUT
  INFO << "A>" << time_stamp("%Y%m%d",DayDate()) << ": (" << Total() << ',' << Avail() << ')';
#endif
  create_TCs();
  AL_TC * atc = tc.head()->allocate(de);
  if (atc) avail--;
#ifdef DETAILED_AL_DIAGNOSTIC_OUTPUT
  INFO << "->(" << Total() << ',' << Avail() << ")\n";
#endif
  return atc;
}

//#define ALGEN_DEBUG
#ifdef ALGEN_DEBUG
bool algen_debug_this = false;
#endif

// The following cache is used to detect if a target date is in an undefined computer time.
//
// NOTE: The test algen_source_day ==Prev() or ==Next() below checks to see if
// we just came from the AL_Day() that we might otherwise next inspect to find where
// the target date belongs. This is necessary, because some computer times in terms of
// the time_t second counts do not exist as calendar dates and times, due to daylight
// savings time shifts. The AL_Day() daydate time_t values take those changes into
// account due to the way utilties.cc:time_add_day() makes use of localtime() and
// mktime() to safely generate the computer times for successive days.
AL_Day * algen_source_day = NULL;

#ifdef INCLUDE_PERIODIC_TASKS
AL_Day * AL_Day::Add_Target_Date(DIL_Virtual_Matrix & dvm) {
  // Adds a target date reference to the day that contains it
  
#ifdef ALGEN_DEBUG
  if (algen_debug_this) { VOUT << '\n'; VOUT.flush(); }
#endif
  if (!dvm.dep) return NULL;
#ifdef ALGEN_DEBUG
  if (algen_debug_this) { VOUT << "  TD=" << time_stamp("%Y%m%d%H%M",dvm.td) << " DD=" << time_stamp("%Y%m%d%H%M",daydate); VOUT.flush(); }
#endif
  time_t tdiff = dvm.td - daydate;
  if (tdiff<0) {
    if (Prev()) {
      if (algen_source_day==Prev()) {
	WARN << "AL_Day::Add_Target_Date()[1]: Target date " << (long) dvm.td << " (" << time_stamp("%Y%m%d%H%M",dvm.td) << ") of\nvirtual task copy of DIL#" << dvm.dep->chars() << " is an undefined computer time.\nEncountered while on AL_Day::daydate = " << daydate << " (" << time_stamp("%Y%m%d%H%M",daydate) << ")\nfrom algen_source_day->daydate = " << algen_source_day->daydate << " (" << time_stamp("%Y%m%d%H%M",algen_source_day->daydate) << ").\nA daylight savings change may be the cause. Skipping that virtual copy.\n";
	algen_source_day=NULL;
	return NULL;
      }
      algen_source_day = this;
      return Prev()->Add_Target_Date(dvm);
    }
    return NULL;
  }
  if (tdiff>=SECONDSPERDAY) {
    if (Next()) {
      if (algen_source_day==Next()) {
	WARN << "AL_Day::Add_Target_Date()[2]: Target date " << (long) dvm.td << " (" << time_stamp("%Y%m%d%H%M",dvm.td) << ") of\nvirtual task copy of DIL#" << dvm.dep->chars() << " is an undefined computer time.\nEncountered while on AL_Day::daydate = " << daydate << " (" << time_stamp("%Y%m%d%H%M",daydate) << ")\nfrom algen_source_day->daydate = " << algen_source_day->daydate << " (" << time_stamp("%Y%m%d%H%M",daydate) << ").\nA daylight savings change may be the cause. Skipping that virtual copy.\n";
	algen_source_day=NULL;
	return NULL;
      }
      algen_source_day = this;
      return Next()->Add_Target_Date(dvm);
    }
    return NULL;
  }
  algen_source_day = NULL; // clear cache
  AL_TD * atd = new AL_TD(dvm);
  td.link_before(atd);
#ifdef ALGEN_DEBUG
  if (algen_debug_this) { VOUT << " linked to ALDAY"; VOUT.flush(); }
#endif
  return this;
}
#else
AL_Day * AL_Day::Add_Target_Date(DIL_entry * de) {
  // Adds a target date reference to the day that contains it
  
  if (!de) return NULL;
  time_t tdiff = de->Target_Date() - daydate;
  if (tdiff<0) {
    if (Prev()) return Prev()->Add_Target_Date(de);
    return NULL;
  }
  if (tdiff>=SECONDSPERDAY) {
    if (Next()) return Next()->Add_Target_Date(de);
    return NULL;
  }
  AL_TD * atd = new AL_TD(*de);
  td.link_before(atd);
  return this;
}
#endif

long AL_Day::remove_unused_TCs() {
  // clean up by removing unused TCs
  // returns remaining number of TCs in AL
  
  if (!tc.head()) { // no TCs created in this day
    tot = 0;
    avail = 0;
    if (Prev()) return Prev()->remove_unused_TCs();
    return 0;
  }
  AL_TC * rtc, * atc = tc.head();
  tot = 0; // insure correct count
  while (atc) { // remove unused TCs in this day
    rtc = atc;
    atc = atc->Next();
    if (!rtc->Get_DE()) rtc->remove();
    else tot++; // faster than calling tchead->length()
  }
  avail = 0;
  if (Prev()) return tot + Prev()->remove_unused_TCs();
  return tot;
}

#ifdef INCLUDE_PERIODIC_TASKS
long AL_Day::remove_unused_and_periodic_only_TCs() {
  // clean up by removing unused TCs as well as AL_Days
  // with only periodic tasks, except the very first day.
  // Starts at the tail and assumes that no nonperiodic
  // tasks were found in later days.
  
  if (!tc.head()) { // no TCs created in this day
    tot = 0;
    avail = 0;
    if (Prev()) return Prev()->remove_unused_and_periodic_only_TCs();
    return 0;
  }
  AL_TC * rtc, * atc = tc.head();
  tot = 0; // insure correct count
  long nonperiodic = 0;
  while (atc) { // remove unused TCs in this day
    rtc = atc;
    atc = atc->Next();
    if (!rtc->Get_DE()) rtc->remove();
    else {
      if (rtc->Get_DE()->tdperiod()==pt_nonperiodic) nonperiodic++;
      tot++; // faster than calling tchead->length()
    }
  }
  avail = 0;
  if (nonperiodic<=0) {
    tc.clear();
    tot = 0;
    if (Prev()) return tot + Prev()->remove_unused_and_periodic_only_TCs();
  } else if (Prev()) return tot + Prev()->remove_unused_TCs(); // from here on, days with only periodic tasks should not be cleared
  return tot;
}
#endif

float AL_Day::squared_day_distribution(time_t td) {
  // distribute values up to target date according to a squared function
  
  float sumval = 0;
  int tddays = days_to(td);
  float fdays = (float) tddays, fi = 0.0;
  long i = 0;
  PLL_LOOP_FORWARD(AL_Day,this,(e->DayDate()<td)) {
    e->val = 1.0 - (aldaydistslope*(fi*fi));
    sumval += e->val;
    i++; fi = ((float) i)/fdays;
  }
  return sumval;
}

float AL_Day::linear_day_distribution(time_t td) {
// linearly distribute values up to target date
  
  float sumval = 0;
  int tddays = days_to(td);
  float fdays = (float) tddays;
  long i = 0;
  PLL_LOOP_FORWARD(AL_Day,this,(e->DayDate()<td)) {
    e->val = 1.0 - (aldaydistslope*((float) i/fdays));
    sumval += e->val;
    i++;
  }
  return sumval;
}

#ifdef INCLUDE_DAY_SPECIFIC_WORK_TIMES
long Active_List::Add_Days(time_t tcsec, time_t curt, time_t dmaxt, time_t algenwt) {
  // This function allocates enough days to provide allength task chunks.
  // The days allocated determine the actual number of task chunks, hence
  // allength is updated.
  // The time available during the first day may be constrained by the
  // current time.
  // Days take into account weekday specific maximum work times, as
  // specified in wdworkendprefs[], as well as the maximum given in
  // seconds by algenwt.
  // returns the number of days allocated
  
  time_t dd = time_stamp_time_date(curtime); // current day start
  long actualallength = 0;
  //VOUT << "seeking " << allength << " chunks\n";
  //VOUT << "current time " << curt << '\n';
  //VOUT << "dmaxt " << dmaxt << '\n';
  for (long tcstoallocate = allength; (tcstoallocate>0); ) {
    AL_Day * newalday = new AL_Day(dd,tcsec,curt,dmaxt);
    al.link_before(newalday);
    long daytcs = newalday->set_avail(algenwt);
    //VOUT << "  " << daytcs;
    tcstoallocate -= daytcs;
    actualallength += daytcs;
    dd = time_add_day(dd); // (See the problems described in TL#200609141144.3.)
  }
  allength = actualallength;
  //VOUT << "\ngot " << allength << '\n';
  return al.length();
}
#else
AL_Day * Active_List::Add_Day(int typ, time_t tcsec, time_t dsmin, time_t dmaxt) {
  // Add a day to the AL
  time_t dd;
  if (al.tail()) dd = time_add_day(al.tail()->DayDate()); // next day
  else {
#ifdef DIAGNOSTIC_OUTPUT
    EOUT << "curtime = " << curtime << ", curtime(time_t) = " << time_stamp_time(curtime,true) << '\n';
#endif
    dd = time_stamp_time_date(curtime); // start with current day in calendar time
    typ = 1; // insure start is marked
  }
#ifdef DIAGNOSTIC_OUTPUT
  EOUT << "DD: " << dd << ", T: " << Time(NULL) << '\n';
  EOUT << "DDS: " << time_stamp("%Y%m%d%H%M",dd) << ", TS: " << time_stamp("%Y%m%d%H%M",Time(NULL)) << '\n';
#endif
  AL_Day * ald = new AL_Day(dd,typ,tcsec,dsmin,dmaxt);
  al.link_before(ald);
  return ald;
}
#endif

Active_List::Active_List(long totreq, long deplen, int algenwt, DIL_entry * sup): superior(sup), supstr("all"), alfilestr("active-list.all"), allength(0), deavail(0) {
  // totreq is the total number of task chunks required, i.e. the sum of all req[i] or the sum of all dvm[i].req
  
  if (superior) {
    supstr = superior->chars();
    alfilestr = "active-list."+supstr;
  }
  allength = Initialize_AL_Length(totreq,deplen,algenwt,sup);
}

#ifdef INCLUDE_DAY_SPECIFIC_WORK_TIMES
long Active_List::Initialize_AL_Length(long totreq, long deplen, time_t algenwt, DIL_entry *superior)
{
  // returns the number of TCs allocated, data is initialized
  // This new version (20060830) uses the wdworkendprefs data to set limits
  // to the number of hours allocated on specific days, while algenwt is
  // used as an upper limit on the number of hours allocated per day
  // (prior to any expansion).
  // algenwt is in seconds
  // totreq is the total number of task chunks required, i.e. the sum of all req[i] or the sum of all dvm[i].req

  // values used on computations
  long tcseconds = timechunksize * 60;
  float c_size = (float)tcseconds;
  long tcsperfullday = algenwt / tcseconds; // floor
#ifdef DIAGNOSTIC_OUTPUT
  EOUT << "tcsperfullday=" << tcsperfullday << '\n';
#endif

  // default initializations
  allength = generatealtcs; // directly into the Active_List::allength variable
#ifdef DIAGNOSTIC_OUTPUT
  INFO << "allength = " << allength << '\n';
#endif
  time_t lastdaytime = SECONDSPERDAY; // default to full day when no generatealmaxt is specified
  time_t allastdaylimit = -1;         // *** why compute this?
  //long aldays = 0;

  // curtime data
  time_t curtimesec = time_stamp_time(curtime); // *** Probably correct with noexit=false (20190127)
  time_t curdaystart = time_stamp_time_date(curtime);
  time_t curdaytime = time_stamp_time_of_day(curtime);
#ifdef DIAGNOSTIC_OUTPUT
  INFO << "curdaytime(time_t) = " << curdaytime << '\n';
#endif
  // *** MAKE NEXT LINE DAY SPECIFIC
  long curdaytcs = (long)ceil(((float)(algenwt - alworked)) / c_size); // ceiling
  long curdaymaxtcs = (SECONDSPERDAY - curdaytime) / tcseconds;        // floor, can't exceed limit
  if (curdaytcs > curdaymaxtcs)
    curdaytcs = curdaymaxtcs;
#ifdef DIAGNOSTIC_OUTPUT
  INFO << "curdaytcs = " << curdaytcs << '\n';
#endif

  // determine allength and aldays
  // THIS NOW CORRECTLY OBTAINS allength FROM totreq WHEN generatealtcs==0, BUT aldays IS OBTAINED LATER BY
  // ASKING EACH DAY FOR THE NUMBER OF TASK CHUNKS AVAILABLE.
  INFO << "AL request initiated with settings generatealtcs=" << generatealtcs << " and generatealmaxt=" << generatealmaxt << '\n';
  if (generatealtcs == 0)
  {
    if (superior)
    { // *** DIL entry specific AL
      ERROR << "Active_List::Initialize_AL_Length(): DIL entry specific AL not yet implemented.\n";
    }

    generatealtcs += totreq; // complete AL
    allength = generatealtcs;
  }
  else
  {
    if (generatealmaxt > 0)
    { // time limit requested
      String maxtstr = time_stamp("%Y%m%d%H%M", generatealmaxt);
      lastdaytime = time_stamp_time_of_day(maxtstr);
      allastdaylimit = lastdaytime - alworkdaystart; // work time on last day of AL
      if (allastdaylimit < 0)
        allastdaylimit = 0;
      if (allastdaylimit > algenwt)
        allastdaylimit = algenwt;
      time_t lastdaystart = time_stamp_time_date(maxtstr);
      if (curdaystart == lastdaystart)
      { // last day work time minus time already worked today
        time_t alrem = allastdaylimit - alworked;
        if (alrem < 0)
          allength = 1; // this can be expanded
        else
          allength = alrem / tcseconds; // floor, can't exceed limit
                                        //aldays = 0;
      }
      else
      {
        long numfulldays = (lastdaystart - (curdaystart + SECONDSPERDAY)) / SECONDSPERDAY; // full days between current and last
        if (numfulldays > 0)
          allength += numfulldays * tcsperfullday; // chunks in those days
        else
          allength = 0;
        allength += allastdaylimit / tcseconds; // floor, can't exceed limit
        allength += curdaytcs;
      }
    }
  }

  // initialize data
#ifdef DIAGNOSTIC_OUTPUT
  INFO << "curdaytime (GM)    = " << time_stamp_GM("%H%M", curdaytime) << '\n';
#endif
  long aldays = Add_Days(tcseconds, curtimesec, lastdaytime, algenwt); // also updates allength
  if (!al.head())
  {
    ERROR << "Active_List::Initialize_AL_Length(): No AL days generated.\n";
    return -1;
  }
#ifdef DIAGNOSTIC_OUTPUT
  INFO << "algenwt = " << algenwt << ", alworked = " << alworked << '\n';
  INFO << "Day TCs (tot,avail): ";
  PLL_LOOP_FORWARD(AL_Day, al.head(), 1)
  EOUT << '(' << e->Total() << ',' << e->Avail() << "),";
  EOUT << '\n';
  INFO << "sum of available TCs in allocated days: allength = " << allength << '\n';
#endif
  INFO << "Preparing AL consisting of " << aldays << " Days that together provide " << allength << " available task chunks.\n";
  return allength;
}

#else // INCLUDE_DAY_SPECIFIC_WORK_TIMES

long Active_List::Initialize_AL_Length(long totreq, long deplen, time_t algenwt, DIL_entry * superior) {
  // returns the number of TCs allocated, data is initialized
  
  // values used on computations
  long tcseconds = timechunksize*60;
  float c_size = (float) tcseconds;
  long tcsperfullday = algenwt/tcseconds; // floor
#ifdef DIAGNOSTIC_OUTPUT
  EOUT << "tcsperfullday=" << tcsperfullday << '\n';
#endif
  // default initializations
  long allength = generatealtcs; // Note that here it is a local variable.
#ifdef DIAGNOSTIC_OUTPUT
  EOUT << "allength = " << allength << '\n';
#endif
  time_t lastdaytime = SECONDSPERDAY; // default to full day when no generatealmaxt is specified
  time_t allastdaylimit = -1; // *** why compute this?
  long aldays = 0;
  // curtime data
  time_t curdaystart = time_stamp_time_date(curtime);
  time_t curdaytime = time_stamp_time_of_day(curtime);
#ifdef DIAGNOSTIC_OUTPUT
  EOUT << "curdaytime(time_t) = " << curdaytime << '\n';
#endif
  long curdaytcs = (long) ceil(((float) (algenwt-alworked))/c_size); // ceiling
  long curdaymaxtcs = (SECONDSPERDAY-curdaytime)/tcseconds; // floor, can't exceed limit
  if (curdaytcs>curdaymaxtcs) curdaytcs = curdaymaxtcs;
#ifdef DIAGNOSTIC_OUTPUT
  EOUT << "curdaytcs = " << curdaytcs << '\n';
#endif
  // determine allength and aldays
  INFO << "AL request initiated with settings generatealtcs=" << generatealtcs << " and generatealmaxt=" << generatealmaxt << '\n';
  if (generatealtcs==0) {
    if (superior) { // *** DIL entry specific AL
      ERROR << "Active_List::Initialize_AL_Length(): DIL entry specific AL not yet implemented.\n";
    }
    generatealtcs += totreq; // complete AL
    allength = generatealtcs;
    aldays = (long) ceil(((float) (allength-curdaytcs))/((float) tcsperfullday)); // ceiling, to include remainder
  } else {
    if (generatealmaxt>0) { // time limit requested
      String maxtstr = time_stamp("%Y%m%d%H%M",generatealmaxt);
      lastdaytime = time_stamp_time_of_day(maxtstr);
      allastdaylimit = lastdaytime - alworkdaystart; // work time on last day of AL
      if (allastdaylimit<0) allastdaylimit = 0;
      if (allastdaylimit>algenwt) allastdaylimit = algenwt;
      time_t lastdaystart = time_stamp_time_date(maxtstr);
      if (curdaystart==lastdaystart) { // last day work time minus time already worked today
	time_t alrem = allastdaylimit - alworked;
	if (alrem<0) allength = 1; // this can be expanded
	else allength = alrem/tcseconds; // floor, can't exceed limit
	aldays = 0;
      } else {
	long aldays = (lastdaystart-(curdaystart+SECONDSPERDAY))/SECONDSPERDAY; // full days between current and last
	if (aldays>0) allength += aldays*tcsperfullday; // chunks in those days
	else allength = 0;
	allength += allastdaylimit/tcseconds; // floor, can't exceed limit
	allength += curdaytcs;
	aldays++; // add last day for day start tags
      }
    } else aldays = (long) ceil(((float) (allength-curdaytcs))/((float) tcsperfullday)); // ceiling, to include remainder
  }
  // initialize data
#ifdef DIAGNOSTIC_OUTPUT
  EOUT << "curdaytime (GM)    = " << time_stamp_GM("%H%M",curdaytime) << '\n';
#endif
  if (!aldays) Add_Day(1,tcseconds,curdaytime,lastdaytime); // current and last day
  else {
    Add_Day(1,tcseconds,curdaytime); // current day
#ifdef DIAGNOSTIC_OUTPUT
    for (int i=0; i<(aldays-1); i++) {
      AL_Day * tmp = Add_Day(0,tcseconds,0); // full days
      EOUT << "daystartmin = " << time_stamp_GM("%Y%m%d -- %H%M",tmp->DayStartMin()) << '\n';
      EOUT << "alworkdaystart = " << time_stamp_GM("%Y%m%d -- %H%M",alworkdaystart) << '\n';
      EOUT << "daystart = " << time_stamp_GM("%Y%m%d -- %H%M",tmp->DayStart()) << '\n';
    }
#else
    for (int i=0; i<(aldays-1); i++) Add_Day(0,tcseconds,0); // full days
#endif
    Add_Day(2,tcseconds,0,lastdaytime); // last day
  }
#ifdef DIAGNOSTIC_OUTPUT
  EOUT << "allength determined to get " << aldays << " days: " << allength << '\n';
#endif
  if (!al.head()) {
    ERROR << "Active_List::Initialize_AL_Length(): No AL days generated.\n";
    return -1;
  }
#ifdef DIAGNOSTIC_OUTPUT
  EOUT << "algenwt = " << algenwt << ", alworked = " << alworked << '\n';
#endif
  allength = al.head()->set_avail(algenwt-alworked); // remaining time to work on current day (prior to any expansions)
  PLL_LOOP_FORWARD(AL_Day,al.head()->Next(),1) allength += e->set_avail(algenwt);
#ifdef DIAGNOSTIC_OUTPUT
  EOUT << "Day TCs (tot,avail): "; PLL_LOOP_FORWARD(AL_Day,al.head(),1) EOUT << '(' << e->Total() << ',' << e->Avail() << "),"; EOUT << '\n';
  EOUT << "sum of available TCs in allocated days: allength = " << allength << '\n';
#endif
  return allength;
}
#endif // INCLUDE_DAY_SPECIFIC_WORK_TIMES

long Active_List::expand(long expandn, time_t td) {
  // expand work time by adding task chunks to days prior to
  // the target date td
  // returns the number of task chunks added (-1==error)
  
  long nadded = 0;
  float sumval, uval;
  switch (aldaydistfunc) { // apply chosen AL day distribution function
  case ALDAYDISTFUNCSQUARED: if ((sumval = al.head()->squared_day_distribution(td))<0) return -1; break;
  default: if ((sumval = al.head()->linear_day_distribution(td))<0) return -1;
  }
#ifdef DETAILED_AL_DIAGNOSTIC_OUTPUT
  VOUT << "AL: deavail =" << deavail << '\n';
#endif
  while ((expandn>nadded) && (sumval>FLT_EPSILON)) {
    // *** should this really be done randomly?
    uval = sumval*(((float) rand()) / ((float) RAND_MAX));
    AL_Day * e;
    for (e = al.head(); (e); e = e->Next()) if (uval>e->val) uval -= e->val; else break;
    if (!e) break; // no further expansion is possible
    if (e->expand(1,td)>0) nadded++;
    else { // day cannot expand further
      sumval -= e->val;
      e->val = 0.0;
    }
  }
  allength += nadded; // update total number of TCs in AL
  deavail += nadded; // update temporary value tracking available TCs
#ifdef DETAILED_AL_DIAGNOSTIC_OUTPUT
  VOUT << "AL: deavail =" << deavail << '\n';
#endif
  return nadded;
}

float Active_List::set_TC_values() {
  // set TC values in AL
  // returns sum of all TC values
  // requires that al contains a valid deavail value
  
  if (deavail<=0) return 0.0;
  float sumval = 0.0;
  long i_start = 0;
  PLL_LOOP_FORWARD(AL_Day,al.head(),1) sumval += e->set_TC_values(i_start,deavail);
  return sumval;
}

#ifdef INCLUDE_PERIODIC_TASKS
float Active_List::allocate_with_value(float uval, DIL_Virtual_Matrix & dvm) {
// allocate a DIL entry to a task chunk according to
// a random value
// returns the value of the allocated task chunk
	if (!al.head()) return 0.0;
	float v = al.head()->allocate_with_value(uval,dvm.dep);
	if (v>0.0) deavail--; // update temporary value tracking available TCs
	return v;
}
#else
float Active_List::allocate_with_value(float uval, DIL_entry * de) {
  // allocate a DIL entry to a task chunk according to
  // a random value
  // returns the value of the allocated task chunk
  
  if (!al.head()) return 0.0;
  float v = al.head()->allocate_with_value(uval,de);
  if (v>0.0) deavail--; // update temporary value tracking available TCs
  return v;
}
#endif

#ifdef INCLUDE_EXACT_TARGET_DATES
#ifdef INCLUDE_PERIODIC_TASKS
long Active_List::allocate_with_date(DIL_Virtual_Matrix & dvm) {
  // allocate req number of task chunks (if available) exactly prior to
  // the target date of de
  
  if (!al.head()) return dvm.req;
  time_t de_tdate = dvm.td;
  if ((al.tail()->DayDate()+SECONDSPERDAY-1)<de_tdate) {
    // *** THIS ONE IS TRICKY!
    //     I want to be able to follow the link.
    INFO << "dil2al: Exact target date " << time_stamp("%Y%m%d%H%M",de_tdate) << " exceeds range of generated AL, allocation of DIL entry ";
    if (calledbyforminput) INFO << "<A HREF=\"" << idfile << '#' << dvm.dep->chars() << "\">";
    INFO << dvm.dep->chars();
    if (calledbyforminput) INFO << "</A>";
    INFO << " excluded\n";
    long secondsrequired = timechunksize*60*dvm.req;
    if ((al.tail()->DayDate()+SECONDSPERDAY-1)>=(de_tdate-secondsrequired)) INFO << "        [The " << dvm.req << " task chunks needed MAY overlap with time allocated in this AL!]\n";
    return 0;
  }
  // Note: Processing below implies that de is within the current time (checked by calling function) to maximum AL_Day range.
  long regularworktimechunks = 0;
  PLL_LOOP_FORWARD(AL_Day,al.head(),1) if ((e->DayDate()+SECONDSPERDAY)>de_tdate) {
    dvm.req = e->allocate_with_date(dvm.dep,de_tdate,dvm.req,regularworktimechunks);
    break;
  }
  deavail -= regularworktimechunks;
  return dvm.req;
}
#else
long Active_List::allocate_with_date(DIL_entry * de, long req) {
  // allocate req number of task chunks (if available) exactly prior to
  // the target date of de
  
  if (!al.head()) return req;
  time_t de_tdate = de->Target_Date();
  if ((al.tail()->DayDate()+SECONDSPERDAY-1)<de_tdate) {
    // *** THIS ONE IS TRICKY!
    //     I want to be able to follow the link.
    INFO << "dil2al: Exact target date " << time_stamp("%Y%m%d%H%M",de_tdate) << " exceeds range of generated AL, allocation of DIL entry ";
    if (calledbyforminput) INFO << "<A HREF=\"" << idfile << '#' << de->chars() << "\">";
    INFO << de->chars();
    if (calledbyforminput) INFO << "</A>";
    INFO << " excluded\n";
    long secondsrequired = timechunksize*60*req;
    if ((al.tail()->DayDate()+SECONDSPERDAY-1)>=(de_tdate-secondsrequired)) INFO << "        [The " << req << " task chunks needed MAY overlap with time allocated in this AL!]\n";
    return 0;
  }
  // Note: Processing below implies that de is within the current time (checked by calling function) to maximum AL_Day range.
  long regularworktimechunks = 0;
  PLL_LOOP_FORWARD(AL_Day,al.head(),1) if ((e->DayDate()+SECONDSPERDAY)>de_tdate) {
    req = e->allocate_with_date(de,de_tdate,req,regularworktimechunks);
    break;
  }
  deavail -= regularworktimechunks;
  return req;
}
#endif
#endif

AL_TC * Active_List::Get_Avail_TC(unsigned long n) {
  // returns the nth available task chunk in the AL
  // creates task chunks if necessary where tchead==NULL
  
  if (!al.head()) return NULL;
  return al.head()->Get_Avail_TC(n);
}

AL_TC * Active_List::allocate(DIL_entry * de) {
  // allocates de to the first available task chunk in the AL
  // returns the allocated task chunk
  // creates task chunks if necessary where tchead==NULL
  // NOTE: This does not use de->Target_Date() and is therefore safe when de is set by DIL_Virtual_Matrix.dep.
  
  if (!al.head()) return NULL;
  return al.head()->allocate(de);
}

#ifdef INCLUDE_PERIODIC_TASKS
AL_Day * Active_List::Add_Target_Date(DIL_Virtual_Matrix & dvm) {
  // Adds a target date reference to the day that contains it
  
  if (!al.head()) return NULL;
  return al.head()->Add_Target_Date(dvm);
}
#else
AL_Day * Active_List::Add_Target_Date(DIL_entry * de) {
  // Adds a target date reference to the day that contains it
  
  if (!al.head()) return NULL;
  return al.head()->Add_Target_Date(de);
}
#endif

#ifdef INCLUDE_PERIODIC_TASKS
AL_TD * Active_List::Add_Passed_Target_Date(DIL_Virtual_Matrix & dvm) {
  // Adds a passed target date reference to the list
  
  if (!dvm.dep) return NULL;
  AL_TD * atd = new AL_TD(dvm);
  td.link_before(atd);
  return atd;
}
#else
AL_TD * Active_List::Add_Passed_Target_Date(DIL_entry * de) {
  // Adds a passed target date reference to the list
  
  if (!de) return NULL;
  AL_TD * atd = new AL_TD(*de);
  td.link_before(atd);
  return atd;
}
#endif

long Active_List::remove_unused_TCs() {
  // clean up by removing unused TCs
  // returns remaining number of TCs in AL
  
  if (!al.tail()) {
    allength = 0;
    return 0;
  }
  allength = al.tail()->remove_unused_TCs();
  return allength;
}

#ifdef INCLUDE_PERIODIC_TASKS
long Active_List::remove_unused_and_periodic_only_TCs() {
  // clean up by removing unused TCs as well as AL_Days
  // with only periodic tasks, except the very first day.
  
  if (!al.tail()) {
    allength = 0;
    return 0;
  }
  allength = al.tail()->remove_unused_and_periodic_only_TCs();
  // Now remove empty tail days
  for (AL_Day * d = al.tail(); (d); d = al.tail()) {
    if (!d->TC_Head()) d->remove();
    else break; // only remove empty days at the end of the list
  }
  return allength;
}
#endif

#define FIVEMINSECS 300
void alCRT_day::init(time_t _daytd) {
  daytd = _daytd;
  for (int i=0; i<288; i++) fiveminchunk[i]=NULL;
  int startchunk = alworkdaystart / FIVEMINSECS; // work from this chunk onward
  int endchunk = wdworkendprefs[time_day_of_week(daytd)] / FIVEMINSECS; // no more work from this chunk onward
  for (int i=0; i<startchunk; i++) fiveminchunk[i]=((DIL_entry_ptr) 1); // code 1 = non-work time
  for (int i=endchunk; i<288; i++) fiveminchunk[i]=((DIL_entry_ptr) 1);
}

bool alCRT_day::reserve_exact(DIL_entry_ptr dep, int & grains, time_t td) {
  // reserves 5 minute grains immediately preceding the exact target
  // date td (td<0 means use time from the end of the day) of dep,
  // returns true if any of the map entries are already occupied,
  // and returns the remaining grains that did not fit into the day.
  
  if (td<0) td = SECONDSPERDAY; // use time starting at the end of the day
  else {
    td -= daytd; // seconds into the day
    if ((td<0) || (td>SECONDSPERDAY)) return false;
  }
  bool overlap = false;
  int before_i = td / FIVEMINSECS;
  for (int i=(before_i-1); ((i>=0) && (grains>0)); i--) {
    if (fiveminchunk[i]>((DIL_entry_ptr) 1)) overlap = true;
    else fiveminchunk[i]=dep;
    grains--;
  }
  return overlap;
}

int alCRT_day::reserve_fixed(DIL_entry_ptr dep, int & grains, time_t td) {
  // reserves 5 minute grains preceding the target date td (td<0
  // means use time from the end of work time of the day) of dep,
  // returns 0 if all goes well (even if more grains remain to be
  // reserved), returns 1 if there is a problem with the td value,
  // and returns 2 if reserved space from an earlier iteration of
  // the same task was encountered (which suggests that periodic
  // tasks may be scheduled too near each other).
  // returns true if there is a problem with the td value,
  // and returns the remaining grains that did not fit into the day.
  // reserves nearest available chunklets from td on down.
  
  int before_i = 288;
  if (td>=0) {
    td -= daytd; // seconds into the day
    if ((td<0) || (td>SECONDSPERDAY)) return 1;
    before_i = td / FIVEMINSECS;
  }
  for (int i=(before_i-1); ((i>=0) && (grains>0)); i--) {
    if (!fiveminchunk[i]) {
      fiveminchunk[i]=dep;
      grains--;
    } else if (fiveminchunk[i]==dep) return 2;
  }
  return 0;
}

time_t alCRT_day::reserve(DIL_entry_ptr dep, int & grains) {
  // reserves 5 minute grains preceding from the earliest
  // available grain onward,
  // returns the target date immediately following the last
  // grain allocated or immediately following the end of the day
  // means use time from the end of work time of the day) of dep,
  
  time_t td = -1;
  for (int i=0; ((i<288) && (grains>0)); i++) {
    if (!fiveminchunk[i]) {
      fiveminchunk[i]=dep;
      grains--;
      td = i+1;
    }
  }
  if (td<0) return td;
  td *= FIVEMINSECS;
  return daytd + td;
}

alCRT_map::alCRT_map(time_t t,int n): num_days(n) {
  if (n<1) { n=1; num_days=1; }
  starttime = t;
  firstdaystart = time_stamp_time_date(time_stamp("%Y%m%d%H%M",t));
  day = new alCRT_day[n];
  time_t daystart = firstdaystart;
  for (int i=0; i<n; i++) {
    day[i].init(daystart);
    daystart = time_add_day(daystart,1);
  }
  int firstdayusedseconds = t - firstdaystart;
  int chunksused = firstdayusedseconds / FIVEMINSECS;
  if ((firstdayusedseconds % FIVEMINSECS)>0) chunksused++;
  if (chunksused>288) {
    ERROR << "alCRT_map::alCRT_map(): The chunksused variable registers as greater than 288. Stopping.\nPLEASE NOTE: If the clock was moved back from daylight savings time today and this function was used with a time greater than 23:00, then more than 288 five minute chunks of the day can indeed have been used. There is no special case for this, since it is a problem that can only occur for a maximum of 60 minutes per year. If this is the reason for the problem then please wait until 00:00 to try again.\n";
    Exit_Now(1);
  }
  for (int i=0; i<chunksused; i++) day[0].fiveminchunk[i]=((DIL_entry_ptr) 1); // unavailable on the first day
}

bool alCRT_map::reserve_exact(DIL_entry_ptr dep, int req, time_t td) {
  // reserves req time chunks in FIVEMINSECS granularity
  // immediately preceding the exact target date td of dep,
  // returns true if any of the map entries are already
  // occupied (i.e. DIL entries with exact target dates
  // overlap).
  
  int grains = (req*timechunksize)/5;
  bool overlap = false;
  int tdday = find_dayTD(td);
  if (tdday<0) return false;
  while ((grains>0) && (tdday>=0)) {
    if (day[tdday].reserve_exact(dep,grains,td)) overlap=true;
    tdday--;
    td = -1;
  }
  return overlap;
}

int alCRT_map::find_dayTD(time_t td) {
  if ((td<day[0].daytd) || (td>=(day[num_days-1].daytd+SECONDSPERDAY))) return -1;
  int bottom = 0, top = num_days, middle = num_days / 2;
  while ((bottom<middle) && (top>middle)) {
    if (td<day[middle].daytd) {
      top = middle;
      middle = bottom + ((middle-bottom) / 2);
    } else {
      bottom = middle;
      middle = middle + ((top-middle) / 2);
    }
  }
  return bottom;
}

bool alCRT_map::reserve_fixed(DIL_entry_ptr dep, int req, time_t td) {
  // reserves req time chunks in FIVEMINSECS granularity
  // preceding the fixed target date td of dep,
  // returns true if there are insufficient grains available
  // reserves nearest available chunklets from td on down.
  
  int grains = (req*timechunksize)/5;
  bool insufficient = false;
  int tdday = find_dayTD(td);
  if (tdday<0) return false;
  time_t td_topflag = td;
  while ((grains>0) && (tdday>=0)) {
    switch (day[tdday].reserve_fixed(dep,grains,td_topflag)) {
    case 2:
      insufficient = true;
      tdday=-1;
      // *** The following should have a link that can be followed, or
      //     something similar, even when using UI_Info_Yad and such.
      if (verbose) WARN << "AL CTR Warning: Periodic task <A HREF=\""+idfile+'#'+dep->str()+"\">DIL#" << dep->str() << "</A> may repeat too frequently before "+time_stamp("%Y%m%d%H%M",td)+"!\n";
      break;;
    case 1:
      insufficient = true;
    }
    tdday--;
    td_topflag = -1; // set to indicate start next at end of preceding day
  }
  if (grains>0) insufficient=true;
  return insufficient;
}

time_t previous_group_td = -1;

time_t alCRT_map::reserve(DIL_entry_ptr dep, int req) {
  // reserves req time chunks in FIVEMINSECS granularity
  // from the earliest available grain onward,
  // returns a corresponding suggested target date taking
  // into account TD preferences, or returns -1 if there
  // are insufficient grains available.
  
  int grains = (req*timechunksize)/5;
  int tdday = 0;
  time_t epstd = starttime;
  while ((grains>0) && (tdday<num_days)) {
    epstd = day[tdday].reserve(dep,grains);
    tdday++;
  }
  if (grains>0) return -1;
  
  // Adapt epstd suggestion to TD preferences
  if (targetdatepreferences) { // determine if the calculated target date should be modified according to preferences
    if (prioritypreferences) { // use separate end-of-day preferences based on task priority (added 20180104, see note at top)
      
      // which end-of-day to use is indicated by the urgency parameter
      time_t priorityendofday = dolaterendofday;
      if (dep->Urgency()>=1.0) priorityendofday = doearlierendofday;
      if (priorityendofday>(23*3600+1800)) priorityendofday -= 1800; // insure space for minute offsets to avoid auto-merging
      
      // compare that with the proposed target date time of day
      tdday--; // back to the last day accessed
      time_t timeofday = epstd - day[tdday].daytd;
      if (timeofday<priorityendofday) { // set to priorityendofday on this day
	epstd = day[tdday].daytd + priorityendofday;
      } else { // set to priorityendofday on next day
	epstd = time_add_day(day[tdday].daytd,1) + priorityendofday;
      }
      
      // check if a previous group already has the target date, if so offset slightly to preserve grouping and group order
      if (epstd<=previous_group_td) epstd = previous_group_td+alCRTepsgroupoffset; // one or a few minutes offset
    } else {
      if (weekdayworkendpreferences) { // preferred target times on specific days of the week
	int dayofweek = atoi((const char *) time_stamp("%w",epstd));
	if (wdworkendprefs[dayofweek]>=0) {
	  tdday--; // back to the last day accessed
	  time_t timeofday = epstd - day[tdday].daytd;
	  if (timeofday<wdworkendprefs[dayofweek]) { // set preferred time
	    epstd = day[tdday].daytd + wdworkendprefs[dayofweek] - 3600; // give one hour room to avoid automatically merging groups
	  } else { // set to preferred time on next day
	    epstd = time_add_day(day[tdday].daytd,1);
	    dayofweek++; if (dayofweek>6) dayofweek = 0;
	    if (wdworkendprefs[dayofweek]>=0) epstd +=  wdworkendprefs[dayofweek] - 3600; // give one hour room to avoid automatically merging groups
	    else epstd += alworkdaystart + (((time_t) rint(alepshours*60.0))*60);
	  }
	  
	  // check if a previous group already has the target date, if so offset slightly to preserve grouping and group order
	  if (epstd<=previous_group_td) epstd = previous_group_td+60; // one minute offset
	}
      }
    }
  }
  return epstd;
}

bool alCRT_map::Plot_Map_to_File(String mapfile) {
  // For visualization, inspection, debugging and optimization.
  // Plots a single character for every 5 minute chunk-let reserved,
  // one day per line, with links to the DIL entries that reserve a
  // chunk-let.
  
  String mapstr("<HTML>\n<HEAD><TITLE>Formalizer: CTR Map of reserved 5 minute chunk-lets</TITLE></HEAD>\n<BODY>\n\n<H1>Formalizer: CTR Map of reserved 5 minute chunk-lets</H1>\n\nLegend: o=open, -=passed, E=exact, e=periodic exact, F=fixed, f=periodic fixed, V=variable\n<P>\n\n<PRE>date \\ hr |0          |1          |2          |3          |4          |5          |6          |7          |8          |9          |10         |11         |12         |13         |14         |15         |16         |17         |18         |19         |20         |21         |22         |23         |\n</PRE>\n");
  
  for (int i = 0; i<num_days; i++) {
    mapstr += time_stamp("%Y%m%d",day[i].daytd) + ": ";
    for (int j = 0; j<288; j++) {
      if (day[i].fiveminchunk[j]==NULL) {
	mapstr += 'o';
      } else if (day[i].fiveminchunk[j]==((DIL_entry_ptr) 1)) {
	mapstr += '-';
      } else {
	String reservedchunklet("V");
	if (day[i].fiveminchunk[j]->tdexact()) {
	  if (day[i].fiveminchunk[j]->tdperiod()==pt_nonperiodic) reservedchunklet = "E"; else reservedchunklet = "e";
	} else if (day[i].fiveminchunk[j]->tdfixed()) {
	  if (day[i].fiveminchunk[j]->tdperiod()==pt_nonperiodic) reservedchunklet = "F"; else reservedchunklet = "f";
	}
	mapstr += "<a href=\""+idfile+'#'+day[i].fiveminchunk[j]->str()+"\"><b>"+reservedchunklet+"</b></a>";
      }
    }
    mapstr += "<BR>\n";
  }
  mapstr += "\n\n</BODY>\n</HTML>\n";
  
  return write_file_from_String(mapfile,mapstr,"",true);
}

#ifdef INCLUDE_PERIODIC_TASKS
void generate_EPS_cells(alCRT_map & map, time_t grouptd, String & cumreqstr, String & rowreqstr, double & groupreq, double cumreq, time_t t_current, DIL_entry & de, bool isvirtual) {
#else
void generate_EPS_cells(alCRT_map & map, time_t grouptd, String & cumreqstr, String & rowreqstr, double & groupreq, double cumreq, time_t t_current, DIL_entry & de) {
#endif
  // generate HTML table cells for the EPS method date presented
  // when alshowcumulativereq==true
  // (See <A HREF="../../doc/html/lists.html#gS1">Earliest Possible Scheduling (EPS)</A>.)
  
  double cumreqhours = (cumreq*((double) timechunksize))/60.0;
  double epsdays = floor(cumreqhours/alepshours);
  double epshours = cumreqhours - (epsdays*alepshours);
  time_t epsdayssec = ((time_t) epsdays)*SECONDSPERDAY;
  time_t epshourssec = ((time_t) rint(epshours*60.0))*60; // note: properly converts 0.33 and 0.67 hours to 20 and 40 minutes respectively
  time_t epsgrouptd = t_current + epsdayssec + epshourssec;
  String epsgrouptreq = String((groupreq*((double) timechunksize))/60.0,"%6.2f");
  String epsgrouptdstr = time_stamp("%Y%m%d%H%M",epsgrouptd);
  
  // ADJUST EPS SUGGESTED NEW TARGET DATES TO ALIGN WITH PREFERENCES SUCH AS ENDS OF WORKDAYS
  if (targetdatepreferences) { // determine if the calculated target date should be modified according to stated preferences
    if (weekdayworkendpreferences) { // preferred target times on specific days of the week
      //	    cout << epsgrouptdstr << " --- ";
      int dayofweek = atoi((const char *) time_stamp("%w",epsgrouptd));
      if (wdworkendprefs[dayofweek]>=0) {
	if (time_stamp_time_of_day(epsgrouptdstr)<wdworkendprefs[dayofweek]) { // to preferred time
	  epsgrouptd = time_stamp_time_date(epsgrouptdstr) + wdworkendprefs[dayofweek];
	} else { // set to preferred time on next day
	  dayofweek++; if (dayofweek>6) dayofweek = 0;
	  if (wdworkendprefs[dayofweek] >= 0 ) epsgrouptd = time_stamp_time_date(epsgrouptdstr) + SECONDSPERDAY + wdworkendprefs[dayofweek];
	  else epsgrouptd = time_stamp_time_date(epsgrouptdstr) + SECONDSPERDAY + alworkdaystart + (((time_t) rint(alepshours*60.0))*60);
	}
	epsgrouptdstr = time_stamp("%Y%m%d%H%M",epsgrouptd);
      }
      //	    cout << epsgrouptdstr << '\n';
    }
  }
  
  // PUT GROUP TIME REQUIRED AND EPS TARGET DATE INTO LINES IN FORM
  cumreqstr.gsub("@GROUPTREQ@",epsgrouptreq); // *** Another way to do this may be faster!
  cumreqstr.gsub("@GROUPEPSTD@",epsgrouptdstr);
  // [***INCOMPLETE] In the lines below, I could indicate fixed target dates by not offering update and text input.
  if (rapidscheduleupdating && (epsgrouptd<=grouptd)) { // option not to update target dates beyond the EPS suggested target dates
    cumreqstr.gsub("@GROUPCHECKED@\"","\" checked");
    rowreqstr += HTML_put_table_cell("",epsgrouptreq);
#ifdef INCLUDE_PERIODIC_TASKS
    if (isvirtual) rowreqstr += HTML_put_table_cell("","<I>(Periodic Task)</I>");
    else 
#endif
      rowreqstr += HTML_put_table_cell("",HTML_put_form_checkbox(String("TDCHKBX")+de.chars(),"noupdate","",true)+HTML_put_form_text(String("TD")+de.chars(),epsgrouptdstr,12));
  } else {
    cumreqstr.gsub("@GROUPCHECKED@","");
    rowreqstr += HTML_put_table_cell("",epsgrouptreq);
#ifdef INCLUDE_PERIODIC_TASKS
    if (isvirtual) rowreqstr += HTML_put_table_cell("","<I>(Periodic Task)</I>");
    else 
#endif
      rowreqstr += HTML_put_table_cell("",HTML_put_form_checkbox(String("TDCHKBX")+de.chars(),"noupdate","")+HTML_put_form_text(String("TD")+de.chars(),epsgrouptdstr,12));
  }
  groupreq = 0.0;
  //return epsgrouptdstr;
}

/*
An alternative simplified method would specify an arrary of DIL_Entry pointers
per day, of which some ranges are regular work time and some are not.
When distributing a task, ask all the days for the number of available
task chunks prior to the target date. Use that sum, and a random distribution
(see implementation in nibr) to select a task chunk the required number of
times. To create this, just work your way through the generate_AL function,
or rather a second version of it, and replace called functions as you go.
*/

bool ALCRT_continue_condition(DIL_entry * de, time_t t_limit) {
  // Indicates if processing for the Cumulative Required Times HTML form
  // should continue, based on settings of alCRTlimit.
    
  switch (alCRTlimit) {
  case 1: if (de->Target_Date()<t_limit) return true; break;;
  case 2: if (de->Target_Date()<(MAXTIME_T)) return true; break;;
  case 3: if (de->Target_Date()<t_ctrstop) return true; break;;
  default: return true;
  }
  return false;
}

#ifdef INCLUDE_PERIODIC_TASKS
time_t Add_to_Date(time_t td, periodictask_t period, int every) {
  if (every<1) every=1;
  switch (period) {
  case pt_daily: td = time_add_day(td,every); break;;
  case pt_workdays: for (int i=every; i>0; i--) { if (time_day_of_week(td)<5) td = time_add_day(td); else td = time_add_day(td,3); } break;;
  case pt_weekly: td = time_add_day(td,every*7); break;;
  case pt_biweekly: td = time_add_day(td,every*14); break;;
  case pt_monthly: td = time_add_month(td,every); break;;
  case pt_endofmonthoffset: for (int i=every; i>0; i--) td = time_add_month_EOMoffset(td); break;;
  case pt_yearly: td = time_add_month(td,every*12); break;;
    //case pt_span: td = time_add_day(td); break;; // for backward compatibility, now means pt_daily
  default:
    ERROR << "Add_to_Date(): Do not know how to increment for period enumerated value #" << (long) period << ". Treating as 'daily'.\n"; // was (unsigned int) period
    td = time_add_day(td,every);
  }
  return td;
}

DIL_Virtual_Matrix * generate_AL_prepare_periodic_tasks(DIL_entry ** dep, int & deplen, time_t algenhpd , DIL_entry * superior) {
  // creates a matrix of tasks data that includes both tasks in
  // the Detailed_Items_List, and virtual copies for the possible
  // additional target dates of periodic tasks.
  //
  // NOTE: delete[] the array that is returned when dealt with!
  
  // 1. Count the number of days
  progress->start();
  progress->update(0,"Prepare periodic tasks: count days");
  INFO << "Preparing virtual task copies for periodic tasks:\n";
  long * req = DIL_Required_Task_Chunks(dep,deplen);
  if (verbose) INFO << "  creating temporary AL day list\n";
  long totreq = 0;
  for (int i = 0; (i<deplen); i++) totreq += req[i];
  long generatealtcscache = generatealtcs;
  Active_List al(totreq,deplen,algenhpd,superior);
  generatealtcs = generatealtcscache;
  if (verbose) INFO << "  number of task chunks in AL without room for virtual periodic tasks: " << al.length() << '\n';
  long numdays = al.days();
  if ((numdays>alperiodictasksincludedayslimit) && (alperiodictasksincludedayslimit>0)) {
    if (verbose) INFO << "  number of days with repeated periodic tasks included limited by alperiodictasksincludedayslimit (" << alperiodictasksincludedayslimit << ").\n";
    numdays = alperiodictasksincludedayslimit; // optional limit
  }
  long * virt = new long[deplen]; long virttot = 0;
  for (int i=0; (i<deplen); i++) virt[i] = 0;
  
  // 2. For each periodic task, determine the likely number of occurrences in those days + 1
  progress->update(10,"Prepare periodic tasks: likely occurrences");
  for (int i=0; (i<deplen); i++) if ((req[i]>0) && (dep[i]->tdperiod()!=pt_nonperiodic) && (dep[i]->Is_Dependency_Of(superior))) { // periodic tasks
    virt[i] = 1;
    // *** possibly divide by "every"
    switch (dep[i]->tdperiod()) {
    case pt_daily: virt[i] += numdays; break;; // daily
    case pt_workdays: virt[i] += ((numdays*5)/7); break;; // workdays
    case pt_weekly: virt[i] += (numdays/7); break;; // weekly
    case pt_biweekly: virt[i] += (numdays/14); break;; // bi-weekly
    case pt_monthly: case pt_endofmonthoffset: virt[i] += (numdays/30); break;; // monthly or offset from end of month
    case pt_yearly: virt[i] += (numdays/365); break;; // yearly
      //OLD: case pt_span: virt[i] += (dep[i]->tdspan()-2); break;; // span (subtract 1 for the original and 1 for the virt[i]=1 initialization)
    default:
      ERROR << "generate_AL_prepare_periodic_tasks(): Do not know how to determine likely number of occurrences for period enumerated value #" << (long) dep[i]->tdperiod() << ". Treating as 'daily'.\n"; // was (unsigned int) dep[i]->tdperiod()
      virt[i] += numdays;
    }
    if (dep[i]->tdspan()>0) if (virt[i]>=dep[i]->tdspan()) virt[i]=dep[i]->tdspan()-1;
    virttot += virt[i];
  }
  if (verbose) INFO << "  adding " <<  virttot << " virtual task copies for periodic tasks\n";
  
  // 3. Create a task matrix that includes virtual copies of periodic tasks at periodic intervals
  progress->update(25,"Prepare periodic tasks: task matrix (originals)");
  long depplusvirt = deplen + virttot;
  DIL_Virtual_Matrix * dvm = new DIL_Virtual_Matrix[depplusvirt];
  for (int i=0; (i<deplen); i++) { // All original tasks
    dvm[i].dep = dep[i];
    dvm[i].req = req[i];
    dvm[i].td = dep[i]->Target_Date();
    dvm[i].isvirtual = false;
  }
  long c_size = ((long) timechunksize)*60;
  long virt_i = deplen;
  progress->update(50,"Prepare periodic tasks: task matrix (virtual copies)");
  for (int i=0; (i<deplen); i++) if (virt[i]>0) { // Virtual copies
    // Completion ratio of periodic tasks is interpreted as follows:
    // 0.0<=completion<1.0 means full time required for each virtual copy
    // completion<0.0 or completion>=1.0 means do not schedule the periodic task
    float completion = dep[i]->Completion_State();
    if ((completion<0.0) || (completion>=1.0)) continue;
    long required = dep[i]->Time_Required();
    if (required % c_size) required = (required/c_size) + 1;
    else required /= c_size;
    time_t virttd = dep[i]->Target_Date();
    for (long j = 0; j<virt[i]; j++) {
      dvm[virt_i].dep = dep[i];
      dvm[virt_i].req = required;
      dvm[virt_i].isvirtual = true;
      virttd = Add_to_Date(virttd,dep[i]->tdperiod(),dep[i]->tdevery());
      dvm[virt_i].td = virttd;
      virt_i++;
    }
  }
  progress->update(75,"Prepare periodic tasks: task matrix (sort)");
  qsort(dvm,depplusvirt,sizeof(DIL_Virtual_Matrix),DIL_Virtual_Matrix_target_date_qsort_compare);
  progress->stop();
  delete[] virt;
  deplen = depplusvirt;
  return dvm;
}
#endif

#ifdef INCLUDE_PERIODIC_TASKS
  #define DEPEL dvm[i].dep
  #define REQEL dvm[i].req
  #define TDEL dvm[i].td
  #define JDEPEL dvm[j].dep
  #define JREQEL dvm[j].req
#else
  #define DEPEL dep[i]
  #define REQEL req[i]
  #define TDEL dep[i]->Target_Date()
  #define JDEPEL dep[j]
  #define JREQEL req[j]
#endif

#ifdef INCLUDE_PERIODIC_TASKS
void generate_iCal_output(int deplen, DIL_Virtual_Matrix dvm[], time_t t_limit, DIL_entry * superior) {
#else
void generate_iCal_output(int deplen, DIL_entry ** dep, long req[], time_t t_limit, DIL_entry * superior) {
#endif
  INFO << "Generating iCal/Google Calendar calendar.ics output.\n";
  int calendar_i = 0;
  String entryexcerpt;
  String calendarstr = "BEGIN:VCALENDAR\nCALSCALE:GREGORIAN\nMETHOD:PUBLISH\nX-WR-CALNAME:dil2al\nX-WR-CALDESC: This calendar was produced by dil2al during CTR generation and contains tasks that meet specific calendar listing criteria.\n";
  /* Not included now:
     X-WR-TIMEZONE:America/New_York\n
     BEGIN:VTIMEZONE\nTZID:America/New_York\nX-LIC-LOCATION:America/New_York\nBEGIN:STANDARD\nTZOFFSETFROM:-0400\n
     TZOFFSETTO:-0500\nTZNAME:EST\nDTSTART:19701025T020000\nRRULE:FREQ=YEARLY;BYMONTH=10;BYDAY=-1SU\nEND:STANDARD\n
     BEGIN:DAYLIGHT\nTZOFFSETFROM:-0500\nTZOFFSETTO:-0400\nTZNAME:EDT\nDTSTART:19700405T020000\n
     RRULE:FREQ=YEARLY;BYMONTH=4;BYDAY=1SU\nEND:DAYLIGHT\nEND:VTIMEZONE\n
  */
  double alCTRGCtc = alCTRGoogleCalendartimecriterion*3600.0;
  time_t alCTRGCtzaseconds = alCTRGCtimezoneadjust*3600;
  time_t alCTRGCextent = 0;
  if (alCTRGoogleCalendardaysextent>0) alCTRGCextent = Time(NULL)+(alCTRGoogleCalendardaysextent*86400);
  for (int i=0; ((i<deplen) && (ALCRT_continue_condition(DEPEL,t_limit))); i++)
    if ((REQEL>0) && (DEPEL->Is_Dependency_Of(superior))) { // non-completed DIL entries
      if ((alCTRGCextent<1) || (TDEL<=alCTRGCextent)) {
	// use criteria to determine if shown in iCal/Google Calendar output
	bool includeinlist = ((alCTRGoogleCalendardaysextent<8) || (DEPEL->tdspan()>0));
	if (!includeinlist) if (DEPEL->tdexact()) includeinlist=true;
	if (!includeinlist) if (DEPEL->Time_Required()>=alCTRGCtc)
			      if (DEPEL->content)
				if (DEPEL->content->valuation>=alCTRGoogleCalendarvaluationcriterion) includeinlist=true;
	// possibly add to iCal/Google Calendar output
	if (includeinlist) {
	  if (DEPEL->tdexact()) calendarstr += "BEGIN:VEVENT\nDTSTART:" + time_stamp("%Y%m%dT%H%M00Z",TDEL - DEPEL->Time_Required()+alCTRGCtzaseconds) + "\nDTEND:" + time_stamp("%Y%m%dT%H%M00Z",TDEL+alCTRGCtzaseconds);
	  else calendarstr += "BEGIN:VEVENT\nDTSTART;VALUE=DATE:" + time_stamp("%Y%m%d",TDEL) + "\nDTEND;VALUE=DATE:" + time_stamp("%Y%m%d",time_add_day(TDEL));
	  if (DEPEL->Entry_Text()) {
	    entryexcerpt = (*DEPEL->Entry_Text());
	    HTML_remove_tags(entryexcerpt);
	    entryexcerpt.gsub(BRXwhite," ");
	    Elipsis_At(entryexcerpt,144);
	  } else entryexcerpt = "(Missing Text Content)";
	  //if (DEPEL->tdperiod() != pt_nonperiodic) entryexcerpt += time_stamp("(P%Y%m%d)",DEPEL->Target_Date());
 	  calendarstr += "\nCLASS:PRIVATE\nSTATUS:CONFIRMED\nSUMMARY:" + entryexcerpt + " [DIL#" + DEPEL->chars() + "]\nTRANSP:OPAQUE\nEND:VEVENT\n";
	  calendar_i++;
	}
      }
    }
  INFO << "  Included " << calendar_i << " DIL entries in calendar.ics (valuation>=" << alCTRGoogleCalendarvaluationcriterion << ",time required>=" << alCTRGoogleCalendartimecriterion << ")\n";
  calendarstr += "END:VCALENDAR\n";
  write_file_from_String(calendarfile,calendarstr,"Calendar Entries");
}

String olddatestr("YYYYmmddHHMM");

bool is_newday(String & newdatestr) {
  // newdatestr must contain a string with at least 12 characters
  
  for (int i=7; i>=0; i--)
    if (olddatestr[i]!=newdatestr[i]) {
      olddatestr=newdatestr;
      return true;
    }
  return false;
}

#define USENEWEPSMETHOD

#ifdef INCLUDE_PERIODIC_TASKS
void generate_AL_CRT(Active_List &al, int deplen, DIL_Virtual_Matrix dvm[], time_t t_current, time_t t_limit, DIL_entry *superior)
{
#else
void generate_AL_CRT(Active_List &al, int deplen, DIL_entry **dep, long req[], time_t t_current, time_t t_limit, DIL_entry *superior)
{
#endif
  // assumes valid values in the following global variables:
  //   alepshours
  //   wdworkendprefs[]
  //   alworkdaystart
  //   basedir
  //   timechunksize
  //   alcumtimereqexcerptlength
  //   htmlformmethod
  //   htmlforminterface

  progress->start();
  progress->update(0, "generate AL CRT: prepare to find EPS TDs ");

  // Set up possible stopping time
  t_ctrstop = 0; // 0 means no stop
  if (alCTRdays>0) t_ctrstop = time_add_day(t_current,alCTRdays);

#ifdef USENEWEPSMETHOD

  // PREPARE TO FIND SUGGESTED EPS TARGET DATES
  if (ctrshowall)
    INFO << "+---------- [SHOW ALL MODE] generate_AL_CRT: ----------+\nNumber of DIL Entry pointers (deplen) = " << deplen << '\n';
  time_t *epstds = new time_t[deplen];
  for (int i = 0; i < deplen; i++)
    epstds[i] = (MAXTIME_T);
  char *epsmarks = new char[deplen];
  for (int i = 0; i < deplen; i++)
    epsmarks[i] = 0;

  // PREPARE CRT TIME AVAILABILITY MAP
  progress->update(10, "generate AL CRT: prepare CRT map ");
  int time_per_week = 0;
  for (int i = 0; i < 7; i++)
    time_per_week += wdworkendprefs[i] - alworkdaystart;
  if (ctrshowall)
    INFO << "Time available per week = " << time_per_week << " seconds\n";
  int total_time_required = 0; // find the total time chunks required without periodic tasks
  for (int i = 0; ((i < deplen) && (ALCRT_continue_condition(DEPEL, t_limit))); i++)
    if ((REQEL > 0) && (DEPEL->Is_Dependency_Of(superior)) && (DEPEL->tdperiod() == pt_nonperiodic))
    {
      if ((ctrshowall) && (REQEL > 108))
        WARN << "**suspiciously large number of task chunks needed (" << REQEL << ") by <A HREF=\"" + idfile + '#' + DEPEL->str() + "\">DIL#" << DEPEL->str() << "</A>\n";
      total_time_required += REQEL;
      if (total_time_required < 0)
      {
        ERROR << "generate_AL_CRT(): Overrun of total_time_required variable. Stopping.\n";
        progress->stop();
        Exit_Now(1);
      }
    }
  if (ctrshowall)
    INFO << "Total number of task chunks needed for NON-PERIODIC tasks = " << total_time_required << '\n';
  time_per_week /= (((long)timechunksize) * 60); // total time chunks available per week
  if (ctrshowall)
    INFO << "Number of task chunks available per week = " << time_per_week << '\n';
  total_time_required /= time_per_week;         // approximate minimum number of weeks needed
  total_time_required *= (7 * alCRTmultiplier); // map size in days suggested by non-period tasks and multiplier
  if (ctrshowall)
    INFO << "Map size in days, suggested by NON-PERIODIC tasks * " << alCRTmultiplier << " = " << total_time_required << '\n';
  if ((ctrshowall) && (alperiodictasksincludedayslimit > total_time_required))
    INFO << "Adjusting map size to be at least " << alperiodictasksincludedayslimit << " days for periodic tasks\n";
  if (alperiodictasksincludedayslimit > total_time_required)
    total_time_required = alperiodictasksincludedayslimit;
  INFO << "Using a map of " << total_time_required << " days at 5 minute granularity.\n";
  VOUT.flush();
  alCRT_map map(Time(NULL), total_time_required); // a map with available and non-available work time is prepared

//     Each stage that is announced here can be a message along with a percentage.
#define OVERLAPFLAG 0x01
#define INSUFFICIENTFLAG 0x02
#define TREATGROUPABLEFLAG 0x04
#define EXACTFLAG 0x10
#define FIXEDFLAG 0x20
#define EPSGROUPMEMBERFLAG 0x40
#define PERIODICLESSTHANYEAR 0x80
  // PARSE FOR EXACT TARGET DATES
  progress->update(20, "generate AL CRT: parse exact TDs ");
  for (int i = 0; ((i < deplen) && (ALCRT_continue_condition(DEPEL, t_limit))); i++)
    if ((REQEL > 0) && (DEPEL->Is_Dependency_Of(superior)) && (DEPEL->tdexact()))
    { // non-completed, exact TD
      epsmarks[i] |= EXACTFLAG;
      if (map.reserve_exact(DEPEL, REQEL, TDEL))
        epsmarks[i] |= OVERLAPFLAG;
      if ((DEPEL->tdperiod() < pt_yearly) && (DEPEL->tdspan() == 0))
        epsmarks[i] |= PERIODICLESSTHANYEAR; // periodic unlimited
    }
  INFO << "Parsed for exact target dates.\n";
  VOUT.flush();
  if (ctrshowall)
  {
    if (map.Plot_Map_to_File(homedir + "tmp/formalizer.CTR-Map-Reserved.Exact.html"))
    {
      // *** THIS ONE IS TRICKY!
      //     I would really like a link or something similar.
      INFO << "Map of 5 minute chunk-lets reserved for EXACT target date tasks: <A HREF=\"" << homedir + "tmp/formalizer.CTR-Map-Reserved.Exact.html\">" + homedir + "tmp/formalizer.CTR-Map-Reserved.Exact.html</A>\n";
    }
    else
    {
      WARN << "***Unable to produce Map of 5 minute chunk-lets reserved for EXACT target date tasks\n";
    }
  }

  // PARSE FOR FIXED TARGET DATES
  progress->update(30, "generate AL CRT: parse fixed TDs ");
  for (int i = 0; ((i < deplen) && (ALCRT_continue_condition(DEPEL, t_limit))); i++)
    if ((REQEL > 0) && (DEPEL->Is_Dependency_Of(superior)) && (DEPEL->tdfixed()) && (!DEPEL->tdexact()))
    { // non-completed, fixed, non-exact TD
      bool isfromlocal;
      int haslocal, numpropagating;
      DEPEL->Target_Date_Info(isfromlocal, haslocal, numpropagating);
      if (!isfromlocal)
      {
        time_t origin_td;
        DIL_entry *origin = DEPEL->Propagated_Target_Date_Origin(origin_td);
        if (origin)
          if (origin->tdfixed())
            isfromlocal = true; // will not move
      }
      if (isfromlocal)
      {
        epsmarks[i] |= FIXEDFLAG;
        if (map.reserve_fixed(DEPEL, REQEL, TDEL))
          epsmarks[i] |= INSUFFICIENTFLAG;
      }
      else
      {
        epsmarks[i] |= TREATGROUPABLEFLAG;
      }
      if ((DEPEL->tdperiod() < pt_yearly) && (DEPEL->tdspan() == 0))
        epsmarks[i] |= PERIODICLESSTHANYEAR; // periodic unlimited
    }
  INFO << "Parsed for fixed target dates.\n";
  VOUT.flush();
  if (ctrshowall)
  {
    if (map.Plot_Map_to_File(homedir + "tmp/formalizer.CTR-Map-Reserved.Fixed.html"))
    {
      INFO << "Map of 5 minute chunk-lets reserved for FIXED target date tasks: <A HREF=\"" << homedir + "tmp/formalizer.CTR-Map-Reserved.Fixed.html\">" + homedir + "tmp/formalizer.CTR-Map-Reserved.Fixed.html</A>\n";
    }
    else
    {
      WARN << "***Unable to produce Map of 5 minute chunk-lets reserved for FIXED target date tasks\n";
    }
  }

  // PARSE FOR VARIABLE TARGET DATES
  progress->update(40, "generate AL CRT: parse variable TDs ");
  int group_first = -1, group_last = -1;
  time_t group_td = -1;
  //int group_req = 0;
  for (int i = 0; ((i < deplen) && (ALCRT_continue_condition(DEPEL, t_limit))); i++)
    if ((REQEL > 0) && (DEPEL->Is_Dependency_Of(superior)) && ((!DEPEL->tdfixed()) || ((epsmarks[i] & TREATGROUPABLEFLAG) != 0)))
    { // non-completed, non-fixed TD
      if (TDEL != group_td)
      { // process the EPS group and start a new one
        if (group_first >= 0)
        {
          time_t epsgroup_td = group_td;
          for (int j = group_first; j <= group_last; j++)
            if ((epsmarks[j] & EPSGROUPMEMBERFLAG) != 0)
            {
              epsgroup_td = map.reserve(JDEPEL, JREQEL);
            }
          if (epsgroup_td < 0)
          { // mostly, for tasks with MAXTIME target dates that don't fit into the map
            for (int j = group_first; j <= group_last; j++)
              epsmarks[j] |= INSUFFICIENTFLAG;
          }
          else
          {
            for (int j = group_first; j <= group_last; j++)
              epstds[j] = epsgroup_td;
            previous_group_td = epsgroup_td; // referenced in map.reserve();
          }
        }
        group_first = i;
        group_td = TDEL;
        //group_req = 0;
      }
      group_last = i; // mark in case there are non-group entries between firsts in this group and the next
      //group_req += REQEL;
      epsmarks[i] |= EPSGROUPMEMBERFLAG;
    }
  INFO << "Parsed for variable target dates.\n";
  VOUT.flush();
  if (ctrshowall)
  {
    if (map.Plot_Map_to_File(homedir + "tmp/formalizer.CTR-Map-Reserved.Variable.html"))
    {
      INFO << "Map of 5 minute chunk-lets reserved for VARIABLE target date tasks: <A HREF=\"" << homedir + "tmp/formalizer.CTR-Map-Reserved.Variable.html\">" + homedir + "tmp/formalizer.CTR-Map-Reserved.Variable.html</A>\n";
    }
    else
    {
      WARN << "***Unable to produce Map of 5 minute chunk-lets reserved for VARIABLE target date tasks\n";
    }
  }

  // CREATE CRT FORM
  // Some remaining options:
  // - I can indicate when a fixed target date is able to "auto-move" with the superior that provides the target date.
  // - I can indicate when a fixed target date may drop out of a group.
  // - I can indicate overlaps between Exact target date tasks.
  progress->update(50, "generate AL CRT: create CRT form ");
  INFO << "Cumulative Required Time DEs:\n";
  VOUT.flush();
  String cumreqstr = DEFAULTALCUMULATIVEREQHEADER;
  cumreqstr.gsub("@SUP@", al.get_supstr());
  cumreqstr.gsub("@EPSHRS@", String((double)alepshours, "%5.2f"));
  cumreqstr.gsub("@FORMMETHOD@", htmlformmethod);
  cumreqstr.gsub("@FORMACTION@", htmlforminterface);
  String dstfile(basedir + RELLISTSDIR + al.filestr() + ".ctr.html");
  String relidfile(relurl(dstfile, idfile));
  String normalskipexcludesstr;
  StringList normalskipexcludes;
  if (read_file_into_String(normalskipexcludesfile, normalskipexcludesstr))
    normalskipexcludes.split(0, normalskipexcludesstr, '\n');
  group_td = -1;
  long group_number = -1;
  String group_td_str;
  int i;
  for (i = 0; ((i < deplen) && (ALCRT_continue_condition(DEPEL, t_limit))); i++)
    if ((REQEL > 0) && (DEPEL->Is_Dependency_Of(superior)))
    { // non-completed DIL entries
      //      cout << "i=" << i << ','; cout.flush();
      DIL_Topical_List *dtl = DEPEL->Topics(0);
      //      if (dtl==NULL) { cout << "NULLdtl"; cout.flush(); }
      //bool isnewday=false;
      String daydivstr("");
      //      if (i>=18689) {cout << "CHKA-"; cout.flush();}
      if (TDEL != group_td)
      {
        group_td = TDEL;
        group_number++;
        //	if (i>=18689) {cout << "CHKB-id="<< DEPEL->str() <<"-grouptd=" << group_td << '(' << MAXTIME_T << ')'; cout.flush(); }
        group_td_str = time_stamp("%Y%m%d%H%M", group_td);
        //isnewday=is_newday(group_td_str);
        //      if (i>=18689) {cout << "CHKC-"; cout.flush();}
        if (is_newday(group_td_str))
          daydivstr = "____________<BR>";
        //      if (i>=18689) {cout << "CHKD-"; cout.flush();}
      }
      String rowreqstr;
      //      if (i>=18689) {cout << "CHK0-"; cout.flush();}
      String depstr(DEPEL->str());
      if ((epsmarks[i] & EPSGROUPMEMBERFLAG) != 0)
        rowreqstr = HTML_put_table_cell("", String(group_number));
      else
        rowreqstr = HTML_put_table_cell("", "FXD");
      rowreqstr += HTML_put_table_cell("", HTML_put_href((relidfile + '#') + depstr, "DIL#" + depstr));
      rowreqstr += HTML_put_table_cell(" WIDTH=240", HTML_put_href((relurl(dstfile, dtl->dil.file) + '#') + depstr, dtl->dil.title) + " [<A HREF=\"file:///cgi-bin/dil2al?dil2al=MEI&DILID=" + depstr + "\">edit</A>]");
      if (DEPEL->tdfixed())
        rowreqstr += HTML_put_table_cell("", daydivstr + "<B>" + group_td_str + "</B>");
      else
        rowreqstr += HTML_put_table_cell("", daydivstr + group_td_str);
      rowreqstr += HTML_put_table_cell("", String((((double)REQEL) * ((double)timechunksize)) / 60.0, "%6.2f"));
      String entryexcerptcell;
      if (DEPEL->Entry_Text())
        entryexcerptcell = (*DEPEL->Entry_Text());
      else
        entryexcerptcell = "(Missing Text Content)";
      HTML_remove_tags(entryexcerptcell);
      Elipsis_At(entryexcerptcell, alcumtimereqexcerptlength);
      entryexcerptcell = HTML_put_table_cell("", entryexcerptcell);
      //      if (i>=18689) {cout << "CHK1-"; cout.flush();}
#ifdef INCLUDE_PERIODIC_TASKS
      if (dvm[i].isvirtual)
      {
        rowreqstr += HTML_put_table_cell("", "<I>(Periodic Task)</I>");
      }
      else
      {
#endif
        if ((epsmarks[i] & EXACTFLAG) != 0)
        {
          bool chkboxchecked = (normalskipexcludes.iselement(depstr) >= 0);
          if ((epsmarks[i] & PERIODICLESSTHANYEAR) != 0)
            rowreqstr += HTML_put_table_cell("", HTML_put_form_checkbox(String("SKIPCHKBX") + depstr, "noskip", "", chkboxchecked) + "<I>(Periodic EXACT TD)</I>");
          else
            rowreqstr += HTML_put_table_cell("", "<I>(EXACT Target Date)</I>");
        }
        else
        {
          if ((epsmarks[i] & FIXEDFLAG) != 0)
          {
            bool chkboxchecked = (normalskipexcludes.iselement(depstr) >= 0);
            if ((epsmarks[i] & PERIODICLESSTHANYEAR) != 0)
              rowreqstr += HTML_put_table_cell("", HTML_put_form_checkbox(String("SKIPCHKBX") + depstr, "noskip", "", chkboxchecked) + "<I>(Periodic FIXED TD)</I>");
            else
              rowreqstr += HTML_put_table_cell("", "<I>(FIXED Target Date)</I>");
          }
          else
          {
            if (rapidscheduleupdating && (epstds[i] <= group_td))
            { // option not to update if EPS target date is earlier than original target date
              rowreqstr += HTML_put_table_cell("", HTML_put_form_checkbox(String("TDCHKBX") + depstr, "noupdate", "", true) + HTML_put_form_text(String("TD") + depstr, time_stamp("%Y%m%d%H%M", epstds[i]), 12));
            }
            else
            {
              rowreqstr += HTML_put_table_cell("", HTML_put_form_checkbox(String("TDCHKBX") + depstr, "noupdate", "", false) + HTML_put_form_text(String("TD") + depstr, time_stamp("%Y%m%d%H%M", epstds[i]), 12));
            }
          }
        }
#ifdef INCLUDE_PERIODIC_TASKS
      }
#endif
      rowreqstr += entryexcerptcell;
      cumreqstr += HTML_put_table_row("", rowreqstr) + '\n';
      //      if (i>=18689) { cout << 'E'; cout.flush(); }
    }
  INFO << "  Processed cumulative required time schedule suggestions for " << i << " of " << deplen << " DIL entries.\n";
  if (i < deplen)
    if (TDEL >= t_limit)
      INFO << "  (Target date " << time_stamp("%Y%m%d%H%M", TDEL) << " of the " << i << "th DIL entry exceeds AL time limit " << time_stamp("%Y%m%d%H%M", t_limit) << ".)\n";
  String cumreqtail(DEFAULTALCUMULATIVEREQTAIL);
  cumreqtail.gsub("@SUP@", al.get_supstr());
  cumreqtail.gsub("@ALFILE@", al.filestr());
  cumreqtail.gsub("@ALDATE@", time_stamp("%c"));
  cumreqstr += cumreqtail;
  write_file_from_String(dstfile, cumreqstr, "AL Cumulative Time Required");

  // CREATE iCAL/GOOGLE CALENDAR OUTPUT
  progress->update(90, "generate AL CRT: create iCAL output ");
#ifdef INCLUDE_PERIODIC_TASKS
  if (alCTRGoogleCalendar)
    generate_iCal_output(deplen, dvm, t_limit, superior);
#else
  if (alCTRGoogleCalendar)
    generate_iCal_output(deplen, dep, req, t_limit, superior);
#endif

  delete[] epstds;
  delete[] epsmarks;

#else // USENEWEPSMETHOD

  double cumreq = 0.0, groupreq = 0.0; int groupnum = 0;
  String cumreqstr, rowreqstr, entryexcerptcell;

  // PREPARE iCAL/GOOGLE CALENDAR
  String calendarstr; int calendar_i = 0;
  if (alCTRGoogleCalendar) calendarstr = "BEGIN:VCALENDAR\nCALSCALE:GREGORIAN\nMETHOD:PUBLISH\nX-WR-CALNAME:dil2al\nX-WR-CALDESC: This calendar was produced by dil2al during CTR generation and contains tasks that meet specific calendar listing criteria.\n";
  /* Not included now:
     X-WR-TIMEZONE:America/New_York\n
     BEGIN:VTIMEZONE\nTZID:America/New_York\nX-LIC-LOCATION:America/New_York\nBEGIN:STANDARD\nTZOFFSETFROM:-0400\n
     TZOFFSETTO:-0500\nTZNAME:EST\nDTSTART:19701025T020000\nRRULE:FREQ=YEARLY;BYMONTH=10;BYDAY=-1SU\nEND:STANDARD\n
     BEGIN:DAYLIGHT\nTZOFFSETFROM:-0500\nTZOFFSETTO:-0400\nTZNAME:EDT\nDTSTART:19700405T020000\n
     RRULE:FREQ=YEARLY;BYMONTH=4;BYDAY=1SU\nEND:DAYLIGHT\nEND:VTIMEZONE\n
  */
  double alCTRGCtc = alCTRGoogleCalendartimecriterion*3600.0;
  time_t alCTRGCextent = 0;
  if (alCTRGoogleCalendardaysextent>0) alCTRGCextent = Time(NULL)+(alCTRGoogleCalendardaysextent*86400);

  // PREPARE CRT FORM
  time_t grouptd = -1;
  INFO << "Cumulative Required Time DEs:\n"; VOUT.flush();
  cumreqstr = DEFAULTALCUMULATIVEREQHEADER;
  cumreqstr.gsub("@SUP@",al.get_supstr());
  cumreqstr.gsub("@EPSHRS@",String((double) alepshours,"%5.2f"));
  cumreqstr.gsub("@FORMMETHOD@",htmlformmethod);
  cumreqstr.gsub("@FORMACTION@",htmlforminterface);
  String dstfile(basedir+RELLISTSDIR+al.filestr()+".ctr.html");
  String relidfile(relurl(dstfile,idfile));

  // *********** OLD METHOD FOLLOWS (NEW IS NOT AN APPROXIMATION)
#ifdef INCLUDE_DAY_SPECIFIC_WORK_TIMES
  // [*** INCOMPLETE] The following is an approximation, not a correct accounting
  // of time used by exact target dates and difference between work time available
  // on different days of the week. For a better implementation, note that the
  // AL is already created when this function is called, and that the exact
  // target dates may be processed in advance in generate_AL to adjust available
  // time during each day.
  float alepshourscache = alepshours;
  time_t wdtimediff, sumtime = 0;
  time_t alepssec = ((time_t) rint(alepshours*60.0))*60;
  for (int wdi=0; wdi<7; wdi++) {
    wdtimediff = wdworkendprefs[wdi] - alworkdaystart;
    if (wdtimediff>alepssec) sumtime += alepssec;
    else sumtime += wdtimediff;
  }
  alepshours = ((float) sumtime) / (7.0*3600.0);
  INFO << "  Using approximate number of hours available for work per day " << alepshours << ".\n";
#endif

  // PROCESS CRT FOR EACH APPLICABLE DIL ENTRY
  DIL_entry * prevde = NULL; // use prevde, since dep[i-1] can point to DIL entry that does not satisfy conditionals below
  int i, prev_i = -1;
  for (i=0; ((i<deplen) && (ALCRT_continue_condition(DEPEL,t_limit))); i++) if ((REQEL>0) && (DEPEL->Is_Dependency_Of(superior))) { // non-completed DIL entries
    DIL_Topical_List * dtl = DEPEL->Topics(0);
    time_t curtd = TDEL;
    if (grouptd>=0) { // if not first row of form
      if (curtd==grouptd) { // if in same group, add @place holders@ to row

	// PREPARE A CRT FORM ROW
	rowreqstr += HTML_put_table_cell("","@GROUPTREQ@");
#ifdef INCLUDE_PERIODIC_TASKS
	if (dvm[prev_i].isvirtual) rowreqstr += HTML_put_table_cell("","<I>(Periodic Task)</I>");
	else 
#endif
	  rowreqstr += HTML_put_table_cell("",HTML_put_form_checkbox(String("TDCHKBX")+prevde->chars(),"noupdate@GROUPCHECKED@","")+HTML_put_form_text(String("TD")+prevde->chars(),"@GROUPEPSTD@",12));
      } else { // if not in same group complete group data up to previous entry (prevde)

	// CRT CALCULATIONS FOR GROUP
#ifdef INCLUDE_PERIODIC_TASKS
	generate_EPS_cells(map,grouptd,cumreqstr,rowreqstr,groupreq,cumreq,t_current,*prevde,dvm[prev_i].isvirtual);
#else
	generate_EPS_cells(map,grouptd,cumreqstr,rowreqstr,groupreq,cumreq,t_current,*prevde);
#endif
	groupnum++;
      }
      // *** optionally remove the following cell once T_{pref} is used
      //     to determine preferable shifts
      rowreqstr += entryexcerptcell;
      cumreqstr += HTML_put_table_row("",rowreqstr)+'\n';
    }

    // WRITE MORE TO A CRT FORM LINE
    grouptd = curtd;
    groupreq += (double) REQEL;
    cumreq += (double) REQEL;
    rowreqstr = HTML_put_table_cell("",String((long) groupnum))+
      HTML_put_table_cell("",HTML_put_href((relidfile+'#')+DEPEL->chars(),"DIL#" + String(DEPEL->chars())))+
      HTML_put_table_cell("",HTML_put_href((relurl(dstfile,dtl->dil.file)+'#')+DEPEL->chars(),dtl->dil.title)+" [<A HREF=\"file:///cgi-bin/dil2al?dil2al=MEI&DILID="+DEPEL->str()+"\">edit</A>]");
    if (DEPEL->tdfixed()) rowreqstr += HTML_put_table_cell("","<B>"+time_stamp("%Y%m%d%H%M",curtd)+"</B>");
    else rowreqstr += HTML_put_table_cell("",time_stamp("%Y%m%d%H%M",curtd));
    rowreqstr += HTML_put_table_cell("",String((((double) REQEL)*((double) timechunksize))/60.0,"%6.2f"))+
      HTML_put_table_cell("",String((cumreq*((double) timechunksize))/60.0,"%6.2f"));
    if (DEPEL->Entry_Text()) entryexcerptcell = (*DEPEL->Entry_Text());
    else entryexcerptcell = "(Missing Text Content)";
    HTML_remove_tags(entryexcerptcell);
    Elipsis_At(entryexcerptcell,alcumtimereqexcerptlength);

    // OUTPUT TASKS THAT MATCH CONDITIONS TO iCAL/GOOGLE CALENDAR
    if ((alCTRGoogleCalendar) && ((alCTRGCextent<1) || (TDEL<=alCTRGCextent))) {
      bool includeinlist = (DEPEL->tdspan()>0);
      if (!includeinlist) if (DEPEL->tdexact()) includeinlist=true;
      if (!includeinlist) if (DEPEL->Time_Required()>=alCTRGCtc) if (DEPEL->content) if (DEPEL->content->valuation>=alCTRGoogleCalendarvaluationcriterion) includeinlist=true;
      if (includeinlist) {
	if (DEPEL->tdexact()) calendarstr += "BEGIN:VEVENT\nDTSTART:" + time_stamp("%Y%m%dT%H%M00",DEPEL->Target_Date()-DEPEL->Time_Required()) + "\nDTEND:" + time_stamp("%Y%m%dT%H%M00",DEPEL->Target_Date());
	else calendarstr += "BEGIN:VEVENT\nDTSTART;VALUE=DATE:" + time_stamp("%Y%m%d",curtd) + "\nDTEND;VALUE=DATE:" + time_stamp("%Y%m%d",time_add_day(curtd));
	calendarstr += "\nCLASS:PRIVATE\nSTATUS:CONFIRMED\nSUMMARY:" + entryexcerptcell + " [DIL#" + DEPEL->chars() + "]\nTRANSP:OPAQUE\nEND:VEVENT\n";
	calendar_i++;
      }
    }
    entryexcerptcell = HTML_put_table_cell("",entryexcerptcell);
    prevde = DEPEL; prev_i = i;
  }

  // DISPLAY INFORMATION ABOUT CRT PROCESSING
  INFO << "  Processed cumulative required time schedule suggestions for " << i << " of " << deplen << " DIL entries.\n";
  if (i<deplen) if (TDEL>=t_limit) INFO << "  (Target date " << time_stamp("%Y%m%d%H%M",TDEL) << " of the " << i << "th DIL entry exceeds AL time limit " << time_stamp("%Y%m%d%H%M",t_limit) << ".)\n";
  if (alCTRGoogleCalendar) INFO << "  Included " << calendar_i << " DIL entries in calendar.ics (valuation>=" << alCTRGoogleCalendarvaluationcriterion << ",time required>=" << alCTRGoogleCalendartimecriterion << ")\n";

  // ************** FROM OLD METHOD
#ifdef INCLUDE_DAY_SPECIFIC_WORK_TIMES
  alepshours = alepshourscache;
#endif

  // *** I could already NOT set up new date content for fixed/exact TD entries, NOT count them into groups, already
  //     use available space for them, then call the EPS function to allocated, move, etc. remaining entries
  //     I could use a different marker to indicate if a DIL entry would break out of a group by being fixed/exact
  //     Make incremental improvements and test them!

  // PREPARE TAIL OF CRT FORM
  String cumreqtail(DEFAULTALCUMULATIVEREQTAIL);
  cumreqtail.gsub("@SUP@",al.get_supstr());
  cumreqtail.gsub("@ALFILE@",al.filestr());
  cumreqtail.gsub("@ALDATE@",time_stamp("%c"));

  // CRT CALCULATIONS FOR FINAL GROUP
#ifdef INCLUDE_PERIODIC_TASKS
  generate_EPS_cells(map,grouptd,cumreqstr,rowreqstr,groupreq,cumreq,t_current,*prevde,dvm[prev_i].isvirtual); // final call to complete grouop data for all entries
#else
  generate_EPS_cells(map,grouptd,cumreqstr,rowreqstr,groupreq,cumreq,t_current,*prevde); // final call to complete grouop data for all entries
#endif

  // COMPLETE THE CRT FORM OUTPUT
  rowreqstr += entryexcerptcell;
  cumreqstr += HTML_put_table_row("",rowreqstr)+'\n' + cumreqtail;
  write_file_from_String(dstfile,cumreqstr,"AL Cumulative Time Required");

  // COMPLETE THE iCAL/GOOGLE CALENDAR OUTPUT
  if (alCTRGoogleCalendar) {
    calendarstr += "END:VCALENDAR\n";
    write_file_from_String(calendarfile,calendarstr,"Calendar Entries");
  }

#endif // USENEWEPSMETHOD

  progress->stop();
  
}

bool generate_AL(DIL_entry ** dep, DIL_entry * superior) {
  // create AL from sorted array of DIL entries
  // if superior == NULL then the AL spans all DIL categories
  // if generatealtcs == 0, a complete AL will be computed, for
  // the completion of all DIL entries that are dependencies of
  // superior
  // if generatealmaxt > 0, the AL will be computed up to the
  // limit date specified
  // if generatealtcs > 0, the AL will be computed for that
  // number of Task Chunks
  //
  // For more details, see ~/src/dil2al/generate_AL.txt

  // check parameters
  if (!dep) {
    ERROR << "generate_AL(): Missing sorted DIL pointer.\n";
    return false;
  }
  if (!dep[0]) {
    ERROR << "generate_AL(): Empty DIL.\n";
    return false;
  }
  if (generatealtcs<0) {
    ERROR << "generate_AL(): Invalid AL interval size (" << generatealtcs << ") requested.\n";
    return false;
  }

  _EXTRA_VERBOSE_REPORT_FUNCTION();

  INFO << "AL automatic expansion is set to ";
  if (alautoexpand) INFO << "EXPAND\n"; else INFO << "STRICT\n";
  
  // prepare DIL entry data
  int deplen = dep[0]->fulllength(); // *** if you supply a custom list of sorted entries, supply the length of the list as well
  // allocate desired AL interval
  // Note: If there is ever a need for intervals beginning
  // at other times than the current time, that can be
  // accomplished easily by setting curtime to the desired
  // start of the interval.
  time_t algenhpd = ((time_t) algenregular)*3600; // seconds per day (before any expansion)
  if (!superior) algenhpd = ((time_t) algenall)*3600;
  else { // detect specific hpd for superior
    // *** (search up or down?)
    ERROR << "generate_AL(): Search for superior-specific hours-per-day parmameter not yet implemented.\n";
  }
  Get_Worked_Estimate(); // estimated time already worked on current day
#ifdef DIAGNOSTIC_OUTPUT
  INFO << "alworked = " << alworked << '\n';
#endif
  long totreq = 0;
#ifdef INCLUDE_PERIODIC_TASKS
  // create vector/matrix with pointers and extra data for all tasks + virtual tasks
  if (verbose) INFO << "Number of DIL entries without virtual copies for periodic tasks: " << deplen << '\n';
  _RESET_TIMER("generate_AL():generate_AL_prepare_periodic_tasks()");
  DIL_Virtual_Matrix * dvm = generate_AL_prepare_periodic_tasks(dep,deplen,algenhpd,superior);
  _REPORT_TIMER("generate_AL():generate_AL_prepare_periodic_tasks()");
  if (verbose) INFO << "Number of DIL entries with virtual copies for periodic tasks   : " << deplen << '\n';
#define LOCAL_CLEAN_EXIT do { delete[] dvm; return false; } while (0)
  for (int i = 0; (i<deplen); i++) totreq += dvm[i].req;
#else // INCLUDE_PERIODIC_TASKS
  long * req = DIL_Required_Task_Chunks(dep,deplen);
#define LOCAL_CLEAN_EXIT do { delete[] req; return false; } while (0)
  for (int i = 0; (i<deplen); i++) totreq += req[i];
#endif //INCLUDE_PERIODIC_TASKS
  if (verbose) INFO << "Total number of task chunks required to complete all tasks: " << totreq << '\n';
  _RESET_TIMER("generate_AL():al()");
  Active_List al(totreq,deplen,algenhpd,superior);
  _REPORT_TIMER("generate_AL():al()");
#ifdef DIAGNOSTIC_OUTPUT
  INFO << "allength = " << al.length() << '\n';
#endif
  if (al.length()<1) LOCAL_CLEAN_EXIT;
  // **no longer used** DIL_entry * prevde = NULL; // use prevde, since dep[i-1] can point to DIL entry that does not satisfy the conditionals below
  
  // distribute task chunks per DIL entry with target date within AL interval
  INFO << "Distributing task chunks per DIL entry over " << al.length() << " AL task chunks\n";
  float sumval=1.0, uval;
  time_t t_current = Time(NULL) + AL_PREPARATION_DELAY; // expected time upon completion of AL generation
  time_t t_start = al.AL_Head()->DayDate()+al.AL_Head()->DayStart(); // AL start
  time_t t_limit = al.AL_Tail()->DayDate()+al.AL_Tail()->DayMaxt(); // AL limit
  srand(t_current);
  if (superior) { // *** DIL entry specific AL
    ERROR << "generate_AL(): Actual dependency check for DIL entry specific AL not yet implemented.\n";
  }
  
  _RESET_TIMER("generate_AL():generate_AL_CRT()");
#ifdef INCLUDE_PERIODIC_TASKS
  if (alshowcumulativereq) generate_AL_CRT(al,deplen,dvm,t_current,t_limit,superior); // Create Cumulative Required Time HTML form
#else
  if (alshowcumulativereq) generate_AL_CRT(al,deplen,dep,req,t_current,t_limit,superior); // Create Cumulative Required Time HTML form
#endif
  _REPORT_TIMER("generate_AL():generate_AL_CRT()");

  int i, cnt = 0;
#ifdef INCLUDE_PERIODIC_TASKS
#define DEPEL dvm[i].dep
#define REQEL dvm[i].req
#define TDEL dvm[i].td
  // **no longer used**	#define GENERATE_AL_DEPLOOP_CONTINUE prevde = dvm[i].dep; continue
#define GENERATE_AL_DEPLOOP_CONTINUE continue
#else
#define DEPEL dep[i]
#define REQEL req[i]
#define TDEL dep[i]->Target_Date()
  // **no longer used**	#define GENERATE_AL_DEPLOOP_CONTINUE prevde = dep[i]; continue
#define GENERATE_AL_DEPLOOP_CONTINUE continue
#endif

  // AL generation for DIL entries with exact target dates (task chunks immediately precede target date)
#ifdef INCLUDE_EXACT_TARGET_DATES
  progress->start();
  progress->update(0,"Exact TD DEs:\n");
#ifdef INCLUDE_PERIODIC_TASKS
  String virtualoverlapsstr(VIRTUALOVERLAPSFILEHEADER); int virtualoverlapscnt = 0;
#endif
  String exactconflictsstr(EXACTCONFLICTSFILEHEADER); int exactconflictscnt = 0;
  // **no longer used** prevde = NULL; // use prevde, since dep[i-1] can point to DIL entry that does not satisfy the conditionals below
  _RESET_TIMER("generate_AL(): Exact TD DEs");
  for (i=0; ((i<deplen) && (TDEL<t_limit)); i++)
    if ((REQEL>0) && (DEPEL->tdexact()) && (DEPEL->Is_Dependency_Of(superior))) { // non-completed exact target date DIL entries
      progress->update((((long) i)*100)/deplen);
#ifdef ALGEN_DEBUG
      INFO << "(genAL.exact) " << i << ':' << DEPEL->chars(); VOUT.flush();
      if (DEPEL->str()==String("20070917105404.1")) algen_debug_this = true;
      else algen_debug_this = false;
#endif
      //if (DEPEL==NULL) { VOUT << "NULL"; VOUT.flush(); }
      if (TDEL<t_start) { // skip DIL entries with target date prior to current time
	// Note: addition to list of passed target date DIL entries is done in DE loop below
	if (verbose) {
	  // *** THIS ONE IS TRICKY!
	  //     I'd really like to have a working link or launch of w3m there.
	  WARN << "dil2al: Warning - EXACT Target Date of DIL entry ";
	  if (calledbyforminput) WARN << "<A HREF=\"" << idfile << '#' << DEPEL->chars() << "\">";
	  WARN << DEPEL->chars();
	  if (calledbyforminput) WARN << "</A>";
	  WARN << " precedes current time\n";
	}
	GENERATE_AL_DEPLOOP_CONTINUE;
      }
#ifdef INCLUDE_PERIODIC_TASKS
      al.Add_Target_Date(dvm[i]); // add target date references for exact target date tasks
#else
      al.Add_Target_Date(dep[i]); // add target date references for exact target date tasks
#endif
#ifdef ALGEN_DEBUG
      INFO << " TD added"; VOUT.flush();
#endif
      // NOTE: Expansion, if allowed, is done in the DE loop below.
      // ALLOCATION OF TASK CHUNKS TO TASK CHUNK TIME SLOTS
      // *** here, we should use a different allocation function in the al
      //     (let the days before the target date handle it)
      // *** warnings should also be different, namely not based on whether
      //     some task chunks are available, but whether a sufficient
      //     block of consecutive task chunks prior to the target date is available.
#ifdef INCLUDE_PERIODIC_TASKS
      REQEL = al.allocate_with_date(dvm[i]);
#else
      REQEL = al.allocate_with_date(dep[i],REQEL);
#endif
#ifdef ALGEN_DEBUG
      INFO << " & allocated\n"; VOUT.flush();
#endif
      if (REQEL>0) {
#ifdef INCLUDE_PERIODIC_TASKS
	// If the task chunk belongs to a virtual copy of a periodic task then a special warning message is added to a conflicts
	// file, and the remaining task chunks are not distributed. Otherwise, a warning is shown immediately, and the
	// remaining task chunks are distributed (see TL#200610260747.3).
	if (dvm[i].isvirtual) {
	  virtualoverlapsstr += "<LI><A HREF=\"" + idfile + '#' + DEPEL->chars() + "\">DIL#" + DEPEL->chars() + "</A>: " + time_stamp("%Y%m%d%H%M",TDEL) + ", " + String(REQEL) + " task chunks omitted.\n";
	  virtualoverlapscnt++;
	  REQEL = 0;
	} else {
#endif
	  exactconflictsstr += "<LI><A HREF=\"" + idfile + '#' + DEPEL->chars() + "\">DIL#" + DEPEL->chars() + "</A>: " + time_stamp("%Y%m%d%H%M",TDEL) + ", " + String(REQEL) + " task chunks omitted.\n";
	  exactconflictscnt++;
	  REQEL = 0;
#ifdef INCLUDE_PERIODIC_TASKS
	}
#endif
      }
      // **no longer used** prevde = DEPEL;
      if (REQEL==0) cnt++; // otherwise, it will also be handled and counteed in the DE loop below
    } // end of exact target dates for-loop and if-then
  _REPORT_TIMER("generate_AL(): Exact TD DEs");
  progress->stop();
#ifdef ALGEN_DEBUG
  INFO << "(genAL.exact) Completed pre-placement of tasks with exact target dates. Reporting exact and periodic overlaps and conflicts.\n";
#endif
#ifdef INCLUDE_PERIODIC_TASKS
  if (virtualoverlapscnt>0) {
    // *** THIS ONE IS TRICKY!
    //     I'd really like to have a working link or launch of w3m there.
    INFO << virtualoverlapscnt << " virtual task copies overlap with other exact target date tasks. Omitted task chunks are listed in ";
    if (calledbyforminput) INFO << "<A HREF=\"" << virtualoverlapsfile << "\">";
    INFO << virtualoverlapsfile;
    if (calledbyforminput) INFO << "</A>";
    INFO << ".\n";
    virtualoverlapsstr += VIRTUALOVERLAPSFILETAIL;
    write_file_from_String(virtualoverlapsfile,virtualoverlapsstr,"Overlaps with virtual task copies");
  }
#endif
  if (exactconflictscnt>0) {
    // *** THIS ONE IS TRICKY!
    //     I'd really like to have a working link or launch of w3m there.
    if (calledbyforminput) INFO << "<B>";
    INFO << exactconflictscnt << " tasks with exact target dates have potential schedule conflicts. Omitted task chunks are listed in ";
    if (calledbyforminput) INFO << "<A HREF=\"" << exactconflictsfile << "\">";
    INFO << exactconflictsfile;
    if (calledbyforminput) INFO << "</A>.</B>\n";
    else INFO << ".\n";
    exactconflictsstr += EXACTCONFLICTSFILETAIL;
    write_file_from_String(exactconflictsfile,exactconflictsstr,"Conflicts between tasks with exact target dates");
  }
#endif

  // AL generation for DIL entries with non-exact target dates (task chunks to be distributed)
  progress->start();
  progress->update(0,"DEs:\n");
  // **no longer used** prevde = NULL; // use prevde, since dep[i-1] can point to DIL entry that does not satisfy the conditionals below
  _RESET_TIMER("generate_AL(): DEs");
  for (i=0; ((i<deplen) && (TDEL<t_limit)); i++)
    if ((REQEL>0) && (DEPEL->Is_Dependency_Of(superior))) { // non-completed DIL entries
      progress->update((((long) i)*100)/deplen);
#ifdef ALGEN_DEBUG
      INFO << "(genAL.regular) " << i << ':' << DEPEL->chars() << '\n'; VOUT.flush();
#endif
#ifdef DIAGNOSTIC_OUTPUT
      INFO << DEPEL->chars() << " (" << REQEL << ") :\n";
#endif
      if (TDEL<t_start) { // skip DIL entries with target date prior to current time
#ifdef INCLUDE_PERIODIC_TASKS
	al.Add_Passed_Target_Date(dvm[i]); // add to list of passed target date DIL entries
#else
	al.Add_Passed_Target_Date(dep[i]); // add to list of passed target date DIL entries
#endif
	// *** THIS ONE IS TRICKY!
	//     I'd like to have a working link or launch w3m.
	INFO << "dil2al: Warning - Target Date of DIL entry ";
	if (calledbyforminput) INFO << "<A HREF=\"" << idfile << '#' << DEPEL->chars() << "\">";
	INFO << DEPEL->chars();
	if (calledbyforminput) INFO << "</A>";
	INFO << " precedes current time\n";
	GENERATE_AL_DEPLOOP_CONTINUE;
      }
#ifdef INCLUDE_PERIODIC_TASKS
      al.Add_Target_Date(dvm[i]); // add target date reference
#else
      al.Add_Target_Date(dep[i]); // add target date reference
#endif
      // *** simplify the below by making member functions
      //     that do things like take req[i], automatically
      //     check if expansion is allowed, do the work, and
      //     return the remainder for req[i]
      long expandn = REQEL - al.available_before(TDEL); // also sets al.deavail
      if ((expandn>0) && (alautoexpand)) {
	INFO << "Expanded for DIL entry #" << DEPEL->chars() << " (" << expandn << ")\n";
	if ((expandn -= al.expand(expandn,TDEL))!=0) {
	  // *** THIS ONE IS TRICKY!
	  //     I'd like to have a working link or launch w3m.
	  INFO << "DIL entry #";
	  if (calledbyforminput) INFO << "<A HREF=\"" << idfile << '#' << DEPEL->chars() << "\">";
	  INFO << DEPEL->chars();
	  if (calledbyforminput) INFO << "</A>";
	  INFO << " tasks cannot be completed before " << time_stamp("%Y%m%d%H%M",TDEL) << "(required: " << REQEL << ", available: " << al.deavail;
	  if (calledbyforminput) INFO << ", <A HREF=\"" << homedir << "doc/html/formalizer-doc.html#AL-update-strategies\">suggestions</A>";
	  INFO <<  ")\n";
	}
      }
      // *** currently only linear distribution function available
      // ALLOCATION OF TASK CHUNKS TO TASK CHUNK TIME SLOTS
      if ((sumval=al.set_TC_values())>0.0) {
	// randomly allocate TCs for this DIL entry
	// *** IF WE WANT TO WORK IN BIGGER BLOCKS OF CHUNKS PER TASK AS PER NEW INSIGHT
	// *** THEN THIS IS WHERE THAT SHOULD BE SELECTABLE AND NEEDS CODE MODIFICATION!
	while ((REQEL>0) && (al.deavail>0)) {
	  uval = sumval*(((float) rand()) / ((float) RAND_MAX));
#ifdef INCLUDE_PERIODIC_TASKS
	  uval = al.allocate_with_value(uval,dvm[i]);
#else
	  uval = al.allocate_with_value(uval,dep[i]);
#endif
	  if (uval>0.0) {
	    sumval -= uval;
	    REQEL--;
#ifdef DIAGNOSTIC_OUTPUT
	    PLL_LOOP_FORWARD(AL_Day,al.AL_Head(),1) {
	      INFO << '|';
	      if (e->TC_Head()) { 
		for (AL_TC * etc = e->TC_Head(); (etc); etc = etc->Next())
		  if (etc->Get_DE()) INFO << 'x'; else INFO << '_';
	      }
	    }
	    INFO << '\n';
#endif
	  }
	}
	if (REQEL>0) {
	  // *** THIS ONE IS TRICKY!
	  //     I'd like to have a working link or launch w3m.
	  INFO << "Not all task chunks of DIL entry #";
	  if (calledbyforminput) INFO << "<A HREF=\"" << idfile << '#' << DEPEL->chars() << "\">";
	  INFO << DEPEL->chars();
	  if (calledbyforminput) INFO << "</A>";
	  INFO << " could be scheduled before target date " << time_stamp("%Y%m%d%H%M",TDEL) << " (required: " << REQEL;
	  if (calledbyforminput) INFO << " ,<A HREF=\"" << homedir << "doc/html/formalizer-doc.html#AL-update-strategies\">suggestions</A>";
	  INFO << ")\n";			  
	}
      } else {
	if (sumval==0.0) {
	  // *** THIS ONE IS TRICKY!
	  //     I'd like to have a working link or launch w3m.
	  INFO << "No task chunks available for DIL entry #";
	  if (calledbyforminput) INFO << "<A HREF=\"" << idfile << '#' << DEPEL->chars() << "\">";
	  INFO << DEPEL->chars();
	  if (calledbyforminput) INFO << "</A>";
	  INFO << " before target date " << time_stamp("%Y%m%d%H%M",TDEL) << " (required: " << REQEL << ", available: " << al.deavail;
	  if (calledbyforminput) INFO << ", <A HREF=\"" << homedir << "doc/html/formalizer-doc.html#AL-update-strategies\">suggestions</A>";
	  INFO <<  ")\n";
	} else {
	  // *** THIS ONE IS TRICKY!
	  //     I'd like to have a working link or launch w3m.
	  INFO << "dil2al: Negative sum of values for DIL entry #";
	  if (calledbyforminput) INFO << "<A HREF=\"" << idfile << '#' << DEPEL->chars() << "\">";
	  INFO << DEPEL->chars();
	  if (calledbyforminput) INFO << "</A>";
	  INFO << " in generate_AL(), continuing\n";
	}
      }
      // **no longer used** prevde = DEPEL;
      cnt++;
#ifdef DEBUG
      if ((i%10)==0) { INFO << '>'; VOUT.flush(); }
#endif
    } // end of DEs for-loop and if-then
  _REPORT_TIMER("generate_AL(): DEs");
  progress->stop();

  // All AL generation iterations have been completed.
  INFO << "Scheduled DIL entries with target date in requested range: " << cnt << '\n';
#ifdef DIAGNOSTIC_OUTPUT
  INFO << "*\n";
#endif
  
  // randomly pick tasks that could not be allocated entirely before their target dates
#ifdef INCLUDE_PERIODIC_TASKS
#define DEPK dvm[k].dep
#define REQK dvm[k].req
#define TDK dvm[k].td
#else
#define DEPK dep[k]
#define REQK req[k]
#define TDK dep[k]->Target_Date()
#endif
  // *** Can put progress indicator here:
  //     reqtotal = 0; for (int k = 0; k<deplen; k++) reqtotal += REQK;
  //     progress->start();
  //     progress->update(0,"Randomly allocating remaining beyond target dates: ";
  //     inside the while-loop: progress->update((reqallocated*100)/reqtotal);
  //     inside the if(al.allocate(DEPK)): reqallocated++;
  //     after the while-loop: progress->stop();
  // *** TESTING WITHOUT THIS LINE! AL_TC * atc = al.Get_Avail_TC(0);
  WARN << "generate_AL(): ALPHA TEST Warning - Testing without al.Get_Avail_TC(0) line.\n";
  int j, k = 1; cnt = 0;
  _RESET_TIMER("generate_AL(): randomly pick tasks not before TDs");
  while (k>=0) {
    // random DIL entry
    j = (int) (((float) i)*((float) rand()) / ((float) RAND_MAX));
    if (j>=i) j=i-1;
    if (j<0) break; // i==0
    // find nearest DIL entry requiring task chunks
    // Note: current order of search gives partially
    // allocated DIL entries priority over DIL entries
    // with target dates prior to current time
    for (k = j; ((k<i) && (REQK<=0)); k++);
    if (k>=i) for (k = (j-1); ((k>=0) && (REQK<=0)); k--);
    // allocate earliest available task chunk in AL
    if (k>=0) {
      if (al.allocate(DEPK)) {
	REQK--;
#ifdef DIAGNOSTIC_OUTPUT
	PLL_LOOP_FORWARD(AL_Day,al.AL_Head(),1) {
	  EOUT << '|';
	  if (e->TC_Head()) { 
	    for (AL_TC * etc = e->TC_Head(); (etc); etc = etc->Next())
	      if (etc->Get_DE()) EOUT << 'x'; else EOUT << '_';
	  }
	}
	EOUT << '\n';
#endif
      } else break;
    }
#ifdef DEBUG
    if ((cnt%10)==0) { VOUT << '+'; VOUT.flush(); }
#endif
    cnt++;
  }
  _REPORT_TIMER("generate_AL(): randomly pick tasks not before TDs");
  INFO << "Scheduled task chunks beyond their intended target dates: " << cnt << '\n';
#ifdef DIAGNOSTIC_OUTPUT
  INFO << "*\n";
#endif
  
  // randomly pick tasks that have target dates exceeding the AL interval
  // *** Can put progress indicator here:
  //     progress->start();
  //     progress->update(0,"Randomly allocating task chunks from tasks with target dates beyond AL range: ";
  //     inside the while-loop: progress->update((k*100)/deplen);
  //     after the while-loop: progress->stop();
  int remlen = deplen - i;
  k = 0; cnt = 0;
  _RESET_TIMER("generate_AL(): randomly pick tasks TDs exceeding AL");
  while (k<deplen) {
    // random DIL entry
    j = (int) (((float) remlen)*((float) rand()) / ((float) RAND_MAX));
    if (j>=remlen) j=remlen-1;
    if (j<0) break;
    j += i;
    // find nearest DIL entry requiring task chunks
    for (k = j; ((k>=i) && (REQK<=0)); k--);
    if (k<i) for (k = (j+1); ((k<deplen) && (REQK<=0)); k++);
    // allocate earliest available task chunk in AL
    if (k<deplen) {
      if (al.allocate(DEPK)) {
	REQK--;
#ifdef DIAGNOSTIC_OUTPUT
	PLL_LOOP_FORWARD(AL_Day,al.AL_Head(),1) {
	  INFO << '|';
	  if (e->TC_Head()) { 
	    for (AL_TC * etc = e->TC_Head(); (etc); etc = etc->Next())
	      if (etc->Get_DE()) INFO << 'x'; else INFO << '_';
	  }
	}
	INFO << '\n';
#endif
      } else break;
    }
#ifdef DEBUG
    if ((cnt%10)==0) { INFO << '"'; VOUT.flush(); }
#endif
  }
  _REPORT_TIMER("generate_AL(): randomly pick tasks TDs exceeding AL");
  INFO << "Scheduled tasks for DIL entries with target dates beyond requested range: " << cnt << '\n';
  
  // clean up by removing unused TCs
#ifdef DIAGNOSTIC_OUTPUT
  PLL_LOOP_FORWARD(AL_Day,al.AL_Head(),1) for (AL_TC * etc = e->TC_Head(); (etc); etc=etc->Next()) if (!etc->Get_DE()) cerr << '?'; cerr << '\n';
#endif
#ifdef INCLUDE_PERIODIC_TASKS
  al.remove_unused_and_periodic_only_TCs();
#else
  al.remove_unused_TCs();
#endif
  
  // create visible AL
  _RESET_TIMER("generate_AL(): al.generate_focused_AL()");
  if (!al.generate_focused_AL(superior,algenhpd)) LOCAL_CLEAN_EXIT;
  _REPORT_TIMER("generate_AL(): al.generate_focused_AL()");
  _RESET_TIMER("generate_AL(): al.generate_wide_AL()");
  if (!al.generate_wide_AL(superior)) LOCAL_CLEAN_EXIT;
  _REPORT_TIMER("generate_AL(): al.generate_wide_AL()");
  
  // update DIL, AL, TL main page
  _RESET_TIMER("generate_AL(): update_main_ALs()");
  if (!update_main_ALs(superior)) LOCAL_CLEAN_EXIT;
  _REPORT_TIMER("generate_AL(): update_main_ALs()");
  
  // clean up
#ifdef INCLUDE_PERIODIC_TASKS
  delete[] dvm;
#else
  delete[] req;
#endif
  
  return true;
}

int get_AL_entry(String & alstr, int alpos, String & rowcontent, int & rowstart, int & rowend) {
  // gets an entry from an AL table and returns whether that row
  // is a Day (0) or a TC (1), returns -1 if row cannot be obtained
  
  String params, cellcontent;
  if ((rowend=HTML_get_table_row(alstr,alpos,params,rowcontent,&rowstart))<0) return -1;
  if (HTML_get_table_cell(rowcontent,0,params,cellcontent)<0) return -1;
  if (cellcontent.contains(BigRegex("[<]!--[ 	]+@select_TL_DIL_refs:[ 	]+SKIP ROW@[ 	]+--[>][^(]*Day"))) return 0;
  return 1;
}

bool remove_AL_TC(String alref, String dilid) {
  // removes the top task chunk if it has DIL entry ID dilid
  // from the AL indicated by alref and updates the suggested
  // start time of that AL day according to the assumed time
  // taken to do the task chunk (desynchronization is noticed
  // and taken care of in select_TL_DIL_refs())
  // day rows at the top of the AL that are not followed by task
  // rows are removed
  // returns false if dilid did not match the DIL entry ID of
  // the top task chunk, or if the visible focused AL became
  // empty

  _EXTRA_VERBOSE_REPORT_FUNCTION();
  
  String alstr, newalstr;
  if (!read_file_into_String(alref,alstr)) return false;
  newalstr.alloc(alstr.length());

  // here you have to skip to the right part of the AL page
  int alpos = alstr.index("<!-- @Focused-AL Days+TCs START@ -->");
  if (alpos<0) {
    ERROR << "remove_AL_TC(): Missing @Focused-AL Days+TCs START@ tag in text\nfrom " << alref << ".\n";
    ERROR << String(alstr.before(10000));
    return false;
  }
  
  // get day row and remove empty days at top of AL
  // and find top TC to compare with dilid
  StringList alentries; int numentries = 0, tcsfound = 0, tcentries[2], rowstart, rowend;
  bool no_iteration_yet = true;
  while (tcsfound<2) {
    switch (get_AL_entry(alstr,alpos,alentries[numentries],rowstart,rowend)) {
    case -1:
      // *** could detect from numentries and tcsfound
      //     to give a different VOUT message if the
      //     visible AL portion contained no more TCs
      ERROR << "remove_AL_TC(): Missing data in AL " << alref << " after position " << alpos << ".\n";
      return false;
      break;
    case 0: break;
    case 1:
      if (numentries==0) {
	ERROR << "remove_AL_TC(): TC without preceding Day ID in " << alref << " after position " << alpos << ".\n";
	return false;
      }
      if (tcsfound==0) {
	String alhrefurl, alhreftext;
	if (HTML_get_href(alentries[numentries],0,alhrefurl,alhreftext)<0) {
	  ERROR << "remove_AL_TC(): Missing DIL ID reference in " << alref << " after position " << alpos << ".\n";
	  return false;
	}
	if (alhreftext!=dilid) { // different task
	  INFO << "Top of AL " << alref << " does not match DIL entry " << dilid << '\n';
	  return false;
	}
      }
      tcentries[tcsfound] = numentries;
      tcsfound++;
      break;
    }
    if (no_iteration_yet) {
      // copy head of AL
      newalstr = alstr.at(0,rowstart);
      no_iteration_yet = false;
    }
    numentries++; alpos = rowend;
  }
  if (tcentries[1]==(tcentries[0]+1)) { // next TC on same day, modify daystart
    BigRegex rds("(suggested start[ 	]+\\([0-9][0-9]:[0-9][0-9]\\)");
    int didx = tcentries[0]-1, dstimepos;
    if ((dstimepos=alentries[didx].index(rds))<0) {
      ERROR << "remove_AL_TC(): Missing suggested start in AL " << alref << " at row " << didx << ".\n";
      return false;
    }
    String daystartstr = alentries[didx].at(rds.subpos(1),rds.sublen(1));
    daystartstr.del(':');
    time_t tds = time_stamp_time_of_day(daystartstr) + ((time_t) (timechunksize*60));
    if (tds>=SECONDSPERDAY) {
      INFO << "Updated suggested start time exceeds length of day, limiting to 23:59.\n";
      tds = SECONDSPERDAY-1;
    }
    daystartstr = time_stamp_GM("%H:%M",tds);
    for (int i = 0; i<5; i++) alentries[didx][rds.subpos(1)+i] = daystartstr[i];
  }
  
  // determine day before second TC and write it
  int didx = tcentries[1]-1;
  if (didx==tcentries[0]) didx--;
  newalstr += HTML_put_table_row(DEFAULTALDAYEMPHASIS,alentries[didx]) + '\n';
  // write second TC with top TC colors
  newalstr += HTML_put_table_row(DEFAULTALFOCUSEDEMPHASIS,alentries[tcentries[1]]) + '\n';
  // copy remainder of AL
  newalstr += alstr.at(alpos,alstr.length()-alpos);
  
  return write_file_from_String(alref,newalstr,"AL");
}

bool update_DIL_to_AL() {
  // generate an updated AL from the DIL hierarchy
  // uses generatealmaxt and generatealtcs
  
  /*
    When computing priorities, also parse for ALs and refresh AL parameters
    for all detail items if prefixed with E (``automatic estimate'').
  */
  // *** can make this first part prepare in the background
  //     before a TC is completed
  
  _RESET_TIMER("update_DIL_to_AL():dilist");
  Detailed_Items_List dilist;
  _REPORT_TIMER("update_DIL_to_AL():dilist");
  _RESET_TIMER("update_DIL_to_AL():dilist.Sort_by_Target_Date()");
  DIL_entry ** dep = dilist.Sort_by_Target_Date(true);
  _REPORT_TIMER("update_DIL_to_AL():dilist.Sort_by_Target_Date()");
#ifdef DEBUG
  if (dep) {
    int entrynum = dep[0]->fulllength();
    for (int i = 0; i<entrynum; i++) cerr << dep[i]->chars() << '\n';
  }
#endif
  if (!generate_AL(dep)) {
    delete[] dep;
    return false;
  }
  delete[] dep; // may not be necessary here
  return true;
}

bool refresh_quick_load_cache() {
  if (!usequickloadcache) {
    WARN << "refresh_quick_load_cache(): The usequickloadcache flag is not set.\n";
    return false;
  }
  Detailed_Items_List dilist;
  if (dilist.Get_All_DIL_ID_File_Parameters()<0) return false;
  if (dilist.Get_All_Topical_DIL_Parameters(true)<0) return false;
  return true;
}

#ifdef INCLUDE_POOL_PLANNING_METHOD

/*
1. create a simple pool
2. create a simple ordering list
3. enable ordering from the pool
4. improve the selection of categorizing in the pool
5. do the actual plan-led rescheduling, with possible program improvements on the way
 */

Detailed_Items_List * oopdilist = NULL;
pool * inserts = NULL;
ooplanner * oopr = NULL;

bool pool::write_data(String poolname) {
  // This writes a binary data file containing the pool entries.
  
  unsigned long numentries = length();
  ofstream pfl(poolfile+poolname+".data",ios::binary);
  if (!pfl) {
    ERROR << "create_pool_all_pending(): Unable to write to " << poolfile << poolname << ".data.\n";
    return false;
  } else {
    pfl.write((const char *) (&numentries),sizeof(numentries)); // number of entries
    if (numentries>0) {
      pool_entry_file_data * pefd = new pool_entry_file_data[numentries]; // *** Alternatively, write to file one by one.
      int i = 0;
      PLL_LOOP_FORWARD(pool_entry,head(),1) {
	pefd[i] = e->File_Data();
	i++;
      }
      pfl.write((const char *) pefd, numentries*sizeof(pool_entry_file_data));
      delete[] pefd;
    }
    pfl.close();
  }
  return true;
}

bool pool::read_data(String poolname, bool warnopen) {
  // This reads a binary data file containing the pool entries.
  
  unsigned long numentries = 0;
  ifstream pfl(poolfile+poolname+".data",ios::binary);
  if (!pfl) {
    if (warnopen) WARN << "pool::read_data(): Unable to read from " << poolfile << poolname << ".data.\n";
    return false;
  } else {
    if ((pfl.read((READSOMETYPE) (&numentries), sizeof(numentries))).gcount() < (streamsize) sizeof(numentries)) return false; // number of entries
    if (numentries>0) {
      pool_entry_file_data * pefd = new pool_entry_file_data[numentries]; // *** Alternatively, read from file one by one.
      if ((pfl.read((READSOMETYPE) pefd, numentries*sizeof(pool_entry_file_data))).gcount() < (streamsize) (numentries*sizeof(pool_entry_file_data))) return false;
      for (unsigned int i = 0; i<numentries; i++) append(pefd[i]);
      delete[] pefd;
    }
    pfl.close();
  }
  return true;
}

void pool::match_DIL_entries() {
  if (!oopdilist) {
    oopdilist = new Detailed_Items_List();
    oopdilist->Get_All_Topical_DIL_Parameters(true);
  }
  if (oopdilist->list.head()) {
    PLL_LOOP_FORWARD(pool_entry,head(),1) if (!e->de) {
      e->de = oopdilist->list.head()->elbyid(e->id);
      if (!e->de) {
	ERROR << "pool::match_DIL_entries(): Unable to find DIL entry with ID " << e->id.str() << ".\n Stopping to avoid references to NULL.\n";
	Exit_Now(1);
      }
    }
  } else {
    ERROR << "pool::match_DIL_entries(): Unable to match DIL entries to ID.\nStopping to avoid references to NULL.\n";
    Exit_Now(1);
  }
}

void pool::write_HTML_FORM(String poolname, String pooldescription, const String & poolcomment) {
  // This turns any selection of pool entries into an HTML FORM that can be used for objective-oriented planning.
  
  String poolstr("<HTML>\n<HEAD>\n<TITLE>Pool: "+pooldescription+"</TITLE>\n</HEAD>\n<BODY>\n\n<H3>Pool: "+pooldescription+"</H3>\n\n"+poolcomment+"\n<P>\n<FORM METHOD=\"GET\" ACTION=\"/cgi-bin/dil2al\"><INPUT type=\"hidden\" name=\"dil2al\" value=\"Pp\">\n<TABLE>\n"); // [***NOTE] Instead, I could append each to file separately.
  PLL_LOOP_FORWARD(pool_entry,head(),1) {
    if (!e->de) match_DIL_entries(); // Get DIL information for a pool generated without it
    bool tagged = false;
    if (inserts) if (inserts->head()) tagged = (inserts->elbyid(*(e->de))!=NULL);
    poolstr += "<TR VALIGN=top><TD><INPUT type=checkbox name=\""+e->de->str()+"\" value=tagged";
    if (tagged) poolstr += " checked";
    poolstr += "></TD><TD><A HREF=\"/cgi-bin/dil2al?dil2al=P";
    if (tagged) poolstr += 'X';
    else poolstr += 'I';
    poolstr += e->de->str()+"\">";
    if (tagged) poolstr += "EXCLUDE";
    else poolstr += "include";
    poolstr += "</A></TD><TD>[<A HREF=\""+idfile+'#'+e->de->str()+"\">" + e->de->str() + "</A>]</TD><TD>";
    String * s = e->de->Entry_Text();
    String entrytext;
    if (s) {
      entrytext = *s;
      HTML_remove_tags(entrytext);
      Elipsis_At(entrytext,160);
      poolstr += entrytext;
    }
    poolstr += "</TD><TD>" + String(((double) e->de->Time_Required())/3600.0,"%4.2f");
    poolstr += "</TD><TD>" + time_stamp("%Y%m%d%H%M",e->de->Target_Date());
    poolstr += "</TD></TR>\n";
  }
  poolstr += "</TABLE>\n<INPUT type=\"submit\" value=\"Include\"> <INPUT type=\"reset\"></FORM>\n";
  poolstr += "<P>\n[<A HREF=\"/cgi-bin/dil2al?dil2al=PU"+poolname+"\">Update</A>]\n<P>\n<A HREF=\""+ooplannerfile+"html\">Objective-Oriented Planner</A></BODY>\n</HTML>\n";
  
  write_file_from_String(poolfile+poolname+".html",poolstr,pooldescription+" Pool");
}

long create_pool_all_pending(Detailed_Items_List & dilist) {
  const String comment("This pool contains all DIL entries that have pending completion values [0,1).");
  // 1. Select entries
  pool per;
  unsigned long res = 0;
  PLL_LOOP_FORWARD(DIL_entry,dilist.list.head(),1) if ((e->Completion_State()<1.0) && (e->Completion_State()>=0.0)) {
    if (oopr) if (oopr->elbyid(*e)) continue; // not entries already in the Objective-Oriented Planner
    pool_entry * pe = new pool_entry(*e);
    per.link_before(pe);
    pe->de->Set_Semaphore(pe->de->Get_Semaphore()+1);
    res++;
  }
  // 2. Create the data file
  if (!per.write_data("all_pending")) return -1;
  // 3. Create the pool form
  per.write_HTML_FORM("all_pending","All Pending",comment);
  per.clear();
  return res;
}

pool_entry * pool::elbyid(DIL_ID & _id) {
  // Returns the address of the pool_entry with the ID _id if found,
  // otherwise returns NULL.
  
  PLL_LOOP_FORWARD(pool_entry,head(),1) if (e->de) {
    if (*(e->de)==_id) return e;
  } else {
    if (e->id==_id) return e;
  }
  return NULL;
}

int sort_entrynum = 0;

DIL_entry ** Prepare_Sorting(Detailed_Items_List & dilist, bool randomize_within) {
  // This sets up the array and global variables needed for sorting.
  
  int i = 0;
  if (!dilist.list.head()) sort_entrynum=dilist.Get_All_Topical_DIL_Parameters();
  else sort_entrynum=dilist.list.length();
  if (sort_entrynum<1) {
    ERROR << "Prepare_Sorting(): No DIL to sort.\n";
    return NULL;
  }
  DIL_entry_ptr * dep = new DIL_entry_ptr[sort_entrynum];
  PLL_LOOP_FORWARD(DIL_entry,dilist.list.head(),(i<sort_entrynum)) { dep[i] = e; i++; }
  if (randomize_within) { // Randomized the array in advance, so that equal value items are encountered in a random order
    if (sort_entrynum<RAND_MAX) {
      for (long j = (2*sort_entrynum)-1; j>=0; j--) {
	register long index1 = (long) ((((double) sort_entrynum)*((double) rand()))/((double) RAND_MAX));
	if (index1>=sort_entrynum) index1=sort_entrynum-1;
	register long index2 = (long) ((((double) sort_entrynum)*((double) rand()))/((double) RAND_MAX));
	if (index2>=sort_entrynum) index2=sort_entrynum-1;
	register DIL_entry_ptr tmp = dep[index1];
	dep[index1] = dep[index2];
	dep[index2] = tmp;
      }
    } else {
      ERROR << "Prepare_Sorting(): Randomizing of more than " << RAND_MAX << " entries is not yet supported.\n";
      Exit_Now(-1);
    }
  }
  return dep;
}

int DIL_entry_time_required_qsort_compare(const void * vde1, const void * vde2) {
  // used with qsort() (<stdlib.h>)
  //
  // NOTE: ISO C++ is strict about providing the correct function
  // declaration in cases such as the call to qsort(). This function
  // was therefore rewritten from the original declaration,
  // DIL_entry_target_date_qsort_compare(DIL_entry ** de1, DIL_entry ** de2)
  // - please keep in mind that the elements are of type DIL_entry **,
  // i.e. arrays/pointers of pointers to objects.
  
  DIL_entry ** de1 = (DIL_entry **) vde1;
  DIL_entry ** de2 = (DIL_entry **) vde2;
  time_t t1 = 0, t2 = 0;
  if (*de1) t1 = (*de1)->Time_Required();
  if (*de2) t2 = (*de2)->Time_Required();
  if (t1==t2) return 0;
  if (t1<t2) return -1;
  return 1;
}

long create_pool_time_required(Detailed_Items_List & dilist) {
  const String comment("This pool contains DIL entries that have not been completed and that have required time greater than some threshold value, sorted in descending order of the amount of time required (without subtracting the amount completed, so that the amount of time required can be regarded as a sort of significance parameter). Periodic tasks and tasks without a target date (and therefore considered less urgent) are excluded.");
#define TIMEREQGREATERTHAN 3600
  pool per;
  unsigned long res = 0;
  DIL_entry ** dep = Prepare_Sorting(dilist,true);
  qsort(dep,sort_entrynum,sizeof(DIL_entry_ptr),DIL_entry_time_required_qsort_compare);
  long deplen = sort_entrynum;
  for (long i = (deplen-1); i>=0; i--) if ((dep[i]->Completion_State()<1.0) && (dep[i]->Completion_State()>=0.0) && (dep[i]->Time_Required()>TIMEREQGREATERTHAN) && (dep[i]->tdperiod()==pt_nonperiodic) && (dep[i]->Target_Date()!=(MAXTIME_T))) {
    if (oopr) if (oopr->elbyid(*(dep[i]))) continue; // not entries already in the Objective-Oriented Planner
    pool_entry * pe = new pool_entry(*(dep[i]));
    per.link_before(pe);
    pe->de->Set_Semaphore(1);
    res++; 
  }
  if (!per.write_data("time_required")) return -1;
  per.write_HTML_FORM("time_required","Time Required (min. >1 hour)",comment);
  per.clear();
  delete[] dep;
  return res;
}

long create_pool_no_target_date(Detailed_Items_List & dilist) {
  const String comment("This pool contains DIL entries that have not been completed and that have no target date, sorted in descending order of the amount of time required. Periodic tasks are also excluded.");
  pool per;
  unsigned long res = 0;
  DIL_entry ** dep = Prepare_Sorting(dilist,true);
  qsort(dep,sort_entrynum,sizeof(DIL_entry_ptr),DIL_entry_time_required_qsort_compare);
  long deplen = sort_entrynum;
  for (long i = (deplen-1); i>=0; i--) if ((dep[i]->Completion_State()<1.0) && (dep[i]->Completion_State()>=0.0) && (dep[i]->tdperiod()==pt_nonperiodic) && (dep[i]->Target_Date()==(MAXTIME_T))) {
    if (oopr) if (oopr->elbyid(*(dep[i]))) continue; // not entries already in the Objective-Oriented Planner
    pool_entry * pe = new pool_entry(*(dep[i]));
    per.link_before(pe);
    pe->de->Set_Semaphore(1);
    res++; 
  }
  if (!per.write_data("no_target_date")) return -1;
  per.write_HTML_FORM("no_target_date","No Target Date",comment);
  per.clear();
  delete[] dep;
  return res;
}

long create_pool_publication(Detailed_Items_List & dilist) {
  const String comment("This pool contains DIL entries that have not been completed, that have required time greater than two hours, and that belong to one of the publication topics (publication, journal, slide, poster).");
  pool per;
  unsigned long res = 0;
  DIL_entry ** dep = Prepare_Sorting(dilist,true);
  qsort(dep,sort_entrynum,sizeof(DIL_entry_ptr),DIL_entry_target_date_qsort_compare);
  long deplen = sort_entrynum;
  for (long i = 0; i<deplen; i++) if ((dep[i]->Completion_State()<1.0) && (dep[i]->Completion_State()>=0.0) && (dep[i]->Time_Required()>7200) && ((dep[i]->Is_in_Topic("publication.html")) || (dep[i]->Is_in_Topic("journal.html")) || (dep[i]->Is_in_Topic("presentation.html")) || (dep[i]->Is_in_Topic("poster.html")))) {
    if (oopr) if (oopr->elbyid(*(dep[i]))) continue; // not entries already in the Objective-Oriented Planner
    pool_entry * pe = new pool_entry(*(dep[i]));
    per.link_before(pe);
    pe->de->Set_Semaphore(1);
    res++; 
  }
  if (!per.write_data("publication")) return -1;
  per.write_HTML_FORM("publication","Publication (min. >2 hours)",comment);
  per.clear();
  delete[] dep;
  return res;
}

long create_pool_near_term(Detailed_Items_List & dilist) {
  const String comment("This pool contains DIL entries that have not been completed, and that have target dates in the near term. Periodic tasks are also excluded.");
#define NEARTERM_T (14*86400)
  time_t t_threshold = Time(NULL) + NEARTERM_T;
  pool per;
  unsigned long res = 0;
  DIL_entry ** dep = Prepare_Sorting(dilist,true);
  qsort(dep,sort_entrynum,sizeof(DIL_entry_ptr),DIL_entry_target_date_qsort_compare);
  long deplen = sort_entrynum;
  for (long i = 0; i<deplen; i++) if ((dep[i]->Completion_State()<1.0) && (dep[i]->Completion_State()>=0.0) && (dep[i]->tdperiod()==pt_nonperiodic) && (dep[i]->Target_Date()<t_threshold)) {
    if (oopr) if (oopr->elbyid(*(dep[i]))) continue; // not entries already in the Objective-Oriented Planner
    pool_entry * pe = new pool_entry(*(dep[i]));
    per.link_before(pe);
    pe->de->Set_Semaphore(1);
    res++; 
  }
  if (!per.write_data("near_term")) return -1;
  per.write_HTML_FORM("near_term","Near Term (2 weeks)",comment);
  per.clear();
  delete[] dep;
  return res;
}

long create_pool_GDDM(Detailed_Items_List & dilist) {
  const String comment("This pool contains DIL entries that have not been completed, and that are Objectives or Goals in Goal Directed Decision Making.");
  pool per;
  unsigned long res = 0;
  PLL_LOOP_FORWARD(DIL_entry,dilist.list.head(),1) if ((e->Completion_State()<1.0) && (e->Completion_State()>=0.0)) {
    int planentrytype = e->Is_Plan_Entry();
    if ((planentrytype==PLAN_ENTRY_TYPE_OBJECTIVE) || (planentrytype==PLAN_ENTRY_TYPE_GOAL)) {
      if (oopr) if (oopr->elbyid(*e)) continue; // not entries already in the Objective-Oriented Planner
      pool_entry * pe = new pool_entry(*e);
      per.link_before(pe);
      pe->de->Set_Semaphore(pe->de->Get_Semaphore()+1);
      res++;
    }
  }
  if (!per.write_data("GDDM")) return -1;
  per.write_HTML_FORM("GDDM","Goal Directed Decision Making",comment);
  per.clear();
  return res;
}

long create_pool_remaining(Detailed_Items_List & dilist) {
  const String comment("This pool contains all DIL entries that were not allocated to other pools. The DIL entries are presented in the order of their current target dates.");
  DIL_entry ** dep = dilist.Sort_by_Target_Date();
  long deplen = dilist.list.length();
  pool per;
  unsigned long res = 0;
  for (long i = 0; i<deplen; i++) if ((dep[i]->Completion_State()<1.0) && (dep[i]->Completion_State()>=0.0) && (dep[i]->Get_Semaphore()==0)) {
    if (oopr) if (oopr->elbyid(*(dep[i]))) continue; // not entries already in the Objective-Oriented Planner
    pool_entry * pe = new pool_entry(*(dep[i]));
    per.link_before(pe);
    pe->de->Set_Semaphore(1);
    res++; 
  }
  if (!per.write_data("other")) return -1;
  per.write_HTML_FORM("other","Other",comment);
  per.clear();
  delete[] dep;
  return res;
}

void ooplanner::match_DIL_entries() {
  if (!oopdilist) {
    oopdilist = new Detailed_Items_List();
    oopdilist->Get_All_Topical_DIL_Parameters(true);
  }
  if (oopdilist->list.head()) {
    PLL_LOOP_FORWARD(ooplanner_entry,head(),1) if ((!e->de) && (e->id.valid())) {
      e->de = oopdilist->list.head()->elbyid(e->id);
      if (!e->de) {
	ERROR << "ooplanner::match_DIL_entries(): Unable to find DIL entry with ID " << e->id.str() << ".\nStopping to avoid references to NULL.\n";
	Exit_Now(1);
      }
    }
  } else {
    ERROR << "ooplanner::match_DIL_entries(): Unable to match DIL entries to IDs.\nStopping to avoid references to NULL.\n";
    Exit_Now(1);
  }
}

void ooplanner::HTML_FORM_content(String & ooplannerstr) {
  ooplannerstr += "<H3>Objective-Oriented Planner</H3>\n\n<FORM METHOD=\"GET\" ACTION=\"/cgi-bin/dil2al\"><INPUT type=\"hidden\" name=\"dil2al\" value=\"Po\">\n<TABLE>\n<TR VALIGN=top><TD><INPUT type=checkbox name=\"TAGtop\" value=tagged></TD><TD><A HREF=\"/cgi-bin/dil2al?dil2al=Pitop\">import</A></TD><TD ALIGN=right>(Write-in:)</TD><TD><INPUT type=text name=\"WItop\"></TD><TD>&nbsp</TD><TD>&nbsp</TD></TR>\n";
  long oopitemnum = 0;
  PLL_LOOP_FORWARD(ooplanner_entry,head(),1) {
    //VOUT << oopitemnum << ':' << e->id << '\n';
    if ((!e->de) && (e->id.valid())) match_DIL_entries();
    if (e->de) {
      ooplannerstr += "<TR VALIGN=top><TD><INPUT type=checkbox name=\"TAG"+String(oopitemnum)+"\" value=tagged></TD><TD><A HREF=\"/cgi-bin/dil2al?dil2al=Pd"+String(oopitemnum)+"\">delete</A></TD><TD>[<A HREF=\""+idfile+'#'+e->de->str()+"\">" + e->de->str() + "</A>]</TD><TD>";
      String * s = e->de->Entry_Text();
      String entrytext;
      if (s) {
	entrytext = *s;
	HTML_remove_tags(entrytext);
	Elipsis_At(entrytext,160);
	ooplannerstr += entrytext;
      }
      ooplannerstr += "</TD><TD>";
      if (e->targetdate>=0) ooplannerstr += time_stamp("%Y%m%d%H%M",e->targetdate);
      else ooplannerstr += "(no target date)";
      ooplannerstr += " <INPUT type=text name=\"TD"+String(oopitemnum)+"\">";
      ooplannerstr += "</TD><TD>" + String(((double) e->de->Time_Required())/3600.0,"%4.2f");
      ooplannerstr += "</TD><TD>" + time_stamp("%Y%m%d%H%M",e->de->Target_Date());
      ooplannerstr += "</TD></TR>\n";
    } else { // A write-in
      ooplannerstr += "<TR VALIGN=top><TD><INPUT type=checkbox name=\"TAG"+String(oopitemnum)+"\" value=tagged></TD><TD><A HREF=\"/cgi-bin/dil2al?dil2al=Pd"+String(oopitemnum)+"\">delete</A></TD><TD><INPUT type=text name=\"WI"+String(oopitemnum)+"\"></TD><TD>"+e->writein+"</TD><TD>";
      if (e->targetdate>=0) ooplannerstr += time_stamp("%Y%m%d%H%M",e->targetdate);
      else ooplannerstr += "(no target date)";
      ooplannerstr += " <INPUT type=text name=\"TD"+String(oopitemnum)+"\">";
      ooplannerstr += "</TD><TD>&nbsp</TD><TD>&nbsp</TD></TR>\n";
    }
    ooplannerstr += "<TR VALIGN=top><TD><INPUT type=checkbox name=\"TAGafter"+String(oopitemnum)+"\" value=tagged></TD><TD><A HREF=\"/cgi-bin/dil2al?dil2al=Piafter"+String(oopitemnum)+"\">import</A></TD><TD ALIGN=right>(Write-in:)</TD><TD><INPUT type=text name=\"WIafter"+String(oopitemnum)+"\"></TD><TD>&nbsp</TD><TD>&nbsp</TD></TR>\n";
    oopitemnum++;
  }
  ooplannerstr += "</TABLE>\n<INPUT type=\"submit\" value=\"Update\"> <INPUT type=\"reset\"></FORM>\n<P>\n";
  ooplannerstr += "&nbsp;<P>\n<B>[<A HREF=\"/cgi-bin/dil2al?dil2al=P\">New</A>]</B>\n<P>\n\
<TABLE>\n\
<TR><TD><A HREF=\""+poolfile+"time_required.html\">Pool: Time Required</A></TD><TD>[<A HREF=\"/cgi-bin/dil2al?dil2al=PUtime_required\">Update pool: Time Required</A>]</TD></TR>\n\
<TR><TD><A HREF=\""+poolfile+"no_target_date.html\">Pool: No Target Date</A></TD><TD>[<A HREF=\"/cgi-bin/dil2al?dil2al=PUno_target_date\">Update pool: No Target Date</A>]</TD></TR>\n\
<TR><TD><A HREF=\""+poolfile+"publication.html\">Pool: Publication</A></TD><TD>[<A HREF=\"/cgi-bin/dil2al?dil2al=PUpublication\">Update pool: Publication</A>]</TD></TR>\n\
<TR><TD><A HREF=\""+poolfile+"near_term.html\">Pool: Near Term</A></TD><TD>[<A HREF=\"/cgi-bin/dil2al?dil2al=PUnear_term\">Update pool: Near Term</A>]</TD></TR>\n\
<TR><TD><A HREF=\""+poolfile+"GDDM.html\">Pool: Goal Directed Decision Making</A></TD><TD>[<A HREF=\"/cgi-bin/dil2al?dil2al=PUGDDM\">Update pool: Goal Directed Decision Making</A>]</TD></TR>\n\
<TR><TD><A HREF=\""+poolfile+"other.html\">Pool: Other</A></TD><TD>[<A HREF=\"/cgi-bin/dil2al?dil2al=PUother\">Update pool: Other</A>]</TD></TR>\n\
</TABLE>\n\
<P>\n<A HREF=\""+basedir+"planning/objective-oriented-planner.html\">Using the Objective-Oriented Planner</A>\n";
}

void ooplanner::write_HTML_FORM() {
  String ooplannerstr("<HTML>\n<HEAD>\n<TITLE>Objective-Oriented Planner</TITLE>\n</HEAD>\n<BODY>\n\n");
  HTML_FORM_content(ooplannerstr);
  ooplannerstr += "</BODY>\n</HTML>\n";
  write_file_from_String(ooplannerfile+"html",ooplannerstr,"Objective-Oriented Planner");
}

bool ooplanner::write_data() {
  // This writes a binary data file containing the objective-oriented planning entries.
  
  unsigned long numentries = length();
  ofstream pfl(ooplannerfile+"data",ios::binary);
  if (!pfl) {
    ERROR << "ooplanner::write_data(): Unable to write to " << ooplannerfile << "data.\n";
    return false;
  } else {
    pfl.write((const char *) (&numentries),sizeof(numentries)); // [1] number of entries
    if (numentries>0) {
      //      VOUT << "numentries = " << numentries << '\n';
      String writeinbuffer;
      ooplanner_entry_file_data * oefd = new ooplanner_entry_file_data[numentries]; // *** Alternatively, write to file one by one.
      int i = 0;
      PLL_LOOP_FORWARD(ooplanner_entry,head(),1) {
	oefd[i] = e->File_Data(writeinbuffer);
	i++;
      }
      pfl.write((const char *) oefd, numentries*sizeof(ooplanner_entry_file_data)); // [2] ooplanner entries data
      //      VOUT << "sizeof(oefd) = " << (numentries*sizeof(ooplanner_entry_file_data)) << '\n';
      INFO << "Plan entries stored: " << (long) numentries << '\n';
      numentries = writeinbuffer.length();
      //      VOUT << "buffer length = " << numentries << '\n';
      pfl.write((const char *) (&numentries),sizeof(numentries)); // [3] size of writeinbuffer
      if (numentries>0) pfl << writeinbuffer; // [4] buffered write-in data
      delete[] oefd;
    }
    pfl.close();
  }
  return true;
}

bool ooplanner::read_data(bool warnopen) {
  // This reads a binary data file containing the objective-oriented planning entries.
  
  unsigned long numentries = 0;
  ifstream pfl(ooplannerfile+"data",ios::binary);
  if (!pfl) {
    if (warnopen) WARN << "ooplanner::read_data(): Unable to read from " << ooplannerfile << "data.\n";
    return false;
  } else {
    if ((pfl.read((READSOMETYPE) (&numentries), sizeof(numentries))).gcount() < (streamsize) sizeof(numentries)) return false; // [1] number of entries
    if (numentries>0) {
      char * writeinbuffer = NULL;
      ooplanner_entry_file_data * oefd = new ooplanner_entry_file_data[numentries]; // *** Alternatively, read from file one by one.
      if ((pfl.read((READSOMETYPE) oefd, numentries*sizeof(ooplanner_entry_file_data))).gcount() < (streamsize) (numentries*sizeof(ooplanner_entry_file_data))) return false; // [2] ooplanner entries data
      unsigned long writeinbufferlen = 0;
      if ((pfl.read((READSOMETYPE) (&writeinbufferlen), sizeof(writeinbufferlen))).gcount() < (streamsize) sizeof(writeinbufferlen)) return false; // [3] size of writeinbuffer
      if (writeinbufferlen>0) {
	writeinbuffer = new char[writeinbufferlen];
	if ((pfl.read((READSOMETYPE) writeinbuffer, writeinbufferlen*sizeof(char))).gcount() < (streamsize) (writeinbufferlen*sizeof(char))) return false; // [4] buffered write-in data
      }
      for (unsigned int i = 0; i<numentries; i++) append(oefd[i],writeinbuffer);
      delete[] oefd;
      if (writeinbufferlen>0) delete writeinbuffer;
    }
    pfl.close();
  }
  return true;
}

ooplanner_entry * ooplanner::elbyid(DIL_ID & _id) {
  // Returns the address of the ooplanner_entry with the ID _id if found,
  // otherwise returns NULL.
  
  PLL_LOOP_FORWARD(ooplanner_entry,head(),1) if (e->de) {
    if (*(e->de)==_id) return e;
  } else {
    if (e->id==_id) return e;
  }
  return NULL;
}

ooplanner_entry_ptr * ooplanner::access_table() {
  // If ooplanner has entries, then an ordered array is allocated
  // that contains pointers to the entries. Such an array is very
  // useful when applying multiple modifications to the list of
  // entries, since their ordinal positions may change, but the
  // access table retains the original order.
  
  ooplanner_entry_ptr * ooprtable = NULL;
  if (head()) {
    ooprtable = new ooplanner_entry_ptr[length()];
    int i = 0;
    PLL_LOOP_FORWARD(ooplanner_entry,head(),1) {
      ooprtable[i] = e;
      i++;
    }
  }
  return ooprtable;
}

long create_objective_oriented_planner() {
  unsigned long res = 0;
  ooplanner oopr;
  if (!oopr.write_data()) return -1;
  oopr.write_HTML_FORM();
  return res;
}

bool create_pool() {
  // This function creates a set of DIL entry pools, along different dimensions and
  // with different selection criteria, as described in TL#200707271009.1. Pools
  // are sorted along the chosen dimension, and randomized within the same value.
  // These pools are used in the Objective-Oriented Planning form.
  
  // 1. Reset the Objective-Oriented Planner and its pending operations
  unlink(poolfile+"inserts.data"); // reset DIL entries tagged for insert
  //inserts = new pool();
  //inserts->read_data("inserts");
  INFO << "Creating a new Objective-Oriented Planner\n";
  create_objective_oriented_planner();
  
  // 2. Collect DIL entry data
  INFO << "Creating a set of pools\n";
  srand(Time(NULL));
  Detailed_Items_List dilist;
  //DIL_entry ** dep = dilist.Sort_by_Target_Date(true);
  dilist.Get_All_Topical_DIL_Parameters(true);
  //unsigned long numentries = 1;
  //if (dilist.list.head()) numentries = dilist.list.length();
  dilist.list.head()->Set_Semaphores(0);
  
  // 3. Per dimension and selection criterion, create (a) a data file and (b) a corresponding pool form
  create_pool_time_required(dilist);
  create_pool_no_target_date(dilist);
  create_pool_publication(dilist);
  create_pool_near_term(dilist);
  create_pool_GDDM(dilist);
  //create_pool_all_pending(dilist);
  create_pool_remaining(dilist);
  //delete inserts;
  //delete[] dep;
  
  return true;
}

void Show_OOP_FORM_Input(String cmdarg, StringList & qlist) {
  INFO << "<HTML>\n<BODY>\n<H1>dil2al Objective-Oriented Planning</H1>\n";
  INFO << "<H3>Displaying OOP FORM Input:</H3>\n";
  INFO << "Main command line argument: <B>P" << cmdarg << "</B>\n<P>\nList of FORM input commands:\n<OL>\n<LI>";
  INFO << qlist.concatenate("\n<LI>") << '\n';
  INFO << "</OL>\n</BODY>\n</HTML>\n";
}

//#define TESTING_OOPLANNING

void Mark_OOP_Insert(String & dilidstr) {
  // Mark to "insert" a specific DIL entry from a pool.
  
  INFO << "<HTML>\n<BODY>\n<PRE>";
  pool inserts;
  inserts.read_data("inserts",false);
  inserts.append(dilidstr);
  inserts.write_data("inserts");
#ifdef TESTING_OOPLANNING
  inserts.write_HTML_FORM("inserts","Inserts");
#endif
  INFO << "</PRE>\n<P>\n<B>DIL#" << dilidstr << " added to Inserts pool.</B>\n"; //</BODY>\n</HTML>\n";
  String ooplannerstr;
  read_file_into_String(ooplannerfile+"html",ooplannerstr);
  
  INFO << ooplannerstr.after("<BODY>");
}

void Update_Pool(String & poolname) {
  // Update a pool to reflect entries in the ooplanner and entries in the
  // insert pool.
  
  INFO << "<HTML>\n<BODY>\n<PRE>Updating the " << poolname << " pool\n<!--";
  oopr = new ooplanner();
  oopr->read_data();
  srand(Time(NULL));
  inserts = new pool();
  inserts->read_data("inserts");
  Detailed_Items_List dilist;
  dilist.Get_All_Topical_DIL_Parameters(true);
  if (poolname.matches("time_required")) create_pool_time_required(dilist);
  else if (poolname.matches("no_target_date")) create_pool_no_target_date(dilist);
  else if (poolname.matches("publication")) create_pool_publication(dilist);
  else if (poolname.matches("near_term")) create_pool_near_term(dilist);
  else if (poolname.matches("GDDM")) create_pool_GDDM(dilist);
  else if (poolname.matches("all_pending")) create_pool_all_pending(dilist);
  else if (poolname.matches("other")) create_pool_remaining(dilist);
  else {
    INFO << "-->\nUnknown pool: " << poolname << "</PRE></BODY></HTML>\n";
    delete inserts;
    delete oopr;
    return;
  }
  delete inserts;
  delete oopr;
  String poolstr;
  read_file_into_String(poolfile+poolname+".html",poolstr);
  INFO << "--></PRE>\n" << poolstr.after("<BODY>");
}

void Import_into_OOP(String & importlocationstr) {
  // Imports one or more inserts into a specific location in the
  // Objective-Oriented Planner.
  
  INFO << "<HTML>\n<BODY>\n<PRE>";
  ooplanner oopr;
  oopr.read_data();
  long importbeforeindex = -1;
  if (importlocationstr.matches("top")) importbeforeindex = 0;
  else if (importlocationstr.contains("after")) importbeforeindex = 1 + atoi(((const char *) importlocationstr)+5);
  if ((importbeforeindex<0) || (importbeforeindex>oopr.length())) {
    ERROR << "Import_into_OOP(): Invalid import index.\n";
    Exit_Now(1);
  }
  pool inserts;
  inserts.read_data("inserts",false);
  ooplanner_entry * succ = oopr.el(importbeforeindex);
  if (succ) PLL_LOOP_FORWARD(pool_entry,inserts.head(),1) oopr.insert_before(succ,new ooplanner_entry(*e));
  else PLL_LOOP_FORWARD(pool_entry,inserts.head(),1) oopr.link_before(new ooplanner_entry(*e));
  oopr.write_data();
  unlink(poolfile+"inserts.data"); // reset DIL entries tagged for insert
  INFO << "</PRE>\n<P>\n<B>Imported " << inserts.length() << " DIL entries into the Objective-Oriented Planner.</B>\n<P>You can [<A HREF=\"/cgi-bin/dil2al?dil2al=Pu\">Update</A>] the Planner now (which avoids import index confusion if further edits are made).\n"; //</BODY>\n</HTML>\n";
  String ooplannerstr;
  read_file_into_String(ooplannerfile+"html",ooplannerstr);
  INFO << ooplannerstr.after("<BODY>");
}

void Update_OOP() {
  // Update the Objective-Oriented Planner to reflect the current data.
  
  INFO << "<HTML>\n<BODY>\n<PRE>Updating the Objective-Oriented Planner\n<!--";
  //srand(Time(NULL));
  ooplanner oopr;
  oopr.read_data();
  oopr.write_HTML_FORM();
  String ooplannerstr;
  read_file_into_String(ooplannerfile+"html",ooplannerstr);
  INFO << "-->\n</PRE>\n" << ooplannerstr.after("<BODY>");
}

void OOP_FORM_Update(StringList & qlist) {
  // This function updates the Objective-Oriented Planner FORM
  // according to FORM commands received.
  
  const BigRegex WIRX("WIafter\\([0-9]+\\)=");
  const BigRegex TDRX("TD\\([0-9]+\\)=");
  INFO << "<HTML><HEAD><TITLE>Applying Objective-Oriented Planner FORM Commands</TITLE><META HTTP-EQUIV=\"Refresh\" CONTENT=\"1;URL=" << ooplannerfile << "html\"></HEAD><BODY><PRE>\n";
  ooplanner oopr;
  oopr.read_data();
  //  VOUT << qlist.concatenate(" ; ") << '\n';
  //  VOUT << "plan length: " << oopr.length() << '\n';
  ooplanner_entry_ptr * ooprtable = oopr.access_table();
  int len = oopr.length();
  int tdchanged = 0; int wiadded = 0;
  //  for (int i=0; i<len; i++) VOUT << "oopr[" << i << "]->id=" << oopr[i]->id << '\n';
  for (unsigned int i=0; i<qlist.length(); i++) {
    if (qlist[i].contains(WIRX)) {
      long wiafterindex = atoi(qlist[i].sub(WIRX,1));
      if ((wiafterindex<0) || (wiafterindex>=len)) {
	WARN << "OOP_FORM_Update(): WIafter index out of bounds. Skipping" << qlist[i] << ".\n";
	continue;
      }
      String wi(qlist[i].after('='));
      if (wi.empty()) continue;
      wi.gsub('+',' ');
      wi = URI_unescape(wi);
      ooplanner_entry * oe = new ooplanner_entry(wi);
      oopr.insert_after(ooprtable[wiafterindex],oe);
      wiadded++;
    } else if (qlist[i].contains("WItop=")) {
      String wi(qlist[i].after('='));
      if (wi.empty()) continue;
      wi.gsub('+',' ');
      wi = URI_unescape(wi);
      ooplanner_entry * oe = new ooplanner_entry(wi);
      oopr.link_after(oe);
      wiadded++;
    } else if (qlist[i].contains(TDRX)) {
      long tdindex = atoi(qlist[i].sub(TDRX,1));
      if ((tdindex<0) || (tdindex>=len)) {
	WARN << "OOP_FORM_Update(): TD index out of bounds. Skipping" << qlist[i] << ".\n";
	continue;
      }
      String td(URI_unescape(qlist[i].after('=')));
      if (td.empty()) continue;
      ooprtable[tdindex]->targetdate=time_stamp_time(td); // *** NOT tested if this needs noexit=true (20190127)
      tdchanged++;
    }
  }
  INFO << "Write-ins added: " << wiadded << " Target Dates changed: " << tdchanged << '\n';
  if (ooprtable) delete[] ooprtable;
  oopr.write_data();
  oopr.write_HTML_FORM();
  INFO << "</PRE></BODY></HTML>\n";
}

//#define DISPLAY_OOP_FORM_INPUT

void Objective_Oriented_Planning(String cmdarg, StringList & qlist) {
  // This function reads command from form input in cmdarg and calls the
  // functions that perform the objective-oriented planning commands.
  
  eout = &cout; // redirect error output to cout to make it visible in the HTML response
  INFO << "Content-Type: text/html\n\n";
  if (cmdarg.empty()) {
    INFO << "<HTML><HEAD><TITLE>Generating NEW Objective-Oriented Planner</TITLE><META HTTP-EQUIV=\"Refresh\" CONTENT=\"1;URL=" << ooplannerfile << "html\"></HEAD><BODY><PRE>\n";
    create_pool();
    INFO << "</PRE></BODY></HTML>\n";
  } else {
    char c = cmdarg[0]; cmdarg.del(0,1);
    switch (c) {
#ifndef DISPLAY_OOP_FORM_INPUT
    case 'I':
      Mark_OOP_Insert(cmdarg);
      break;
    case 'U':
      Update_Pool(cmdarg);
      break;
    case 'i':
      Import_into_OOP(cmdarg);
      break;
    case 'u':
      Update_OOP();
      break;
    case 'o':
      OOP_FORM_Update(qlist);
      break;
#endif
    default: Show_OOP_FORM_Input(c+cmdarg,qlist);
    };
  }
}

#endif
