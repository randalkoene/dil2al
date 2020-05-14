// search.cc
// Randal A. Koene, 20011126

#include "dil2al.hh"

/*
	This may search by following links starting from a specific file,
	directory or search engine search term, or may search a set of archive
	locations.
	Locations that point to directories or that are search engine search
	terms must be turned into a list of specific targets to search. Note
	that targets obtained in followlinks mode by Search_Target objects may
	also point to such locations. In the case of breadth-first search the
	conversion to specific targets may be done by the same function that
	controls the searching process. Otherwise, the conversion must take
	place in Search_Target, while adding targets to the list.

	See <A HREF="dil2al.hh">dil2al.hh</A>.
*/

void Target_Access::set_target(const char * t, const Target_Access * parent) {
  // Identify type of target and verify proper specification
  // *** generate new target, target is local if relative and current is local, followlinks-1
  // - put absolute file references in as such + server&user if current is remote
  // - append relative file references to current target
  // Note: This function is currently not intended to be used to reinitialize Target_Access objects,
  // as it does not reset server and user parameters if the new target is local
  
  target = t;
  String preslash(target.before('/'));
  preslash.upcase();
  if ((preslash == "HTTP:") || (preslash == "FTP:")) { // Server and path references for URLs
    if (preslash == "HTTP:") type = TA_HTTP;
    else type = TA_FTP;
    remove_name_reference(target);
    int serverstart = target.index("://",String::SEARCH_END,0);
    if (serverstart<0) {
      if (verbose) EOUT << "dil2al: Warning - malformed URL, " << target << ", in Target_Access::Target_Access()\n";
      type = TA_UNKNOWN;
      return;
    }
    int serverend = target.index('/',serverstart);
    if (serverend<0) serverend = target.length();
    if (serverstart==serverend) {
      if (verbose) EOUT << "dil2al: Warning - malformed URL server, " << target << ", in Target_Access::Target_Access()\n";
      type = TA_UNKNOWN;
      return;
    }
    server = target.at(serverstart,serverend-serverstart);
    path = target.from(serverend);
    if (type == TA_HTTP) return;
    // FTP specific initialization
    user = server.before('@');
    if (!user.empty()) server = server.after('@');
    return;
  }
  // Local file specific initialization
  path = target;
  if (parent) { // reference relative to parent
    if ((parent->type==TA_HTTP) || (parent->type==TA_FTP)) { // relative to remote
      type = parent->type;
      server = parent->server;
      user = parent->user;
      // *** problem: if parent target was an ftp or http directory without a '/' at
      //     the end, then there is no way to know if the last part of the target path
      //     is part of the path to prepend or not
      //     You actually *have* this information, because if you are setting up a
      //     target relative to a parent, then you have that parent's contents, from
      //     which you got the link... so you can see if it's a directory index
      // *** problem: at some point between reading the citeseer site and the nec.com
      //     site, dil2al appears to develop a memory leak and grows rapidly then crashes
      // *** problem: there has to be another way around the long wget wait for
      //     connections, perhaps a site's availability can be tested some other way
      //     before attempting a wget
      remove_name_reference(path);
      if (path.firstchar()!='/') path.prepend(const_cast<String &>(parent->path).through('/',-1));
      remove_back_references(path);
      if (type==TA_HTTP) target = "http://"+server+path;
      else target = "ftp://"+user+'@'+server+path;
      return;
    } else if (path.firstchar()!='/') { // relative to local
      if (parent->type==TA_LOCAL_DIRECTORY) {
	if (parent->path.lastchar()!='/') path.prepend('/');
	path.prepend(parent->path);
      } else path.prepend(const_cast<String &>(parent->path).through('/',-1));
    }
  }
  // local reference
  remove_back_references(path);
  target = path; // complete local target reference
  if (file_is_directory(path)) type = TA_LOCAL_DIRECTORY;
  else type = TA_LOCAL;
}

void generic_remove_name_reference(String & tpath) {
  int nameref = tpath.index('#',-1);
  int fileref = tpath.index('/',-1);
  if ((nameref>=0) && (nameref>fileref)) tpath.del(nameref,tpath.length()-nameref);
}

void Target_Access::remove_name_reference(String & tpath) {
  // Removes a potential #name reference from an http target or path
  if (type == TA_HTTP) generic_remove_name_reference(tpath);
}

void Target_Access::remove_back_references(String & tpath) {
  // Removes ./ and ../ references from a path
  tpath.gsub("/./","/");
  if ((tpath.firstchar()=='.') && (tpath[1]=='/')) tpath.del(0,2);
  int i, j;
  while ((i=tpath.index("/../"))>=0) {
    if ((j=tpath.index('/',(i-tpath.length())-1))<0) j=-1;
    j++;
    tpath.del(j,(i-j)+4);
  }
}

#ifdef TARGET_ACCESS_DEBUG
void Target_Access::Report() const {
  VOUT << "Target_Access::target = " << target << '\n';
  VOUT << "Target_Access::user = " << user << '\n';
  VOUT << "Target_Access::server = " << server << '\n';
  VOUT << "Target_Access::path = " << path << '\n';
  switch (type) {
  case TA_LOCAL: VOUT << "Target_Access::type = TA_LOCAL\n"; break;
  case TA_HTTP: VOUT << "Target_Access::type = TA_HTTP\n"; break;
  case TA_FTP: VOUT << "Target_Access::type = TA_FTP\n"; break;
  case TA_UNKNOWN: VOUT << "Target_Access::type = TA_UNKNOWN\n"; break;
  }
}
#endif

void Target_Access::transformed_name(String & transformed, String & prevname) {
  if (transformed.empty()) transformed = prevname;
  if ((prevname==path) || (prevname==target)) {
    transformed.gsub('/','%');
    transformed.prepend(tareadfile);
  }
}

void Target_Access::transformed_name(String & transformed, String & prevname, const BigRegex & transrx) {
  transformed = prevname.before(transrx.subpos());
  transformed_name(transformed,prevname);
}

bool Target_Access::make_local_copy_of_directory() {
  // create HTML list of links for directory entries
  // Note: localcopy must point to a valid temporary file name
  // get directory
  StringList dirtargets;
  unsigned long num = Get_Target_Directory(dirtargets,0,path);
  // create HTML links
  String dirfile("<HTML>\n<HEAD>\n<TITLE>Local Directory: "+path+"</TITLE>\n</HEAD>\n<BODY>\n<H1>Local Directory: "+path+"</H1>\n\n");
  for (unsigned long i = 0; i<num; i++) dirfile += HTML_put_href(dirtargets[i],dirtargets[i].after('/',-1)) + "\n<P>\n";
  dirfile += "(Temporary local HTML copy of directory created by dil2al "+curtime+")\n\n</BODY>\n</HTML>\n";
  // write local file
  write_file_from_String(localcopy,dirfile,"",true); // message used to be: "Local copy of "+path+" directory"
  return true;
}

bool Target_Access::make_local_copy_of_HTTP(){
  // Note: The wget utility has a smaller footprint than the lynx browser, but
  // has some difficulties with unresponsive connections, as it tends to wait
  // just as long for a connection as it does between network packages (specified
  // with the -T option, with a default of 15 minutes).
  // *** An alternative solution for this is to do searching or at least remote
  //     retrieval in the background with multiple threads, as indicated in the
  //     DIL entry <A HREF="../../doc/html/lists/dil2al.html#20000930123422.1">DIL#20000930123422.1</A>.
  return syscommand("lynx","-connect_timeout=30 -source "+target+" > "+localcopy);
  //return syscommand("wget","-q -t 2 -O "+localcopy+' '+target);
}

bool Target_Access::make_local_copy_of_FTP(){
  // Note: The wget utility has a smaller footprint than the lynx browser, but
  // has some difficulties with unresponsive connections, as it tends to wait
  // just as long for a connection as it does between network packages (specified
  // with the -T option, with a default of 15 minutes).
  return syscommand("lynx","-connect_timeout=30 -source "+target+" > "+localcopy);
  //return syscommand("wget","-q -t 2 --retr-symlinks -O "+localcopy+' '+target);
}

bool Target_Access::make_local_copy() {
  // makes sure a local copy is available
  if (!localcopy.empty()) return true;
  if (type==TA_UNKNOWN) {
    if (verbose) EOUT << "dil2al: Unable to access unknown target type in Target_Access::make_local_copy()\n";
    return false;
  }
  if (type==TA_LOCAL) localcopy = path;
  else if (type==TA_LOCAL_DIRECTORY) {
    transformed_name(localcopy,path);
    remove(localcopy); // remove possible stale file
    return make_local_copy_of_directory();
  } else {
    transformed_name(localcopy,target);
    remove(localcopy); // remove possible stale file
    if (type==TA_HTTP) return make_local_copy_of_HTTP();
    else return make_local_copy_of_FTP();
  }
  return true;
}

bool Target_Access::syscommand(String program, String args, int success) const {
  int sysret = System_Call(program+' '+args);
  if (sysret==success) return true;
  switch (sysret) {
  case -1:
    EOUT << "dil2al: Error in system(<" << program << ">) call in Target_Access::syscommand()\n";
    return false;
  case 127:
    EOUT << "dil2al: Execve() call for /bin/sh failed in Target_Access::syscommand()\n";
    return false;
  default:
    EOUT << "dil2al: Return value of " << program << " was " << sysret << " in Target_Access::syscommand()\n";
    return false;
  }
}

bool Target_Access::uncompress() {
  // makes sure an uncompressed version is available
  if (!uncompressed.empty()) return true;
  if (!make_local_copy()) return false;
  const BigRegex gzrx("[.][Gg]?[Zz]$");
  if (localcopy.contains(gzrx)) {
    transformed_name(uncompressed,localcopy,gzrx);
    //cerr << "Uncompressed: " << uncompressed << '\n';
    remove(uncompressed); // remove possible stale file
    if (!syscommand("gzip","-d -c "+localcopy+" > "+uncompressed)) return false;
  } else uncompressed = localcopy;
  return true;
}

bool Target_Access::cleartext_conversion() {
  // makes sure a clear text version is available
  if (!cleartext.empty()) return true;
  if (!uncompress()) return false;
  const BigRegex psrx("[.][Pp][Ss]$"); const BigRegex pdfrx("[.][Pp][Dd][Ff]$");
  if (uncompressed.contains(psrx)) {
    transformed_name(cleartext,uncompressed,psrx);
    remove(cleartext); // remove possible stale file
    if (!syscommand("pstotext",uncompressed+" > "+cleartext)) return false;
  } else if (uncompressed.contains(pdfrx)) {
    transformed_name(cleartext,uncompressed,pdfrx);
    remove(cleartext); // remove possible stale file
    if (!syscommand("pdftotext","-raw "+uncompressed+' '+cleartext)) return false;
  } else cleartext = uncompressed;
  //cerr << "Cleartext: " << cleartext << '\n';
  return true;
}

bool Target_Access::read_raw_into_String(String & text, bool warnopen) {
  if (!make_local_copy()) return false;
  if (!read_file_into_String(localcopy,text,warnopen)) return false;
  if (!retaintransformed) remove_local_copy();
  return true;
}

bool Target_Access::read_uncompressed_into_String(String & text, bool warnopen) {
  if (!uncompress()) return false;
  if (!read_file_into_String(uncompressed,text,warnopen)) return false;
  if (!retaintransformed) {
    remove_uncompressed();
    remove_local_copy();
  }
  return true;
}

bool Target_Access::read_cleartext_into_String(String & text, bool warnopen) {
  if (!cleartext_conversion()) return false;
  if (!read_file_into_String(cleartext,text,warnopen)) return false;
  if (!retaintransformed) {
    remove_cleartext();
    remove_uncompressed();
    remove_local_copy();
  }
  return true;
}

void Search_Target::check_encountered() {
  if (encountered) {
    if (encountered->read((const char *) target.Target())) searched = 1;
    else (*encountered) << (const char *) target.Target();
  }
  if ((!remotesearch) && ((target.Type()==Target_Access::TA_HTTP) || (target.Type()==Target_Access::TA_FTP))) searched = -1;
}

//#define SEARCH_TARGET_DEBUG
long Search_Target::search() {
  // Searches the target file for occurrences of searchkey and stores the results
#ifdef SEARCH_TARGET_DEBUG
  EOUT << "dil2al: Search_Target::search(), followlinks=" << followlinks << ", target.Target() = " << target.Target() << '\n';
#endif
  if (searched!=0) {
    if (verbose) {
      if (searched==1) EOUT << target.Target() << " already searched elsewhere\n";
      else EOUT << target.Target() << " restricted, do not search\n";
    }
    return 0;
  }
  String t;
  if (!target.read_cleartext_into_String(t)) return -1;
  int i = 0, res = 0; BigRegex skrx(searchkey);
  while ((i=t.index(skrx,String::SEARCH_END,i))>=0) {
    Search_Result * r = new Search_Result(searchkey,target,t,skrx);
    if (((r->Type()==Search_Result::SR_DIRECTORY_ENTRY) || (r->Type()==Search_Result::SR_URL)) && (results->tail())) if (*(results->tail())==*r) {
	delete r; // try to avoid duplicates
	continue;
      }
    results->link_before(r);
    res++;
  }
  if (followlinks>0) links(t); // if followlinks>0 then links are sought and added to the search
  searched = 1;
  return res;
}

long Search_Target::links(const String & t) {
  // Finds links in the text t and adds them to the list of targets
  const BigRegex linksrx("[<][Aa][^>]+[Hh][Rr][Ee][Ff]");
  const BigRegex lkendrx("[<][/][Aa][ \t]?[^>]*[>]");
  const BigRegex urlstartrx("=[ \t]*\"?");
  const BigRegex mailtorx("^[ \t]*[Mm][Aa][Ii][Ll][Tt][Oo][ \t]*:");
  int i = 0;
  long numlinksfound = 0;
  while ((i=t.index(linksrx,String::SEARCH_END,i))>=0) {
    int urlstart = t.index(urlstartrx,String::SEARCH_END,i); // find URL start
    if (urlstart>=0) {
      int urlend;
      if (t[urlstart-1]=='"') urlend = t.index('"',urlstart); // find matching '"'
      else urlend = t.index(BRXwhite,urlstart); // find white-space terminated URL
      if (urlend>=0) {
	String url(const_cast<String &>(t).at(urlstart,urlend-urlstart)); // get URL
	generic_remove_name_reference(url);
	if (!(url.empty() || url.contains(mailtorx))) {
	  // *** currently assumes breadth-first search
	  Search_Target * st = new Search_Target(*this,url);
	  Root()->link_before(st); // append st to list
	  numlinksfound++;
	}
      }
    }
    i = t.index(lkendrx,String::SEARCH_END,i); // find end of hyperlink
  }
  return numlinksfound;
}

Search_Result::Search_Result(const char * s, const Target_Access & t, String & text, const BigRegex & skrx): searchkey(s), target(t), location(skrx.subpos()) {
  // Obtains the matched keytext and some pretext and posttext
  const int PTEXTSIZE = 160;
  keytext = text.sub(skrx,0);
  if (t.Type()!=Target_Access::TA_LOCAL_DIRECTORY) { // get pretext and posttext
    // *** DETECT HERE IF SR_URL!
    type = SR_REGULAR;
    if (skrx.subpos()>PTEXTSIZE) pretext = text.at(skrx.subpos()-PTEXTSIZE,PTEXTSIZE); else pretext = text.before(skrx.subpos());
    int skrxend = skrx.subpos()+skrx.sublen();
    // note that text.length() loses some value range here due to
    // conversion to signed int
    if ((skrxend+PTEXTSIZE)<(int) text.length()) posttext = text.at(skrxend,PTEXTSIZE); else posttext = text.from(skrxend);
  } else { // get complete directory entry reference
    type = SR_DIRECTORY_ENTRY;
    int entrystart = text.index("<A HREF=",(int) (skrx.subpos()-text.length()));
    if (entrystart>=0) {
      int entryend = text.index("</A>",skrx.subpos());
      if (entryend>=0) {
	int urlend = text.index("\">",String::SEARCH_END,entrystart);
	if (urlend>=0) {
	  pretext = text.at(entrystart,urlend-entrystart); // get URL
	  // split reference text into pre, key and post parts
	  posttext = text.at(urlend,entryend-urlend);
	  if (posttext.contains(skrx)) {
	    pretext += posttext.before(skrx.subpos());
	    keytext = posttext.sub(skrx,0);
	    posttext = posttext.from(skrx.subpos()+skrx.sublen());
	  } else { // should never occur in a SR_DIRECTORY_ENTRY, just for extra safety
	    pretext += '[';
	    posttext.prepend("] ");
	  }
	  posttext += "</A>";
	}
      }
    }
  }
}

int Search_Result::isequal(const Search_Result & sr) const {
  // Equality comparison operator
  // Note: This could also have been defined as a function that takes two arguments
  if (searchkey!=sr.searchkey) return 0;
  if (target!=sr.target) return 0;
  if (pretext!=sr.pretext) return 0;
  if (keytext!=sr.keytext) return 0;
  if (posttext!=sr.posttext) return 0;
  if (type!=sr.type) return 0;
  return 1;
}

String Search_Result::HTML_put_text() const {
  // Produce HTML output for search result
  static String res;
  String pre, post, key;
  // *** IMPROVE:
  // - identify any links in the pretext+keytext+posttext and turn them into correct relative links
  // - beware of searchkey found in the middle of an HREF URL
  switch (type) {
  case SR_REGULAR: // *** this one will be modified with the improvements mentioned above
    pre = pretext; key = keytext; post = posttext;
    res = "<A HREF=\""+target.Target()+"\">"+target.Target()+"</A> ["+String(location)+"]: "+(*HTML_safe(pre))+"<FONT COLOR=\"#FF0000\"><B>"+(*HTML_safe(key))+"</B></FONT>"+(*HTML_safe(post))+"\n<P>\n\n";
    break;
  case SR_DIRECTORY_ENTRY:
    res = "<A HREF=\""+target.Target()+"\">"+target.Target()+"</A>: "+pretext+"<FONT COLOR=\"#FF0000\"><B>"+keytext+"</B></FONT>"+posttext+"\n<P>\n\n";
    break;
  case SR_URL: // *** implement this correctly!
    res = "<A HREF=\""+target.Target()+"\">"+target.Target()+"</A> ["+String(location)+"]: "+pretext+"<FONT COLOR=\"#FF0000\"><B>"+keytext+"</B></FONT>"+posttext+"\n<P>\n\n";
    break;
  }
  return res;
}

long search_output(PLLRoot<Search_Result> * sr, String & res, bool stringoutput = true) {
  // output results
  if (stringoutput) PLL_LOOP_FORWARD(Search_Result,sr->head(),1) res += e->HTML_put_text();
  else {
    PLL_LOOP_FORWARD(Search_Result,sr->head(),1) VOUT << e->HTML_put_text();
    VOUT.flush();
  }
  long num = sr->length();
  sr->clear();
  return num;
}

String search(StringList & targets, String searchkey, bool stringoutput) {
  // Search the target for searchkey
  // Result output is returned as a string or on VOUT.
  // Returns results as string or a string with the number of results.
  // Note: Directories are conceptually handled in the same manner
  // as HTML pages, but they are read differently and can contain
  // only links to follow for recursive searching.
  // Note: It is quite easy to make alternative search functions using
  // the Search_Target and Search_Result classes.
  // initialize global lists
  static String res; // stored in function, not objects, static to avoid problems with lifetime of object that return value points to
  PLLRoot<Search_Result> results;
  PLLRoot<Search_Target> globaltargets;
  Quick_String_Hash encountered;
  // search target objects for searchkey
  // *** Whether the list of Search_Target objects ever becomes
  //     greater than one depends on whether a followlinks search
  //     is conducted in breadth-first or depth-first fashion.
  Search_Target * t; String progressfile(tareadfile+"progress"), progress; long resnum = 0;
  for (unsigned int i = 0; i<targets.length(); i++) if (!targets[i].empty()) {
      //cerr << searchkey << " in " << targets[i] << '\n'; cerr.flush();
      t = new Search_Target(&results,searchkey,targets[i],followlinks,false,&encountered);
      globaltargets.link_before(t);
      // *** Not quite sure where to put the next lines, in this
      //     loop or outside it, and how that affects the search
      //     order if additional targets are added by finding
      //     links to follow in a breadth-first followlinks search.
      long i_search = 0;
      PLL_LOOP_FORWARD(Search_Target,globaltargets.head(),1) {
	if (searchprogressmeter) { // write a progress meter to a file
	  progress = String((long) i)+','+String(i_search)+':'+e->Target();
	  write_file_from_String(progressfile,progress,"",true);
	  i_search++;
	}
	e->search();
	if (immediatesearchoutput) resnum += search_output(&results,res,stringoutput);
	// *** caused problems here: e->remove(); // frees up space and avoids repeated searches
      }
      globaltargets.clear(); // frees up space and avoids repeated searches
    }
  if (!immediatesearchoutput) resnum = search_output(&results,res,stringoutput);
  if (!stringoutput) res = String(resnum);
  return res;
}

unsigned long Get_Target_Directory(StringList & targets, unsigned long i, String dirpath) {
  // add filenames from a directory to the list of targets
  // returns the number of targets added
  unsigned long num;
  // get directory content
  String t;
  if (dirpath.lastchar()=='/') dirpath.del((int) (dirpath.length()-1),1);
  Directory_Access da(dirpath);
  num = da.read(t);
  if (num<0) {
    EOUT << "dil2al: Unable to read directory " << dirpath << '\n';
    return 0;
  }
  t.del(0,1); // remove first separator character
  StringList df(t,'/'); String f;
  // add verified files to targets list and adjust num as needed
  for (unsigned long j = 0; j<(df.length()-1); j++) {
    f = dirpath+'/'+df[j];
    if (file_is_regular_or_symlink(f)) {
      targets[i] = f;
      i++;
    } else if (followlinks>0) { // can enable recursive searching of directories
      if ((file_is_directory(f)) && (df[j]!=".") && (df[j]!="..")) {
	targets[i] = f;
	i++;
      } else num --;
    } else num--;
  }
  return num;
}

unsigned long Get_Target_Regex(StringList & targets, unsigned long i, String dirpath) {
  // add filenames from a directory to the list of targets that match a regex
  // returns the number of targets added
  // the regex is provided as "@regex-pattern@" following the directory, '@' need not be escaped
  StringList d;
  BigRegex trx("@[^/]+@$");
  // get regex
  if (!dirpath.contains(trx)) {
    EOUT << "dil2al: No regex pattern in " << dirpath << " in Get_Target_Regex()\n";
    return 0;
  }
  String fileregex = dirpath.sub(trx,0); fileregex.del(0,1); fileregex.del((int) (fileregex.length()-1),1);
  // get directory content
  unsigned long num = Get_Target_Directory(d,0,dirpath.before(trx.subpos()));
  // add files that match the regex to targets list and ajust num as needed
  String filename; BigRegex frx(fileregex);
  for (unsigned long j = 0; j<d.length(); j++) {
    filename = d[j].from(trx.subpos());
    if (filename.matches(frx)) {
      targets[i] = d[j];
      i++;
    } else num--;
  }
  return num;
}

bool search_cmd(String grepinfo) {
  // Command line interface to search()
  // Note: For optimal use, wrap the output of this call in a complete framework
  // for an HTML file. It is also possible to direct the error output to another
  // file and to link to that one from the HTML file with the search results.
  const BigRegex trx("@[^/]+@$");
  const BigRegex rrx("\\(^[Hh][Tt][Tt][Pp]:\\|^[Ff][Tt][Pp]:\\)");
  if (grepinfo.empty()) {
    EOUT << "dil2al: Missing search information in search_cmd()\n";
    return false;
  }
  char separator = grepinfo[0];
  grepinfo.gsub(replicate(separator,2),(char) 255); // temporarily replace escaped characters
  int keyend = grepinfo.index(separator,1);
  if (keyend<0) {
    EOUT << "dil2al: Missing search key end in search_cmd()\n";
    return false;
  }
  if (keyend==1) {
    EOUT << "dil2al: Zero length search key in search_cmd()\n";
    return false;
  }
  // note that grepinfo.length() loses some value range here due to
  // conversion to signed int, which is needed since keyend must be
  // able to hold negative return values of grepinfo.index() above
  if ((keyend+1)>=(int) grepinfo.length()) {
    EOUT << "dil2al: Missing search target in search_cmd()\n";
    return false;
  }
  // put all search targets into a StringList
  String searchkey(grepinfo.at(1,keyend-1)); searchkey.gsub((char) 255,separator); // restore escaped characters
  StringList targets;
  grepinfo.del(0,keyend+1);
  for (unsigned long i = 0; (!grepinfo.empty()); i++) {
    // get next target from command line arguments
    targets[i] = grepinfo.before(separator);
    if (targets[i].empty()) targets[i] = grepinfo;
    targets[i].gsub((char) 255,separator); // restore escaped characters
    // interpret target
    // *** add: tar(gz) and zip archives, archive declarator, file containing target lines, default set
    if (targets[i].contains(trx)) i = Get_Target_Regex(targets,i,targets[i]) - 1;
    // *** does not yet properly handle remote files matching regex pattern or remote files pointing to archives
    // Now handled within search: else if (file_is_directory(targets[i])) i = Get_Target_Directory(targets,i,targets[i]) - 1;
    else if ((!targets[i].contains(rrx)) && (file_is_directory(targets[i])) && (followlinks<=0)) { // follow links one deep into the directory
      followlinks = 1;
      if (verbose) VOUT << "(Setting followlinks=1 to search directory.)\n";
    }
    grepinfo = grepinfo.after(separator);
  }
  VOUT << '\n';
  String res = search(targets,searchkey,false);
  VOUT << "(Number of search results: <B>" << res << "</B>)\n<P>\n";
  return true;
}

bool search_FORM_cmd(StringList * qlist) {
  // Perform a dil2al regex search with input from an HTML FORM.
  const char escapedseparator[3] = {(char) 254,(char) 254,0};
  const char separator = (char) 254;
  if (!qlist) return false;
  String grepinfo(separator);
  int paridx; String parstr;
  DIL2AL_CONDITIONAL_ERROR_RETURN((paridx=qlist->contains("regex="))<0,"No regex parameter found in form data in search_FORM_cmd()\n");
  parstr=URI_unescape((*qlist)[paridx].after('='));
  DIL2AL_CONDITIONAL_ERROR_RETURN(parstr.empty(),"Empty regex parameter in search_FORM_cmd()\n");
  parstr.gsub(separator,escapedseparator);
  grepinfo += parstr + separator;
  DIL2AL_CONDITIONAL_ERROR_RETURN((paridx=qlist->contains("targets="))<0,"No targets parameter found in form data in search_FORM_cmd()\n");
  parstr=URI_unescape((*qlist)[paridx].after('='));
  DIL2AL_CONDITIONAL_ERROR_RETURN(parstr.empty(),"Empty targets parameter in search_FORM_cmd()\n");
  parstr.gsub(separator,escapedseparator);
  parstr.gsub('\r',"");
  parstr.gsub('\n',separator);
  grepinfo += parstr;
  return search_cmd(grepinfo);
}
