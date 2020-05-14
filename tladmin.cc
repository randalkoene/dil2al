// tladmin.cc
// Randal A. Koene, 20000304

#include "dil2al.hh"

#define DEBUG_SELECT_TL_DIL_REFS 0
#define _DEBUG_SELECT_TL_DIL_REFS_BUFSTR(DebugLabel) \
  do { if (DEBUG_SELECT_TL_DIL_REFS) { VOUT << "DBG " << DebugLabel << ": bufstr=(" << bufstr << "), bufstr.length()=" << bufstr.length(); if (bufstr.empty()) VOUT << '\n'; else VOUT << ", bufstr[0]=#" << (int) bufstr[0] << '\n'; } } while (0)
#define _DEBUG_SELECT_TL_DIL_REFS_SELECTION(DebugLabel,Selection) \
  do { if (DEBUG_SELECT_TL_DIL_REFS) { VOUT << "DBG " << DebugLabel << ": Selection detected = " << Selection << '\n'; } } while (0)

bool _add_TL_section_flag = false;

String get_TL_head() {
// resolves the TL symbolic link to the TL head file
  if (taskloghead!="") return taskloghead;
  const int LLEN = 10240;
  char lbuf[LLEN];
  int res;
  if ((res=readlink(tasklog,lbuf,LLEN))<0) {
    ERROR << "get_TL_head(): Unable to read Task Log symbolic link\n" << tasklog << '\n';
    return String("");
  } else {
    lbuf[res]='\0';
    if (lbuf[0]=='/') taskloghead=lbuf;
    else {
      taskloghead = tasklog;
      taskloghead.gsub(BigRegex("[^/]*$"),"");
      taskloghead += lbuf;
    }
    if (verbose) INFO << "Task Log symbolic link resolved to " << taskloghead << '\n';
    return taskloghead;
  }
}

int max_chunk_entry(String & tlchunk) {
// searches for the largest entry number in a Task Log chunk
// returns 0 if there are no entries
// note that this works even if the entries are not in order or entry numbers are missing
#ifdef DEBUG
  cout << "TLCHUNK:\n" << tlchunk << '\n';
#endif
  int maxentry = 0, entryindex = -1;
  while ((entryindex = tlchunk.index(BigRegex("[<]!--[ 	]*entry Begin[ 	]*--[>]"),entryindex+1))>=0) {
    String entryid = tlchunk.before(BigRegex("[ 	]*[<]/B[>]"),entryindex);
    entryid = entryid.after("<B>",-1);
    int entryidnum = 0;
    if (entryid.matches(BRXint)) entryidnum = atoi((const char *) entryid);
#ifdef DEBUG
    cout << "entryid = " << entryid << "\nentryidnum = " << entryidnum << '\n';
#endif
    if (entryidnum>maxentry) maxentry = entryidnum;
  }
#ifdef DEBUG
  cout << "maxentry = " << maxentry << '\n';
#endif
  return maxentry;
}

String generate_TL_chunk_header(String nametag, String newalref, String altitle, String alhead, String newdilref, String diltitle, String dilhead, int generateparts) {
// generatparts = 0 both parts, -1 begin part, 1 end part
#ifdef DEBUG
  cout << "Generating TL chunk header with tag = " << nametag << ", AL = " << newalref << ", DIL entry = " << newdilref << '\n';
#endif
  String chunkheader;
  if (generateparts<=0) {
    chunkheader = "<!-- chunk Begin --><TR><TD><FONT COLOR=\"#FFFFFF\"><A NAME=\""+nametag+"\"><B>"+nametag+"</B></A>\n<!-- chunk AL --><A HREF=\""+newalref+"\">"+altitle+"</A> (";
    if (alhead!="") chunkheader += "<A HREF=\""+alhead+"\">previous</A>/next)\n<!-- chunk Context -->";
    else chunkheader += "previous/next)\n<!-- chunk Context -->";
    if (newdilref[0]=='!') chunkheader += newdilref.after("!")+" (";
    else chunkheader += "<A HREF=\""+newdilref+"\">"+diltitle+" "+newdilref.after("#")+"</A> (";
    if (dilhead!="") chunkheader +=  "<A HREF=\""+dilhead+"\">previous</A>/next)</FONT>\n<P>\n\n";
    else chunkheader += "previous/next)</FONT>\n<P>\n\n";
  }
  if (generateparts>=0) {
    chunkheader += "<!-- chunk End --></TD></TR>\n\n";
  }
  return chunkheader;		
}

String generate_TL_entry_header(String nametag, String newalref, String altitle, String alhead, String newdilref, String diltitle, String dilhead) {
  String entryheader;
  entryheader = "<!-- entry Begin --><FONT COLOR=\"#FFFFFF\">[<A NAME=\""+nametag+"\"><B>"+nametag.after(".")+"</B></A>";
  if ((newalref[0]!='*') || (newdilref[0]!='*')) {
    entryheader += " -\n<!-- entry AL -->";
    if (newalref[0]!='*') {
      entryheader += "<A HREF=\""+newalref+"\">"+altitle+"</A> (";
      if (alhead!="") entryheader += "<A HREF=\""+alhead+"\">previous</A>/next)\n<!-- entry Context -->";
      else entryheader += "previous/next)\n<!-- entry Context -->";
    } else entryheader += "\n<!-- entry Context -->";
    if (newdilref[0]!='*') {
      if (newdilref[0]=='!') entryheader += newdilref.after("!")+" (";
      else entryheader += "<A HREF=\""+newdilref+"\">"+diltitle+" "+newdilref.after("#")+"</A> (";
      if (dilhead!="") entryheader +=  "<A HREF=\""+dilhead+"\">previous</A>/next)]</FONT>\n<BR>\n";
      else entryheader += "previous/next)]</FONT>\n<BR>\n";
    } else entryheader += "]</FONT>\n<BR>\n";
  } else entryheader += "]</FONT>\n"; //*** add "<BR>\n" here?
  return entryheader;
}

// Don't worry if some of these things aren't optimal yet, they can always be
// improved with experience from use.

String get_most_recent_TC_AL(const String & tl) {
// returns the AL reference of the most recent task chunk
// returns an empty string if none is found
  String curAL;
  // *** NOTE: This FAILS if tl contains a new empty Task Log with no chunk in it.
  //cout << "Length of tl = " << tl.length() << '\n'; cout.flush();
  //cout << "tl = " << tl << '\n'; cout.flush();
  curAL = const_cast<String &>(tl).at(BigRegex("[<]!-- chunk AL [^(]*"),-1);
  if (curAL.empty()) return curAL;
  curAL.del(BigRegex("[<]!-- chunk AL -->[^\"]*\""));
  curAL.del(BigRegex("\".*"));
  curAL.prepend(basedir+RELLISTSDIR);
  return curAL;
}

String get_most_recent_TC_DE(const String & tl) {
// returns the DIL entry reference of the most recent task chunk
// returns an empty string if none is found
  String curDE;
  curDE = const_cast<String &>(tl).at(BigRegex("[<]!-- chunk Context --[>][^\n][^(]*("),-1);
  if (curDE.empty()) return curDE;
  curDE.del(BigRegex("[<]!-- chunk Context --[>]"));
  curDE.del(BigRegex("[<]A [^>]*[Hh][Rr][Ee][Ff][ 	]*=[^\"]*\""));
  curDE.gsub("\">",":");
  curDE.del(BigRegex("\\([<][^>]*[>]\\)?[ 	]*("));
  return curDE;
}

bool stop_TL_chunk(String & tl) {
  // Mark the completion time of a Task Log chunk if there is one open.
  // tl contains the most recent Task Log Section.
  // Returns success status.
  //
  // NOTE A: AL options determine if and how the current AL and closed Chunk's
  // DIL entry should be updated, and the corresponding update is done.
  // See TL#200610270457.1 about periodicautoupdate.
  //
  // NOTE B: On 20190215 I corrected a bug that I had introduced on 20190208.
  // This function should NOT return false if no open chunk End tag is found.
  // This function should close an open chunk IF there is one, and otherwise
  // it should not interfere in state or operations.

  _EXTRA_VERBOSE_REPORT_FUNCTION();

  // ** THIS IS PROBABLY WHERE ADDING A @ACTTYPE=@ AND OTHER TASK-CHUNK SPECIFIC DATA GOES! ADD A HOOK HERE. **
  
  // Mark the Task Chunk closed
  if (tl.gsub(BigRegex("[<]!--[ 	]+chunk End[ 	]+--[>][ 	]*[<]/TD[>][ 	]*[<]/TR[>]"),"<!-- chunk End --><I>"+curtime+"</I></TD></TR>")<=0) {
    // See NOTE B.
    if (_add_TL_section_flag) INFO << "Task Chunk was already closed while new Task Section was created.\n";
    else WARN << "stop_TL_chunk(): Cannot find unclosed chunk End tag to fill in. Continuing.\n";
    return true;
  }
  
  // determine if AL and completion ratios should be updated automatically
  if (alautoupdate==AAU_NO) return true;
  if (alautoupdate==AAU_ASK) if (!confirmation("Update AL and completion ratios? (Y/n) ",'n')) return true;
  
  // find the most recent Task Chunk
  String chunkid; int lasttlchunk;
  if ((lasttlchunk = locate_most_recent_TL_chunk(tl,-1,&chunkid))<0) {
    WARN << "stop_TL_chunk(): Unable to update AL and DIL entry. Continuing.\n";
    return true;
  }
  
  // obtain AL and DIL information from the most recent chunk
  bool recomputeAL = false;
  String curAL = get_most_recent_TC_AL(tl);
  String curDE = get_most_recent_TC_DE(tl);
  // a proper DIL entry ID is returned in the form:
  // some-DIL-file.html#XXXXXXXXXXXXXX.Y:DIL-title-and-entry-ID
  curDE = curDE.before(':'); curDE = curDE.after('#');
  if ((curDE.empty()) || (!curDE.matches(BRXdouble))) { // testing empty() is necessary (see BigRegex.hh)
    INFO << "Completed task chunk was not a DIL entry.\n";
    recomputeAL = true;
  } else {
    // Looks like a time stamp, let's compare with current time
    time_t tdiff = time_stamp_diff(chunkid,curtime);
    // Older notes:
    // *** suggest tdiff, but allow entry of other time, in which
    //     case request if required time and completion status should
    //     be modified directly
    // *** if tdiff > taskchunksize*1.25 recompute AL... this test
    //     may not be necessary, since any significant delay will
    //     cause desynchronization between the updated suggested
    //     start time in the AL and the current time, which causes
    //     recomputation of the AL when a significant difference is
    //     detected in select_TL_DIL_refs()
    if (!update_DIL_entry_elements(curDE,0.0,false,0.0,true,0.0,false,tdiff,periodicautoupdate)) recomputeAL = true;
    if (!remove_AL_TC(curAL,curDE)) recomputeAL = true;
  }

  // recompute the AL
  if (recomputeAL) {
    INFO << "Recomputing AL.\n";
    Detailed_Items_List dilist;
    DIL_entry ** dep = dilist.Sort_by_Target_Date(true);
    if (!generate_AL(dep)) WARN << "stop_TL_chunk(): Unable to generate AL. Continuing as is.\n";
    delete[] dep;
  }
  
  return true;
}

// inlined here, because only used locally
inline int get_DIL_preset(StringList & diloptions) {
  // Get the preset if there is a preset file.
  // diloptions[0] returns the preset (if found).
  // Returns 0 if there was no preset.
  // Returns 1 if there was a preset.
  // NOTE: No warning is produced if the preset file is not found, since
  // that is the normal case.

  String dilpresetstr;
  if (!read_file_into_String(dilpresetfile,dilpresetstr,false)) return 0;
  if (dilpresetstr.firstchar()!='*') {
    diloptions[0] = dilpresetstr; // put the preset into the default
    if (unlink(dilpresetfile)!=0) ERROR << "get_DIL_preset(): Unable to remove " << dilpresetfile << " file.\nContinuing as is.\n";
  } else diloptions[0] = dilpresetstr.from(1); // put the preset into the default and retain the file
  return 1;
}

// inlined here, because only used locally
inline void get_AL_for_choices(const String & tl, String & newalref, String & curAL) {
  // Get the Active List to use for default/quick choices.
  // From:
  // - the most recent Task Chunk, or
  // - specification in newalref, or
  // - empty.
  // tl is not modified.
  // if newalref starts with '*' that character is removed.

  if (newalref.empty()) curAL = get_most_recent_TC_AL(tl); // obtain current AL from TL
  else {
    if (newalref[0]=='*') newalref.del("*");
    curAL = basedir+RELLISTSDIR+newalref;
  }

  if (verbose) INFO << "Current Active List: " << curAL << '\n';

  if (!curAL.empty()) {
    if (curAL[curAL.length()-1] == '/') {
      WARN << "get_AL_for_choices(): CurAL appears to point to a directory. Ignoring.\n";
      curAL = "";
    }
  }  
}

// inlined here, because only used locally
inline bool get_updated_AL(const String & curAL, String & curALstr) {
  // Fetch the contents of the specified AL, making sure it is up-to-date.
  // The AL is recomputed if the current time is significantly later than the next
  // Task Chunk start time should typically be.
  // curALstr returns the AL contents.
  // curAL is the specified AL and is not modified.
  // Returns true if AL content was retrieved.

  if (curAL.empty()) return false;

  _EXTRA_VERBOSE_REPORT_FUNCTION();
  
  if (!read_file_into_String(curAL,curALstr)) return false;

  // find the start of the table of AL Days
  int alpos = curALstr.index("<!-- @Focused-AL Days+TCs START@ -->");
  if (alpos<0) {
    ERROR << "get_updated_AL(): Missing @Focused-AL Days+TCs START@ tag in text\nfrom " << curAL << ".\n";
    // *** start test only
    ERROR << String(curALstr.before(10000));
    ERROR.flush();
    const int LLEN = 1024;
    char lbuf[LLEN];
    cin.getline(lbuf,LLEN);
    // *** end test only
    return false;
  }
  // find the first cell of the table with the day's suggested tasks
  String tmpstr, cellcontent;
  HTML_get_table_cell(curALstr,alpos,tmpstr,cellcontent);
  HTML_remove_tags(cellcontent);
  BigRegex alr("Day[ 	]+\\([0-9]+\\).*suggested[ 	]+start[ 	]+\\([0-9][0-9]\\):\\([0-9][0-9]\\)");
  if (cellcontent.index(alr)!=0) {
    WARN << "get_updated_AL(): Day row missing at top of AL " << curAL << "\nContinuing as is.\n";
    WARN << cellcontent << '\n';
    return false;
  }
    
  // get suggested start time for next TC
  tmpstr = cellcontent.at(alr.subpos(1),alr.sublen(1))+cellcontent.at(alr.subpos(2),alr.sublen(2))+cellcontent.at(alr.subpos(3),alr.sublen(3));

  // possibly update AL
  time_t tdiff = time_stamp_diff(tmpstr,curtime); // positive if curtime is later than suggested time
  if (tdiff>alsyncslack) { // recompute the AL
    // determine if AL updating should be automatic
    // *** Note: A separate configuration variable should be used if alautoupdate
    //     should apply only to the recomputation of the AL in stop_TL_chunk()
    //     above.
    switch (alautoupdate_TCexceeded) {
    case AAU_NO:
      if (verbose) WARN << "AL desynchronized, no automatic recomputation (alautoupdate==AAU_NO).\n";
      break;
    case AAU_ASK:
      if (confirmation("AL desynchronized, update now? (y/N) ",'y',"No","Yes")) break;
    default:
      if (verbose) INFO << "Recomputing AL.\n";
      Detailed_Items_List dilist;
      DIL_entry ** dep = dilist.Sort_by_Target_Date(true);
      // *** make this able to request generation of an AL
      //     corresponding to the superior that generated curAL
      if (!generate_AL(dep)) WARN << "get_updated_AL(): Unable to generate AL. Continuing as is.\n";
      else { // get new version of AL
	curALstr = "";
	read_file_into_String(curAL,curALstr);
      }
      delete[] dep;
    }
  }
  // *** it is possible to add a message here if curtime is a lot
  //     smaller than the suggested start time of the AL, e.g.
  //     if all suggested TCs for this AL day have been completed

  return true;
}

// inlined here, because only used locally
inline void get_options_from_AL(const String & curALstr, StringList & diloptions, int & numdiloptions) {
  // Fetch options from high priority items in the current AL.
  // curALstr contains the AL content, it is not modified.
  // Added options are returned in diloptions.
  // The total number of DIL options in diloptions is updated in numdiloptions.

  if (curALstr.empty()) return;
  
  int cellindex = 0; String cellparameters, cellcontent;
  while ((numdiloptions<6) && ((cellindex=HTML_get_table_cell(curALstr,cellindex,cellparameters,cellcontent))>=0))
    if (!cellcontent.contains(BigRegex("[<]!--[ 	]+@select_TL_DIL_refs:[ 	]+SKIP ROW@[ 	]+--[>]"))) { // skip marked rows
      diloptions[numdiloptions]=cellcontent;
      String idhreftext;
      if (HTML_get_href(cellcontent,0,cellparameters,idhreftext)>=0) {
	// only unique DIL options
	for (int i=0; i<numdiloptions; i++) if (diloptions[i].contains(idhreftext)) { cellparameters=""; break; }
	if (cellparameters.length()>0) {
	  diloptions[numdiloptions]=cellparameters+':'+cellcontent.after(BigRegex("[]]\n?"));
	  HTML_remove_tags(diloptions[numdiloptions]);
	  remove_whitespace(diloptions[numdiloptions]);
	  if (diloptions[numdiloptions]!="") numdiloptions++;
	}
      } else WARN << "get_options_from_AL(): Missing DIL entry reference in AL. Continuing.\n";
    }
}

// inlined here, because only used locally
inline void get_options_from_TL(const String & tl, StringList & diloptions, int & numdiloptions) {
  // Fetch options from recent DIL entries in the Task Log.
  // tl contains the current Task Log Section, it is not modified.
  // Added options are returned in diloptions.
  // The total number of DIL options in diloptions is updated in numdiloptions.

  BigRegex r("[<]!-- \\(chunk\\|entry\\) Context --[>]\\([^\n][^(]*\\)(");
  int pos = tl.length(), matchlen;
  while (((pos = r.search(tl,pos-1,matchlen,-1))>=0) && (numdiloptions<10)) {
    int subpos;
    if (r.match_info(subpos,matchlen,2)) {
      diloptions[numdiloptions] = const_cast<String &>(tl).at(subpos,matchlen);
      String idhrefname = diloptions[numdiloptions].at(BigRegex("#[0-9]+[.][0-9]+"));
      // only unique DIL options
      for (int i=0; i<numdiloptions; i++) if (diloptions[i].contains(idhrefname)) { idhrefname=""; break; }
      if (idhrefname!="") {
	diloptions[numdiloptions].del(BigRegex("[<]A [^>]*[Hh][Rr][Ee][Ff][ 	]*=[^\"]*\""));
	diloptions[numdiloptions].gsub("\">",":");
	diloptions[numdiloptions].del(BigRegex("\\([<][^>]*[>]\\)?[ 	]*("));
	if (diloptions[numdiloptions]!="") numdiloptions++;
      }
    }
  }
}

// inlined here, because only used locally
inline String select_TL_DIL_refs_default(bool newchunk, StringList & diloptions) {
  // Returns the default option (with override management).
  
  char override;
  String overridestr(diloptions[0].after(":"));
  if (overridestr.empty()) override = 'n';
  else override = overridestr[0];
  
  String res = diloptions[0].before(":");
  if ((!newchunk) && (override!='!')) res.prepend("*"); // same as chunk DIL (no entry specific reference required)

  return res;
}

// inlined here, because only used locally
inline String select_TL_DIL_refs_numeric(const char * numstr, StringList & diloptions, int numdiloptions) {
  // Returns the numeric choice from diloptions.
  
  int q = atoi(numstr);
  String res;
  if ((q<0) || (q>=numdiloptions)) {
    WARN << "select_TL_DIL_refs_numeric(): Selection (" << (long) q << ") is not one of the DIL reference options.\n";
    return res;
  }
  res = diloptions[q].before(":");
  return res;
}

// inlined here, because only used locally
inline String select_TL_DIL_refs_DILID(const char * dilidstr) {
  // Returns the DIL reference by DIL ID.

  String res;
  ifstream dbid(idfile);
  if (!dbid) {
    WARN << "select_TL_DIL_refs_DILID(): Unable to read " << idfile << ".\n";
    return res;
  }

  const int LLEN = 10240;
  char lbuf[LLEN];
  res = "#";
  res += dilidstr;
  if (!find_line(&dbid,res,lbuf,LLEN)) {
    dbid.close();
    WARN << "select_TL_DIL_refs_DILID(): DIL reference " << res << " not found in " << idfile << '\n';
    res = "";
    return res;
  }
  
  dbid.close();
  res = lbuf;
  res.del(BigRegex("[^<]*[<]TD[^>]*[>][<]A[^>]*[Hh][Rr][Ee][Ff]=\""));
  res = res.before("\"");
  return res;
}


// inlined here, because only used locally
inline String select_TL_DIL_refs_browser() {
  // Returns DIL reference if stored in dilref file from browser.
  // The dilref file is removed after the reference is retrieved.
  // NOTE: Before 20110822 the browser was directed to curAL.
  
  _EXTRA_VERBOSE_REPORT_FUNCTION();

  if (System_Call(browser+" "+nexttaskbrowserpage)<0) WARN << "select_TL_DIL_refs_browser(): Unable to browse. Continuing\n";
  
  ifstream drf(dilref);
  if (!drf) return ""; // It's not necessarily an error.

  const int LLEN = 10240;
  char lbuf[LLEN];
  if (!find_line(&drf,"Current URL",lbuf,LLEN)) {
    drf.close();
    unlink(dilref);
    return "";
  }
  
  String urlline = lbuf; // the line containing "Current URL"
  drf.getline(lbuf,LLEN);
  drf.close();
  unlink(dilref);

  // if getline managed to put another line into lbuf that starts with a space
  if (lbuf[0]==' ') urlline += lbuf; // include that
  urlline.del("Current URL");
  urlline.del("file://");
  urlline.gsub(" ","");
  urlline.del(basedir+RELLISTSDIR);

  String checkidfile=idfile.after('/',-1); // grab the detailed-items-by-ID.html part
  if (urlline.contains(checkidfile)) {
    if (confirmation("ATTENTION: Do you REALLY want the ID file '"+urlline+"'\nas DIL reference ID or identifying comment?\nTHIS IS NOT RECOMMENDED!\nPlease (c)onfirm or (n)o. ",'c',"No","Confirm")) return "";
    else return urlline; // A very strange choice
  }

  if (!confirmation("Take '"+urlline+"'\nas DIL reference ID or identifying comment? (y/N) ",'y',"NO","Yes")) return urlline;

  return "";
}

String select_TL_DIL_refs(String & tl, bool newchunk, String newalref) {
  // Select DIL reference for new TL Chunk or Entry.
  // - tl should provide a reference to a String containing the most recent Section of the Task Log.
  // - newchunk indicates if a new Chunk or a new Entry is being prepared.
  // - If newalref is not "" (empty) then use it as AL for default/quick choices
  // Return value:
  //   "*<DIL-ref>" = same as Chunk,
  //   "!<DIL-ref>" = non-DIL identifying comment,
  //   "*" = decision postponed (see variableAL),
  //   "" = error,
  //   if a DIL-ID is selected then "<DIL-file>#<DIL-ID>" is returned.
  // When newchunk=true, the default is the highest priority item in the current AL.
  // When newchunk=false, the default is the same as the current Chunk.
  //
  // For use of a preset, see how ~/local/bin/personentry attempts to use this option.
  // A preset put into a ~/.dil2al-DILidpreset file should be of the form:
  // [*]someDILfile.html#DILID:[!]
  // If preprended by * then the preset file will not be erased after use.
  // If followed by ! then the preset value overrides even in the case where no new Chunk is created.
  // Otherwise, no new Chunk would result in no DIL reference in the default case.
  
  //const int LLEN = 10240;
  //char lbuf[LLEN];
  StringList diloptions;
  int numdiloptions = 0;

  _EXTRA_VERBOSE_REPORT_FUNCTION();
  
  // find the current Active List
  String curAL;
  get_AL_for_choices(tl,newalref,curAL);
    
  // get the preset as default if there is a preset file
  int preset = get_DIL_preset(diloptions);
  numdiloptions += preset;

  // possibly get the current Chunk as default
  if ((!newchunk) && (!preset)) { // current chunk DIL ref (or identifying comment) is default
    diloptions[0] = get_most_recent_TC_DE(tl);
    if (diloptions[0].empty()) {
      ERROR << "select_TL_DIL_refs(): Entry is not in a Task Log Chunk.\n";
      return String("");
    }
    numdiloptions++;
  }
  
  // get the (possibly updated) AL
  String curALstr;
  get_updated_AL(curAL,curALstr);
	
  // some options from high priority current AL items
  get_options_from_AL(curALstr,diloptions,numdiloptions);

  // some options from recent TL entries
  get_options_from_TL(tl,diloptions,numdiloptions);

  // optionally immediately pick the default
  if (!askALDILref) return select_TL_DIL_refs_default(newchunk,diloptions);

  // improve readability
  for (int i=0; i<numdiloptions; i++) HTML_remove_tags(diloptions[i]);
  
  // can enter DIL ids directly, or quick-chooser choice from priority list, or textual comment instead of DIL
  StringList specialsel("?,\"\";?&;*",';');
  StringList specialopt("browse;fork browser;postpone",';');
  UI_Options_RQData dilopt = { NULL, "Optional direct entry of DIL ID or Comment:", diloptions, numdiloptions, "d", diloptions[0], &specialsel, &specialopt, 3, NULL };
  String message, image;
  if (newchunk) {
    message = "\nTask CHUNK DIL ID or identifying comment (deprecated) [can include HREFs]:\n  Note A: with 'browser' option in w3m save URL to ~/tmp/dilref with 2,ESC,SHIFT+M,d.\n  Note B: postpone to set a new Active List (current AL = "+curAL.after(basedir+RELLISTSDIR)+")\n";
    image = (basedir+RELLISTSDIR)+"dil2al-add-chunk.png";
  } else {
    message = "\nTask ENTRY DIL ID or identifying comment (deprecated) [can include HREFs]:\n  Note A: with 'browser' option in w3m save URL to ~/tmp/dilref with 2,ESC,SHIFT+M,d.\n  Note B: postpone to set a new Active List (current AL = "+curAL.after(basedir+RELLISTSDIR)+")\n";
    image = (basedir+RELLISTSDIR)+"dil2al-add-entry.png";
  }
  dilopt.message = message; // Build this in String first for a persisten const char * buffer (otherwise weird things happen)
  dilopt.image = image;
  String affiliation;
  do {

    String bufstr(options->request(dilopt));
    _DEBUG_SELECT_TL_DIL_REFS_BUFSTR("select_TL_DIL_refs()");
      
    // detect 'd' default chosen
    if (downcase(bufstr) == "d") {
      _DEBUG_SELECT_TL_DIL_REFS_SELECTION("select_TL_DIL_refs()","d - default chosen");
      affiliation = select_TL_DIL_refs_default(newchunk,diloptions);
      continue;
    }

    // detect selection via browser
    if (bufstr.empty() || (bufstr=="?")) {
      _DEBUG_SELECT_TL_DIL_REFS_SELECTION("select_TL_DIL_refs()","empty/? - via browser");
      affiliation = select_TL_DIL_refs_browser();
      continue;
    }

    // detect numeric choice from diloptions
    // NOTE: The bufstr.empty() test has to precede the BRXint or BRXdouble tests,
    // because those also return 1 for empty strings.
    if (bufstr.matches(BRXint)) {
      _DEBUG_SELECT_TL_DIL_REFS_SELECTION("select_TL_DIL_refs()","BRXint - numeric choice");
      affiliation = select_TL_DIL_refs_numeric(bufstr,diloptions,numdiloptions);
      continue;
    }

    // detect DIL ID number
    if (bufstr.matches(BRXdouble)) {
      _DEBUG_SELECT_TL_DIL_REFS_SELECTION("select_TL_DIL_refs()","BRXdouble - DIL ID");
      affiliation = select_TL_DIL_refs_DILID(bufstr);
      continue;
    }

    // possibly spawn a concurrent browser at curAL
    if (bufstr=="?&") {
      _DEBUG_SELECT_TL_DIL_REFS_SELECTION("select_TL_DIL_refs()","?& - spawn concurrent browser");
      if (noX) {
	if (System_Call(browser+" "+curAL+" &")<0) WARN << "select_TL_DIL_refs(): Unable to spawn a browser. Continuing.\n";
      } else {
	if (System_Call("urxvt -e "+browser+" "+curAL+" &")<0) WARN << "select_TL_DIL_refs(): Unable to spawn a browser. Continuing.\n";
      }
      affiliation = "";
      continue;
    }

    // detect postpone
    if (bufstr=="*") {
      _DEBUG_SELECT_TL_DIL_REFS_SELECTION("select_TL_DIL_refs()","* - postpone");
      affiliation = bufstr;
      continue;
    }

    // detect explicit Cancel
    if (bufstr=="Cancel") {
      _DEBUG_SELECT_TL_DIL_REFS_SELECTION("select_TL_DIL_refs()","Cancel - cancel");
      return String("");
    }
    
    // otherwise, interpret as a non-DIL identifying comment (double-check that this is intended!)
    _DEBUG_SELECT_TL_DIL_REFS_SELECTION("select_TL_DIL_refs()","other - comment");
    if (confirmation("Your selection designates a comment unrelated to a specific DIL entry.\nIs this your true intent (y/N)? ",'y',"No","Yes")) continue;
    affiliation = "!";
    affiliation += bufstr;

  } while (affiliation.empty());
  
  return affiliation;
}

// inlined here, because only used locally
inline void get_options_from_listfile(const String & curAL, StringList & aloptions, int & numaloptions) {
  // Get AL options from the listfile (lists.html).
  // aloptions returns the options found.
  // numaloptions returns the number of options found.
  // curAL contains the most recent AL, it is not modified.

  ifstream alf(listfile);
  if (!alf) {
    WARN << "get_options_from_listfile(): Unable to read " << listfile << ".\nContinuing.\n";
    return;
  }
  
  const int LLEN = 10240;
  char lbuf[LLEN];
  if (!find_line(&alf,"[<]A[ 	][^>]*[Nn][Aa][Mm][Ee][ 	]*=[ 	]*\"AL\"",lbuf,LLEN)) {
    alf.close();
    WARN << "get_options_from_listfile(): No AL references in " << listfile << ".\nContinuing.\n";
    return;
  }
  
  String alid[2];
  while (find_in_line(&alf,"[<]LI[>][<]A[ 	][^>]*[Hh][Rr][Ee][Ff][ 	]*=[ 	]*\"(.+)[<][/]A[>]",2,alid,"[<][/]UL[>]",lbuf,LLEN)==1) {
    alid[1].gsub("\">",":");
    alid[1].del(RELLISTSDIR);
    if (!curAL.contains(alid[1].before(":"))) { // *** Perhaps it's fine to remove this test and add it to the list too
      aloptions[numaloptions] = alid[1];
      numaloptions++;
    }
  }
  alf.close();
}


// inlined here, because only used locally
inline String select_TL_AL_refs_numeric(const char * numstr, StringList & aloptions, int numaloptions) {
  // Returns the numeric choice from aloptions.

  int q = atoi(numstr);

  String res;
  if ((q<0) || (q>=numaloptions)) {
    WARN << "select_TL_AL_refs_numeric(): Selection (" << (long) q << ") is not one of the AL reference options.\n";
    return res;
  }
  res = aloptions[q].before(":");
  return res;
}

// inlined here, because only used locally
inline String select_TL_AL_refs_browser() {
  // Returns AL reference if stored in dilref file from browser.
  // The dilref file is removed after the reference is retrieved.
  
  if (System_Call(browser+" "+listfile)<0) WARN << "select_TL_AL_refs_browser(): Unable to browse. Continuing\n";
  
  String res;
  ifstream drf(dilref);
  if (!drf) return res; // It's not necessarily an error.

  const int LLEN = 10240;
  char lbuf[LLEN];
  if (!find_line(&drf,"Current URL",lbuf,LLEN)) {
    drf.close();
    unlink(dilref);
    return res;
  }
  
  String urlline = lbuf;
  drf.getline(lbuf,LLEN);
  drf.close();
  unlink(dilref);
  
  if (lbuf[0]==' ') urlline += lbuf;
  urlline.del("Current URL");
  urlline.del("file://");
  urlline.gsub(" ","");
  urlline.del(basedir+RELLISTSDIR);
  
  if (!confirmation("Take '"+urlline+"'\nas AL reference ID? (y/N) ",'y',"No","Yes")) return urlline;

  return res;
}

String select_TL_AL_refs(String & tl, bool newchunk, String newdilref) {
  // Select an AL reference for new TL Chunk or Entry.
  // - tl should provide a reference to a String containing the most recent Section of the Task Log.
  // - newchunk indicates if a new Chunk or a new Entry is being prepared.
  // - if newdilref=="*" then possibly choose a new AL
  // Return value:
  //   "*<AL-ref>" = same as Chunk,
  //   "" = error.
  // The default is the current AL.
  // Alternatives are read from list.html.
  
  const int LLEN = 10240;
  char lbuf[LLEN];
  StringList aloptions;
  int numaloptions = 0;
  
  // obtain current AL from TL
  String curAL = get_most_recent_TC_AL(tl);

  // optionally immediately pick the default
  if ((!askALDILref) || (newdilref!="*")) {
    //*** Could insure that if newdilref!="*" then the default Active List that is returned is
    //    curAL, as long as the DIL is in that. Otherwise, the primary one is the one that the
    //    DIL in newdilref is in.
    curAL = curAL.after(basedir+RELLISTSDIR);
    if (!newchunk) curAL.prepend("*");
    return curAL;
  }

  // some options from listfile
  get_options_from_listfile(curAL,aloptions,numaloptions);
  
  // can enter AL ids directly, or quick-chooser choice from priority list
  StringList specialsel("?;?&",';');
  StringList specialopt("browse;fork browser",';');
  String defaultoptstr = String(curAL.after(basedir+RELLISTSDIR));
  const char * defaultopt = defaultoptstr.chars();
  if (defaultoptstr.empty()) defaultopt = NULL;
  UI_Options_RQData alopt = { NULL, "Optional direct entry of AL:", aloptions, numaloptions, "\"\"", defaultopt, &specialsel, &specialopt, 2, NULL };
  String message;
  if (newchunk) message = "\nTask Chunk Active List (AL) reference ID:\n  Note: with browser option in w3m save URL to ~/tmp/dilref with 2,ESC,SHIFT+M,d.\n";
  else message = "\nTask Entry Active List (AL) reference ID:\n  Note: with browser option in w3m save URL to ~/tmp/dilref with 2,ESC,SHIFT+M,d.\n";
  alopt.message = message; // Build this in String first for a persisten const char * buffer (otherwise weird things happen)
  String affiliation;
  do {
    String bufstr(options->request(alopt));
    _DEBUG_SELECT_TL_DIL_REFS_BUFSTR("select_TL_AL_refs()");

    // *** TESTING
    //cout << "bufstr = " << bufstr << '\n';
    //return String("");
    // ***

    // detect default chosen
    // NOTE: The bufstr.empty() test has to precede the BRXint test, because that test
    // also returns 1 for empty strings.
    if (bufstr.empty()) {
      _DEBUG_SELECT_TL_DIL_REFS_SELECTION("select_TL_AL_refs()","empty - default");
      affiliation = curAL.after(basedir+RELLISTSDIR);
      if (!newchunk) affiliation.prepend("*"); // same as chunk AL (no entry specific reference required)
      continue;
    }
    
    // detect numeric choice from aloptions
    if (bufstr.matches(BRXint)) {
      _DEBUG_SELECT_TL_DIL_REFS_SELECTION("select_TL_AL_refs()","BRXint - numeric choice");
      affiliation = select_TL_AL_refs_numeric(bufstr,aloptions,numaloptions);
      continue;
    }

    // detect selection via browser
    if (bufstr=="?") {
      _DEBUG_SELECT_TL_DIL_REFS_SELECTION("select_TL_AL_refs()","? - via browser");
      affiliation = select_TL_AL_refs_browser();
      continue;
    }

    // possibly spawn a concurrent browser at curAL
    if (bufstr=="?&") {
      _DEBUG_SELECT_TL_DIL_REFS_SELECTION("select_TL_AL_refs()","?& - spawn concurrent browser");
      if (noX) {
	if (System_Call(browser+" "+curAL+" &")<0) WARN << "select_TL_AL_refs(): Unable to spawn a browser. Continuing.\n";
      } else {
	if (System_Call("urxvt -e "+browser+" "+curAL+" &")<0) WARN << "select_TL_AL_refs(): Unable to spawn a browser. Continuing.\n";
      }
      affiliation = "";
      continue;
    }

    // detect explicit Cancel
    if (bufstr=="Cancel") {
      _DEBUG_SELECT_TL_DIL_REFS_SELECTION("select_TL_AL_refs()","Cancel - cancel");
      return String("");
    }

    // verify that string is AL ID
    _DEBUG_SELECT_TL_DIL_REFS_SELECTION("select_TL_AL_refs()","other - direct entry");
    ifstream alf(listfile);
    if (!alf) {
      ERROR << "select_TL_AL_ref(): Unable to read " << listfile << ".\n";
      return String("");
    }    
    if (!find_line(&alf,"[<]A[ 	][^>]*[Nn][Aa][Mm][Ee][ 	]*=[ 	]*\"AL\"",lbuf,LLEN)) {
      alf.close();
      ERROR << "select_TL_AL_ref(): No AL references found in " << listfile << ".\n";
      return String("");
    }
    bufstr.prepend(RELLISTSDIR);
    bufstr.prepend("[<]LI[>][<]A[ 	][^>]*[Hh][Rr][Ee][Ff][ 	]*=[ 	]*\"");
    if (!find_line(&alf,bufstr,lbuf,LLEN)) {
      alf.close();
      EOUT << "select_TL_AL_ref(): AL reference " << bufstr << " not found in " << listfile << '\n';
      affiliation = "";
      continue;
    }
    alf.close();
    affiliation = lbuf;
    affiliation = affiliation.after(BigRegex("[Hh][Rr][Ee][Ff][ 	]*=[ 	]*\""));
    affiliation = affiliation.after(RELLISTSDIR);
    affiliation = affiliation.before("\">");
    
  } while (affiliation.empty());
  
  return affiliation;
}

// inlined right here, because only used locally
inline bool get_TL_Entry_NAME_tag(TL_chunk_access & TL, String & entry_nametag, int & chunk_end_index) {
  // Find the most recent 'chunk End' tag and create a NAME tag for the new Entry.
  // TL.tl contains the Task Log Section.
  // TL.chunkid is the most recent Chunk's NAME tag.
  // TL.tlinsertloc is the search position (negative for backwards search).
  // entry_nametag returns the proposed NAME tag.
  // chunk_end_index returns the location of the 'Chunk End' tag.
  // TL is not modified.
  // Returns success status.
  
  // Initialize Entry NAME tag
  entry_nametag = TL.chunkid;
  // Find Chunk 'chunk End' tag
  if ((chunk_end_index = TL.tl.index(BigRegex("[<]!--[ 	]*chunk End[ 	]*--[>]"),TL.tlinsertloc))<0) {
    ERROR << "get_TL_Entry_NAME_tag(): Unable to find Task Log Chunk end.\n";
    return false;
  }
  String tlchunk = TL.tl.at(TL.tlinsertloc,chunk_end_index-TL.tlinsertloc);
  entry_nametag = entry_nametag + "." + String((long) max_chunk_entry(tlchunk)+1);
  return true;
}

// inlined right here, because only used locally
inline bool get_AL_head_and_title(String & newalref, DILAL_head_title & ALLF) {
  // Find head and title of the specified AL in the lists file.
  // newalref specifies the AL to use.
  // ALLF returns head, title, file content and RegEx data.
  // newalref is not modified.

  if (newalref[0]=='*') return true;

  if (!read_file_into_String(listfile,ALLF.str)) return false;
  
  ALLF.rx = "[<]LI[>][ 	]*[<]A[ 	][^>]*[Hh][Rr][Ee][Ff][ 	]*=[ 	]*\""+RX_Search_Safe(RELLISTSDIR+newalref)+"\"[^\n]*";
  ALLF.title = ALLF.str.at(BigRegex(ALLF.rx));
  ALLF.head = ALLF.title.at(BigRegex("([^)]*AL head"));
  ALLF.title.del(BigRegex("[ 	]*([^)]*AL head.*"));
  ALLF.title.gsub(BigRegex("[<][^>]*[>]"),"");
  if (ALLF.head!="") {
      ALLF.head.del(BigRegex("([^,]*,[ 	]*[<]A [^>]*[Hh][Rr][Ee][Ff][ 	]*=[ 	]*\""));
      if (ALLF.head[0]!='/') {
	if (ALLF.head.contains(RELLISTSDIR,0)) ALLF.head.del(RELLISTSDIR);
	else ALLF.head.prepend("../");
      }
      ALLF.head = ALLF.head.before("\"");
      ALLF.head.del(get_TL_head());
  }
  return true;
}

// inlined right here, because only used locally
inline bool get_DIL_head_and_title(String & newdilref, TL_chunk_access & TL, DILAL_head_title & DL) {
  // Find head and title of the specified DIL Entry (or non-DIL identifying comment).
  // newdilref specifies the DIL Entry.
  // TL.tl contains the most recent Section of the Task Log.
  // DL returns head, title, file content and RegEx data.
  // newdilref is not modified.
  // TL is not modified.
  // NOTE: In practice, non-DIL identifying comments are not presently in use.

  if (newdilref[0]=='*') return true;
  
  if (newdilref[0]=='!') {
    // non-DIL identifying comment
    int dilheadindex;
    if ((dilheadindex = TL.tl.index(BigRegex("Context[ 	]*--[>][ 	]*"+RX_Search_Safe(newdilref.after("!"))+"[^(]*([^/]*/next"),-1))>=0) {
      
      DL.head = TL.tl.at(BigRegex("Begin[ 	]*-->[^\n]*[<]A[ 	]+[^>]*[Nn][Aa][Mm][Ee][ 	]*=[ 	]*\"[^\"]+"),dilheadindex-TL.tl.length());
      if (!DL.head.empty()) {
	DL.head = DL.head.after("\"",-1);
	DL.head.prepend('#');
      } else WARN << "add_TL_chunk_or_entry(): Context error, no matching Begin. Continuing.\n";
      
    } else {
      // search for Context in previous TL sections
      String prevtlfile = TL.tl.at(BigRegex("[<]/H1[>]\n+[<]TABLE.*[>][ 	]*previous[ 	]+TL[ 	]+section"));
      if (!prevtlfile.empty()) {
	prevtlfile = prevtlfile.after(BigRegex("[<]A[ 	]+[Hh][Rr][Ee][Ff]=\""));
	prevtlfile = basedir+RELLISTSDIR+prevtlfile.before("\"");
	while (!prevtlfile.empty()) {
	  String prevtl;
	  if (!read_file_into_String(prevtlfile,prevtl)) {
	    WARN << "get_DIL_head_and_title(): Continuing without non-DIL identifying comment previous dilhead.\n";
	    break;
	  }
	  if ((dilheadindex = prevtl.index(BigRegex("Context[ 	]*--[>][ 	]*"+RX_Search_Safe(newdilref.after("!"))+"[^(]*([^/]*/next"),-1))>=0) {
	    DL.head = prevtl.at(BigRegex("Begin[ 	]*-->[^\n]*[<]A[ 	]+[^>]*[Nn][Aa][Mm][Ee][ 	]*=[ 	]*\"[^\"]+"),dilheadindex-TL.tl.length());
	    if (!DL.head.empty()) {
	      DL.head = DL.head.after("\"",-1);
	      DL.head.prepend('#');
	      break;
	    } else WARN << "get_DIL_head_and_title(): Context error, no matching Begin. Continuing.\n";
	  }
	  prevtlfile = prevtl.at(BigRegex("[<]/H1[>]\n+[<]TABLE.*[>][ 	]*previous[ 	]+TL[ 	]+section"));
	  if (!prevtlfile.empty()) { // for the next while-test
	    prevtlfile = prevtlfile.after(BigRegex("[<]A[ 	]+[Hh][Rr][Ee][Ff]=\""));
	    prevtlfile = basedir+RELLISTSDIR+prevtlfile.before("\"");
	  }
	}
      } else WARN << "get_DIL_head_and_title(): Context error, no previous Task Log file. Continuing.\n";
    }
    
  } else {
    // DIL reference ID (newdilref="<DIL-file>#<DIL-ID>")
    if (!read_file_into_String(basedir+RELLISTSDIR+newdilref.before("#"),DL.str)) return false;
    // DIL title from newdilref file
    DL.title = DL.str.at(BigRegex("[<]TITLE[>][^<]*[<]/TITLE[>]"));
    DL.title.del("<TITLE>");
    DL.title.del("</TITLE>");
    // DIL head from newdilref file
    DL.rx = "[<]TD[^>]*[>][<]A [^>]*[Nn][Aa][Mm][Ee][ 	]*=[ 	]*\""+RX_Search_Safe(newdilref.after("#"))+"\"[^\n]*([^)]*head";
    DL.head = DL.str.at(BigRegex(DL.rx));
    if (!DL.head.empty()) {
      DL.head.del(BigRegex("[<]TD[^>]*[>][<]A [^>]*[Nn][Aa][Mm][Ee][ 	]*=[ 	]*\""+RX_Search_Safe(newdilref.after("#"))+"\"[^\n]*([^,]*,[ 	]*[<]A [^>]*[Hh][Rr][Ee][Ff][ 	]*=[ 	]*\""));
      DL.head = DL.head.before("\"");
      DL.head.del(get_TL_head());
    }
  }

  return true;
}

// inlined right here, because only used locally
inline bool modify_AL_head_next(TL_chunk_access & TL, DILAL_head_title & ALLF, DILAL_newref & nr, TLALDIL_headfile & HF, String & nametag, String & tlp) {
  // Modify 'next' in previous AL head.
  // tlp returns the contents of the Task Log file indicated by alheadfile.
  // TL is not modified.
  // ALLF is not modified.
  // nr is not modified.
  // HF is not modified.
  // nametag is not modified.

  if (ALLF.head.empty()) return true;

  // A) create newalnext reference
  // B) get the contents of the Task Log file indicated by alheadfile
  String newalnext, altlfile = basedir+RELLISTSDIR+HF.al; // TL file pointed to by AL head
  //*** This is not very optimized, requiring some copying of long Strings. This is due to a ``bug'' in the
  //    String class, which caused modifications via a pointer tlp->at.() or reference tlp.at() to add
  //    several characters of junk at the end of the string. Note that we cannot simply overwrite TL.tl
  //    here as it may be needed later.
  if (HF.tl==HF.al) {
    tlp = TL.tl;
    newalnext = '#'+nametag;
  } else {
    if (!read_file_into_String(altlfile,tlp)) return false;
    newalnext = HF.tl+'#'+nametag;
  }

  // find the AL head
  BigRegex alindexrx("\\(chunk\\|entry\\)[ 	]+Begin[ 	]+--[>][^\n]*[<]A[ 	][^>]*[Nn][Aa][Mm][Ee][ 	]*=[ 	]*\""+ALLF.head.after("#")+'"');
  //*** could make this a forward search by continuing to search until -1 is returned
  //    NOT NECESSARY WITH NON-PARANOID OR FASTER BACKWARD SEARCH METHOD
  int alnextindex=tlp.index(alindexrx,-1);
  if (alnextindex<0) {
    ERROR << "modify_AL_head_next(): Unable to find previous AL head in " << HF.al << ".\n";
    return false;
  }

  // create new 'next' HREF reference
  String alnext;
  BigRegex alnextrx("AL[ 	]+--[>][^\n]*\""+nr.al+"\"[^(]*([^)]*/next");
  alnext = tlp.at(alnextrx,alnextindex);
  alnext = alnext.before("next",-1) + "<A HREF=\""+newalnext+"\">next</A>";

  // insert the new 'next' HREF reference
  String ttmp = tlp.before(BigRegex("AL[ 	]+--[>][^\n]*\""+RX_Search_Safe(nr.al)+"\"[^(]*([^)]*/next"),alnextindex);
  ttmp += alnext;
  ttmp += tlp.after(BigRegex("AL[ 	]+--[>][^\n]*\""+RX_Search_Safe(nr.al)+"\"[^(]*([^)]*/next"),alnextindex);
  tlp = ttmp;
  //*** THIS WORKED... SO IT IS OBVIOUSLY DANGEROUS TO USE THE SUBSTRING REPLACEMENT
  //    METHOD FOR INSERTION, BUT THE ABOVE IS OF COURSE VERY NON-OPTIMAL!
  //    tlp.at(BigRegex("AL[ 	]+--[>][^\n]*\""+newalref+"\"[^(]*([^)]*/next"),alnextindex)=alnext;

  // if this Task Log file needs no further modification then store its updated content
  if (HF.al!=HF.dil) {
    if (!write_file_from_String(altlfile,tlp,"AL head Task Log")) return false;
  }
  
  return true;
}

// inlined right here, because only used locally
inline bool modify_DIL_head_next(TL_chunk_access & TL, DILAL_head_title & DL, DILAL_newref & nr, TLALDIL_headfile & HF, String & nametag, String & tlp) {
  // Modify 'next' in previous DIL head.
  // tlp returns the contents of the Task Log file indicated by dilheadfile.
  // TL is not modified.
  // DL is not modified.
  // nr is not modified.
  // HF is not modified.
  // nametag is not modified.

  if (DL.head.empty()) return true;

  // A) create newdilnext reference
  // B) get the contents of the Task Log file indicated by dilheadfile
  String newdilnext, diltlfile = basedir+RELLISTSDIR+HF.dil; // TL file pointed to by DIL head
  if (HF.dil==HF.tl) newdilnext = '#'+nametag;
  else newdilnext = HF.tl+'#'+nametag;
  if (HF.al!=HF.dil) {
    if (HF.tl==HF.dil) tlp = TL.tl;
    else if (!read_file_into_String(diltlfile,tlp)) return false;
  }

  // find the DIL head
  String dilnext;
  //*** could make this a forward search by continuing to search until -1 is returned
  //    NOT NECESSARY WITH NON-PARANOID OR FASTER BACKWARD SEARCH METHOD
  int dilnextindex=tlp.index(BigRegex("\\(chunk\\|entry\\)[ 	]+Begin[ 	]+--[>][^\n]*[<]A[ 	][^>]*[Nn][Aa][Mm][Ee][ 	]*=[ 	]*\""+RX_Search_Safe(DL.head.after("#")+'"')),-1);
  if (dilnextindex<0) {
    ERROR << "modify_DIL_head_next: Unable to find previous DIL head (" << DL.head.after("#") << ") in " << HF.dil << ".\n";
    return false;
  }

  // A) create new 'next' HREF reference
  // B) insert the new 'next' HREF reference
  if (nr.dil[0]=='!') {
    dilnext=tlp.at(BigRegex("Context[ 	]+--[>][ 	]*"+RX_Search_Safe(nr.dil.after("!"))+"[^(]*([^)]*/next"),dilnextindex);
    dilnext.at("next",-1)="<A HREF=\""+newdilnext+"\">next</A>";
    tlp.at(BigRegex("Context[ 	]+--[>][ 	]*"+RX_Search_Safe(nr.dil.after("!"))+"[^(]*([^)]*/next"),dilnextindex)=dilnext;
  } else {
    dilnext=tlp.at(BigRegex("Context[ 	]+--[>][^\n]*\""+RX_Search_Safe(nr.dil)+"\"[^(]*([^)]*/next"),dilnextindex);
    dilnext.at("next",-1)="<A HREF=\""+newdilnext+"\">next</A>";
    tlp.at(BigRegex("Context[ 	]+--[>][^\n]*\""+RX_Search_Safe(nr.dil)+"\"[^(]*([^)]*/next"),dilnextindex)=dilnext;
  }

  // store the updated content of this 
  if (!write_file_from_String(diltlfile,tlp,"Context head Task Log")) return false;
  
  return true;
}

// inlined right here, because only used locally
inline bool update_listfile_AL_head(DILAL_newref & nr, TLALDIL_headfile & HF, String & nametag, DILAL_head_title & ALLF) {
  // Update the AL head reference in the listfile.
  // ALLF contains the listfile and is updated and written to the file.
  // nr is not modified.
  // HF is not modified.
  // nametag is not modified.

  if (nr.al[0]=='*') return true;
  
  if (!ALLF.head.empty()) {
    String newalnext = ALLF.str.at(BigRegex(ALLF.rx));
    newalnext.at(BigRegex("[Hh][Rr][Ee][Ff][ 	]*=[ 	]*\"[^\"]+\""),-1) = "HREF=\""+(RELLISTSDIR+HF.tl)+'#'+nametag+"\"";
    ALLF.str.at(BigRegex(ALLF.rx)) = newalnext;
  } else {
    String newalnext = ALLF.str.at(BigRegex("[<]LI[>][ 	]*[<]A[ 	][^>]*[Hh][Rr][Ee][Ff][ 	]*=[ 	]*\""+RX_Search_Safe(RELLISTSDIR+nr.al)+"\"[^\n]*"));
    newalnext.at("<BR>",-1) = " (<A HREF=\""+(RELLISTSDIR+HF.tl)+'#'+nametag+"\">AL tail</A>,<A HREF=\""+(RELLISTSDIR+HF.tl)+('#'+nametag)+"\">AL head</A>)<BR>";
    ALLF.str.at(BigRegex("[<]LI[>][ 	]*[<]A[ 	][^>]*[Hh][Rr][Ee][Ff][ 	]*=[ 	]*\""+RX_Search_Safe(RELLISTSDIR+nr.al)+"\"[^\n]*")) = newalnext;
  }
  
  if (!write_file_from_String(listfile,ALLF.str,"Main Lists")) return false;

  return true;
}

// inlined right here, because only used locally
inline bool update_DIL_file_DIL_head(DILAL_newref & nr, TLALDIL_headfile & HF, String & nametag, String & newdilidstr, DILAL_head_title & DL) {
  // Update the DIL head reference in the DIL file.
  // DL contains the DIL file and is updated and written to the file.
  // nr is not modified.
  // HF is not modified.
  // nametag is not modified.
  // newdilidstr is not modified.

  if (!DL.head.empty()) {
    String newdilnext = DL.str.at(BigRegex(DL.rx)); 
    newdilnext.at(BigRegex("[Hh][Rr][Ee][Ff][ 	]*=[ 	]*\"[^\"]+\""),-1) = "HREF=\""+HF.tl+'#'+nametag+"\"";
    DL.str.at(BigRegex(DL.rx)) = newdilnext;
  } else {
    String newdilnext = DL.str.at(BigRegex("[<]TD[^>]*[>][<]A [^>]*[Nn][Aa][Mm][Ee][ 	]*=[ 	]*\""+RX_Search_Safe(newdilidstr)+"\"[^\n]*"));
    newdilnext += " (<A HREF=\""+HF.tl+'#'+nametag+"\">tail</A>,<A HREF=\""+HF.tl+'#'+nametag+"\">head</A>)<BR>";
    DL.str.at(BigRegex("[<]TD[^>]*[>][<]A [^>]*[Nn][Aa][Mm][Ee][ 	]*=[ 	]*\""+RX_Search_Safe(newdilidstr)+"\"[^\n]*")) = newdilnext;
  }
  if (!write_file_from_String(basedir+RELLISTSDIR+nr.dil.before("#"),DL.str,"DIL")) return false;

  return true;
}
  
String add_TL_chunk_or_entry(String & notestr, bool newchunk, TL_chunk_access & TL) {
  // Add a new Chunk or a new Entry to the Task Log.
  // notestr contains the Entry contents to be added (it is ignored if newchunk==true)
  // newchunk specifies to add a Chunk (true) or an Entry (false).
  // TL.tl contains the most recent Section of the Task Log.
  // TL.chunkid is the ID for the new Chunk or Entry.
  // TL.tlinsertloc is used when adding an Entry, and it is set by this function when adding a Chunk.
  // Returns nametag (if successful) or "" (if unsuccessful).

  _EXTRA_VERBOSE_REPORT_FUNCTION();

  // prepare NAME tag and possible Entry insertion location
  String nametag;
  int chunkendindex = -1; // used to find NAME tag and for entry insertion
  if (!newchunk) {
    // set new Entry NAME tag and get last position in current Chunk
    if (!get_TL_Entry_NAME_tag(TL,nametag,chunkendindex)) return String("");
  } else {
    // set a new Chunk NAME tag and close any open current Chunk
    nametag = curtime;
    if (!stop_TL_chunk(TL.tl)) return String(""); // also updates AL and completion ratios
  }
  
  // obtain DIL reference ID or identifying comment and AL reference ID
  // NOTE: This asks for the DIL Entry to use, or the AL from which to propose DIL entries.
  DILAL_newref nr;
  while ((nr.dil=="*") || (nr.dil=="")) {
    if ((nr.dil=select_TL_DIL_refs(TL.tl,newchunk,nr.al))=="") return String(""); // usually gets "<DIL-file>#<DIL-ID>" pair
    if ((nr.al=="") || (nr.dil=="*")) if ((nr.al=select_TL_AL_refs(TL.tl,newchunk,nr.dil))=="") return String("");
  }

  // find the previous AL reference with the same ID
  // NOTE: This is usually the previous Chunk in the TL, since we tend to only use one AL.
  DILAL_head_title ALLF; // AL data from lists file (typically lists.html)
  if (!get_AL_head_and_title(nr.al,ALLF)) return String("");

  // find the previous DIL reference for the DIL Entry
  DILAL_head_title DL;
  if (!get_DIL_head_and_title(nr.dil,TL,DL)) return String("");
    
  // A) in notepre, prepend newalref, newdilref, ALLF.head, DL.head, ALLF.title and DL.title information to note
  // B) in TL.tlinsertloc, set proper note insertion location
  String notepre;
  if (newchunk) {
    notepre = generate_TL_chunk_header(nametag, nr.al, ALLF.title, ALLF.head, nr.dil, DL.title, DL.head);
    // find new chunk insertion location (becomes Tl.tlinsertloc for entry also)
    if ((TL.tlinsertloc = TL.tl.index(BigRegex("[<]!--[ 	]+section End[ 	]+-->"))) < 0) {
      ERROR << "add_TL_chunk_or_entry(): Unable to find Section End in " << taskloghead << ".\n";
      return String("");
    }
  } else {
    notepre = generate_TL_entry_header(nametag, nr.al, ALLF.title, ALLF.head, nr.dil, DL.title, DL.head);
    // reposition at End of chunk for insertion of TL entry
    TL.tlinsertloc = chunkendindex;
  }
  
  // insert note and store if operations on TL.tl are complete
  String tltail = TL.tl.from(TL.tlinsertloc);
  if (newchunk) TL.tl = TL.tl.before(TL.tlinsertloc) + notepre + tltail;
  else TL.tl = TL.tl.before(TL.tlinsertloc) + notepre + notestr + "<P>\n\n" + tltail;
  get_TL_head();
  TLALDIL_headfile HF;
  HF.tl = taskloghead.after(basedir+RELLISTSDIR);
  HF.al = ALLF.head.before("#"); if ((!ALLF.head.empty()) && (HF.al.empty())) HF.al = HF.tl;
  HF.dil = DL.head.before("#"); if ((!DL.head.empty()) && (HF.dil.empty())) HF.dil = HF.tl;
  if ((HF.tl!=HF.al) && (HF.tl!=HF.dil)) {
    // if further modifications are in other files then store the updated Task Log file
    if (!write_file_from_String(taskloghead,TL.tl,"Task Log")) return String("");
  }

  // modify 'next' in previous AL head
  String tlp;
  if (!modify_AL_head_next(TL,ALLF,nr,HF,nametag,tlp)) return String("")
										      ;
  // modify 'next' in previous DIL head
  if (!modify_DIL_head_next(TL,DL,nr,HF,nametag,tlp)) return String("");
  
  // update AL head in listfile
  if (!update_listfile_AL_head(nr,HF,nametag,ALLF)) return String("");

  // if the Chunk or Entry was not for a DIL-ID, or it's the same DIL-ID as before, then we're done here
  if ((nr.dil[0]=='*') || (nr.dil[0]=='!')) {
    if (newchunk) chunkcreated = true; // update process variable
    return nametag;
  }

  // DIL Entry ID
  String newdilidstr(nr.dil.after('#'));
  
  // update DIL head in DIL file
  if (!update_DIL_file_DIL_head(nr,HF,nametag,newdilidstr,DL)) return String("");
   
  // Now provide a temporary file with a history of entries related to the task with the new DIL ID
  if (!newdilidstr.empty()) {
    if (!get_collected_task_entries(newdilidstr,taskhistorypage)) WARN << "add_TL_chunk_or_entry(): Unable to create temporary file with history of entries for DIL#" << newdilidstr << ".\n";
    else {
      if (noX) VOUT << "Temporary file with history of entried for DIL#" << newdilidstr << " in " << taskhistorypage << " (e.g. use `fhistory` to view)\n";
      else System_Call(taskhistorycall+" "+taskhistorypage+" &"); // *** Should this use nohup?
    }
  }
  
  // If this is the first time of the day, show the metrics!
  if (detect_temporary_file(showmetricsflagfile)) System_Call("formalizer-showmetrics.sh");
    
  if (newchunk) chunkcreated = true; // update process variable
  return nametag;
}

bool add_TL_section(String & tl) {
  // Add a new section to the Task Log
  // Note: modifies current Task Log head file, tl, taskloghead,
  //       Main List File (listfile), Task Log symbolic link (tasklog);
  //       update in-memory copies and any variables that reference specific
  //       items or locations in the modified variables and files if necessary

  _EXTRA_VERBOSE_REPORT_FUNCTION();
  
  // close any currently active TL chunk
  if (!stop_TL_chunk(tl)) return false;
  // initialize new TL section
  get_TL_head();
  String curtlid = taskloghead.after("task-log.");
  curtlid = curtlid.before(".html");
  if (curtlid==curdate) {
    ERROR << "add_TL_section(): Cannot create Task Log section with same ID as current Task Log head.\n";
    return false;
  }
  String newreltlhead = "task-log."+curdate+".html";
  String newtlsection = "<HTML>\n<HEAD><TITLE>Task Log ("+curdate+")</TITLE>\n</HEAD>\n<BODY BGCOLOR=\"#000000\" TEXT=\"#FFDF00\" LINK=\"#AFAFFF\" VLINK=\"#7F7FFF\">\n<H1><FONT COLOR=\"#00FF00\">Task Log ("
    + curdate + ")</FONT></H1>\n\n<TABLE WIDTH=\"100%\" CELLPADDING=3 BGCOLOR=\"#007F00\"><TR><TD WIDTH=\"30%\" ALIGN=CENTER><A HREF=\"task-log."
    + curtlid + ".html\"><FONT COLOR=\"#FFFFFF\">previous TL section ("+curtlid+")</FONT></A><TD WIDTH=\"40%\" BGCOLOR=\"#005F00\">&nbsp;<TD WIDTH=\"30%\" ALIGN=CENTER><FONT COLOR=\"#FFFFFF\">(no next TL section)</FONT></TABLE>\n\n"
    + "<P>\n<LI><A HREF=\"../lists.html#AL\">Project Active Lists</A>\n"
    + "<LI><A HREF=\"../lists.html#DIL\">Topical Detailed Items Lists</A>\n"
    + "<LI><A HREF=\"detailed-items-by-ID.html\">Detailed Items by ID</A>\n"
    + "<LI><A HREF=\"../work-log.html\">Work Log</A>\n<P>\n<HR>\n<P>\n"
    + "<!-- section Begin --><TABLE WIDTH=\"100%\" CELLPADDING=3 BGCOLOR=\"#3F3F3F\">\n\n"
    + "<!-- section End --></TABLE>\n\n<P>\n<HR>\n"
    + "<A NAME=\"ENDOFSECTION\">~</A>/doc/<A HREF=\"../maincont.html\">html</A>/<A HREF=\"../lists.html\">lists</A>/"+newreltlhead+"\n\n</BODY>\n";
  String newtaskloghead = basedir+RELLISTSDIR+newreltlhead;
  if (!write_file_from_String(newtaskloghead,newtlsection,"New Task Log section",true)) return false;
  // update next TL section reference in tl and store current Task Log
  tl.at(BigRegex("[<]FONT[ 	]+COLOR[ 	]*=[ 	]*\"#FFFFFF\"[>][ 	]*([ 	]*no next TL section[ 	]*)[ 	]*[<]/FONT[>][<]/TABLE[>]")) = "<A HREF=\""+newreltlhead+"\"><FONT COLOR=\"#FFFFFF\">next TL section ("+curdate+")</FONT></A></TABLE>";
  if (!write_file_from_String(taskloghead,tl,"Task Log")) return false;
  // update list of Task Log sections in listfile
  String lfstr;
  if (!read_file_into_String(listfile,lfstr)) return false;
  int lftlindex = lfstr.index("<A NAME=\"TL\">");
  if (lftlindex>=0) if ((lftlindex=lfstr.index("</OL>",lftlindex))>=0) lfstr.at(lftlindex,1) = "<LI><A HREF=\""+(RELLISTSDIR+newreltlhead)+"\">Task Log: section initiation date mark "+curdate+"</A>\n<";
  if (lftlindex<0) WARN << "add_TL_section(): Unable to update Task Log sections list in " << listfile << ".\nContinuing as is.\n";
  if (!write_file_from_String(listfile,lfstr,"Main List")) return false;
  // update Task Log symbolic link in tasklog
  if (unlink(tasklog)<0) {
    ERROR << "add_TL_section(): Unable to remove Task Log symbolic link " << tasklog << " to task-log." << curtlid << ".html.\n";
    return false;
  }
  if (symlink(newreltlhead,tasklog)<0) {
    ERROR << "add_TL_section(): Unable to create Task Log symbolic link " << tasklog << " to " << newreltlhead << ".\n";
    return false;
  }
  // set variables to new Task Log section
  tl = newtlsection;
  taskloghead = newtaskloghead;
  return true;
}

int locate_most_recent_TL_chunk(String & tl, int seekfrom, String * chunkid) {
// seeks from seekfrom through the task log to find the
// most recent chunk, set seekfrom==-1 to search backwards
// from the end of the task log
// returns the starting location of the most recent chunk or the
// beginning of the section (-1 == not found)
// if chunkid!=NULL sets (*chunkid) to the chunk ID or to "" if
// the beginning of the section is returned
// requires a valid task log section in tl
  int tlinsertloc;
  if ((tlinsertloc = tl.index(BigRegex("[<]!--[ 	]+chunk Begin[ 	]+--[>]"),seekfrom))<0) {
    if (chunkid) (*chunkid) = ""; // no chunks in TL
    if ((tlinsertloc = tl.index(BigRegex("[<]!--[ 	]+section Begin[ 	]+--[>]")))<0) {
      // TL section not properly formatted
      ERROR << "allocate_most_recent_TL_chunk(): Section Begin not found in " << taskloghead << ".\n";
      return -1;
    } 
    return tlinsertloc;
  }
  if (chunkid) {
    // get chunk time stamp at tlinsertloc
    (*chunkid) = tl.at(BigRegex("[<]TR[>][<]TD[>][<][^>]*=[^>]*[>][<]A[ 	]+[Nn][Aa][Mm][Ee][ 	]*=[ 	]*\"[0-9]+"),tlinsertloc);
    chunkid->del(BigRegex("[<]TR[>][<]TD[>][<][^>]*=[^>]*[>][<]A[ 	]+[Nn][Aa][Mm][Ee][ 	]*=[ 	]*\""));
  }
  return tlinsertloc;
}

// *** THIS IS A PLACE FOR SIMPLIFICATION
//     BUT NOTE THAT THIS IS ALREADY UNDER FINE-GRAINED CONTROL BY
//     timechunkoverstrategy and timechunkunderstrategy, so does
//     not need to have simplifyuserrequests testing added.
bool decide_add_TL_chunk(TL_chunk_access & TL, bool comparetimes, bool createifgreater) {
  // Add a Task Log chunk based on strategies and time since creation
  // of most recent chunk
  // time-based decisions can be either
  // create-if-greater-than-chunksize-plus-slack or
  // create-unless-smaller-than-chunksize
  // initialize TL.tlinsertloc with -1 to search from end of Task Log
  // TL.tlinsertloc is updated for use when adding entry to Task Log
  // TL.chunkid is set.
  // Returns success status.

  _EXTRA_VERBOSE_REPORT_FUNCTION();
  
  // find most recent TL chunk (immediately preceding @INSERT-TL-NOTE@ if present)
  bool newchunk = !createifgreater;
  if ((TL.tlinsertloc = locate_most_recent_TL_chunk(TL.tl,TL.tlinsertloc,&TL.chunkid))<0) return false;
  if (TL.chunkid.empty()) newchunk=true;
  else {
#ifdef DEBUG
    cout << "decision chunkid = " << TL.chunkid << "\ncurtime = " << curtime << '\n';
#endif

    // compare chunk time stamp with curtime
    if (createifgreater) {
      
      // test if curtime > chunk_creation_time+timechunksize+timechunkslack
      if ((comparetimes) && ((time_stamp_diff(TL.chunkid,curtime)/60)>(timechunksize+timechunkslack))) {
	if (verbose) INFO << "timechunkoverstrategy = " << timechunkoverstrategy << '\n';
	if (timechunkoverstrategy==TCS_NEW) newchunk=true;
	else if (timechunkoverstrategy==TCS_ASK) {
	  newchunk = !confirmation("The current time is more than "+String((long) (timechunksize+timechunkslack))+" minutes later than the time stamp\nof the current Task Log Chunk. Create a new Task Log Chunk? (y/N) ",'y',"No","Yes");
	}
      }
    } else {
      
      // test if curtime == chunk_creation_time, e.g. created during make_note() opportunity in chunk_controller()
      if (TL.chunkid==curtime) newchunk=false;
      else {
	
	// test if curtime < chunk_creation_time+timechunksize
	if ((comparetimes) && ((time_stamp_diff(TL.chunkid,curtime)/60)<timechunksize)) {
	  if (verbose) INFO  <<  "timechunkunderstrategy = " << timechunkunderstrategy << '\n';
	  if (timechunkunderstrategy==TCS_CURRENT) newchunk=false;
	  else if (timechunkunderstrategy==TCS_ASK) {
	    newchunk = !confirmation("The current time is less than "+String((long) timechunksize)+" minutes after the time stamp\nof the current Task Log Chunk. Create a new Task Log Chunk anyway? (y/N) ",'y',"No","Yes");
	  }
	}
      }
    }
  }
  
  if (newchunk) {

    // if TL contains more than sectionsize chunks consider starting a new section
    bool newsection = false;
    int numchunks = BigRegex_freq(TL.tl,"[<]!--[ 	]+chunk Begin[ 	]+--[>]");
    if (numchunks>=sectionsize) {
      if (verbose) INFO << "sectionstrategy = " << sectionstrategy << '\n';
      if (sectionstrategy==TSS_NEW) newsection = true;
      else if (sectionstrategy==TSS_ASK) {
	newsection = confirmation("There are "+String((long) numchunks)+" Chunks in this Task Log Section. Create a new Section? (Y/n) ",'n',"Yes","No");
      }
    }
    _add_TL_section_flag = false; // this is checked in stop_TL_chunk()
    if (newsection) {
      // add a new Section to the Task Log
      _add_TL_section_flag = true; // this is checked in stop_TL_chunk()
      if (!add_TL_section(TL.tl)) {
	ERROR << "decide_add_TL_chunk(): Unable to add new Task Log Section.\n";
	return false;
      }
    }
    
    // add a new Chunk to the Task Log
    // NOTE: Since 'notestr' is ignored when newchunk==true in add_TL_chunk_or_entry,
    // we can put TL.chunkid in its place to fill the necessary parameter.
    if ((TL.chunkid = add_TL_chunk_or_entry(TL.chunkid,true,TL))=="") {
      ERROR << "decide_add_TL_chunk(): Unable to add new Task Log Chunk.\n";
      return false;
    }
  }
  
  return true;
}

void track_performance(String & dilid, time_t tdiff, double completion, double required) {
  // Tracks performance data in ~/.dil2al-TLperformance.m. This data can be used to generate
  // statistics in files such as  ~/doc/html/planning/system-results.m.
  // This function is called only if tltrackperformance is true.
  // tdiff is assumed to be the time worked
  const BigRegex datadaterx("datadate[ \t]*=[ \t]*\\([0-9]+\\)");
  const BigRegex dayworkedrx("dayworked[ \t]*=[ \t]*\\([0-9]+\\)");
  const BigRegex daytelapsedrx("daytelapsed[ \t]*=[ \t]*\\([0-9]+\\)");
  if (verbose) INFO << "Updating performance data (" << tltrackperformancefile << ")\n";
  String tlperformancedatastr;
  tdiff /= 60; // minutes
  time_t telapsed = tdiff; // equal if this is the first work of the day
  long dayworked = 0, daytelapsed = 0;
  bool isnew = false;
  // **not used** bool newday = false;
  if (read_file_into_String(isnew,tltrackperformancefile,tlperformancedatastr)) { // existing file may have data
    if (tlperformancedatastr.index(datadaterx,0)>=0) { // has datadate
      String datadatestr(tlperformancedatastr.sub(datadaterx,1));
      int j; for (j = 7; j>=0; j--) if (datadatestr[j]!=curtime[j]) break;
      // **not used** if (j>=0) newday = true;
      // **not used** else {
      if (j<0) {
	time_t datadate = time_stamp_time(datadatestr); // *** Probably correct with noexit=false (20190127).
	time_t t = time_stamp_time(curtime); // *** Probably correct with noexit=false (20190127).
	telapsed = (t - datadate)/60; // minutes, this can be compared with time worked
	if (tlperformancedatastr.index(dayworkedrx,0)>=0) { // has dayworked
	  dayworked = atol((const char *) tlperformancedatastr.sub(dayworkedrx,1));
	} else WARN << "Missing dayworked data in track_performance(), assuming dayworked=0.\n";
	if (tlperformancedatastr.index(daytelapsedrx,0)>=0) { // has daytelapsed
	  daytelapsed = atol((const char *) tlperformancedatastr.sub(daytelapsedrx,1));
	} else WARN << "Missing daytelapsed data in track_performance(), assuming daytelapsed=0.\n";
      }
    } // **not used** else newday = true;
  } // **not used** else newday = true;
  dayworked += tdiff;
  daytelapsed += telapsed;
  tlperformancedatastr = "# .dil2al-TLperformance.m\n# Peformance data\ndatadate = ";
  tlperformancedatastr += curtime;
  tlperformancedatastr += ";\ndayworked = ";
  tlperformancedatastr += String(dayworked);
  tlperformancedatastr += ";\ndaytelapsed = ";
  tlperformancedatastr += String(daytelapsed) + ";\n";
  write_file_from_String(tltrackperformancefile,tlperformancedatastr,"Performance file",isnew);
}
