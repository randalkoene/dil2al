// interface.cc
// Randal A. Koene, 20190202

#include "dil2al.hh"

/*
  User interaction in dil2al shouldn't directly call VOUT or readline, etc.
  Instead, it should use functions that are specific to the type of information,
  e.g. Info, Warning, Error, Confirmation, Option, Menu, Chooser, etc.

  Those type-specific functions can then be overloaded depending on configuration
  settings, command line arguments, etc.

  They can operate in multiple ways, e.g. replicating the classic way, prepending
  a code, so that external parses can decide how to work with the input/output,
  or even call internal or external gui tools.
*/

/*
  Interface options provided here need to play nice with the -O option, so that
  the different ways to call dil2al make sense.
 */

/*
  This can get very meta. For example, instead of doing a system call straight
  to Yad, a system call can be done to a String infodisplaycmd. That, in turn,
  could call a script ~/local/bin/dil2al-info-display.sh that takes the input
  arguments and sends that to Yad or Zenity, or even to another server.

  This means, there will be 2 ways to accomplish any sort of display set:
  1) Define its functions directly in interface.cc and dil2al.hh, and make
     it a selectable option.
  2) Define the function set in terms of external programs/scripts, make
     those external calls configurable, and select the UI_Info derived class
     that uses the external call configuration.
 */

/*
  Setting up some kind of 'flush' stream manipulator, so that it's fine to
  write INFO << "this info" << flush; would be great, but the matter of
  writing your own stream manipulators is unnecessarily complicated for this
  version.
 */

// ** INFO should be a) collected and b) non-blocking (like a window in its own detached process, possibly with timeout
// ** ERROR should be a) collected, b) displayed together upon exit, and c) can be blocking or non-blocking,
//    or... it should be a) shown immediately
// ** WARN should be a) collected and b) non-blocking, possibly with timeout (like INFO, but a different style)

bool yad_canceled = false; // A global flag that can be set to signal if a yad window was canceled.
int yad_pclose_retval = 0;

static const char * _outputlog_head = "<HTML>\n<HEAD>\n<TITLE>dil2al - Output Log</TITLE>\n</HEAD>\n<BODY>\n<H1>dil2al - Output Log</H1>\n\n<PRE>\n";
static const char * _outputlog_tail = "\n</PRE>\n\n</BODY>\n</HTML>\n";
String _outputlogstr(_outputlog_head); // *** It could be smart to pre-allocate a solid buffer size to this, since it is appended often.

long ui_info_counter = 0; // This counter can provide temporal order information.
String ui_info_count_delimiter(": "); // *** Can make this a configurable parameter if desired.

// This is cycled through to generate new shared memory keys in case
// previously used shared memory hangs around too long.
key_t k;
String keystr;

void Output_Log_Append(const String & s) {
  // Append s to the HTML output log file for this run of dil2al.
  // s typically refers to a string that was also sent to a UI_Info
  // object.

  _outputlogstr += s;
}

void Output_Log_Append(const char * s) {
  // Append s to the HTML output log file for this run of dil2al.
  // s typically refers to a string that was also sent to a UI_Info
  // object.

  if (s) _outputlogstr += s;
}

void Output_Log_Append(char c) {
  // Append s to the HTML output log file for this run of dil2al.
  // s typically refers to a string that was also sent to a UI_Info
  // object.

  _outputlogstr += c;
}

void Output_Log_Flush() {
  // Flush the contents of _outputlogstr to the file outputlog.
  // This appends closing tags to form a proper HTML file.
  //
  // NOTE A: Unlike some UI_Info derived ::flush() functions,
  // this output is NOT modified to be usable as a command line
  // argument.
  // *** It is also not made HTML safe, but perhaps it should be!
  // *** In fact, 2 problems need to be solved simultaneously:
  //     1) we want some HTML directives to pass unaltered, such
  //        as <a href...></a>, but
  //     2) we want other HTML characters to be escaped, such as
  //        when <...> is used in _dil2al_usage.
  //
  // NOTE B: This function should definitely be called in
  // dil2al.cc:exit_postop(), but it can also be called at other
  // times when a fresh output log should be started, such as
  // after a Daemon version of dil2al completes work on one
  // request, as it prepares for the next request in the queue.

  // *** We probably want this to APPEND until the log becomes
  //     greater than some limit. Otherwise, succesive calls,
  //     such as updating firstcallpage right after a dil2al -S
  //     could erase relvant output information too quickly.

  _outputlogstr += _outputlog_tail;
  if (!write_file_from_String(outputlog,_outputlogstr,"",true)) ERROR << "Output_Log_Flush(): Unable to create output log file " << outputlog << ".\n";  
  
  // Now reinitialize the buffer string.
  _outputlogstr = _outputlog_head;
}

String ui_info_count() {
  // Maintains the ui_counter and inserts a count string into
  // UI_ output, so that temporal order is clear across various
  // instances of UI_Info derived classes.
  //
  // NOTE: For the test to work, ui_info_count() must be called
  // before the new output is added to the _outputlogstr. This
  // is why Output_Log_Append() is called after UI_Info append.

  // The global parameter showuicounter controls this.
  
  if (!showuicounter) return "";

  if ((!_outputlogstr.empty()) && (_outputlogstr[_outputlogstr.length()-1]!='\n')) return "";
  
  ui_info_counter++;
  return String(ui_info_counter)+ui_info_count_delimiter;
}

bool UI_Info_Classic::write(const char * s) {
  // Send directly to VOUT
  if (!s) return false;
  UIOUT << ui_info_count() << s;
  Output_Log_Append(s);
  return UIOUT.fail();
}

UI_Info& UI_Info_Classic::operator<<(const char * s) {
  // Send directly to VOUT, don't even redirect to write() first
  if (!s) return static_cast<UI_Info&>(*this);
  UIOUT << ui_info_count() << s;
  Output_Log_Append(s);
  return static_cast<UI_Info&>(*this);
}

UI_Info& UI_Info_Classic::operator<<(char c) {
  // Send directly to VOUT, don't even redirect to write() first
  UIOUT << ui_info_count() << c;
  Output_Log_Append(c);
  return static_cast<UI_Info&>(*this);
}

UI_Info& UI_Info_Classic::operator<<(const String & s) {
  // Send directly to VOUT, don't even redirect to write() first
  UIOUT << ui_info_count() << s; 
  Output_Log_Append(s);
  return static_cast<UI_Info&>(*this);
}

UI_Info& UI_Info_Classic::operator<<(const SubString & s) {
  // Send directly to VOUT, don't even redirect to write() first
  UIOUT << ui_info_count() << s;
  Output_Log_Append(s.chars());
  return static_cast<UI_Info&>(*this);
}

UI_Info& UI_Info_Classic::operator<<(int l) {
  // Send directly to VOUT, don't even redirect to write() first
  UIOUT << ui_info_count() << l;
  Output_Log_Append(String((long) l));
  return static_cast<UI_Info&>(*this);
}

UI_Info& UI_Info_Classic::operator<<(long l) {
  // Send directly to VOUT, don't even redirect to write() first
  UIOUT << ui_info_count() << l;
  Output_Log_Append(String(l));
  return static_cast<UI_Info&>(*this);
}

UI_Info& UI_Info_Classic::operator<<(double d) {
  // Send directly to VOUT, don't even redirect to write() first
  UIOUT << ui_info_count() << d;
  Output_Log_Append(String(d,""));
  return static_cast<UI_Info&>(*this);
}

void UI_Info_Classic::flush() {
  // NOTE: This flush is purposely NOT synchronized or combined
  // with Output_Log_Flush(). Sometimes you want the output log
  // to simply continue collecting perusable output while a
  // UI_Info object needs to be flushed to present information
  // that is relevant in the moment for following user
  // interaction.
  
  UIOUT.flush();
}

/*UI_Info_Classic & flush(UI_Info_Classic & uio) {
  uio.flush();
  return uio;
  }*/

bool UI_Info_Yad::write(const char * s) {
  // Buffer for System_Call() to Yad
  if (!s) return false;
  infostr += ui_info_count() + s; 
  Output_Log_Append(s);
  return true;
}

UI_Info& UI_Info_Yad::operator<<(const char * s) {
  // Buffer for System_Call() to Yad
  if (!s) return static_cast<UI_Info&>(*this);
  write(s);
  return static_cast<UI_Info&>(*this);
}

UI_Info& UI_Info_Yad::operator<<(char c) {
  // Buffer for System_Call() to Yad
  infostr += ui_info_count() + c;
  Output_Log_Append(c);
  return static_cast<UI_Info&>(*this);
}

UI_Info& UI_Info_Yad::operator<<(const String & s) {
  // Buffer for System_Call() to Yad
  write(s);
  return static_cast<UI_Info&>(*this);
}

UI_Info& UI_Info_Yad::operator<<(const SubString & s) {
  // Buffer for System_Call() to Yad
  write(s.chars());
  return static_cast<UI_Info&>(*this);
}

UI_Info& UI_Info_Yad::operator<<(int l) {
  // Buffer for System_Call() to Yad
  write(String((long) l));
  return static_cast<UI_Info&>(*this);
}

UI_Info& UI_Info_Yad::operator<<(long l) {
  // Buffer for System_Call() to Yad
  write(String(l));
  return static_cast<UI_Info&>(*this);
}

UI_Info& UI_Info_Yad::operator<<(double d) {
  // Buffer for System_Call() to Yad
  write(String(d,""));
  return static_cast<UI_Info&>(*this);
}

void UI_Info_Yad::flush() {
  // NOTE: See note in UI_Info_Classic::flush().
  
  if (infostr.empty()) return;
  if ((joined!=ui_NOT_JOINED) && (ui_joined_yad)) {
    ui_joined_yad->flush();
    return;
  }
  
  HTML_safe(infostr);
  escape_for_single_quote_cmdarg(infostr);
  String timeoutstr;
  if (usetimeout) timeoutstr = " --timeout="+String((long) ui_info_warn_timeout)+" --timeout-indicator=top";
  if (System_Call("nohup yad --title='"+rcmd_get()+timeoutstr+"' --no-focus --image="+ui_icon+" --image-on-top --button='gtk-ok:0' --text='"+infostr+"' > /tmp/dil2al-yad-error.log 2>&1 &")<0) {
    EOUT << "UI_Info_Yad::flush() error: System_call() returned error.\n";
  }
  infostr = "";
}

#ifdef UI_JOINED_YAD_LIST_STRINGS
void UI_Joined_Yad::flush() {
  // This combines flushing for all output that has
  // been unified, e.g. INFO+WARN, INFO+WARN+ERROR.
  // All of the joined outputs are flushed when one
  // of the joined members requests flush().
  // If any member does not use a timeout then the
  // resulting display does not time out.
  //
  // NOTE: See note in UI_Info_Classic::flush().

  bool joined_timeout = true;
  int partstoflush = 0;
  String yadstr("nohup yad --list --geometry=640x640-1-1 --no-focus --editable --button='gtk-ok:0' --title='"+rcmd_get()+"' --window-icon="+homedir+String(WINDOWICON)+" --column=Type:IMG --column=Messages:TEXT");

  for (int i=0; i<ui_NOT_JOINED; i++) {
    if (joined[i]) {
      //UI_Info_Yad & uiinfoyad = (*joined[i]);
      if (!(*joined[i]).infostr.empty()) {
	HTML_safe((*joined[i]).infostr);
	escape_for_single_quote_cmdarg((*joined[i]).infostr);
	//	yadstr += " --image="+uiinfoyad.ui_icon+" --image-on-top --text='"+uiinfoyad.infostr+"'";
	yadstr += ' '+(*joined[i]).ui_icon+" '"+(*joined[i]).infostr+"'";
	if (!(*joined[i]).usetimeout) joined_timeout = false;
	(*joined[i]).infostr = "";
	partstoflush++;
      }
    }
  }

  if (partstoflush>0) {
    String timeoutstr;
    if (joined_timeout) timeoutstr = " --timeout="+String((long) ui_info_warn_timeout)+" --timeout-indicator=top";
    yadstr += timeoutstr + " > /tmp/dil2al-yad-error.log 2>&1 &";
    if (System_Call(yadstr)<0) {
      EOUT << "UI_Joined_Yad::flush() error: System_call() returned error.\n";
    } 
  }
}
#endif

#ifdef UI_JOINED_YAD_LIST_STRINGLISTS
void UI_Joined_Yad::flush() {
  // This combines flushing for all output that has
  // been unified, e.g. INFO+WARN, INFO+WARN+ERROR.
  // All of the joined outputs are flushed when one
  // of the joined members requests flush().
  // If any member does not use a timeout then the
  // resulting display does not time out.
  //
  // NOTE: See note in UI_Info_Classic::flush().

  bool joined_timeout = true;
  int partstoflush = 0;
  String yadstr("nohup yad --list --geometry=640x640-1-1 --no-focus --editable --button='gtk-ok:0' --title='"+rcmd_get()+"' --window-icon="+homedir+String(WINDOWICON)+" --column=Type:IMG --column=Messages:TEXT");

  for (int i=0; i<ui_NOT_JOINED; i++) {
    if (joined[i]) {
      //UI_Info_Yad & uiinfoyad = (*joined[i]);
      if (!(*joined[i]).infostr.empty()) {
	HTML_safe((*joined[i]).infostr);
	escape_for_single_quote_cmdarg((*joined[i]).infostr);
	StringList jlist((*joined[i]).infostr,'\n');
	for (StringList * j = &jlist; j!=NULL ; j = j->get_Next()) {
	  yadstr += ' '+(*joined[i]).ui_icon+" '"+j->get_Value()+"'";
	}
	if (!(*joined[i]).usetimeout) joined_timeout = false;
	(*joined[i]).infostr = "";
	partstoflush++;
      }
    }
  }

  if (partstoflush>0) {
    String timeoutstr;
    if (joined_timeout) timeoutstr = " --timeout="+String((long) ui_info_warn_timeout)+" --timeout-indicator=top";
    yadstr += timeoutstr + " > /tmp/dil2al-yad-error.log 2>&1 &";
    if (System_Call(yadstr)<0) {
      EOUT << "UI_Joined_Yad::flush() error: System_call() returned error.\n";
    } 
  }
}
#endif

#ifdef UI_JOINED_YAD_FORM_STRINGS
// In this version, each joined is shown as a single string, but a
// form is used in an attempt to make it scrollable.

#endif

#ifdef UI_JOINED_YAD_TEMPORAL
// In this version, each message is added to a combined StringList
// with Error, Warn, Info labels (possibly kept in a LongList),
// so that all messages are shown in the order received.
// NOTE A: This requires different handling up-front, but then again,
// it might be a good idea to maintain a StringList infostr
// anyway instead of a String infostr.
// NOTE B: The same effect may be accomplished with the separately
// maintained less ephemeral output report in /tmp/dil2al-output.log/html.
#endif

#ifdef UI_JOINED_YAD_NOTEBOOK
// In this version, each joined receives its own tab and within
// that is a text.

int UI_Joined_Yad::build_joined_yad_flush_script() {
  // Builds a sequence of yad command strings for a temporary shell script.
  // Returns the number of streams to flush. If that is zero then
  // there is no script to run.
  // Returns -1 if writing the script to file failed.
  //
  // NOTE A: This is used in UI_Joined_Yad::flush(), which creates a
  // notebook with multiple yad calls. This was initially done via
  // successive System_Call() calls, since there was no need to read
  // combined output, but it turns out that it is better to finish
  // with a call to ipcrm, and that call has to be part of the
  // nohup launch(es) even if dil2al terminates. It is therefore best
  // to combine them all in a script that can run independent of
  // dil2al.
  //
  // NOTE B: Since the script built uses cat to pipe text to
  // yad --info-text it may not be necessary to call
  // escape_for_single_quote_cmdarg() or HTML_safe() on the text
  // first. It won't be used as an argument in a command line.
  // *** 20190214: Testing this.

  k = ftok(listfile,projid);
  keystr = String((long) k);
  if (!(++projid)) projid = 1;

  bool joined_timeout = true;
  int partstoflush = 0;
  String yadstr("#! /bin/bash\n# Generated by dil2al "+curtime+"\n\n");

  //yadstr += "echo \"DID RUN `date`\" > /tmp/didrun\n\n";

  // build the calls for each tab of the notebook
  for (int i=0; i<ui_NOT_JOINED; i++) {
    if (joined[i]) {
      //UI_Info_Yad & uiinfoyad = (*joined[i]);
      if (!(*joined[i]).infostr.empty()) {
	partstoflush++;
	//HTML_safe((*joined[i]).infostr); // *** May not be necessary!
	//escape_for_single_quote_cmdarg((*joined[i]).infostr); // *** May not be necessary!
	yadstr += "cat <<'_E_O_T_' | yad --plug="+keystr+" --tabnum="+String((long) partstoflush)+" --no-markup --text-info > /tmp/dil2al-yad-tab"+String((long) partstoflush)+".log 2>&1 & \n";
	yadstr += (*joined[i]).infostr;
	yadstr += "\n_E_O_T_\n\n";
	if (!(*joined[i]).usetimeout) joined_timeout = false;
      }
    }
  }

  if (partstoflush<=0) return partstoflush;
  
  // if there are streams to flush then write the yad --notebook script
  String timeoutstr;
  if (joined_timeout) timeoutstr = " --timeout="+String((long) ui_info_warn_timeout)+" --timeout-indicator=top";
  yadstr += "yad --notebook --key="+keystr+" ";
  int tabnumber = 1;
  for (int i=0; i<ui_NOT_JOINED; i++) {
    if (joined[i]) {
      if (!(*joined[i]).infostr.empty()) {
	yadstr += " --tab=!" + (*joined[i]).ui_icon;
	tabnumber++;
	(*joined[i]).infostr = "";
      }
    }
  }
  yadstr += " --geometry=640x640-1-1 --no-focus --title='"+rcmd_get();
  yadstr += "' --window-icon="+homedir+WINDOWICON;
  yadstr += " --button=gtk-ok";    
  yadstr += timeoutstr + " > /tmp/dil2al-yad-notebook.log 2>&1 \n\n";

  yadstr += "ipcrm -M "+keystr+" > /tmp/dil2al-ipcrm.log 2>&1 \n\n";
  
  // write script to temporary file
  unlink("/tmp/dil2al-yadrequest.sh"); // remove possible previous temporary script
  if (!write_file_from_String("/tmp/dil2al-yadrequest.sh",yadstr,"",true)) return -1;

  if (chmod("/tmp/dil2al-yadrequest.sh",S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)<0) return -1;

  return partstoflush;
}

void UI_Joined_Yad::flush() {
  // This combines flushing for all output that has
  // been unified, e.g. INFO+WARN, INFO+WARN+ERROR.
  // All of the joined outputs are flushed when one
  // of the joined members requests flush().
  // If any member does not use a timeout then the
  // resulting display does not time out.
  //
  // NOTE A: The actually call to the script that was
  // generated is done in dil2al-info-warn-error.sh.
  // This makes it a lot easier to change aspects of
  // that call without having to recompile dil2al.
  //
  // NOTE B: See note in UI_Info_Classic::flush().

  int partstoflush = build_joined_yad_flush_script();
  if (partstoflush<0) {
    EOUT << "UI_Joined_Yad::flush(): Unable to build and write executable script.\n";
    return;
  }
  if (partstoflush==0) return;
  
  if (System_Call("/home/randalk/local/bin/dil2al-info-warn-error.sh")<0) {
    EOUT << "UI_Joined_Yad::flush(): System_Call() returned error.\n";
  }
}
#endif

/* UI_Info_Yad & flush(UI_Info_Yad & uio) {
  uio.flush();
  return uio;
  }*/

bool UI_Info_External::write(const char * s) {
  // Buffer for System_Call() to infocmd
  if (!s) return false;
  infostr += ui_info_count() + s;
  Output_Log_Append(s);
  return true;
}

UI_Info& UI_Info_External::operator<<(const char * s) {
  // Buffer for System_Call() to infocmd
  if (!s) return static_cast<UI_Info&>(*this);
  write(s);
  return static_cast<UI_Info&>(*this);
}

UI_Info& UI_Info_External::operator<<(char c) {
  // Buffer for System_Call() to infocmd
  infostr += ui_info_count() + c;
  Output_Log_Append(c);
  return static_cast<UI_Info&>(*this);
}

UI_Info& UI_Info_External::operator<<(const String & s) {
  // Buffer for System_Call() to infocmd
  write(s);
  return static_cast<UI_Info&>(*this);
}

UI_Info& UI_Info_External::operator<<(const SubString & s) {
  // Buffer for System_Call() to infocmd
  write(s.chars());
  return static_cast<UI_Info&>(*this);
}

UI_Info& UI_Info_External::operator<<(int l) {
  // Buffer for System_Call() to infocmd
  write(String((long) l));
  return static_cast<UI_Info&>(*this);
}

UI_Info& UI_Info_External::operator<<(long l) {
  // Buffer for System_Call() to infocmd
  write(String(l));
  return static_cast<UI_Info&>(*this);
}

UI_Info& UI_Info_External::operator<<(double d) {
  // Buffer for System_Call() to infocmd
  write(String(d,""));
  return static_cast<UI_Info&>(*this);
}

void UI_Info_External::flush() {
  // NOTE: See note in UI_Info_Classic::flush().
  
  if (infostr.empty()) return;
  if (infocmd.empty()) return;
  String timeoutstr;
  if (usetimeout) timeoutstr = " --timeout="+String((long) ui_info_warn_timeout);
  escape_for_single_quote_cmdarg(infostr);
  if (System_Call("nohup "+infocmd+" --title='"+rcmd_get()+'\''+timeoutstr+" --text='"+infostr+"' > /dev/null 2>&1 &")<0) {
    EOUT << "UI_Info_External::flush() error: System_call() returned error.\n";
  }
  infostr = "";
}

String UI_Entry_Classic::request(const char * message, bool allowcancel) {
  // Returns the string entered, up to but not including the newline character.
  // This derived class does not support a 'Cancel' option, but in classical terminal
  // mode that can be accomplished with CTRL+C (allowcancel is ignored). 
  
  if (message) UIOUT << message;

  // *** Should the following be read from cin instead of DIN for safety?
  const int LLEN = 10240;
  char lbuf[LLEN];
  DIN.getline(lbuf,LLEN); // lbuf will contain at least '\0'
  return lbuf;
}

String UI_Entry_Yad::request(const char * message, bool allowcancel) {
  // Returns the string entered, up to but not including any newline character.
  // Returns the empty string "" if canceled and sets the global flag yad_canceled.

  yad_canceled = false;
  
  if (!message) {
    yad_canceled = true;
    return "";
  }
  
  String msg(message);
  HTML_safe(msg);
  escape_for_single_quote_cmdarg(msg);
  String cancelbutton(" --button='Cancel:2'");
  if (!allowcancel) cancelbutton = "";

  const int LLEN = 10240;
  char lbuf[LLEN];
  FILE *fp;

  fp = popen("yad --entry --title='"+rcmd_get()+"' --window-icon="+homedir+String(WINDOWICON)+" --buttons-layout=spread --button='OK:0'"+cancelbutton+" --text='"+msg+"' 2> /tmp/dil2al-yad-error.log", "r");
  if (fp == NULL) {
    EOUT << "UI_Entry_Yad::request(): popen() returned error.\n";
    yad_canceled = true;
    return "";
  }
  
  StringList yadresults;
  int numyadresults = 0;
  while (fgets(lbuf, LLEN, fp) != NULL) yadresults[numyadresults++] = lbuf;
  yad_pclose_retval = pclose(fp);
  if (yad_pclose_retval<0) {
    ERROR << "UI_Entry_Yad::request(): Error detected by pclose().\n";
    yad_canceled = true;
    return "";
  }
  yad_pclose_retval = WEXITSTATUS(yad_pclose_retval);

  if (yad_pclose_retval!=0) {
    yad_canceled = true;
    return "";
  }

  if (!yadresults[0].empty())
    if (yadresults[0].lastchar()=='\n')
      yadresults[0].del((_BIGSTRING_SIZE) yadresults[0].length()-1,1);
  
  return yadresults[0];
}

String UI_Entry_External::request(const char * message, bool allowcancel) {
  // Returns the string entered, up to but not including any newline character.
  // Returns the empty string "" if canceled and sets the global flag yad_canceled.

  yad_canceled = false;
  
  if (!message) {
    yad_canceled = true;
    return "";
  }
  
  String msg(message);
  HTML_safe(msg);
  escape_for_single_quote_cmdarg(msg);
  String cancelbutton(" --button='Cancel:2'");
  if (!allowcancel) cancelbutton = "";

  const int LLEN = 10240;
  char lbuf[LLEN];
  FILE *fp;

  fp = popen(entrycmd+"--title='"+rcmd_get()+"' --button='OK:0'"+cancelbutton+" --text='"+msg+"' 2> /tmp/dil2al-yad-error.log", "r");
  if (fp == NULL) {
    EOUT << "UI_Entry_Yad::request(): popen() returned error.\n";
    yad_canceled = true;
    return "";
  }
  
  StringList yadresults;
  int numyadresults = 0;
  while (fgets(lbuf, LLEN, fp) != NULL) yadresults[numyadresults++] = lbuf;
  yad_pclose_retval = pclose(fp);
  if (yad_pclose_retval<0) {
    ERROR << "UI_Entry_Yad::request(): Error detected by pclose().\n";
    yad_canceled = true;
    return "";
  }
  yad_pclose_retval = WEXITSTATUS(yad_pclose_retval);

  if (yad_pclose_retval!=0) {
    yad_canceled = true;
    return "";
  }

  if (!yadresults[0].empty())
    if (yadresults[0].lastchar()=='\n')
      yadresults[0].del((_BIGSTRING_SIZE) yadresults[0].length()-1,1);
  
  return yadresults[0];
}



int UI_Confirm_Classic::request(const char * message, char nondefault, const char * defoption, const char * nondefoption, bool allowcancel) {
  // Returns 1 if the first character of the answer was the nondefault character.
  // Returns 0 if for any other answer (i.e. default behavior chosen).
  // This derived class does not support a 'Cancel' option, but in classical terminal
  // mode that can be accomplished with CTRL+C (allowcancel is ignored). 
  // This derived class does not explicitly use defoption and nondefoption.
  //
  // NOTE A: This function IS case sensitive for the nondefault character.
  // Whitespace is expressly filtered out.
  //
  // NOTE B: I tried to use readline(DIN,resp) here with String resp, but unfortunately
  // that does not behave well with empty responses, such as simply pressing ENTER
  // (see comment in BigString.cc:readline()).
  
  if (message) UIOUT << message;

  // *** Should the following be read from cin instead of DIN for safety?
  const int LLEN = 16;
  char lbuf[LLEN];
  DIN.getline(lbuf,LLEN); // lbuf will contain at least '\0'
  String res(lbuf);
  delete_all_whitespace(res);

  if (!res.empty()) if (res[0]==nondefault) return 1;
  return 0;
}

char UI_Confirm_Classic::multi_request(const char * message, String respchars, StringList & respdescriptors) {
  // Returns a character representing one of the possible responses in respchars or 0 if
  // the request did not conclude with one of the possible responses.
  // NOTE: This function ignores whether characters in respchars and response
  // are uppercase or lowercase. Whitespace is expressly filtered out.

  if (message) UIOUT << message;

  const int LLEN = 16;
  char lbuf[LLEN];
  // *** Should the following be read from cin instead of DIN for safety?
  DIN.getline(lbuf,LLEN); // lbuf will contain at least '\0' (istream::getline() does not include the delimiter '\n')
  String res(lbuf);
  delete_all_whitespace(res);

  if (res.empty()) return 0;

  res.downcase();
  respchars.downcase();
  if (respchars.contains(res[0])) return res[0];

  return 0;
}

int UI_Confirm_Yad::request(const char * message, char nondefault, const char * defoption, const char * nondefoption, bool allowcancel) {
  // Returns 1 if the nondefoption is chosen.
  // Returns 0 if the defoption is chosen.
  // Returns -1 if allowcancel is true and cancel is chosen, or if window closed or ESC.
  // This derived class does not explicitly use the nondefault character.

  if (!message) return -1;
  String msg(message);
  HTML_safe(msg);
  escape_for_single_quote_cmdarg(msg);
  String cancelbutton(" --button='Cancel:2'");
  if (!allowcancel) cancelbutton = "";
  int retval = -1;
  if ((retval=System_Call("yad --title='"+rcmd_get()+"' --window-icon="+homedir+String(WINDOWICON)+" --image=dialog-question --image-on-top --buttons-layout=spread --button='"+defoption+":0' --button='"+nondefoption+":1'"+cancelbutton+" --text='"+msg+"' > /tmp/dil2al-yad-error.log 2>&1"))<0) {
    EOUT << "UI_Confirm_Yad::request(): System_call() returned error.\n";
    return -1;
  }
  retval = WEXITSTATUS(retval);
  if (retval>1) return -1; // Esc pressed, window closed (, or timeout)
  return retval;
}

char UI_Confirm_Yad::multi_request(const char * message, String respchars, StringList & respdescriptors) {
  // Returns a character representing one of the possible responses in respchars or 0 if
  // the request did not conclude with one of the possible responses.
  // Returns -1 if the system call fails.

  String msg;
  if (message) msg = message;
  HTML_safe(msg);
  escape_for_single_quote_cmdarg(msg);

  String yadstr("yad --title='"+rcmd_get()+"' --window-icon="+homedir+String(WINDOWICON)+" --image=dialog-question --image-on-top --buttons-layout=spread --text='"+msg+"'");
  int rdlen = respdescriptors.length();
  for (int i=0; i<((int) respchars.length()); i++) {
    yadstr += " --button='";
    if ((i<rdlen) && (!respdescriptors[i].empty())) yadstr += respdescriptors[i];
    else yadstr += respchars[i];
    yadstr += ':'+String((long) i)+"'";
  }
  yadstr += " > /tmp/dil2al-yad-error.log 2>&1";
  
  int retval = -1;
  if ((retval=System_Call(yadstr))<0) {
    EOUT << "UI_Confirm_Yad::multi_request(): System_call() returned error.\n";
    return -1;
  }
  retval = WEXITSTATUS(retval);
  if (retval>=((int) respchars.length())) return 0; // Esc pressed, window closed (, or timeout)

  return respchars[retval];
}

int UI_Confirm_External::request(const char * message, char nondefault, const char * defoption, const char * nondefoption, bool allowcancel) {
  // Returns 1 if the nondefoption is chosen.
  // Returns 0 if the defoption is chosen.
  // Returns -1 if allowcancel is true and cancel is chosen.
  // This derived class does not explicitly use the nondefault character.

  if (!message) return -1;
  String msg(message);
  HTML_safe(msg);
  escape_for_single_quote_cmdarg(msg);
  String cancelbutton(" --button='Cancel:2'");
  if (!allowcancel) cancelbutton = "";
  int retval = -1;
  if ((retval=System_Call(confirmcmd+" --title='"+rcmd_get()+"' --button='"+defoption+":0' --button='"+nondefoption+":1'"+cancelbutton+" --text='"+msg+"' 2> /dev/null"))<0) {
    EOUT << "UI_Confirm_Extern::request(): System_call() returned error.\n";
    return -1;
  }
  if (retval>1) return -1; // Esc pressed, window closed (, or timeout)
  return retval;
}

char UI_Confirm_External::multi_request(const char * message, String respchars, StringList & respdescriptors) {
  // Returns a character representing one of the possible responses in respchars or 0 if
  // the request did not conclude with one of the possible responses.
  // Returns -1 if the system call fails.

  String msg;
  if (message) msg = message;
  HTML_safe(msg);
  escape_for_single_quote_cmdarg(msg);

  String yadstr("yad --title='"+rcmd_get()+"' --text='"+msg+"'");
  int rdlen = respdescriptors.length();
  for (int i=0; i<((int) respchars.length()); i++) {
    yadstr += " --button='";
    if ((i<rdlen) && (!respdescriptors[i].empty())) yadstr += respdescriptors[i];
    else yadstr += respchars[i];
    yadstr += ':'+String((long) i)+"'";
  }
  yadstr += " 2> /dev/null";
  
  int retval = -1;
  if ((retval=System_Call(yadstr))<0) {
    EOUT << "UI_Confirm_External::multi_request(): System_call() returned error.\n";
    return -1;
  }
  retval = WEXITSTATUS(retval);
  if (retval>=((int) respchars.length())) return 0; // Esc pressed, window closed (, or timeout)

  return respchars[retval];
}

String UI_Options_Classic::request(UI_Options_RQData & opt) {
  // Requests a choice from a list of options and special choice options.
  //
  // NOTE A: This _Classic version does not compare the choice with
  // defaultsel or the sepcialsel characters, because open entry is also
  // permitted. It is assumed that the calling function handles all
  // tests and embeds this call in a loop if necessary.
  //
  // NOTE B: Because open entry is permitted, whitespace is NOT filtered
  // out.
  //
  // NOTE B: Using cin.getline() discards the delimiting newline character
  // (istream::getline() differs from the C getline() function that way)
  // and returns a string containing at least the terminating '\0'.

  ANSI_underline_on();
  if (opt.message) UIOUT << opt.message;
  ANSI_underline_off();
  
  for (int i=0; i<opt.numoptions; i++) UIOUT << "  (" << i << ") " << opt.options[i] << '\n';

  ANSI_bold_on();
  if ((opt.defaultsel) && (opt.defaultopt)) {
    UIOUT << "  (" << opt.defaultsel << ") DEFAULT: " << opt.defaultopt << '\n';
  }
  if ((opt.specialsel) && (opt.specialopt)) {
    for (int i = 0; i<opt.numspecial; i++) {
      UIOUT << "  (" << (*opt.specialsel)[i] << ") " << (*opt.specialopt)[i] << '\n';
    }
  }
  UIOUT << "Your choice: ";
  ANSI_bold_off();
  
  const int LLEN = 10240;
  char lbuf[LLEN];
  cin.getline(lbuf,LLEN);
  String choice(lbuf);

  return choice;
}

/*
NOTE: The following function needs to present both a list and an entry box, or
something similar. Yad can't do that within a single instance under normal
circumstances, but there are a number of different possible solutions:

a) yad-test1.sh paned solution with entry box on top and list on bottom, buttons
   all the way on bottom. You then need to press ok to select from list (can't
   just double click, unless double click can be used to cause OK in the notebook
   yad through an attached command).
b) yad-test2.sh notebook solution (similar to paned, but with tabs).
c) yad-test3.sh form solution with combo box instead of list.
d) yad-test4.sh a button causes call to another yad for the entry box.

Presently, I've opted for solution a), as demonstrated in yad-test1.sh. All of
the solutions need extra clicks, either for selection from the list, or for
manual entry of a DIL-ID. The chosen solution seems to look best.
*/

// inlined, because used only locally
inline bool build_yad_request_script(UI_Options_RQData & opt) {
  // Builds a sequence of yad command strings for a temporary shell script.
  // Returns success status.
  // NOTE: This is used in UI_Options_Yad::request(), which creates a
  // paned set of 3 yad calls. The stdout output of all 3 calls needs to be
  // read by the same popen(). The cleanest solution is to source this
  // temporary script in the popen() call.

  // Attempt to generate a unique shared memory key
  k = ftok(listfile,projid);
  keystr = String((long) k);
  if (!(++projid)) projid = 1;
  
  // build command line for yad call #1: entry box in the top pane
  String yadstr("yad --plug="+keystr+" --tabnum=1 --entry");
  if (opt.entrymsg) {
    String entrymsg(opt.entrymsg);
    escape_for_single_quote_cmdarg(entrymsg);
    yadstr += " --no-markup --text='";
    yadstr += entrymsg;
    yadstr += "'";
  }
  yadstr += " &\n";

  // build command line for yad call #2: list in the bottom pane
  yadstr += "yad --plug="+keystr+" --tabnum=2 --list --column=Num.:NUM --column=Option:TEXT";
  for (int i=0; i<opt.numoptions; i++) {
    escape_for_single_quote_cmdarg(opt.options[i]);
    yadstr += ' ' + String((long) i) + " '" + opt.options[i] + "'";
  }
  yadstr += " --no-markup --text='Suggestions:' &\n";

  // build command line for yad call #3: paned with buttons
  yadstr += "yad --paned --key="+keystr+" --height=640 --title='"+rcmd_get();
  yadstr += "' --window-icon="+homedir+WINDOWICON;
  if (opt.image) yadstr += " --image="+String(opt.image)+" --image-on-top";
  if ((opt.defaultsel) && (opt.defaultopt)) yadstr += " --button=DEFAULT:2";
  yadstr += " --button=Select:0";
  if ((opt.specialsel) && (opt.specialopt)) {
    for (int i = 0; i<opt.numspecial; i++) {
      escape_for_single_quote_cmdarg((*opt.specialopt)[i]);
      yadstr += " --button='";
      yadstr += (*opt.specialopt)[i];
      yadstr += "':";
      yadstr += String((long) (i+3));
    }
  }
  yadstr += " --button=Cancel:1 --buttons-layout=spread --no-markup --text='";
  if (opt.message) {
    String message(opt.message);
    escape_for_single_quote_cmdarg(message);
    yadstr += message;
  }
  if ((opt.defaultsel) && (opt.defaultopt)) {
    String defaultopt(opt.defaultopt);
    escape_for_single_quote_cmdarg(defaultopt);
    yadstr += "DEFAULT: ";
    yadstr += defaultopt;
  }
  yadstr += "'\n";

  // write script to temporary file
  if (!write_file_from_String("/tmp/dil2al-yadrequest.sh",yadstr,"",true)) return false;

  return true;
}

// inlined, because used only locally
inline int run_yad_request_script(StringList & yadresults, int & numyadresults) {
  // Runs the script created with build_yad_request_script().
  // yadresults returns script output lines read.
  // numyadresults returns the number of script output lines read.
  // Returns the exit status of the script that was run.
  // Returns -1 if an error was encountered.
  // NOTE: This is used in UI_Options_Yad::request() and uses popen()
  // instead of system() in order to read command stdout output.
  // *** Does this cause any trouble when dil2al is called from a form?
  
  const int LLEN = 10240;
  char lbuf[LLEN];
  FILE *fp;
  
  // NOTE: Unlike bash, a POSIX sh (e.g. dash) is not guaranteed to know the
  // 'source' built-in command. We therefore use the POSIX compliant '.'.
  String scriptstr(". /tmp/dil2al-yadrequest.sh 2> /tmp/dil2al-yad-error.log");
  fp = popen(scriptstr, "r");
  if (fp == NULL) {
    ERROR << "run_yad_request_script(): Unable to run command in popen().\n";
    return -1;
  }
  numyadresults = 0;
  while (fgets(lbuf, LLEN, fp) != NULL) yadresults[numyadresults++] = lbuf;
  int retval = pclose(fp);
  if (retval<0) {
    ERROR << "run_yad_request_script(): Error detected by pclose().\n";
    return -1;
  }
  retval = WEXITSTATUS(retval);

  // clean up shared memory used by yad processes
  System_Call("ipcrm -M "+keystr+" > /tmp/dil2al-yad-error.log 2>&1");
  
  return retval;
}

String UI_Options_Yad::request(UI_Options_RQData & opt) {
  // Requests a choice from a list of options and special choice options.
  // NOTE: opt.specialsel[] strings interpret ',' as a separator between multiple
  // selection strings that refer to the same option.
  // ***If this request() function should also be used with option selection
  //    where direct entry is not possible, then there should probably be a
  //    flag to that effect and a single yad should be launched without a pane
  //    for such an entry.

  // remove possible previous temporary script
  unlink("/tmp/dil2al-yadrequest.sh");

  // build the yad script
  if (!build_yad_request_script(opt)) {
    ERROR << "UI_Options_Yad::request(): Unable to build temporary yad commands script.\n";
    return "Cancel";
  }

  // run the yad script
  StringList yadresults;
  int retval, numyadresults;
  retval = run_yad_request_script(yadresults,numyadresults);

  // *** TESTING
  //cout << "retval = " << retval << '\n';
  //cout << "yadresults[0] = (" << yadresults[0] << ")\n";
  //cout << "yadresults[1] = (" << yadresults[1] << ")\n";
  // ***
  
  // Interpreting the yad results to convert them into UI_Options* output:
  //   0 = OK. This is paired with a row selection.
  //   1 = Cancel button selected.
  //   252 = ESC pressed or Window Close button clicked.
  //   2 = DEFAULT button selected.
  //   3..N = Special option (retval-3) button selected.

  switch (retval) {
  case 0:
    {
      if (numyadresults<1) {
	ERROR << "UI_Options_Yad::request(): Missing entry and row selection output.\n";
	return "Cancel";
      }
      String optsel(yadresults[0]);
      if ((!optsel.empty()) && (optsel[0]!='\n')) { // direct entry
	optsel.gsub(BRXwhite,""); // totally remove whitespace (remove_whitespace() only reduces it to a single space)
	return optsel; // Return the direct entry
      }
      // suggestion chosen
      if (numyadresults<2) {
	ERROR << "UI_Options_Yad::request(): Missing row selection output.\n";
	return "Cancel";
      }
      optsel = yadresults[1].before('|');
      if (optsel.empty()) {
	ERROR << "UI_Options_Yad::request(): Unexpected output format.\n";
	return "Cancel";
      }
      if (!optsel.matches(BRXint)) {
	ERROR << "UI_Options_Yad::request(): Output was not an expected integer.\n";
	return "Cancel";
      }
      return optsel; // Return the option number
    }
    break;
  case 1:
  case 252:
    return "Cancel";
    break;
  case 2:
    if (opt.defaultsel) return opt.defaultsel;
    return "0";
    break;
  default:
    if ((retval<3) || (retval>=(opt.numspecial+3))) {
      ERROR << "UI_Options_Yad::request(): Selection outside range of special options provided.\n";
      return "Cancel";
    }
    String specialstr((*opt.specialsel)[retval-3].before(','));
    if (specialstr.empty()) specialstr = (*opt.specialsel)[retval-3];
    if (specialstr=="\"\"") specialstr = "";
    return specialstr;
  };
    
  return "Cancel";
}
  
String UI_Options_External::request(UI_Options_RQData & opt) {
  // Requests a choice from a list of options and special choice options.

  return String("");
}

bool UI_Progress_Classic::start() {
  // Starts a new progress indicator.
  // Returns success status.
  // NOTE: There is not much to prepare here.

  last_percentage = -1;
  active = true;
  
  return true;
}

bool UI_Progress_Classic::update(long percentage, const char * progressstr) {
  // Updates a progress indicator.
  // percentage is the new indicator percentage.
  // progressstr is an optional new progress indicator text (# and \n are added here).
  // Returns success status.

  if (!active) return false;
  if (percentage<=last_percentage) return false;

  String updatestr;
  if (progressstr) {
    updatestr += progressstr;
    updatestr += ": ";
  }
  if (percentage<0) percentage=0;
  if (percentage>100) percentage=100;
  if (percentage>0) updatestr += '#';
  //updatestr += String((long) percentage);
  UIOUT << updatestr;
  UIOUT.flush();
  last_percentage = percentage;

  return true;
}

int UI_Progress_Classic::stop() {
  // Stops a progress indicator.
  // Returns the status of the yad process or -1 if an error occurred.

  if (!active) return false;

  UIOUT << '\n';
  active = false;
  
  return 1;
}

bool UI_Progress_Yad::start() {
  // Starts a new progress indicator.
  // This creates a popen() pipe to a yad process.
  // Returns success status.

  last_percentage = -1;
  fp = popen("yad --progress --percentage=0 --auto-close --width=400 --no-focus --title='"+rcmd_get()+"' --window-icon="+homedir+String(WINDOWICON)+" > /tmp/dil2al-yad-error.log 2>&1", "w");
  if (fp == NULL) {
    WARN << "UI_Progress_Yad::start(): Unable to run yad --progress in popen().\n";
    return false;
  }
  return true;
}

bool UI_Progress_Yad::update(long percentage, const char * progressstr) {
  // Updates a progress indicator.
  // percentage is the new indicator percentage.
  // progressstr is an optional new progress indicator text (# and \n are added here).
  // Returns success status.

  if (!fp) return false;
  if (percentage<=last_percentage) return false;

  String updatestr;
  if (progressstr) {
    updatestr = '#';
    updatestr += progressstr;
    updatestr += '\n';
  }
  if (percentage<0) percentage=0;
  if (percentage>100) percentage=100;
  updatestr += String(percentage);
  updatestr += '\n';
  if (fputs(updatestr,fp)<0) {
    WARN << "UI_Progress_Yad::update(): Unable to write to the yad process.\n";
    return false;
  }
  if (fflush(fp)!=0) {
    WARN << "UI_Progress_Yad::update(): Flush to yad process encountered an error.\n";
    return false;
  }
  last_percentage = percentage;

  return true;
}

int UI_Progress_Yad::stop() {
  // Stops a progress indicator.
  // This closes a popen() pipe.
  // Returns the status of the yad process or -1 if an error occurred.
  // NOTE: The status of the yad process may not be available to pclose().
  // For this reason, we ignore a -1 return value if errno==ECHILD.
  // ***For more fine-grained control, consider building a lower level
  //    approach using fork(), execve(), dup2(), pipe(), and wait().
  //    E.g. see https://daniel.haxx.se/stuff/popen2.c.

  if (!fp) return 0;

  update(100);
  int retval = pclose(fp);
  fp = NULL;
  if (retval==-1) {
    if (errno==ECHILD) return 0;
    ERROR << "UI_Progress_Yad::stop(): Error (" << (long) errno << ") detected by pclose().\n";
    return -1;
  }
  return  WEXITSTATUS(retval);
}

bool UI_Progress_External::start() {
  // Starts a new progress indicator.
  // Returns success status.

  //*** Hasn't been implemented yet.
  
  return true;
}

bool UI_Progress_External::update(long percentage, const char * progressstr) {
  // Updates a progress indicator.
  // percentage is the new indicator percentage.
  // progressstr is an optional new progress indicator text (# and \n are added here).
  // Returns success status.

  //*** Hasn't been implemented yet.
  
  return true;
}

int UI_Progress_External::stop() {
  // Stops a progress indicator.
  // Returns the status of the yad process or -1 if an error occurred.

  //*** Hasn't been implemented yet.
  
  return 1;
}

/*
// *** This can be a UI_ class that collects informative links to inspect.
//     In UI_CLASSIC mode, these can be expressed as links embedded in
//     standard output.
// *** To find places to put this, just go through alcomp.cc and look for
//     places marked TRICKY.
//     In UI_YAD mode, these can each be expressed either as:
//     a) A FORM item with launch buttons for each link that call a configurable
//        command (e.g. w3m) - but perhaps that's for UI_EXTERNAL.
//     b) A link in an HTML page launched with w3m in urxvt.
//     c) A link in the dil2al-output.log with that log being written as an
//        HTML file (dil2al-output.html) instead. In this case, no special
//        function may be needed at all. Simply don't strip HTML code or
//        anything else for the copy of output to the log.
UI_Info_Links::add() {
}
*/

/*
// Optionally, create a class that inherits ostream and that overrides the <<
// operator to catch a formatted output stream in a String, then use type
// specific output.

class uispecific_ostream: public ostream {
public:
  uispecific_ostream(): ostream(cout.rdbuf()), somevariable(false) {}
  void dosomething() {
    if(false == somevariable) {
      static_cast<ostream&>(*this) << "HEADER" << endl;
      somevariable = true;
    }
  }
  template <typename T>
  friend uispecific_ostream& operator<<(uispecific_ostream& uio, const T& v);
private:
  bool somevariable;
};

template <typename T>
uispecific_ostream& operator<<(uispecific_ostream& uio, const T& v){
  uio.dosomething();
  static_cast<std::ostream&>(myo) << v;
  return myo;
}
*/

