// utilities.cc
// Randal A. Koene, 20000304, 20051006, 20190127

#include "dil2al.hh"

//#define DEBUG_NOVELTYMARKER
//#define TEST_CACHE_READ

String debug_time_stamp_time_caller;
#define _DEBUG_TIME_STAMP_TIME_CALLER_REPORT(DebugLabel) \
  do { if (DEBUG_TIME_STAMP_TIME) VOUT << "DBG " << DebugLabel << ": caller is " << debug_time_stamp_time_caller << '\n'; } while (0)

#define READSOMETYPE char*

#define TEST_LBUF_BOUNDS(slen,funcname) if (slen>=LLEN) { ERROR << funcname << ": Number of characters exceeds lbuf bounds\n"; return false; }

#define PLAN_ENTRY_IDENTIFIER_TAG "<!-- @begin: PLAN DIL TEMPLATE: "
#define PLAN_ENTRY_IDENTIFIER_TAG_LENGTH 32
#define PLAN_ENTRY_DECISION_TAG "<BR><B>decision:"
#define PLAN_ENTRY_STRING_ACTION "action"
#define PLAN_ENTRY_STRING_GOAL "goal"
#define PLAN_ENTRY_STRING_OBJECTIVE "objective"
#define PLAN_ENTRY_STRING_OUTCOME "outcome"
#define PLAN_ENTRY_STRING_VARIABLE "variable"
#define PLAN_ENTRY_STRING_SEEKPOSSIBLESOLUTIONS "seek-possible-solutions"
#define PLAN_ENTRY_STRING_PEERREVIEWSOLUTIONS "peer-review-solutions"
#define PLAN_ENTRY_STRING_MILESTONEOBJECTIVE "milestone objective"
#define PLAN_ENTRY_STRING_PROBLEM "problem"
#define PLAN_ENTRY_STRING_POSSIBLESOLUTION "possible solution"
#define NUM_PLAN_ENTRY_TYPES 10

#define EXTRA_PARAMETERS_IDENTIFIER_TAG "<!-- @begin: EXTRA PARAMETERS: "
#define EXTRA_PARAMETERS_IDENTIFIER_TAG_LENGTH 31

const char plan_entry_type_strings[NUM_PLAN_ENTRY_TYPES][24] = {PLAN_ENTRY_STRING_ACTION,PLAN_ENTRY_STRING_GOAL,PLAN_ENTRY_STRING_OBJECTIVE,PLAN_ENTRY_STRING_OUTCOME,PLAN_ENTRY_STRING_VARIABLE,PLAN_ENTRY_STRING_SEEKPOSSIBLESOLUTIONS,PLAN_ENTRY_STRING_PEERREVIEWSOLUTIONS,PLAN_ENTRY_STRING_MILESTONEOBJECTIVE,PLAN_ENTRY_STRING_PROBLEM,PLAN_ENTRY_STRING_POSSIBLESOLUTION};

String Time::caltime2local(const char * dateformat) {
	return time_stamp(dateformat,t);
}

StringList::StringList(String cs, const char sep) {
// initialize list from a string with a specified separator
// (Note: In the current implementation this leaves an unused
// list element at the end of the list.
// Beware! If you fix this to leave no unused list element at
// the end of the list you must check all source code where
// this initialization function is used to insure that the
// code is modified so as not to depend on this "feature"!)
	Next=NULL;
	split(0,cs,sep);
}

StringList::StringList(String cs, const String sep, int csstart) {
// initialize list from a string with a specified separator
// (Note: In the current implementation this leaves an unused
// list element at the end of the list.
// Beware! If you fix this to leave no unused list element at
// the end of the list you must check all source code where
// this initialization function is used to insure that the
// code is modified so as not to depend on this "feature"!)
	Next=NULL;
	split(0,cs,sep,csstart);
}

String & StringList::operator[](unsigned long n) {
	if (!n) return s;
	if (!Next) Next = new StringList();
	return (*Next)[n-1];
}

StringList * StringList::List_Index(long n) {
  if (n<0) return NULL;
  if (!n) return this;
  if (!Next) return NULL;
  return Next->List_Index(n-1);
}

void StringList::append(unsigned long n, String & ins) {
  if (!n) Next = new StringList(ins,Next);
  else
    if (Next) Next->append(n-1,ins);
    else {
      Next = new StringList();
      Next->append(n-1,ins);
    }
}

void StringList::insert(unsigned long n, String & ins) {
  if (!n) { Next = new StringList(s,Next); s = ins; }
  else
    if (Next) Next->insert(n-1,ins);
    else 
      if (n>1) {
	Next = new StringList();
	Next->insert(n-1,ins);
      } else Next = new StringList(ins);
}

long StringList::iselement(String el, unsigned long n) {
  if (el == s) return n;
  else {
    if (Next) return Next->iselement(el,n+1);
    else return -1;
  }
}

long StringList::contains(String substr, unsigned long n) {
  if (s.contains(substr)) return n;
  else {
    if (Next) return Next->contains(substr,n+1);
    else return -1;
  }
}

long StringList::first_contained_in(String & sourcestr, unsigned long n) {
  if (sourcestr.contains(s)) return n;
  else {
    if (Next) return Next->first_contained_in(sourcestr,n+1);
    else return -1;
  }
}

unsigned long StringList::split(unsigned long n, String cs, const char sep) {
// insert list elements from a string with a specified separator
  if (cs.length()<=0) return length();
  String el;
  if (cs.index(sep)>=0) el = cs.before(sep); else el = cs;
  // *** this is not a speed optimized implementation
  insert(n,el);
  split(n+1,cs.after(sep),sep);
  return length();
}

unsigned long StringList::split(unsigned long n, String & cs, const String sep, int csstart) {
// insert list elements from a string with a specified separator
  if (cs.length() <= (unsigned int) csstart) return length();
  String el; int seploc;
  if ((seploc=cs.index(sep,csstart))>=0) el = cs.at(csstart,seploc-csstart);
  else el = cs.from(csstart);
  insert(n,el);
  split(n+1,cs,sep,seploc+sep.length());
  return length();
}

String StringList::concatenate(String sepstr) {
  if (Next) return s+sepstr+Next->concatenate(sepstr);
  else return s;
}

LongList::LongList(String cs, const char sep) {
// initialize list from a string with a specified separator
// (Note: In the current implementation this leaves an unused
// list element at the end of the list.)
  Next=NULL;
  split(0,cs,sep);
}

long & LongList::operator[](unsigned long n) {
  if (!n) return s;
  if (!Next) Next = new LongList();
  return (*Next)[n-1];
}

void LongList::append(unsigned long n, long & ins) {
  if (!n) Next = new LongList(ins,Next);
  else
    if (Next) Next->append(n-1,ins);
    else {
      Next = new LongList();
      Next->append(n-1,ins);
    }
}

void LongList::insert(unsigned long n, long & ins) {
  if (!n) { Next = new LongList(s,Next); s = ins; }
  else
    if (Next) Next->insert(n-1,ins);
    else 
      if (n>1) {
	Next = new LongList();
	Next->insert(n-1,ins);
      } else Next = new LongList(ins);
}

long LongList::iselement(long el, unsigned long n) {
  if (el == s) return n;
  else {
    if (Next) return Next->iselement(el,n+1);
    else return -1;
  }
}

long LongList::contains(long substr, unsigned long n) {
  return iselement(substr,n);
}

unsigned long LongList::split(unsigned long n, String cs, const char sep) {
// insert list elements from a string with a specified separator
  if (cs.length()<=0) return length();
  String el;
  if (cs.index(sep)>=0) el = cs.before(sep); else el = cs;
  // *** this is not a speed optimized implementation
  long elval = atol(el.chars());
  insert(n,elval);
  split(n+1,cs.after(sep),sep);
  return length();
}

String LongList::concatenate(String sepstr) {
  if (Next) return String(s)+sepstr+Next->concatenate(sepstr);
  else return String(s);
}

Quick_String_Hash * Quick_String_Hash::add_list_element(int i) {
// Note: currently there is no way for this to return NULL, since allocation
// errors are not caught.
  if (i<0) i = 0; // reduce the impact of roll-over errors
  if (!list) { // allocate list of pointers
    len = 1;
    list = new Quick_String_Hash*[len];
#ifdef CONST_QUICK_STRING_HASH_ENDNODE
    list[0] = const_cast<Quick_String_Hash *>(&_qsh_endnode);
#else
    list[0] = new Quick_String_Hash();
#endif
    offset = i;
    return list[0];
  }
  if (i<offset) { // prepend to the list
    int oldlen = len;
    len += (offset-i);
    offset = i;
    Quick_String_Hash ** oldlist = list;
    list = new Quick_String_Hash*[len];
    for (int j = 0; j<len; j++) list[j] = NULL; // initialize list
#ifdef CONST_QUICK_STRING_HASH_ENDNODE
    list[0] = const_cast<Quick_String_Hash *>(&_qsh_endnode);
#else
    list[0] = new Quick_String_Hash();
#endif
    for (int j = oldlen-1, k = len-1; j>=0; j--, k--) list[k] = oldlist[j];
    delete[] oldlist;
    return list[0];
  }
  i -= offset;
  if (i>=len) { // append to the list
    int oldlen = len;
    len = i+1;
    Quick_String_Hash ** oldlist = list;
    list = new Quick_String_Hash*[len];
    for (int j = 0; j<len; j++) list[j] = NULL; // initialize list
#ifdef CONST_QUICK_STRING_HASH_ENDNODE
    list[i] = const_cast<Quick_String_Hash *>(&_qsh_endnode);
#else
    list[i] = new Quick_String_Hash();
#endif
    for (int j = 0; j<oldlen; j++) list[j] = oldlist[j];
    delete[] oldlist;
    return list[i];
  }
  if (!list[i]) { // add list element
#ifdef CONST_QUICK_STRING_HASH_ENDNODE
    list[i] = const_cast<Quick_String_Hash *>(&_qsh_endnode);
#else
    list[i] = new Quick_String_Hash();
#endif
    return list[i];
  }
  return list[i];
}

Quick_String_Hash & Quick_String_Hash::insert(const char * s) {
// *** can adapt this to the faster method according to results obtained with read and read_recursive
  if (s[0]=='\0') return (*this);
  int i = ((int) s[0]);	// implied list coordinate
  int r = i - offset;		// real list coordinate
  if (r<0) {
    if (add_list_element(i)==NULL) return (*this);
    r = i - offset;	// adjust to new offset
  } else if (r>=len) {
    if (add_list_element(i)==NULL) return (*this); // this also takes care of the case where the list is unallocated
    r = i - offset;	// adjust to new offset
  } else if (!list[r]) {
    if (add_list_element(i)==NULL) return (*this);
  }
#ifdef CONST_QUICK_STRING_HASH_ENDNODE
  if ((s[1]!='\0') && (list[r]==&_qsh_endnode)) list[r] = new Quick_String_Hash(); // allocate so that list can be made
#endif
  list[r]->insert(&s[1]);
  return (*this);
}

#ifdef RECURSIVE_QUICK_STRING_HASH
int Quick_String_Hash::read(const char * s) const {
  // Determines if the next character in s exists in the tree
  if (s[0]=='\0') return 1;
  int i = ((int) s[0]);	// implied list coordinate
  if (i<offset) return 0;
  i -= offset;			// real list coordinate
  if (i>=len) return 0;	// this also takes care of the case where the list is unallocated
  if (list[i]) return list[i]->read(&s[1]);
  return 0;
}
#else
int Quick_String_Hash::read(const char * s) const {
  // Determines if the next character in s exists in the tree
  int i; const Quick_String_Hash * qsh = this;
  for (const char * c = s; (*c!='\0'); c++) {
    i = ((int) *c);	// implied list coordinate
    if (i<qsh->offset) return 0;
    i -= qsh->offset;	// real list coordinate
    if (i>=qsh->len) return 0; // this also takes care of the case where the list is unallocated
    if (!qsh->list[i]) return 0;
    qsh = qsh->list[i];	// traverse tree
  }
  return 1;
}
#endif

unsigned long Quick_String_Hash::nodes() const {
#ifdef CONST_QUICK_STRING_HASH_ENDNODE
  if (this==&_qsh_endnode) return 0;
#endif
  unsigned long res = 1;
  for (int i=0; i<len; i++) if (list[i]) res += list[i]->nodes();
  return res;
}

unsigned long Quick_String_Hash::size() const {
#ifdef CONST_QUICK_STRING_HASH_ENDNODE
  if (this==&_qsh_endnode) return 0;
#endif
  unsigned long res = sizeof(*this);
  for (int i=0; i<len; i++) if (list[i]) res += list[i]->size();
  return res;
}
#ifdef CONST_QUICK_STRING_HASH_ENDNODE
const Quick_String_Hash _qsh_endnode;
#endif

int Directory_Access::read(String & filenames, char separator) {
  // reads the directory into a String
  // (using String as the buffer keeps Directory_Access independent
  // of more specialized classes, such as StringList)
  // note: the separator character is placed at the start of each
  // filename - calls to read can therefore concatenate directories
  // into one String
  // returns -1 if an error occurred, otherwise returns number of
  // filenames found
  struct dirent *d;
  DIR * D = opendir(dirpath);
  if (!D) return -1;
  int num;
  for (num = 0; (d = readdir(D)); num++) filenames += separator+String(d->d_name);
  return num;
}

Novelty_Marker::Novelty_Marker(int hdepth): id(""), marktext(""), inclurl(""), imp(NULL), impnum(0), len(0.0), hierdepth(hdepth), nmtype(NM_NOVELTY_ITEM) {
  context[0] = "";
  section[0] = "";
}

int Novelty_Marker::Get_Novelty_from_Paper_Plan(String & pptext, int startp, int starthierdepth) {
// searches for Novelty marker data in a paper plan, returns the location at
// which one was found and sets all object variables to match, returns -1 if
// none was found, starts searching at character number startp in pptext
  int nmidx, hierstart, hierend = -1;
  //	if (starthierdepth>0) {
  hierdepth = starthierdepth; // set to current depth
  hierend = pptext.index("</UL>",startp);
  //	}
  nmidx = pptext.index(BigRegex("[<]B[>][ 	]*Section[ 	]*[<]/B[>][ 	]*:"),startp);
  hierstart = pptext.index(BigRegex("[<]!--[ 	]*@TOPICAL CONTEXT@[ 	]*--[>]"),startp);
  if ((hierend<0) && (nmidx<0) && (hierstart<0)) return -1;
  if (((unsigned int) nmidx < (unsigned int) hierstart) && ((unsigned int) nmidx < (unsigned int) hierend)) { // get item
#ifdef DEBUG_NOVELTYMARKER
    cout << '!';
#endif
    nmtype = NM_NOVELTY_ITEM;
    int nmend = pptext.index(")",nmidx);
#ifdef DEBUG_NOVELTYMARKER
    if (nmend<0) cout << '>';
#endif
    if (nmend<0) return -1;
    nmidx = pptext.index("<LI>",nmidx-pptext.length());
#ifdef DEBUG_NOVELTYMARKER
    if (nmidx<startp) cout << '<';
#endif
    if (nmidx<startp) return -1;
    String nmexcerpt = pptext.at(nmidx,(nmend-nmidx)+1);
    // get marked text
    marktext = nmexcerpt.before("(",-1);
    marktext.gsub("<BR>\n","");
    // identify REQUIRED item
    if (marktext.contains(BigRegex("[<]LI[>][ 	]*\\([<][^>]*[>]\\)?[ 	]*REQUIRE[ 	]*:"))) {
      nmtype = NM_REQUIRED_ITEM;
      marktext.del(BigRegex("[<]LI[>][ 	]*\\([<][^>]*[>]\\)?[ 	]*REQUIRE[ 	]*:[ 	]*\\([<][^>]*[>]\\)?[ 	]*"));
    } else {
      // get source file
      source = marktext.at(BigRegex("[<]A[ 	]+[^>]*[Hh][Rr][Ee][Ff][^\"]*\"[^\"]+\""));
      source.del(BigRegex("[<]A[ 	]+[^>]*[Hh][Rr][Ee][Ff][^\"]*\""));
      source.del("\"");
      // get NOVELTY ID
      id = source.after("#");
      //cout << "NMID=" << id << '\n'; cout.flush();
      // clean up source file
      if (id!="") source = source.before("#");
      // clean up marked text
      marktext.gsub(BigRegex("[[][^]]*NOVELTY-[^]]*[]]"),"");
    }
    // clean up marked text
    marktext.gsub(BigRegex("[<][^>]*[>]"),"");
    // get contexts
    nmexcerpt = nmexcerpt.after(BigRegex("([^C]*Context[^:]*:[ 	]*"),-1);
    String c = nmexcerpt.before(BigRegex("\\([<][^>]*[>]\\)?[ 	]*Importance[ 	]*\\([<][^>]*[>]\\)?[ 	]*:"));
    int cidx = 0; String cc;
    while ((cc = c.at(BigRegex("[^ 	][^,]*")))!="") {
      cc.del(BigRegex("[ 	]*$"));
      context[cidx] = cc;
      cidx++;
      c = c.after(BigRegex("[^ 	][^,]*,?"));
    }
    // get importances
    nmexcerpt = nmexcerpt.after(BigRegex("\\([<][^>]*[>]\\)?[ 	]*Importance[ 	]*\\([<][^>]*[>]\\)?[ 	]*:[ 	]*"));
    c = nmexcerpt.before(BigRegex("\\([<][^>]*[>]\\)?[ 	]*Length[ 	]*\\([<][^>]*[>]\\)?[ 	]*:"));
    if (imp) delete[] imp;
    impnum = context.length();
    imp = new float[impnum];
    for (int i=0; i<impnum; i++) imp[i] = 0.0;
    cidx = 0;
    while ((cc = c.at(BigRegex("[^ 	][^,]*")))!="") {
      cc.del(BigRegex("[ 	]*$"));
      imp[cidx] = (float) atof((const char *) cc);
      cidx++;
      c = c.after(BigRegex("[^ 	][^,]*,?"));
    }
    if (!cidx) {
      delete[] imp;
      imp = NULL;
      impnum = 0;
    }
    if (cidx==1) {
      float * impone = new float[1];
      impone[0] = imp[0];
      delete[] imp;
      imp = impone;
      impnum = 1;
    }
    // get length
    nmexcerpt = nmexcerpt.after(BigRegex("\\([<][^>]*[>]\\)?[ 	]*Length[ 	]*\\([<][^>]*[>]\\)?[ 	]*:[ 	]*"));
    c = nmexcerpt.before(BigRegex("\\([<][^>]*[>]\\)?[ 	]*Section[ 	]*\\([<][^>]*[>]\\)?[ 	]*:"));
    if ((cc = c.at(BigRegex("[^ 	][^,]*")))!="") {
      cc.del(BigRegex("[ 	]*$"));
      len = (float) atof((const char *) cc);
    }
    // get sections
    nmexcerpt = nmexcerpt.after(BigRegex("\\([<][^>]*[>]\\)?[ 	]*Section[ 	]*\\([<][^>]*[>]\\)?[ 	]*:[ 	]*"));
    c = nmexcerpt.before(")");
    cidx = 0;
    while ((cc = c.at(BigRegex("[^ 	][^,]*")))!="") {
      cc.del(BigRegex("[ 	]*$"));
      section[cidx] = cc;
      cidx++;
      c = c.after(BigRegex("[^ 	][^,]*,?"));
    }
    return nmend;
  } else if ((unsigned int) hierstart < (unsigned int) hierend) { // increase hierarchy depth
#ifdef DEBUG_NOVELTYMARKER
    cout << '^';
#endif
    nmtype = NM_TCH_DEPTH_INC;
    String nmexcerpt = pptext.at(BigRegex("[>][^\n]+"),hierstart);
    if (nmexcerpt=="") {
      EOUT << "dil2al: Incomplete @TOPICAL CONTEXT@ detected in Novelty_Marker::Get_Novelty_from_Paper_Plan()\n";
      return -1;
    }
    nmexcerpt.del(">");
    nmexcerpt.gsub(BigRegex("[<][^>]*[>]"),"");
    context[0] = nmexcerpt;
    hierdepth++;
    return hierstart+nmexcerpt.length()+1;
  } else { // decrease hierarchy depth
#ifdef DEBUG_NOVELTYMARKER
    cout << 'v';
#endif
    nmtype = NM_TCH_DEPTH_DEC;
    hierdepth--;
    if (hierdepth<0) return -1;	// beyond end of TCH
    return hierend+4;
  }
}

Novelty_Marker & Novelty_Marker_List::operator[](unsigned long n) {
  if (!n) return s;
  if (!Next) Next = new Novelty_Marker_List();
  return (*Next)[n-1];
}

void Novelty_Marker_List::append(unsigned long n, Novelty_Marker & ins) {
  if (!n) Next = new Novelty_Marker_List(ins,Next);
  else
    if (Next) Next->append(n-1,ins);
    else {
      Next = new Novelty_Marker_List();
      Next->append(n-1,ins);
    }
}

void Novelty_Marker_List::insert(unsigned long n, Novelty_Marker & ins) {
  if (!n) { Next = new Novelty_Marker_List(s,Next); s = ins; }
  else
    if (Next) Next->insert(n-1,ins);
    else 
      if (n>1) {
	Next = new Novelty_Marker_List();
	Next->insert(n-1,ins);
      } else Next = new Novelty_Marker_List(ins);
}

long Novelty_Marker_List::iselement(Novelty_Marker & el, unsigned long n) {
  if ((el.source == s.source) && (el.id == s.id)) return n;
  else {
    if (Next) return Next->iselement(el,n+1);
		else return -1;
  }
}

void Novelty_Marker_List::Get_Context_Ranks(StringList & cc, LongList & ccr) {
// creates a list of concatenated contexts and a ranking according to their
// average Novelty Item importance parameter values (rank 0 is highest)
  int cclen = 0, i; String s; LongList ccnum;
  for (Novelty_Marker_List * nml = this; (nml); nml=nml->get_Next()) if ((*nml)[0].nmtype==Novelty_Marker::NM_NOVELTY_ITEM) {
      s = (*nml)[0].context.concatenate(", ");
      if ((i=cc.iselement(s))<0) { // add new concatenated context
	cc[cclen] = s; ccnum[cclen] = 0; i = cclen; cclen++;
      }
      ccnum[i]++;
      float imp = 0.0;
      for (int j = 0; j<(*nml)[0].impnum; j++) imp += (*nml)[0].imp[j];
      if ((*nml)[0].impnum>1) imp /= (float) (*nml)[0].impnum; // average importance of concatenated contexts of Novelty Item
      ccr[i] = (long) (imp*1000.0);
    }
  for (i=0; i<cclen; i++) ccr[i] /= ccnum[i]; // average importances of concatenated contexts
  long biggest, rank = 0, cctodo = cclen;
  while (cctodo>0) {
    // find biggest
    biggest = -1;
    for (i=0; i<cclen; i++) if (ccr[i]>biggest) biggest = ccr[i];
    if (biggest<0) break;
    // set all at biggest size to next rank
    for (i=0; i<cclen; i++) if (ccr[i]==biggest) {
			ccnum[i] = rank; ccr[i] = -1; cctodo--;
      }
    rank++;
  }
  for (i=0; i<cclen; i++) ccr[i] = ccnum[i];
}

String DIL_ID::str() const {
  static String idstr; // stored in function, not objects, static to avoid problems with lifetime of object that return value points to
  idstr = String((long) idmajor[1]);
  while (idstr.length()<6) idstr.prepend('0');
  idstr.prepend(String((long) idmajor[0]));
  idstr += '.' + String((long) idminor);
  /*cout << "chars() RESULT at end of str() function: ";
    int i = 0;
    for (char * tp = (char *) idstr.chars(); (*tp!='\0') && (i<80); tp++, i++) cout << (*tp) << '{' << (int) (*tp) << '}';
    cout << '\n';*/
  return idstr;
}

bool DIL_ID::Write_DIL_ID_to_Binary_Cache(ofstream & cfl) {
  // DIL_ID
  cfl.write((const char *) this, sizeof(DIL_ID));
  return true;
}

bool DIL_ID::Read_DIL_ID_from_Binary_Cache(ifstream & cfl) {
  // DIL_ID
  if ((cfl.read((READSOMETYPE) this, sizeof(DIL_ID))).gcount() < (streamsize) sizeof(DIL_ID)) return false;
  // alternative code:
  //  if ((cfl.read((READSOMETYPE) idmajor, sizeof(idnumber)*2)).gcount()<(sizeof(idnumber)*2)) return false;
  //  if ((cfl.read((READSOMETYPE) (&idminor), sizeof(idminor))).gcount()<sizeof(idminor)) return false;
  return true;
}

void Plan_entry_content::Parse_Entry_Content(int parsefrom) {
  // find plan specific information in DIL entry text content
  if (planentrytype==PLAN_ENTRY_TYPE_GOAL) {
    String & ctxt = *(entry->content->text);
    int i;
    // note: This assumes that action alternatives are elements of the first
    // ordered list in the text.
    if ((i=ctxt.index("<OL>",parsefrom))>=0) {
      String listparams, listcontent;
      if ((i=HTML_get_list(ctxt,i,listparams,listcontent))>=0) {
	HTML_remove_comments(listcontent);
	String itemcontent; int j = 0; decision = -1;
	while ((j=HTML_get_list_item(listcontent,j,listparams,itemcontent))>=0) {
	  decision++;
	  // *** The decision tag has not yet been tested
	  if (itemcontent.contains(PLAN_ENTRY_DECISION_TAG)) {
	    String hreftext;
	    if (HTML_get_href(itemcontent,0,listparams,hreftext)>=0) {
	      decisionID = hreftext;
	      break;
	    }
	  }
	}
	if (decision==0) if (HTML_get_href(itemcontent,0,listparams,listcontent)>=0) decisionID = listcontent;
	if ((decision>0) && (!decisionID.valid())) decision = -1; // alternatives exist, but no decision specified
      }
    }
  }
}

DIL_entry_content::DIL_entry_content(DIL_entry & e): entry(&e), planentrytype(PLAN_ENTRY_TYPE_UNASSIGNED), hasextraparameters(EXTRA_PARAMETERS_UNDETERMINED), plancontent(NULL), started(0), required(0), completion(0.0), valuation(0.0), text(NULL) {}

void DIL_entry_content::set_TL_tail_reference(String tailurl) {
  TL_tail.title=tailurl.after('#');
  if (TL_tail.title.empty()) TL_tail.file=tailurl;
  else TL_tail.file=tailurl.before('#');
}

void DIL_entry_content::set_TL_head_reference(String headurl) {
  TL_head.title=headurl.after('#');
  if (TL_head.title.empty()) TL_head.file=headurl;
  else TL_head.file=headurl.before('#');
}

int DIL_entry_content::Is_Plan_Entry() {
  // attempts to determine if the DIL entry content contains PLAN entry data
  // *** this should be upgraded to obtain the DIL entry content if it is
  //     not available
  if (planentrytype!=PLAN_ENTRY_TYPE_UNASSIGNED) return planentrytype;
  if (!text) {
    WARN << "DIL_entry_content::Is_Plan_Entry(): No content->text available\n --- obtaining content->text automatically is not yet implemented. Continuing as is.\n";
    return PLAN_ENTRY_TYPE_UNASSIGNED;
  }
  String & ctxt = (*text);
  // Note: The following expects a specific tag, including spaces.
  int j,k; planentrytype = PLAN_ENTRY_TYPE_NONE;
  if ((j=ctxt.index(PLAN_ENTRY_IDENTIFIER_TAG))<0) return PLAN_ENTRY_TYPE_NONE;
  j += PLAN_ENTRY_IDENTIFIER_TAG_LENGTH;
  if ((k=ctxt.index('@',j))<0) return PLAN_ENTRY_TYPE_NONE;
  String idtagstr(ctxt.at(j,k-j));
  for (j=0; j<NUM_PLAN_ENTRY_TYPES; j++) if (idtagstr==plan_entry_type_strings[j]) { planentrytype=j+1; break; }
  // provide other information
  if (planentrytype!=PLAN_ENTRY_TYPE_NONE) plancontent = new Plan_entry_content(*entry,planentrytype,k);
  return planentrytype;
}

int DIL_entry_content::Has_Extra_Parameters() {
  // attempts to determine if the DIL entry content contains extra parameters
  // for dil2al to take into account with regards to this DIL entry
  // *** this should be upgraded to obtain the DIL entry content if it is
  //     not available
  if (hasextraparameters!=EXTRA_PARAMETERS_UNDETERMINED) return hasextraparameters;
  if (!text) {
    WARN << "DIL_entry_content::Has_Extra_Parameters(): Warning - no content->text available\n --- obtaining content->text automatically is not yet implemented. ontinuing as is.\n";
    return EXTRA_PARAMETERS_UNDETERMINED;
  }
  String & ctxt = (*text);
  // Note: The following expects a specific tag, including spaces.
  int j,k; hasextraparameters = EXTRA_PARAMETERS_NONE;
  if ((j=ctxt.index(EXTRA_PARAMETERS_IDENTIFIER_TAG))<0) return EXTRA_PARAMETERS_NONE;
  j += EXTRA_PARAMETERS_IDENTIFIER_TAG_LENGTH;
  if ((k=ctxt.index('@',j))<0) return EXTRA_PARAMETERS_NONE;
  hasextraparameters = EXTRA_PARAMETERS_AVAILABLE;
  extraparameters = new Extra_DIL_entry_parameters(*entry,k);
  return hasextraparameters;
}

bool DIL_entry_content::Write_Topical_to_Binary_Cache(ofstream & cfl, bool gottext) {
  // started
  cfl.write((const char *) (&started), sizeof(started));
  // required
  cfl.write((const char *) (&required), sizeof(required));
  // completion
  cfl.write((const char *) (&completion), sizeof(completion));
  // valuation
  cfl.write((const char *) (&valuation), sizeof(valuation));
  // TL_tail
  int tailfilelen = TL_tail.file.length();
  cfl.write((const char *) (&tailfilelen), sizeof(tailfilelen));
  if (tailfilelen>0) cfl.write((const char *) TL_tail.file.chars(), tailfilelen);
  int tailnamelen = TL_tail.title.length();
  cfl.write((const char *) (&tailnamelen), sizeof(tailnamelen));
  if (tailnamelen>0) cfl.write((const char *) TL_tail.title.chars(), tailnamelen);
  // TL_head
  int headfilelen = TL_head.file.length();
  cfl.write((const char *) (&headfilelen), sizeof(headfilelen));
  if (headfilelen>0) cfl.write((const char *) TL_head.file.chars(), headfilelen);
  int headnamelen = TL_head.title.length();
  cfl.write((const char *) (&headnamelen), sizeof(headnamelen));
  if (headnamelen>0) cfl.write((const char *) TL_head.title.chars(), headnamelen);
  // text
  if (gottext) { // *** WHOA! This looks dangerous! Could this corrupt binary cache files??
    int textlen = 0;
    if (text) textlen = text->length();
    cfl.write((const char *) (&textlen), sizeof(textlen));
    cfl.write((const char *) text->chars(), textlen);
  }
  return true;
}

bool DIL_entry_content::Read_Topical_from_Binary_Cache(ifstream & cfl, bool gottext) {
  const unsigned int LLEN = 10240;
  char lbuf[LLEN];
  // started
  if ((cfl.read((READSOMETYPE) (&started), sizeof(started))).gcount() < (streamsize) sizeof(started)) return false;
  // required
  if ((cfl.read((READSOMETYPE) (&required), sizeof(required))).gcount() < (streamsize) sizeof(required)) return false;
  // completion
  if ((cfl.read((READSOMETYPE) (&completion), sizeof(completion))).gcount() < (streamsize) sizeof(completion)) return false;
  // valuation
  if ((cfl.read((READSOMETYPE) (&valuation), sizeof(valuation))).gcount() < (streamsize) sizeof(valuation)) return false;
  // TL_tail
  unsigned int tailfilelen;
  if ((cfl.read((READSOMETYPE) (&tailfilelen), sizeof(tailfilelen))).gcount() < (streamsize) sizeof(tailfilelen)) return false;
  TEST_LBUF_BOUNDS(tailfilelen,"DIL_Topical_List::Read_from_Binary_Cache()")
  if (static_cast<streamsize>(cfl.read((READSOMETYPE) lbuf, tailfilelen).gcount()) < static_cast<streamsize>(tailfilelen)) return false;
  lbuf[tailfilelen]='\0';
  TL_tail.file=lbuf;
  unsigned int tailnamelen;
  if ((cfl.read((READSOMETYPE) (&tailnamelen), sizeof(tailnamelen))).gcount() < (streamsize) sizeof(tailnamelen)) return false;
  TEST_LBUF_BOUNDS(tailnamelen,"DIL_Topical_List::Read_from_Binary_Cache()")
  if (static_cast<streamsize>(cfl.read((READSOMETYPE) lbuf, tailnamelen).gcount()) < static_cast<streamsize>(tailnamelen)) return false;
  lbuf[tailnamelen]='\0';
  TL_tail.title=lbuf;
  // TL_head
  unsigned int headfilelen;
  if ((cfl.read((READSOMETYPE) (&headfilelen), sizeof(headfilelen))).gcount() < (streamsize) sizeof(headfilelen)) return false;
  TEST_LBUF_BOUNDS(headfilelen,"DIL_Topical_List::Read_from_Binary_Cache()")
  if (static_cast<streamsize>(cfl.read((READSOMETYPE) lbuf, headfilelen).gcount()) < static_cast<streamsize>(headfilelen)) return false;
  lbuf[headfilelen]='\0';
  TL_head.file=lbuf;
  unsigned int headnamelen;
  if ((cfl.read((READSOMETYPE) (&headnamelen), sizeof(headnamelen))).gcount() < (streamsize) sizeof(headnamelen)) return false;
  TEST_LBUF_BOUNDS(headnamelen,"DIL_Topical_List::Read_from_Binary_Cache()")
  if (static_cast<streamsize>(cfl.read((READSOMETYPE) lbuf, headnamelen).gcount()) < static_cast<streamsize>(headnamelen)) return false;
  lbuf[headnamelen]='\0';
  TL_head.title=lbuf;
  // text
  if (gottext) {
    unsigned int textlen;
    if ((cfl.read((READSOMETYPE) (&textlen), sizeof(textlen))).gcount() < (streamsize) sizeof(textlen)) return false;
    TEST_LBUF_BOUNDS(textlen,"DIL_Topical_List::Read_from_Binary_Cache()")
      // Note that the following casts on both the left and right hand sides
      // are needed for compatibility with GCC versions < 3.0, since gcount
      // there returns a type _IO_size_t and streamsize is defined as a type
      // _IO_ssize_t. Only the right hand side cast is needed for GCC
      // versions >= 3.0 and may also be a traditional (streamsize) cast.
      // Also, it only seems to be necessary when TEST_LBUF_BOUNDS precedes
      // it. It may well be no more than a preprocessor or older GCC bug.
      if (static_cast<streamsize>(cfl.read((READSOMETYPE) lbuf, textlen).gcount()) < static_cast<streamsize>(textlen)) return false;
    lbuf[textlen]='\0';
    delete text; // remove prior allocation
    text = new String(lbuf);
  }
  return true;
}

bool DIL_entry_content::Binary_Cache_Diagnostic(DIL_entry_content * cached,int & testedbytes) {
  // test if the quick load cache process works reliably
  // this is the original entry_content, cached is the one retrieved from the binary cache
  int localtestedbytes = 0;
  // *** Add caching and diagnostics for Plan_entry_content
  localtestedbytes += sizeof(planentrytype) + sizeof(plancontent);
  if (plancontent) {
    localtestedbytes += sizeof(Plan_entry_content);
    WARN << "Implementation Warning - Plan_entry_content is not yet cached!\n";
  }
  if ((*entry)!=(*(cached->entry))) INFO << "*** DIL_entry_content entry mismatch\n"; // testing if DIL_ID==DIL_ID
  localtestedbytes += sizeof(entry);
  if (started!=cached->started) INFO << "*** DIL_entry_content started mismatch\n";
  localtestedbytes += sizeof(started);
  if (required!=cached->required) INFO << "*** DIL_entry_content required mismatch\n";
  localtestedbytes += sizeof(required);
  if (completion!=cached->completion) INFO << "*** DIL_entry_content completion mismatch\n";
  localtestedbytes += sizeof(completion);
  if (valuation!=cached->valuation) INFO << "*** DIL_entry_content valuation mismatch\n";
  localtestedbytes += sizeof(valuation);
  if (TL_tail.file!=cached->TL_tail.file) INFO << "*** DIL_entry_content TL_tail.file mismatch\n";
  localtestedbytes += TL_tail.file.length();
  if (TL_tail.title!=cached->TL_tail.title) INFO << "*** DIL_entry_content TL_tail.title mismatch\n";
  localtestedbytes += TL_tail.title.length();
  if (TL_head.file!=cached->TL_head.file) INFO << "*** DIL_entry_content TL_head.file mismatch\n";
  localtestedbytes += TL_head.file.length();
  if (TL_head.title!=cached->TL_head.title) INFO << "*** DIL_entry_content TL_head.title mismatch\n";
  localtestedbytes += TL_head.title.length();
  if (text) {
    if (cached->text) {
      if ((*text)!=(*(cached->text))) INFO << "*** DIL_entry_content text mismatch\n";
      localtestedbytes += text->length();
    } else INFO << "*** DIL_entry_content original has text, but cached does not\n";
  } else if (cached->text) INFO << "*** DIL_entry_content cached has text, but original does not\n";
  localtestedbytes += sizeof(text);
  testedbytes += localtestedbytes;
  return true;
}

DIL_Topical_List::DIL_Topical_List(DIL_entry & e): entry(&e), relevance(0.0) {}

DIL_Topical_List::DIL_Topical_List(DIL_entry & e, String tfile, String ttitle, float trelevance): entry(&e), relevance(trelevance) {
  dil.file = tfile;
  dil.title = ttitle;
}

bool DIL_Topical_List::Write_to_Binary_Cache(ofstream & cfl) {
  // dil.file
  int dilfilelen = dil.file.length();
  cfl.write((const char *) (&dilfilelen), sizeof(dilfilelen));
  cfl.write((const char *) dil.file.chars(), dilfilelen);
  // dil.title
  int diltitlelen = dil.title.length();
  cfl.write((const char *) (&diltitlelen), sizeof(diltitlelen));
  cfl.write((const char *) dil.title.chars(), diltitlelen);
  // relevance
  cfl.write((const char *) (&relevance), sizeof(relevance));
  return true;
}

bool DIL_Topical_List::Read_from_Binary_Cache(ifstream & cfl) {
  const unsigned int LLEN = 1024;
  char lbuf[LLEN];
  // dil.file
  unsigned int dilfilelen;
  if ((cfl.read((READSOMETYPE) (&dilfilelen), sizeof(dilfilelen))).gcount() < (streamsize) sizeof(dilfilelen)) return false;
  TEST_LBUF_BOUNDS(dilfilelen,"DIL_Topical_List::Read_from_Binary_Cache()")
  if (static_cast<streamsize>((cfl.read((READSOMETYPE) lbuf, dilfilelen)).gcount()) < static_cast<streamsize>(dilfilelen)) return false;
  lbuf[dilfilelen]='\0';
  dil.file = lbuf;
  // dil.title
  unsigned int diltitlelen;
  if ((cfl.read((READSOMETYPE) (&diltitlelen), sizeof(diltitlelen))).gcount() < (streamsize) sizeof(diltitlelen)) return false;
  TEST_LBUF_BOUNDS(diltitlelen,"DIL_Topical_List::Read_from_Binary_Cache()")
  if (static_cast<streamsize>((cfl.read((READSOMETYPE) lbuf, diltitlelen)).gcount()) < static_cast<streamsize>(diltitlelen)) return false;
  lbuf[diltitlelen]='\0';
  dil.title = lbuf;
  // relevance
  if ((cfl.read((READSOMETYPE) (&relevance), sizeof(relevance))).gcount() < (streamsize) sizeof(relevance)) return false;
  return true;
}

bool DIL_Topical_List::Binary_Cache_Diagnostic(DIL_Topical_List * cached, int & testedbytes) {
  // test if the quick load cache process works reliably
  // this is the original Topical_List, cached is the one retrieved from the binary cache
  //   start of this object test
  int localtestedbytes = sizeof(PLLHandle<DIL_Topical_List>);
  if ((*entry)!=(*(cached->entry))) INFO << "*** DIL_Topical_List entry mismatch\n"; // testing if DIL_ID==DIL_ID
  localtestedbytes += sizeof(entry);
  if (dil.file!=cached->dil.file) INFO << "*** DIL_Topical_List dil.file mismatch\n";
  if (dil.title!=cached->dil.title) INFO << "*** DIL_Topical_List dil.title mismatch\n";
  localtestedbytes += sizeof(dil);
  if (relevance!=cached->relevance) INFO << "*** DIL_Topical_List relevance mismatch\n";
  localtestedbytes += sizeof(relevance);
  //   end of this object test
  if (localtestedbytes!=sizeof(DIL_Topical_List)) INFO << "*** testedbytes!=sizeof(DIL_Topical_List)\n";
  testedbytes += localtestedbytes;
  return true;
}

// *** Beware, I changed the default target date value from 0 to unspecified here!
DIL_AL_List::DIL_AL_List(DIL_entry & e): entry(&e), superior(NULL), _targetdate(DILSUPS_TD_UNSPECIFIED), _tdproperty(0),
#ifdef INCLUDE_EXACT_TARGET_DATES
					 _tdexact(false), _tdperiod(pt_nonperiodic), _tdspan(0), _tdevery(1),
#endif
					 relevance(DILSUPS_REL_UNSPECIFIED), unbounded(DILSUPS_UNB_UNSPECIFIED), bounded(DILSUPS_BND_UNSPECIFIED), urgency(DILSUPS_URG_UNSPECIFIED), priority(DILSUPS_PRI_UNSPECIFIED) {}

DIL_AL_List::DIL_AL_List(DIL_entry & e, String pfile, String ptitle, float prelevance, float punbounded, float pbounded, time_t ptargetdate, int ptdprop,
#ifdef INCLUDE_EXACT_TARGET_DATES
			 bool ptdexact, periodictask_t ptdperiod, int ptdspan, int ptdevery,
#endif
			 float purgency, float ppriority): entry(&e), superior(NULL), _targetdate(ptargetdate), _tdproperty(ptdprop),
#ifdef INCLUDE_EXACT_TARGET_DATES
							   _tdexact(ptdexact), _tdperiod(ptdperiod), _tdspan(ptdspan), _tdevery(ptdevery),
#endif
							   relevance(prelevance), unbounded(punbounded), bounded(pbounded), urgency(purgency), priority(ppriority) {
  al.file = pfile;
  al.title = ptitle;
}

#ifdef INCLUDE_EXACT_TARGET_DATES
void DIL_AL_List::set_tdperiod(periodictask_t period) {
  if (period<=pt_nonperiodic) {
    _tdperiod = period;
    if (_tdperiod==pt_nonperiodic) set_tdproperty(tdproperty() & (~DILSUPS_TDPROP_PERIODIC)); // corrected 20070705 to fix char#8 problem
  } else {
    ERROR << "DIL_AL_List::set_tdperiod(): Unable to set periodicity to enumerated values greater than pt_nonperiodic, parameter unchanged.\n";
  }
}
#endif

bool DIL_AL_List::Write_to_Binary_Cache(ofstream & cfl) {
  // _targetdate
  cfl.write((const char *) (&_targetdate), sizeof(_targetdate));
  // _tdproperty
#ifdef INCLUDE_EXACT_TARGET_DATES
  if (_tdexact) _tdproperty = _tdproperty | DILSUPS_TDPROP_FIXED | DILSUPS_TDPROP_EXACT; // insure properties sanity
#endif
  cfl.write((const char *) (&_tdproperty), sizeof(_tdproperty));
#ifdef INCLUDE_EXACT_TARGET_DATES
  // _tdperiod
  cfl.write((const char *) (&_tdperiod), sizeof(_tdperiod));
  // _tdspan
  cfl.write((const char *) (&_tdspan), sizeof(_tdspan));
  // _tdevery
  cfl.write((const char *) (&_tdevery), sizeof(_tdevery));
#endif
  // al.file
  int alfilelen = al.file.length();
  cfl.write((const char *) (&alfilelen), sizeof(alfilelen));
  cfl.write((const char *) al.file.chars(), alfilelen);
  // al.title
  int altitlelen = al.title.length();
  cfl.write((const char *) (&altitlelen), sizeof(altitlelen));
  cfl.write((const char *) al.title.chars(), altitlelen);
  // relevance
  cfl.write((const char *) (&relevance), sizeof(relevance));
  // unbounded
  cfl.write((const char *) (&unbounded), sizeof(unbounded));
  // bounded
  cfl.write((const char *) (&bounded), sizeof(bounded));
  // urgency
  cfl.write((const char *) (&urgency), sizeof(urgency));
  // priority
  cfl.write((const char *) (&priority), sizeof(priority));
  return true;
}

bool DIL_AL_List::Read_from_Binary_Cache(ifstream & cfl) {
  const unsigned int LLEN = 1024;
  char lbuf[LLEN];
  // _targetdate
  if ((cfl.read((READSOMETYPE) (&_targetdate), sizeof(_targetdate))).gcount() < (streamsize) sizeof(_targetdate)) return false;
  // _tdproperty
  if ((cfl.read((READSOMETYPE) (&_tdproperty), sizeof(_tdproperty))).gcount() < (streamsize) sizeof(_tdproperty)) return false;
#ifdef INCLUDE_EXACT_TARGET_DATES
  if (_tdproperty & DILSUPS_TDPROP_EXACT) {
    _tdexact = true;
    _tdproperty = _tdproperty | DILSUPS_TDPROP_FIXED; // insure properties sanity
  } else _tdexact = false;
#endif
#ifdef TEST_CACHE_READ
  VOUT << "&&& _targetdate = " << _targetdate << ", _tdproperty = " << _tdproperty << '\n'; VOUT.flush();
#endif
#ifdef INCLUDE_EXACT_TARGET_DATES
  // _tdperiod
  if ((cfl.read((READSOMETYPE) (&_tdperiod), sizeof(_tdperiod))).gcount() < (streamsize) sizeof(_tdperiod)) return false;
  // _tdspan
  if ((cfl.read((READSOMETYPE) (&_tdspan), sizeof(_tdspan))).gcount() < (streamsize) sizeof(_tdspan)) return false;
  // [***NOTE] The following is a correction to make old style period&span values compatible with the new method.
  if (_tdperiod==pt_nonperiodic) _tdspan=0; // default span value
  else { // periodic tasks either have unlimited (0) span or a limited span > 1, span==1 implies non-periodic
    if (_tdspan==1) { // anything STORED with span==1 was probably an old unlimited periodic task
      _tdspan=0;
    }
    if (_tdperiod==OLD_pt_span) _tdperiod=pt_daily;
  }
  // _tdevery
  if ((cfl.read((READSOMETYPE) (&_tdevery), sizeof(_tdevery))).gcount() < (streamsize) sizeof(_tdevery)) return false;
#endif
  // al.file
  unsigned int alfilelen;
  if ((cfl.read((READSOMETYPE) (&alfilelen), sizeof(alfilelen))).gcount() < (streamsize) sizeof(alfilelen)) return false;
#ifdef TEST_CACHE_READ
  VOUT << "&&& alfilelen = " << alfilelen << '\n'; VOUT.flush();
#endif
  TEST_LBUF_BOUNDS(alfilelen,"DIL_AL_List::Read_from_Binary_Cache()")
  if (static_cast<streamsize>((cfl.read((READSOMETYPE) lbuf, alfilelen)).gcount()) < static_cast<streamsize>(alfilelen)) return false;
  lbuf[alfilelen]='\0';
  al.file = lbuf;
#ifdef TEST_CACHE_READ
  VOUT << "&&& al.file = " << al.file << '\n'; VOUT.flush();
#endif
  // al.title
  unsigned int altitlelen;
  if ((cfl.read((READSOMETYPE) (&altitlelen), sizeof(altitlelen))).gcount() < (streamsize) sizeof(altitlelen)) return false;
  TEST_LBUF_BOUNDS(altitlelen,"DIL_AL_List::Read_from_Binary_Cache()")
  if (static_cast<streamsize>((cfl.read((READSOMETYPE) lbuf, altitlelen)).gcount()) < static_cast<streamsize>(altitlelen)) return false;
  lbuf[altitlelen]='\0';
  al.title = lbuf;
  // relevance
  if ((cfl.read((READSOMETYPE) (&relevance), sizeof(relevance))).gcount() < (streamsize) sizeof(relevance)) return false;
  // unbounded
  if ((cfl.read((READSOMETYPE) (&unbounded), sizeof(unbounded))).gcount() < (streamsize) sizeof(unbounded)) return false;
  // bounded
  if ((cfl.read((READSOMETYPE) (&bounded), sizeof(bounded))).gcount() < (streamsize) sizeof(bounded)) return false;
  // urgency
  if ((cfl.read((READSOMETYPE) (&urgency), sizeof(urgency))).gcount() < (streamsize) sizeof(urgency)) return false;
  // priority
  if ((cfl.read((READSOMETYPE) (&priority), sizeof(priority))).gcount() < (streamsize) sizeof(priority)) return false;
  return true;
}

bool DIL_AL_List::Binary_Cache_Diagnostic(DIL_AL_List * cached, int & testedbytes) {
  // test if the quick load cache process works reliably
  // this is the original AL_List, cached is the one retrieved from the binary cache
  //   start of this object test
  int localtestedbytes = sizeof(PLLHandle<DIL_AL_List>) + sizeof(superior);
  if ((*entry)!=(*(cached->entry))) INFO << "*** DIL_AL_List entry mismatch\n"; // testing if DIL_ID==DIL_ID
  localtestedbytes += sizeof(entry);
  if (al.file!=cached->al.file) INFO << "*** DIL_AL_List al.file mismatch\n";
  if (al.title!=cached->al.title) INFO << "*** DIL_AL_List al.title mismatch\n";
  localtestedbytes += sizeof(al);
  if (_targetdate!=cached->_targetdate) INFO << "*** DIL_AL_List targetdate mismatch\n";
  localtestedbytes += sizeof(_targetdate);
  if (_tdproperty!=cached->_tdproperty) INFO << "*** DIL_AL_List _tdproperty mismatch\n";
#ifdef INCLUDE_EXACT_TARGET_DATES
  if (_tdexact!=cached->_tdexact) INFO << "*** DIL_AL_List _tdexact mismatch (note: derived rather than stored)\n";
  if (_tdperiod!=cached->_tdperiod) INFO << "*** DIL_AL_List _tdperiod mismatch\n";
  if (_tdspan!=cached->_tdspan) INFO << "*** DIL_AL_List _tdspan mismatch\n";
  if (_tdevery!=cached->_tdevery) INFO << "*** DIL_AL_List _tdevery mismatch\n";
#endif
  localtestedbytes += sizeof(_tdproperty);
  if (relevance!=cached->relevance) INFO << "*** DIL_AL_List relevance mismatch\n";
  localtestedbytes += sizeof(relevance);
  if (unbounded!=cached->unbounded) INFO << "*** DIL_AL_List unbounded mismatch\n";
  localtestedbytes += sizeof(unbounded);
  if (bounded!=cached->bounded) INFO << "*** DIL_AL_List bounded mismatch\n";
  localtestedbytes += sizeof(bounded);
  if (urgency!=cached->urgency) INFO << "*** DIL_AL_List urgency mismatch\n";
  localtestedbytes += sizeof(urgency);
  if (priority!=cached->priority) INFO << "*** DIL_AL_List priority mismatch\n";
  localtestedbytes += sizeof(priority);
  //   end of this object test
  if (localtestedbytes!=sizeof(DIL_AL_List)) INFO << "*** testedbytes!=sizeof(DIL_AL_List)\n";
  testedbytes += localtestedbytes;
  return true;
}

bool DIL_entry_parameters::Write_to_Binary_Cache(ofstream & cfl) {
  // origloc
  cfl.write((const char *) (&origloc), sizeof(origloc));
  // topics
  int numtopics = topics.length();
  cfl.write((const char *) (&numtopics), sizeof(numtopics));
  if (numtopics>0) PLL_LOOP_FORWARD(DIL_Topical_List,topics.head(),1) if (!e->Write_to_Binary_Cache(cfl)) return false;
  // projects
  int numprojects = projects.length();
  cfl.write((const char *) (&numprojects), sizeof(numprojects));
  if (numprojects>0) PLL_LOOP_FORWARD(DIL_AL_List,projects.head(),1) if (!e->Write_to_Binary_Cache(cfl)) return false;
  return true;
}

bool DIL_entry_parameters::Read_from_Binary_Cache(ifstream & cfl) {
  // origloc
  if ((cfl.read((READSOMETYPE) (&origloc), sizeof(origloc))).gcount() < (streamsize) sizeof(origloc)) return false;
  // topics
  int numtopics;
  if ((cfl.read((READSOMETYPE) (&numtopics), sizeof(numtopics))).gcount() < (streamsize) sizeof(numtopics)) return false;
#ifdef TEST_CACHE_READ
  VOUT << "&&& numtopics = " << numtopics << '\n'; VOUT.flush();
#endif
  if (numtopics>0) for (int i = 0; i<numtopics; i++) {
    DIL_Topical_List * dtl = new DIL_Topical_List(*entry);
    if (!dtl->Read_from_Binary_Cache(cfl)) { delete dtl; return false; }
    topics.link_before(dtl);
  }
  // projects
  int numprojects;
  if ((cfl.read((READSOMETYPE) (&numprojects), sizeof(numprojects))).gcount() < (streamsize) sizeof(numprojects)) return false;
#ifdef TEST_CACHE_READ
  VOUT << "&&& numprojects = " << numtopics << '\n'; VOUT.flush();
#endif
  if (numprojects>0) for (int i = 0; i<numprojects; i++) {
    DIL_AL_List * dal = new DIL_AL_List(*entry);
    if (!dal->Read_from_Binary_Cache(cfl)) { delete dal; return false; }
    projects.link_before(dal);
  }
  return true;
}

bool DIL_entry_parameters::Binary_Cache_Diagnostic(DIL_entry_parameters * cached, int & testedbytes) {
  // test if the quick load cache process works reliably
  // this is the original entry_parameters, cached is the one retrieved from the binary cache
  //   start of this object test
  int localtestedbytes = 0;
  if ((*entry)!=(*(cached->entry))) INFO << "*** DIL_entry_parameters entry mismatch\n"; // testing if DIL_ID==DIL_ID
  localtestedbytes += sizeof(entry);
  if (origloc!=cached->origloc) INFO << "*** DIL_entry_parameters origloc mismatch\n";
  localtestedbytes += sizeof(origloc);
  if (topics.length()!=cached->topics.length()) INFO << "*** DIL_entry_parameters topics length mismatch, " << (long) topics.length() << " vs. " << (long) cached->topics.length() << '\n';;
  localtestedbytes += sizeof(topics);
  if (projects.length()!=cached->projects.length()) INFO << "*** DIL_entry_parameters projects length mismatch, " << (long) projects.length() << " vs. " << (long) cached->projects.length() << '\n';;
  localtestedbytes += sizeof(projects);
  //   end of this object test
  if (localtestedbytes!=sizeof(DIL_entry_parameters)) INFO << "*** testedbytes!=sizeof(DIL_entry_parameters)\n";
  for (DIL_Topical_List * e = topics.head(), * c = cached->topics.head(); ((e) && (c)); e=e->Next(), c=c->Next()) if (!e->Binary_Cache_Diagnostic(c,testedbytes)) return false;
  for (DIL_AL_List * e = projects.head(), * c = cached->projects.head(); ((e) && (c)); e=e->Next(), c=c->Next()) if (!e->Binary_Cache_Diagnostic(c,testedbytes)) return false;
  testedbytes += localtestedbytes;
  return true;
}

DIL_entry::DIL_entry(Text_File_Buffers & t): DIL_ID(), tfb(&t), content(NULL), parameters(NULL) {}

DIL_entry::DIL_entry(Text_File_Buffers & t, String & idstr): DIL_ID(idstr), tfb(&t), content(NULL), parameters(NULL) {}

DIL_entry::DIL_entry(Text_File_Buffers & t, const SubString & idstr): DIL_ID(idstr), tfb(&t), content(NULL), parameters(NULL) {}

DIL_entry * DIL_entry::elbyid(const DIL_ID & id) const {
// return pointer to list element with DIL ID id
//	if ((*const_cast<DIL_entry *>(this))==id) return const_cast<DIL_entry *>(this);
  if ((*this)==id) return const_cast<DIL_entry *>(this);
  for (DIL_entry * de=Next(); de; de=de->Next()) if (*de==id) return de;
  for (DIL_entry * de=Prev(); de; de=de->Prev()) if (*de==id) return de;
  return NULL;
}

#ifdef LOOP_PROTECT
time_t DIL_entry::Propagated_Target_Date(bool warnflag, bool top) const {
#else
time_t DIL_entry::Propagated_Target_Date(bool warnflag) const {
#endif
// return nearest target date, as given for this DIL entry in
// relation to one of its project superiors, or as given for its
// project superiors, or MAXTIME_T
// *** could implement a local buffer for superior target date
//     look-ups, but such a buffer must be refreshed if local
//     or relevant superior target dates are changed
#ifdef LOOP_PROTECT
  if ((!top) && (!semaphore)) {
    if (warnflag) {
      if (!looplist) looplist = new Detailed_Items_List();
      DIL_entry * lhde = looplist->list.head();
      if ((!lhde) || (!lhde->elbyid(*this))) { // add to looplist
	//			if ((!lhde) || (!lhde->elbyid(*(static_cast<DIL_ID *>(this))))) { // add to looplist
	String ldeid(chars());
	DIL_entry * lde = new DIL_entry(*looplist,ldeid);
	looplist->list.link_before(lde);
      }
    }
    return (MAXTIME_T);
  }
#endif
  DIL_entry * sup;
  time_t t = (MAXTIME_T); // default where no target date is known
  // priority target dates are target dates specific to this DIL entry
  if (parameters) PLL_LOOP_FORWARD(DIL_AL_List,parameters->projects.head(),1) {
      time_t tdate = e->targetdate();
      if (tdate<0) { // missing target dates are obtained from the corresponding project superior
	// *** this can be made more efficient once the pointers to
	//     superiors are implemented
	if (str()!=e->al.title) { // not self-referential
	  DIL_ID did(e->al.title);
	  if (did.valid()) {
	    sup = elbyid(did);
	    if (sup) {
#ifdef LOOP_PROTECT
	      if (top) head()->Set_Semaphores(TD_PROTECT);
	      semaphore = 0; // set this one parsed
	      tdate = sup->Target_Date(false);
#else
	      tdate = sup->Target_Date();
#endif
	    } else {
	      if (warnflag) WARN << "DIL_entry::Propagated_Target_Date: Superior DIL#" << did.chars() << " not found at DIL#" << chars() << '\n';
	      tdate = (MAXTIME_T);
	    }
	  } else {
	    if (warnflag) WARN << "DIL_entry::Propagated_Target_Date: Invalid superior reference detected at DIL#" << chars() << '\n';
	    tdate = (MAXTIME_T);
	  }
	} else {
	  if (warnflag) WARN << "DIL_entry::Propagated_Target_Date: Self-reference without specific target date detected at DIL#" << chars() << '\n';
	  tdate = (MAXTIME_T);
	}
      }
      if (tdate<t) t = tdate;
    }
  return t;
}

DIL_entry * DIL_entry::Propagated_Target_Date_Origin(time_t & t) const {
// return the origin DIL entry that provides the nearest target date,
// as given for this DIL entry in
// relation to one of its project superiors, or as given for its
// project superiors, or NULL
  DIL_entry * sup;
  t = (MAXTIME_T);
  DIL_entry * origin = NULL; // default where no target date is known
  // priority target dates are target dates specific to this DIL entry
  if (parameters) PLL_LOOP_FORWARD(DIL_AL_List,parameters->projects.head(),1) {
      DIL_entry * candidate = const_cast<DIL_entry *>(this);
      time_t tdate = e->targetdate();
      if (tdate<0) { // missing target dates are obtained from the corresponding project superior
	if (str()!=e->al.title) { // not self-referential
	  DIL_ID did(e->al.title);
	  if (did.valid()) {
	    sup = elbyid(did);
	    if (sup) {
	      candidate = sup->Propagated_Target_Date_Origin(tdate);
	    } else {
	      tdate = (MAXTIME_T);
	      candidate = NULL;
	    }
	  } else {
	    tdate = (MAXTIME_T);
	    candidate = NULL;
	  }
	} else {
	  tdate = (MAXTIME_T);
	  candidate = NULL;
	}
      }
      if (tdate<t) {
	t = tdate;
	origin = candidate;
      }
    }
  return origin;
}

time_t DIL_entry::Target_Date_Info(bool & isfromlocal, int & haslocal, int & numpropagating) const {
  // This function returns the propagated target date, as well as information
  // about the target dates that influence this DIL entry.
  // isfromlocal: is true when the returned target date was specified locally,
  //              i.e. not propagated from a superior.
  // haslocal: is 1 if this DIL entry has any locally specified target dates
  //           that may determine the target date when they are not later than
  //           any propagated target dates,
  //           is 2 if some of the locally specified target dates are
  //           fixed (or of a related nature, e.g. exact).
  //           is -2 if all target date references are undefined, and at least
  //           one of those is fixed, i.e. a fixed superior target date.
  // numpropagating: returns the number of superiors that can propagate target
  //                 dates to this DIL entry, i.e. for which the target date
  //                 is not locally specified.
  time_t t = Propagated_Target_Date(false);
  isfromlocal = false;
  haslocal = 0;
  numpropagating = 0;
  time_t tdate = -1;
  for (int i=0; tdate>-2; i++) {
    tdate = Local_Target_Date(i);
    if (t==tdate) isfromlocal = true;
    if ((tdate>-1) && (haslocal<2)) {
      if (Projects(i)->tdproperty()>0) haslocal = 2;
      else haslocal = 1;
    }
    if (tdate==-1) {
      numpropagating++;
      if ((haslocal==0) && (Projects(i)->tdproperty()>0)) haslocal = -2;
    }
  }
  return t;
 }

bool DIL_entry::Get_Entry_TL_References(String & referencestr) {
  // referencestr contains (among other things) data of the form
  // <'('>["<A HREF=tl-reference>"]<tail>["</A>"]<,>["<A HREF=tl-reference>"]<head>["</A>"]<')'>
  String tailstr(referencestr.before(','));
  String headstr(referencestr.after(','));
  if (!tailstr.empty()) {
    String hrefurl, hreftext;
    if (HTML_get_href(tailstr,0,hrefurl,hreftext)>=0) {
      // *** could add test here to check that hreftext=="tail"
      if (!content) content = new DIL_entry_content(*this);
      content->set_TL_tail_reference(hrefurl);
    }
  }
  if (!headstr.empty()) {
    String hrefurl, hreftext;
    if (HTML_get_href(headstr,0,hrefurl,hreftext)>=0) {
      // *** could add test here to check that hreftext=="head"
      if (!content) content = new DIL_entry_content(*this);
      content->set_TL_head_reference(hrefurl);
    }
  }
  return true;
}

bool DIL_entry::Get_Entry_Parameters(String & parstr) {
// parstr contains entry parameters to be parsed and stored
  int parstart = -1; double pars[4];
  for (int parnum = 0; parnum<4; parnum++) {
    if ((parstart=parstr.index(':',parstart+1))<0) return false;
    pars[parnum] = atof(String(parstr.after(parstart)));
  }
  if (!content) content = new DIL_entry_content(*this);
  content->started  = double2time(pars[0]); // *** can remove this (see dil2al.hh)
  content->required = (time_t) (ceil(pars[1]*60.0)*60.0); // convert hours to seconds
  content->completion = pars[2];
  content->valuation = pars[3];
  return true;
}

bool DIL_entry::Is_in_Topic(String topicfilestr) {
  // searches associated topics and reports if the topic
  // indicated by topicfilestr is one of them
  if (!parameters) return false;
  PLL_LOOP_FORWARD(DIL_Topical_List,parameters->topics.head(),1) if (e->dil.file.contains(topicfilestr)) return true;
  return false;
}

bool DIL_entry::Is_Direct_Dependency_Of(const DIL_entry * superior) {
// checks if this DIL entry points to superior through one of its
// DIL_AL_List references
// A self-reference is not reported as a direct dependency.
  if (superior==this) return false;
  if (Projects(0)) PLL_LOOP_FORWARD(DIL_AL_List,Projects(0),1) if (e->Superiorbyid()==superior) return true;
  return false;
}

int IDOentered = 0;
int IDOpassed = 0;

bool DIL_entry::Is_Dependency_Of(const DIL_entry * superior, bool recursing) {
// checks if this DIL entry is a dependency of superior
// superior==NULL is interpreted as "all"
// dependencies do not need to be direct
  IDOentered++;
  if (!superior) return true;
  // *** Or, Implement DIL hierarchy, use the Set_Semaphores
  //     recursive approach as in Target_Date()
  if (!recursing) head()->Set_Semaphores(0); // initialize
  if (Get_Semaphore()>0) return false; // already visited this one
  IDOpassed++;
  Set_Semaphore(1);
  if (Is_Direct_Dependency_Of(superior)) return true;
  if (Projects(0)) PLL_LOOP_FORWARD(DIL_AL_List,Projects(0),1) if (e->Superiorbyid()) if (e->Superiorbyid()->Is_Dependency_Of(superior,true)) return true;
  return false;
}

DIL_Superiors * DIL_entry::Superior_Parameters(const DIL_entry * superior) {
  // If superior is indeed a direct superior of this DIL entry then
  // this function returns the corresponding DIL_Superiors reference.
  if (Projects(0)) PLL_LOOP_FORWARD(DIL_Superiors,Projects(0),1) if (e->Superiorbyid()==superior) return e;
  return NULL;
}

bool DIL_entry::Has_Superiors(bool taggedonly) {
  // superior references to the DIL entry itself do not count
  // if taggedonly==true then only superiors with a set semaphore count
  if (taggedonly) {
    if (Projects(0)) PLL_LOOP_FORWARD(DIL_AL_List,Projects(0),1) {
      DIL_entry * dilsup = e->Superiorbyid();
      if ((!(dilsup==this)) && (dilsup->Get_Semaphore()>0)) return true;
    }
  } else {
    if (Projects(0)) PLL_LOOP_FORWARD(DIL_AL_List,Projects(0),1) if (!(e->Superiorbyid()==this)) return true;
  }
  return false;
}

int DIL_entry::Is_Plan_Entry() {
  // attempts to determine if the DIL entry content contains PLAN entry data
  if (!content) {
    WARN << "DIL_entry::Is_Plan_Entry(): No content available\n --- obtaining content automatically is not yet implemented.\nContinuing as is.\n";
    return PLAN_ENTRY_TYPE_UNASSIGNED;
  }
  return content->Is_Plan_Entry();
}

bool DIL_entry::Write_to_Binary_Cache(ofstream & cfl) {
  // DIL_ID
  if (!Write_DIL_ID_to_Binary_Cache(cfl)) return false;
  bool hasparameters = (parameters!=NULL);
  cfl.write((const char *) (&hasparameters), sizeof(hasparameters));
  if (!hasparameters) return true;
  // parameters
  return parameters->Write_to_Binary_Cache(cfl);
}

bool DIL_entry::Write_Topical_to_Binary_Cache(ofstream & cfl, bool gottext) {
  // DIL_ID
  if (!Write_DIL_ID_to_Binary_Cache(cfl)) return false;
  bool hascontent = (content!=NULL);
  cfl.write((const char *) (&hascontent), sizeof(hascontent));
  if (!hascontent) return true;
  // content
  return content->Write_Topical_to_Binary_Cache(cfl,gottext);
}

bool DIL_entry::Read_from_Binary_Cache(ifstream & cfl) {
  // Reads a DIL entry from a binary cache file and sets the DIL ID
  // DIL_ID
  if (!Read_DIL_ID_from_Binary_Cache(cfl)) return false;
  bool hasparameters;
  if ((cfl.read((READSOMETYPE) (&hasparameters), sizeof(hasparameters))).gcount() < (streamsize) sizeof(hasparameters)) return false;
  if (!hasparameters) return true;
  // parameters
  delete parameters; // clear any previous allocation and parameter values
  parameters = new DIL_entry_parameters(*this);
#ifdef TEST_CACHE_READ
  VOUT << "&&& parameters = " << (long) parameters << '\n'; VOUT.flush();
#endif
  return parameters->Read_from_Binary_Cache(cfl);
}

bool DIL_entry::Read_Topical_from_Binary_Cache(ifstream & cfl, bool gottext) {
  // Reads a DIL entry from a binary cache file and sets the DIL ID
  // DIL_ID
  // done in outer loop: if (!Read_DIL_ID_from_Binary_Cache(cfl)) return false;
  bool hascontent;
  if ((cfl.read((READSOMETYPE) (&hascontent), sizeof(hascontent))).gcount() < (streamsize) sizeof(hascontent)) return false;
#ifdef TEST_CACHE_READ
  VOUT << "&&& hascontent = " << hascontent << '\n'; VOUT.flush();
#endif
  if (!hascontent) return true;
  // content
#ifdef TEST_CACHE_READ
  VOUT << "&&& content (before clearing) = " << (long) content << '\n'; VOUT.flush();
#endif
  delete content; // clear any previous allocation and content
  content = new DIL_entry_content(*this);
#ifdef TEST_CACHE_READ
  VOUT << "&&& content (after clearing) = " << (long) content << '\n'; VOUT.flush();
#endif
  return content->Read_Topical_from_Binary_Cache(cfl,gottext);
}

bool DIL_entry::Binary_Cache_Diagnostic(DIL_entry * cached, int & testedbytes) {
  // test if the quick load cache process works reliably
  // this is the original entry, cached is the one retrieved from the binary cache
  //   start of this object test
  int localtestedbytes = 0;
  if ((*this)!=(*cached)) VOUT << "*** DIL_ID mismatch\n"; // testing if DIL_ID==DIL_ID
  localtestedbytes += sizeof(DIL_ID) + sizeof(PLLHandle<DIL_entry>) + sizeof(semaphore) + sizeof(tfb) + sizeof(content) + sizeof(parameters);
  //   end of this object test
  if (localtestedbytes!=sizeof(DIL_entry)) INFO << "*** testedbytes!=sizeof(DIL_entry)\n";
  if (content) {
    if (cached->content) {
      if (!content->Binary_Cache_Diagnostic(cached->content,testedbytes)) return false;
    } else INFO << "*** entry has content, cached does not\n";
  } else if (cached->content) INFO << "*** entry has no content, cached does\n";
  if (parameters) {
    if (cached->parameters) {
      if (!parameters->Binary_Cache_Diagnostic(cached->parameters,testedbytes)) return false;
    } else INFO << "*** entry has parameters, cached does not\n";
  } else if (cached->parameters) INFO << "*** entry has no parameters, cached does\n";
  testedbytes += localtestedbytes;
  return true;
}
 
bool Detailed_Items_List::Entry_Has_Dependencies(const DIL_entry * de, bool taggedonly) {
  // if taggedonly==true then only dependencies with a set semaphore count
  if (!de) return false;
  if (taggedonly) {
    PLL_LOOP_FORWARD(DIL_entry,list.head(),1) if (e->Get_Semaphore()>0) if (e->Is_Direct_Dependency_Of(de)) return true;
  } else {
    PLL_LOOP_FORWARD(DIL_entry,list.head(),1) if (e->Is_Direct_Dependency_Of(de)) return true;
  }
  return false;
}

int Detailed_Items_List::Get_Project_Data(String & dildata, int projloc, String & hrefurl, String & hreftext, DIL_entry_parameters & dilentrypars) {
  // dildata contains the DIL entry project data
  // projloc indicates how much of dildata has already been parsed
  // hrefurl and hreftext have already been obtained
  // dilentrypars receives the DIL entry project related parameters
  // *** Modify this if the data format changes
  
  int nextproj, dataloc, dataend;
  // insure prior to next project URL
  if ((nextproj = dildata.index('<',projloc))<0) nextproj=dildata.length();
  // start of project data
  if ((dataloc = find_delimiter(dildata,projloc,nextproj,'('))<0) return -1;
  dataloc++;
  // find relevance data
  if ((dataend = find_delimiter(dildata,dataloc,nextproj,','))<0) return -1;
  double relevance = atof(String(dildata.at(dataloc,dataend-dataloc)));
  dataloc=dataend+1;
  // find unbounded importance data
  if ((dataend = find_delimiter(dildata,dataloc,nextproj,','))<0) return -1;
  double unbounded = atof(String(dildata.at(dataloc,dataend-dataloc)));
  dataloc=dataend+1;
  // find bounded importance data
  if ((dataend = find_delimiter(dildata,dataloc,nextproj,','))<0) return -1;
  double bounded = atof(String(dildata.at(dataloc,dataend-dataloc)));
  dataloc=dataend+1;
  // find target date and target date properties
  if ((dataend = find_delimiter(dildata,dataloc,nextproj,','))<0) return -1;
  String ymdHM = dildata.at(dataloc,dataend-dataloc);
  int tdprop = 0;
#ifdef INCLUDE_EXACT_TARGET_DATES
  bool tdex = false;
  periodictask_t periodicity = pt_nonperiodic;
  int span = 0;
  int every = 1;
#endif
  if (ymdHM.length()>0) {
    if (ymdHM[0]=='F') {
      tdprop = DILSUPS_TDPROP_FIXED;
      ymdHM.del(0,1);
    }
#ifdef INCLUDE_EXACT_TARGET_DATES
    else if (ymdHM[0]=='E') {
      tdprop = DILSUPS_TDPROP_FIXED | DILSUPS_TDPROP_EXACT;
      tdex = true;
      ymdHM.del(0,1);
    }
    // IF FIXED or EXACT detect possible PERIODIC
    if ((tdprop & DILSUPS_TDPROP_FIXED)!=0) {
      if (ymdHM.length()>0) { // periodicity
	char periodicchar = ymdHM[0];
	for (unsigned int i = 0; i<pt_nonperiodic; i++) if (periodicchar==periodictaskchar[i]) {
	    periodicity = (periodictask_t) i;
	    if (periodicity==OLD_pt_span) periodicity=pt_daily; // update old span method to new span method, leave span indicator in string
	    else ymdHM.del(0,1);
	    tdprop = tdprop | DILSUPS_TDPROP_PERIODIC;
	    break;
	  }
	if ((periodicity==pt_nonperiodic) && ((periodicchar<'0') || (periodicchar>'9')) && (periodicchar!='?')) WARN << "Detailed_Items_List::Get_Project_Data(): Suspicious target date '" << ymdHM << "' for DIL#" << dilentrypars.DE()->chars() << " read.\n";
      }
      // IF PERIODIC detect possible EVERY and SPAN
      if ((periodicity!=pt_nonperiodic) && (ymdHM.length()>0)) {
	// look for an "every" indicator
	if (ymdHM[0]=='e') { // occurs every so many multiples of the periodicity
	  ymdHM.del(0,1);
	  int everyend = ymdHM.index('_');
	  if (everyend>=0) {
	    every = atoi((const char *) String(ymdHM.before(everyend)));
	    if (every<1) {
	      WARN << "Detailed_Items_List::Get_Project_Data(): Invalid tdevery in target date '" << ymdHM << "' for DIL#" << dilentrypars.DE()->chars() << ", setting to 1\n";
	      every = 1;
	    }
	    ymdHM.del(0,everyend+1);
	  } else {
	    WARN << "Detailed_Items_List::Get_Project_Data(): Missing tdevery in target date '" << ymdHM << "' for DIL#" << dilentrypars.DE()->chars() << ", setting to 1\n";
	    every = 1;
	  }
	}
	// look for a "span" indicator
	if (ymdHM[0]=='s') { // has limited span
	  ymdHM.del(0,1);
	  int spanend = ymdHM.index('_');
	  if (spanend>=0) {
	    span = atoi((const char *) String(ymdHM.before(spanend)));
	    if (span<2) {
	      WARN << "Detailed_Items_List::Get_Project_Data(): Invalid tdspan in target date '" << ymdHM << "' for DIL#" << dilentrypars.DE()->chars() << ", setting to Unlimited (0)\n";
	      span = 0;
	    }
	    ymdHM.del(0,spanend+1);
	  } else {
	    WARN << "Detailed_Items_List::Get_Project_Data(): Missing tdspan in target date '" << ymdHM << "' for DIL#" << dilentrypars.DE()->chars() << ", setting to Unlimited (0)\n";
	    span = 0;
	  }
	}
      }
    }
#endif
  }
  // Note that from this point forward, a date string that does not conform to one of the
  // following conventions is interpreted as "unspecified": negative numbers are special indicators,
  // numbers with 8 or 12 digits are considered regular target dates.
  if ((ymdHM.length()>=8) && (ymdHM.length()<12)) ymdHM += "0000"; // year, month, day, hour, minute
  _DEBUG_TIME_STAMP_TIME_CALLER(__PRETTY_FUNCTION__);
  time_t targetdate = time_stamp_time(ymdHM); // local time stamp stored in calendar time, probably correct with noexit=false (20190127)
  dataloc=dataend+1;
  // find urgency data
  if ((dataend = find_delimiter(dildata,dataloc,nextproj,','))<0) return -1;
  double urgency = atof(String(dildata.at(dataloc,dataend-dataloc)));
  dataloc=dataend+1;
  // find priority data
  if ((dataend = find_delimiter(dildata,dataloc,nextproj,')'))<0) return -1;
  double priority = atof(String(dildata.at(dataloc,dataend-dataloc)));
  // store project data
  dilentrypars.project_append(hrefurl,hreftext,relevance,unbounded,bounded,targetdate,tdprop,
#ifdef INCLUDE_EXACT_TARGET_DATES
			      tdex,periodicity,span,every,
#endif
			      urgency,priority);
  return dataend+1;
}	

String topicrelurl, topictitle, topicrelevance, suprelurl, suptext;

int Get_DIL_ID_File_Topics_Parameters(String & tfstr,DIL_entry * de,int & idloc,int idloc_sup, int entrynum) {
  int idloc_next = tfstr.index('#',idloc);
  if ((idloc_next<0) || (idloc_next>=idloc_sup)) {
    delete de;
    ERROR << "Detailed_Items_List::Get_All_DIL_ID_File_Parameters(): Missing topic file reference in DIL entry " << entrynum << ".\n";
    return -1;
  }
  topicrelurl = tfstr.at(idloc,idloc_next-idloc);
  idloc = idloc_next + 19; // right past '>'
  idloc_next = tfstr.index('<',idloc);
  if ((idloc_next<0) || (idloc_next>=idloc_sup)) {
    delete de;
    ERROR << "Detailed_Items_List::Get_All_DIL_ID_File_Parameters(): Missing topic title in DIL entry " << entrynum << ".\n";
    return -1;
  }
  topictitle = tfstr.at(idloc,idloc_next-idloc);
  idloc = idloc_next + 6; // right past '('
  idloc_next = tfstr.index(')',idloc);
  if ((idloc_next<0) || (idloc_next>=idloc_sup)) {
    delete de;
    ERROR << "Detailed_Items_List::Get_All_DIL_ID_File_Parameters(): Missing topic relevance in DIL entry " << entrynum << ".\n";
    return -1;
  }
  topicrelevance = tfstr.at(idloc,idloc_next-idloc);
  idloc = idloc_next + 1; // past ')'
  de->parameters->topic_append(absurl(idfile,topicrelurl),topictitle,atof(topicrelevance));
  return 0;
}

double str2decimal(String & s, int sstartloc, int spastendloc) {
// Convert simple positive or negative decimal strings into double.
// Returns 0.0 for non-decimal content
  int i=sstartloc;
  bool isnegative = (s[i]=='-');
  if (isnegative) i++;
  char c;
  double decimaltotal = 0.0, dmultiplier = 10.0;
  for (; ((i<spastendloc) && ((c=s[i])!='.')); i++) {
    if ((c>='0') && (c<='9')) {
      decimaltotal = (dmultiplier*decimaltotal) + (double) ((int) (c-'0'));
    } else return 0.0; // non-number code
  }
  i++;
  dmultiplier = 0.1;
  for (; (i<spastendloc); i++) {
    c=s[i];
    if ((c>='0') && (c<='9')) {
      decimaltotal += (dmultiplier * (double) ((int) (c-'0')));
      dmultiplier *= 0.1;
    } else return 0.0; // non-number code
  }
  if (isnegative) return -decimaltotal;
  return decimaltotal;
}

//#define DEBUG_Get_DIL_ID_File_Superior_TargetDate
#ifdef  DEBUG_Get_DIL_ID_File_Superior_TargetDate
String didteststr("20171129191701.1");
DIL_ID didtest(didteststr);
bool didtestinspect = false;
#endif
 
int Get_DIL_ID_File_Superior_TargetDate(String & tfstr, int idloc, int idloc_next, time_t & targetdate, int & tdprop
#ifdef INCLUDE_EXACT_TARGET_DATES
					, bool & tdex, periodictask_t & periodicity, int & span, int & every
#endif
					) {
  // Negative return means error.
  // Even when targetdate is "unspecified" (-1), tdprop can contain valid data (e.g. "F").
  // [***Note]: If you want to line up more flags, e.g. "FE", then read consecutively, without the "else" before the next "if", and possibly use tdprop = tdprop | FLAG rather than tdprop = FLAG.
  
  // Targetdate property defaults
  //VOUT << "idloc_next = " << idloc_next << '\n';
  tdprop = 0;
#ifdef INCLUDE_EXACT_TARGET_DATES
  periodicity = pt_nonperiodic;
  tdex = false;
  span = 0;
  every = 1;
#endif
  if (idloc_next<=idloc) {
    ERROR << "Get_DIL_ID_File_Superior_TargetDate(): Missing target date (not even a '?' code).\n";
    targetdate = -1;
    return -1;
  }

  char c = tfstr[idloc];
  // Property: Fixed?
  if (c=='F') {
    tdprop = DILSUPS_TDPROP_FIXED;
    idloc++;
  }
#ifdef INCLUDE_EXACT_TARGET_DATES
  else if (c=='E') { // Property: Exact?
    tdprop = DILSUPS_TDPROP_FIXED | DILSUPS_TDPROP_EXACT;
    tdex = true;
    idloc++;
  }
  // IF FIXED or EXACT detect possible PERIODIC
  if ((tdprop & DILSUPS_TDPROP_FIXED)!=0) {
    // Property: Periodicity?
    if (idloc_next>idloc) {
      c=tfstr[idloc];
      for (unsigned int i = 0; i<pt_nonperiodic; i++) if (c==periodictaskchar[i]) {
	  periodicity = (periodictask_t) i;
	  if (periodicity==OLD_pt_span) periodicity=pt_daily; // update old span method to new span method, leave span indicator in string
	  else idloc++;
	  tdprop = tdprop | DILSUPS_TDPROP_PERIODIC;
	  break;
	}
      if ((periodicity==pt_nonperiodic) && ((c<'0') || (c>'9')) && (c!='?')) {
	ERROR << "Get_DIL_ID_File_Superior_TargetDate(): Suspicious target date '" << String(tfstr.at(idloc,idloc_next-idloc)) << ".\n";
	targetdate = -1;
	return -1;
      }
    }
    if ((periodicity!=pt_nonperiodic) && (idloc_next>idloc)) {
      // IF PERIODIC detect possible EVERY
      // Property: Every?
      if (tfstr[idloc]=='e') {
	idloc++;
	int everyend = tfstr.index('_',idloc);
	if (everyend<0) {
	  ERROR << "Get_DIL_ID_File_Superior_TargetDate(): Missing tdevery end mark '_' in target date '" << String(tfstr.at(idloc,idloc_next-idloc)) << ".\n";
	  targetdate = -1;
	  return -1;
	}
	every = atoi((const char *) String(tfstr.at(idloc,everyend-idloc)));
	if (every<1) {
	  ERROR << "Get_DIL_ID_File_Superior_TargetDate(): Negative tdevery in target date '" << String(tfstr.at(idloc,idloc_next-idloc)) << ".\n";
	  targetdate = -1;
	  return -1;
	}
	idloc = everyend + 1; // past '_'
      }
      // IF PERIODIC detect possible SPAN
      // Property: Span?
      if (tfstr[idloc]=='s') {
	idloc++;
	int spanend = tfstr.index('_',idloc);
	if (spanend<0) {
	  ERROR << "Get_DIL_ID_File_Superior_TargetDate(): Missing tdspan end mark '_' in target date '" << String(tfstr.at(idloc,idloc_next-idloc)) << ".\n";
	  targetdate = -1;
	  return -1;
	}
	span = atoi((const char *) String(tfstr.at(idloc,spanend-idloc)));
	if (span<2) {
	   ERROR << "Get_DIL_ID_File_Superior_TargetDate(): Invalid tdspan (<2) in target date '" << String(tfstr.at(idloc,idloc_next-idloc)) << ".\n";
	   targetdate = -1;
	   return -1;
	 }
	idloc = spanend + 1; // past '_'
      }
    }
  }
#endif // INCLUDE_EXACT_TARGET_DATES

  // Note that from this point forward, a date string that does not conform to one of the
  // following conventions is interpreted as "unspecified": negative numbers are special indicators,
  // numbers with 8 or 12 digits are considered regular target dates.
  _DEBUG_TIME_STAMP_TIME_CALLER(__PRETTY_FUNCTION__);
#ifdef  DEBUG_Get_DIL_ID_File_Superior_TargetDate
  if (didtestinspect) {
    INFO << "idloc_next = " << idloc_next << '\n';
    INFO << "idloc = " << idloc << '\n';
    INFO << "tfstr.at(idloc,idloc_next-idloc) = " << String(tfstr.at(idloc,idloc_next-idloc)) << '\n';
  }
#endif
  int tdstrlen = idloc_next - idloc;
  if (tdstrlen==8) targetdate = time_stamp_time(tfstr.at(idloc,tdstrlen)+"0000"); // ymdHM, probably correct with noexit=false (20190127)
  else if (tdstrlen==12) targetdate =  time_stamp_time(tfstr.at(idloc,tdstrlen),true); // Needs noexit=true to detect '?' (20190130).
  else if ((tdstrlen==1) && (tfstr[idloc]=='?')) targetdate = -1; // much stricted now (20190210), let's keep it clean
  else {
    ERROR << "Get_DIL_ID_File_Superior_TargetDate(): Target date string (" << String(tfstr.at(idloc,idloc_next-idloc)) << ") does not have ? or 8 or 12 characters.\n";
    targetdate = -1;
    return -1;
  }
#ifdef  DEBUG_Get_DIL_ID_File_Superior_TargetDate
  if (didtestinspect) {
    INFO << "targetdate = " << (long) targetdate << " (" << time_stamp("%Y%m%d%H%M",targetdate) << ")\n";
  }
#endif  
  //if (((idloc_next-idloc)>=8) && ((idloc_next-idloc)<12)) targetdate = time_stamp_time(tfstr.at(idloc,idloc_next-idloc)+"0000"); // ymdHM, probably correct with noexit=false (20190127)
  //else targetdate =  time_stamp_time(tfstr.at(idloc,idloc_next-idloc),true); // Needs noexit=true to detect '?' (20190130).
  return 0;
}

int Get_DIL_ID_File_Superior_Parameters(String & tfstr,DIL_entry * de,int & idloc,int idloc_nextrow, int entrynum) {
  // Here, improve upon the perfomance of Get_Project_Data().
  idloc += 9; // past quotation mark
  // Get superior URL
  int idloc_next = tfstr.index('"',idloc);
  if ((idloc_next<0) || (idloc_next>=idloc_nextrow)) {
    delete de;
    ERROR << "Get_DIL_ID_File_Superior_Parameters(): Missing superior URL end quotation mark in DIL entry " << entrynum << "\n";
    return -1;
  }
  suprelurl = tfstr.at(idloc,idloc_next-idloc);
  suprelurl = absurl(idfile,suprelurl);
  idloc = idloc_next + 2; // past '>'
  // Get superior textref
  idloc_next = tfstr.index('<',idloc);
  if ((idloc_next<0) || (idloc_next>=idloc_nextrow)) {
    delete de;
    ERROR << "Get_DIL_ID_File_Superior_Parameters(): Missing superior text reference end tag in DIL entry " << entrynum << ".\n";
    return -1;
  }
  suptext = tfstr.at(idloc,idloc_next-idloc);
  idloc = idloc_next + 6; // past '('
  // Get relevance
  idloc_next = tfstr.index(',',idloc);
  if ((idloc_next<0) || (idloc_next>=idloc_nextrow)) {
    delete de;
    ERROR << "Get_DIL_ID_File_Superior_Parameters(): Missing comma after relevance parameter in DIL entry " << entrynum << ".\n";
    return -1;
  }
  double relevance = str2decimal(tfstr,idloc,idloc_next);
  idloc = idloc_next + 1; // past ','
  // Get unbounded importance
  idloc_next = tfstr.index(',',idloc);
  if ((idloc_next<0) || (idloc_next>=idloc_nextrow)) {
    delete de;
    ERROR << "Get_DIL_ID_File_Superior_Parameters(): Missing comma after unbounded importance parameter in DIL entry " << entrynum << ".\n";
    return -1;
  }
  double unbounded = str2decimal(tfstr,idloc,idloc_next);
  idloc = idloc_next + 1; // past ','
  // Get bounded importance
  idloc_next = tfstr.index(',',idloc);
  if ((idloc_next<0) || (idloc_next>=idloc_nextrow)) {
    delete de;
    ERROR << "Get_DIL_ID_File_Superior_Parameters(): Missing comma after bounded importance parameter in DIL entry " << entrynum << ".\n";
    return -1;
  }
  double bounded = str2decimal(tfstr,idloc,idloc_next);
  idloc = idloc_next + 1; // past ','
  // Get target date and target date properties
  idloc_next = tfstr.index(',',idloc);
  if ((idloc_next<0) || (idloc_next>=idloc_nextrow)) {
    delete de;
    ERROR << "Get_DIL_ID_File_Superior_Parameters(): Missing target date parameter in DIL entry " << entrynum << ".\n";
    return -1;
  }
  time_t targetdate = -1;
  int tdprop;
#ifdef INCLUDE_EXACT_TARGET_DATES
  bool tdex;
  periodictask_t periodicity;
  int span, every;
#endif
#ifdef  DEBUG_Get_DIL_ID_File_Superior_TargetDate
  didtestinspect=false;
  if ((*de)==didtest) {
    //if (de->str().matches("20171129191701.1")) {
    didtestinspect = true;
    INFO << "TEST TEST TEST :\n";
    INFO << "tfstr.length() = " << (long) tfstr.length() << '\n';
    INFO << "idloc = " << idloc << '\n';
    INFO << "idloc_next = " << idloc_next << '\n';
    INFO << "tfstr.at(idloc,idloc_next-idloc) = " << String(tfstr.at(idloc,idloc_next-idloc)) << '\n';
  }
#endif
  if (Get_DIL_ID_File_Superior_TargetDate(tfstr,idloc,idloc_next,targetdate,tdprop
#ifdef INCLUDE_EXACT_TARGET_DATES
					  ,tdex,periodicity,span,every
#endif
					  )<0) {
    ERROR << "Get_DIL_ID_File_Superior_Parameters(): Unable to interpret Superior Target Date\nfor DIL#" << de->chars() << ".\n";
    delete de;
    return -1;
  }
  idloc = idloc_next + 1; // past ','
  // Get urgency
  idloc_next = tfstr.index(',',idloc);
  if ((idloc_next<0) || (idloc_next>=idloc_nextrow)) {
    delete de;
    ERROR << "Get_DIL_ID_File_Superior_Parameters(): Missing comma after urgency parameter in DIL entry " << entrynum << ".\n";
    return -1;
  }
  double urgency = str2decimal(tfstr,idloc,idloc_next);
  idloc = idloc_next + 1; // past ','
  // Get priority
  idloc_next = tfstr.index(')',idloc);
  if ((idloc_next<0) || (idloc_next>=idloc_nextrow)) {
    delete de;
    ERROR << "Get_DIL_ID_File_Superior_Parameters(): Missing comma after priority parameter in DIL entry " << entrynum << ".\n";
    return -1;
  }
  double priority = str2decimal(tfstr,idloc,idloc_next);
  idloc = idloc_next + 1; // past ')', possibly onto the comma to the next superior
  de->parameters->project_append(suprelurl,suptext,relevance,unbounded,bounded,targetdate,tdprop,
#ifdef INCLUDE_EXACT_TARGET_DATES
				 tdex,periodicity,span,every,
#endif
				 urgency,priority);
  // End up with idloc past the parameters, where a comma might be
  return 0;
}

#define NEW_GET_ALL_PARAMETERS

#ifdef NEW_GET_ALL_PARAMETERS
/* This version of the function takes advantage of the limited complexity of
   the DIL by ID HTML text file to find elements more quickly. For example,
   it is not expected that the file would contain tables within tables.
 */
int Detailed_Items_List::Get_All_DIL_ID_File_Parameters(String * dilid) {
// Obtain parameters from the DIL by ID file for all DIL entries
// or for the specific DIL entry dilid if dilid!=NULL
// Note: deletes any previous linked list at listhead and refreshes
// the DIL status
  if (Read_from_Binary_Cache(dilid)) return list.length();

  int ididx, idloc;
  if ((ididx=get_file_in_list(idfile,tf,tfname,tfnum))<0) return -1;
  if ((idloc=tf[ididx].index("<!-- dil2al: DIL ID begin -->"))<0) {
    ERROR << "Detailed_Items_List::Get_All_DIL_ID_File_Parameters(): Missing <!-- dil2al: DIL ID begin --> in " << idfile << ".\n";
    return -1;
  }
  idloc += 29;
  list.clear();

  progress->start();
  progress->update(0,"PARs: ");
  // Find all <TR> blocks
  //StringList dilrows(tf[ididx],"<TR>\n<TD><A NAME=\"",idloc); // 0th element is just header
  //int dilrowslen = dilrows.length();
  // When there are no more <TR>s then the last block ends with <!-- dil2al: DIL ID end -->
  //String & lastrow_s = dilrows[dilrowslen-1];
  //if ((idloc=lastrow_s.index("<!-- dil2al: DIL ID end -->"))<0) { // was (*lastrow)[0].index
  //  EOUT << "dil2al Error: Missing <!-- dil2al: DIL ID end --> in " << idfile << " in Detailed_Items_List::Get_All_DIL_ID_File_Parameters()\n";
  //  return -1;
  //}
  //lastrow_s.del(idloc,lastrow_s.length()-idloc);
  // Collect parameter data from each entry
  int idloc_next, entrypos, entrynum = 0;
  //for (StringList * dilrow_el = dilrows.List_Index(1); (dilrow_el); dilrow_el = dilrow_el->get_Next()) {
  String idsearchstr("<TR>\n<TD><A NAME=\"");
  if (dilid) idsearchstr += *dilid; // Only look for parameters for the one DIL ID
  int idloc_nextrow = tf[ididx].index(idsearchstr,idloc);
  if (idloc_nextrow<0) idloc_nextrow=tf[ididx].length();
  while ((idloc=idloc_nextrow) < (int) tf[ididx].length() ) {
    progress->update((((long) idloc)*100)/tf[ididx].length()); // Fair approximation of progress, since the file is one long list of entries.
    entrypos = idloc;
    idloc += 18; // skip past NAME="
    idloc_nextrow = tf[ididx].index(idsearchstr,idloc);
    if (idloc_nextrow<0) idloc_nextrow=tf[ididx].length();
    // Candidate entry
    idloc_next=tf[ididx].index('"',idloc);
    if ((idloc_next<0) || (idloc_next>=idloc_nextrow)) {
      ERROR << "Detailed_Items_List::Get_All_DIL_ID_File_Parameters(): Missing ending quotation mark around ID in DIL entry " << entrynum << ".\n";
      progress->stop();
      return -1;
    }
    DIL_entry * de = new DIL_entry(*this,tf[ididx].at(idloc,idloc_next-idloc));
    if (!de->valid()) {
      delete de;
      ERROR << "Detailed_Items_List::Get_All_DIL_ID_File_Parameters(): Invalid ID in DIL entry " << entrynum << ".\n";
      progress->stop();
      return -1;
    }
    idloc = idloc_next + 1; // superfluous, since the next line also sets idloc
#ifdef DIAGNOSTIC_OUTPUT
    INFO << "ID = " << de->chars() << '\n';
#endif
    // Entry topics
    idloc=tf[ididx].index("<TD><A HREF=\"",idloc);
    if ((idloc<0) || (idloc>=idloc_nextrow)) {
      delete de;
      ERROR << "Detailed_Items_List::Get_All_DIL_ID_File_Parameters(): Missing topics table cell in DIL entry " << entrynum << ".\n";
      progress->stop();
      return -1;
    }
    idloc += 13;
    int idloc_sup = tf[ididx].index("<TD>",idloc);
    if ((idloc_sup<0) || (idloc_sup>=idloc_nextrow)) {
      delete de;
      ERROR << "Detailed_Items_List::Get_All_DIL_ID_File_Parameters(): Missing superiors table cell in DIL entry " << entrynum << ".\n";
      progress->stop();
      return -1;
    }
    // Now topics are between idloc and idloc_sup, superiors are between idloc_sup+4 and idloc_nextrow
    // One entry topic is obligatory
    de->parameters = new DIL_entry_parameters(*de,entrypos);
    if (Get_DIL_ID_File_Topics_Parameters(tf[ididx],de,idloc,idloc_sup,entrynum)<0) return -1;
    // Get additional topics
    while ((tf[ididx])[idloc]==',') {
      idloc = tf[ididx].index('"',idloc);
      if ((idloc<0) || (idloc>=idloc_sup)) {
	delete de;
	ERROR << "Detailed_Items_List::Get_All_DIL_ID_File_Parameters(): Missing topic file begin quotation mark in DIL entry " << entrynum << ".\n";
	progress->stop();
	return -1;
      }
      idloc++; // past quotation mark
      if (Get_DIL_ID_File_Topics_Parameters(tf[ididx],de,idloc,idloc_sup,entrynum)<0) return -1;
    }
    // Get superiors parameters
    idloc = idloc_sup + 4; // past "<TD>"
    while ((tf[ididx])[idloc]=='<') {
      if (Get_DIL_ID_File_Superior_Parameters(tf[ididx],de,idloc,idloc_nextrow,entrynum)<0) return -1;
      if ((tf[ididx])[idloc]==',') {
	idloc = tf[ididx].index('<',idloc);
	if ((idloc<0) || (idloc>=idloc_nextrow)) {
	  delete de;
	  ERROR << "Detailed_Items_List::Get_All_DIL_ID_File_Parameters(): Missing superior reference in DIL entry " << entrynum << ".\n";
	  progress->stop();
	  return -1;
	}
      }
    }
    
    // Attach to list
    list.link_before(de);
    entrynum++;
    //if ((entrynum % 100)==0) { VOUT << '.'; VOUT.flush(); }
  }
  progress->stop();
#ifdef DIAGNOSTIC_OUTPUT
  INFO << "Number of entries = " << entrynum << '\n';
#endif
  
  if (!Write_to_Binary_Cache()) WARN << "Detailed_Items_List::Get_All_DIL_ID_File_Parameters(): Unable to write to binary cache file " << cacheidfile << ".\nContinuing as is.\n";
  return entrynum;
  
  //*** BEFORE RUNNING THIS FULLY, I SHOULD TEST IT BY PRINTING THE PARAMETERS FOUND!
  //*** CHECK IF ?.? returned as -1.0 is CORRECT FOR PROCESSING!
}

#else // NEW_GET_ALL_PARAMETERS
int Detailed_Items_List::Get_All_DIL_ID_File_Parameters(String * dilid) {
// Obtain parameters from the DIL by ID file for all DIL entries
// or for the specific DIL entry dilid if dilid!=NULL
// Note: deletes any previous linked list at listhead and refreshes
// the DIL status

// *** THIS IS THE OLD, SLOWER VERSION ***
// *** But for now, it's also the only fully TESTED version! ***

/*
INCREASING PARSING SPEED:

A.

To insure that the functionality is preserved, first properly comment each
step done in the current version of the code.

B.

1. Can translate all A-Z characters to a-z to avoid capitalization issues.

2. Can search for "<!--", then "-->", then test if find right tag between.
   You can also still use the Regex search here, since it is only one search.

3. Can do the same for the end tag and copy that part of the text for parsing.

4. Can find all chunks between <TR> <TR> and copy each to a separate String.

5. Try to avoid Regex operations wherever possible.
*/

  if (Read_from_Binary_Cache(dilid)) return list.length();
  
  int ididx, idloc, entrynum = 0;
  if ((ididx=get_file_in_list(idfile,tf,tfname,tfnum))<0) return -1;
  if ((idloc=tf[ididx].index(BigRegex("[<]!--[ 	]*dil2al:[ 	]*DIL ID begin[ 	]*--[>]"),String::SEARCH_END))<0) return -1;
  list.clear();
  // find next entry and obtain ID
  progress->start();
  progress->update(0,"PARS: ");
  String idfilter("[^\"]*"); if (dilid) idfilter = (*dilid);
  BigRegex entryid(("[<]TR[>][^<]*[<]TD[>][^<]*[<]A[ 	]+NAME[ 	]*=[ 	]*\"\\("+idfilter)+"\\)\"[^\n]*\n");
  while ((idloc=tf[ididx].index(entryid,String::SEARCH_END,idloc))>=0) {
    progress->update((((long) idloc)*100)/tf[ididx].length()); // Fair estimate since the file is full of entries
    int sp,ml;
    if (!entryid.match_info(sp,ml,1)) {
      WARN << "Detailed_Items_List::Get_All_DIL_ID_File_Parameters(): Missing ID in DIL entry " << entrynum << ".\nContinuing.\n";
      continue;
    }
    DIL_entry * de = new DIL_entry(*this,tf[ididx].at(sp,ml)); // create candidate entry
    if (!de->valid()) {
      delete de;
      WARN << "Detailed_Items_List::Get_All_DIL_ID_File_Parameters(): Invalid ID in DIL entry " << entrynum << ".\nContinuing.\n";
      continue;
    }
#ifdef DIAGNOSTIC_OUTPUT
    INFO << "ID = " << de->chars() << '\n';
#endif
    // find comma separated list of topics
    String topicline(tf[ididx].at(BigRegex("[<]TD[>][^\n]*\n"),idloc)); // topic data line
    if (topicline.length()==0) {
      delete de;
      WARN << "Detailed_Items_List::Get_All_DIL_ID_File_Parameters(): Missing topical DIL reference in DIL entry " << entrynum << ".\nContinuing.\n";
      continue;
    }
    idloc += topicline.length(); // position after data line
    de->parameters = new DIL_entry_parameters(*de,entryid.subpos());
    int topicloc = 0, numtopics = 0;
    BigRegex entrytopic("\\([<]TD[>]\\|,\\)[^<]*[<]A[ 	]+HREF[ 	]*=[ 	]*\"\\([^#]*\\)#[^\"]*\"[^>]*[>]\\([^<]*\\)[<]/A[>][ 	]*(\\([^)]*\\))");
    while ((topicloc=topicline.index(entrytopic,String::SEARCH_END,topicloc))>=0) {
      int sp2, ml2, sp3, ml3;
      if (!(entrytopic.match_info(sp,ml,2) && entrytopic.match_info(sp2,ml2,3) && entrytopic.match_info(sp3,ml3,4))) {
	WARN << "Detailed_Items_List::Get_All_Parameters(): Invalid topical DIL reference in DIL entry " << entrynum << ".\nContinuing.\n";
	continue;
      }
      de->parameters->topic_append(absurl(idfile,topicline.at(sp,ml)),topicline.at(sp2,ml2),atof(String(topicline.at(sp3,ml3))));
#ifdef DIAGNOSTIC_OUTPUT
      if (DIL_Topical_List * dtl = de->Topics(numtopics)) EOUT << "\ttopic file = " << dtl->dil.file << ", topic title = " << dtl->dil.title << "\n\t\trelevance = " << dtl->relevance << '\n';
#endif
      numtopics++;
    }
    if (!numtopics) {
      delete de;
      WARN << "Detailed_Items_List::Get_All_DIL_ID_File_Parameters(): Missing data in topical DIL field of DIL entry " << entrynum << ".\nContinuing.\n";
      continue;
    }
    // find comma separated list of projects
    int nextidloc; String cellparams, dildata;
    if ((nextidloc=HTML_get_table_cell(tf[ididx],idloc,cellparams,dildata))<0) {
      delete de;
      WARN << "Detailed_Items_List::Get_All_DIL_ID_File_Parameters(): Missing project reference in DIL entry " << entrynum << ".\nContinuing.\n";
      continue;
    }
    idloc = nextidloc;
    HTML_remove_comments(dildata);
    int projloc = 0, numprojects = 0; String hrefurl, hreftext;
    while ((projloc=HTML_get_href(dildata,projloc,hrefurl,hreftext))>=0) {
      int projidx;
      hrefurl = absurl(idfile,hrefurl);
      if ((projidx=Get_Project_Data(dildata,projloc,hrefurl,hreftext,*(de->parameters)))<0) {
	WARN << "Detailed_Items_List::Get_All_DIL_ID_File_Parameters(): Missing project data in DIL entry " << entrynum << ".\nContinuing.\n";
	continue;
      }
      projloc = projidx;
      // (currently ignoring whether or not comma separates project references)
#ifdef DIAGNOSTIC_OUTPUT
      if (DIL_AL_List * dal = de->Projects(numprojects)) EOUT << "\tproject file = " << dal->al.file << ", project title = " << dal->al.title << "\n\t\trelevance = " << dal->relevance << ", unb.imp. = " << dal->unbounded << ", bound.imp. = " << dal->bounded << "\n\t\ttarget date = " << dal->targetdate() << ", urgency = " << dal->urgency << ", priority = " << dal->priority << '\n';
#endif
      numprojects++;
    }
    // *** Currently we tolerate missing project related data
    /*		if (!numprojects) {
		delete de;
		EOUT << "dil2al: Missing data in project reference of DIL entry " << entrynum << " in Detailed_Items_List::Get_All_Parameters(), continuing\n";
		continue;
		}
    */
    // attach to list
    list.link_before(de);
    entrynum++;
    //if ((entrynum % 10)==0) { VOUT << '.'; VOUT.flush(); }
  }
  progress->stop();
#ifdef DIAGNOSTIC_OUTPUT
  INFO << "Number of entries = " << entrynum << '\n';
#endif
  
  if (!Write_to_Binary_Cache()) WARN << "dil2alDetailed_Items_List::Get_All_DIL_ID_File_Parameters(): Unable to write to binary cache file " << cacheidfile << ".\nContinuing as is.\n";
  
  return entrynum;
}
#endif // NEW_GET_ALL_PARAMETERS

int Detailed_Items_List::Get_All_Topical_DIL_Parameters(bool gettext, bool cacherefresh) {
  // Obtain parameters from the Topical DIL files for all DIL entries
  // Note: calls Get_All_DIL_ID_File_Parameters() if necessary
  // to link data to a linked list of DIL entries
  // *** could add options to either overwrite prior data
  //     (current state) or skip duplicate occurrances
  // NOTE: dil[n] refers to the Topical_DIL_Files_Root that is defined in dil2al.hh. It
  // starts out empty, except for its first entry, and extends itself as parsing proceeds.
  // (Consequently, dil.dil.length() cannot be used to estimate a progress percentage.)
  // On the other hand, the globally accessible variable numDILs is updated by
  // Topical_DIL_Files::Refresh(), which is called (if necessary) when the operator[] is
  // used in dil[n].
  
  int ididx,idloc,entrynum,topicalentrynum = 0;
  PLLRoot<DIL_entry_Pointer> dep; // used to note DIL entries per cache
  if (!list.head()) entrynum=Get_All_DIL_ID_File_Parameters();
  else entrynum=list.length();
  progress->start();
  progress->update(0,"DILs");
  for (int n=0; (dil[n]); n++) {
    int percentage = (n*100)/numDILs;
    progress->update(percentage);
    if ((usequickloadcache) && (!cacherefresh)) if (Read_Topical_from_Binary_Cache(n,gettext)) continue;

    dep.clear();
    if ((ididx=get_file_in_list(*(dil[n]),tf,tfname,tfnum))<0) return -1;
    if ((idloc=tf[ididx].index(BigRegex("[<]!--[ 	]*dil2al:[ 	]*DIL begin[ 	]*--[>]"),String::SEARCH_END))<0) return -1;
    // find next entry and obtain ID
    DIL_ID dilid; String markparams, rowcontent, cellcontent, reftext; int rowloc,cellloc, topicalDILentrynum = 0;
    while ((idloc=HTML_get_table_row(tf[ididx],idloc,markparams,rowcontent))>=0) {
      // get the cell 1 of row 1 of DIL entry content, i.e. DIL ID, TL tail, TL head
      if ((rowloc=HTML_get_table_cell(rowcontent,0,markparams,cellcontent))<0) {
	ERROR << "Get_All_Topical_DIL_Parameters(): Missing DIL ID in " << *(dil[n]) << " at entry " << topicalDILentrynum << ".\n";
	progress->stop();
	return -1;
      }
      if ((cellloc=HTML_get_name(cellcontent,0,markparams,reftext))<0) {
	ERROR << "Get_All_Topical_DIL_Parameters(): Missing DIL ID name in " << *(dil[n]) << " at entry " << topicalDILentrynum << ".\n";
	progress->stop();
	return -1;
      }
      dilid = markparams; // obtain DIL entry ID
      DIL_entry * de;
      if ((de = list.head()->elbyid(dilid))==NULL) { // find ID in DIL
	ERROR << "Get_All_Topical_DIL_Parameters(): DIL ID " << dilid.chars() << " in " << *(dil[n]) << " not found in " << idfile << ".\n";
//#define FIND_DILID_LIST_ERROR
#ifdef FIND_DILID_LIST_ERROR
	ERROR << "List of DIL entries:\n";
	PLL_LOOP_FORWARD(DIL_entry,list.head(),1) ERROR << e->chars() << '\n';
#endif
	progress->stop();
	return -1;
      }
      if (cellcontent.contains('(')) if (!de->Get_Entry_TL_References(cellcontent)) {
	  ERROR << "Get_All_Topical_DIL_Parameters(): Missing TL references in " << *(dil[n]) << " at entry " << topicalDILentrynum << ".\n";
	  progress->stop();
	  return -1;
	}
      // get the cell 1 of row 1 of DIL entry content, i.e. obtain parameters
      if ((rowloc=HTML_get_table_cell(rowcontent,rowloc,markparams,cellcontent))<0) {
	ERROR << "Get_All_Topical_DIL_Parameters(): Missing parameters in " << *(dil[n]) << " at entry " << topicalDILentrynum << ".\n";
	progress->stop();
	return -1;
      }
      HTML_remove_tags(cellcontent);
      if (!de->Get_Entry_Parameters(cellcontent)) {
	ERROR << "Get_All_Topical_DIL_Parameters(): Invalid entry parameters in " << *(dil[n]) << " at entry " << topicalDILentrynum << ".\n";
	progress->stop();
	return -1;
      }
      // obtain text
      if ((idloc=HTML_get_table_row(tf[ididx],idloc,markparams,rowcontent))<0) {
	ERROR << "Get_All_Topical_DIL_Parameters(): Missing DIL text row in " << *(dil[n]) << " at entry " << topicalDILentrynum << ".\n";
	progress->stop();
	return -1;
      }
      if (gettext) {
	if ((rowloc=HTML_get_table_cell(rowcontent,0,markparams,cellcontent))<0) {
	  ERROR << "Get_All_Topical_DIL_Parameters(): Missing DIL text in " << *(dil[n]) << " at entry " << topicalDILentrynum << ".\n";
	  progress->stop();
	  return -1;
	}
	//HTML_remove_comments(cellcontent);
	if (!de->content) de->content = new DIL_entry_content(*de);
	delete de->content->text; // remove prior allocation
	de->content->text = new String(cellcontent);
      }
#ifdef DIAGNOSTIC_OUTPUT
      INFO << "ID = " << de->chars() << " (" << de->content->started << ',' << de->content->required << ',' << de->content->completion << ',' << de->content->valuation << ")\n";
#endif
      if (usequickloadcache) dep.link_before(new DIL_entry_Pointer(de));
      topicalDILentrynum++;
    }

    if (!Write_Topical_to_Binary_Cache(n,dep,gettext)) WARN << "Get_All_Topical_DIL_Parameters(): Unable to write topical to binary cache file " << dil.File(n)->chars() << ".cache.\nContinuing as is.\n";

    topicalentrynum += topicalDILentrynum;
  }
  progress->update(100);
  progress->stop();
#ifdef DIAGNOSTIC_OUTPUT
  INFO << "Number of topical entries = " << topicalentrynum << '\n';
#endif
  return entrynum;
}

bool Lock_File(String lockfile, String msgstr) {
  // check for lock file
  struct stat statbuf;
  if (stat(lockfile,&statbuf)!=-1) {
    WARN << "Lock_File(): Lock file " << lockfile <<  " in use by other process, waiting for release of lock.\n";
    WARN.flush(); // This needs to be flushed early so that the lock file can be removed if necessary!
    // *** Alternatively, add an override option here!
  }
  while (stat(lockfile,&statbuf)!=-1) sleep(5);
  // lock and write to binary cache file
  return prepare_temporary_file(lockfile,msgstr);
}

bool Unlock_File(String lockfile) {
  if (unlink(lockfile)<0) {
    WARN << "Unlock_File(): Unable to remove lock file " << lockfile << ".\nContinuing as is.\n";
    return false;
  }
  return true;
}

bool Detailed_Items_List::Write_to_Binary_Cache() {
  // Write a binary copy of the DIL parameters list to a cache file for quick loading.
  bool res = true;
  if (usequickloadcache) {
    if (Lock_File(lockidfile,idfile+" locked by Detailed_Items_List::Write_to_Binary_Cache()\n")) {
      if (verbose) INFO << "Writing parameters to binary cache file\n";
      ofstream cfl(cacheidfile,ios::binary);
      if (!cfl) {
	WARN << "Detailed_Items_List::Write_to_Binary_Cache(): Unable to write to " << cacheidfile << ".\n";
	res=false;
      } else {
	int numentries = list.length();
	cfl.write((const char *) (&numentries),sizeof(numentries));
	PLL_LOOP_FORWARD(DIL_entry,list.head(),1) if (!e->Write_to_Binary_Cache(cfl)) { res=false; break; }
	cfl.close();
      }
      // write new CRC for idfile and unlock
      if (res) {
	CRC_File dilcrc(idfile,crcidfile);
	dilcrc.Write();
      }
      if (!Unlock_File(lockidfile)) WARN << "Detailed_Items_List::Write_to_Binary_Cache(): Unable to remove lock file " << lockidfile << ".\nContinuing as is.\n";
    } else {
      WARN << "Detailed_Items_List::Write_to_Binary_Cache(): Unable to prepare a Quick Load Cache.\n";
      res=false;
    }
  }
  return res;
}

bool Detailed_Items_List::Write_Topical_to_Binary_Cache(int diln, PLLRoot<DIL_entry_Pointer> & dep, bool gottext) {
  // Write a binary copy of the DIL parameters list to a cache file for quick loading.
  /*
    the structure in each cached topical content file is:
    bool gottext
    int DILentrynum
    for each entry:
      DIL_ID dilid
      bool hascontent
      time_t started
      time_t required
      float completion
      float valuation
      int textlen
      char[] text
  */
  bool res = true;
  if (usequickloadcache) {
    String filename((*dil.File(diln)));
    String lockfile(filename+".lock"), cachefile(filename+".cache");
    if (Lock_File(lockfile,filename+" locked by Detailed_Items_List::Write_Topical_to_Binary_Cache()\n")) {
      if (verbose) INFO << "Writing topical content to binary cache file\n";
      ofstream cfl(cachefile,ios::binary);
      if (!cfl) {
	WARN << "Detailed_Items_List::Write_Topical_to_Binary_Cache(): Unable to write to " << cachefile << ".\n";
	res=false;
      } else {
	cfl.write((const char *) (&gottext),sizeof(gottext));
	int DILentrynum = dep.length();
	cfl.write((const char *) (&DILentrynum),sizeof(DILentrynum));
	PLL_LOOP_FORWARD(DIL_entry_Pointer,dep.head(),1) if (!e->Entry()->Write_Topical_to_Binary_Cache(cfl,gottext)) { res=false; break; }
	cfl.close();
      }
      // write new CRC for filename and unlock
      if (res) {
	CRC_File dilcrc(filename);
	dilcrc.Write();
      }
      if (!Unlock_File(lockfile)) WARN << "Detailed_Items_List::Write_Topical_to_Binary_Cache(): Unable to remove lock file " << lockfile << ".\nContinuing as is.\n";
    } else {
      WARN << "Detailed_Items_List::Write_Topical_to_Binary_Cache(): Unable to prepare a Quick Load Cache.\n";
      res=false;
    }
  }
  return res;
}

bool Detailed_Items_List::Read_from_Binary_Cache(String * dilid) {
  // Read a binary copy of the DIL parameters list from a cache file for quick loading.
  if (!usequickloadcache) return false;
  if (!Lock_File(lockidfile,idfile+" locked by Detailed_Items_List::Read_from_Binary_Cache()\n")) return false;
  if (verbose) INFO << "Reading parameters from binary cache file\n";
  CRC_File dilcrc(idfile,crcidfile);
  bool res = dilcrc.IsValid();
  if (verbose && (!res)) INFO << "CRC difference - parsing of authoritative parameter file forced\n";
  if (res) { // read parameters from binary cache file
    ifstream cfl(cacheidfile,ios::binary);
    if (!cfl) {
      WARN << "Detailed_Items_List::Read_from_Binary_Cache(): Unable to read from " << cacheidfile << ".\n";
      res=false;
    } else {
      list.clear();
      int numentries, numentriesread = 0;
      if ((cfl.read((READSOMETYPE) (&numentries),sizeof(numentries))).gcount()< (streamsize) sizeof(numentries)) res = false;
#ifdef TEST_CACHE_READ
      INFO << "&&& numentries = " << numentries << '\n'; cout.flush();
#endif
      while (res) {
#ifdef TEST_CACHE_READ
	INFO << "&&& numentriesread = " << numentriesread << '\n'; cout.flush();
#endif
	DIL_entry * de = new DIL_entry(*this);
	if (!de->Read_from_Binary_Cache(cfl)) { 
	  delete de;
	  if (numentriesread<numentries) {
	    WARN << "Detailed_Items_List::Read_from_Binary_Cache(): Insufficient data retrieved from binary cache file.\n";
	    res = false;
	  }
	  break;
	}
	if (dilid) {
		if (de->str()==(*dilid)) list.link_before(de); else delete de;
	} else list.link_before(de);
	numentriesread++;
	if (numentriesread>numentries) {
	  WARN << "Detailed_Items_List::Read_from_Binary_Cache(): Actual amount of data exceeds cached DIL length retrieved from binary cache file.\n";
	  res = false;
	}
      }
      cfl.close();
      if ((res) && (numentriesread<DILBYID_CACHE_NUMENTRIES_SUSPICIOUS)) {
	WARN << "Number of cached entries read (" << (long) numentriesread << ") deemed suspiciously few, parsing of authoritative parameter file forced\n";
	res = false;
      }
    }
  }
  if (!Unlock_File(lockidfile)) WARN << "Detailed_Items_List::Read_from_Binary_Cache(): Unable to remove lock file " << lockidfile << ".\nContinuing as is.\n";
  return res;
}

bool Detailed_Items_List::Read_Topical_from_Binary_Cache(int diln, bool gettext) {
  // Read a binary copy of the DIL Topical content from a cache file for quick loading.
  if (!usequickloadcache) return false;
  String filename((*dil.File(diln)));
  String lockfile(filename+".lock"), cachefile(filename+".cache");
  if (!Lock_File(lockfile,filename+" locked by Detailed_Items_List::Read_Topical_from_Binary_Cache()\n")) return false;
  if (verbose) INFO << "Reading topical content from binary cache file\n";
  CRC_File dilcrc(filename);
  bool res = dilcrc.IsValid();
  if (verbose && (!res)) INFO << "CRC difference - parsing of authoritative topical content file forced\n";
  if (res) { // read parameters from binary cache file
    ifstream cfl(cachefile,ios::binary);
    if (!cfl) {
      WARN << "Detailed_Items_List::Read_Topical_from_Binary_Cache(): Unable to read from " << cachefile << ".\n";
      res=false;
    } else {
      bool gottext;
      if ((cfl.read((READSOMETYPE) (&gottext),sizeof(gottext))).gcount() < (streamsize) sizeof(gottext)) res = false;
      int numentries, numentriesread = 0;
      if ((cfl.read((READSOMETYPE) (&numentries),sizeof(numentries))).gcount() < (streamsize) sizeof(numentries)) res = false;
#ifdef TEST_CACHE_READ
      VOUT << "&&& numentries = " << numentries << '\n'; cout.flush();
#endif
      if ((gettext) && (!gottext)) {
	INFO << "Text content needed and not available in cache, parsing of authoritative topical content file forced\n";
	res = false;
      }
      while (res) {
#ifdef TEST_CACHE_READ
	VOUT << "&&& numentriesread = " << numentriesread << '\n'; cout.flush();
#endif
	DIL_ID did;
	if (!did.Read_DIL_ID_from_Binary_Cache(cfl)) {
	  if (numentriesread<numentries) {
	    WARN << "Detailed_Items_List::Read_Topical_from_Binary_Cache(): Insufficient data retrieved from binary cache file.\n";
	    res = false;
	  }
	  break;
	} else {
#ifdef TEST_CACHE_READ
          VOUT << "&&& did = " << did.chars() << '\n'; cout.flush();
#endif
	  DIL_entry * de = list.head()->elbyid(did);
#ifdef TEST_CACHE_READ
          VOUT << "&&& de = " << de->chars() << '\n'; cout.flush();
#endif
	  if (!de) {
	    WARN << "Detailed_Items_List::Read_Topical_from_Binary_Cache(): DIL ID " << did.chars() << " not found.\n";
	    res = false;
	  } else {
#ifdef TEST_CACHE_READ
            VOUT << "&&& de->Read_Topical_from_Binary_Cache(cfl," << gottext << ")\n"; cout.flush();
#endif
	    if (!de->Read_Topical_from_Binary_Cache(cfl,gottext)) { 
	      WARN << "Detailed_Items_List::Read_Topical_from_Binary_Cache(): Unable to read topical content for " << did.chars() << ".\n";
	      res = false;
	    } else {
	      /*	    if (dilid) {
			    if (de->str()==(*dilid)) list.link_before(de); else delete de;
			    } else list.link_before(de); */
	      numentriesread++;
	      if (numentriesread>numentries) {
		WARN << "Detailed_Items_List::Read_Topical_from_Binary_Cache(): Actual amount of data exceeds cached DIL length retrieved from binary cache file in.\n";
		res = false;
	      }
	    }
	  }
	}
      }
      cfl.close();
      if ((res) && (numentriesread<1)) {
	WARN << "Number of cached entries read (" << (long) numentriesread << ") deemed suspiciously few, parsing of authoritative topical content file forced\n";
	res = false;
      }
    }
  }
  if (!Unlock_File(lockfile)) WARN << "Detailed_Items_List::Read_Topical_from_Binary_Cache(): Unable to remove lock file " << lockfile << ".\nContinuing as is.\n";
  return res;
}

bool Detailed_Items_List::Binary_Cache_Diagnostic() {
  // test if the quick load cache process works reliably
  INFO << "dil2al::Detailed_Items_List::Binary_Cache_Diagnostic()\nTesting reliability of quick load cache process\n(Note that this is a good integrity test if modifications of the dilentry\nobjects necessitate corresponding modifications of the caching functions.)\n  flag `usequickloadcache' is ";
  if (usequickloadcache) INFO << "set\n";
  else INFO << "not set\n";
  // parse the HTML file
  bool _usequickloadcache = usequickloadcache;
  usequickloadcache = false;
  INFO << "Parsing authoritative DIL parameters file\n";
  if (Get_All_DIL_ID_File_Parameters()<0) return false;
  INFO << "Parsing authoritative DIL topical content files and writing to binary cache files\n";
  usequickloadcache = true;
  if (Get_All_Topical_DIL_Parameters(true,true)<0) return false;
  // save to cache
  INFO << "Writing to binary cache file " << cacheidfile << '\n';
  if (!Write_to_Binary_Cache()) {
    ERROR << "Detailed_Items_List::Binary_Cache_Diagnostic(): Unable to write to binary cache file " << cacheidfile << ".\n";
    return false;
  }
  // load from cache into temporary list
  INFO << "Reading from binary cache file into temporary list for comparison\n";
  Detailed_Items_List tmplist;
  if (!tmplist.Read_from_Binary_Cache()) return false;
  for (int n = 0; (dil[n]); n++) if (!tmplist.Read_Topical_from_Binary_Cache(n,true)) return false;
  // compare the original and temporary lists
  //   start of this object test
  int testedbytes = sizeof(Text_File_Buffers);
  INFO << "Original length   = " << (long) list.length() << '\n';
  INFO << "Cache read length = " << (long) tmplist.list.length() << '\n';
  testedbytes += sizeof(list);
  //   end of this object test
  if (testedbytes!=sizeof(Detailed_Items_List)) INFO << "*** testedbytes!=sizeof(Detailed_Items_List)\n";
  for (DIL_entry * e = list.head(), * c = tmplist.list.head(); ((e) && (c)); e = e->Next(), c = c->Next()) if (!e->Binary_Cache_Diagnostic(c,testedbytes)) return false;
  INFO << "Total bytes tested: " << (long) testedbytes << '\n';
  usequickloadcache = _usequickloadcache;
  return true;
}

DIL_entry ** Detailed_Items_List::Sort_by_Target_Date(bool gettext) {
// Create an array of pointers to DIL entries and sort it in
// ascending order of target dates
  int entrynum,i = 0;
  _RESET_TIMER("Detailed_Items_List::Sort_by_Target_Date:Get_All_Topical_DIL_Parameters()");
  if (!list.head()) entrynum=Get_All_Topical_DIL_Parameters(gettext);
  else entrynum=list.length();
  if (entrynum<1) {
    EOUT << "dil2al: No DIL to sort in Detailed_Items_List::Sort_by_Target_Date()\n";
    return NULL;
  }
  _REPORT_TIMER("Detailed_Items_List::Sort_by_Target_Date:Get_All_Topical_DIL_Parameters()");
  // set up array
  _RESET_TIMER("Detailed_Items_List::Sort_by_Target_Date:qsort()");
  progress->start();
  progress->update(15,"Sort DIL by Target Date");
  DIL_entry_ptr * dep = new DIL_entry_ptr[entrynum];
  PLL_LOOP_FORWARD(DIL_entry,list.head(),(i<entrynum)) { dep[i] = e; i++; }
  // sort array
  qsort(dep,entrynum,sizeof(DIL_entry_ptr),DIL_entry_target_date_qsort_compare);
  _REPORT_TIMER("Detailed_Items_List::Sort_by_Target_Date:qsort()");
  progress->stop();
  return dep;
}

bool Detailed_Items_List::Write_to_File() {
// writes a modified DIL-by-ID file
// requires that Topic and Project/Superior parameters are available
// (such as provided by a prior call to Get_All_DIL_ID_File_Parameters())
#ifdef CAUTIOUS_OBJECT_INTERFACES
  int ididx = FileIndex(idfile);
  if (ididx<0) return false;
#else
  const int ididx = 0;
#endif
  // copy free-style header and table column headings
  int idloc;
  if ((idloc=tf[ididx].index(BigRegex("[<]!--[ 	]*dil2al:[ 	]*DIL ID begin[ 	]*--[>]"),String::SEARCH_END))<0) {
    WARN << "Detailed_Items_List::Write_to_File(): Missing header in " << idfile << ".\nContinuing as is.\n";
    return false;
  }
  String newidfile(tf[ididx].through(idloc)), rowparams, rowcontent;
  idloc = HTML_get_table_row(tf[ididx],idloc,rowparams,rowcontent);
  rowparams = HTML_marker_arguments(rowparams);
  newidfile += "\n\n" + HTML_put_table_row(rowparams,rowcontent,true);
  // generate table
  DIL_entry * de = list.head();
  if (!de) {
    WARN << "Detailed_Items_List::Write_to_File(): No DIL entries in list to write to " << idfile << ".\nContinuing as is.\n";
    return false;
  }
  if (de->parameters) {
    String topics, projects, tstamp;
    PLL_LOOP_FORWARD(DIL_entry,de,1) {
      topics = ""; PLL_LOOP_FORWARD_NESTED(DIL_Topical_List,e->Topics(0),1,te) {
	if (topics.length()>0) topics += ", ";
	topics += HTML_put_href(relurl(idfile,te->dil.file)+'#'+e->chars(),te->dil.title)+" (";
	if (te->relevance==DILTOPICS_REL_UNSPECIFIED) topics += "?.?)";
	else topics += String(te->relevance,DILBYID_FLOAT_FORMAT)+')';
      }
      if (topics.length()<=0) {
	topics = "@MISSING TOPICAL CONTENT REFERENCE@";
	WARN << "Detailed_Items_List::Write_to_File(): Missing Topical Content Reference for DIL#" << e->chars() << " in " << idfile << ". Continuing.\n";
      }
      projects = ""; PLL_LOOP_FORWARD_NESTED(DIL_AL_List,e->Projects(0),1,pe) {
	if (projects.length()>0) projects += ", ";
	projects += HTML_put_href(relurl(idfile,pe->al.file),pe->al.title)+" (";
	if (pe->relevance==DILSUPS_REL_UNSPECIFIED) projects += "?.?,";
	else projects += String(pe->relevance,DILBYID_FLOAT_FORMAT)+',';
	if (pe->unbounded==DILSUPS_UNB_UNSPECIFIED) projects += "?.?,";
	else projects += String(pe->unbounded,DILBYID_FLOAT_FORMAT)+',';
	if (pe->bounded==DILSUPS_BND_UNSPECIFIED) projects += "?.?,";
	else projects += String(pe->bounded,DILBYID_FLOAT_FORMAT)+',';
#ifdef INCLUDE_EXACT_TARGET_DATES
	bool possibleperiodic = false;
	if ((pe->tdexact()) || (pe->tdproperty() & DILSUPS_TDPROP_EXACT)) {
	  projects += 'E';
	  possibleperiodic = true;
	}
	else if (pe->tdproperty() & DILSUPS_TDPROP_FIXED) {
	  projects += 'F';
	  possibleperiodic = true;
	}
	if (possibleperiodic) {
	  if (pe->tdproperty() & DILSUPS_TDPROP_PERIODIC) {
	    periodictask_t periodicity = pe->tdperiod();
	    if (periodicity>=pt_nonperiodic) {
	      if (calledbyforminput) VOUT << "dil2al ERROR: Unknown enumerateed value #" << (int) periodicity << " for task periodicity, unable to update DIL ID file in Detailed_Items_List::Write_to_File(), continuing as is\n";
	      else WARN << "Detailed_Items_List::Write_to_File(): Unknown enumerateed value #" << (int) periodicity << " for task periodicity, unable to update DIL ID file.\nContinuing as is.\n";
	      return false;
	    }
	    projects += periodictaskchar[periodicity];
	    long every = pe->tdevery();
	    if (every>1) projects += 'e' + String(every) + '_';
	    long span = pe->tdspan();
	    if (span>1) projects += 's' + String(span) + '_';
	  }
	}
#else
	if (pe->tdproperty()==DILSUPS_TDPROP_FIXED) projects += 'F';
#endif
	if (pe->targetdate()==DILSUPS_TD_UNSPECIFIED) projects += "?,";
	else {
	  tstamp = time_stamp("%Y%m%d%H%M",pe->targetdate());
	  if (tstamp.at((int) tstamp.length()-4,4)=="0000") tstamp = tstamp.before((int) tstamp.length()-4);
	  projects += tstamp+',';
	}
	if (pe->urgency==DILSUPS_URG_UNSPECIFIED) projects += "?.?,";
	else projects += String(pe->urgency,DILBYID_FLOAT_FORMAT)+',';
	if (pe->priority==DILSUPS_PRI_UNSPECIFIED) projects += "?.?)";
	else projects += String(pe->priority,DILBYID_FLOAT_FORMAT)+')';
      }
      if (projects.length()<=0) projects = "&nbsp;";
      newidfile += HTML_put_table_row("",'\n'+HTML_put_table_cell("",HTML_put_name(e->chars(),e->chars()),true)+'\n'+
				      HTML_put_table_cell("",topics,true)+'\n'+
				      HTML_put_table_cell("",projects,true),true)+"\n\n";
    }
  }
  // copy free-style footer
  if ((idloc=tf[ididx].index(BigRegex("[<]!--[ 	]*dil2al:[ 	]*DIL ID end[ 	]*--[>]"),idloc))<0) {
    WARN << "Detailed_Items_List::Write_to_File(): Missing footer in " << idfile << ".\nContinuing as is.\n";
    return false;
  }
  newidfile += tf[ididx].from(idloc);
  if (!write_file_from_String(idfile,newidfile,"DIL ID")) return false;
  if (immediatecachewriteback) Write_to_Binary_Cache();
  return true;
}

int System_Call(String syscall) {
  // This is a wrapper for the system() call that is needed in the case
  // where dil2al is run as a CGI program from w3m in the Linux 2.6 Gentoo
  // environment. Without it, system returns -1 and the errno for No Child
  // Processes, even when the system call execution succeeded. A similar
  // wrapper would be needed for wait() when used with fork().
  // Warning: Do not use this when you need to inspect the return value
  // for information and when the call might occur within CGI execution.
  // Set a semaphore instead while in the child process (if using fork())
  // or while in the shell process.
  // Yes, this is a hassle! Check bug lists to see when this problem is
  // solved or to see if there is a special protocol for CGI programs.
  
  if (calledbyforminput) {
    // WARN << "Using unchecked system call while in CGI mode.\n";
    int retval = system(syscall); // -1 may not be meaningful
    if (retval<-1) ERROR << "System call returned error (" << retval << ") with call: " << syscall <<".\n";
    return 0;
    /* This should probably use semaphores and perhaps clone() to communicate
       success or failure from child to parent.
       pid_t p = fork();
       if (p==-1) return -1; // fork failed
       if (p==0) { // child
       String progstr;
       const char * argstr[];
       int res = execvp(progstr,argstr);
       if (res==-1) exit(-1); // execlp failed
       } else { // parent
       }
    */
  } else return system(syscall);
}

int DIL_entry_target_date_qsort_compare(const void * vde1, const void * vde2) {
// used with qsort() (<stdlib.h>)
// Treats unspecified self-referential target dates as MAXTIME
// for the ordering of the corresponding DIL entry  - and avoids
// the accompanying warning messages.
// (This is useful when groups of DIL entries with desired
// target dates are sorted, in which some DIL entries are marked
// with -2 codes).
// Note: ISO C++ is strict about providing the correct function
// declaration in cases such as the call to qsort(). This function
// was therefore rewritten from the original declaration,
// DIL_entry_target_date_qsort_compare(DIL_entry ** de1, DIL_entry ** de2)
// - please keep in mind that the elements are of type DIL_entry **,
// i.e. arrays/pointers of pointers to objects.
  
  DIL_entry ** de1 = (DIL_entry **) vde1;
  DIL_entry ** de2 = (DIL_entry **) vde2;
  time_t t1 = (MAXTIME_T), t2 = (MAXTIME_T);
  if (*de1) t1 = (*de1)->Propagated_Target_Date(false);
  if (*de2) t2 = (*de2)->Propagated_Target_Date(false);
  if (t1==t2) return 0;
  if (t1<t2) return -1;
  return 1;
}

int DIL_Virtual_Matrix_target_date_qsort_compare(const void * vde1, const void * vde2) {
// used with qsort() (<stdlib.h>)
// Treats unspecified self-referential target dates as MAXTIME
// for the ordering of the corresponding DIL_Virtual_Matrix entry -
// and avoids the accompanying warning messages.
// (This is useful when groups of DIL entries with desired
// target dates are sorted, in which some DIL entries are marked
// with -2 codes).
// Note that this function also sorts entries with equal target
// dates depending on whether they are virtual copies or not
// (see TL#200610260847.1).
  
  DIL_Virtual_Matrix * de1 = (DIL_Virtual_Matrix *) vde1;
  DIL_Virtual_Matrix * de2 = (DIL_Virtual_Matrix *) vde2;
  time_t t1 = (MAXTIME_T), t2 = (MAXTIME_T);
  if (de1->dep) {
    if (de1->isvirtual) t1 = de1->td;
    else t1 = de1->dep->Propagated_Target_Date(false);
  }
  if (de2->dep) {
    if (de2->isvirtual) t2 = de2->td;
    else t2 = de2->dep->Propagated_Target_Date(false);
  }
  if (t1==t2) {
    if (de1->isvirtual) {
      if (de2->isvirtual) return 0;
      return 1; // do de2 first
    }
    if (de2->isvirtual) return -1; // do de1 first
    return 0;
  }
  if (t1<t2) return -1;
  return 1;
}

int BigRegex_freq(String & str, const BigRegex & rx) {
  int i = 0, cnt = 0;
  while ((i=str.index(rx,String::SEARCH_END,i))>=0) cnt++;
  return cnt;
}

int BigRegex_freq(String & str, String rxstr) {
  BigRegex rx(rxstr);
  return BigRegex_freq(str,rx);
}

int BigRegex_freq(String & str, const char * rxstr) {
  BigRegex rx(rxstr);
  return BigRegex_freq(str,rx);
}

String RX_Search_Safe(String s) {
// converts a regular string to that
// string's match in a BigRegex search
  s.gsub("[","[[]");
  s.gsub("]","[]]");
  s.gsub(BigRegex("[\\.*+?$]"),"[_0]",'_');
  if ((s.length()>0) && (s[0]=='^')) s = "[^]" + s.after(0);
  return s;
}

time_t Time(time_t *tloc) {
// Local time() wrapper, allows emulation of different current time
// returns time and sets (*tloc) to the same time if tloc!=NULL
  if (emulatedtime) {
    if (tloc) *tloc = emulatedtime;
    return emulatedtime;
  }
  return time(tloc);
}

String time_stamp(const char * dateformat, time_t t)  {
// time stamp in local time
  char dstr[80];
  if (t<0) t = Time(NULL);
  strftime(dstr,80,dateformat,localtime(&t));
  return String(dstr);
}

String time_stamp_GM(const char * dateformat, time_t t) {
  // Greenwich Meantime time stamp
  //
  // NOTE: Use this when expressing time differences
  //       such as hours and minutes since 0.
  
  char dstr[80];
  if (t<0) t = Time(NULL);
  strftime(dstr,80,dateformat,gmtime(&t));
  return String(dstr);
}

time_t time_add_day(time_t t, int days) {
  // This is relatively safe, because days are added by using localtime() and mktime()
  // instead of just adding days*SECONDSPERDAY. That should keep the time correct, even
  // through daylight savings time.
  //
  // NOTE: Nevertheless, when adding many days at once, this can break. It
  // has been shown to break for some iterations of virtual periodic task
  // target dates generated in alcomp.cc:generate_AL_prepare_periodic_tasks(),
  // which subsequently caused an alert in AL_Day::Add_Target_Date().
  
  struct tm *tm;
  tm = localtime(&t);
  tm->tm_mday += days;
  tm->tm_isdst = -1; // this tells mktime to determine if DST is in effect
  return mktime(tm);
}

time_t time_add_month(time_t t, int months) {
  struct tm *tm;
  tm = localtime(&t);
  int years = months/12;
  months -= (years*12);
  tm->tm_year += years;
  tm->tm_mon += months;
  if (tm->tm_mon>11) {
    tm->tm_year++;
    tm->tm_mon -= 12;
  }
  tm->tm_isdst = -1;
  return mktime(tm);
}

int time_day_of_week(time_t t) {
  struct tm *tm;
  tm = localtime(&t);
  return tm->tm_wday;
}

int time_month_length(time_t t) {
  struct tm *tm;
  tm = localtime(&t);
  tm->tm_mday = 32;
  tm->tm_isdst = -1;
  time_t t2 = mktime(tm);
  tm = localtime(&t2);
  int m2day = tm->tm_mday;
  m2day--; // the number of days shorter than 31 that this month is
  return 31 - m2day;
}

time_t time_add_month_EOMoffset(time_t t) {
  int m1len = time_month_length(t);
  int m2len = time_month_length(time_add_month(t));
  struct tm *tm;
  tm = localtime(&t);
  int offset = m1len - tm->tm_mday;
  tm->tm_mday = m2len - offset;
  tm->tm_mon++;
  if (tm->tm_mon>11) {
    tm->tm_year++;
    tm->tm_mon=0;
  }
  tm->tm_isdst = -1;
  return mktime(tm);
}

String date_string() {
  return time_stamp("%Y%m%d");
}

time_t time_stamp_time(String timestr, bool noexit) {
  // Local time string to calendar time time_t.
  // The time string must be either:
  // 1) of the format *YYYYmmddHHMM (e.g. "201901270714", " 201901270714", etc), or
  // 2) a negative number (e.g. "-2")
  // Negative numbers are returned as they are so that special codes can be detected.
  //
  // NOTE: White space or other characters before the 12 digit time stamp is ignored.
  // The noexit flag allows stricttimestamps behavior to be overridden in cases
  // where a -1 return is used specifically to detect empty/unfinished/etc time stamps.
  
  struct tm ts;
  int tl = timestr.length();
  
  // Detect special codes
  if (tl>0) if (timestr[0]=='-') return (time_t) atol((const char *) timestr);
  
  // Confirm *YYYYmmddHHMM format
  if (tl<12) {
    if (!noexit) {
      if (verbose) WARN << timestr << " is not a proper *YYYYmmddHHMM time_stamp_time(), returning -1.\n";
      _DEBUG_TIME_STAMP_TIME_CALLER_REPORT("time_stamp_time()");
      if (stricttimestamps) Exit_Now(1);
    }
    return (time_t) -1;
  }
  const char * tschars = (const char *) timestr;
  for (int i = tl-12; i<tl; i++) if ((tschars[i]<'0') || (tschars[i]>'9')) {
      if (!noexit) {
	if (verbose) WARN << timestr << " is not a proper *YYYYmmddHHMM time_stamp_time(), returning -1.\n";
	_DEBUG_TIME_STAMP_TIME_CALLER_REPORT("time_stamp_time()");
	if (stricttimestamps) Exit_Now(1);
      }
      return (time_t) -1;
    }
  ts.tm_sec = 0;
  // Since we've already assured they'd digits, might as well compute directly instead of slowing ourselves down with atoi()
  ts.tm_min = (tschars[tl-2]-'0')*10 + (tschars[tl-1]-'0');
  ts.tm_hour = (tschars[tl-4]-'0')*10 + (tschars[tl-3]-'0');
  ts.tm_mday = (tschars[tl-6]-'0')*10 + (tschars[tl-5]-'0');
  ts.tm_mon = (tschars[tl-8]-'0')*10 + (tschars[tl-7]-'0') - 1;
  ts.tm_year = ((tschars[tl-12]-'0')*1000 + (tschars[tl-11]-'0')*100 + (tschars[tl-10]-'0')*10 + (tschars[tl-9]-'0'));
  if (ts.tm_year<1900) {
    if (!noexit) {
      if (verbose) WARN << timestr << " is before the year 1900 in time_stamp_time(), returning -1.\n";
      _DEBUG_TIME_STAMP_TIME_CALLER_REPORT("time_stamp_time()");
      if (stricttimestamps) Exit_Now(1);
    }
    return (time_t) -1;
  }
  ts.tm_year -= 1900;
  ts.tm_wday = 0;
  ts.tm_yday = 0;
  ts.tm_isdst = -1; // computed by mktime since indicated "unknown" here
  return mktime(&ts);
}

/*
Previous, less safe version of time_stamp_time():
time_t time_stamp_time(String timestr) {
  // Local time string to calendar time time_t.
  // negative numbers are returned as they are so that
  // special codes can be detected
	struct tm ts;
	int tl = timestr.length();
	if (tl>0) if (timestr[0]=='-') return (time_t) atol((const char *) timestr);
	if (tl<9) {
	  if (verbose) EOUT << "Warning: " << timestr << " is not a proper time_stamp_time(), returning -1.\n";
	  return (time_t) -1;
	}
	ts.tm_sec = 0;
	ts.tm_min = atoi((const char *) String(timestr.at(tl-2,2)));
	ts.tm_hour = atoi((const char *) String(timestr.at(tl-4,2)));
	ts.tm_mday = atoi((const char *) String(timestr.at(tl-6,2)));
	ts.tm_mon = atoi((const char *) String(timestr.at(tl-8,2)))-1;
	ts.tm_year = atoi((const char *) String(timestr.at(0,tl-8)))-1900;
	ts.tm_wday = 0;
	ts.tm_yday = 0;
	ts.tm_isdst = -1; // computed by mktime since indicated "unknown" here
	return mktime(&ts);
}
*/

time_t time_stamp_time_of_day(String timestr) {
  int tl = timestr.length();
  String h(timestr.at(tl-4,2)),m(timestr.at(tl-2,2));
  return ((((time_t) atoi(h.chars()))*60)+((time_t) atoi(m.chars())))*60;
}


time_t time_stamp_time_date(String timestr) {
// local time string %Y%m%d%H%M set to 00:00 time of
// day to calendar time
  int tl = timestr.length() - 4;
  _DEBUG_TIME_STAMP_TIME_CALLER(__PRETTY_FUNCTION__);
  return time_stamp_time(String(timestr.before(tl))+"0000"); // *** Probably correct with noexit=false (20190127).
}

time_t time_stamp_diff(String oldtime,String newtime) {
  _DEBUG_TIME_STAMP_TIME_CALLER(__PRETTY_FUNCTION__);
  return time_stamp_time(newtime) - time_stamp_time(oldtime); // *** Probably correct with noexit=false (20190127).
}

time_t double2time(double t) {
  struct tm td;
  td.tm_sec = 0; // seconds not explicit
  td.tm_min = (int) fmod(t,100.0); t /= 100.0;
  td.tm_hour = (int) fmod(t,100.0); t /= 100.0;
  td.tm_mday = (int) fmod(t,100.0); t /= 100.0;
  td.tm_mon = ((int) fmod(t,100.0))-1; t /= 100.0;
  td.tm_year = ((int) t)-1900;
  td.tm_wday = 0;
  td.tm_yday = 0;
  td.tm_isdst = 0;
  return mktime(&td);
}

bool confirmation(String message, char nondefault, const char * defoption, const char * nondefoption) {
  // requests confirmation
  // returns false if the response == nondefault
  // returns true for any other response
  
  return (confirm->request(message,nondefault,defoption,nondefoption,false)<=0);
}
/*
Old confirmation():
  const int LLEN = 16;
  char lbuf[LLEN];
  cout << message;
  cin.getline(lbuf,LLEN);
  return (lbuf[0]!=nondefault);
*/

char auto_interactive(String * autostr, String message, String respchars, StringList & respdescriptors) {
  // Enables interactive responses or takes responses from autostr if available.
  // autostr points to a string containing response characters.
  // If autostr==NULL then this behaves much like UI_Confirm*.
  // This accomplishes a degree of scriptability of expected interactions.

  if ((autostr) && (!autostr->empty())) {
    char res = autostr->elem(0);
    autostr->del(0,1);
    return res;
  }

  return confirm->multi_request(message,respchars,respdescriptors);
}

String relurl(String srcurl, String dsturl) {
  String tmp = common_prefix(srcurl.through('/',-1),dsturl.through('/',-1));
  if ((!tmp.empty()) && (tmp.lastchar()!='/')) tmp = tmp.through('/',-1);
  if (tmp!="") {
    srcurl = srcurl.after(tmp);
    dsturl = dsturl.after(tmp);
    tmp = dsturl.from('#'); // for dsturl references in srcurl file
    if (tmp.length()>0) {
      String tmp2 = srcurl.before('#'); if (tmp2.length()==0) tmp2 = srcurl;
      if (tmp2==dsturl.before('#')) return tmp;
    }
    while (srcurl.before('/')!="") { srcurl=srcurl.after('/'); dsturl.prepend("../"); }
  }
  return dsturl;
}

String absurl(String srcurl, String relurl) {
  // srcurl is an absolute filename or a directory path
  // ending with "/"
  // relurl is a URL relative to srcurl or an absolute URL
  //
  // NOTE: This function may be confused by directory names
  //       of the form BigRegex(".+[.][.]")
  
  if (relurl=="") return srcurl;
  if (relurl.contains("://")) relurl = relurl.after("://");
  if (relurl[0]=='/') return relurl;
  if (relurl[0]=='#') return srcurl+relurl;
  int upcnt = relurl.freq("../");
  relurl.gsub("../","");
  srcurl = srcurl.before('/',-1); // prune source filename
  for (int i = 0; i < upcnt; i++) srcurl = srcurl.before('/',-1);
  return srcurl+'/'+relurl;
}

/*
for every URL found in a file in the list of files
1. determine if relative or absolute URL, remember that
2. get absolute URL from HREF
3. compare with URL to be replaced (ignoring # if # is not in the to-be-replaced)
4. if matches, use new URL to create relative or absolute version, keeping # intact if not in to-be-replaced
this should be applied to the files that are moved as well as to files referencing them
and in the moved files all relative URLs have to be modified
*/

bool file_is_regular(String filepath) {
  struct stat fs;
  if (stat(filepath,&fs)!=0) {
    if (verbose) ERROR << "file_is_regular(): Error reading file attributes of " << filepath << ".\n";
    return false;
  }
  return S_ISREG(fs.st_mode);
}

bool file_is_regular_or_symlink(String filepath) {
  struct stat fs;
  if (stat(filepath,&fs)!=0) {
    if (verbose) ERROR << "file_is_regular_or_symlink(): Error reading file attributes of " << filepath << ".\n";
    return false;
  }
  return (S_ISREG(fs.st_mode) || (S_ISLNK(fs.st_mode)));
}

bool file_is_directory(String filepath) {
  struct stat fs;
  if (stat(filepath,&fs)!=0) {
    if (verbose) ERROR << "file_is_directory(): Error reading file attributes of " << filepath << ".\n";
    return false;
  }
  return S_ISDIR(fs.st_mode);
}

bool prepare_temporary_file(String tmpfile, String initstr) {
// Can be used to create temporary files (e.g. the dilref or,
// note or DIL entry stubs), and automatically back up the
// previous file if it exists. The stub is filled with the
// content of initstr and closed.
  if (rename(tmpfile,tmpfile+".bak")<0) if (errno!=ENOENT) {
      ERROR << "prepare_temporary_file(): Unable to rename " << tmpfile << " to " << tmpfile+".bak.\n";
      return false;
    }
  ofstream ntf(tmpfile);
  if (!ntf) {
    ERROR << "prepare_temporary_file(): Unable to create temporary file " << tmpfile << ".\n";
    return false;
  }
  if (initstr!="") ntf << initstr;
  ntf.close();
  return true;
}

// NOTE: detect_temporary_file() was made inline and the definition consequently
// moved to the header file.

bool queue_temporary_file(String tmpfile, String initstr) {
  // Creates the temporary file tmpfile and initializes it with the
  // string in initstr when that file is not possibly in used by
  // another active process.
  //
  // This is similar to prepare_temporary_file, except it is assumed
  // that if the temporary file already exists, then another process
  // may still be using it. Care is taken to queue successive uses
  // of a specific temporary file. An example where this is applied
  // is in the use of the dil2al-note.html file (see note.cc:make_note()).
  //
  // If this function is called while processing FORM input then the
  // check engages a wait loop that automatically fails out after
  // 5 minutes (100 three second iterations).
  // Otherwise the check allows the user to interactively decide to
  // check again or to override the collision safety. Checking again
  // is done a maximum of 100 times.

  // Check if the temporary file exists to avoid collisions with other
  // processes using that file.
  int i = 100;
  if (calledbyforminput) {
    for (; (detect_temporary_file(tmpfile) && (i>0)); i--) {
      // Try again for up to 5 minutes
      INFO << "Temporary file " << tmpfile << " exists - possibly being used by\nanother active process.\nNext attempt in 3 seconds. (" << i << " remaining)\n";
      INFO.flush(); // This needs to be seen right away.
      sleep(3);
    }
    if (i<=0) return false;
  } else {
    for (; (detect_temporary_file(tmpfile) && (i>0)); i--) {
      // Interactively decide to check again or override
      if (!confirmation("Temporary file "+tmpfile+" exists - possibly being used by\nanother active process.\nTest again or 'o' to override? ",'o')) {
	// override by renaming the existing temporary file
	if (rename(tmpfile,tmpfile+".bak")<0) if (errno!=ENOENT) {
	    ERROR << "queue_temporary_file(): Unable to rename " << tmpfile << " to " << tmpfile+".bak.\n";
	    ERROR.flush();
	    return false;
	  }
	break;
      }
    }
    if (i<=0) return false;
  }

  // Create and initialize the temporary file
  ofstream ntf(tmpfile);
  if (!ntf) {
    ERROR << "queue_temporary_file(): Unable to create temporary file " << tmpfile << ".\n";
    ERROR.flush();
    return false;
  }
  if (!initstr.empty()) ntf << initstr;
  ntf.close();

  return true;
}

bool backup_and_rename(String filename, String message, bool isnew) {
  // Backup previous version of file with date stamp and make
  // new version current.
  
  String backupname = filename+'.'+curdate+".bak";
  if (!isnew) if (rename(filename,backupname)<0) {
      if (errno!=ENOENT) {
	ERROR << "backup_and_rename(): Unable to rename " << filename << " to " << backupname << ".\n(New version remains in " << filename << ".new)\n";
	return false;
      } else WARN << "backup_and_rename(): No old version of " << filename << " to backup.\n";
    }
  if (rename(filename+".new",filename)<0) {
    ERROR << "backup_and_rename(): Unable to rename " << filename << ".new to " << filename << ".\n(New version remains in " << filename << ".new)\nRestoring previous " << filename << '\n';
    if (!isnew) if (rename(backupname,filename)<0) ERROR << "backup_and_rename(): Unable to restore " << backupname << " to " << filename << ".\n";
    return false;
  }
  if (verbose) { // *** perhaps this should be suppressed if errno==ENOENT was caught above
    if (isnew) INFO << message << " initialized as " << filename << '\n';
    else INFO << "Old " << message << " file moved to " << backupname << '\n';
  }
  return true;
}

bool read_file_into_String(ifstream & sfl, String & filestr, bool warnnoopen) {
  // read from an open file stream into the string filestr,
  // optionally warning about the inability to open the file
  // returns true if file content was read into filestr
  
  if (sfl) {
    filestr = sfl;
    /*		sfl.seekg(0,ios::end); // seek to EOF
		long int sflsize = sfl.tellg();
		sfl.seekg(0);
		char * s;
		s = new char[sflsize+1];
		sfl.read(s,sflsize);
		s[sfl.gcount()]='\0';
		filestr = s;
		delete[] s;
		if (sfl.gcount()!=sflsize) EOUT << "dil2al: Characters read (" << sfl.gcount() << ") does not equal expected file size (" << sflsize << ") in read_file_into_String(), continuing as is\n";
    */
    sfl.close();
    return true;
  } else {
    if (warnnoopen) ERROR << "read_file_into_String(): Unable to open file.\n";
    return false;
  }
}

bool read_file_into_String(String filename, String & filestr, bool warnnoopen) {
  // read the file called filename into the string filestr,
  // optionally warning about the inability to open the file
  // returns true if file content was read into filestr
  
  if (filename.empty()) {
    if (warnnoopen) WARN << "read_file_into_String(): Missing file name.\n";
    return false;
  }
  ifstream sfl(filename);
  if (sfl.fail()) {
    if (warnnoopen) WARN << "read_file_into_String(): Unable to open " << filename << " as ifstream.\n";
    return false;
  }
  if (read_file_into_String(sfl,filestr,false)) return true;
  
  if (warnnoopen) WARN << "read_file_into_String(): Unable to open " << filename << ".\n";
  return false;
}

bool read_file_into_String(bool & isnew, String filename, String & filestr) {
// read the file called filename into filestr if it exists,
// optionally offer to create the file if it does not exist,
// or warn about the inability to open the file
// returns true if file content was read into filestr
// note that no file is created at this point, but the setting
// of the isnew flag can be used later (e.g. by the function
// write_string_to_file()) to anticipate new file creation
  
  if (read_file_into_String(filename,filestr,false)) {
    isnew = false;
    return true;
  } else {
    if (offercreate) {
      if (confirmation("File "+filename+"\ndoes not exist, create (y/N)? ",'y',"No","Create")) return false;
      isnew = true;
      return true;
    } else {
      WARN << "read_file_into_String(): Unable to open " << filename << ".\n";
      return false;
    }
  }
}

int get_file_in_list(String & fname, StringList & srcf, StringList &srcfname, int & srcfnum) {
  // Load a file if it is not already in a list of file-strings.
  // Parameters:
  //   fname is the name of the file to get
  //   srcf is a list of loaded file contents
  //   srcfname is a list of the file names for which content was already loaded in srcf
  //   srcfnum is the number of files for which content was already loaded in srcf
  // Returns the index number if fname is found in the list srcfname.
  // Returns the new index number if fname was added to srcfname and new file content was
  // loaded to srcf. In this case srcfnum is increased.
  // Returns -1 if fname could not be loaded.
  // See the class Text_File_Buffers for an easy way to maintain lists of file content.
  // NOTE: There is no limit imposed here to the number of files that can be loaded
  // into String buffers. This can consume considerable memory.
  
  int srcidx;
  if ((srcidx = srcfname.iselement(fname))<0) {
    if (!read_file_into_String(fname,srcf[srcfnum])) {
      ERROR << "get_file_in_list(): Unable to load source document " << fname << ".\n";
      return -1;
    }
    srcfname[srcfnum] = fname;
    srcidx = srcfnum;
    srcfnum++;
  }
  return srcidx;
}

bool write_file_from_String(String filename, const String & filestr, String message, bool isnew) {
  // Write a file from string content, with automatic backup of
  // a previous version (if one exists).
  
#ifdef DEBUG
  bool b = (filename==taskloghead);
  if (b) cout << '+';
#endif
  String newfilename = filename;
  if (!isnew) newfilename += ".new";
  ofstream ntl(newfilename);
  if (!ntl) {
    ERROR << "write_file_from_String(): Unable to create " << newfilename << ".\n";
    return false;
  }
  ntl << filestr;
  ntl.close();
#ifdef DEBUG
  if (b) cout << '+';
#endif
  if (isnew) { if (verbose && (!message.empty())) INFO << message << " initialized as " << filename << '\n'; }
  else if (!backup_and_rename(filename,message)) return false;
#ifdef DEBUG
  if (b) cout << "+\n";
#endif
  return true;
}

void read_data_from_stream(istream & ifs, String & dstr, const char * terminate) {
// Reads data from a stream such as the data input stream
// DIN line by line. Returns when end of file or a line
// stating the terminate string is found.
// *** This can be replaced by String::readtotag if LLEN=4096 is sufficient.
  
  const int LLEN = 10240;
  char lbuf[LLEN];
  while (ifs) {
    ifs.getline(lbuf,LLEN);
    if (ifs.eof()) break;
    if (strcmp(lbuf,terminate)==0) break;
    dstr = (dstr + lbuf) + '\n';
  }
}

inline int find_delimiter(String & str, int startpos, int lenlimit, char delimiter) {
  int delimpos = str.index(delimiter,startpos);
  if (delimpos>=lenlimit) return -1;
  return delimpos;
}

bool pattern_in_line(const char * pattern, char * lbuf) {
  regex_t re;
  int mres;
  if (regcomp(&re,pattern, REG_EXTENDED|REG_NOSUB) != 0) {
    ERROR << "pattern_in_line(): Unable to compile regular expression.\n";
    return false;
  }
  mres = regexec(&re,lbuf, (size_t) 0, NULL, 0);
  regfree(&re);
  if ((mres != 0) && (mres != REG_NOMATCH)) {
    ERROR << "pattern_in_line(): Internal error while matching.\n";
    return false;
  }
  return (mres==0);
}

bool substring_from_line(const char * pattern, int nsub, String * dilid, const char * lbuf) {
  const int SLEN = 1024;
  char sbuf[SLEN];
  regex_t re;
  regmatch_t * rm;
  int mres;
  if (nsub) {
    if (regcomp(&re,pattern, REG_EXTENDED) != 0) {
      ERROR << "substring_from_line(): Unable to compile regular expression.\n";
      return false;
    }
    rm = new regmatch_t[nsub];
    mres = regexec(&re,lbuf, (size_t) nsub, rm, 0);
    if ((mres != 0) && (mres != REG_NOMATCH)) {
      ERROR << "substring_from_line(): Internal error while matching.\n";
    } else {
      if (!mres) {
	if (rm[nsub-1].rm_so==-1) {
	  WARN << "substring_from_line(): Wrong number of substring matches.\n";
	  mres = 3;
	} else {
	  for (int i=0; i<nsub; i++) {
	    strncpy(sbuf,&lbuf[rm[i].rm_so],rm[i].rm_eo-rm[i].rm_so);
	    sbuf[rm[i].rm_eo-rm[i].rm_so]='\0';
	    dilid[i] = sbuf;
	  }
	}
      }
    }
    delete[] rm;
    return (mres==0);
  } else {
    ERROR << "substring_from_line(): nsub==0.\n";
    return false;
  }
}

bool find_line(ifstream * lf, const char * pattern, char * lbuf, int llen) {
  // takes an open ifstream and searches for the regex pattern
  // if found, lbuf returns the line containing pattern
  
  regex_t re;
  int mres;
  if (*lf) {
    if (regcomp(&re,pattern, REG_EXTENDED|REG_NOSUB) != 0) {
      ERROR << "find_line(): Unable to compile regular expression.\n";
      return false;
    }
    mres = 1;
    while (mres!=0) {
      if (lf->eof()) {
	regfree(&re);
	return false;
      }
      lf->getline(lbuf,llen);
      mres = regexec(&re,lbuf, (size_t) 0, NULL, 0);
      if ((mres != 0) && (mres != REG_NOMATCH)) {
	ERROR << "find_line(): Internal error while matching.\n";
	regfree(&re);
	return false;
      }
    }
    regfree(&re);
    return true;
  } else {
    ERROR << "find_line(): file not open.\n";
    return false;
  }
}

int copy_until_line(ifstream * lf, ofstream * of, const char * pattern, char * lbuf, int llen) {
// returns 1 if line was encountered, 0 if not, and <0 for errors
// the line containing pattern is not copied, but is returned in lbuf
  
  regex_t re;
  int mres;
  if (!lf) {
    ERROR << "copy_until_line(): input file not open.\n";
    return -1;
  }
  if (!of) {
    ERROR << "copy_until_line(): output file not open.\n";
    return -1;
  }
  if (regcomp(&re,pattern, REG_EXTENDED|REG_NOSUB) != 0) {
    ERROR << "copy_until_line(): Unable to compile regular expression.\n";
    return -2;
  }
  mres = 1;
  WARN << "copy_until_line(<from>,<to>," << pattern << ",<lbuf>,<llen>) may be broken as EOF is not detected when line length exceeds llen=" << (long) llen << ", this code should use BigRegex functions instead!\n";
  while (mres!=0) {
    if (lf->eof()) {
      regfree(&re);
      return 0;
    }
    lf->getline(lbuf,llen);
    mres = regexec(&re,lbuf, (size_t) 0, NULL, 0);
    if ((mres != 0) && (mres != REG_NOMATCH)) {
      ERROR << "copy_until_line(): Internal error while matching.\n";
      regfree(&re);
      return -3;
    }
    if (mres!=0) (*of) << lbuf << endl;
  }
  regfree(&re);
  return 1;
}

int find_in_line(ifstream * lf, const char * pattern, int nsub, String * dilid, const char * before, char * lbuf, int llen) {
  // takes an open ifstream and searches for the regex pattern with nsub subexpressions
  // before a line with the ``before'' pattern (NULL = EOF)
  // returns substrings in dilid and the return value
  // 1 (found ``pattern''), 0 (found ``before''), -1 (EOF),
  // -2 (wrong #substrings), -3 (regex error) -4 (file not open)
  
  const int SLEN = 1024;
  char sbuf[SLEN];
  regex_t re,be;
  regmatch_t * rm;
  int mres = -1;
  bool done;
  if (*lf) {
    if (nsub) rm = new regmatch_t[nsub];
    else rm = NULL;
    if (regcomp(&re,pattern, REG_EXTENDED) != 0) {
      ERROR << "find_in_line(): Unable to compile ``pattern'' regular expression.\n";
      return -3;
    }
    if (before) {
      if (regcomp(&be,before, REG_EXTENDED|REG_NOSUB) != 0) {
	regfree(&re);
	ERROR << "find_in_line(): Unable to compile ``before'' regular expression.\n";
	return -3;
      }
    }
    done = false;
    while (!done) {
      if (lf->eof()) { mres = 2; done=true; }
      else {
	lf->getline(lbuf,llen);
	if (before) mres = regexec(&be,lbuf, (size_t) 0, NULL, 0);
	if ((before) && (mres != 0) && (mres != REG_NOMATCH)) {
	  ERROR << "find_in_line(): Internal error while matching ``before''.\n";
	  mres = 4; done = true;
	} else {
	  if ((before) && (mres==0)) { mres = 1; done = true; }
	  else {
	    mres = regexec(&re,lbuf, (size_t) nsub, rm, 0);
	    if ((mres != 0) && (mres != REG_NOMATCH)) {
	      ERROR << "find_in_line(): Internal error while matching ``pattern''.\n";
	      mres = 4; done = true;
	    } else {
	      if (!mres) {
		done = true;
		if (nsub) {
		  if (rm[nsub-1].rm_so==-1) {
		    WARN << "find_in_line(): Wrong number of substring matches.\n";
		    mres = 3;
		  } else {
		    for (int i=0; i<nsub; i++) {
		      strncpy(sbuf,&lbuf[rm[i].rm_so],rm[i].rm_eo-rm[i].rm_so);
		      sbuf[rm[i].rm_eo-rm[i].rm_so]='\0';
		      dilid[i] = sbuf;
		    }
		  }
		}
	      }
	    }
	  }
	}
      }
    }
    regfree(&re);
    regfree(&be);
    if (nsub) delete[] rm;
    return (-mres) + 1;
  } else {
    ERROR << "find_in_line(): file not open.\n";
    return -4;
  }
}

bool initialize_new_DIL_TL(String initializecmd, bool includeinput) {
  // sends a DIL template with a new DIL ID, or a Task Log chunk or entry template to stdout
  
  if (initializecmd=="tc") { // initialize Task Log chunk
    cout << generate_TL_chunk_header("{"+curtime+"}","{relative-active-list-url}","{Active List Title}","{#tag-previous-AL-head}","relative-DIL-url","DIL Title/Non-DIL Identifying Comment","#tag-previous-DIL-head",-1) << "<!-- edit items in ``{}'' brackets";
    if (!includeinput) cout << " and add entries here";
    cout << " then close chunk -->\n";
  } else if (initializecmd=="te") { // initialize Task Log entry
    cout << generate_TL_entry_header("{"+curtime+"}.{1}","{relative-active-list-url}","{Active List Title}","{#tag-previous-AL-head}","relative-DIL-url","DIL Title/Non-DIL Identifying Comment","#tag-previous-DIL-head") << "<!-- edit items in ``{}'' brackets";
    if (!includeinput) cout << " and add content here";
    cout << " -->\n";
  } else { // initialize DIL entry
    String newdilid = get_new_DIL_ID();
    if (newdilid=="") {
      ERROR << "initialize_new_DIL_TL(): Unable to generate new DIL ID.\n";
      return false;
    }
    cout << "<TR BGCOLOR=\"#00005F\">\n<TD WIDTH=\"15%\"><A NAME=\""
	 << newdilid << "\" HREF=\"detailed-items-by-ID.html#"
	 << newdilid << "\">" << newdilid
	 << "</A>\n<TD><B>time started:</B> - <B>time required:</B> ?.?hrs <B>completion ratio:</B> 0.0 <B>valuation:</B> ?.?\n<TR>\n<TD COLSPAN=2>";
  }
  if (includeinput) {
    String dentrybuf;
    read_data_from_stream(DIN,dentrybuf,"@End of DIL entry@");
    cout << dentrybuf;
  }
  if (initializecmd=="tc") { // Task Log chunk
    cout << generate_TL_chunk_header("","","","","","","",1);
  } else if (initializecmd=="te") { // Task Log entry
    cout << "<P>\n\n";
  } else { // DIL entry
    cout << "\n\n";
  }
  
  return true;
}

int D2A_get_tag(String & d2astr, int strloc, String d2atag, String & tagtext) {
  // searches for a dil2al standardized tag with the specific regex
  // id d2atag and returns the corresponding specific information
  // in tagtext, as well as the position in d2astr immediately
  // following the tag
  
  BigRegex tm('@'+d2atag+":\\([^@]*\\)@");
  if ((strloc=d2astr.index(tm,String::SEARCH_END,strloc))>=0) {
    int s,l;
    if (!tm.match_info(s,l,1)) return -1;
    if (l==0) WARN << "D2A_get_tag(): Non-specific tag at " << s << ". Continuing.\n";
    tagtext = d2astr.at(s,l);
  }
  return strloc;
}

int HTML_get_marker(String & htmlstr, int strloc, String & marker,bool locate) {
  // If 'locate'==false: at 'strloc' in 'htmlstr' take content between and
  // including HTML tag markers (e.g. "<TITLE>") and put that in 'marker',
  // return the location after those characters. If no marker was found then
  // it returns -1.
  // If 'locate'==true: search for the next HTML tag marker (e.g. "<TITLE>"),
  // starting at 'strloc' in 'htmlstr'. If found, it puts the tag into
  // 'marker' and returns the location after those characters. If not, then
  // it returns -1.
  
  const BigRegex m("[<][^>]+[>]");
  // find start of marker
  if (locate) {
    if ((strloc=htmlstr.index(m,strloc))<0) return -1;
    // get marker
    int s,l;
    m.match_info(s,l,0);
    marker = htmlstr.at(s,l);
    return strloc+l;
  }
  
  // get marker
  marker = htmlstr.at(m,strloc); // assumes marker starts at strloc
  if (marker.empty()) return -1;
  return strloc+marker.length();
}

String HTML_marker_arguments(String & marker) {
  const BigRegex a("[ 	]");
  String res = marker.from(a); if (res.length()>=1) res = res.before((int) res.length()-1);
  return res;
}

int HTML_get_tagged_environment(const BigRegex & begintag, const BigRegex & endmatch, const BigRegex & endtag, String & htmlstr, int strloc, String & envparams, String & envcontent, int * envstart) {
  // searches for an environment beginning with a tag matching begintag,
  // determines the environment end according to possible matches in endmatch,
  // returns tag parameters in envparams, environment contents in envcontent,
  // optionally returns the start of the environment in envstart, and
  // returns the position directly after the environment and a potential
  // endtag
  
  // find start of environment
  if ((strloc=htmlstr.index(begintag,strloc))<0) return -1;
  if (envstart) (*envstart) = strloc;
  // get environment tag parameters
  if ((strloc=HTML_get_marker(htmlstr,strloc,envparams,false))<0) return -1;
  // get environment content
  int envend = htmlstr.index(endmatch,strloc);
  if (envend<0) envend = htmlstr.length();
  envcontent = htmlstr.at(strloc,envend-strloc);
  // skip possible end tag
  int envendtagend = htmlstr.index(endtag,String::SEARCH_END,envend);
  if (envendtagend>=0) envend = envendtagend;
  return envend;
}

int HTML_get_table_cell(const String & htmlstr, int strloc, String & cellparams, String & cellcontent, int * cellstart) {
  const BigRegex cm("[<][Tt][Dd]");
  const BigRegex c("\\([<]/?[Tt][DdRr]\\|[<]/[Tt][Aa][Bb][Ll][Ee]\\|[<]/[Bb][Oo][Dd][Yy]\\|[<]/[Hh][Tt][Mm][Ll]\\)");
  const BigRegex ce("[<]/[Tt][Dd][^>]*[>]");
  return HTML_get_tagged_environment(cm,c,ce,const_cast<String &>(htmlstr),strloc,cellparams,cellcontent,cellstart);
}

int HTML_get_table_row(String & htmlstr, int strloc, String & rowparams, String & rowcontent, int * rowstart) {
  const BigRegex rm("[<][Tt][Rr]");
  const BigRegex r("\\([<]/?[Tt][Rr]\\|[<]/[Tt][Aa][Bb][Ll][Ee]\\|[<]/[Bb][Oo][Dd][Yy]\\|[<]/[Hh][Tt][Mm][Ll]\\)");
  const BigRegex re("[<]/[Tt][Rr][^>][>]");
  return HTML_get_tagged_environment(rm,r,re,htmlstr,strloc,rowparams,rowcontent,rowstart);
}

int HTML_get_list(String & htmlstr, int strloc, String & listparams, String & listcontent, int * liststart) {
  const BigRegex lm("[<][UuOo][Ll]");
  const BigRegex l("\\([<]/[UuOo][Ll]\\|[<]/[Tt][Aa][Bb][Ll][Ee]\\|[<]/[Bb][Oo][Dd][Yy]\\|[<]/[Hh][Tt][Mm][Ll]\\)");
  const BigRegex le("[<]/[UuOo][Ll][^>][>]"); 
  return HTML_get_tagged_environment(lm,l,le,htmlstr,strloc,listparams,listcontent,liststart);
}

int HTML_get_list_item(String & htmlstr, int strloc, String & itemparams, String & itemcontent, int * itemstart) {
  const BigRegex im("[<][Ll][Ii]");
  const BigRegex i("\\([<]/?[Ll][Ii]\\|[<]/[UuOo][Ll]\\|[<]/[Tt][Aa][Bb][Ll][Ee]\\|[<]/[Bb][Oo][Dd][Yy]\\|[<]/[Hh][Tt][Mm][Ll]\\)");
  const BigRegex ie("[<]/[Ll][Ii][^>][>]");
  return HTML_get_tagged_environment(im,i,ie,htmlstr,strloc,itemparams,itemcontent,itemstart);
}

int HTML_get_href(String & htmlstr, int strloc, String & hrefurl, String & hreftext, int * hrefstart) {
  // *** warning, this function can't deal properly yet with HREFs that contain
  //     HTML font formatting commands (e.g. <B>) in the reference text!

  const BigRegex h("[<][Aa][ 	]+[^>]*[Hh][Rr][Ee][Ff][ 	]*=[ 	]*\"\\([^\"]*\\)\"[^>]*[>]\\([^<]*\\)[<]/[Aa][ 	]?[^>]*[>]");
  if ((strloc=htmlstr.index(h,String::SEARCH_END,strloc))>=0) {
    int s,l;
    if (hrefstart) {
      if (!h.match_info(s,l,0)) return -1;
      else *hrefstart = s;
    }
    if (!h.match_info(s,l,1)) return -1;
    if (l==0) {
      ERROR << "HTML_get_href(): Missing HREF URL at " << s << ".\n";
      return -1;
    }
    hrefurl = htmlstr.at(s,l);
    if (!h.match_info(s,l,2)) return -1;
    if (l==0) WARN << "HTML_get_href(): Invisible HREF anchor at " << s << ". Continuing.\n";
    hreftext = htmlstr.at(s,l);
  }
  return strloc;
}

int HTML_get_name(String & htmlstr, int strloc, String & nametag, String & nametext) {
  const BigRegex n("[<][Aa][ 	]+[^>]*[Nn][Aa][Mm][Ee][ 	]*=[ 	]*\"\\([^\"]*\\)\"[^>]*[>]\\([^<]*\\)[<]/[Aa][ 	]?[^>]*[>]");
  if ((strloc=htmlstr.index(n,String::SEARCH_END,strloc))>=0) {
    int s,l;
    if (!n.match_info(s,l,1)) return -1;
    if (l==0) {
      ERROR << "HTML_get_name(): Missing NAME tag at " << s << ".\n";
      return -1;
    }
    nametag = htmlstr.at(s,l);
    if (!n.match_info(s,l,2)) return -1;
    if (l==0) WARN << "HTML_get_name(): Invisible NAME anchor at " << s << ". Continuing.\n";
    nametext = htmlstr.at(s,l);
  }
  return strloc;
}

String * HTML_safe(String & textstr) {
  textstr.gsub("&","&amp;");
  textstr.gsub("<","&lt;");
  textstr.gsub(">","&gt;");
  return &textstr;
}

String * HTML_remove_comments(String & htmlstr) {
  int cidx, cend = 0; String res;
  res.alloc(htmlstr.length());
  while ((cidx=htmlstr.index("<!--",cend))>=0) {
    if (cidx>cend) res += htmlstr.at(cend,cidx-cend);
    if ((cend=htmlstr.index(BigRegex("--[ 	]*[>]"),String::SEARCH_END,cidx))<0) cend=htmlstr.length();
  }
  cidx=htmlstr.length();
  if (cidx>cend) res += htmlstr.at(cend,cidx-cend);
  htmlstr = res;
  return &htmlstr;
}

/*String * HTML_remove_comments(String & htmlstr) {
// *** Is there a problem with String::del()?
  int cidx = 0, cend;
  while ((cidx=htmlstr.index("<!--",cidx))>=0) {
    if ((cend=htmlstr.index(BigRegex("--[ 	]*[>]"),String::SEARCH_END,cidx))<0) cend=htmlstr.length();
    
    htmlstr.del(cidx,cend-cidx);
    cidx=cend;
  }
  return &htmlstr;
  }*/

String * HTML_remove_tags(String & htmlstr) {
  const BigRegex m("[<][^>]+[>]");
  htmlstr.gsub(m,"");
  return &htmlstr;
}

String HTML_put_structure(String tag, String params, String content, bool relaxedhtml) {
  params.prepend('<'+tag);
  params += '>'+content;
  if (relaxedhtml) return params;
  params += "</"+tag+">\n";
  return params;
}

String HTML_put_table_row(String rowparams, String rowcontent, bool relaxedhtml) {
  return HTML_put_structure("TR",rowparams,rowcontent,relaxedhtml);
}

String HTML_put_table_cell(String cellparams, String cellcontent, bool relaxedhtml) {
  return HTML_put_structure("TD",cellparams,cellcontent,relaxedhtml);
}

String HTML_put_list(char listtype,String htmlstr) {
  return HTML_put_structure(listtype+String("L"),"",htmlstr);
}

String HTML_put_list_item(String htmlstr, bool relaxedhtml) {
  return HTML_put_structure("LI","",htmlstr,relaxedhtml);
}
 
String HTML_put_href(String hrefurl, String hreftext) {
  return "<A HREF=\""+hrefurl+"\">"+hreftext+"</A>";
}
 
String HTML_put_name(String nametag, String nametext) {
  return "<A NAME=\""+nametag+"\">"+nametext+"</A>";
}
 
String HTML_put_form(String formmethod, String formaction, String formcontent) {
  return "<FORM METHOD="+formmethod+" ACTION=\""+formaction+"\">"+formcontent+"</FORM>";
}
 
String HTML_put_form_input(String inputtype, String inputargs) {
  String inputstr("<INPUT type=\""+inputtype+'"');
  if (inputargs.length()>0) return inputstr+' '+inputargs+'>';
   else return inputstr+'>';
}

String HTML_put_form_submit(String submittext) {
  return HTML_put_form_input("submit","value=\""+submittext+'"');
}

String HTML_put_form_reset() {
  return HTML_put_form_input("reset","");
}

String HTML_put_form_radio(String radioname, String radiovalue, String radiotext, bool checked) {
  String checkedstr;
  if (checked) checkedstr = "\" checked"; else checkedstr = "\"";
  return HTML_put_form_input("radio","name=\""+radioname+"\" value=\""+radiovalue+checkedstr)+' '+radiotext;
}

String HTML_put_form_checkbox(String checkboxname, String checkboxvalue, String checkboxtext, bool checked) {
  String checkedstr;
  if (checked) checkedstr = "\" checked"; else checkedstr = "\"";
  return HTML_put_form_input("checkbox","name=\""+checkboxname+"\" value=\""+checkboxvalue+checkedstr)+' '+checkboxtext;
}

String HTML_put_form_text(String textname, String textvalue, long maxlen) {
  textname += '"';
  if (textvalue.length()>0) textname += " value=\"" + textvalue + '"';
  if (maxlen>0) textname += " maxlength=" + String(maxlen);
  return HTML_put_form_input("text","name=\""+textname);
}

String HTML_put_form_text(String textname, long maxlen) {
  return HTML_put_form_text(textname,"",maxlen);
}

int Elipsis_At(String & s, unsigned int maxsize) {
  // returns the length of the resulting string
  
  if (s.length()>maxsize) {
    s = s.before((int) maxsize);
    s = s.before(BigRegex("[ 	\n.,;:]"),-1);
    s += "...";
  }
  return s.length();
}

String URI_unescape(String escapedURI) {
  // convert URI strings with escaped characters to regular character strings
  
  String s; char c; int d1,d2;
  for (unsigned int i = 0; i<escapedURI.length(); i++) {
    c = escapedURI[i];
    if (c=='%') {
      i++; c = escapedURI[i];
      if ((c>='0') && (c<='9')) d1 = (int) (c - '0');
      else if ((c>='A') && (c<='F')) d1 = ((int) (c - 'A')) + 10;
      else d1 = ((int) (c - 'a')) + 10;
      i++; c = escapedURI[i];
      if ((c>='0') && (c<='9')) d2 = (int) (c - '0');
      else if ((c>='A') && (c<='F')) d2 = ((int) (c - 'A')) + 10;
      else d2 = ((int) (c - 'a')) + 10;
      d1 *=16; d1 += d2;
      c = (char) d1;
    }
    s += c;
  }
  return s;
}

String Int2HexStr(unsigned int i) {
  // convert integer to hexadecimal string
  
  String s;
  if (i==0) s = "0";
  else {
    while (i>0) {
      int j = i % 16; i = i / 16;
      if (j<10) s.prepend(('0'+(char) j));
      else s.prepend(('A'+(char) (j-10)));
    }
  }
  return s;
}

String * remove_whitespace(String & str) {
  // Note that this removes EXCESS white space, leaving a single
  // space in place of any sequence of whitespace characters.
  
  str.gsub(BRXwhite," ");
  return &str;
}

String * delete_all_whitespace(String & str) {
  // This removes all whitespace characters completely.
  
  str.gsub(BRXwhite,"");
  return &str;
}

String * escape_for_single_quote_cmdarg(String & str) {
  // This replaces all single quote characters, so that
  // the resulting string can be safely passed to a shell
  // command as an argument by enclosing it in single quotes.
  // NOTE: Regular escaping with "\'" is not safe enough,
  // because arguments are sometimes parsed twice. For
  // example, this happens in
  // interface.cc:run_yad_request_script().
  // For this reason, single quotes are replaced
  // completely.
  
  str.gsub("'","^");
  return &str;
}
 
int get_int(String valstr, int defval, bool * gotint) {
  // attempts to extract an int variable value from a string
  // if no int variable value is found then the value defval is assigned
  
  bool gotintdummy;
  if (!gotint) gotint = &gotintdummy;
  if (valstr.empty()) { (*gotint) = false; return defval; }
  if (((*gotint)=valstr.matches(BRXint))==true) return atoi((const char *) valstr);
  else return defval;
}
 
double get_double(String valstr, double defval, bool * gotdouble) {
  // attempts to extract a double variable value from a string
  // if no double variable value is found then the value defval is assigned
  
  bool gotdoubledummy;
  if (!gotdouble) gotdouble = &gotdoubledummy;
  if (valstr.empty()) { (*gotdouble) = false; return defval; }
  if (((*gotdouble)=valstr.matches(BRXdouble))==true) return atof((const char *) valstr);
  else return defval;
}
