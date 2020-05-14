// dil2al.hh
// Randal A. Koene, 20000304

/*
======== 1) HEADER COMMENTS ========
 */

/*
For information about this project:
- See the Google Doc 'Formalizer'.
- See the documentation referred to in the Formalizer Google doc.
- See the further documentation snippets referred from from those documents.
- See the 'DIL, AL and TL automation' DIL file and the Task Log entries of the tasks described there.

This header file contains the following sections:

1) Header comments (you are reading them right now).
2) Include statements
3) Debugging macros
4) Compile-time options
5) Constant definitions
6) Short-hand macros and old-style inline macros
7) Types and enumerators
8) Classes and early declarations
9) Configuration parameter defaults
10) Global parameters and variables declarations
11) Exported function declarations
12) Inline member functions
13) Inline non-member functions
 */

// NOTES:
// - This was designed with the assumption of a platform
//   that defines an int as at least 32 bits wide
//   (if compiled on a platform where int is smaller,
//   the possible sizes of some file sections is restricted).
// - The script ~/local/bin/liststar creates archives of essential
//   files. It also deletes *.bak and *~ files, so that they are not
//   archived.

// * dil2al can warn about violations of the hints about efficient working

#ifndef __DIL2AL_HH
#define __DIL2AL_HH

/*
======== 2) INCLUDE STATEMENTS ========
 */

#include "BigRegex.hh"
#include "BigString.hh"
#include "templates.hh"
/*#include <sys/types.h>
extern "C" {
#ifdef _USE_ALT_REGEX
	#include _ALT_REGEX_H
#else
	#ifdef SYSTEM_RX
		#include <regex.h>
	#else
		#include "rx.h"
	#endif
#endif
}*/
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
using namespace std;
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <math.h>
#include <values.h>
#include <float.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
//#include <sys/shm.h
#include <dirent.h>
#include "cksum.hh"

/*
======== 3) DEBUGGING MACROS ========
 */

// NOTE: Even the debug defines are kept as local as possible. They are
// found in the respective source files. E.g. tlfilter.cc includes
// definitions of DEBUGTLFILTER_A, DEBUGTLFILTER_A.
// Debugging definitions shown here are meant to be available in every
// scope.

// The following is a more modern (and localized) approach to inline debugging.
#define DEBUG_TIME_STAMP_TIME 0
extern String debug_time_stamp_time_caller;
#define _DEBUG_TIME_STAMP_TIME_CALLER(CallerLabel) \
  do { if (DEBUG_TIME_STAMP_TIME) debug_time_stamp_time_caller=CallerLabel; } while (0)

// These belong to the older approach to inline debugging.
//#define DEBUG
#ifdef DEBUG
//	#define DEBUGTIER1
//	#define DEBUGTIER2
//	#define VERBOSEDEBUG
//	#define DIAGNOSTIC_OUTPUT
//	#define DETAILED_AL_DIAGNOSTIC_OUTPUT
//	#define DEBUG_REPORT_DEVIATIONS
#endif

// provide resettimer() and readtimer() to profile time spent in code snippets
//#define INCLUDE_PROFILING_TIMERS
#ifdef INCLUDE_PROFILING_TIMERS
  void resettimer(); // (dil2al.cc)
  void readtimer(long & sec, long & usec); // (dil2al.cc)
  #define PROFILING_TIMERS_ON 1
#else
  #define PROFILING_TIMERS_ON 0
#define resettimer() \
  do { if (0) {} } while (0)
#define readtimer(SeC,UseC) \
  do { if (0) {} } while (0)
#endif
#define _RESET_TIMER(CallerLabel) \
  do { if (PROFILING_TIMERS_ON) { INFO << "Profiling start at " << CallerLabel << '\n'; resettimer(); } } while (0)
#define _REPORT_TIMER(CallerLabel) \
  do { if (PROFILING_TIMERS_ON) { long sec, usec; readtimer(sec,usec); INFO << "Profiling times for " << CallerLabel << " sec=" << sec << " usec=" << usec << '\n'; } } while (0)
#ifdef INCLUDE_PROFILING_TIMERS
#endif

#define _EXTRA_VERBOSE_REPORT_FUNCTION() \
  if (extra_verbose) INFO << '[' << __FUNCTION__ << "]\n"

/*
======== 4) COMPILE-TIME OPTIONS ========
 */

// NOTE: Not all compile-time options are chosen here. Some may be
// included as -D<something> arguments in the Makefile.

#define LOOP_PROTECT
#define CAUTIOUS_OBJECT_INTERFACES
#define INCLUDE_EXACT_TARGET_DATES
#define INCLUDE_DAY_SPECIFIC_WORK_TIMES
#ifdef INCLUDE_DAY_SPECIFIC_WORK_TIMES
  #ifdef INCLUDE_EXACT_TARGET_DATES
     #define INCLUDE_PERIODIC_TASKS
  #endif
#endif
#define INCLUDE_POOL_PLANNING_METHOD

//#define UI_JOINED_YAD_LIST_STRINGLISTS
#define UI_JOINED_YAD_NOTEBOOK


/*
======== 5) CONSTANT DEFINITIONS ========
 */

// version using gcc 2.96 on basal
/*#ifdef _TYPEBITS
	#define HITIME_T ((time_t)(1 << _TYPEBITS(time_t) - 1))
#else
// prior version, as compiled on aurora
	#define HITIME_T ((time_t)(1 << BITS(time_t) - 1))
#endif*/
#ifdef _TYPEBITS
#define HITIME_T ((time_t)(((unsigned long) 1) << (_TYPEBITS(time_t) - 1)))
#else
// prior version, as compiled on aurora
#define HITIME_T ((time_t)(((unsigned long) 1) << (BITS(time_t) - 1)))
#endif

// The following is a precaution for the localtime() function on 64 bit platforms, because localtime() often cannot produce time stamps with years greater than 9999.
#ifdef __x86_64__
#define MAXTIME_T 253202544000
#else
#define MAXTIME_T ((time_t)(~HITIME_T))
#endif

#define SECONDSPERDAY (60*60*24)
#define DATEINSECONDS19990101 ((time_t) 915177600)

#define ANSI_BOLD_ON		"\33[1m"
#define ANSI_BOLD_OFF		"\33[22m"
#define ANSI_UNDERLINE_ON	"\33[4m"
#define ANSI_UNDERLINE_OFF	"\33[24m"
#define ANSI_TEXT_RED		"\33[31m"
#define ANSI_TEXT_GREEN		"\33[32m"
#define ANSI_NORMAL		"\33[m"

extern const char default_defoption[];
extern const char default_nondefoption[];

/*
======== 6) SHORT-HAND MACROS AND OLD-STYLE INLINE MACROS ========
 */

// Useful short-hands:
#define DIN (*din)
#define EOUT (*eout)
#define VOUT (*vout)
#define INFO (*info)
#define ERROR (*error)
#define WARN (*warn)

// NOTE: The following short-hand is only valid within UI_* class context
#define UIOUT (*uiout)

// NOTE: Old-style inline macros are deprecated! They should probably all be
// replaced with more appropriate functions or converted into inline functions.

#define DIL2AL_CONDITIONAL_ERROR_RETURN(_condition,_message) if (_condition) { ERROR << _message; return false; }

/*
======== 7) TYPES AND ENUMERATORS ========
 */

typedef unsigned long idnumber; // use 2 for 14+ digit DIL entry major ID numbers

typedef struct {
 	String file;
	String title;
} filetitle_t;

enum periodictask_t {pt_daily,pt_workdays,pt_weekly,pt_biweekly,pt_monthly,pt_endofmonthoffset,pt_yearly,OLD_pt_span,pt_nonperiodic};
// NOTE: The pt_span has been removed and replaced by OLD_pt_span, so that
// pt_nonperiodic retains its ordinal value, which was already heavily used
// and stored. The span is now taken into account for all periodic tasks,
// where a span of 0 means an unlimited number of cycles.

extern const char periodictaskchar[pt_nonperiodic+1];
extern const char periodictaskstr[pt_nonperiodic+1][16];

enum ui_joined_t { ui_joined_error, ui_joined_warn, ui_joined_info, ui_NOT_JOINED };

// Used by DIL_entry_content and beyond
#define PLAN_ENTRY_TYPE_UNASSIGNED -1
#define PLAN_ENTRY_TYPE_NONE 0
#define PLAN_ENTRY_TYPE_ACTION 1
#define PLAN_ENTRY_TYPE_GOAL 2
#define PLAN_ENTRY_TYPE_OBJECTIVE 3
#define PLAN_ENTRY_TYPE_OUTCOME 4
#define PLAN_ENTRY_TYPE_VARIABLE 5
#define PLAN_ENTRY_TYPE_SEEKPOSSIBLESOLUTIONS 6
#define PLAN_ENTRY_TYPE_PEERREVIEWSOLUTIONS 7
#define PLAN_ENTRY_TYPE_MILESTONEOBJECTIVE 8
#define PLAN_ENTRY_TYPE_PROBLEM 9
#define PLAN_ENTRY_TYPE_POSSIBLESOLUTION 10

// Used by DIL_entry_content
#define PLAN_OUTCOME_DESIRABILITY valuation
#define COMPLETION_STATE_OBSOLETE -1.0
#define COMPLETION_STATE_REPLACED -2.0
#define COMPLETION_STATE_DONE_DIFFERENTLY -3.0
#define COMPLETION_STATE_NO_LONGER_POSSIBLE -4.0
#define EXTRA_PARAMETERS_UNDETERMINED -1
#define EXTRA_PARAMETERS_NONE 0
#define EXTRA_PARAMETERS_AVAILABLE 1

// Used by DIL_AL_List (also known as DIL_Superiors)
//   A value of 0.0 for unspecified relevance is a reasonable
//   choice, as there is no purpose in specifying a connection
//   to a superior to which the dependency is of no relevance.
#define DILSUPS_REL_UNSPECIFIED	0.0
#define DILSUPS_UNB_UNSPECIFIED	0.0
#define DILSUPS_BND_UNSPECIFIED	0.0
#define DILSUPS_TD_UNSPECIFIED	-1
#define DILSUPS_URG_UNSPECIFIED	0.0
#define DILSUPS_PRI_UNSPECIFIED	0.0
//   The following TDPROP_ values were meant to be used as bit flags, but
//   due to the available space and ease of boolean manipulation and testing,
//   I am actually implementing the new property (exact) as a boolean.
#define DILSUPS_TDPROP_FIXED 1
#ifdef INCLUDE_EXACT_TARGET_DATES
  #define DILSUPS_TDPROP_EXACT 2
  #define DILSUPS_TDPROP_PERIODIC 4
#endif
#define PLAN_OUTCOME_LIKELIHOOD relevance

// Used by Calendar_Task
#define CALTASK_EMPTY '_'
#define CALTASK_EXACT 'e'
#define CALTASK_FIXED 'f'
#define CALTASK_VARIABLE 'v'
#define CALTASK_TD_EXACT 'E'
#define CALTASK_TD_FIXED 'F'
#define CALTASK_TD_VARIABLE 'V'
#define CALTASK_TD 'T'
#define CALTASK_INTERVAL '='

// Used by UI_ user interface classes
#define UI_CLASSIC 0
#define UI_YAD 1
#define UI_EXTERNAL 2

// Used to determine interactive and non-interactive responses
#define TCS_CURRENT	0
#define TCS_NEW	1
#define TCS_ASK	2

#define TSS_CURRENT	0
#define TSS_NEW	1
#define TSS_ASK	2

#define UNIE_NO	0
#define UNIE_YES	1
#define UNIE_ASK	2

#define ACW_TIME	0
#define ACW_TL		1
#define ACW_ASK	2

#define AAU_NO		0
#define AAU_YES	1
#define AAU_ASK	2

/*
======== 8) CLASSES AND EARLY DECLARATIONS ========
 */

// NOTE: Some non-member function declarations are included at specific places
// in this section, because those functions are called by inline member
// functions of classes where the inline member function is defined in the
// class declaration.
// *** This is actually rather ugly. Instead, inline member functions should
//     only be declared in class declarations, and their definitions should
//     follow all class declarations in the Inline Function Definitions
//     section.
// *** Also, in some of the older code, the inline statements were not
//     placed in (stylistically) best way possible. For example, there
//     are explicit inline statements in member function declarations.
//     It is best to go with official styles guides.

class Time {
protected:
  time_t t;
public:
  Time(): t(0) {}
  Time(time_t tinit) { if (tinit<0) tinit=time(NULL); t=tinit; }
  Time(const char * tlocal, bool withseconds = false) { local2caltime(tlocal,withseconds); }
  Time(int h, int m, int s) { t = (((((time_t) h)*60)+((time_t) m))*60)+((time_t) s); }
  void local2caltime(const char * tlocal, bool withseconds = false) { cerr << "Error: Time::local2caltime() is not yet implemented\n"; exit(1); } // time_stamp_time integrated with time_stamp_time_of_day
  String caltime2local(const char * dateformat); // time_stamp
  String caltime2GM(const char * dateformat) { cerr << "Error: Time::caltime2GM() is not yet implemented\n"; exit(1); return String(""); } // time_stamp_GM
  String caltime2interval(const char * dateformat) { return caltime2GM(dateformat); }
  time_t date(Time * tdate) { cerr << "Error: Time::date() is not yet implemented\n"; exit(1); return -1; } // time_stamp_time_date
};

class StringList {
protected:
  StringList * Next;
  String s;
public:
  ~StringList() { if (Next) delete Next; }
  StringList() { Next=NULL; }
  StringList(unsigned long n) { if (n) Next = new StringList(n-1); else Next = NULL; }
  StringList(const String cs, StringList * cn = NULL) { s=cs; Next=cn; }
  StringList(String cs, const char sep);
  StringList(String cs, const String sep, int csstart = 0);
  String & operator[](unsigned long n); // (utilties.cc)
  StringList * get_Next() { return Next; }
  String & get_Value() { return s; }
  StringList * List_Index(long n); // (utilities.cc)
  // append to n (without breaking link to n+1)
  void append(unsigned long n, String & ins); // (utilties.cc)
  // insert at n (n becomes n+1)
  void insert(unsigned long n, String & ins); // (utilties.cc)
  // count number of items following this one
  unsigned long length() { if (Next) return 1+Next->length(); else return 1; }
  // find in list
  long iselement(String el, unsigned long n = 0); // (utilities.cc)
  // find substring in list
  long contains(String substr, unsigned long n = 0); // (utilities.cc)
  // find firsrt in list that is substring of source string
  long first_contained_in(String & sourcestr, unsigned long n = 0); // (utilities.cc)
  // split a string into list elements
  unsigned long split(unsigned long n, String cs, const char sep = '\n');
  unsigned long split(unsigned long n, String & cs, const String sep = String("\n"), int csstart = 0);
  // concatenate list
  String concatenate(String sepstr = ""); // (utilities.cc)
};

// declared here so that it can be used in class descriptors below
int get_file_in_list(String & fname, StringList & srcf, StringList &srcfname, int & srcfnum);

class LongList {
#define LONGLIST_INITIAL_VALUE -1
protected:
  LongList * Next;
  long s;
public:
  ~LongList() { if (Next) delete Next; }
  LongList() { Next=NULL; s = LONGLIST_INITIAL_VALUE; }
  LongList(unsigned long n) { s = LONGLIST_INITIAL_VALUE; if (n) Next = new LongList(n-1); else Next = NULL; }
  LongList(const long cs, LongList * cn = NULL) { s=cs; Next=cn; }
  LongList(String cs, const char sep);
  long & operator[](unsigned long n); // (utilties.cc)
  LongList * get_Next() { return Next; }
  long & get_Value() { return s; }
  // append to n (without breaking link to n+1)
  void append(unsigned long n, long & ins); // (utilties.cc)
  // insert at n (n becomes n+1)
  void insert(unsigned long n, long & ins); // (utilties.cc)
  // count number of items following this one
  unsigned long length() { if (Next) return 1+Next->length(); else return 1; }
  // find in list
  long iselement(long el, unsigned long n = 0); // (utilities.cc)
  // find substring in list
  long contains(long substr, unsigned long n = 0); // (utilities.cc)
  // split a string into list elements
  unsigned long split(unsigned long n, String cs, const char sep = '\n');
  // concatenate list
  String concatenate(String sepstr = ""); // (utilities.cc)
};

// User Interface Handlers:
//
// NOTE: The same class can be used for Warnings, Errors and Alerts by
// creating an instance for separate pointers (info, warn, error, alert)
// and by giving them a few different characteristics, such as different
// icons (info, error,
// /usr/share/icons/elementary/status/48/error.svg,
// /usr/share/icons/elementary/emblems/48/emblem-danger.svg).

// Handles informative output
class UI_Info {
protected:
  bool usetimeout;
public:
  UI_Info(bool _usetimeout): usetimeout(_usetimeout) {}
  virtual ~UI_Info() {}
  virtual bool write(const char * s) = 0;
  // NOTE: There is no need here for separate definitions of write(String s), etc.
  // The same result can be achieved (probably with less string copying) by using
  // calls such as:
  //   String s("something"); INFO.write(s.chars());
  //   String s("something"); INFO.write(String("this plus "+s).chars());
  //   INFO.write(String(cin).chars());
  //   INFO.write(String((long) 12345).chars());
  //   INFO.write(String(0.25,"%.2").chars());
  virtual UI_Info& operator<<(const char * s) = 0;
  virtual UI_Info& operator<<(char c) = 0;
  virtual UI_Info& operator<<(const String & s) = 0;
  virtual UI_Info& operator<<(const SubString & s) = 0;
  virtual UI_Info& operator<<(int l) = 0;
  virtual UI_Info& operator<<(long l) = 0;
  virtual UI_Info& operator<<(double d) = 0;
  // *** Not needed yet: & operator<<(const istream & is) = 0;
  virtual void flush() = 0;
};

//UI_Info & flush(UI_Info & uio); // interface.cc

// Classic informative output goes to VOUT
// NOTE: The usetimeout flag is ignored in this derived class.
class UI_Info_Classic: public UI_Info {
protected:
  ostream * uiout; // a stream output such as &cout, &vout, &cerr, &eout, or even an output file
public:
  UI_Info_Classic(ostream * _uiout, bool _usetimeout): UI_Info(_usetimeout), uiout(_uiout) { if (!uiout) uiout=&cout; }
  virtual ~UI_Info_Classic() { flush(); }
  virtual bool write(const char * s);
  virtual UI_Info& operator<<(const char * s);
  virtual UI_Info& operator<<(char c);
  virtual UI_Info& operator<<(const String & s);
  virtual UI_Info& operator<<(const SubString & s);
  virtual UI_Info& operator<<(int l);
  virtual UI_Info& operator<<(long l);
  virtual UI_Info& operator<<(double d);
  // *** Not needed yet: & operator<<(const istream & is);
  virtual void flush();
  //friend UI_Info_Classic & flush(UI_Info_Classic & uio);
};

// Informative output is shown in a yad window
class UI_Info_Yad: public UI_Info {
protected:
  String infostr;
  String ui_icon; // path to an image file or name of icon in current active icon set
  ui_joined_t joined;
public:
  UI_Info_Yad(const char * _ui_icon, bool _usetimeout, ui_joined_t _joined = ui_NOT_JOINED): UI_Info(_usetimeout) {
    if (_ui_icon) ui_icon=_ui_icon; else ui_icon="info";
    Join(_joined);
  }
  virtual ~UI_Info_Yad() { flush(); }
  void Join(ui_joined_t _joined); // inline, see Inline Member Functions section below
  virtual bool write(const char * s);
  virtual UI_Info& operator<<(const char * s);
  virtual UI_Info& operator<<(char c);
  virtual UI_Info& operator<<(const String & s);
  virtual UI_Info& operator<<(const SubString & s);
  virtual UI_Info& operator<<(int l);
  virtual UI_Info& operator<<(long l);
  virtual UI_Info& operator<<(double d);
  // *** Not needed yet: & operator<<(const istream & is);
  virtual void flush();
  //friend UI_Info_Classic & flush(UI_Info_Classic & uio);

  friend class UI_Joined_Yad;
};

// A means to unify separate process UI_*Yad output in a single window
class UI_Joined_Yad {
protected:
  UI_Info_Yad * joined[ui_NOT_JOINED];
public:
  UI_Joined_Yad() { for (int i = 0; i<ui_NOT_JOINED; i++) joined[i]=NULL; }
  void Join(ui_joined_t join, UI_Info_Yad & ui_info_yad) { joined[join]=&ui_info_yad; }
#ifdef UI_JOINED_YAD_NOTEBOOK
  int build_joined_yad_flush_script(); // interface.cc
#endif
  void flush(); // interface.cc
};

// Informative output is shown via external command
// NOTE: An explicit built-in method for joining multiple UI_*External outputs
// is not needed, because this can be achieved externally through the commands
// that are attached.
class UI_Info_External: public UI_Info {
protected:
  String infostr;
  String infocmd;
public:
  UI_Info_External(String _infocmd, bool _usetimeout): UI_Info(_usetimeout), infocmd(_infocmd) {}
  virtual ~UI_Info_External() { flush(); }
  virtual bool write(const char * s);
  virtual UI_Info& operator<<(const char * s);
  virtual UI_Info& operator<<(char c);
  virtual UI_Info& operator<<(const String & s);
  virtual UI_Info& operator<<(const SubString & s);
  virtual UI_Info& operator<<(int l);
  virtual UI_Info& operator<<(long l);
  virtual UI_Info& operator<<(double d);
  // *** Not needed yet: & operator<<(const istream & is);
  virtual void flush();
};

// Handles interactive string entry
class UI_Entry {
public:
  UI_Entry() {}
  virtual ~UI_Entry() {}
  virtual String request(const char * message, bool allowcancel = true) = 0;
};

class UI_Entry_Classic: public UI_Entry {
protected:
  ostream * uiout; // a stream output such as &cout, &vout, &cerr, &eout, or even an output file
public:
  UI_Entry_Classic(ostream * _uiout): UI_Entry(), uiout(_uiout) { if (!uiout) uiout=&cout; }
  virtual ~UI_Entry_Classic() {}
  virtual String request(const char * message, bool allowcancel = true); // interfaces.cc
};

class UI_Entry_Yad: public UI_Entry {
public:
  UI_Entry_Yad(): UI_Entry() {}
  virtual ~UI_Entry_Yad() {}
  virtual String request(const char * message, bool allowcancel = true); // interfaces.cc
};

class UI_Entry_External: public UI_Entry {
protected:
  String entrycmd;
public:
  UI_Entry_External(String _entrycmd): UI_Entry(), entrycmd(_entrycmd) {}
  virtual ~UI_Entry_External() {}
  virtual String request(const char * message, bool allowcancel = true); // interfaces.cc
};

// Handles interactive confirmation
class UI_Confirm {
public:
  UI_Confirm() {}
  virtual ~UI_Confirm() {}
  virtual int request(const char * message, char nondefault, const char * defoption = default_defoption, const char * nondefoption = default_nondefoption, bool allowcancel = true) = 0;
  virtual char multi_request(const char * message, String respchars, StringList & respdescriptors) = 0;
};

class UI_Confirm_Classic: public UI_Confirm {
protected:
  ostream * uiout; // a stream output such as &cout, &vout, &cerr, &eout, or even an output file
public:
  UI_Confirm_Classic(ostream * _uiout): UI_Confirm(), uiout(_uiout) { if (!uiout) uiout=&cout; }
  virtual ~UI_Confirm_Classic() {}
  virtual int request(const char * message, char nondefault = 'n', const char * defoption = default_defoption, const char * nondefoption = default_nondefoption, bool allowcancel = true); // interfaces.cc
  virtual char multi_request(const char * message, String respchars, StringList & respdescriptors); // interfaces.cc
};

class UI_Confirm_Yad: public UI_Confirm {
public:
  UI_Confirm_Yad(): UI_Confirm() {}
  virtual ~UI_Confirm_Yad() {}
  virtual int request(const char * message, char nondefault = 'n', const char * defoption = default_defoption, const char * nondefoption = default_nondefoption, bool allowcancel = true); // interfaces.cc
  virtual char multi_request(const char * message, String respchars, StringList & respdescriptors); // interfaces.cc
};

class UI_Confirm_External: public UI_Confirm {
protected:
  String confirmcmd;
public:
  UI_Confirm_External(String _confirmcmd): UI_Confirm(), confirmcmd(_confirmcmd) {}
  virtual ~UI_Confirm_External() {}
  virtual int request(const char * message, char nondefault = 'n', const char * defoption = default_defoption, const char * nondefoption = default_nondefoption, bool allowcancel = true); // interfaces.cc
  virtual char multi_request(const char * message, String respchars, StringList & respdescriptors); // interfaces.cc
};

struct UI_Options_RQData {
  // A useful container for UI_Options::request() data.
  const char * message = NULL; // The message that explains what is being requested.
  const char * entrymsg = NULL; // The entry box message.
  StringList & options; // The main list of enumerated options.
  int numoptions = 0; // The number of enumerated options.
  const char * defaultsel = NULL; // Optionally, a choice identifier for the default option.
  const char * defaultopt = NULL; // Optionally, the default option.
  StringList * specialsel = NULL; // Optionally, a list of special choice identifiers.
  StringList * specialopt = NULL; // Optionally, a list of special option descriptors.
  int numspecial = 0; // The number of special options.
  const char * image = NULL; // Optional image path.
};

// Handles option selection
class UI_Options {
public:
  UI_Options() {}
  virtual ~UI_Options() {}
  virtual String request(UI_Options_RQData & opt) = 0;
};

class UI_Options_Classic: public UI_Options {
protected:
  ostream * uiout; // a stream output such as &cout, &vout, &cerr, &eout, or even an output file
public:
  UI_Options_Classic(ostream * _uiout): UI_Options(), uiout(_uiout) { if (!uiout) uiout=&cout; }
  virtual ~UI_Options_Classic() {}
  virtual String request(UI_Options_RQData & opt); // interfaces.cc
};

class UI_Options_Yad: public UI_Options {
public:
  UI_Options_Yad(): UI_Options() {}
  virtual ~UI_Options_Yad() {}
  virtual String request(UI_Options_RQData & opt); // interfaces.cc
};

class UI_Options_External: public UI_Options {
protected:
  String optionscmd;
public:
  UI_Options_External(String _optionscmd): UI_Options(), optionscmd(_optionscmd) {}
  virtual ~UI_Options_External() {}
  virtual String request(UI_Options_RQData & opt); // interfaces.cc
};

// Handles progress indicators
class UI_Progress {
protected:
  long last_percentage; // This must be reset to -1 by start().
public:
  UI_Progress(): last_percentage(-1) {}
  virtual ~UI_Progress() {}
  virtual bool start() = 0; // interface.cc
  virtual bool update(long percentage, const char * progressstr = NULL) = 0; // interface.cc
  virtual int stop() = 0; // interface.cc
};

class UI_Progress_Classic: public UI_Progress {
protected:
  ostream * uiout; // a stream output such as &cout, &vout, &cerr, &eout, or even an output file
  bool active;
public:
  UI_Progress_Classic(ostream * _uiout): UI_Progress(), uiout(_uiout), active(false) { if (!uiout) uiout=&cout; }
  virtual ~UI_Progress_Classic() { stop(); }
  virtual bool start(); // interface.cc
  virtual bool update(long percentage, const char * progressstr = NULL); // interface.cc
  virtual int stop(); // interface.cc
};

class UI_Progress_Yad: public UI_Progress {
protected:
  FILE *fp; // NULL means that no pipe is currently open
public:
  UI_Progress_Yad(): UI_Progress(), fp(NULL) {}
  virtual ~UI_Progress_Yad() { stop(); }
  virtual bool start(); // interface.cc
  virtual bool update(long percentage, const char * progressstr = NULL); // interface.cc
  virtual int stop(); // interface.cc
};

class UI_Progress_External: public UI_Progress {
protected:
  String optionscmd;
public:
  UI_Progress_External(String _optionscmd): UI_Progress(), optionscmd(_optionscmd) {}
  virtual ~UI_Progress_External() { stop(); }
  virtual bool start(); // interface.cc
  virtual bool update(long percentage, const char * progressstr = NULL); // interface.cc
  virtual int stop(); // interface.cc
};

// The recursive version of the Quick_String_Hash::read() function can be slightly
// faster (may be machine, compiler and compilation dependent, e.g. a compiler may
// optimize one of the functions better), but will consume a bit more stack space
// during operation as a recursion is made for each character in a string.
#define RECURSIVE_QUICK_STRING_HASH
// CONST_QUICK_STRING_HASH_ENCODE can achieve a memory savings of maximally 1 node per
// hashed string. Often, less is saved, since some strings may be subsets of other
// strings.
// Beware: For some odd reason, using this compile-time option can cause segmentation
// faults when inserting to a dynamically allocated Quick_String_Hash object, e.g.
// Quick_String_Hash * qsh = new Quick_String_Hash(); qsh->insert(sl[i].chars());
// #define CONST_QUICK_STRING_HASH_ENDNODE
// *** There is an even more memory efficient implementation where a conversion
//     table is maintained. Only letters that occur receive a conversion value
//     below 255. There are then fewer and shorter empty spans in allocated lists.
// #define CONVERSION_TABLE_QUICK_STRING_HASH

class Quick_String_Hash {
	// The quick allocate version of this class immediately allocates all 256 elements and
	// sets the offset to 0.
	// Note that this does not use an actual hash algorithm. It is instead a tree generator,
	// intended only to allocate and test the allocation of hash key strings.
	protected:
		Quick_String_Hash ** list; // pointer offset from allocated list
		int offset; // 1 means list starts at 1 instead of 0
		int len; // 0 = none allocated
		Quick_String_Hash * add_list_element(int i);
	public:
		// constructor
		Quick_String_Hash(): list(NULL), offset(0), len(0) {}
		// destructor
		~Quick_String_Hash() { if (list) { for (int i=0; i<len; i++) delete list[i]; delete[] list; } }
		// functions
		Quick_String_Hash & insert(const char * s);
		Quick_String_Hash & operator<<(const char * s) { return insert(s); }
		int read(const char * s) const;
		int operator[](const char * s) const { return read(s); }
		unsigned long nodes() const;
		unsigned long size() const;
};
#ifdef CONST_QUICK_STRING_HASH_ENDNODE
extern const Quick_String_Hash _qsh_endnode;
#endif

class Text_File_Buffers {
public:
  // constructor
  Text_File_Buffers() { tfnum=0; }
  // parameters
  StringList tf; // text file buffers
  StringList tfname; // text file names
  int tfnum; // number of buffers in use
  // functions
  int FileIndex(String fname) { return get_file_in_list(fname,tf,tfname,tfnum); }
  // was (pre 20190129): int FileIndex(String fname) { for (int i=0; i<tfnum; i++) if (tfname[i]==fname) return i; return get_file_in_list(fname,tf,tfname,tfnum); }
};

class Directory_Access {
	protected:
		String dirpath;
	public:
		// constructor
		Directory_Access(String dp): dirpath(dp) {}
		// functions
		int read(String & filenames, char separator = '/'); // reads the directory into a String (see utilities.cc)
};

/*
	<A NAME="NOVELTY-marker">
	NOVELTY marker and content range tags:
	</A>

		Marker begin
			<A NAME="NOVELTY-some-id">[<B>N</B>]</A>
		when the item has been included in a paper draft this becomes
			<A NAME="NOVELTY-some-id" HREF="nearest-anchor-in-paper">[<B>N</B>]</A>

		Marker end
			<!-- @NOVELTY-some-id@ -->
		when all qualifying and quantifying information is known this becomes
			<!-- @NOVELTY-some-id@ @CONTEXT: cid1,...,cidn@ @IMP: X1.x1,...,Xn.xn [0,inf]@ @LEN: X.x [0,inf]@ @SECTION: sec1,...,secm@ -->

		Dereferenced marker begin
			<A NAME="NOVELTYREF-some-id" HREF="document-containing-NOVELTY-tags">

		Dereferenced marker end
			</A><!-- @NOVELTYREF-some-id@  -->

		Content range begin
			<!-- @Begin NOVELTY-some-id@ -->

		Content range end
			<!-- @End NOVELTY-some-id@ -->

	(extracted) NOVELTY item in Topical Context Hierarchy:
		e.g.
		(Context: JLI model definition, Importance: 2.0, Length: 0.5, Section: INT)
			 
*/

class Novelty_Marker {
	public:
		Novelty_Marker(int hdepth = 0);	// (utilties.cc)
		~Novelty_Marker() { if (imp) delete[] imp; }

		String	id;		// ID in <A NAME="NOVELTY-some-id">
		String	source;	// file containing NOVELTY item marker
		String	marktext;	// text between marker begin and end
		String	inclurl;	// HREF="nearest-anchor-in-paper"

		StringList	context;	// @CONTEXT: cid1,...,cidn@
		float *		imp;		// @IMP: X1.x1,...,Xn.xn [0,inf]@
		int			impnum;	// number of Importance values, 1 or number of Context titles
		float		len;		// @LEN: X.x [0,inf]@
		StringList	section;	// @SECTION: sec1,...,secm@

		int		hierdepth;	// depth in the Topical Context Hierarchy		
		enum		nmtype_t { NM_NOVELTY_ITEM, NM_TCH_DEPTH_INC, NM_TCH_DEPTH_DEC, NM_STANDARD_ITEM, NM_PREVPAPER_ITEM, NM_REF_CITATION, NM_REQUIRED_ITEM, NM_CONTENT_HREF };
		/*	0 = Novelty item
		1 = Topical Context hierarchy depth increase
		2 = Topical Context hierarchy depth decrease
		3 = Standard item
		4 = Previous Paper included Novelty item
		5 = Reference citation
		6 = Required item */
		nmtype_t	nmtype;	// type of item

		int Get_Novelty_from_Paper_Plan(String & pptext, int startp, int starthierdepth = 0);
};

class Novelty_Marker_List {
	protected:
		Novelty_Marker_List * Next;
		Novelty_Marker s;
	public:
		~Novelty_Marker_List() { if (Next) delete Next; }
		Novelty_Marker_List() { Next=NULL; }
		Novelty_Marker_List(unsigned long n) { if (n) Next = new Novelty_Marker_List(n-1); else Next = NULL; }
		Novelty_Marker_List(const Novelty_Marker cs, Novelty_Marker_List * cn = NULL) { s=cs; Next=cn; }
		Novelty_Marker & operator[](unsigned long n); // (utilties.cc)
		Novelty_Marker_List * get_Next() { return Next; }
		// append to n (without breaking link to n+1)
		void append(unsigned long n, Novelty_Marker & ins); // (utilties.cc)
		// insert at n (n becomes n+1)
		void insert(unsigned long n, Novelty_Marker & ins); // (utilties.cc)
		// count number of items following this one
		unsigned long length() { if (Next) return 1+Next->length(); else return 1; }
		// find in list
		long iselement(Novelty_Marker & el, unsigned long n = 0); // (utilities.cc)
		// get estimated significance ranks for concatenated contexts
		void Get_Context_Ranks(StringList & cc, LongList & ccr); // (utilities.cc)
};

// Class used to load, maintain and update a list of Topical
// DIL files.

class Topical_DIL_Files: public PLLHandle<Topical_DIL_Files> {
	protected:
		filetitle_t ft; // file and title
		bool updated;
		void Set_File(String file, int n = 0); // (diladmin.cc)
		void Set_Title(String title, int n = 0); // (diladmin.cc)
	public:
		// constructor
		Topical_DIL_Files(): updated(false) {}
		// destructor (see http://www.geocrawler.com/archives/3/361/1998/1/0/2004286/)
		~Topical_DIL_Files() {} 
		// functions
		inline String * operator[](int n); // (diladmin.cc)
		inline String * File(int n = 0); // (diladmin.cc)
		inline String * Title(int n = 0); // (diladmin.cc)
		inline filetitle_t * Topic(int n = 0); // (diladmin.cc)
		inline void Invalidate(); // (diladmin.cc)
		int Refresh(); // (diladmin.cc)
};

class Topical_DIL_Files_Root {
// used to anchor the first element that is created by default
// and to access all elements in the list
// Note: the first element is created by default (while lists normally
// begin empty), since Refresh() is called in it.
// *** an alternative is to move the actual implementation of most
//     member functions to this Root object and to use a static
//     member function to refresh and create the list
	public:
		// constructor
		Topical_DIL_Files_Root() { Topical_DIL_Files * tdf = new Topical_DIL_Files(); dil.link_before(tdf); }
		// parameters
		PLLRoot<Topical_DIL_Files> dil;
		// functions
		inline String * operator[](int n) { if (dil.head()) return (*dil.head())[n]; return NULL; }
		inline String * File(int n = 0) { if (dil.head()) return dil.head()->File(n); return NULL; }
		inline String * Title(int n = 0) { if (dil.head()) return dil.head()->Title(n); return NULL; }
		inline filetitle_t * Topic(int n = 0) { if (dil.head()) return dil.head()->Topic(n); return NULL; }
		void Invalidate() { if (dil.head()) dil.head()->Invalidate(); }
		int Refresh() { if (dil.head()) return dil.head()->Refresh(); return 0; }
};

// Classes used to load, store and manipulate DIL entries
// and their parameters, as obtained from DIL by ID and
// topical DIL files.

class DIL_entry;
class DIL_entry_Pointer;

// DIL ID storage, operators and conversion
class DIL_ID {
protected:
  void init(String & idstr) {
    String idmajorstr(idstr.before('.'));
    idmajor[0] = (idnumber) atol((const char *) String(idmajorstr.at(0,idmajorstr.length()-6)));
    idmajor[1] = (idnumber) atol((const char *) String(idmajorstr.from(((int) idmajorstr.length())-6)));
    idminor = (unsigned int) atoi((const char *) String(idstr.after('.')));
  }
  //		mutable String idstr; (see utilities.cc)
public:
  // constructors
  DIL_ID(): idminor(0) { idmajor[0]=0; idmajor[1]=0; }
  DIL_ID(idnumber idmaj0, idnumber idmaj1, unsigned int idmin): idminor(idmin) { idmajor[0]=idmaj0; idmajor[1]=idmaj1; }
  DIL_ID(const DIL_ID & id): idminor(id.idminor) { idmajor[0]=id.idmajor[0]; idmajor[1]=id.idmajor[1]; }
  DIL_ID(String & idstr) { init(idstr); }
  DIL_ID(const SubString & idstr) { String s(idstr); init(s); }
  // parameters
  // major ID numbers are usually composed of a concatenation
  // of year, month, day, hour, minute, second
  // minor ID numbers distinguish DIL entries with
  // identical major ID numbers
  idnumber idmajor[2]; // major ID number (assumed 14+ digits)
  unsigned int idminor; // minor ID number
  // DIL_ID,DIL_ID operators
  DIL_ID & operator=(const DIL_ID & id) { idmajor[0]=id.idmajor[0]; idmajor[1]=id.idmajor[1]; idminor=id.idminor; return *this; }
  bool operator==(const DIL_ID & id2) const { return ((idmajor[0]==id2.idmajor[0]) && (idmajor[1]==id2.idmajor[1]) && (idminor==id2.idminor)); }
  bool operator!=(const DIL_ID & id2) const { return (!(*this==id2)); }
  bool operator<(const DIL_ID & id2) const { return ((idmajor[0]<id2.idmajor[0]) || ((idmajor[0]==id2.idmajor[0]) && (idmajor[1]<id2.idmajor[1])) || ((idmajor[0]==id2.idmajor[0]) && (idmajor[1]==id2.idmajor[1]) && (idminor<id2.idminor))); }
  bool operator<=(const DIL_ID & id2) const { return ((*this<id2) || (*this==id2)); }
  bool operator>(const DIL_ID & id2) const { return (id2<*this); }
  bool operator>=(const DIL_ID & id2) const { return (id2<=*this); }
  // DIL_ID,String operators
  DIL_ID & operator=(String & idstr) { init(idstr); return *this; }
  DIL_ID & operator=(String * idstr) { init(*idstr); return *this; }
  String str() const;
  // Beware that the returned pointer is only valid
  // until the next call to this function and as long
  // as the corresponding DIL_ID object exists. If
  // it is not desirable that pointers are invalidated
  // by calls to this function from other DIL_ID objects
  // then idstr must be included as a mutable variable
  // of each DIL_ID object.
  // Note that the static variable (or object variable)
  // is used so that the returned pointer points to an
  // existing character string.
  inline const char* chars() const { 
    /*cout << "CHECKING: ";
      for (char * tp = (char *) str().chars(); *tp!='\0'; tp++) cout << (*tp) << '[' << (int) (*tp) << ']';
      cout << '\n';*/
    /*String idstr(str());
      cout << "idstr in chars() function: ";
      int i = 0;
      for (char * tp = (char *) idstr.chars(); (*tp!='\0') && (i<80); tp++, i++) cout << (*tp) << '{' << (int) (*tp) << '}';
      cout << '\n';*/
    static String idstr; // need static here to avoid trouble with life-time of object that chars() return points to
    idstr = str();
    return idstr.chars(); }
  operator const char*() const { return chars(); }
  // functions
  bool valid() { return ((idmajor[0]!=0) && (idmajor[1]!=0) && (idminor!=0)); }
  bool Write_DIL_ID_to_Binary_Cache(ofstream & cfl); // (see utilities.cc)
  bool Read_DIL_ID_from_Binary_Cache(ifstream & cfl); // (see utilities.cc)
};

class Plan_entry_content {
protected:
  DIL_entry * entry;
  int planentrytype;
  int decision;
  DIL_ID decisionID;
  DIL_entry * decisionptr;
public:
  Plan_entry_content(DIL_entry & e, int pet, int parsefrom = -1): entry(&e), planentrytype(pet), decision(-1), decisionID(), decisionptr(NULL) { if (parsefrom>=0) Parse_Entry_Content(parsefrom); }
  int get_decision() { return decision; }
  DIL_ID & Decision() { return decisionID; }
  inline DIL_entry * Decision_Ptr(); // (see below)
  void Parse_Entry_Content(int parsefrom = 0); // (see utilities.cc)
};

class Extra_DIL_entry_parameters {
  // This class can be used to access extra parameters stored in the text
  // content of a DIL entry. It is initialized by a call to
  // DIL_entry_content::Has_Extra_Parameters().
  // Examples of extra parameters are: information about periodic rescheduling
  // for daily, weekly, monthly DIL entries; information about whether an
  // incomplete entry expires if its next periodic target date comes up;
  // a next periodic target date; etc.
protected:
  DIL_entry * entry;
public:
  Extra_DIL_entry_parameters(DIL_entry & e, int parsefrom = -1): entry(&e) {}
};

// DIL entry content
class DIL_entry_content {
protected:
  DIL_entry * entry;
  int planentrytype;
  int hasextraparameters;
  Plan_entry_content * plancontent;
  Extra_DIL_entry_parameters * extraparameters;
public:
  // destructor
  ~DIL_entry_content() { delete text; delete plancontent; }
  // constructors
  DIL_entry_content(DIL_entry & e);
  // parameters
  // Note that the current target resolution for
  // time specifications is minutes.
  filetitle_t TL_tail; // reference to tail of TL entries for this task (title=reference NAME)
  filetitle_t TL_head; // reference to head of TL entries for this task (title=reference NAME)
  time_t started; // time started on detail item tasks
  // note: can remove/repurpose this, since the tail
  // link to a TL entry indicates this
  time_t required; // time required for detail item tasks (seconds)
  float completion; // completion ratio (also see special values defined above)
  float valuation; // value (compared to 1.0)
  String * text; // detail item content
  void set_TL_tail_reference(String tailurl);
  void set_TL_head_reference(String headurl);
  int Is_Plan_Entry(); // (see utilities.cc)
  int Has_Extra_Parameters(); // (see utilities.cc)
  Plan_entry_content * Get_Plan_Entry_Content() { return plancontent; }
  bool Write_Topical_to_Binary_Cache(ofstream & cfl, bool gottext);
  bool Read_Topical_from_Binary_Cache(ifstream & cfl, bool gottext);
  bool Binary_Cache_Diagnostic(DIL_entry_content * cached, int & testedbytes);
};

// Topical DIL in which DIL entry appears
class DIL_Topical_List: public PLLHandle<DIL_Topical_List> {
#define DILTOPICS_REL_UNSPECIFIED	0.0
protected:
  DIL_entry * entry;
public:
  // constructors
  DIL_Topical_List(DIL_entry & e); // (see utilities.cc)
  DIL_Topical_List(DIL_entry & e, String tfile, String ttitle, float trelevance); // (see utilities.cc)
  // destructor (see http://www.geocrawler.com/archives/3/361/1998/1/0/2004286/)
  ~DIL_Topical_List() {} 
  // parameters
  filetitle_t dil; // DIL file (absolute URL without #ID) and DIL file title
  float relevance; // relevance to DIL topic
  bool Write_to_Binary_Cache(ofstream & cfl);
  bool Read_from_Binary_Cache(ifstream & cfl);
  bool Binary_Cache_Diagnostic(DIL_Topical_List * cached, int & testedbytes);
};

// Project AL in which DIL entry appears
// <A NAME="DIL_AL_List"></A>
// This class describes one element (specified in variable 'al')
// in a list of Superiors of a DIL_entry (specified by 'entry').
// *** As the dependency hierarchy is implemented, this will
//     provide a linked list to superior DIL entries
//     Note (20010422): The parameter "superior" below provides
//     some of these facilities now, and is used as a cache for
//     Superiorbyid(), but perhaps a further enhancement can still
//     be made to offer additional benefits.
class DIL_AL_List: public PLLHandle<DIL_AL_List> {
#define DIL_Superiors DIL_AL_List
protected:
  DIL_entry * entry;
  DIL_entry * superior; // caches information returned by Superiorbyid()
  time_t _targetdate; // target date (in calendar time) for completion (values < 0 should be interpreted as MAXTIME_T, as they are in DIL_entry::Propagated_Target_Date() and DIL_entry::Propagated_Target_Date_Origin())
  int _tdproperty; // target date properties (e.g. TDPROP_FIXED)
#ifdef INCLUDE_EXACT_TARGET_DATES
  bool _tdexact; // target date is exact (i.e. indicates actual time of task completion, no distribution of task chunks possible)
  periodictask_t _tdperiod; // period length for tasks with the DILSUPS_TDPROP_PERIODIC property
  int  _tdspan; // span in numbers of cycles for periodic tasks (the old span 's' type is translated as daily with limited span)
  int _tdevery; // a number greater than 1 means skip multiples of _tdperiod between
#endif
public:
  // constructors
  DIL_AL_List(DIL_entry & e); // (see utilities.cc)
  DIL_AL_List(DIL_entry & e, String pfile, String ptitle, float prelevance, float punbounded, float pbounded, time_t ptargetdate, int ptdprop,
#ifdef INCLUDE_EXACT_TARGET_DATES
	      bool ptdexact, periodictask_t ptdperiod, int ptdspan, int ptdevery,
#endif
	      float purgency, float ppriority); // (see utilities.cc)
  // destructor (see http://www.geocrawler.com/archives/3/361/1998/1/0/2004286/)
  ~DIL_AL_List() {} 
  // parameters
  filetitle_t al; // AL file (absolute URL with #ID) and title (#ID)
  // Note: By default, if al.title is not a DIL ID, then
  // it is assumed to be a self-reference.
  float relevance; // relevance to project / dependence of superior (see DIL#20001006114606.1)
        // if target entry is OUTCOME this parameter is the "likelihood"
  float unbounded; // unbounded importance
  float bounded; // bounded importance
  float urgency; // computed urgency
  float priority; // computed priority
  // *** change these functions if a linked list is provided
  inline DIL_entry * Superiorbyid(); // returns superior DIL entry if found, otherwise returns "*entry"
  inline bool IsSelfReference();
  time_t targetdate() { return _targetdate; }
  int tdproperty() { return _tdproperty; }
  bool tdfixed() { return (_tdproperty & DILSUPS_TDPROP_FIXED); }
#ifdef INCLUDE_EXACT_TARGET_DATES
  bool tdexact() { return _tdexact; }
  void set_tdexact(bool tdex) { _tdexact = tdex;  }
  periodictask_t tdperiod() { return _tdperiod; }
  void set_tdperiod(periodictask_t period); // (see utilities.cc)
  int  tdspan() { return _tdspan; }
  int  tdevery() { return _tdevery; }
  void set_tdspan(int span) { _tdspan = span; }
  void set_tdevery(int every) { _tdevery = every; }
#endif
  bool set_targetdate(time_t td) {
#ifdef INCLUDE_EXACT_TARGET_DATES
    if (_tdproperty & DILSUPS_TDPROP_FIXED) return false;
#else
    if (_tdproperty==DILSUPS_TDPROP_FIXED) return false;
#endif
    _targetdate = td; return true; }
  void force_targetdate(time_t td) { // Warning: Use this only to update periodic tasks!
    _targetdate = td;
  }
  void set_tdproperty(int tdprop) { _tdproperty=tdprop; }
  bool Write_to_Binary_Cache(ofstream & cfl);
  bool Read_from_Binary_Cache(ifstream & cfl);
  bool Binary_Cache_Diagnostic(DIL_AL_List * cached, int & testedbytes);
};

/*
	<A NAME="DIL-Parameter-Entry">
	DIL by ID Parameter Entry File FOrmat (Mandatory Elements):
	</A>
	
	Entries List Begin Marker
		<!-- dil2al: DIL ID begin -->
	Entry Data Begin Marker
		<TR>
	Entry Data Lines
		<TD><A NAME="-id-number-">-id-number-</A>\n
		<TD><A HREF="-topical-DIL-reference-">-topic-title-</A> (-relevance-), <A HREF="-topical-DIL-reference-">-topic-title-</A> (-relevance-), -etc.-\n
		<TD><A HREF="-AL-reference-">-AL-title-</A> (-relevance-,-unb.imp.-,-bound.imp.-,-target-date-,-urgency-,-priority-), -etc.-\n
	*** As the dependency hierarchy is implemented, replace the
	    AL references with superior DIL entry references
*/

// DIL list parameters
//   1) topics (=topical DIL files & specific parameters)
//   2) projects (= superiors & superior specific parameters)
// Notes:
// 1. The topics information does not include the referenced content
// in the topical DIL files. That is found through the DIL_entry_content
// connected from the DIL_entry.
// 2. The list of DIL_AL_List elements connected to from the 'projects'
// root variable is the list of Superiors of a DIL_entry ('entry'). It
// is called 'projects', because historically these Superiors were
// thought to involve the DIL_entry in any of a number of Projects,
// each with their own Active_List (AL). Presently (20130916) we use
// only one overall AL and all DIL entries belong to one large
// hierarchy. This variable could be renamed 'superiors'.
// 3. The list of DIL_Topical_List elements connected to from the 'topics'
// root variable is the list of Topical files (DIL files) in which this
// 'entry' appears. Historically, it seemed logical that an entry could
// relate to many Topics, but keeping actual copies in multiple files was
// unnecessary and less practical. Now, DIL entries tend to be described
// in only one DIL file, while they may connect to many logical topics
// through their Superiors in the hierarchy. Mostly, DIL entries therefore
// have 'topics' lists with only 1 element.
class DIL_entry_parameters {
protected:
  DIL_entry * entry;
  int origloc; // original location of DIL entry when read from DIL-by-ID file
public:
  // constructors
  DIL_entry_parameters(DIL_entry & e, int loc = -1): entry(&e), origloc(loc) {}
  // parameters
  PLLRoot<DIL_Topical_List> topics;
  PLLRoot<DIL_AL_List> projects;
  // functions
  bool topic_append(String tfile, String ttitle, float trelevance) { return topics.link_before(new DIL_Topical_List(*entry,tfile,ttitle,trelevance)); }
  bool topic_prepend(String tfile, String ttitle, float trelevance) { return topics.link_after(new DIL_Topical_List(*entry,tfile,ttitle,trelevance)); }
  bool project_append(String pfile, String ptitle, float prelevance, float punbounded, float pbounded, time_t ptargetdate, int ptdprop,
#ifdef INCLUDE_EXACT_TARGET_DATES
		      bool ptdexact, periodictask_t ptdperiod, int ptdspan, int ptdevery,
#endif
		      float purgency, float ppriority) {
    return projects.link_before(new DIL_AL_List(*entry,pfile,ptitle,prelevance,punbounded,pbounded,ptargetdate,ptdprop,
#ifdef INCLUDE_EXACT_TARGET_DATES
						ptdexact, ptdperiod, ptdspan, ptdevery,
#endif
						purgency,ppriority));
  }
  bool project_prepend(String pfile, String ptitle, float prelevance, float punbounded, float pbounded, time_t ptargetdate, int ptdprop,
#ifdef INCLUDE_EXACT_TARGET_DATES
		       bool ptdexact, periodictask_t ptdperiod, int ptdspan, int ptdevery,
#endif
		       float purgency, float ppriority) {
    return projects.link_after(new DIL_AL_List(*entry,pfile,ptitle,prelevance,punbounded,pbounded,ptargetdate,ptdprop,
#ifdef INCLUDE_EXACT_TARGET_DATES
					       ptdexact, ptdperiod, ptdspan, ptdevery,
#endif
					       purgency,ppriority));
  }
  int OrigLoc() { return origloc; }
  DIL_entry * DE() { return entry; }
  // The two functions below report if AT LEAST ONE target date is exact, period, or fixed.
#ifdef INCLUDE_EXACT_TARGET_DATES
  bool tdexact() { PLL_LOOP_FORWARD(DIL_AL_List,projects.head(),1) if (e->tdexact()) return true; return false; }
  // [***NOTE] For efficiency, the function below does not seek out the smallest period, but simply the first that it encounters.
  periodictask_t tdperiod() { PLL_LOOP_FORWARD(DIL_AL_List,projects.head(),1) if (e->tdperiod()!=pt_nonperiodic) return e->tdperiod(); return pt_nonperiodic;  }
  int  tdspan() {
    int span = 0;
    PLL_LOOP_FORWARD(DIL_AL_List,projects.head(),1) if (e->tdspan()>span) span = e->tdspan();
    return span;
  }
  int  tdevery() {
    int every = 1;
    PLL_LOOP_FORWARD(DIL_AL_List,projects.head(),1) if (e->tdevery()>every) every = e->tdevery();
    return every;
  }
#endif
  bool tdfixed() { PLL_LOOP_FORWARD(DIL_AL_List,projects.head(),1) if (e->tdfixed()) return true; return false; }
  float Unbounded() { if (!projects.head()) return 0.0; float max_unbounded=-MAXFLOAT; PLL_LOOP_FORWARD(DIL_AL_List,projects.head(),1) if (e->unbounded>max_unbounded) max_unbounded = e->unbounded; return max_unbounded; }
  float Urgency() { if (!projects.head()) return 0.0; float max_urgency=-MAXFLOAT; PLL_LOOP_FORWARD(DIL_AL_List,projects.head(),1) if (e->urgency>max_urgency) max_urgency = e->urgency; return max_urgency; }
  bool Write_to_Binary_Cache(ofstream & cfl);
  bool Read_from_Binary_Cache(ifstream & cfl);
  bool Binary_Cache_Diagnostic(DIL_entry_parameters * cached, int & testedbytes);
};

// DIL identifier and link to content
//  1) id
//  2) content
//  3) parameters (= topics & projects)
class DIL_entry: public DIL_ID, public PLLHandle<DIL_entry> {
protected:
#ifdef LOOP_PROTECT
  #define TD_PROTECT	1
#endif
  mutable int semaphore; // beware of simultaneous use for different purposes!
  Text_File_Buffers * tfb; // points to Detailed_Items_List or other source (is cleaned up there)
public:
  // constructors
  DIL_entry(Text_File_Buffers & t); // (see utilities.cc)
  DIL_entry(Text_File_Buffers & t, String & idstr); // preferred constructor
  DIL_entry(Text_File_Buffers & t, const SubString & idstr); // preferred constructor
  // destructor
  ~DIL_entry() { delete content; delete parameters; }
  // parameters
  DIL_entry_content * content;
  DIL_entry_parameters * parameters;
  // *** Can add functions to obtain individual DIL entry
  //     content or parameter data
  // functions
  DIL_entry * elbyid(const DIL_ID & id) const;
  DIL_entry * operator[](DIL_ID & id) { return elbyid(id); }
  DIL_Topical_List * Topics(int n) { if (parameters) if (parameters->topics.head()) return parameters->topics.head()->el(n); return NULL; }
  DIL_AL_List * Projects(int n) const { if (parameters) if (parameters->projects.head()) return parameters->projects.head()->el(n); return NULL; }
  void Set_Semaphore(int svalue) { semaphore = svalue; }
  int Get_Semaphore() { return semaphore; }
  void Set_Semaphores(int svalue) { semaphore = svalue; if (Next()) Next()->Set_Semaphores(svalue); }
#ifdef LOOP_PROTECT
  time_t Propagated_Target_Date(bool warnflag, bool top = true) const;
  time_t Target_Date(bool top = true) const { return Propagated_Target_Date(true,top); }
#else
  time_t Propagated_Target_Date(bool warnflag) const;
  time_t Target_Date() const { return Propagated_Target_Date(true); }
#endif
  time_t Local_Target_Date(int n, time_t unspecified = -2) const { if (Projects(n)) return Projects(n)->targetdate(); return unspecified; } // checks the locally specified target date of ONLY the n-th Superior!
  time_t Target_Date_Info(bool & isfromlocal, int & haslocal, int & numpropagating) const;
  DIL_entry * Propagated_Target_Date_Origin(time_t & t) const;
#ifdef INCLUDE_EXACT_TARGET_DATES
  bool tdexact() { if (!parameters) return false; return parameters->tdexact(); }
  periodictask_t tdperiod() { if (!parameters) return pt_nonperiodic; return parameters->tdperiod(); }
  int  tdspan() { if (!parameters) return 0; return parameters->tdspan(); }
  int  tdevery() { if (!parameters) return 1; return parameters->tdevery(); }
#endif
  bool tdfixed() { if (!parameters) return false; return parameters->tdfixed(); }
  float Unbounded() { if (!parameters) return 0.0; return parameters->Unbounded(); }
  float Urgency() { if (!parameters) return 0.0; return parameters->Urgency(); }
  filetitle_t * TL_tail_ref() { if (content) return &(content->TL_tail); return NULL; }
  filetitle_t * TL_head_ref() { if (content) return &(content->TL_head); return NULL; }
  time_t Time_Required() { if (content) return content->required; return (time_t) 0; }
  float Completion_State() { if (content) return content->completion; return (float) 0.0; }
  float Valuation() { if (content) return content->valuation; return (float) 0.0; }
  String * Entry_Text() { if (content) return content->text; return NULL; }
  bool Get_Entry_TL_References(String & referencestr);
  bool Get_Entry_Parameters(String & parstr);
  bool Is_in_Topic(String topicfilestr);
  bool Is_Direct_Dependency_Of(const DIL_entry * superior); // (see utilities.cc)
  bool Is_Dependency_Of(const DIL_entry * superior, bool recursing = false); // (see utilities.cc)
  DIL_Superiors * Superior_Parameters(const DIL_entry * superior); // (see utilities.cc)
  bool Has_Superiors(bool taggedonly = false); // (see utilities.cc)
  int Is_Plan_Entry(); // (see utilities.cc)
  bool Write_to_Binary_Cache(ofstream & cfl); // (see utilities.cc)
  bool Write_Topical_to_Binary_Cache(ofstream & cfl, bool gottext); // (see utilities.cc)
  bool Read_from_Binary_Cache(ifstream & cfl); // (see utilities.cc)
  bool Read_Topical_from_Binary_Cache(ifstream & cfl, bool gottext); // (see utilities.cc)
  bool Binary_Cache_Diagnostic(DIL_entry * cached, int & testedbytes); // (see utilities.cc)
};

typedef DIL_entry * DIL_entry_ptr;

class DIL_Virtual_Matrix {
  // This is used in alcomp.cc:generate_AL().
public:
  DIL_entry_ptr dep;
  long req;
  time_t td;
  bool isvirtual;
  DIL_Virtual_Matrix(): dep(NULL), req(0), td(-1), isvirtual(false) {}
};

// DIL
class Detailed_Items_List: public Text_File_Buffers {
protected:
  bool Write_to_Binary_Cache(); // write a binary cache version for quick loading
  bool Read_from_Binary_Cache(String * dilid = NULL); // read a binary cache version for quick loading
  bool Write_Topical_to_Binary_Cache(int diln, PLLRoot<DIL_entry_Pointer> & dep, bool gottext); // write a binary cache version for quick loading
  bool Read_Topical_from_Binary_Cache(int diln, bool gettext); // read a binary cache version fro quick loading
public:
  // parameters
  PLLRoot<DIL_entry> list;
  // functions
  bool Entry_Has_Dependencies(const DIL_entry * de, bool taggedonly = false); // (see utilities.cc)
  int Get_Project_Data(String & dildata, int projloc, String & hrefurl, String & hreftext, DIL_entry_parameters & dilentrypars);
  int Get_All_DIL_ID_File_Parameters(String * dilid = NULL);
  int Get_All_Topical_DIL_Parameters(bool gettext = false, bool cacherefresh = false);
  DIL_entry ** Sort_by_Target_Date(bool gettext = false);
  bool Write_to_File(); // make a modified DIL-by-ID file
  bool Binary_Cache_Diagnostic(); // test if the quick load cache process works reliably
};

class DIL_entry_Pointer: public PLLHandle<DIL_entry_Pointer> {
  // Not to be confused with DIL_entry_ptr! This is a protected
  // linked list of pointers to DIL_entry objects.
  // You can easily manage a list of these through a root,
  // e.g. PLLRoot<DIL_entry_Pointer> deptrlist, which provides
  // extra functions such as push() and pop(). See, for example,
  // how it is used in DIL_Visualize_GRAPH::Visualize_Element().
protected:
  DIL_entry * e;
public:
  DIL_entry_Pointer(DIL_entry * eptr): e(eptr) {}
  DIL_entry * Entry() { return e; }
};

class DIL_entry_Pair: public PLLHandle<DIL_entry_Pair> {
  // Just like DIL_entry_Pointer, but for a pair of pointers
  // to DIL_entry objects.
protected:
  DIL_entry * p;
  DIL_entry * q;
public:
  DIL_entry_Pair(DIL_entry * _p, DIL_entry * _q): p(_p), q(_q) {}
  DIL_entry * First() { return p; }
  DIL_entry * Second() { return q; }
  bool Same(DIL_entry * _p, DIL_entry * _q) { return ((p==_p) && (q==_q)); }
};

// Data to be used at a specific level of DIL hierarchy generation
class DIL_Hierarchy_Level_Data: public PLLHandle<DIL_Hierarchy_Level_Data> {
protected:
  DIL_entry * entry;
public:
  DIL_Hierarchy_Level_Data(DIL_entry & e,double l = DILSUPS_REL_UNSPECIFIED): entry(&e), likelihood(l), benefit(0.0), risk(0.0) {}
  double likelihood;
  double benefit;
  double risk;
  DIL_entry * Entry() { return entry; }
  virtual void reset_benefit_risk() { benefit=0.0; risk=0.0; }
  virtual bool has_benefit_risk() { return ((benefit!=0.0) || (risk!=0.0)); }
};

// DIL Hierarchy Visualization
class DIL_Visualize {
protected:
  double cumlikelihood;
  DIL_Hierarchy_Level_Data * leveldata;
public:
  DIL_Visualize(DIL_Hierarchy_Level_Data * ld = NULL): cumlikelihood(0.0), leveldata(ld) { }
  virtual ~DIL_Visualize() {}
  // virtual function templates, these functions are guaranteed
  // to be available in all classes that inherit DIL_Visualize
  PLLRoot<DIL_Hierarchy_Level_Data> ldroot;
  virtual void Initialize() = 0; // Must be defined and called in constructor (!!) by derived classes!
  virtual void Attach_Level_Data(DIL_Hierarchy_Level_Data & ld) { leveldata = &ld; }
  virtual void Detach_Level_Data() { leveldata = NULL; }
  virtual DIL_Hierarchy_Level_Data * Level_Data() { return leveldata;  }
  virtual bool Visualize_Element(DIL_entry * de, int depth, int width, DIL_entry * prede) = 0;
  virtual void Visualize_NotShown(int numdependencies, int depth, int width, DIL_entry * prede) = 0;
  virtual void Visualize_BeyondWidth(int width, DIL_entry * de) = 0;
  virtual String Output() = 0;
  virtual double get_cumlikelihood() { return cumlikelihood; }
  virtual void set_cumlikelihood(double cumlike) { cumlikelihood=cumlike; }
};

// DIL Hierarchy with Tabs
class DIL_Visualize_with_Tabs: public DIL_Visualize {
protected:
  String s;
  String content;
public:
  DIL_Visualize_with_Tabs(DIL_Hierarchy_Level_Data * ld = NULL): DIL_Visualize(ld) { Initialize(); }
  virtual void Initialize() { s = ""; content = ""; }
  virtual bool Visualize_Element(DIL_entry * de, int depth, int width, DIL_entry * prede); // (see diladmin.cc)
  virtual void Visualize_NotShown(int numdependencies, int depth, int width, DIL_entry * prede); // (see diladmin.cc)
  virtual void Visualize_BeyondWidth(int width, DIL_entry * de) {}
  virtual String Output() { return s; }
  virtual String Output_Content() { return content; }
};

// DIL Hierarchy with HTML and Tabs
class DIL_Visualize_with_HTML_Tabs: public DIL_Visualize {
protected:
  String s;
  String content;
public:
  DIL_Visualize_with_HTML_Tabs(DIL_Hierarchy_Level_Data * ld = NULL): DIL_Visualize(ld) { Initialize(); }
  virtual void Initialize() { s = ""; content = ""; }
  virtual bool Visualize_Element(DIL_entry * de, int depth, int width, DIL_entry * prede); // (see diladmin.cc)
  virtual void Visualize_NotShown(int numdependencies, int depth, int width, DIL_entry * prede); // (see diladmin.cc)
  virtual void Visualize_BeyondWidth(int width, DIL_entry * de) {}
  virtual String Output() { return s; }
  virtual String Output_Content() { return content; }
};

// DIL Hierarchy with FORM HTML and Tabs
class DIL_Visualize_with_FORM_Tabs: public DIL_Visualize {
protected:
  String s;
  String content;
  virtual bool Visualize_Plan_Entry(DIL_entry * de); // (see diladmin.cc)
public:
  DIL_Visualize_with_FORM_Tabs(DIL_Hierarchy_Level_Data * ld = NULL): DIL_Visualize(ld) { Initialize(); }
  virtual void Initialize() { s = ""; content = ""; }
  virtual bool Visualize_Element(DIL_entry * de, int depth, int width, DIL_entry * prede); // (see diladmin.cc)
  virtual void Visualize_NotShown(int numdependencies, int depth, int width, DIL_entry * prede); // (see diladmin.cc)
  virtual void Visualize_BeyondWidth(int width, DIL_entry * de) {}
  virtual String Output() { return s; }
  virtual String Output_Content() { return content; }
  virtual void Append(String appendstr) { s += appendstr; }
};

// DIL Hierarchy GRAPH
struct OOP_Status {
 public:
  bool has_detailing_task;
  bool identified_problems;
  bool identified_solutions;
  bool OOP_hierarchy_set;
 public:
  OOP_Status(): has_detailing_task(false), identified_problems(false), identified_solutions(false), OOP_hierarchy_set(false) {}
  void get(DIL_entry * de);
};

class DIL_Visualize_GRAPH: public DIL_Visualize {
protected:
  String s;
  String content; // not usually used in this class, see comments in DIL_Visualize_GRAPH::Visualize_Element().
  char graphtype;
  PLLRoot<DIL_entry_Pointer> objectivesstack; // a secondary stack used to track most recent Objectives
  PLLRoot<DIL_entry_Pair> objectivespairs; // pairs of Objectives with (in)direct connection
  //virtual bool Visualize_Plan_Entry(DIL_entry * de); // (see diladmin.cc)
public:
  DIL_Visualize_GRAPH(DIL_Hierarchy_Level_Data * ld = NULL): DIL_Visualize(ld), graphtype('D') { Initialize(); }
  virtual void Initialize() { s = ""; content = ""; }
  virtual bool Visualize_Element(DIL_entry * de, int depth, int width, DIL_entry * prede); // (see diladmin.cc)
  virtual void Visualize_NotShown(int numdependencies, int depth, int width, DIL_entry * prede); // (see diladmin.cc)
  virtual void Visualize_BeyondWidth(int width, DIL_entry * de);
  virtual void Add_Connections(DIL_entry * de);
  virtual String Output() { return s; }
  virtual String Output_Content() { return content; }
  //virtual void Append(String appendstr) { s += appendstr; }
  virtual void Add_Abstract_Connections();
};

class AL_TC: public PLLHandle<AL_TC> {
// AL Task Chunk
// *** Note: Using the PLL approach, a typical work day of 7 hours with 20 minute task chunks requires 21 * (3 PLL pointers + 1 PLL signal)
//     + 21 * (de + val) = 273 + 252 = 525 bytes (minimum). If I simply allocated an array for all 24 hours, then it would require
//     24*3* (de + val) = 864 bytes.
protected:
  DIL_entry * de; // allocated DIL entry
  float val; // value set by random distribution function (val < 0 means off-limits to regular scheduled work)
#ifdef INCLUDE_EXACT_TARGET_DATES
  time_t tcstart;
#endif
public:
  // constructors
#ifdef INCLUDE_EXACT_TARGET_DATES
  AL_TC(time_t _tcstart): de(NULL), val(0.0), tcstart(_tcstart) {}
  AL_TC(time_t _tcstart, long itc, long i_limit);
#else
  AL_TC(): de(NULL), val(0.0) {}
  AL_TC(long itc, long i_limit);
#endif
  // destructor (see http://www.geocrawler.com/archives/3/361/1998/1/0/2004286/)
  ~AL_TC() {} 
  // functions
#ifdef INCLUDE_EXACT_TARGET_DATES
  time_t TCstart() { return tcstart; }
#endif
  AL_TC * next_avail_el(); // (see alcomp.cc)
  AL_TC * avail_el(unsigned int n); // (see alcomp.cc)
  void set_val(float v) { val = v; }
  float get_val() { return val; }
  DIL_entry * Get_DE() { return de; }
  operator DIL_entry *() { return Get_DE(); }
  long available_before(unsigned int n); // (see alcomp.cc)
  float allocate_with_value(float uval, DIL_entry * d); // (see alcomp.cc)
#ifdef INCLUDE_EXACT_TARGET_DATES
  bool allocate_if_available(DIL_entry * d); // (see alcomp.cc)
#endif
  // *** Is the Set_DE() function dangerous? The only additional
  //     power over allocate() is the ability to allocate to TCs
  //     that already have de!=NULL. Could this happen accidentally?
  DIL_entry * Set_DE(DIL_entry * d) { DIL_entry * dprev = de; de = d; return dprev; }
  AL_TC * allocate(DIL_entry * d); // (see alcomp.cc)
};

#ifdef INCLUDE_PERIODIC_TASKS
class AL_TD: public PLLHandle<AL_TD> {
// Target Date DIL entries
	protected:
		DIL_Virtual_Matrix * dvm;
	public:
		// constructor
		AL_TD(DIL_Virtual_Matrix & d): dvm(&d) {}
		// destructor (see http://www.geocrawler.com/archives/3/361/1998/1/0/2004286/)
		~AL_TD() {} 
		// functions
		AL_TD * operator[](int n) { return el(n); }
		DIL_Virtual_Matrix * DE() const { return dvm; }
		operator DIL_Virtual_Matrix *() const { return dvm; }
};
#else
class AL_TD: public PLLHandle<AL_TD> {
// Target Date DIL entries
	protected:
		DIL_entry * de;
	public:
		// constructor
		AL_TD(DIL_entry & d): de(&d) {}
		// destructor (see http://www.geocrawler.com/archives/3/361/1998/1/0/2004286/)
		~AL_TD() {} 
		// functions
		AL_TD * operator[](int n) { return el(n); }
		DIL_entry * DE() const { return de; }
		operator DIL_entry *() const { return de; }
};
#endif

// Declared here, so that it can be used in an inline member function below.
String time_stamp(const char * dateformat, time_t t = -1) ; // (see utilities.cc)

class AL_Day: public PLLHandle<AL_Day> {
// AL Day
protected:
  // parameters
  int daytype; // 0 = full day, 1 = start, 2 = end
#ifdef INCLUDE_DAY_SPECIFIC_WORK_TIMES
  int dayofweek; // Note: This is only valid if set_avail() or DayofWeek() has been called!
#endif
  long avail; // task chunks available (not yet allocated)
  long tot; // total number of task chunks (allocated+available)
  time_t daydate; // date of day (at 00:00 in local time)
  time_t daystart; // work start offset (in seconds, GM time interval)
  time_t daystartmin; // lower limit (e.g. current time, GM time interval)
  time_t daymaxt; // upper limit (e.g. generatealmaxt, GM time interval)
  time_t tcseconds; // task chunk size in seconds, cached value for computations
  bool expanded; // flag set if day is expanded
  long cache_i_start;
  long cache_i_limit;
public:
  // constructor
#ifdef INCLUDE_DAY_SPECIFIC_WORK_TIMES
  AL_Day(time_t dd, time_t tcsec, time_t curt, time_t dmaxt = SECONDSPERDAY);
#else
  AL_Day(time_t dd, int typ, time_t tcsec, time_t dsmin, time_t dmaxt = SECONDSPERDAY);
#endif
  // destructor (see http://www.geocrawler.com/archives/3/361/1998/1/0/2004286/)
  ~AL_Day() {} 
  // parameters
  PLLRoot<AL_TC> tc; // list of task chunks
  PLLRoot<AL_TD> td; // list of target date DIL entries
  // *** perhaps protect val by offering a Reset() function that Active_List can use
  float val; // value set by random distribution function
  // functions
  int DayType() { return daytype; }
#ifdef INCLUDE_DAY_SPECIFIC_WORK_TIMES
  int DayofWeek() { if (dayofweek<0) dayofweek = atoi((const char *) time_stamp("%w",daydate)); return dayofweek; }
#endif
  time_t DayDate() { return daydate; }
  time_t DayStart() { return daystart; }
  time_t DayStartMin() { return daystartmin; }
  time_t DayMaxt() { return daymaxt; }
  bool Expanded() { return expanded; }
  long Avail() { return avail; }
  long Total() { return tot; }
  AL_TC * TC_Head() { return tc.head(); }
  AL_TC * TC_Tail() { return tc.tail(); }
  AL_TD * TD_Head() { return td.head(); }
  AL_TD * TD_Tail() { return td.tail(); }
  long set_avail(time_t av); // (see alcomp.cc)
  long available_before(time_t td); // (see alcomp.cc)
  int days_to(time_t td); // (see alcomp.cc)
  void create_TCs(bool setvalues = false); // (see alcomp.cc)
  enum TC_add_method { TAIL_REPLACE, RANDOM, BEFORE_HEAD };
  void add_TCs(long tcs, TC_add_method m, time_t addborder = (MAXTIME_T)); // (see alcomp.cc)
  long expand(long expandn, time_t td); // (see alcomp.cc)
  float set_TC_values(long & i_start, long i_limit); // (see alcomp.cc)
  float allocate_with_value(float uval, DIL_entry * de); // (see alcomp.cc)
#ifdef INCLUDE_EXACT_TARGET_DATES
  AL_TC * create_TCs_to_date(time_t tctdate, long req); // (see alcomp.cc)
  long allocate_with_date(DIL_entry * de, time_t tctdate, long req, long & regularworktimechunks); // (see alcomp.cc)
#endif
  AL_TC * Get_Avail_TC(unsigned long n); // (see alcomp.cc)
  AL_TC * allocate(DIL_entry * de); // (see alcomp.cc)
#ifdef INCLUDE_PERIODIC_TASKS
  AL_Day * Add_Target_Date(DIL_Virtual_Matrix & dvm); // (see alcomp.cc)
#else
  AL_Day * Add_Target_Date(DIL_entry * de); // (see alcomp.cc)
#endif
  long remove_unused_TCs(); // (see alcomp.cc)
  long remove_unused_and_periodic_only_TCs(); // (see alcomp.cc)
  float linear_day_distribution(time_t td); // (see alcomp.cc)
  float squared_day_distribution(time_t td); // (see alcomp.cc)
};

class Active_List {
  // Active List
protected:
  // parameters
  DIL_entry * superior; // superior DIL entry of AL (NULL=AL with all DIL entries)
  String supstr; // string identifying superior, default "all"
  String alfilestr; // file base string, default "active-list.all"
  long allength; // number of TCs in AL
  // functions
  long Initialize_AL_Length(long totreq, long deplen, time_t algenwt, DIL_entry * superior = NULL); // (see alcomp.cc)
public:
  // constructor
  Active_List(long totreq, long deplen, int algenwt, DIL_entry * sup = NULL); // (see alcomp.cc)
  // parameters
  PLLRoot<AL_Day> al; // list of AL days
  PLLRoot<AL_TD> td; // list of passed target date DIL entries
  long deavail; // temporary variable for tracking of DIL entry available TCs
		// note: deavail is set by available_before(), expand(),
		//       allocate_with_value()
		// functions
  String get_supstr() { return supstr; }
  String filestr() { return alfilestr; }
  long length() { return allength; }
#ifdef INCLUDE_DAY_SPECIFIC_WORK_TIMES
  long Add_Days(time_t tcsec, time_t curt, time_t dmaxt, time_t algenwt); // (see alcomp.cc)
#else
  AL_Day * Add_Day(int typ, time_t tcsec, time_t dsmin, time_t dmaxt = SECONDSPERDAY); // (see alcomp.cc)
#endif
  long available_before(time_t td) { if (al.head()) deavail=al.head()->available_before(td); else deavail=0; return deavail; }
  long expand(long expandn, time_t td); // (see alcomp.cc)
  AL_Day * AL_Head() { return al.head(); }
  AL_Day * AL_Tail() { return al.tail(); }
  long days() { return al.length(); }
  float set_TC_values(); // (see alcomp.cc)
#ifdef INCLUDE_PERIODIC_TASKS
  float allocate_with_value(float uval, DIL_Virtual_Matrix & dvm); // (see alcomp.cc)
#else
  float allocate_with_value(float uval, DIL_entry * de); // (see alcomp.cc)
#endif
#ifdef INCLUDE_EXACT_TARGET_DATES
#ifdef INCLUDE_PERIODIC_TASKS
  long allocate_with_date(DIL_Virtual_Matrix & dvm);  // (see alcomp.cc)
#else
  long allocate_with_date(DIL_entry * de, long req);  // (see alcomp.cc)
#endif
#endif
  AL_TC * Get_Avail_TC(unsigned long n); // (see alcomp.cc)
  AL_TC * allocate(DIL_entry * de); // (see alcomp.cc)
#ifdef INCLUDE_PERIODIC_TASKS
  AL_Day * Add_Target_Date(DIL_Virtual_Matrix & dvm); // (see alcomp.cc)
  AL_TD * Add_Passed_Target_Date(DIL_Virtual_Matrix & dvm); // (see alcomp.cc)
#else
  AL_Day * Add_Target_Date(DIL_entry * de); // (see alcomp.cc)
  AL_TD * Add_Passed_Target_Date(DIL_entry * de); // (see alcomp.cc)
#endif
  long remove_unused_TCs(); // (see alcomp.cc)
  long remove_unused_and_periodic_only_TCs(); // (see alcomp.cc)
  bool generate_focused_AL(DIL_entry * superior, time_t algenwt); // (see alcomp.cc)
  bool generate_wide_AL(DIL_entry * superior); // (see alcomp.cc)
};

struct alCRT_day {
  // In a 32-bit environment, a day structure consumes less than 1.2 KBytes
public:
  DIL_entry_ptr fiveminchunk[288]; // five minute granularity
  time_t daytd;
  //int dayofweek; // 0 = Sunday
public:
  alCRT_day(): daytd(0) { for (int i=0; i<288; i++) fiveminchunk[i]=NULL; }
  alCRT_day(time_t _daytd) { init(_daytd); }
  void init(time_t _daytd);
  bool reserve_exact(DIL_entry_ptr dep, int & grains, time_t td);
  int reserve_fixed(DIL_entry_ptr dep, int & grains, time_t td);
  time_t reserve(DIL_entry_ptr dep, int & grains);
};

/*struct alCRT_week {
public:
  alCRT_day day[7];
  time_t weektd;
public:
  alCRT_week(time_t _weektd): weektd(_weektd) {
    time_t t = _weektd;
    for (int i=0; i<7; i++) {
      day[i].init(t);
      t = time_add_day(t,1);
    }
  }
  };*/

struct alCRT_map {
public:
  int num_days;
  time_t starttime;
  time_t firstdaystart;
  alCRT_day * day;
public:
  alCRT_map(time_t t,int n);
  ~alCRT_map() { if (day) delete[] day; }
  int find_dayTD(time_t td);
  bool reserve_exact(DIL_entry_ptr dep, int req, time_t td);
  bool reserve_fixed(DIL_entry_ptr dep, int req, time_t td);
  time_t reserve(DIL_entry_ptr dep, int req);
  bool Plot_Map_to_File(String mapfile);
};

struct pool_entry_file_data {
public:
  DIL_ID id;
public:
  pool_entry_file_data() {}
  pool_entry_file_data(DIL_ID & _id): id(_id) {}
  //pool_entry_file_data(String & _id): id(_id) {}
};

class pool_entry: public PLLHandle<pool_entry> {
public:
  DIL_entry * de;
  //String dilidstr;
  DIL_ID id;
public:
  pool_entry(DIL_entry & _de): de(&_de) {}
  pool_entry(String & _dilidstr): de(NULL), id(_dilidstr) {} //dilidstr(_dilidstr) {}
  pool_entry(DIL_ID & _id): de(NULL), id(_id) {}
  pool_entry_file_data File_Data() { 
    if (de) {
      pool_entry_file_data pefd(*de);
      return pefd;
    } else {
      pool_entry_file_data pefd(id); //(dilidstr);
      return pefd;
    }
  }
};

class pool: public PLLRoot<pool_entry> {
  // see alcomp.cc for function definitions
public:
  pool() {}
  bool write_data(String poolname);
  bool read_data(String poolname, bool warnopen = true);
  void write_HTML_FORM(String poolname, String pooldescription, const String & poolcomment);
  void append(String & dilidstr) {
    pool_entry * pe = new pool_entry(dilidstr);
    link_before(pe);
  }
  void append(pool_entry_file_data & pefd) {
    pool_entry * pe = new pool_entry(pefd.id);
    link_before(pe);
  }
  void match_DIL_entries();
  pool_entry * elbyid(DIL_ID & _id);
};

struct ooplanner_entry_file_data {
public:
  DIL_ID id;
  long writeinbufoffset;
  int writeinlen;
  time_t targetdate;
public:
  ooplanner_entry_file_data(): writeinbufoffset(0), writeinlen(0), targetdate(-1) {}
  ooplanner_entry_file_data(DIL_ID & _id, time_t _targetdate = -1): id(_id), writeinbufoffset(0), writeinlen(0), targetdate(_targetdate) {}
  ooplanner_entry_file_data(String & writeinbuffer, String & writein, time_t _targetdate = -1): writeinbufoffset(writeinbuffer.length()), writeinlen(writein.length()), targetdate(_targetdate) { writeinbuffer += writein; }
};

class ooplanner_entry: public PLLHandle<ooplanner_entry> {
public:
  DIL_entry * de;
  DIL_ID id;
  String writein; // a write-in that does not have an existing DIL entry
  time_t targetdate; // [***Note] This is independent of de->Target_Date(). If it is not defined during planning, then a strategy must be chosen to give it a value during rescheduling. One possibly strategy is that if a de->Target_Date() is known and if that date fits into the plan, given known surrounding targetdate values, then that value in retained.
public:
  ooplanner_entry(DIL_entry & _de, time_t _targetdate = -1): de(&_de), targetdate(_targetdate) {}
  ooplanner_entry(DIL_ID & _id, time_t _targetdate = -1): de(NULL), id(_id), targetdate(_targetdate) {}
  ooplanner_entry(String _writein, time_t _targetdate = -1): de(NULL), writein(_writein), targetdate(_targetdate) {}
  ooplanner_entry(pool_entry & pe): de(pe.de), id(pe.id), targetdate(-1) {}
  ooplanner_entry_file_data File_Data(String & writeinbuffer) {
    if (de) {
      ooplanner_entry_file_data oefd(*de,targetdate);
      return oefd;
    } else if (id.valid()) {
      ooplanner_entry_file_data oefd(id,targetdate);
      return oefd;
    } else {
      ooplanner_entry_file_data oefd(writeinbuffer,writein,targetdate);
      return oefd;
    }
  }
};

typedef ooplanner_entry * ooplanner_entry_ptr;

class ooplanner: public PLLRoot<ooplanner_entry> {
  // see alcomp.cc for function definitions
public:
  ooplanner() {}
  bool write_data();
  bool read_data(bool warnopen = true);
  void HTML_FORM_content(String & ooplannerstr);
  void write_HTML_FORM();
  void append(ooplanner_entry_file_data & oefd, const char * writeinbuffer) {
    if (oefd.id.valid()) {
      ooplanner_entry * oe = new ooplanner_entry(oefd.id,oefd.targetdate);
      link_before(oe);
    } else {
      String writein(&(writeinbuffer[oefd.writeinbufoffset]),oefd.writeinlen);
      ooplanner_entry * oe = new ooplanner_entry(writein,oefd.targetdate);
      link_before(oe);
    }
  }
  void match_DIL_entries();
  ooplanner_entry_ptr * access_table();
  ooplanner_entry * elbyid(DIL_ID & _id);
};

// Objects for Searching
// ---------------------
// See <A HREF="search-objects.fig">search-objects.fig</A>.

/*
	The function that uses the following classes to do searches may search
	by following links starting from a specific file, directory or search
	engine search term, or may search a set of archive locations.
	Locations that point to directories or that are search engine search
	terms must be turned into a list of specific targets to search. Note
	that targets obtained in followlinks mode by Search_Target objects may
	also point to such locations. In the case of breadth-first search the
	conversion to specific targets may be done by the same function that
	controls the searching process. Otherwise, the conversion must take
	place in Search_Target, while adding targets to the list.
	Directories, search engine results and tar/zip archives can be handled
	in the same fashion.

	See <A HREF="search.cc">search.cc</A>.
*/

//#define TARGET_ACCESS_DEBUG
class Target_Access {
// Target access object, can handle local and remote targets of various formats
// Recognized original target references are of the following forms:
// (a) local file: /some/path/to/file.ext
// (b) remote HTTP: http://some.server/some/path/to/file.ext
// (c) remote FTP: ftp://some_user@some.server/some/path/to/file.ext
// When accessed, the target is automatically copied to a local file if necessary
// and uncompressed (.gz, .Z).
// *** May turn this into several classes eventually.
// *** If localcopy, uncompressed and cleartext are turned into special objects, their
//     destructors can take care of optional removal instead of the remove_* functions.
// *** This should be documented for more wide-spread use.
	public:
		enum Target_Access_types { TA_LOCAL, TA_LOCAL_DIRECTORY, TA_HTTP, TA_FTP, TA_UNKNOWN };
	protected:
		String target;	// original target reference
		String user, server, path;
		Target_Access_types type;
		String localcopy, uncompressed, cleartext; // transformed versions
		bool retaintransformed;
		void set_target(const char * t, const Target_Access * parent = NULL);
		void remove_name_reference(String & tpath);
		void remove_back_references(String & tpath);
		void transformed_name(String & transformed, String & prevname);
		void transformed_name(String & transformed, String & prevname, const BigRegex & transrx);
		bool make_local_copy_of_directory();
		bool make_local_copy_of_HTTP();
		bool make_local_copy_of_FTP();
		bool syscommand(String program, String args, int success = 0) const;
	public:
		// constructor
		Target_Access(const char * t): localcopy(""), uncompressed(""), cleartext(""), retaintransformed(false) { set_target(t); }
		Target_Access(const Target_Access & t): target(t.target), user(t.user), server(t.server), path(t.path), type(t.type), localcopy(t.localcopy), uncompressed(t.uncompressed), cleartext(t.cleartext), retaintransformed(t.retaintransformed) {}
		Target_Access(const Target_Access & parent, const char * t): localcopy(""), uncompressed(""), cleartext(""), retaintransformed(false) { set_target(t,&parent); } // relative to parent target
		int isequal(const Target_Access & ta) const { return (target==ta.target); }
		int operator==(const Target_Access & ta) const { return isequal(ta); }
		int operator!=(const Target_Access & ta) const { return (!isequal(ta)); }
		bool make_local_copy();
		bool uncompress();
		bool cleartext_conversion();
		bool read_raw_into_String(String & text, bool warnopen = true);
		bool read_uncompressed_into_String(String & text, bool warnopen = true);
		bool read_cleartext_into_String(String & text, bool warnopen = true);
		void remove_local_copy() { if (!localcopy.empty()) { if (type!=TA_LOCAL) remove(localcopy); localcopy = ""; } }
		void remove_uncompressed() { if (!uncompressed.empty()) { if (uncompressed!=path) remove(uncompressed); uncompressed = ""; } }
		void remove_cleartext() { if (!cleartext.empty()) { if (cleartext!=path) remove(cleartext); cleartext = ""; } }
		String Target() const { return target; }
		Target_Access_types Type() const { return type; }
		void Retain_Transformed() { retaintransformed = true; } // mode selector: whether to keep transformed versions or not
		void Discard_Transformed() { retaintransformed = false; }
#ifdef TARGET_ACCESS_DEBUG
		void Report() const;
#endif
};

class Search_Result;

class Search_Target: public PLLHandle<Search_Target> {
// Target to Search
	// pllroot references list of targets
	protected:
		PLLRoot<Search_Result> * results;	// reference to global list of results
		String searchkey;				// Search key to search for
		Target_Access target;			// Target in which to search
		int followlinks;				// Search links to this depth from the current target
		bool localtargets;				// True if depth-first search following links
		Quick_String_Hash * encountered;	// Optional tree of previously encountered targets
		// Having encountered a target before does not remove the target from any lists
		// or disable actions. This is good, since statistics or other activities may wish
		// to be done on repeated occurrences of a target. Having encountered a target before
		// merely sets the default to searched = true, so that the efficient default is not to
		// search it again. That parameter can be explicitly reset.
		int searched; // 0 = remains to be searched, 1 = already searched, -1 = do not search
		void check_encountered();
	public:
		// constructor
		Search_Target(PLLRoot<Search_Result> * r, const char * s, const char * t, int f, bool l, Quick_String_Hash * e = NULL): results(r), searchkey(s), target(t), followlinks(f), localtargets(l), encountered(e), searched(0) { check_encountered(); }
		Search_Target(const Search_Target & parent, const char * t): results(parent.results), searchkey(parent.searchkey), target(parent.target,t), followlinks(parent.followlinks-1), localtargets(parent.localtargets), encountered(parent.encountered), searched(0) { check_encountered(); } // relative to a parent target
		// *** Take care not to delete results or breadth-first targets when this object is removed!
		// functions
		String Target() const { return target.Target(); }
		long search();
		long links(const String & t);
		void set_searched(int sval = 1) { searched = sval; }
		void reset_searched() { searched = 0; }
};

class Search_Result: public PLLHandle<Search_Result> {
// Result of Search
	// pllroot references list of results
	public:
		enum Search_Result_types { SR_REGULAR, SR_DIRECTORY_ENTRY, SR_URL };
	protected:
		// searchkey and target are copied here so that Search_Target can be deleted and results retained
		String searchkey;		// Search key for which this result was found
		Target_Access target;	// Target in which this result was found
		String pretext, keytext, posttext;	// Key found and surrounding text
		long location;			// Result location in target
		Search_Result_types type;
	public:
		// constructor
		Search_Result(const char * s, const Target_Access & t, String & text, const BigRegex & skrx);
		// functions
		Search_Result_types Type() { return type; }
		int isequal(const Search_Result & sr) const;
		int operator==(const Search_Result & sr) const { return isequal(sr); }
		int operator!=(const Search_Result & sr) const { return (!isequal(sr)); }
		String HTML_put_text() const;
		operator const char *() const { return (const char *) HTML_put_text(); }
};

class Financial_Note: public PLLHandle<Financial_Note> {
  // Financial note item
  // pllroot references list of financial notes
protected:
  time_t date;
  double expense;
  String type; // e.g. variable, regular, planned, car, baby, utilities, etc.
  bool handled; // true if this item has already been included in the financial accounting
  String note;
  String comment;
public:
  Financial_Note(time_t d, double e, String t, bool h, String n, String c): date(d), expense(e), type(t), handled(h), note(n), comment(c) {}
  Financial_Note(): date(0), expense(0.0), type(""), handled(false), note(""), comment("") {}
  Financial_Note(String & s, time_t & t, int & sloc) { sloc = get(s,t,sloc); }
  int get(String & s, time_t & t, int sloc = 0);
  String put() const;
  void link_sorted(PLLRoot<Financial_Note> & r);
  time_t Date() const { return date; }
  bool Handled() const { return handled; }
  String Type() const { return type; }
  double Expense() const { return expense; }
};

class Financial_Monthly_Category;

class Financial_Monthly_SubCategory: public PLLHandle<Financial_Monthly_SubCategory> {
  // Monthly subcategory results from Financial note items
  // pllroot references list of subcategories
  friend class Financial_Monthly_Category;
protected:
  String subcategory;
  double result;
  void add(double e) { result += e; }
public:
  Financial_Monthly_SubCategory(String s, double r = 0.0): subcategory(s), result(r) {}
  String SubCategory() const { return subcategory; }
  Financial_Monthly_SubCategory * Find_SubCategory(String s) const;
  double Result() const { return result; }
};

class Financial_Monthly;

class Financial_Monthly_Category: public PLLHandle<Financial_Monthly_Category> {
  // Monthly category results from Financial note items
  // pllroot references list of categories
protected:
  String category;
  double result;
public:
  Financial_Monthly_Category(String c, double r = 0.0): category(c), result(r) {}
  PLLRoot<Financial_Monthly_SubCategory> subcategories;
  String Category() const { return category; }
  Financial_Monthly_Category * Find_Category(String c) const;
  double Result() const { return result; }
  void Copy_SubCategories(Financial_Monthly_Category * c);
  void add(String s, double e, Financial_Monthly & month);
};

class Financial_Monthly: public PLLHandle<Financial_Monthly> {
  // Monthly results from Financial note items
  // pllroot references list of months
protected:
  String month;
public:
  Financial_Monthly(String m): month(m) {}
  PLLRoot<Financial_Monthly_Category> categories;
  String Month() const { return month; }
  Financial_Monthly * Find_Month(String m) const;
  void Copy_Categories(Financial_Monthly * h);
  void add(const Financial_Note & fn);
};

class Financial_Monthly_Root: public PLLRoot<Financial_Monthly> {
  // Monthly results from Financial note items
  // pllroot references list of months
public:
  Financial_Monthly_Root() {};
  void add(const Financial_Note & fn);
  String put_results() const;
};

class TL_entry_content;
class Metrics_Days;

class Metrics_Category: public PLLHandle<Metrics_Category> {
public:
  String category;
  int categorynumber;
  StringList DIL;
  StringList DE;
  Metrics_Category(String c, int cnum): category(c), categorynumber(cnum) {}
  void add_chunk(TL_entry_content * chunk, Metrics_Days & MD); // (tlfilter.cc)
  void add_labeled_chunk(TL_entry_content * chunk, Metrics_Days & MD, double metricvalue); // (tlfilter.cc)
  void invalidate_chunk(TL_entry_content * chunk, Metrics_Days & MD); // (tlfilter.cc)
  String csv_String(); // (tlfilter.cc)
  String category_rc_String(); // (tlfilter.cc)
};

class Metrics_Categories {
public:
  PLLRoot<Metrics_Category> list;
  unsigned int nummetricscategories;
  Metrics_Categories(String categoriesfile, String labelid); // (tlfilter.cc)
  Metrics_Category * get_category_by_name(const char * c); // (tlfilter.cc)
  Metrics_Category * get_category_by_number(int cnum); // (tlfilter.cc)
  int get_category_number_by_name(const char * c); // (tlfilter.cc)
  unsigned char get_map_code_by_name(const char * c); // (tlfilter.cc)
  String csv_String(); // (tlfilter.cc)
  String categories_rc_String(); // (tlfilter.cc)
};

class Metrics_Day: public PLLHandle<Metrics_Day> {
  // This is a linked list of Days that are used as sensible
  // partitions within which to track certain metrics
public:
  time_t daydate;
  double * secondspercategory; // array keeping count for each category
  unsigned int nummetricscategories;
  unsigned char * daymap; // a byte-map per day if enabled by the -zMAP option
  Metrics_Day(time_t d, unsigned int numcategories, bool mapit = false); // (tlfilter.cc)
  ~Metrics_Day() { delete[] secondspercategory; delete[] daymap; }
  bool MapIt() { return (daymap!=NULL); }
  void add_to_day(time_t ddate, double addseconds, unsigned int categorynumber); // (tlfilter.cc)
  void add_to_day_map(time_t ddate, time_t starttime, time_t endtime, unsigned int categorynumber); // (tlfilter.cc)
  int count_in_map(unsigned char mapcode, unsigned int offset = 0, unsigned int minutes = 1440); // (tlfilter.cc)
  String csv_String(); // (tlfilter.cc)
};

class Metrics_Days {
public:
  PLLRoot<Metrics_Day> list;
  Metrics_Categories * MC;
  unsigned int nummetricscategories;
  String labelid;
  bool mapit;
  Metrics_Days(Metrics_Categories & mc, String & _labelid): MC(&mc), nummetricscategories(mc.nummetricscategories), labelid(_labelid), mapit(false) { if (labelid=="MAP") mapit=true; }
  void add_to_day(time_t ddate, double addseconds, unsigned int categorynumber); // (tlfilter.cc)
  void add_to_day_map(time_t ddate, time_t starttime, time_t endtime, unsigned int categorynumber); // (tlfilter.cc)
  time_t * day_dates_array(int * numdays = NULL); // (tlfilter.cc)
  double * seconds_in_category_array(const char * c); // (tlfilter.cc)
  double * seconds_in_category_array(const char * c, double divideby); // (tlfilter.cc)
  double * count_in_maps(unsigned char mapcode, unsigned int offset = 0, unsigned int minutes = 1440, double divideby = 1.0);
  String csv_String(); // (tlfilter.cc)
  void map();
};

class TL_entry_content {
  // This class is used to contain structured data about the content of one
  // chunk or entry referred to in a task log file.
protected:
  void TL_get_chunk_data(String & tlstr, int tlloc); // (tlfilter.cc)
public:
  TL_entry_content(): ischunk(true), chunkendtime(-1) {};
  TL_entry_content(String & tlstr, int tlloc, filetitle_t & entryref); // (tlfilter.cc)
  TL_entry_content(String & tlstr, int tlchunkbegin, int tlchunkend, filetitle_t & entryref); // (tlfilter.cc)
  filetitle_t source; // file and reference name in original source location
  bool ischunk; // is it a chunk or single entry reference?
  filetitle_t al; // chunk AL
  filetitle_t alprev, alnext; // references to previous and next in AL
  filetitle_t dil; // chunk DIL
  filetitle_t dilprev, dilnext; // references to previous and next in DIL task
  String htmltext; // the written content of the entry
  time_t _chunkstarttime;
  time_t chunkendtime;
  time_t chunk_seconds(); // (tlfilter.cc)
  String str(); // (tlfilter.cc)
  void metrics_labelid(Metrics_Days & MD, String & labelid); // (tlfilter.cc)
  void metrics(Metrics_Days & MD, String & labelid); // (tlfilter.cc)
};

class Task_Log: public Text_File_Buffers {
  // This object facilitates access to structured content with Task Log files.
  // This class is used in functions such as:
  //   get_collected_task_entries() (tlfilter.cc)
  //   metrics_parse_task_log (tlfilter.cc)
  // This object inherits its own Text_File_Buffers (just like Detailed_Items_List).
  // NOTE: In future, this object may be enhanced to offer conversion of HTML
  // Task Log file content into a linked list of Task Log content classes.
public:
  // Task Chunk operations variables
  String chunktasklog; // Full file name of Task Log section for Task Chunk operation
  int chunkseekloc; // Current seek position in Task Log section for Task Chunk operation
  
  Task_Log(); // (tlfilter.cc)
  TL_entry_content * get_task_log_entry(filetitle_t * entryref); // (tlfilter.cc)
  bool preceding_task_log_section_filename(String & tfstr, String & prevTLfilename); // (tlfilter.cc)
  TL_entry_content * get_previous_task_log_entry(); // (tlfilter.cc)
};

// used in tladmin.cc
struct TL_chunk_access {
  // A convenient container for access information about a Chunk in
  // the Task Log.
  // This is used by tladmin.cc:process_note_TL() and related functions.
  String tl; // for contents of a Task Log Section
  String chunkid; // the NAME tag ID of the Chunk
  int tlinsertloc; // next Entry insertion location
};

// used in tladmin.cc
struct DILAL_head_title {
  // A convenient container for data used to search for and store
  // header and title information from DIL or AL files.
  // This is used by tladmin.cc:add_TL_chunk_or_entry() and related functions.
  String head;
  String title;
  String str; // file contents
  String rx; // head and title search RegEx
};

// used in tladmin.cc
struct DILAL_newref {
  // A convenient container for DIL and AL reference data.
  // This is used by tladmin.cc:add_TL_chunk_or_entry() and related functions.
  String al; // AL reference ID
  String dil; // DIL reference ID or identifying comment
};

// used in tladmin.cc
struct TLALDIL_headfile {
  // A convenient container for TL, AL and DIL head reference data.
  // This is used by tladmin.cc:add_TL_chunk_or_entry() and related functions.
  String tl; // TL head reference
  String al; // AL head reference
  String dil; // DIL head reference
};

class Calendar_Task: public PLLHandle<Calendar_Task> {
public:
  DIL_entry * de;
  char * allocation;
  int numchunks;
  int chunkdivider;
  int earliest;
  int tdidx;
  int numdays;
  Calendar_Task(DIL_entry & _de, unsigned int multi_days = 1); // (alcomp.cc)
  ~Calendar_Task() { if (allocation) delete[] allocation; }
  void mark_chunk(time_t tcstart, time_t daystart); // (alcomp.cc)
  void mark_target_date(time_t td, time_t daystart); // (alcomp.cc)
  void mark_intervals(); // (alcomp.cc)
  String HTML_Chunks_by_Task(String & dstfile); // (alcomp.cc)
};

class Calendar_Day {
  // This class is used to collect data from the AL and arrange it in useful
  // ways around calendar days.
public:
  PLLRoot<Calendar_Task> tasks;
  time_t daystart;
  int numdays;
  Calendar_Day(time_t ds, unsigned int multi_days = 1): daystart(ds), numdays(multi_days) {}
  Calendar_Task * get_task(DIL_entry & de); // (alcomp.cc)
  Calendar_Task * add_task(DIL_entry & de); // (alcomp.cc)
  void gather_day_tasks(AL_Day & AD); // (alcomp.cc)
  String HTML_Day_Chunks_by_Task(String dstfile); // (alcomp.cc)
};

/*
======== 9) CONFIGURATION PARAMETER DEFAULTS ========
*/

// NOTE: Not all of the following are run-time settable through the
// Set_Parameter() function.
// *** It may be better to separate those that are run-time settable
//     from those that are only compiled-time settable configuration
//     parameters. (They would still be separate from compile-time
//     options, which are more about code choices.)

#define CONFIGFILE ".dil2alrc"
#define RELDIRHTML "doc/html/"
#define DILMAIN "lists.html"
#define RELLISTSDIR "lists/"
#define DILBYIDFILE "lists/detailed-items-by-ID.html"
#define DEFAULTOUTPUTLOG "/tmp/dil2al-output.html"
#define SHOWMETRICSFLAGFILE ".formalizer-SHOWMETRICS"
#define OWNER "Randal A. Koene"
#define WINDOWICON "images/icons/makenote.png"
#define INFOICON_LARGE "images/icons/info-48x48.svg"
#define WARNICON_LARGE "images/icons/warn-48x48.svg"
#define ERRORICON_LARGE "images/icons/error-48x48.svg"
#define INFOICON_SMALL "images/icons/info-16x16.svg"
#define WARNICON_SMALL "images/icons/warn-16x16.png"
#define ERRORICON_SMALL "images/icons/error-16x16.svg"
// time chunks
#define DEFAULTCHUNKSIZE 20
#define DEFAULTCHUNKSLACK 5
#define DEFAULTSECTIONSIZE 150
#define DEFAULTALTCS (3*3*12)
#define DEFAULTALFOCUSED (3*3*12)
#define DEFAULTALFOCUSEDCALEXCERPTLENGTH 76
#define DEFAULTALFOCUSEDEXCERPTLENGTH 100
#define DEFAULTALFOCUSEDCALDAYS 1
#define DEFAULTALFOCUSEDCALCOLUMNS 72
#define DEFAULTALWIDE (12*3*12)
#define DEFAULTALGENREGULAR 8
#define DEFAULTALGENALL 13
#define DEFAULTALEPSHOURS (24.0 - 6.0 - 2.5 - 1.0 - 3.5)
#define DEFAULTDOLATERENDOFDAY (23*3600)
#define DEFAULTDOEARLIERENDOFDAY (19*3600)
#define DEFAULTALWORKDAYSTART ((time_t) 9 * (time_t) 60 * (time_t) 60)
#define DEFAULTALSYNCSLACK ((time_t) 60 * (time_t) 60)
#define DEFAULTALDAYDISTSLOPE	0.9
#define DEFAULTALCUMTIMEREQEXCERPTLENGTH 25
#define DEFAULTHIERARCHYMAXDEPTH 15
#define DEFAULTHIERARCHYEXCERPTLENGTH 50
#define DEFAULTEDITOR "emacs"
#define DEFAULTBROWSER "w3m"
#define NOTETMPFILE "tmp/dil2al-note.html"
#define TLENTRYTMPFILE "tmp/dil2al-TLentry.html"
#define TLTRACKPERFORMANCEFILE ".dil2al-TLperformance.m"
#define DILPRESETFILE ".dil2al-DILidpreset"
#define NORMALSKIPEXCLUDESFILE ".dil2al-normal-skip-excludes"
#define VIRTUALOVERLAPSFILE "active-list.virtual-overlaps.html"
#define EXACTCONFLICTSFILE "active-list.exact-conflicts.html"
#define TASKLOGFILE "lists/task-log.html"
#define FIRSTCALLPAGETEMPLATE "lists/first_call_page_template.html"
#define FIRSTCALLPAGETARGET "lists/first_call_page.html"
#define PAPERPLANSFILE "lists/paper-plans.html"
#define POOLFILE "lists/pool."
#define OOPLANNERFILE "lists/ooplanner."
#define FIGURESDIRECTORY "lists/figures-directory.html"
#define BIBINDEXFILE "planning/bibliography.html"
#define ARTICLEHEADTEMPLATE "lists/template.Article-Head.tex"
#define ARTICLETAILTEMPLATE "lists/template.Article-Tail.tex"
#define TEMPLATEARTICLESUBSECTION "lists/template.Article-SubSection.tex"
#define TEMPLATEARTICLESUBSUBSECTION "lists/template.Article-SubSubSection.tex"
#define DILREFFILE "tmp/dilref"
#define TAREADFILE "tmp/dil2al-TA-read."
#define FINNOTESFILE "financial-notes.html"
#define CALENDARFILE "lists/calendar.ics"
// The old one was used with DEFAULTEDITOR "nedit"
#define DEFAULTUNIECMD "nc -noask -do 'revert_to_saved()' @notedst@ -do 'set_cursor_pos(@res@)' @notedst@"
#define DEFAULTTASKHISTORYPAGE "lists/formalizer.current-task-history.html"
#define DEFAULTTASKHISTORYCALL "urxvt -rv -bg lightgreen -geometry 150x70+900+0 -fade 30 -title history -e w3m"
#define DEFAULTMETRICSDAYSPAGE "lists/formalizer.metrics-days-categories.csv"
#define DEFAULTMAINMETRICSCATEGORIESFILE "lists/formalizer.task-metrics-categories.rc"
#define DEFAULTMETRICSMAPFILE "tmp/formalizer.metrics-days-categories.map"
#define DEFAULTSYSTEMSELFEVALUATIONDATA "tmp/formalizer.system-self-evaluation-data.csv"
#define DEFAULTMETRICSRCREPRODUCTION "tmp/formalizer.metrics-categories-reproduction.rc"
#define DEFAULTNEXTTASKBROWSERPAGE "formalizer.next-task-browser-page.html"
#define DEFAULTFLAGCMD "xnotification \"TTC\" &"
#define DEFAULTATCOMMAND "at now"
#define DEFAULTATCONTROLLER "rxvt -display 'localhost:0.0' -e dil2al -S &"
#define DEFAULT_HTMLFORM_INTERFACE "/cgi-bin/dil2al"
#define DEFAULT_HTMLFORM_METHOD "GET"
#define MAX_RECURSIVE_PP2PD_EXTRACTION_DEPTH 7
#define DILBYID_CACHE_NUMENTRIES_SUSPICIOUS 10

#define DILBYID_FLOAT_FORMAT	"%.4g"

#define ALDAYDISTFUNCLINEAR	0
#define ALDAYDISTFUNCSQUARED	1

#define DEFAULTALDAYEMPHASIS " BGCOLOR=\"#3F005F\""

#define DEFAULTALFOCUSEDHEADER "<HTML>\n<HEAD>\n<TITLE>Active List - Focused: @SUP@</TITLE>\n</HEAD>\n\
<BODY BGCOLOR=\"#000000\" TEXT=\"#F0E0A0\" LINK=\"#AFAFFF\" VLINK=\"#7F7FFF\">\n\
<H1><FONT COLOR=\"#00FF00\">Active List - Focused: @SUP@</FONT></H1>\n\n\
<B>Involvement of DIL topics, I_{proj,top} in [0,1]:</B>\n\
<UL>\n(currently not in use)\n</UL>\n\n\
<B>AL Target Date:</B> @TD@ <B>General Daily Work Time:</B> @GENERAL-WORK-TIME@\n<P>\n\
@PASSED@\
<TABLE WIDTH=\"100%\" CELLPADDING=3 BGCOLOR=\"#4F4F3F\">\n\n"

#define DEFAULTALFOCUSEDTAIL "</TABLE>\n<P>\n\n\
<LI><A HREF=\"@ALFILE@.wide.html\">Active List - Wide: @SUP@</A>\n\
@CUMULATIVE@\
<LI><A HREF=\"../lists.html#AL\">Active Lists</A>\n\
<LI><A HREF=\"../lists.html#DIL\">Topical Detailed Items Lists</A>\n\
<P><HR>~/doc/<A HREF=\"../maincont.html\">html</A>/<A HREF=\"../lists.html\">lists</A>/@ALFILE@.html - \
(generated: @ALDATE@)\n\n</BODY>\n</HTML>\n"

#define DEFAULTALFOCUSEDEMPHASIS " BGCOLOR=\"#3F5F00\""

#define DEFAULTALWIDEHEADER "<HTML>\n<HEAD>\n<TITLE>Active List - Wide: @SUP@</TITLE>\n</HEAD>\n\
<BODY BGCOLOR=\"#000000\" TEXT=\"#F0E0A0\" LINK=\"#AFAFFF\" VLINK=\"#7F7FFF\">\n\
<H1><FONT COLOR=\"#00FF00\">Active List - Wide: @SUP@</FONT></H1>\n\n\
<OL><B>DIL Reference</B>\n"

#define DEFAULTALWIDETAIL "</OL>\n\n<P>\n\
<LI><A HREF=\"@ALFILE@.html\">Active List - Focused: @SUP@</A>\n\
@CUMULATIVE@\
<LI><A HREF=\"../lists.html#AL\">Active Lists</A>\n\
<LI><A HREF=\"../lists.html#DIL\">Topical Detailed Items Lists</A>\n\
<P>\n<HR>\n\
~/doc/<A HREF=\"../maincont.html\">html</A>/<A HREF=\"../lists.html\">lists</A>/@ALFILE@.wide.html - \
(generated: @ALDATE@)\n\n</BODY>\n</HTML>\n"

// *** optionally remove the <TH>Excerpt</TH> part once T_{pref} is used
//     to determine preferable shifts
#define DEFAULTALCUMULATIVEREQHEADER "<HTML>\n<HEAD>\n<TITLE>Active List - Cumulative Time Required: @SUP@</TITLE>\n</HEAD>\n\
<BODY BGCOLOR=\"#000000\" TEXT=\"#F0E0A0\" LINK=\"#AFAFFF\" VLINK=\"#7F7FFF\">\n\
<H1><FONT COLOR=\"#00FF00\">Active List - Cumulative Time Required: @SUP@</FONT></H1>\n\n\
Tips:\n<UL>\n<LI>Variable target dates are for those tasks easiest to move to update the AL\n<LI>Fixed target dates are for tasks where the target date matters enough that additional resources or other solutions might be better than changing the target date.\n<LI>Periodic tasks (exact or fixed) provide regular review, which differs from single tasks where separation into different work chunks is done by AL generation.\n<LI>Cram tasks only as needed, i.e. cram them where it fulfills requirements to achieve Objectives on time. Cramming in other tasks can only make achieving Objectives on time more difficult.\n</UL>\n\
<P>\n\n\
<FORM METHOD=\"@FORMMETHOD@\" ACTION=\"@FORMACTION@\"><INPUT type=\"hidden\" name=\"dil2al\" value=\"U\">\n\
<INPUT type=\"submit\" value=\"Update Unlimited Periodic Task Target Dates\"></FORM>\n\n\
<P>\n\n\
Assumed hours available per day for the EPS method: <FONT COLOR=\"#FFFF00\">@EPSHRS@</FONT>\n<P>\n\
<FORM METHOD=\"@FORMMETHOD@\" ACTION=\"@FORMACTION@\"><INPUT type=\"hidden\" name=\"dil2al\" value=\"MEtgroup\">\n\
<INPUT type=\"submit\" name=\"skip\" value=\"Skip\"> Unlimited Periodic Task Target Dates up to time (default=current): <INPUT type=\"text\" name=\"T\" maxlength=12> (see <A HREF=\"../../../.dil2al-normal-skip-excludes\">~/.dil2al-normal-skip-excludes</A>)\n\n\
<TABLE CELLPADDING=3 BGCOLOR=\"#4F4F3F\">\n\n\
<TR><TH>GRP</TH><TH>ID</TH><TH>DIL (primary)</TH><TH>Target Date</TH><TH>Treq</TH><TH>EPS TD (chkbx=no update)</TH><TH>Excerpt</TH>\n\n"

#define DEFAULTALCUMULATIVEREQTAIL "</TABLE>\n\
<INPUT type=\"radio\" name=\"TDupdate\" value=\"group\" checked> Minimize fragmentation <INPUT type=\"radio\" name=\"TDupdate\" value=\"direct\"> Direct<BR>\n\
<INPUT type=\"submit\" name=\"update\" value=\"Update\"> <INPUT type=\"reset\"></FORM>\n\n<P>\n\
<LI><A HREF=\"@ALFILE@.html\">Active List - Focused: @SUP@</A>\n\
<LI><A HREF=\"@ALFILE@.wide.html\">Active List - Wide: @SUP@</A>\n\
<LI><A HREF=\"../lists.html#AL\">Active Lists</A>\n\
<LI><A HREF=\"../lists.html#DIL\">Topical Detailed Items Lists</A>\n\
<P>\n<HR>\n\
~/doc/<A HREF=\"../maincont.html\">html</A>/<A HREF=\"../lists.html\">lists</A>/@ALFILE@.ctr.html - \
(generated: @ALDATE@)\n\n</BODY>\n</HTML>\n"

#define VIRTUALOVERLAPSFILEHEADER "<HTML>\n<HEAD>\n<TITLE>Active List - Overlaps of Virtual Task Copies</TITLE>\n</HEAD>\n\
<BODY BGCOLOR=\"#FFFFFF\" LINK=\"#006600\" VLINK=\"#006600\" ALINK=\"#FF0000\">\n\
<H1>Active List - Overlaps of Virtual Task Copies</H1>\n\n\
This lists the DIL ID of each virtual task copy of a periodic task, the target date for which the overlap occurs and the number of task chunks that were not allocated.\n\
<OL>\n"

#define VIRTUALOVERLAPSFILETAIL "</OL>\n\
<P>\n\
<LI><A HREF=\"../lists.html#AL\">Active Lists</A>\n\
<LI><A HREF=\"../lists.html#DIL\">Topical Detailed Items Lists</A>\n\
<P>\n<HR>\n\
~/doc/<A HREF=\"../maincont.html\">html</A>/<A HREF=\"../lists.html\">lists</A>/active-list.virtual-overlaps.html\n\n</BODY>\n</HTML>\n"

#define EXACTCONFLICTSFILEHEADER "<HTML>\n<HEAD>\n<TITLE>Active List - Potential Conflicts between Tasks with Exact Target Dates</TITLE>\n</HEAD>\n\
<BODY BGCOLOR=\"#FFFFFF\" LINK=\"#006600\" VLINK=\"#006600\" ALINK=\"#FF0000\">\n\
<H1>Active List - Potential Conflicts between Tasks with Exact Target Dates</H1>\n\n\
This lists the DIL ID of each task with an exact target date that may conflict with the target date of another task (or virtual copy of a periodic task) with an exact target date, as well as the number of task chunks that were not allocated.\n\
<OL>\n"

#define EXACTCONFLICTSFILETAIL "</OL>\n\
<P>\n\
<LI><A HREF=\"../lists.html#AL\">Active Lists</A>\n\
<LI><A HREF=\"../lists.html#DIL\">Topical Detailed Items Lists</A>\n\
<P>\n<HR>\n\
~/doc/<A HREF=\"../maincont.html\">html</A>/<A HREF=\"../lists.html\">lists</A>/active-list.exact-conflicts.html\n\n</BODY>\n</HTML>\n"

/*
======== 10) GLOBAL PARAMETERS AND VARIABLES DECLARATIONS
*/

extern const char * _dil2al_usage;
extern const char * _formalizer_usage;
extern String runnablename;
extern String runningcommand; // Which command request is running, prepended with runnablename for use in UI_ output.
extern bool noX;
extern bool stricttimestamps; // This makes functions like time_stamp_time() exit for bad time stamps.
extern String homedir;
extern String basedir;
extern String listfile;
extern String idfile;
extern String outputlog;
extern String lockidfile;
extern String crcidfile;
extern String cacheidfile;
extern String showmetricsflagfile;
extern String tasklog;
extern String taskloghead;
extern String firstcallpagetemplate;
extern String firstcallpagetarget;
extern String paperplansfile;
extern String poolfile;
extern String ooplannerfile;
extern String figuresdirectory;
extern String dilref;
extern String normalskipexcludesfile;
extern String bibindexfilename;
extern String bibindexfile;
extern String articleheadtemplate;
extern String articletailtemplate;
extern String templatearticlesubsection;
extern String templatearticlesubsubsection;
extern ifstream dinfile;
extern istream * din; // data input (default cin)
extern ofstream eoutfile;
extern ostream * eout; // error output (default cerr)
extern ostream * vout; // verbouse output (default cout)
extern char projid; // randomized using the current time for use in UI_ functions
extern UI_Info * info; // user interface hook for info output
extern UI_Info * error; // user interface hook for error output
extern UI_Info * warn; // user interface hook for warn output
extern UI_Joined_Yad * ui_joined_yad; // may be used with UI_Info_Yad
extern UI_Entry * input;
extern UI_Confirm * confirm; // user interface hook for confirmations
extern UI_Options * options; // user interface hook for options selection
extern UI_Progress * progress; // user interface hook for progress indicators
extern int ui_info_warn_timeout;
extern bool showuicounter;
extern bool yad_canceled; // A global flag that can be set to signal if a yad window was canceled.
extern int yad_pclose_retval;
extern int numDILs;
extern Topical_DIL_Files_Root dil; // list of topical DILs
extern bool verbose;
extern bool extra_verbose;
extern bool simplifyuserrequests;
extern bool suggestnameref;
extern bool askprocesstype;
extern bool offercreate;
extern bool askALDILref;
extern bool showflag;
extern bool isdaemon;
extern bool useansi;
extern bool askextractconcat;
extern bool usequickloadcache; // use binary cache files for quick loading
extern bool immediatecachewriteback; // immediately write changes back to the cache
extern bool formguaranteestdinout; // guarantee that stdin & stdout are used when called from FORM
extern time_t emulatedtime; // possible alternative current time
extern String curdate, curtime;
extern String owner;
extern int timechunksize;
extern int timechunkslack;
extern int timechunkoverstrategy; // new time chunk decision strategy when over timechunksize+timechunkslack
extern int timechunkunderstrategy; // new time chunk decision strategy when under timechunksize
extern int sectionsize;
extern int sectionstrategy; // new TL section decision strategy
extern long generatealtcs; // number of AL TCs to generate (0=complete)
extern time_t generatealmaxt; // maximum date to which to general an AL
extern int alfocused; // number of AL TCs to show in the shorter listing
extern int alfocusedCALexcerptlength; // character columns of TCs in Cal._Day
extern int alfocusedCALdays; // number of days to show in top-calendar format
extern int alfocusedCALcolumns; // number of columns to squeeze the top-calendar into
extern int alfocusedexcerptlength; // character columns of TCs in AL list
extern int alwide; // number of AL TCs to show in the longer listing
extern int algenregular; // generic number of hours in a regular AL-day
extern int algenall; // generic number of hours in an AL-day for an AL with all DIL-entries
extern float alepshours; // assumed available hours per day for EPS scheduling method
extern bool alautoexpand; // automatically expand number of hours per day as necessary
extern bool alshowcumulativereq; // show cumulative time required
extern int alcumtimereqexcerptlength; // length of DIL entry text excerpt in row of Cumulative Time Required form
extern int alCRTlimit; // test used to determine if subsequent entries should be processed for the Cumulative Time Required form
extern int alCTRdays; // a number of days limits for the CTR AL page (used if alCTRlimit is set to CTRDAYS)
extern int alCRTmultiplier; // multiplier to use to have a safe map size for available time chunks
extern time_t alCRTepsgroupoffset; // offset to add to suggested EPS group variable target dates to preserve grouping
extern bool alCTRGoogleCalendar; // create output for the Google calendar and similar calendar formats
extern float alCTRGoogleCalendarvaluationcriterion;
extern float alCTRGoogleCalendartimecriterion;
extern time_t alCTRGoogleCalendardaysextent;
extern time_t alCTRGCtimezoneadjust;
extern bool ctrshowall;
extern int alperiodictasksincludedayslimit; // if positive, limit the number of days for which periodic tasks are included in AL generation
extern bool periodicautoupdate; // automatically update completion ratios and target dates of periodic tasks
extern bool periodicadvancebyskip; // try periodicskipone before periodicautoaupdate to advance periodic target dates
extern bool rapidscheduleupdating; // use RSU instead of EPS options
extern bool targetdatepreferences; // use target date preferences with the RSU method
extern bool prioritypreferences; // separate end-of-day scheduling proposals based on priority rating
extern time_t dolaterendofday; // proposed TD time later in day with prioritypreferences
extern time_t doearlierendofday; // proposed TD time earlier in day with prioritypreferences
extern bool weekdayworkendpreferences; // specific days of the week have preffered target times
extern time_t wdworkendprefs[7]; // preferred end of work day for each day of the week starting with Sunday
extern time_t alworkdaystart; // standard start time of work on a regular work day (seconds since midnight)
extern int alcurdayworkedstrategy; // how to estimate the time already worked during the current day
extern time_t alworked; // time already worked during current day
extern int alautoupdate; // automatically update AL and completion ratios
extern int alautoupdate_TCexceeded; // automatically update AL and completion rations if the Task Chunk size was significantly exceeded
extern time_t alsyncslack; // allowable desynchronization in seconds
extern int aldaydistfunc; // AL Day distribution function
extern float aldaydistslope; // slope of AL Day distribution
extern bool tltrackperformance; // track performance data during task chunk work (statistics may be derived from these for system-results.m)
extern bool reversehierarchy; // branch to superiors instead of dependencies
extern bool hierarchyaddnonobjectives; // add some information about non-Objectives when focusing on Objectives
extern bool dotgraph; // hierarchy output in the dot language of GraphViz
extern int maxdotgraphwidth; // maximum width that should be displayed in GraphViz output
extern bool hierarchyplanparse; // put in some additional processing to display more PLAN entry information
extern bool hierarchysorting; // sort entries at each hierarchical level
extern int hierarchymaxdepth; // default depth of hieararchy (note that FORM input has an additional variable with which to specify this)
extern int hierarchyexcerptlength; // length of DIL entry text excerpt shown in a DIL hierarchy
extern String hierarchylabel; // only include DIL entries with a specific @label:LABEL@
extern String updatenoteineditorcmd;
extern int updatenoteineditor; // update note destinations in open editors
extern String atcommand; // comand used to schedule an at-job
extern String atcontroller; // at-job scheduled controller command
extern String htmlforminterface; // interface to dil2al for HTML form data
extern String htmlformmethod; // HTML form submission method
extern String editor;
extern String browser;
extern String flagcmd;
extern String taskhistorypage;
extern String taskhistorycall;
extern String metricsdayspage;
extern String mainmetricscategoriesfile;
extern String metricsmapfile;
extern String systemselfevaluationdata;
extern String metricsrcreproduction;
extern time_t metricsearliestday;
extern bool metrics_are_ratios;
extern bool labeled_metric_strict_invalidate;
extern String nexttaskbrowserpage;
extern String notetmpfile;
extern String tlentrytmpfile;
extern String configfile;
extern String tltrackperformancefile;
extern String dilpresetfile;
extern String virtualoverlapsfile;
extern String exactconflictsfile;
extern String tareadfile; // prepend this to temporary files used during a search
extern String finnotesfile;
extern String calendarfile;
extern bool calledbyforminput; // flag indicating if dil2al was called by HTML form input
extern int followlinks; // follow links to this depth during a search
extern bool searchprogressmeter; // optionally indicate the search progress in a special file
extern bool immediatesearchoutput; // optionally process search results after each target search
extern bool remotesearch; // allow or disallow searching remote targets
extern StringList quicknotedst;
extern StringList quicknotedsttitle;
extern StringList quicknotedescr;
extern int quicknotedstnum;
extern StringList indexkeys;
extern int indexkeysnum;
extern String newTLID; // remembers newly created TL entry ID
extern bool chunkcreated; // remembers if a new chunk was created
extern time_t ImmediateTD;
extern time_t UrgentTD;
extern time_t NearTermTD;
#ifdef LOOP_PROTECT
extern Detailed_Items_List * looplist; // list of DIL entries for which loops were detected in the DIL hierarchy
#endif

extern int IDOentered;
extern int IDOpassed;

/*
======== 11) EXPORTED FUNCTION DECLARATIONS
 */

// NOTE: These are listed here by the source file they are defined in.
// *** It would be much more legible if these were actually declared in
//     their own .hh header files, which could be included in dil2al.hh.
//     On the other hand... it can be useful to have one place that
//     lists all function declarations for overview.

// dil2al (dil2al.cc)
void rcmd_set(const char * cmdlabel);
String & rcmd_get();
void Set_Parameter(char * lbuf);
String generate_tag(String & tagid);
bool Diagnostic(String diagnosticstr);
void exit_postop(); // This should be called during every program exit!
void Exit_Now(int status);

// utilities (utilities.cc)
double str2decimal(String & s, int sstartloc, int spastendloc);
#ifdef INCLUDE_EXACT_TARGET_DATES
int Get_DIL_ID_File_Superior_TargetDate(String & tfstr, int idloc, int idloc_next, time_t & targetdate, int & tdprop, bool & tdex, periodictask_t & periodicity, int & span, int & every);
#else
int Get_DIL_ID_File_Superior_TargetDate(String & tfstr, int idloc, int idloc_next, time_t & targetdate, int & tdprop);
#endif
int System_Call(String syscall);
int DIL_entry_target_date_qsort_compare(const void * vde1, const void * vde2);
int DIL_Virtual_Matrix_target_date_qsort_compare(const void * vde1, const void * vde2);
int BigRegex_freq(String & str, const BigRegex & rx);
int BigRegex_freq(String & str, String rxstr);
int BigRegex_freq(String & str, const char * rxstr);
String RX_Search_Safe(String s);
time_t Time(time_t * tloc);
String time_stamp_GM(const char * dateformat, time_t t = -1);
time_t time_add_day (time_t t, int days = 1);
time_t time_add_month (time_t t, int months = 1);
int time_day_of_week(time_t t);
int time_month_length(time_t t);
time_t time_add_month_EOMoffset(time_t t);
String date_string();
time_t time_stamp_time(String timestr, bool noexit = false);
time_t time_stamp_time_of_day(String timestr);
time_t time_stamp_time_date(String timestr);
time_t time_stamp_diff(String oldtime,String newtime);
time_t double2time(double t);
bool confirmation(String message, char nondefault, const char * defoption = default_defoption, const char * nondefoption = default_nondefoption);
char auto_interactive(String * autostr, String message, String respchars, StringList & respdescriptors);
String relurl(String srcurl, String dsturl);
String absurl(String srcurl, String relurl);
bool file_is_regular(String filepath);
bool file_is_regular_or_symlink(String filepath);
bool file_is_directory(String filepath);
bool prepare_temporary_file(String tmpfile, String initstr);
//bool detect_temporary_file(String tmpfile); -- see Inline non-member functions section
bool queue_temporary_file(String tmpfile, String initstr);
bool backup_and_rename(String filename, String message, bool isnew = false);
bool read_file_into_String(ifstream & sfl, String & filestr, bool warnnoopen = true);
bool read_file_into_String(String filename, String & filestr, bool warnnoopen = true);
bool read_file_into_String(bool & isnew, String filename, String & filestr);
bool write_file_from_String(String filename, const String & filestr, String message, bool isnew = false);
void read_data_from_stream(istream & ifs, String & dstr, const char * terminate);
bool Lock_File(String lockfile, String msgstr);
bool Unlock_File(String lockfile);
inline int find_delimiter(String & str, int startpos, int lenlimit, char delimiter = '\n');
bool pattern_in_line(const char * pattern, char * lbuf);
bool substring_from_line(const char * pattern, int nsub, String * dilid, const char * lbuf);
bool find_line(ifstream * lf, const char * pattern, char * lbuf, int llen);
int copy_until_line(ifstream * lf, ofstream * of, const char * pattern, char * lbuf, int llen);
int find_in_line(ifstream * lf, const char * pattern, int nsub, String * dilid, const char * before, char * lbuf, int llen);
bool initialize_new_DIL_TL(String initializecmd, bool includeinput);
int D2A_get_tag(String & d2astr, int strloc, String d2atag, String & tagtext);
int HTML_get_marker(String & htmlstr, int strloc, String & marker,bool locate = true);
String HTML_marker_arguments(String & marker);
int HTML_get_tagged_environment(const BigRegex & begintag, const BigRegex & endmatch, const BigRegex & endtag, String & htmlstr, int strloc, String & envparams, String & envcontent, int * envstart = NULL);
int HTML_get_table_cell(const String & htmlstr, int strloc, String & cellparams, String & cellcontent, int * cellstart = NULL);
int HTML_get_table_row(String & htmlstr, int strloc, String & rowparams, String & rowcontent, int * rowstart = NULL);
int HTML_get_list(String & htmlstr, int strloc, String & listparams, String & listcontent, int * liststart = NULL);
int HTML_get_list_item(String & htmlstr, int strloc, String & itemparams, String & itemcontent, int * itemstart = NULL);
int HTML_get_href(String & htmlstr, int strloc, String & hrefurl, String & hreftext, int * hrefstart = NULL);
int HTML_get_name(String & htmlstr, int strloc, String & nametag, String & nametext);
String * HTML_safe(String & textstr);
String * HTML_remove_comments(String & htmlstr);
String * HTML_remove_tags(String & htmlstr);
String HTML_put_structure(String tag, String params, String content, bool relaxedhtml = false);
String HTML_put_table_row(String rowparams, String rowcontent, bool relaxedhtml = false);
String HTML_put_table_cell(String cellparams, String cellcontent, bool relaxedhtml = false);
String HTML_put_list(char listtype,String htmlstr);
String HTML_put_list_item(String htmlstr, bool relaxedhtml = false);
String HTML_put_href(String hrefurl, String hreftext);
String HTML_put_name(String nametag, String nametext);
String HTML_put_form(String formmethod, String formaction, String formcontent);
String HTML_put_form_input(String inputtype, String inputargs);
String HTML_put_form_submit(String submittext);
String HTML_put_form_reset();
String HTML_put_form_radio(String radioname, String radiovalue, String radiotext, bool checked = false);
String HTML_put_form_checkbox(String checkboxname, String checkboxvalue, String checkboxtext, bool checked = false);
String HTML_put_form_text(String textname, String textvalue, long maxlen = -1);
String HTML_put_form_text(String textname, long maxlen = -1);
int Elipsis_At(String & s, unsigned int maxsize);
String URI_unescape(String escapedURI);
String Int2HexStr(unsigned int i);
String * remove_whitespace(String & str);
String * delete_all_whitespace(String & str);
String * escape_for_single_quote_cmdarg(String & str);
int get_int(String valstr, int defval = 0, bool * gotint= NULL);
double get_double(String valstr, double defval = 0.0, bool * gotdouble = NULL);

// AL compilation (alcomp.cc)
int get_topic_keywords();
String generate_AL_full_entry(String dilid, int chunkfreq);
String generate_AL_entry(String dilid, bool ishighestpriority = false);
String AL_Day_Marker(String & dstfile, AL_Day * ald);
String AL_Passed_Target_Dates(String & dstfile, AL_TD * atd);
bool update_main_ALs(DIL_entry * superior);
float linear_distribution(float i, float i_limit);
long * DIL_Required_Task_Chunks(DIL_entry ** dep, int deplen);
void Get_Worked_Estimate();
float linear_day_distribution(AL_Day * ald,time_t td);
time_t Add_to_Date(time_t td, periodictask_t period, int every);
bool generate_AL(DIL_entry ** dep, DIL_entry * superior = NULL);
int get_AL_entry(String & alstr, int alpos, String & rowcontent, int & rowstart, int & rowend);
bool remove_AL_TC(String alref, String dilid);
bool update_DIL_to_AL();
bool refresh_quick_load_cache();
bool create_pool();
void Objective_Oriented_Planning(String cmdarg, StringList & qlist);

// DIL administration (diladmin.cc)
bool get_active_DIL_IDs(StringList & idfileids, unsigned long * resnumidfileids = NULL);
bool add_new_to_DIL(String dilfile, StringList * supinfoptr);
bool create_new_DIL(String dilfile);
bool send_dil_to_DIL(String dilfile);
String get_new_DIL_ID();
bool generate_modify_element_FORM_interface(StringList * qlist = NULL);
bool modify_element_through_FORM_interface(StringList * qlist = NULL, bool periodicautoupdate_stopTLchunk = false);
bool Update_Unlimited_Periodic_Target_Dates();
bool Skip_Unlimited_Periodic_Target_Dates(StringList * exclude = NULL);
bool update_DIL_entry_elements(String dilid, double valuation, bool updateval, double completion, bool updatecomp, double required, bool updatereq, time_t tdiff = -1, bool periodicautoupdate_stopTLchunk = false);
bool update_DIL_entry_parameter_elements(Detailed_Items_List & dilist, DIL_ID dilid, PLLRoot<DIL_Superiors> & addsups, StringList * removesups, int removesupsnum);
bool modify_DIL_entry_target_date(DIL_ID dilid, time_t tdate, Detailed_Items_List & dilist);
bool modify_DIL_entry_target_date(DIL_ID dilid, time_t tdate);
bool modify_DIL_group_target_dates(Detailed_Items_List & dilgroup, bool minfragmentation = true);
bool modify_DIL_group_target_dates_cmd(bool minfragmentation, StringList * qlist = NULL);
bool extract_DIL_IDs_from_form(StringList * qlist = NULL);
void Show_DIL_Hierarchy(DIL_entry * topde, DIL_Visualize & dv, const int maxdepth, int depth = 0, int width = 0, DIL_entry * prede = NULL, double likelihood = DILSUPS_REL_UNSPECIFIED);
String Tabbed_DIL_Hierarchy(String dilidstr, int maxdepth = 50);
String Tabbed_HTML_DIL_Hierarchy(String dilidstr, int maxdepth = 50);
String Tabbed_FORM_DIL_Hierarchy(String dilidstr, int maxdepth = 50);
String GRAPH_DIL_Hierarchy(String dilidstr, int maxdepth = 50, char graphtype = 'D');
bool DIL_Hierarchy_cmd(String dilidstr, int maxdepth = 50);
void Get_Metrics(Detailed_Items_List & dilist, String & simplestring, const int maxdepth, String expressions);
String Metrics_List(String dilidstr, int maxdepth = 50);
bool DIL_Metrics_cmd(String dilidstr, int maxdepth = 50);

// TL administration (tladmin.cc)
String get_TL_head();
int max_chunk_entry(String & tlchunk);
String generate_TL_chunk_header(String nametag, String newalref, String altitle, String alhead, String newdilref, String diltitle, String dilhead, int generateparts = 0);
String generate_TL_entry_header(String nametag, String newalref, String altitle, String alhead, String newdilref, String diltitle, String dilhead);
String get_most_recent_TC_AL(const String & tl);
String get_most_recent_TC_DE(const String & tl);
bool stop_TL_chunk(String & tl);
String select_TL_DIL_refs(String & tl, bool newchunk, String newalref);
String select_TL_AL_refs(String & tl, bool newchunk, String newdilref);
String add_TL_chunk_or_entry(String & notestr, bool newchunk, TL_chunk_access & TL);
bool add_TL_section(String & tl);
int locate_most_recent_TL_chunk(String & tl, int seekfrom, String * chunkid);
bool decide_add_TL_chunk(TL_chunk_access & TL, bool comparetimes, bool createifgreater = true);
void track_performance(String & dilid, time_t tdiff, double completion, double required);

// note processing (note.cc)
void process_HTML(String & notestr);
int pre_process_note(String notefile, String notedst, String typestr, String & notestr, String & notedststr, bool & isnew);
int post_process_note(String notedst, String & notestr, String & notedststr, int insertindex, bool isnew);
int process_note_TL(String notefile, String notedst);
int process_note_HTML(String notefile, String notedst);
int process_note_TeX(String notefile, String notedst);
int process_note_CC(String notefile, String notedst);
int process_note_generic(String notefile, String notedst);
bool process_note(String notefile, String notedst);
bool quick_select_target(String & notedst, String & notedsttitle);
bool make_note(String initnotefromfile);

// chunk controller (controller.cc)
String chunk_controller(String controllercmd);
time_t time_chunk_remaining(String chunkid);
bool schedule_controller(String controllercmd);
bool first_call_page();
bool second_call_page();

// TL filtering (tlfilter.cc)
bool filter_TL_by_AL();
bool generate_TL_keyword_index();
bool search_TL(String searchstr);
bool extract_paper_novelties(String fname, String paperplanurl, ostream & ostr);
bool cmdline_extract_paper_novelties(String filepaper);
bool get_collected_task_entries(String dilidstr, String collectionfile);
bool metrics_parse_task_log(String labelid);
bool System_self_evaluation(Metrics_Categories & MC, Metrics_Days & MD);
  
// Paper plan filtering (ppfilter.cc)
String generate_Figures_Directory_entry(String figid);
void get_figure_data(String & figfile, String & figlabel, String & figcaption, String & figintext, String &psconversions, StringList & srcf, StringList & srcfname, int & srcfnum);
bool get_Paper_Plans_Section_IDs(StringList & ppsectionids, StringList & ppsectiontitles, StringList & ppsectiontemplates);
int TeX_get_scope(String & textstr, int start, String & scopecontent);
int TeX_get_argument(String & textstr, int start, String & argstr, char argtype, int * argstart = NULL);
int TeX_get_command(String & textstr, int start, String & cmdstr);
String TeX_safe_text(String & title);
void TeX_format_corrections(String & ostrtext, bool estimate = false);
int Text_find_line_end(String & textstr, int start, char lend = '\n');
void Text_limit_line_length(String & textstr, int optlinemax, int abslinemax, char lend = '\n');
void convert_item_content_to_TeX(String & itemcontent);
void unconvert_item_content_from_TeX(String & itemcontent);
bool extract_paper_plan_to_paper(String ppname, String pout, String pprevious);
bool cmdline_extract_paper_plan_to_paper(String ppfile);
bool Quantitative_Assessment(char qatype, String ppfile);

// Searching (search.cc)
String search(StringList & targets, String searchkey, bool stringoutput = true);
unsigned long Get_Target_Directory(StringList & targets, unsigned long i, String dirpath);
unsigned long Get_Target_Regex(StringList & targets, unsigned long i, String dirpath);
bool search_cmd(String grepinfo);
bool search_FORM_cmd(StringList * qlist = NULL);

// Finances (finances.cc)
bool read_financial_notes_file(PLLRoot<Financial_Note> & fnotes, String & finnotestop, String & finnotesend);
bool write_financial_notes_file(PLLRoot<Financial_Note> & fnotes, String & finnotestop, String & finnotesend);
bool financial_notes();
bool financial_monthly();

// Interfaces (interface.cc)
void Output_Log_Append(const String & s);
void Output_Log_Append(const char * s);
void Output_Log_Append(char c);
void Output_Log_Flush();

// Wizard guides (wizard.cc)
bool System_Metrics_AL_morning_wizard();

// Test code (testcode.cc)
bool test_code();

/*
======== 12) INLINE MEMBER FUNCTIONS ========
*/

inline void UI_Info_Yad::Join(ui_joined_t _joined) {
  joined = _joined; // NOTE: If this is ui_NOT_JOINED it does not tell ui_joined_yad about it.
  if (joined!=ui_NOT_JOINED) {
    if (!ui_joined_yad) ui_joined_yad = new UI_Joined_Yad();
    ui_joined_yad->Join(joined,(*this));
  }
}

inline String * Topical_DIL_Files::operator[](int n) {
	Protected_Linked_List_Indexed_Access(Topical_DIL_Files,File(),File());
}

inline void Topical_DIL_Files::Invalidate() {
	updated=false;
	if (Prev()) Prev()->Invalidate();
	if (Next()) Next()->Invalidate();
}

// Refreshes from the head if accessed components needed to be updated
#define Topical_DIL_Files_Updated_Access(Here,There,ThereSteps) \
	if (updated) { \
		Protected_Linked_List_Indexed_Access(Topical_DIL_Files,Here,There); \
	} \
	if (!Prev()) { \
		if (Refresh()) return ThereSteps(n); \
		else return NULL; \
	} else { \
		Invalidate(); \
		return Prev()->ThereSteps(n+1); \
	}


inline String * Topical_DIL_Files::File(int n) {
	Topical_DIL_Files_Updated_Access(&(ft.file),File(),File)
}

inline String * Topical_DIL_Files::Title(int n) {
	Topical_DIL_Files_Updated_Access(&(ft.title),Title(),Title)
}

inline filetitle_t * Topical_DIL_Files::Topic(int n) {
	Topical_DIL_Files_Updated_Access(&ft,Topic(),Topic)
}

inline DIL_entry * DIL_AL_List::Superiorbyid() {
// assumes entry is in Detailed_Items_List
// returns pointer to superior if found, pointer to self (*entry) if
// no valid DIL ID was given, or NULL if the valid DIL ID could not
// be found in the Detailed_Items_List
	if (!superior) { // fill the cache
		DIL_ID did(al.title);
		if (did.valid()) superior = entry->head()->elbyid(did);
		else superior = entry;
	}
	return superior;
}

inline DIL_entry * Plan_entry_content::Decision_Ptr() {
  if (!decisionptr) decisionptr=entry->elbyid(decisionID);
  return decisionptr;
}

inline bool DIL_AL_List::IsSelfReference() {
	DIL_ID did(al.title);
	return ((entry->str()==al.title) || (!did.valid()));
}

/*
======== 13) INLINE NON-MEMBER FUNCTIONS ========
 */

inline bool detect_temporary_file(String tmpfile) {
  // Checks if tmpfile exists. If it does, returns true.
  if (access(tmpfile,F_OK) == 0) return true;
  return false;
}

inline void ANSI_underline_on() {
  if (useansi) cout << ANSI_UNDERLINE_ON;
}

inline void ANSI_underline_off() {
  if (useansi) cout << ANSI_UNDERLINE_OFF;
}

inline void ANSI_bold_on() {
  if (useansi) cout << ANSI_BOLD_ON;
}

inline void ANSI_bold_off() {
  if (useansi) cout << ANSI_BOLD_OFF;
}
 
#endif
