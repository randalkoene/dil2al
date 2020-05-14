// tlfilter.cc
// Randal A. Koene, 20000304

#include "dil2al.hh"

#define DEBUGTLFILTER_A 0
#define DEBUGTLFILTER_B 0
long debug_showchunks = 20;

bool filter_TL_by_AL() {
  ERROR << "filter_TL_by_AL(): This  function is not yet implemented.\n";
  Exit_Now(1);
  return false;
}

bool generate_TL_keyword_index() {
// keywords can be created with a special tag that you insert anywhere
// in a task log entry, or anywhere in HTML or TeX documents or source code or
// plain text, plus in the dil2al configuration file
  ERROR << "generate_TL_keyword_index(): This function is not yet implemented.\n";
  Exit_Now(1);
  return false;
}

bool search_TL(String searchstr) {
// searchstr is a regexp
  ERROR << "search_TL(): This function is not yet implemented.\n";
  Exit_Now(1);
  return false;
}

bool extract_paper_novelties(String fname, String paperplanurl, ostream & ostr) {
// searches a document for <A NAME="NOVELTY-some-id">[<B>N</B>]</A>/<!-- @NOVELTY-some-id@ -->
// tags and returns the items between the tags
  String fcont;
  if (!read_file_into_String(fname,fcont)) {
    EOUT << "dil2al: Unable to read file " << fname << " in extract_paper_novelties()\n";
    return false;
  }
  _BIGSTRING_ITERATOR i=-1;
  while ((i=fcont.index(BigRegex("[<]A[ 	]+[Nn][Aa][Mm][Ee][ 	]*=[ 	]*\"NOVELTY-[^\"]*\"[>]"),i+1))>=0) {
    _BIGSTRING_ITERATOR ii = fcont.index("\">",i); // find end of ID
    if (ii>=0) {
      String nid;
      nid = fcont.at(i,ii-i);
      nid = nid.after("\"",-1);
      ii = fcont.index("</A>",ii); // find start of content
      if (ii>=0) {
	BigRegex en("[<]!--[ 	]+@"+RX_Search_Safe(nid)+"@[ 	]+\\([^>]*\\)--[>]");
	_BIGSTRING_ITERATOR iii = fcont.index(en,ii); // get end marker
	if (iii>=0) {
	  String cstr, istr, lstr, sstr;
	  if (en.sublen(1)>0) { // extract possible qualifying and quantifying information
	    String qqinfo = fcont.at(en.subpos(1),en.sublen(1));
	    const BigRegex qqi("@CONTEXT:[ 	]*\\([^@]*\\)@[ 	]*@IMP:[ 	]*\\([^@]*\\)@[ 	]*@LEN:[ 	]*\\([^@]*\\)@[ 	]*@SECTION:[ 	]*\\([^@]*\\)@");
	    if (qqinfo.index(qqi)>=0) {
	      cstr = qqinfo.at(qqi.subpos(1),qqi.sublen(1));
	      istr = qqinfo.at(qqi.subpos(2),qqi.sublen(2));
	      lstr = qqinfo.at(qqi.subpos(3),qqi.sublen(3));
	      sstr = qqinfo.at(qqi.subpos(4),qqi.sublen(4));
	    }
	  }
	  String ncont;
	  ncont = fcont.at(ii+4,iii-(ii+4)); // get content
	  ncont.gsub(BigRegex("[<][^>]*[>]"),""); // clean up content
	  String fshortname = fname.after("/",-1);
	  if (fshortname == "") fshortname = fname;
	  ostr << "<LI>" << ncont << " [<A HREF=\"" << relurl(paperplanurl,fname) << '#' << nid << "\">" << fshortname << '#' << nid << "</A>]<BR>\n"; // return content
	  //*** selection criteria suggestions can be automatically generated (see DIL#20000310091924.1)
	  ostr << "(<B>Context</B>: " << cstr << ", <B>Importance</B>: " << istr << ", <B>Length</B>: " << lstr << ", <B>Section</B>: " << sstr << ")\n"; // add selection criteria
	}
      }
    }
  } // else may already have HREF reference to paper in which it is included
  return true;
}

bool cmdline_extract_paper_novelties(String filepaper) {
// Commandline interface to the extraction of NOVELTY tagged items
  // **not used** bool res;
  String fname, paperplanurl;
  fname = filepaper.before("+");
  if (fname == "") {
    fname = filepaper;
    paperplanurl = homedir+"doc/tex/somepaper/somepaper.html"; // dummy URL
    // **not used** res = extract_paper_novelties(fname,paperplanurl,cout);
    extract_paper_novelties(fname,paperplanurl,cout);
  } else {
    paperplanurl = filepaper.after("+");
    if (paperplanurl == "") {
      paperplanurl = homedir+"doc/tex/somepaper/somepaper.html"; // dummy URL
      // **not used** res = extract_paper_novelties(fname,paperplanurl,cout);
      extract_paper_novelties(fname,paperplanurl,cout);
    } else {
      if (paperplanurl[0]!='/') paperplanurl.prepend(homedir);
      ofstream ostr(paperplanurl+".new");
      if (!ostr) {
	EOUT << "dil2al: Unable to create " << paperplanurl << ".new in cmdline_extract_paper_novelties()\n";
	return false;
      }
      ifstream istr(paperplanurl);
      bool isnew = (!istr);
      if (!isnew) ostr << istr.rdbuf(); // copy existing content (without regard to a possible </BODY> tag)
      istr.close();
      // **not used** res = extract_paper_novelties(fname,paperplanurl,ostr);
      extract_paper_novelties(fname,paperplanurl,ostr);
      ostr.close();
      if (!backup_and_rename(paperplanurl,"Paper Plan",isnew)) return false;
    }
  }
  return true;
}

void TL_entry_content::TL_get_chunk_data(String & tlstr, int tlloc) {
  // This is used by several initialization functions.
  
  // confirm chunk AL data location
  int chunkALstartloc = tlstr.index("<!-- chunk AL",tlloc);
  if ((chunkALstartloc<0) || (chunkALstartloc>=((int) tlstr.length()))) return;
  // confirm chunk Context data location
  int chunkContextstartloc = tlstr.index("<!-- chunk Context",chunkALstartloc);
  if ((chunkContextstartloc<0) || (chunkContextstartloc>=((int) tlstr.length()))) return;
  // confirm HTML text location
  int chunkstartloc = tlstr.index('\n',chunkContextstartloc) + 1;
  if ((chunkstartloc<=0) || (chunkstartloc>=((int) tlstr.length()))) return;
  
  // get chunk AL data
  String ALdatastr(tlstr.at(chunkALstartloc,chunkContextstartloc-chunkALstartloc)); // minimize String to manipulate
  al.file=ALdatastr.after('"'); al.title=al.file.after('>'); alprev.file=al.title.after('('); alnext.file=alprev.file.after('/');
  al.file=al.file.before('"'); al.title=al.title.before('<'); alprev.file=alprev.file.before('/');
  alprev.file=alprev.file.after('"');
  if (!alprev.file.empty()) {
    alprev.file=alprev.file.before('"'); alprev.title=alprev.file.after('#'); alprev.file=alprev.file.before('#');
  }
  alnext.file=alnext.file.after('"');
  if (!alnext.file.empty()) {
    alnext.file=alnext.file.before('"'); alnext.title=alnext.file.after('#'); alnext.file=alnext.file.before('#');
  }
  
  // get chunk Context data
  String Contextdatastr(tlstr.at(chunkContextstartloc,chunkstartloc-chunkContextstartloc)); // minimize String to manipulate
  dil.file=Contextdatastr.after('"'); dilprev.file=dil.file.after('('); dilnext.file=dilprev.file.after('/');
  dil.file=dil.file.before('"'); dil.title=dil.file.after('#'); dil.file=dil.file.before('#'); dilprev.file=dilprev.file.before('/');
  dilprev.file=dilprev.file.after('"');
  if (!dilprev.file.empty()) {
    dilprev.file=dilprev.file.before('"'); dilprev.title=dilprev.file.after('#'); dilprev.file=dilprev.file.before('#');
  }
  dilnext.file=dilnext.file.after('"');
  if (!dilnext.file.empty()) {
    dilnext.file=dilnext.file.before('"'); dilnext.title=dilnext.file.after('#'); dilnext.file=dilnext.file.before('#');
  }
  
  // get HTML text
  int chunkendloc = tlstr.index("<!-- chunk End",chunkstartloc);
  if (chunkendloc<0) chunkendloc = tlstr.length(); // to end of file if no marks are found
  if (chunkendloc>chunkstartloc) htmltext = tlstr.at(chunkstartloc,chunkendloc-chunkstartloc);
  
  // get chunk end time
  // NOTE: Do exit for bad chunk start time stamp. The NAME reference in source.title should be a valid time stamp.
  _chunkstarttime = time_stamp_time(source.title);
  // NOTE: Do not exit for bad chunk end time stamp. It's used here to detect an unclosed/unfinished chunk.
  if ((chunkendloc+33)<static_cast<int>(tlstr.length())) chunkendtime = time_stamp_time(tlstr.at(chunkendloc+21,12),true); // noexit=true (20190127)
}

TL_entry_content::TL_entry_content(String & tlstr, int tlloc, filetitle_t & entryref): source(entryref), chunkendtime(-1) {
  ischunk = !source.title.contains('.');
  if (ischunk) {
    // get all data from Begin to End of chunk
    TL_get_chunk_data(tlstr,tlloc);
  } else {
    // get all data from entry location to next entry or end of chunk
    int entrystartloc = tlstr.index('\n',tlloc) + 1;
    if ((entrystartloc<=0) || (entrystartloc>=((int) tlstr.length()))) return;
    int entryContextloc = tlstr.index("<!-- entry Context",entrystartloc);
    int nextentrybeginloc = tlstr.index("<!-- entry Begin",entrystartloc);
    int nextchunkendloc = tlstr.index("<!-- chunk End",entrystartloc);
    // figure out end of entry text
    int entryendloc = tlstr.length(); // to end of file if no marks are found
    if ((nextentrybeginloc>=0) || (nextchunkendloc>=0)) {
      if (nextentrybeginloc<0) entryendloc = nextchunkendloc;
      else if (nextchunkendloc<0) entryendloc = nextentrybeginloc;
      else if (nextchunkendloc<nextentrybeginloc) entryendloc = nextchunkendloc;
      else entryendloc = nextentrybeginloc;
    }
    // determine if this entry has reference links
    if ((entryContextloc>=0) && (entryContextloc<entryendloc)) {
      entrystartloc = tlstr.index('\n',entryContextloc) + 1;
      String Contextdatastr(tlstr.at(entryContextloc,entrystartloc-entryContextloc)); // minimize String to manipulate
      dil.file=Contextdatastr.after('"'); dilprev.file=dil.file.after('('); dilnext.file=dilprev.file.after('/');
      dil.file=dil.file.before('"'); dil.title=dil.file.after('#'); dil.file=dil.file.before('#'); dilprev.file=dilprev.file.before('/');
      dilprev.file=dilprev.file.after('"');
      if (!dilprev.file.empty()) {
	dilprev.file=dilprev.file.before('"'); dilprev.title=dilprev.file.after('#'); dilprev.file=dilprev.file.before('#');
      }
      dilnext.file=dilnext.file.after('"');
      if (!dilnext.file.empty()) {
	dilnext.file=dilnext.file.before('"'); dilnext.title=dilnext.file.after('#'); dilnext.file=dilnext.file.before('#');
      }      
    }
    if (entryendloc>entrystartloc) htmltext = tlstr.at(entrystartloc,entryendloc-entrystartloc);
  }
}

TL_entry_content::TL_entry_content(String & tlstr, int tlchunkbegin, int tlchunkend, filetitle_t & entryref): source(entryref), ischunk(true), chunkendtime(-1) {
  // This initialization is for chunks only, not for single entries.
  // *** NOTE: This doesn't appear to use tlchunkend at all!
  // NOTE: While chunkendtime is explicitly set, chunk start time is
  // implicitly available through source.title.
  
  // get source
  if (source.file.contains('/')) source.file = source.file.after('/',-1);
  if (source.title.empty()) {
    int lineendloc = tlstr.index('\n',tlchunkbegin);
    source.title = tlstr.at(tlchunkbegin,lineendloc-tlchunkbegin);
    source.title = source.title.after("NAME=\"");
    source.title = source.title.before('"');
  }
  // get all data from Begin to End of chunk
  TL_get_chunk_data(tlstr,tlchunkbegin);
}

TL_entry_content * Task_Log::get_task_log_entry(filetitle_t * entryref) {
  // Takes an entry reference with file=<TL-file>, title=<entry-id>.
  // Makes sure the requisite task log file content is available.
  // Finds the entry, which can be an entire chunk.
  // Extracts data from the entry and puts it into a TL_entry_content structure.
  if (!entryref) return NULL;
  //VOUT << "\n\nRECEIVED TL ENTRY REFERENCE FILE:" << entryref->file << '\n';
  //VOUT << "RECEIVED TL ENTRY REFERENCE NAME:" << entryref->title << "\n\n";
  int tlidx,tlloc;
  // get the right task log file
  String TLfile(basedir+RELLISTSDIR+entryref->file);
  if ((tlidx=get_file_in_list(TLfile,tf,tfname,tfnum))<0) return NULL;
  // find the task log chunk or entry reference
  String tlreference("NAME=\""+entryref->title+'"');
  if ((tlloc=tf[tlidx].index(tlreference))<0) {
    ERROR << "Task_Log::get_task_log_entry(): Missing chunk/entry reference " << entryref->title << " in TL file " << TLfile << ".\n";
    return NULL;
  }
  // obtain content
  TL_entry_content * TLec = new TL_entry_content(tf[tlidx],tlloc,*entryref);
  return TLec;
}

long collectedTLminutes = 0;

time_t TL_entry_content::chunk_seconds() {
  if (source.title.empty()) return 0;
  //time_t chunkstarttime = time_stamp_time(source.title); // *** Probably correct with noexit=false (20190127).
  if (_chunkstarttime<=chunkendtime) return chunkendtime-_chunkstarttime;
  return 0;
}

String TL_entry_content::str() {
  // Produces an HTML formatted string intended for the efficiently viewable
  // collection of logged task history.
  String outputstr("<p>\n");
  if (ischunk) outputstr += "<B>C</B>: ";
  else outputstr += "<B>e</B>: ";
  outputstr += HTML_put_href(source.file+'#'+source.title,"source");
  // *** skipping output of al data here, since it seems to be always identical at present
  // *** skipping AL prev and next for now, since that can be found at the source
  // *** skipping DIL prev and next for now, since that should already be in preceding and following output text
  if (ischunk) {
    outputstr += " [" + source.title + ',';
    outputstr += time_stamp("%Y%m%d%H%M",chunkendtime) + "] ";
    time_t chunkseconds = chunk_seconds();
    long chunkminutes = chunkseconds/60;
    collectedTLminutes += chunkminutes;
    long chunkhours = chunkminutes/60;
    chunkminutes -= (chunkhours*60);
    outputstr += "<B>" + String(chunkhours) + ':' + String(chunkminutes) + "</B>";
  }
  // if ischunk add start and end time, and time taken (also sum those)
  outputstr += '\n'+htmltext;
  outputstr += "</p>\n";
  return outputstr;
}

//int test = 0;

/*
To find @A2C=X.Y@ and similar data labels in Task Chunks, we reuse the
TL_entry_content class and modified copies of some of its functions,
such as the metrics() function.

PRECEDENCE:
1. @label=x.y@ search is carried out first. It is in addition to
   following metrics steps, not instead of. (It does not cause
   TL_entry_content::metrics() to return.)
2. @CATEGORY:<category>@ is 2nd and has highest priority among
   exclusive metrics steps. Specifying the category this way
   within a Task Log Chunk overrules all other category rules.
   If the tag is found the category is one of those in the
   list of categories then TL_entry_content::metrics() returns.
3. Category by Task ID is 3rd. If the Chunk's Task ID is found
   to belong to a category in the list of categories then
   TL_entry_content::metrics() returns.
4. Category by DIL file (e.g. family.html) is 4th. If one of
   the categories in the list of categories includes the DIL
   file of the Chunk's Task, then that is chosen and
   TL_entry_content::metrics() finishes.
5. The "other" category is 5th. If it is explicitly specified
   in the list of categories then it is assumed that it is
   specified last. (It will otherwise be added to the list
   automatically.) If none of the above applied then the
   Chunk is assigned to this "other" category.

Note that because each Task Log Chunk is checked once and allocated to
one category, the sum across categories in one day can never be more
than 24 hours, and a Chunk cannot belong to multiple categories. The
categorization is unique. If overlapping sets are desired for some
report then first create many small categories and then combine some
in post-processing to account for overlap areas.

GOALS:

The first order of business is to find the data and to report the data
somehow. It can then be analyzed and shown in some way by other
software.

There are several options concerning how the data can be collected,
e.g.:

1) Stick with the Metrics_Days() format and compile the data by,
   a) linearly adding it, weighted by the duration of the Task Chunk
      in which the data was found, or
   b) creating categories such as "A2C=-1.0", "A2C=1.0" and adding
      the duration of the task chunk in which such a value was found
      for the label to the corresponding category for that day.

2) Create a smaller chunk granularity (e.g. 20 minutes, 5 minutes,
   1 minute) and for that granularity output the value found.

3) For each Task Chunk, simply output the value found and the chunk
   start and end (and/or duration) times.

In all cases, a non-number value can be output when output is needed
for some chunk/duration but no value was found.

In a case such as 1a, if the output chunk includes periods with no
found values then it may be most useful and honest to invalidate the
value for the whole output chunk and send a non-number value.

Simply, because we presently (20140604) have no output data for
self-measure performance values such as @A2C=X.Y@ at all, and
because it is the easiest (quickest) adaptation of the existing
code, I begin by implementing option 1a.

Around 20190127, several of the points above may have been
addressed or subsumed by the new -zMAP mode.
*/

void TL_entry_content::metrics_labelid(Metrics_Days & MD, String & labelid) {
  // Find a possible instance of @labelid=X.Y@ within the HTML text content
  // of a Task Chunk.
  // If a valid instance is found then add its value to its category in that day.
  // If not then that day's data for that category is set to 'invalid'.
  // This means that the labelid needs to be found for every chunk in a day to
  // have a valid value for that day. It is a metric that is calculated over all
  // Task Chunks in a day.
  //
  // Update 20191225: Seeking to reuse this function for progress hours
  // accumulation with the ACTTYPE label, I am adding a flag called
  // labeled_metric_strict_invalidate. This flag will be used to turn off
  // the strict requirement where NOT finding the labelid in the text of
  // a chunk leads to immediate invalidation of the data for that
  // category for all days into which the chunk spans.
  // Documentation is unfortunately too sparse and it is time consuming to
  // reconstruct all of the reasoning that went into the strict test. I do
  // think it was intended for use with the A2C label, and that the A2C label
  // used to be expected for every chunk. This could be investigated further.
  // For more, see TL#201912241759.
  
  Metrics_Category * mc = MD.MC->get_category_by_name(labelid);
  if (!mc) return;
  int labeledtextloc = htmltext.index('@'+labelid+'=');
  if (labeledtextloc<0) {
    if (labeled_metric_strict_invalidate) mc->invalidate_chunk(this,MD); // see Update in comments above
    return;
  }
  labeledtextloc += 2 + labelid.length();
  int labeledtextend = htmltext.index('@',labeledtextloc);
  if (labeledtextend<0) { // no valid value found (broken labeled entry)
    mc->invalidate_chunk(this,MD);
    return;
  }
  String metricvaluestr = htmltext.at(labeledtextloc, labeledtextend-labeledtextloc);
  // *** The following line needs more sophistication to deal with @ACTTYPE=A100@.
  // *** - add a test for a letter, and collect for letter categories
  // *** - add a test for the matricsareratios parameter to see if percentages
  //double metricvalue = atof(metricvaluestr.chars());
  double metricvalue = 0.0;
  if (!metricvaluestr.empty()) {
    //cout << metricvaluestr << "\n";
    if ((metricvaluestr[0]<'0') || (metricvaluestr[0]>'9')) {
      // found letter indicating label sub-type
      // *** we don't know what to do with this yet, but it should count the
      //     task chunk time towads the label subtype category
      //     (of course, for ACTTYPE, we really only want to separately
      //     accumulate for "[A]ctive".
      // then skip that letter
      metricvaluestr.del(0,1);
    }
    //cout << metricvaluestr << "\n";
    if (metrics_are_ratios) metricvalue = atof(metricvaluestr.chars());
    else metricvalue = ((double) atoi(metricvaluestr.chars()))/100.0;
    //cout << metricvalue << "\n";
  }
  mc->add_labeled_chunk(this,MD,metricvalue);
}

void TL_entry_content::metrics(Metrics_Days & MD, String & labelid) {
  // Try to add the data about this Task Chunk (e.g. duration of Chunk) to a
  // specified category. Categorization precedence is as described in
  // PRECEDENCE comments above.
  
  //if (test<50) VOUT << dil.file << " (" << dil.file.length() << ")\n";
  //test++;
  
  // Check for category related tags in TL entry text
  if (!htmltext.empty()) {
    // Look for additional labeled metrics data @labelid=X.Y@ unless -zMAP
    if ((!labelid.empty()) && (!MD.mapit)) { 
      metrics_labelid(MD,labelid);
    }
    // Look for @CATEGORY:<category>@ tag 
    int categorytextloc = htmltext.index("@CATEGORY:");
    if (categorytextloc>=0) {
      // *** If it seems worthwhile, I can further detail this by allowing percentages to be added
      //     for multiple categories, which are then each called with proportions to multiply time by.
      categorytextloc += 10;
      int categorytextend = htmltext.index('@',categorytextloc);
      if (categorytextend>=0) {
	String category(htmltext.at(categorytextloc,categorytextend-categorytextloc));
	Metrics_Category * mc = MD.MC->get_category_by_name(category);
	if (mc) mc->add_chunk(this,MD);
      }
    }
  }
  
  // Check if this Task Chunk is specifically categorized by DIL Entry ID
  if (!dil.title.empty()) {
    PLL_LOOP_FORWARD(Metrics_Category,MD.MC->list.head(),1) {
      if (e->DE.iselement(dil.title)>=0) {
	e->add_chunk(this,MD);
	return;
      }
    }
  }
  
  // Check if this Task Chunk is categorized by DIL File name
  PLL_LOOP_FORWARD(Metrics_Category,MD.MC->list.head(),1) {
    if (e->DIL.iselement(dil.file)>=0) {
      e->add_chunk(this,MD);
      return;
    }
  }

  // Lastly, if the "other" category was appended then categorize the Task Chunk there
  // *** NOTE: If I used MD.MC->get_category_by_name("other") then it wouldn't need
  //     to be at the end of the list of categories.
  if (MD.MC->list.tail()->category=="other") MD.MC->list.tail()->add_chunk(this,MD);
}

bool Task_Log::preceding_task_log_section_filename(String & tfstr, String & prevTLfilename) {
  // Looks for the filename of the preceding Task Log section, according to
  // the required tags.
  // The Task Log section to search must already be buffered at tfstr.
  // Returns true if found and returns the filename in prevTLfilename.
  // Returns false if the tags or the filename could not be found.
  int prevsectionloc = tfstr.index("previous TL section");
  if (prevsectionloc<0) return false;
  prevTLfilename = tfstr.before(prevsectionloc);
  prevTLfilename = prevTLfilename.after("<A HREF=\"",-1);
  if (prevTLfilename.empty()) return false;
  prevTLfilename = prevTLfilename.before('"');
  if (prevTLfilename.empty()) return false;
  // prevTLfilename contains the filename of the preceding Task Log section
  return true;
}

#define _DEBUG_GETPREVIOUSTASKLOGENTRY_A(DebugLabel,SeekSpec,ChunkBegin,ChunkEnd) \
  do { if (DEBUGTLFILTER_A) VOUT << "DBG " << DebugLabel << ": " << chunkseekloc << '=' << prevchunkbeginloc << ':' << prevchunkendloc << '\n'; } while (0)

#define _DEBUG_GETPREVIOUSTASKLOGENTRY_B(DebugLabel) \
  do { if (DEBUGTLFILTER_B) { \
	 debug_showchunks--; \
         if (debug_showchunks>=0) {   \
	   VOUT << "DBG " << DebugLabel << ": " << "_chunkstarttime=" << TLec->_chunkstarttime << " (" << TLec->source.title << ")   "; \
	   VOUT << "chunkendtime=" << TLec->chunkendtime << " (" << time_stamp("%Y%m%d%H%M",TLec->chunkendtime) << ")   "; \
           VOUT << "chunk duration=" << TLec->chunkendtime-TLec->_chunkstarttime << " seconds\n"; \
         } \
    } } while (0)

TL_entry_content * Task_Log::get_previous_task_log_entry() {
  // Finds the preceding Task Log Chunk based on the current position of the
  // index 'chunkseekloc'. The value of chunkseekloc must be negative.
  // If found then a new TL_entry_content object is created that is initialized
  // with data about the Task Chunk. That object is returned.
  // Returns NULL if:
  // - the Task Log file was unable to be loaded and buffered as needed
  // - no properly specified previous Task Chunk was found in the TL file
  //   and no preceding TL section was found
  // - a preceding TL section could not be loaded and buffered
  // Exits with error message if:
  // - chunkseekloc>=0
  // - a Task Log section appears to contain no Task Chunks
  // - a Task Chunk does not have proper Begin and End tags
  // NOTE: This Task_log object is initialized according to the  TL file
  // specified in the global 'tasklog' variable and with chunkseekloc=-1 to start
  // from the end of the file.

  if (chunkseekloc>=0) {
    ERROR << "Task_Log::get_previous_task_log_entry(): Backwards seeking not possible with positive chunkseekloc!\n";
    Exit_Now(1);
  }

  // Get the Task Log file in an indexed buffer
  int tlidx;
  if ((tlidx=get_file_in_list(chunktasklog,tf,tfname,tfnum))<0) return NULL;
  String& tfstr = tf[tlidx]; // declaring a reference for clarity
  
  // Find the preceding Task Chunk begin and end tags
  int prevchunkendloc = tfstr.index("<!-- chunk End",chunkseekloc);
  int prevchunkbeginloc = tfstr.index("<!-- chunk Begin",chunkseekloc);
  _DEBUG_GETPREVIOUSTASKLOGENTRY_A("get_previous_task_log_entry()",chunkseekloc,prevchunkbeginloc,prevchunkendloc);

  // If at least one of the tags is missing then fetch the preceding Task Log file.
  // (Note that Task Chunks may never be broken over multiple Task Log files.)
  if ((prevchunkendloc<0) || (prevchunkbeginloc<0)) {
    String prevTLfilename;
    if (!preceding_task_log_section_filename(tfstr,prevTLfilename)) return NULL;
    if (verbose) VOUT << "Now parsing " << prevTLfilename << '\n';
    chunktasklog = basedir+RELLISTSDIR+prevTLfilename;
    chunkseekloc = -1; // start at the end of the file again

    // Get the preceding Task Log file in an indexed buffer
    if ((tlidx=get_file_in_list(chunktasklog,tf,tfname,tfnum))<0) return NULL;
    String& tf2str = tf[tlidx]; // a new reference for clarity

    // Try again to find the preceding Task Chunk begin and end tags
    prevchunkendloc = tf2str.index("<!-- chunk End",chunkseekloc);
    prevchunkbeginloc = tf2str.index("<!-- chunk Begin",chunkseekloc);
    _DEBUG_GETPREVIOUSTASKLOGENTRY_A("get_previous_task_log_entry()-A",chunkseekloc,prevchunkbeginloc,prevchunkendloc);

    // If at least one of the tags is missing AGAIN then there is something seriously
    // wrong with the Task Log section. Exit here with an error message! This problem
    // will need to be addressed separately first.
    if ((prevchunkendloc<0) || (prevchunkbeginloc<0)) {
      ERROR << "Task_Log::get_previous_task_log_entry(): The Task Log file " << chunktasklog << " appears to have no Task Chunks!\n";
      Exit_Now(1);
    }
  }
  
  // A properly defined Task Chunk always has both a Begin tag and an End tag,
  // even if the Chunk End Time has not yet been filled in. Exit here with an
  // error message! This problem should be addressed first to insure that
  // Task Log content is uncorrupted.
  if (prevchunkendloc<=prevchunkbeginloc) {
    ERROR << "Task_Log::get_previous_task_log_entry(): The Task Log file " << chunktasklog << " contains at least one Task Chunk without proper Begin and End tags!\n";
    Exit_Now(1);
  }

  // Create new TL_entry_content object for the Task Chunk found
  // NOTE: We have to use tf[tlidx] here, because we might need either tfstr or tf2str.
  filetitle_t chunkref; chunkref.file = chunktasklog;
  TL_entry_content * TLec = new TL_entry_content(tf[tlidx],prevchunkbeginloc,prevchunkendloc,chunkref);
  _DEBUG_GETPREVIOUSTASKLOGENTRY_B("get_previous_task_log_entry()-B");
  
  // Update chunkseekloc, negative for backwards seeking
  chunkseekloc = prevchunkbeginloc-tf[tlidx].length();
  
  return TLec;
}

bool get_collected_task_entries(String dilidstr, String collectionfile) {
  // This function can be called as soon as a task is selected.
  // It collects and provides previous task log entries related to that task,
  // which is a useful overview in order to efficiently continue work on a task.
  // The TL head pointer of a DIL entry is used to find most recent TL entry content.
  // The content is added to an output string, including a link to the original location of the entry.
  // Pointers to preceding entries are used to find more relevant content.
  // The output is made available in a temporary file.
  // Other useful data about the task can also be displayed, as obtained from the DIL_entry data in the Detailed_Items_List (found using DIL_entry::elbyID()).
  // Good places to call this from may be:
  // - in decide_add_TL_chunk() (tladmin.cc), if "newchunk" is true
  // - in add_TL_chunk_or_entry() (tladmin.cc), probably from here, since that function provides "newdilref"
  // - in chunk_controller() (controller.cc), when a chunkid is selected
  // Create a DIL_ID
  DIL_ID dilid(dilidstr);
  // Load the Detailed_Items_List
  Detailed_Items_List dilist;
  dilist.Get_All_DIL_ID_File_Parameters();
  dilist.Get_All_Topical_DIL_Parameters(true); // we need this for the pointers to entries
  if (!dilist.list.head()) return false;
  // Find the DIL_entry by ID
  DIL_entry * de = dilist.list.head()->elbyid(dilid);
  if (!de) return false;
  String dedilfile(idfile);
  if (de->parameters) if (de->parameters->topics.head()) dedilfile=de->parameters->topics.head()->dil.file;
  String outputstr("<HTML>\n<HEAD>\n<TITLE>Collected Task History for DIL#"+dilidstr+"</TITLE>\n</HEAD>\n<BODY>\n<H1>Collected Task History for <A HREF=\""+dedilfile+'#'+dilidstr+"\">DIL#"+dilidstr+"</A></H1>\n\n<HR>\n\n");
  // DIL entry information
  outputstr += "target date = <B>" + time_stamp("%Y%m%d%H%M",de->Target_Date()) + "</B>";
  String registeredtimespent;
  if (de->content) {
    if (de->Is_Plan_Entry()==PLAN_ENTRY_TYPE_OUTCOME) outputstr += "<B>desirability</B> = " + String((double) de->content->PLAN_OUTCOME_DESIRABILITY,"%.2f");
    else outputstr += ", valuation = " + String((double) de->content->valuation,"%.2f");
    outputstr += ", completion ratio = " + String((double) de->content->completion,"%.2f");
    outputstr += ", time required = " + String((double) (de->content->required/3600.0),"%.2f");
    registeredtimespent = "\nRegistered time spent on this task: <B>" + String((double) (de->content->completion*de->content->required/3600.0),"%.3f") + "</B><BR>\n";
  }
  outputstr += "<P>\n";
  if (de->Entry_Text()) {
    String textstr(""),hrefurl,hreftext; int hrefstart,hrefend = 0,hrefnextend; 
    while ((hrefnextend=HTML_get_href(*(de->Entry_Text()),hrefend,hrefurl,hreftext,&hrefstart))>=0) {
      if (hrefstart>hrefend) textstr += de->Entry_Text()->at(hrefend,hrefstart-hrefend);
      hrefurl = absurl(basedir+RELLISTSDIR,hrefurl);
      textstr += HTML_put_href(hrefurl,hreftext);
      hrefend = hrefnextend;
    }
    if (((int) (de->Entry_Text()->length()))>hrefend) textstr += de->Entry_Text()->at(hrefend,de->Entry_Text()->length()-hrefend);
    // Identify possible delegation of task work to a working group (WG). [See diladmin.cc:generate_modify_element_FORM_interface_DIL_entry_content().]
    // If so then time required represents time required to manage the task that is being carried out by the working group.
    int wgidx = -1;
    if ((wgidx=textstr.index("{WG="))>=0) {
      int wgend = -1;
      if ((wgend=textstr.index('}',wgidx))<wgidx) {
	outputstr + "working group = <B>INCOMPLETE DATA</B><BR>\n";
      } else {
	String wg(textstr.at(wgidx+4,wgend-(wgidx+4)));
	outputstr += "orking group = <B>" + wg + "</B> (time required is management time)<BR>\n";
	textstr.del(wgidx,(wgend+1)-wgidx);
      }
    }
    // *** Could put here: Identify possible relevant contacts specific to this task (RC).
    outputstr += textstr + "<P>\n";
  }
  // Obtain the TL head pointer
  filetitle_t * TLhead = de->TL_head_ref();
  if (!TLhead) return false;
  // Use get_task_log_entry() to obtain entry contents
  // *** Note: It is fairly easy to switch to earliest at top, most recent at bottom order by starting at tail and using dilnext.
  Task_Log TaskLog;
  collectedTLminutes = 0;
  TL_entry_content * TLentrycontent = TaskLog.get_task_log_entry(TLhead);
  String contentstr;
  for (int i=0; ((TLentrycontent) && (i<10000)); i++) { // for-loop prevents getting stuck in endless loops
    contentstr += TLentrycontent->str();
    TL_entry_content * TLpreventrycontent = NULL;
    if (!TLentrycontent->dilprev.title.empty()) {
      if (TLentrycontent->dilprev.file.empty()) TLentrycontent->dilprev.file=TLhead->file;
      TLpreventrycontent = TaskLog.get_task_log_entry(&(TLentrycontent->dilprev));
    }
    delete TLentrycontent;
    TLentrycontent = TLpreventrycontent;
  }
  long collectedTLhours = collectedTLminutes/60;
  collectedTLminutes -= (collectedTLhours*60);
  outputstr += registeredtimespent;
  outputstr += "<B>Collected time over all task chunks for this task</B>: <B>" + String(collectedTLhours) + ':' + String(collectedTLminutes); 
  outputstr += "</B>\n<P>\n<HR>\n<P>\n" + contentstr;
  // Write output string to temporary file for display
  outputstr += "\n\n</BODY>\n</HTML>\n";
  write_file_from_String(collectionfile,outputstr,"Current Task History",true);
  return true;
}

// A default CMYK 2-bit per color color map for categories.
unsigned char metricsmapcol[16] =
  { 0xC5, 0xCC, 0x89, 0xA1,
    0x0C, 0x1E, 0x3C, 0x38,
    0x55, 0x2C, 0x48, 0x97,
    0x30, 0x3F, 0x00, 0xFF};

Metrics_Categories::Metrics_Categories(String categoriesfile, String labelid) {
  const BigRegex categoryrx(".*:[ \t\v\f]*$");
  const BigRegex categorycomment("^[ \t\v\f]*#.*$");
  String categoriesdatastr;
  if (read_file_into_String(categoriesfile,categoriesdatastr)) {
    StringList datalines(categoriesdatastr,'\n');
    int numlines = datalines.length();
    // Find categories and specific DIL Entries (DE) or DIL Files (DIL) belonging to them.
    Metrics_Category * mc = NULL; int categorynumber = 0, DILnumber = 0, DEnumber = 0;
    for (int i=0; i<numlines; i++) {
      if ((datalines[i].empty()) || (datalines[i].matches(BRXwhite))) continue; // skip empty lines and whitespace
      if ((datalines[i][0]=='#') || (datalines[i].matches(categorycomment))) continue; // skip comment lines
      if (datalines[i].matches(categoryrx)) { // new category
	String categoryname(datalines[i].before(':'));
	// if a category label value for -zMAP is given (e.g. =0xC5) then update metricsmapcol
	String categorymaplabel(categoryname.after('='));
	if (!categorymaplabel.empty()) {
	  categoryname = categoryname.before('=');
	  unsigned long catmaplabel = strtoul(categorymaplabel,NULL,0); // non-number cause 0 value
	  metricsmapcol[categorynumber % 16] = (unsigned char) catmaplabel;
	  if (verbose) INFO << '(' << (long) catmaplabel << ") ";
	}
	if (verbose) INFO << "category: " << categoryname << '\n';
	mc = new Metrics_Category(categoryname,categorynumber);
	categorynumber++;
	DILnumber = 0; DEnumber = 0;
	list.link_before(mc);
      } else { // if not empty another memory for category
	if (mc) {
	  if ((datalines[i][0]>='0') && (datalines[i][0]<='9') && (datalines[i].length()>=16)) { // specific task
	    mc->DE[DEnumber] = datalines[i].before(16); // ignores following comments, notes, etc.
	    DEnumber++;
	  } else { // task topic
	    mc->DIL[DILnumber] = datalines[i];
	    DILnumber++;
	  }
	  //VOUT << "DIL=" << datalines[i] << " (" << datalines[i].length() << ")\n";
	}
      }
    }
    // add the "other" category
    if (!list.tail()) {
      list.link_before(new Metrics_Category("other",categorynumber));
      categorynumber++;
      if (verbose) INFO << "category: other\n";
    } else if (!(list.tail()->category.matches("other"))) {
      list.link_before(new Metrics_Category("other",categorynumber));
      categorynumber++;
      if (verbose) INFO << "category: other\n";
    }
    // add a possible labeled category as indicated by labelid (20140604)
    if (!labelid.empty()) {
      if (labelid=="MAP") {
	if (verbose) INFO << "MAP option: Including days map output at " << metricsmapfile << '\n';
      } else {
	if (verbose) INFO << "labeled category: " << labelid << '\n';
	mc = new Metrics_Category(labelid,categorynumber);
	categorynumber++;
	DILnumber = 0; DEnumber = 0;
	list.link_before(mc);
      }
    }
  }
  nummetricscategories = list.length();
  if ((nummetricscategories>15) && (verbose)) WARN << "More than 15 categories defined, but metricmapcol specifies only 15+1 codes.\n";
}

Metrics_Category * Metrics_Categories::get_category_by_name(const char * c) {
  // returns NULL if not found
  if (!c) return NULL;
  PLL_LOOP_FORWARD(Metrics_Category,list.head(),1) {
    if (e->category==c) return e;
  }
  return NULL;
}

Metrics_Category * Metrics_Categories::get_category_by_number(int cnum) {
  // returns NULL if not found
  PLL_LOOP_FORWARD(Metrics_Category,list.head(),1) {
    if (e->categorynumber==cnum) return e;
  }
  return NULL;
}

int Metrics_Categories::get_category_number_by_name(const char * c) {
  // returns -1 if not found
  if (!c) return -1;
  PLL_LOOP_FORWARD(Metrics_Category,list.head(),1) {
    if (e->category==c) {
      if (((unsigned int) e->categorynumber)>=nummetricscategories) return -1;
      else return e->categorynumber;
    }
  }
  return -1;
}

unsigned char Metrics_Categories::get_map_code_by_name(const char * c) {
  // Find the color map metricsmapcol code that is presently assigned to the
  // category with the label string c.
  // Returns 0xFF if c==NULL or if the category could not be found.
  if (!c) {
    if (verbose) WARN << "tlfilter.cc:Metrics_Categories::get_map_code_by_name(): Empty category label.\n";
    return 0xFF;
  }
  int cnum = get_category_number_by_name(c);
  if (cnum<0) {
    if (verbose) WARN << "tlfilter.cc:Metrics_Categories::get_map_code_by_name(): " << c << " category not found.\n";
    return 0xFF;
  }
  return metricsmapcol[cnum % 16];
}

String Metrics_Categories::csv_String() {
  String csvstring;
  PLL_LOOP_FORWARD(Metrics_Category,list.head(),1) {
    csvstring += e->csv_String();
    if (e==list.tail()) csvstring += '\n';
    else csvstring += ',';
  }
  return csvstring;
}

String Metrics_Categories::categories_rc_String() {
  // Reproduces the categories definitions as initialized.
  // This can be used to confirm the .rc file that was
  // used to produce category metrics.

  String rcstr;
  PLL_LOOP_FORWARD(Metrics_Category,list.head(),1) {
    rcstr += e->category_rc_String();
    if (e!=list.tail()) rcstr+= '\n';
  }
  return rcstr;
}

#define _DEBUG_MCADDCHUNK_B(DebugLabel) \
  do { if (DEBUGTLFILTER_B) { \
         if (debug_showchunks>=0) {   \
	   VOUT << "DBG " << DebugLabel << ": " << time_stamp("%Y%m%d%H%M",startday) << ": added " << addtime << " seconds to category " << category << '\n'; \
         } \
    } } while (0)

#define _DEBUG_MCADDLABELEDCHUNK_B(DebugLabel)		\
  do { if (DEBUGTLFILTER_B) { \
         if (debug_showchunks>=0) {   \
	   VOUT << "DBG " << DebugLabel << ": " << time_stamp("%Y%m%d%H%M",startday) << ": added " << metricvalue << '*' << addtime << " seconds to category " << category << '\n'; \
         } \
    } } while (0)

void Metrics_Category::add_chunk(TL_entry_content * chunk, Metrics_Days & MD) {
  // Takes the chunk and adds the computed metrics about it to the
  // data in this category.
  // E.g. for the duration of the chunk it uses information about
  // the days and times when the task chunk took place.
  // With the -zMAP option this also calls add_to_day_map() for each day
  // that is touched or spanned by the Task Chunk, so that the daymap can
  // be populated accordingly.
  // If the Task Chunk does not have a properly specified chunkendtime
  // (as determined in TL_entry_content::TL_get_chunk_data()) then
  // it returns without any metrics updates.
  // This is called by TL_entry_content::metrics().
  if (chunk->chunkendtime<0) return;

  // get the start day
  time_t startday = time_stamp_time_date(chunk->source.title);
  time_t starttime = chunk->_chunkstarttime; // time_stamp_time(chunk->source.title); // *** Probably correct with noexit=false (20190127).
  time_t starttimeinday = starttime - startday;
  if (chunk->chunkendtime<=starttime) {
    // Task chunks that took less than a minute are not accumulated.
    // if (chunk->chunkendtime==starttime) VOUT << "Chunk start and end times are the same at " << chunk->source.title << ", skipping!\n";
    if (chunk->chunkendtime<starttime) if (verbose) WARN << "Beware: Chunk start time is later than chunk end time at " << chunk->source.title << "! Skipping it.\n";
    return;
  }
  if ((chunk->chunkendtime-starttime)>SECONDSPERDAY) if (verbose) INFO << "Notice: Time chunk larger than one day at " << chunk->source.title << '\n'; // This is possible, but it is nice to get a warning and check it anyway.
  // A Task Chunk can span time intervals in more than one day
  for (int daynum=0; ((daynum<1000) && (startday<chunk->chunkendtime)); daynum++) { // the daynum test prevents extremes
    time_t nextday = time_add_day(startday,1);
    time_t addtime = 0;
    if (nextday<chunk->chunkendtime) { // Task Chunk spans into nextday
      if (daynum==0) addtime = SECONDSPERDAY-starttimeinday; // Task Chunk starts in this day
      else addtime = SECONDSPERDAY; // Task Chunk spans this entire day
    } else { // Task Chunk ends within the day indicated by startday
      if (daynum==0) addtime = chunk->chunkendtime-starttime; // Task Chunk starts and ends in this day
      else addtime = chunk->chunkendtime-startday; // Task Chunk started on earlier day and ends in this day
    }
    //cout << time_stamp("%Y%m%d%H%M",chunk->_chunkstarttime) << " OTHER? " << addtime << "\n";
    MD.add_to_day(startday,(double) addtime,categorynumber);
    if (MD.mapit) MD.add_to_day_map(startday,starttime,chunk->chunkendtime,categorynumber); // -zMAP
    _DEBUG_MCADDCHUNK_B("Metrics_Category::add_chunk()-B");
    startday = nextday;
  }
}

void Metrics_Category::add_labeled_chunk(TL_entry_content * chunk, Metrics_Days & MD, double metricvalue) {
  // Takes a chunk and adds the computed metrics about it to those
  // of the explicit @labelid=X.Y@ category.
  // For example, this requires information about the days
  // and times when the task chunk took place.
  // get the start day
  // This is not called for -zMAP, i.e. when labelid=="MAP".
  if (chunk->chunkendtime<0) return;
  
  time_t startday = time_stamp_time_date(chunk->source.title);
  time_t starttime = chunk->_chunkstarttime; // time_stamp_time(chunk->source.title); // *** Probably correct with noexit=false (20190127).
  time_t starttimeinday = starttime - startday;
  if (chunk->chunkendtime<=starttime) {
    // Task chunks that took less than a minute are not accumulated.
    // if (chunk->chunkendtime==starttime) VOUT << "Chunk start and end times are the same at " << chunk->source.title << ", skipping!\n";
    if (chunk->chunkendtime<starttime) if (verbose) WARN << "Beware: Chunk start time is later than chunk end time at " << chunk->source.title << "! Skipping it.\n";
    return;
  }
  if ((chunk->chunkendtime-starttime)>SECONDSPERDAY) if (verbose) INFO << "Notice: Time chunk larger than one day at " << chunk->source.title << '\n'; // This is possible, but it is nice to get a warning and check it anyway.
  // A Task Chunk can span time intervals in more than one day
  for (int daynum=0; ((daynum<1000) && (startday<chunk->chunkendtime)); daynum++) { // the daynum test prevents extremes
    time_t nextday = time_add_day(startday,1);
    time_t addtime = 0;
    if (nextday<chunk->chunkendtime) { // Task Chunk spans into nextday
      if (daynum==0) addtime = SECONDSPERDAY-starttimeinday; // Task Chunk starts in this day
      else addtime = SECONDSPERDAY; // Task Chunk spans this entire day
    } else { // Task Chunk ends within the day indicated by startday
      if (daynum==0) addtime = chunk->chunkendtime-starttime; // Task Chunk starts and ends in this day
      else addtime = chunk->chunkendtime-startday; // Task Chunk started on earlier day and ends in this day
    }
    //cout << time_stamp("%Y%m%d%H%M",chunk->_chunkstarttime) << ": " << metricvalue << "*" << addtime << " (" << (addtime/60.0) << " mins) " << "#" << categorynumber << "\n";
    MD.add_to_day(startday,metricvalue*((double) addtime),categorynumber);
    _DEBUG_MCADDLABELEDCHUNK_B("Metrics_Category::add_labeled_chunk()-B");
    startday = nextday;
  }
}

void Metrics_Category::invalidate_chunk(TL_entry_content * chunk, Metrics_Days & MD) {
  // Takes a chunk and invalidates @labelid=X.Y@ category metrics within the chunk
  // due to missing data about such labelid metrics there.
  // For example, this requires information about the days
  // and times when the task chunk took place.
  // get the start day
  // This is not called for -zMAP, i.e. when labelid=="MAP".
  if (chunk->chunkendtime<0) return;
  
  time_t startday = time_stamp_time_date(chunk->source.title);
  time_t starttime = chunk->_chunkstarttime; // time_stamp_time(chunk->source.title); // *** Probably correct with noexit=false (20190127).
  //time_t starttimeinday = starttime - startday;
  if (chunk->chunkendtime<=starttime) {
    // Task chunks that took less than a minute are not accumulated.
    // if (chunk->chunkendtime==starttime) VOUT << "Chunk start and end times are the same at " << chunk->source.title << ", skipping!\n";
    if (chunk->chunkendtime<starttime) if (verbose) WARN << "Beware: Chunk start time is later than chunk end time at " << chunk->source.title << "! Skipping it.\n";
    return;
  }
  if ((chunk->chunkendtime-starttime)>SECONDSPERDAY) if (verbose) INFO << "Notice: Time chunk larger than one day at " << chunk->source.title << '\n'; // This is possible, but it is nice to get a warning and check it anyway.
  // A Task Chunk can span time intervals in more than one day
  for (int daynum=0; ((daynum<1000) && (startday<chunk->chunkendtime)); daynum++) { // the daynum test prevents extremes
    time_t nextday = time_add_day(startday,1);
    MD.add_to_day(startday,NAN,categorynumber);
    startday = nextday;
  }
}

String Metrics_Category::csv_String() {
  return category;
}

String Metrics_Category::category_rc_String() {
  // Reproduces the category definition as initialized.
  // This can be used to confirm the .rc file that was
  // used to produce category metrics.

  return category+"=0x"+Int2HexStr((unsigned int) metricsmapcol[categorynumber % 16])+":\n"+DE.concatenate("\n")+'\n'+DIL.concatenate("\n")+'\n';
}

Metrics_Day::Metrics_Day(time_t d, unsigned int numcategories, bool mapit): daydate(d), nummetricscategories(numcategories), daymap(NULL) {
  secondspercategory = new double[numcategories];
  for (unsigned int i=0; i<numcategories; i++) secondspercategory[i] = 0.0;
  if (mapit) {
    daymap = new unsigned char[24*60];
    for (unsigned int i=0; i<(24*60); i++) daymap[i] = 0xFF; // was 0; // was 'A'; was 255;
  }
}

void Metrics_Day::add_to_day(time_t ddate, double addseconds, unsigned int categorynumber) {
  // Finds the day in the linked list, or creates days as needed.
  // Adds addseconds to the categorynumber for the day specified by ddate.

  // Date sanity check
  if (ddate<0) {
    if (verbose) WARN << "SUSPICIOUS DATE " << ddate << " (negative) in this task log file, skipping it\n";
    return;
  }
  if (ddate<DATEINSECONDS19990101) { // #define DATEINSECONDS19990101 ((time_t) 915177600) (defined in dil2al.hh)
    if (verbose) WARN << "SUSPICIOUS DATE " << time_stamp("%Y%m%d",ddate) << " (earlier than 19990101) in this task log file, skipping it\n";
    return;
  }

  // If this is the right day then add time to category here
  if (daydate==ddate) {
    if (isnan(addseconds)) secondspercategory[categorynumber] = NAN; // data invalidated (see explanations above)
    else secondspercategory[categorynumber] += addseconds;
    if (secondspercategory[categorynumber]>(double) SECONDSPERDAY) if (verbose) WARN << "Beware! Category aggregated more than one day's worth of seconds on " << time_stamp("%Y%m%d",ddate) << '\n';
    return;
  }

  // Follow or extend the linked list to find the right day
  if (daydate<ddate) { // go to or create following day
    if (!Next()) Root()->link_before(new Metrics_Day(time_add_day(daydate,1),nummetricscategories,MapIt()));
    Next()->add_to_day(ddate,addseconds,categorynumber);
  } else { // go to or create previous day
    if (Prev()) Prev()->add_to_day(ddate,addseconds,categorynumber);
    else {
      // NOTE: Adding days only works forward (due to the mechanism in time_add_day(). So,
      // we create a new linked list here from ddate to this day, then link the new list
      // ahead of ours.
      PLLRoot<Metrics_Day> Mroot;
      Metrics_Day * md = new Metrics_Day(ddate,nummetricscategories,MapIt());
      Mroot.link_before(md);
      time_t dd = time_add_day(ddate,1);
      while (dd<daydate) {
	Mroot.link_before(new Metrics_Day(dd,nummetricscategories,MapIt()));
	dd = time_add_day(dd,1);
      }
      Root()->link_after(&Mroot); // take from Mroot and place at front
      md->add_to_day(ddate,addseconds,categorynumber);
    }
  }
}

void Metrics_Day::add_to_day_map(time_t ddate, time_t starttime, time_t endtime, unsigned int categorynumber) {
  // Finds the day in the linked list, or creates days as needed.
  // Maps categorynumber into daymap from starttime to endtime, bounded by ddate and ddate+SECONDSINDAY.
  // When endtime<=starttime no minutes are mapped in.

  if (!daymap) return;
  
  // Date sanity check
  if (ddate<0) {
    if (verbose) WARN << "SUSPICIOUS DATE " << ddate << " (negative) in this task log file, skipping it\n";
    return;
  }
  if (ddate<DATEINSECONDS19990101) { // #define DATEINSECONDS19990101 ((time_t) 915177600) (defined in dil2al.hh)
    if (verbose) WARN << "SUSPICIOUS DATE " << time_stamp("%Y%m%d",ddate) << " (earlier than 19990101) in this task log file, skipping it\n";
    return;
  }

  // If this is the right day then map in according to categorynumber
  if (daydate==ddate) {
    int i_start = (starttime-ddate)/60; // one map entry per minute of the day
    if (i_start<0) i_start=0;
    int i_end = (endtime-ddate)/60;
    if (i_end>(24*60)) i_end=24*60;
    for (int i=i_start; i<i_end; i++) daymap[i] = metricsmapcol[categorynumber % 16];
    return;
  }
  
  // Follow or extend the linked list to find the right day
  if (daydate<ddate) {// go to or create following day
    if (!Next()) Root()->link_before(new Metrics_Day(time_add_day(daydate,1),nummetricscategories,MapIt()));
    Next()->add_to_day_map(ddate,starttime,endtime,categorynumber);
  } else { // go to or create previous day
    if (Prev()) Prev()->add_to_day_map(ddate,starttime,endtime,categorynumber);
    else {
      // NOTE: Adding days only works forward (due to the mechanism in time_add_day(). So,
      // we create a new linked list here from ddate to this day, then link the new list
      // ahead of ours.
      PLLRoot<Metrics_Day> Mroot;
      Metrics_Day * md = new Metrics_Day(ddate,nummetricscategories,MapIt());
      Mroot.link_before(md);
      time_t dd = time_add_day(ddate,1);
      while (dd<daydate) {
	Mroot.link_before(new Metrics_Day(dd,nummetricscategories,MapIt()));
	dd = time_add_day(dd,1);
      }
      Root()->link_after(&Mroot); // take from Mroot and place at front
      md->add_to_day_map(ddate,starttime,endtime,categorynumber);
    }
  }
}

int Metrics_Day::count_in_map(unsigned char mapcode, unsigned int offset, unsigned int minutes) {
  // Finds the number of instances of mapcode in the daymap within a stretch of length
  // minutes starting at offset.
  int maplimit = offset + minutes;
  if (maplimit>1440) maplimit = 24*60;
  int mapcodecount = 0;
  if (daymap) for (int minidx = offset; minidx<maplimit; minidx++) if (daymap[minidx]==mapcode) mapcodecount++;
  return mapcodecount;
}

String Metrics_Day::csv_String() {
  String csvstring(time_stamp("%Y%m%d",daydate)+',');
  for (unsigned int i=0;i<nummetricscategories;i++) {
    //csvstring += String((long) secondspercategory[i]);
    csvstring += String(secondspercategory[i],"%.2f");
    if (i<(nummetricscategories-1)) csvstring += ',';
    else csvstring += '\n';
  }
  return csvstring;
}

time_t * Metrics_Days::day_dates_array(int * numdays) {
  // Returns an array with the daydate data of all days for which data was collected.
  // If numdays!=NULL then the number of days for which data was collected is
  // returned there as well.
  // If data was collected for zero days then *numdays will be set to 0 and
  // the function returns NULL.
  int _numdays = list.length();
  if (numdays) *numdays = _numdays;
  if (_numdays<=0) return NULL;
  time_t * daydate = new time_t[_numdays];
  int dayidx = 0;
  PLL_LOOP_FORWARD(Metrics_Day,list.head(),1) {
    daydate[dayidx] = e->daydate;
    dayidx++;
  }
  return daydate;
}

double * Metrics_Days::seconds_in_category_array(const char * c) {
  // Returns an array of doubles containing the seconds collected for
  // category c in each day for which data was collected.
  // Returns NULL if data was collected for zero days.
  // Returns NULL if the category c was not found.
  int numdays = list.length();
  if (numdays<1) return NULL;
  int cnum = MC->get_category_number_by_name(c);
  if (cnum<0) {
    if (verbose) WARN << "tlfilter.cc:seconds_in_category_array(): " << c << " category not found.\n";
    return NULL;
  }
  double * categoryseconds = new double[numdays];
  int dayidx = 0;
  PLL_LOOP_FORWARD(Metrics_Day,list.head(),1) {
    categoryseconds[dayidx] = e->secondspercategory[cnum];
    dayidx++;
  }
  return categoryseconds;
}

double * Metrics_Days::seconds_in_category_array(const char * c, double divideby) {
  // Returns an array of doubles containing the seconds collected
  // divided by divideby (e.g. 3600.0 to find hours) for
  // category c in each day for which data was collected.
  // Returns NULL if data was collected for zero days.
  // Returns NULL if the category c was not found.
  // Returns NULL if divideby<=0.0.
  if (divideby<=0.0) return NULL;
  int numdays = list.length();
  if (numdays<1) return NULL;
  int cnum = MC->get_category_number_by_name(c);
  if (cnum<0) {
    if (verbose) WARN << "tlfilter.cc:seconds_in_category_array(): " << c << " category not found.\n";
    return NULL;
  }
  double * categoryseconds = new double[numdays];
  int dayidx = 0;
  PLL_LOOP_FORWARD(Metrics_Day,list.head(),1) {
    categoryseconds[dayidx] = e->secondspercategory[cnum]/divideby;
    dayidx++;
  }
  return categoryseconds;
}

double * Metrics_Days::count_in_maps(unsigned char mapcode, unsigned int offset, unsigned int minutes, double divideby) {
  // Find in all days the number of instances of mapcode in the daymap within
  // a stretch of length minutes starting at offset. Divides by divideby
  // (e.g. 60.0 to convert from minutes to hours).
  // Returns an array of double of length this->list.length().
  // Returns NULL if this->list.length() is zero.
  int numdays = list.length();
  if (numdays<1) return NULL;
  double * mapcodecounts = new double[numdays];
  int dayidx = 0;
  PLL_LOOP_FORWARD(Metrics_Day,list.head(),1) {
    int mapcodecount = e->count_in_map(mapcode,offset,minutes);
    mapcodecounts[dayidx] = mapcodecount / divideby;
    dayidx++;
  }
  return mapcodecounts;
}

void Metrics_Days::add_to_day(time_t ddate, double addseconds, unsigned int categorynumber) {
  // Finds the day in the linked list, or creates days as needed.
  // Adds addseconds to the categorynumber.
  if (categorynumber>nummetricscategories) return;
  if (!list.head()) list.link_before(new Metrics_Day(ddate,nummetricscategories,mapit));
  list.head()->add_to_day(ddate,addseconds,categorynumber);
}

void Metrics_Days::add_to_day_map(time_t ddate, time_t starttime, time_t endtime, unsigned int categorynumber) {
  // Finds the day in the linked list, or creates days as needed.
  // Maps categorynumber into a map of the day from starttime to endtime.
  if (categorynumber>nummetricscategories) return;
  if (!list.head()) list.link_before(new Metrics_Day(ddate,nummetricscategories,mapit));
  list.head()->add_to_day_map(ddate,starttime,endtime,categorynumber);
}

String Metrics_Days::csv_String() {
  String csvstring("date,"+MC->csv_String());
  PLL_LOOP_FORWARD(Metrics_Day,list.head(),1) {
    csvstring += e->csv_String();
  }
  return csvstring;
}

void Metrics_Days::map() {
  ofstream mapfile(metricsmapfile,ios::out | ios::binary);
  if (mapfile.is_open()) {
    PLL_LOOP_FORWARD(Metrics_Day,list.head(),1) {
      if (e->daymap) mapfile.write((const char *) e->daymap, 24*60);
      else WARN << "Metrics_Days::map(): Missing e->daymap.\n";
    }
    mapfile.close();
    if (verbose) INFO << "Metrics Days Categories Map initialized as " << metricsmapfile << '\n';
  } else ERROR << "Metrics_Days::map(): Unable to open and write to " << metricsmapfile << ".\n";
}

Task_Log::Task_Log(): chunktasklog(tasklog), chunkseekloc(-1) {}

bool metrics_parse_task_log(String labelid) {
  // This function is used to analyze task log entries
  // for metrics data.
  // Normally (i.e. when labelid=="") this is about collecting
  // information about time spent on specific task categories
  // (see DIL#20110818170443.1).
  // When labelid!="" then, in addition to the category data,
  // the corresponding labels @label=X.Y@ are sought in each
  // Task Chunk and their time-weighed value data is reported
  // (see comments above with the metrics() function).
  // The special labelid=="MAP" (from the option -zMAP) means
  // that a granular map is greated for every day, specifying
  // the task category that appears there according to the Task Log.

  // Start with task-log.html
  Task_Log TaskLog;
  Metrics_Categories MC(mainmetricscategoriesfile,labelid);
  Metrics_Days MD(MC,labelid); // this initialization also determines the MD.mapit flag

  // While the TL has a previous task log file reference, continue
  TL_entry_content * TLentrycontent;
  if (verbose) INFO << "Collecting from task-log.html\n";
  // Find task chunks from most recent to earliest (searching backwards and bounded by metricsearliestday).
  while ((TLentrycontent = TaskLog.get_previous_task_log_entry()) != NULL) {
    // Bounded by metricsearliestday (defined and set in dil2al.hh/dil2al.cc)
    if ((TLentrycontent->chunkendtime>=0) && (TLentrycontent->chunkendtime<=metricsearliestday)) {
      if (verbose) INFO << "Data collected bounded by 'metricsearliestday'\n";
      break;
    }
    // Collect metrics data in each Task Chunk
    TLentrycontent->metrics(MD,labelid);
    delete TLentrycontent; // *** Note: Instead of deleting, add to a linked list if extracting and analyzing separately, e.g. when using a binary cache.
  }

  // Output results
  if (verbose) INFO << "Generating CSV file\n";
  if (MD.mapit) {
    MD.map();
    System_self_evaluation(MC,MD);
  }
  String rcstr(MC.categories_rc_String());
  write_file_from_String(metricsrcreproduction,rcstr,"Metrics Categories RC reproduction",true);
  String csvstring(MD.csv_String());
  return write_file_from_String(metricsdayspage,csvstring,"Metrics per Day per Category",true);
}

/*
Note 20190128:

The System_self_evaluation() could be done outside of dil2al instead. The map file could be
used to provide the necessary data. An external version could be a separate C++ program, a
compiled program in another language, or a script (e.g. an Octave script).

Right now, the need for something as math and linear algebra focused as Octave is not so
great, because the calculations are quite easy, even in C++.

Putting the function here does mean that changing my self-evaluation means re-compiling
dil2al.

The function will run fast, and there is the possible advantage that I can choose not to
write the map to a file and still produce the self-evaluation data.
 */

bool System_self_evaluation(Metrics_Categories & MC, Metrics_Days & MD) {
  // This function uses data collected in the 'daymap' arrays and data compiled
  // in the 'secondspercategory' arrays to generate output data in the format
  // most directly useful to daily self-evaluation as carried out with my System
  // and described in the 'System Summary' section of the Notes on Change document.
  // This output data is used to update the Daily-Weekly Self Evaluation spreadsheet.
  //
  // Update 2020-04-20: Seconds counted in the BUILDSYSTEM category will now be
  // added to dayIntentions_plus_IPAB, because System effort is foundational and
  // directly relevant to Milestones on a Path in the Personal Roadmap
  // (e.g. LEARNED-LVL1), and because there are now other ways to discourage
  // productive procrastination (e.g. the direct modification of the BSA, explicit
  // tracking of advancements on Active Milestone Steps). This is in response
  // to insights noted in TL#202004182210.1. (Prior to this BUILDSYSTEM was not
  // reported through the reduced set of metrics variables shown in Time Tracking.)

  // 0) Dates of the days for which data was collected
  int numdays = 0;
  time_t * daydate = MD.day_dates_array(&numdays);
  if (!daydate) {
    if (verbose) ERROR << "tlfilter:System_self_evaluation(): No data collected for any days.\n";
    return false;
  }
  
  // 1) Previous night sleep + day naps, actual (hrs)
  // Total time spent asleep during a 24 hour period.
  // The easiest approach is to use the sum within a day from 0:00 to 23:59.
  // The approach that was previously used in the spreadsheet was to count
  // overnight sleep time and add to that naps during the preceding day.
  // In the 20190128 version, we use the 0:00 to 23:59 approach, but
  // NOTE that the sleep hours are shown for the same date, because it is
  // assumed that 'previous night' is largely from 0:00 onwards.
  double * previous_night_sleep = MD.seconds_in_category_array("SLEEP",3600.0);

  // 2) Solo entertainment , actual (hrs)
  double * solo_entertainment = MD.seconds_in_category_array("SOLO",3600.0);
  
  // 3) Socializing , actual (hrs)
  double * socializing = MD.seconds_in_category_array("SOCIAL",3600.0);

  // 4) Exercising + self-care, actual (hrs) -- but truncated (see note)
  // NOTE: The way this parameter has been estimated and used is not very clearly
  // determined. The implementation here attempts to approximate that, although
  // a better approach is probably warranted. See TL#201901282102.1.
  double * wellbeing = MD.seconds_in_category_array("WELLBEING",3600.0);
  // ** possible truncation: for (int i = 0; i<numdays; i++) if (wellbeing[i]>1.0) wellbeing[i]=1.0;

  // 5) morning IPA/B hours, day IPA/B hours, day Intentions hours
  double * dayIPAB = MD.seconds_in_category_array("IPAB",3600.0);

  double * morningIPAB = NULL;
  unsigned mapcode = MC.get_map_code_by_name("IPAB");
  if (mapcode!=0xFF) morningIPAB = MD.count_in_maps(mapcode,0,12*60,60.0);

  // *** Note that the dayIntentions_plus_IPAB variable probably should be renamed
  //     to avoid confusion, as per the clarification of how the term Intentions
  //     is used in the context of the System, Formalizer, etc. For details, see
  //     TL#202003280846.2 or the documentation in the Notes on Change document.
  //     The same applies to the DAYINT label in the output.
  double * dayIntentions_plus_IPAB = MD.seconds_in_category_array("DAYINT",3600.0);
  if (!dayIntentions_plus_IPAB) dayIntentions_plus_IPAB = dayIPAB;
  else if (dayIPAB) for (int i = 0; i<numdays; i++) dayIntentions_plus_IPAB[i] += dayIPAB[i];
  // added 2020-04-20 (see update comment at top of function)
  double * dayBUILDSYSTEM = MD.seconds_in_category_array("BUILDSYSTEM",3600.0);
  if (!dayIntentions_plus_IPAB) dayIntentions_plus_IPAB = dayBUILDSYSTEM;
  else if (dayBUILDSYSTEM) for (int i = 0; i<numdays; i++) dayIntentions_plus_IPAB[i] += dayBUILDSYSTEM[i];

  // Output and cleanup
  String csvstring("DATE,SLEEP,SOLO,SOCIAL,WELLBEING,morningIPAB,dayIPAB,dayIntentions\n");
  for (int i = 0; i<numdays; i++) {
    csvstring += String(time_stamp("%Y%m%d",daydate[i])) + ',';
    if (previous_night_sleep) csvstring += String(previous_night_sleep[i],"%.2f") + ','; else csvstring += "-1.0,";
    if (solo_entertainment) csvstring += String(solo_entertainment[i],"%.2f") + ','; else csvstring += "-1.0,";
    if (socializing) csvstring += String(socializing[i],"%.2f") + ','; else csvstring += "-1.0,";
    if (wellbeing) csvstring += String(wellbeing[i],"%.2f") + ','; else csvstring += "-1.0,";
    if (morningIPAB) csvstring += String(morningIPAB[i],"%.2f") + ','; else csvstring += "-1.0,";
    if (dayIPAB) csvstring += String(dayIPAB[i],"%.2f") + ','; else csvstring += "-1.0,";
    if (dayIntentions_plus_IPAB) csvstring += String(dayIntentions_plus_IPAB[i],"%.2f") + '\n'; else csvstring += "-1.0\n";
  }
  delete[] daydate;
  delete[] previous_night_sleep;
  delete[] solo_entertainment;
  delete[] socializing;
  delete[] wellbeing;
  delete[] dayBUILDSYSTEM;
  delete[] dayIPAB;
  delete[] morningIPAB;
  delete[] dayIntentions_plus_IPAB;
  return write_file_from_String(systemselfevaluationdata,csvstring,"System self evaluation data",true);
}

/*
  OLD VERSION OF TL_entry_content::metrics_labelid():
  Keeping this just until the new version is sufficiently tested.

      int labeledtextloc = htmltext.index('@'+labelid+'=');
      if (labeledtextloc>=0) {
	labeledtextloc += 2 + labelid.length();
	int labeledtextend = htmltext.index('@',labeledtextloc);
	if (labeledtextend>=0) {
	  String metricvaluestr = htmltext.at(labeledtextloc, labeledtextend-labeledtextloc);
	  double metricvalue = atof(metricvaluestr.chars());
	  PLL_LOOP_FORWARD(Metrics_Category,MD.MC->list.head(),1) {
	    if (e->category==labelid) {
	      e->add_labeled_chunk(this,MD,metricvalue);
	      break;
	    }
	  }
	} else labeledtextloc=-1; // no valid value found (broken labeled entry)
      }
      if (labeledtextloc<0) { // no value recorded in this Task Chunk, invalidates Day value for this metric!
	PLL_LOOP_FORWARD(Metrics_Category,MD.MC->list.head(),1) {
	  if (e->category==labelid) {
	    e->invalidate_chunk(this,MD);
	    break;
	  }
	}
      }
*/
