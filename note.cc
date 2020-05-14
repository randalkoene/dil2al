// note.cc
// Randal A. Koene, 20000304

#include "dil2al.hh"

void process_HTML(String & notestr) {
  // Process note shorthand codes to HTML and fill in @tags@
  
  // line breaks
  notestr.gsub("\n","<BR>\n");
  // paragraphs
  notestr.gsub(BigRegex("<BR>\n\\(<BR>\n\\)+"),"\n<P>\n");
  // lists with `-' or `*' item indicators
  notestr.gsub(BigRegex("^[ 	]*-[ 	]+"),"<LI>");
  int lstart, lend;
  while ((lstart=notestr.index(BigRegex("^[ 	]*[*][ 	]+")))>=0) {
    notestr.at(BigRegex("^[ 	]*[*][ 	]+"),lstart)="<UL>\n<LI>";
    while ((lend=notestr.index("\n",lstart))>=0) {
      lstart = lend+1;
      if (notestr.contains(BigRegex("\n\\([ 	]+[^ 	]+\\|[ 	]*[*][ 	]+\\)"),lend)) {
	notestr.at(BigRegex("\n[ 	]*[*][ 	]+"),lend) = "\n<LI>";
      } else break;
    }
    if (lend<0) { lend = notestr.length(); notestr += "\n"; }
    notestr.at("\n",lend) = "\n</UL>\n";
  }
  // note that HTML references are already correct
  // tag definitions
  // NOVELTY tags
  long novcount = 0;
  lstart = -1;
  while ((lstart=notestr.index("@NOV@",lstart+1))>=0) {
    if ((lend=notestr.index("@/NOV@",lstart))>=0) {
      String tmp1,tmp2;
      novcount++;
      tmp1 = notestr.at(lstart+5,lend-(lstart+4));
      tmp2 = notestr.after(lend+6);
      notestr = notestr.before(lstart) + "<A NAME=\"NOVELTY-"+((curtime+'-')+String(novcount))+"\">[<B>N</B>]</A> "
	+ tmp1 + "<!-- @NOVELTY-"+((curtime+'-')+String(novcount))+"@ -->" + tmp2;
    } else WARN << "process_HTML(): @NOV@ without matching @/NOV@. Continuing as is.\n";
  }
  //*** add code here to replace @tags@ and tags defined in the configuration file
  // clean up
  notestr.gsub("<BR><BR>","<BR>");
  notestr.gsub("<BR><P>","<P>");
  notestr.gsub("<P><BR>","<P>");
  notestr.gsub("<BR>\n<P>","\n<P>");
  notestr.gsub("<P>\n<P>","<P>");
  notestr.gsub("<BR>\n<UL>","\n<UL>");
  notestr.gsub("<BR>\n<LI>","\n<LI>");
  notestr.gsub("<BR>\n</UL>","\n</UL>");
}

int pre_process_note(String notefile, String notedst, String typestr, String & notestr, String & notedststr, bool & isnew) {

  if (!read_file_into_String(notefile,notestr)) return -1;
  if (notestr=="") { // if note is empty, don't do anything, but return true
    if (verbose) INFO << "Note is empty, no change.\n";
    return 0;
  }
  if (verbose) INFO << "Processing destination file as " << typestr << ".\n";
  process_HTML(notestr);
  
  // read note destination into memory
  if (!read_file_into_String(isnew,notedst,notedststr)) return -1;
  
  return 1;
}

int post_process_note(String notedst, String & notestr, String & notedststr, int insertindex, bool isnew) {
  
  // insert note into notedst
  if (insertindex>=0) {
    String c = notedststr.at(insertindex,1);
    notedststr.at(insertindex,1) = notestr+c;
  } else {
    insertindex = notedststr.length();
    notedststr += notestr;
  }
  
  // store and backup
  if (!write_file_from_String(notedst,notedststr,"Note destination",isnew)) return -1;
  
  return insertindex;
}

int process_note_TL(String notefile, String notedst) {
// A task log entry will be added to the current TL chunk, or to the TL chunk
// immediately preceding a @INSERT-TL-NOTE@ tag, or optionally to a new TL chunk,
// possibly in a new TL section.
  
  String notestr; bool isnew; int res;
  TL_chunk_access TL;
  if ((res = pre_process_note(notefile,notedst,"Task Log",notestr,TL.tl,isnew))<=0) return res;

  // search for <!-- @INSERT-TL-NOTE@ --> in TL
  bool comparetimes = true;
  if ((TL.tlinsertloc = TL.tl.index("<!-- @INSERT-TL-NOTE@ -->"))>=0) {
    TL.tlinsertloc -= TL.tl.length();
    comparetimes = false;
  }

  // determine whether to add a new Task Log chunk
  if (!decide_add_TL_chunk(TL,comparetimes)) return -1;

  // HTML process entry (and fill in @tag@ codes)
  process_HTML(notestr);

  // add entry to Task Log chunk
  newTLID = add_TL_chunk_or_entry(notestr,false,TL);
  if (newTLID=="") {
    ERROR << "process_note_TL(): Unable to add new Task Log entry.\n";
    return -1;
  }
  return TL.tlinsertloc;
}

int process_note_HTML(String notefile, String notedst) {
  
  String notestr, notedststr; bool isnew; int res;
  if ((res = pre_process_note(notefile,notedst,"HTML",notestr,notedststr,isnew))<=0) return res;

  // find a possible <!-- @INSERT-AT:<tag>@ -->
  int insertindex = -1;
  String inserttag = notestr.at(BigRegex("[<]!--[ 	]*@INSERT-AT:[^@]+@[ 	]*--[>]"));
  if (inserttag!="") {
    inserttag = inserttag.after("INSERT-AT:");
    inserttag = inserttag.before("@",-1);
    insertindex = notedststr.index(BigRegex("[<]!--[ 	]*@"+inserttag+"@[ 	]*--[>]"));
  }

  // find a possible <!-- @INSERT-TL-NOTE@ -->
  if (insertindex<0) insertindex = notedststr.index(BigRegex("[<]!--[ 	]*@INSERT-TL-NOTE@[ 	]*--[>]"));

  // find </BODY>
  if (insertindex<0) insertindex = notedststr.index("</BODY>");
  if (insertindex<0) WARN << "process_note_HTML(): Note destination " << notedst << "\nmissing </BODY> tag. Continuing.\n";
  
  // insert note into notedst
  return post_process_note(notedst,notestr,notedststr,insertindex,isnew);
}

int process_note_TeX(String notefile, String notedst) {
  String notestr, notedststr; bool isnew; int res;
  if ((res = pre_process_note(notefile,notedst,"TeX",notestr,notedststr,isnew))<=0) return res;
  // find a possible % <!-- @INSERT-AT:<tag>@ -->
  int insertindex = -1;
  String inserttag = notestr.at(BigRegex("[<]!--[ 	]*@INSERT-AT:[^@]+@[ 	]*--[>]"));
  if (inserttag!="") {
    inserttag = inserttag.after("INSERT-AT:");
    inserttag = inserttag.before("@",-1);
    insertindex = notedststr.index(BigRegex("%[\n]*[<]!--[ 	]*@"+inserttag+"@[ 	]*--[>]"));
  }
  // find a possible % <!-- @INSERT-TL-NOTE@ -->
  if (insertindex<0) insertindex = notedststr.index(BigRegex("%[\n]*[<]!--[ 	]*@INSERT-TL-NOTE@[ 	]*--[>]"));
  // find \end{document}
  if (insertindex<0) insertindex = notedststr.index("\\end{document}");
  // find \endinput
  if (insertindex<0) insertindex = notedststr.index("\\endinput");
  if (insertindex<0) WARN << "process_note_TeX(): Note destination " << notedst << "\nmissing \\end{document} and \\endinput tags. Continuing.\n";
  // convert to TeX
  notestr.gsub(BigRegex("[<][Bb][Rr]\\([ 	]+[^>][>]\\|[>]\\)"),"\\\\"); // <BR>
  notestr.gsub(BigRegex("[<][Pp]\\([ 	]+[^>]*[>]\\|[>]\\)"),"\n\n"); // <P>
  notestr.gsub(BigRegex("[<][Uu][Ll]\\([ 	]+[^>]*[>]\\|[>]\\)"),"\\begin{itemize}"); // <UL>
  notestr.gsub(BigRegex("[<]/[Uu][Ll]\\([ 	]+[^>]*[>]\\|[>]\\)"),"\\end{itemize}"); // </UL>
  notestr.gsub(BigRegex("[<][Oo][Ll]\\([ 	]+[^>]*[>]\\|[>]\\)"),"\\begin{enumerate}"); // <OL>
  notestr.gsub(BigRegex("[<]/[Oo][Ll]\\([ 	]+[^>]*[>]\\|[>]\\)"),"\\end{enumerate}"); // </OL>
  notestr.gsub(BigRegex("[<][Ll][Ii]\\([ 	]+[^>]*[>]\\|[>]\\)"),"\\item "); // <LI>
  notestr.gsub(BigRegex("[<][Hh]1\\([ 	]+[^>]*[>]\\|[>]\\)"),"\\section{"); // <H1>
  notestr.gsub(BigRegex("[<][Hh]2\\([ 	]+[^>]*[>]\\|[>]\\)"),"\\subsection{"); // <H2>
  notestr.gsub(BigRegex("[<][Hh]3\\([ 	]+[^>]*[>]\\|[>]\\)"),"\\subsubsection{"); // <H3>
  notestr.gsub(BigRegex("[<][Hh]4\\([ 	]+[^>]*[>]\\|[>]\\)"),"\\paragraph{"); // <H3>
  notestr.gsub(BigRegex("[<]/[Hh][1-4]\\([ 	]+[^>]*[>]\\|[>]\\)"),"}"); // </H1>,</H2>,</H3>,</H4>
  notestr.gsub(BigRegex("[<][Hh][Rr]\\([ 	]+[^>]*[>]\\|[>]\\)"),"\\hline "); // <HR>
  notestr.gsub(BigRegex("[<]!--"),"% <!--"); // <!-- -->
  notestr.gsub(BigRegex("[<][Tt][Aa][Bb][Ll][Ee][ 	]*[^>]*[>]"),"\\begin{tabular}{???}"); // <TABLE>
  notestr.gsub(BigRegex("[<]/[Tt][Aa][Bb][Ll][Ee][ 	]*[^>]*[>]"),"\\end{tabular}"); // </TABLE>
  notestr.gsub(BigRegex("[<][Tt][Rr][ 	]*[^>]*[>]"),"\\\\\n"); // <TR>
  notestr.gsub(BigRegex("[<]/[Tt][Rr][ 	]*[^>]*[>]"),""); // <TR>
  notestr.gsub(BigRegex("[<][Tt][Dd][ 	]*[^>]*[>]")," & "); // <TD>
  notestr.gsub(BigRegex("[<]/[Tt][Dd][ 	]*[^>]*[>]"),""); // <TD>
  notestr.gsub(BigRegex("[<][Tt][Hh][ 	]*[^>]*[>]")," & {\\bf "); // <TH>
  notestr.gsub(BigRegex("[<]/[Tt][Hh][ 	]*[^>]*[>]"),"}"); // <TH>
  notestr.gsub(BigRegex("[<][Bb]\\([ 	]+[^>]*[>]\\|[>]\\)"),"{\\bf "); // <B>
  notestr.gsub(BigRegex("[<][Ii]\\([ 	]+[^>]*[>]\\|[>]\\)"),"{\\em "); // <I>
  notestr.gsub(BigRegex("[<][Uu]\\([ 	]+[^>]*[>]\\|[>]\\)"),"\\underline{"); // <U>
  notestr.gsub(BigRegex("[<]/[BbIiUu]\\([ 	]+[^>]*[>]\\|[>]\\)"),"}"); // </B>,</I>,</U>
  notestr.gsub(BigRegex("[<][Ee][Mm]\\([ 	]+[^>]*[>]\\|[>]\\)"),"{\\em "); // <EM>
  notestr.gsub(BigRegex("[<]/[Ee][Mm]\\([ 	]+[^>]*[>]\\|[>]\\)"),"}"); // </EM>
  notestr.gsub(BigRegex("[<][Ss][Tt][Rr][Oo][Nn][Gg][ 	]*[^>]*[>]"),"{\\bf "); // <STRONG>
  notestr.gsub(BigRegex("[<]/[Ss][Tt][Rr][Oo][Nn][Gg][ 	]*[^>]*[>]"),"}"); // </STRONG>
  // <A HREF="http://the-reference-url/">the-reference-text</A>
  // <A NAME="the-reference-name-tag">the-reference-anchor</A>
  int i = 0, j = 0, k;
  while (i>=0) {
    i = notestr.index(BigRegex("[<][Aa][ 	]+[^>]*\\([Hh][Rr][Ee][Ff]\\|[Nn][Aa][Mm][Ee]\\)[ 	]*=[ 	]*[^>]*[>].*[<]/[Aa][ 	]*[^>]*[>]"),j);
    if (i>=0) {
      k = notestr.index(">",i) + 1;
      j = notestr.index(BigRegex("[<]/[Aa][ 	]*[^>]*[>]"),i);
      if (k<j) {
	String tmpstr = "</A><!--\n"+notestr.at(k,j-k)+'\n'+notestr.after(BigRegex("[<]/[Aa][ 	]*[^>]*[>]"),j);
	notestr = notestr.before(BigRegex("[<]/[Aa][ 	]*[^>]*[>]"),j);
	notestr += tmpstr;
	//notestr.at(BigRegex("[<]/[Aa][ 	]*[^>]*[>]"),j)="</A><!--\n"+notestr.at(k,j-k)+'\n';
      }
      String tmpstr = "% -->"+notestr.from(i);
      notestr = notestr.before(i);
      notestr += tmpstr;
      //notestr.at(i,1)="% --><";
    }
  }
  // other <...> tags
  i = 0; j = 0;
  while (i>=0) {
    i = notestr.index(BigRegex("[<][^>]*[>]"),j);
    if (i>=0) {
      j = notestr.index(">",i);
      String tmpstr = "% "+notestr.at(i,(j-i)+1)+"\n"+notestr.after(j);
      notestr = notestr.before(i)+tmpstr;
      //notestr.at(j,1) = ">\n";
      //notestr.at(i,1) = "% <";
    }
  }
  notestr.gsub("&lt;","<"); // &lt;
  notestr.gsub("&gt;",">"); // &gt;
  notestr.gsub("&amp;","\\&"); // &amp;
  notestr.gsub("&nbsp;","\\relax"); // &nbsp;
  // clean up
  notestr.gsub("\n\n","\n");
  notestr.gsub(BigRegex("[ 	]+\n"),"\n");
  // insert note into notedst
  return post_process_note(notedst,notestr,notedststr,insertindex,isnew);
}

int process_note_CC(String notefile, String notedst) {
  String notestr, notedststr; bool isnew; int res;
  if ((res = pre_process_note(notefile,notedst,"C++",notestr,notedststr,isnew))<=0) return res;
  // find a possible // <!-- @INSERT-AT:<tag>@ -->
  int insertindex = -1;
  String inserttag = notestr.at(BigRegex("[<]!--[ 	]*@INSERT-AT:[^@]+@[ 	]*--[>]"));
  if (inserttag!="") {
    inserttag = inserttag.after("INSERT-AT:");
    inserttag = inserttag.before("@",-1);
    insertindex = notedststr.index(BigRegex("//[\n]*[<]!--[ 	]*@"+inserttag+"@[ 	]*--[>]"));
  }
  // find a possible // <!-- @INSERT-TL-NOTE@ -->
  if (insertindex<0) insertindex = notedststr.index(BigRegex("//[\n]*[<]!--[ 	]*@INSERT-TL-NOTE@[ 	]*--[>]"));
  if ((insertindex<0) && (verbose))  INFO << "No specific insertion location in " << notedst << ", appending note to end\n";
  // convert to C++
  notestr.gsub(BigRegex("[<][Bb][Rr][ 	]*[^>][>]\n?"),"\n"); // <BR>
  notestr.gsub(BigRegex("\n?[<][Pp][ 	]*[^>]*[>]\n?"),"\n\n"); // <P>
  notestr.gsub(BigRegex("[<][Uu][Ll][ 	]*[^>]*[>]"),""); // <UL>
  notestr.gsub(BigRegex("[<]/[Uu][Ll][ 	]*[^>]*[>]"),""); // </UL>
  notestr.gsub(BigRegex("[<][Oo][Ll][ 	]*[^>]*[>]"),""); // <OL>
  notestr.gsub(BigRegex("[<]/[Oo][Ll][ 	]*[^>]*[>]"),""); // </OL>
  notestr.gsub(BigRegex("[<][Ll][Ii][ 	]*[^>]*[>]"),"- "); // <LI>
  // <A HREF="http://the-reference-url/">the-reference-text</A>
  // <A NAME="the-reference-name-tag">the-reference-anchor</A>
  int i = 0, j = 0, k;
  while (i>=0) {
    i = notestr.index(BigRegex("[<][Aa][ 	]+[^>]*\\([Hh][Rr][Ee][Ff]\\|[Nn][Aa][Mm][Ee]\\)[ 	]*=[ 	]*[^>]*[>].*[<]/[Aa][ 	]*[^>]*[>]"),j);
    if (i>=0) {
      j = notestr.index(BigRegex("//[^\n]*"),i-notestr.length()); // check if already // commented
      if (j<0) {
	j = notestr.index("/*",i-notestr.length());
	if (j>=0) {
	  k = notestr.index("*/",i-notestr.length());
	  if (k>j) j = -1;
	}
      }
      if (j<0) {
	j = notestr.index(BigRegex("[<]/[Aa][ 	]*[^>]*[>]"),i);
	j = notestr.index(">",j);
	String tmpstr = "/* "+notestr.at(i,(j-i)+1)+" */"+notestr.after(j);
	notestr = notestr.before(i)+tmpstr;
	//notestr.at(j,1) = "> */";
	//notestr.at(i,1) = "/* <";
      }
    }
  }
  // other <...> tags
  i = 0; j = 0;
  while (i>=0) {
    i = notestr.index(BigRegex("[<][^>]*[>]"),j);
    if (i>=0) {
      j = notestr.index(BigRegex("//[^\n]*"),i-notestr.length()); // check if already // commented
      if (j<0) {
	j = notestr.index("/*",i-notestr.length());
	if (j>=0) {
	  k = notestr.index("*/",i-notestr.length());
	  if (k>j) j = -1;
	}
      }
      if (j<0) { // convert tag to comment
	j = notestr.index(">",i);
	String tmpstr = "/* "+notestr.at(i,(j-i)+1)+" */"+notestr.after(j);
	notestr = notestr.before(i)+tmpstr;
	//notestr.at(j,1) = "> */";
	//notestr.at(i,1) = "/* <";
      }
    }
  }
  notestr.gsub("&lt;","<"); // &lt;
  notestr.gsub("&gt;",">"); // &gt;
  notestr.gsub("&amp;","&"); // &amp;
  notestr.gsub("&nbsp;"," "); // &nbsp;
  // clean up
  notestr.gsub(BigRegex("[ 	]+\n"),"\n");
  // insert note into notedst
  return post_process_note(notedst,notestr,notedststr,insertindex,isnew);
}

int process_note_generic(String notefile, String notedst) {
  String notestr, notedststr; bool isnew; int res;
  if ((res = pre_process_note(notefile,notedst,"generic text",notestr,notedststr,isnew))<=0) return res;
  // find a possible <!-- @INSERT-AT:<tag>@ -->
  int insertindex = -1;
  String inserttag = notestr.at(BigRegex("[<]!--[ 	]*@INSERT-AT:[^@]+@[ 	]*--[>]"));
  if (inserttag!="") {
    inserttag = inserttag.after("INSERT-AT:");
    inserttag = inserttag.before("@",-1);
    insertindex = notedststr.index(BigRegex("[<]!--[ 	]*@"+inserttag+"@[ 	]*--[>]"));
  }
  // find a possible <!-- @INSERT-TL-NOTE@ -->
  if (insertindex<0) insertindex = notedststr.index(BigRegex("[<]!--[ 	]*@INSERT-TL-NOTE@[ 	]*--[>]"));
  if ((insertindex<0) && (verbose))  INFO << "No specific insertion location in " << notedst << ", appending note to end\n";
  // convert to generic text
  notestr.gsub(BigRegex("[<][Bb][Rr][ 	]*[^>][>]\n?"),"\n"); // <BR>
  notestr.gsub(BigRegex("\n?[<][Pp][ 	]*[^>]*[>]\n?"),"\n\n"); // <P>
  notestr.gsub(BigRegex("[<][Uu][Ll][ 	]*[^>]*[>]"),""); // <UL>
  notestr.gsub(BigRegex("[<]/[Uu][Ll][ 	]*[^>]*[>]"),""); // </UL>
  notestr.gsub(BigRegex("[<][Oo][Ll][ 	]*[^>]*[>]"),""); // <OL>
  notestr.gsub(BigRegex("[<]/[Oo][Ll][ 	]*[^>]*[>]"),""); // </OL>
  notestr.gsub(BigRegex("[<][Ll][Ii][ 	]*[^>]*[>]"),"- "); // <LI>
  notestr.gsub("&lt;","<"); // &lt;
  notestr.gsub("&gt;",">"); // &gt;
  notestr.gsub("&amp;","&"); // &amp;
  notestr.gsub("&nbsp;"," "); // &nbsp;
  // clean up
  notestr.gsub(BigRegex("[ 	]+\n"),"\n");
  // insert note into notedst
  return post_process_note(notedst,notestr,notedststr,insertindex,isnew);
}

bool process_note(String notefile, String notedst) {
  // Process a note and put it into its destination file.
  // Depending on the type of the destination file, one of a set of
  // type-specific processing functions is called.
  enum file_type_t { ft_generic, ft_tasklog, ft_HTML, ft_TeX, ft_CC , ft_UNDETERMINED };
  typedef int (*pn_func)(String, String);
  pn_func process_note_ft[ft_UNDETERMINED] = { &process_note_generic, &process_note_TL, &process_note_HTML, &process_note_TeX, &process_note_CC };
  
  // determine destination file type
  file_type_t filetype = ft_generic;
  if (notedst==tasklog) {
    filetype = ft_tasklog;
    if ((notedst = get_TL_head())=="") return false;
  } else if (notedst.matches(BigRegex(".*[.][Hh][Tt][Mm][Ll]?\\([---_.].*\\)?"))) filetype = ft_HTML; // HTML type
  else if (notedst.matches(BigRegex(".*[.][Tt][Ee][Xx]\\([---_.].*\\)?"))) filetype = ft_TeX; // TeX type
  else if (notedst.matches(BigRegex(".*[.]\\([Cc][Cc]|[Hh][Hh]\\)\\([---_.].*\\)?"))) filetype = ft_CC; // C++ type
  else if (askprocesstype) {
    filetype = ft_UNDETERMINED;
    StringList respdescriptors("generic;Task Log;HTML;TeX;C++",';');
    String respchars("01234");
    while ((filetype<0) || (filetype>=ft_UNDETERMINED)) {
      String ftstr(confirm->multi_request("Destination file type:\n  0 = generic\n  1 = Task Log\n  2 = HTML\n  3 = TeX\n  4 = C++\nPlease specify: ",respchars,respdescriptors));
      if (ftstr.matches(BRXint)) filetype = static_cast<file_type_t>(atoi((const char *) ftstr));
    }
  }
  
  // process and insert note in destination file
  long res = process_note_ft[filetype](notefile,notedst);

  // possibly open destination file in an editor
  if (verbose) INFO << "updatenoteineditor = " << updatenoteineditor << '\n';
  bool updateineditor = (updatenoteineditor == UNIE_YES);
  if (updatenoteineditor==UNIE_ASK) {
    updateineditor = !confirmation("\nOpen"+notedst+" in editor? (y/N) ",'y',"NO","Yes");
  }
  if (updateineditor) {
    String uniecmd = updatenoteineditorcmd;
    uniecmd.gsub("@res@",String(res));
    if (filetype==ft_tasklog) notedst=tasklog; // revert to symbolic link in case new Task Log section was added
    uniecmd.gsub("@notedst@",notedst);
    if (System_Call(uniecmd)<0) WARN << "process_note(): Unable to open " << notedst << "\nin editor. Continuing.\n";
  }
  return true;
}

bool clean_up_reference_tag(String filename, String & tagpattern) {
  // Clean up unnecessary (self-)reference NAME tag.
  // NOTE: This function does signal whether it was successful or not, but
  // the calling functions presently do not use the return value, since
  // the default behavior is to 'continue as is'.
  
  ifstream ntf(filename);
  if (!ntf) {
    WARN << "clean_up_reference_tag(): Unable to open " << filename << "\nfor removal of spurious NAME tag. Continuing as is.\n";
    return false;
  }
  ofstream ntfnew(filename+".tlref_cleaned");
  if (!ntfnew) {
    WARN << "clean_up_reference_tag(): Unable to create " << filename << ".tlref_cleaned.\nContinuing as is.\n";
    ntf.close();
    return false;
  }

  const int LLEN = 65525; // was 10240
  char lbuf[LLEN];
  int res = -1;
  if ((res=copy_until_line(&ntf,&ntfnew,tagpattern,lbuf,LLEN))!=1) { // everything up to the tag pattern
    ntf.close(); ntfnew.close();
    if (res<0) WARN << "clean_up_reference_tag(): Unable to find and remove unnecessary NAME tag.\nContinuing as is.\n";
    if (unlink(filename+".tlref_cleaned")<0) WARN << "clean_up_reference_tag(): Unable to remove " << filename << ".tlref_cleaned.\nContinuing as is.\n";
    return false;
  }
  
  String noteline(lbuf); noteline.del(tagpattern); // remove tag pattern from buffered line
  ntfnew << noteline << '\n';
  ntfnew << ntf.rdbuf(); // remaining lines
  ntf.close(); ntfnew.close();
  
  if (rename(filename+".tlref_cleaned",filename)<0) {
    WARN << "clean_up_reference_tag(): Unable to rename " << filename << ".tlref_cleaned to " << filename << ".\nContinuing as is.\n";
    return false;
  }
  
  return true;
}

bool quick_select_target(String & notedst, String & notedsttitle) {
  // This little helper function lets the user select a target from a
  // list that was defined in .dil2alrc, the default target (designated
  // by an empty input), or a specific file path.
  // Returns true unless Cancel is chosen.
  // notedst returns the target file path (or "" for default).
  // notedsttitle returns a descriptive title (or the filename).

  StringList quicknoteoptions;
  for (int i=0; i<quicknotedstnum; i++) quicknoteoptions[i] = quicknotedsttitle[i]+quicknotedescr[i];
  UI_Options_RQData uioptrqdata = { "Destination of note (empty = default [Task Log])\n\n", "\nor enter file path ('c' to cancel): ", quicknoteoptions, quicknotedstnum, "", quicknoteoptions[0], NULL, NULL, 0, NULL }; 
  while (true) {
    notedst = options->request(uioptrqdata);
    
    // Is it the default target? (e.g. Task Log)
    if (notedst.empty()) return true;

    // Is it Cancel?
    if ((notedst=="c") || (notedst=="Cancel")) return false;
    
    // In case it's a specific file path
    String notedsttitle(notedst);
    notedsttitle.gsub(BigRegex(".*/"),""); // filename without directories
    
    // Is it an enumerated quick select target?
    if (notedst.matches(BRXint)) {
      int q = atoi((const char *) notedst);
      if ((q>=0) && (q<quicknotedstnum)) {
	notedst = quicknotedst[q];
	notedsttitle = quicknotedsttitle[q];
	if (notedst[0]!='/') notedst.prepend(homedir);
	return true;
      } else WARN << "quick_select_target(): Number was not within enumerated selections available\n";
    } else { // A specific file path
      notedsttitle = notedst;
      notedsttitle.gsub(BigRegex(".*/"),""); // filename without directories
      if (notedst[0]!='/') notedst.prepend(homedir);
      return true;
    }
  }
}

// There are two versions of make_note() below.
// One version is for use with SIMPLIFY_USER_REQUESTS.
// The other is the (older) default.

bool make_note_TL(String initnotefromfile) {
  // A note is created and added to the Task Log.
  // NOTE: In this version, there are no alternative destination files and no reference
  // links between a note in another file and a Task Log entry.

  // open an editor for a plain text entry
  String noteinit;
  if (!initnotefromfile.empty()) {
    INFO << "Initializing note from " << initnotefromfile << '\n';
    String initnotestr;
    if (!read_file_into_String(initnotefromfile,initnotestr)) {
      ERROR << "make_note(): Unable to initialize note from " << initnotefromfile << '\n';
      return false;
    }
    noteinit += initnotestr;
  }
  if (!queue_temporary_file(notetmpfile,noteinit)) return false;
  if (System_Call(editor+" -T 'Make Your Note as Task Log Entry' "+notetmpfile)<0) return false;
  // refresh curtime to avoid misrepresentations due to constantly open note editing window
  curtime = time_stamp("%Y%m%d%H%M");

  // Note goes into Task Log
  String notedst(tasklog);
  if (verbose) INFO << "Note destination: " << notedst << '\n';

  if (!process_note(notetmpfile,notedst)) {
    ERROR << "make_note_TL(): Unable to process note, leaving note in " << notetmpfile << '\n';
    return false;
  }
  // Move the temporary file for subsequent queued access
  if (rename(notetmpfile,notetmpfile+".bak")<0) if (errno!=ENOENT) {
      WARN << "make_note_TL(): Unable to rename " << notetmpfile << " to " << notetmpfile+".bak.\n";
  }	
  return true;
}

bool make_note(String initnotefromfile) {
  // A note is created and added to a file.
  // 1) The default destination for the note is the Task Log. If that is where it goes
  //    then process_note() puts it there.
  // 2) If the note is destined for a different file then an opportunity is given to
  //    create an additional referring note in the Task Log. The possible results are:
  //    a) A non-empty referring note is also created. That referring note is added to
  //       the Task Log by a separate call to process_note() and it contains an HREF
  //       link to the destination file and the main note. The Task Log URL and entry
  //       NAME are put into the main note as a link back to the Task Log entry. The
  //       main note is added to the destination file by a call to process_note().
  //    b) The referring note is empty and therefore not created. The main note is
  //       therefore independent of the Task Log. There is no reference from the Task
  //       Log to the note, and the reference stub in the main note is removed. A
  //       call to process_note() puts the main note into its destination file.

  _EXTRA_VERBOSE_REPORT_FUNCTION();
  
  if (simplifyuserrequests) return make_note_TL(initnotefromfile);
  
  const int LLEN = 65525; // was 10240
  char lbuf[LLEN];
  int res = -1;
  //int q;

  // wait for uncontested availability of temporary file and open in an editor to write the note
  String noteinit;
  if (suggestnameref) {
    noteinit = "<A NAME=\"TL-ref-" + curtime + "\" HREF=\"@TLURL@#@TLNEXTENTRY@\">_</A>";
    if (verbose) INFO << "If the destination of the Note is the Task Log, and if the suggested\nreference is not altered, then\n " << noteinit << "\nwill be cleaned up automatically.\n";
  }
  if (!initnotefromfile.empty()) {
    INFO << "Initializing note from " << initnotefromfile << '\n';
    String initnotestr;
    if (!read_file_into_String(initnotefromfile,initnotestr)) {
      ERROR << "make_note(): Unable to initialize note from " << initnotefromfile << '\n';
      return false;
    }
    noteinit += initnotestr;
  }
  if (!queue_temporary_file(notetmpfile,noteinit)) return false;
  if (System_Call(editor+" -T 'Make Your Note' "+notetmpfile)<0) return false;

  // refresh curtime to avoid misrepresentations due to constantly open note editing window
  curtime = time_stamp("%Y%m%d%H%M");

  // determine the destination file for the note
  String notedst, notedsttitle;
  if (!quick_select_target(notedst,notedsttitle)) return false; // Cancel was selected

  if (notedst.empty()) {
    // Task Log
    notedst = tasklog;
    if (suggestnameref) clean_up_reference_tag(notetmpfile,noteinit);
    if (verbose) INFO << "Note destination: " << notedst << '\n';
    
  } else {
    // Other file
    if (verbose) INFO << "Note destination: " << notedst << '\n';

    // if possible, use the NAME reference in the temporary note file to create the HREF from the Task Log
    String tlentryinit("<A HREF=\""+relurl(get_TL_head(),notedst)); // a suggested default
    ifstream ntf(notetmpfile);
    if (!ntf) {
      ERROR << "make_note(): Unable to open " << notetmpfile << '\n';
      return false;
    }
    // the NAME found will be valid, even if it was manually modified
    if (find_line(&ntf,"[<][Aa][ 	]+[^>]*[Nn][As][Mm][Ee]=[ 	]*\"[^>]+[>]",lbuf,LLEN)) {
      String tlref(lbuf); tlref = tlref.after(BigRegex("[<][Aa][ 	]+[^>]*[Nn][As][Mm][Ee]=[ 	]*\""));
      tlentryinit += '#'; tlentryinit += tlref.before("\"");
    }
    ntf.close();
    tlentryinit += "\">(" + notedsttitle + ")</A>";

    // create a Task Log entry that is linked to the note through the HREF
    // NOTE: This can be prepare_temporary_file() instead of queue_temporary_file(),
    // because the temporary note file will not be moved to .bak until after
    // make_note(). So, no competing process can be working with these temporary
    // files at the same time. (Technically, a collision could happen when
    // 'override' is manually chosen above, of course.)
    if (!prepare_temporary_file(tlentrytmpfile,tlentryinit)) return false;
    if (System_Call(editor+" -T 'Task Log Entry' "+tlentrytmpfile)<0) return false;
    newTLID = ""; // clear to test if properly generated
    if (!process_note(tlentrytmpfile,tasklog)) return false;
    
    if (newTLID=="") {

      // if the Task Log entry was empty (none) then remove the TL reference from the note
      if (verbose) INFO << "No new Task Log entry ID (possibly empty):\nRemoving @TLURL@ and\n@TLNEXTENTRY@ references from note.\n";
      ntf.open(notetmpfile);
      if (!ntf) WARN << "make_note(): Unable to open " << notetmpfile << "\nto remove spurious @TLURL@ and @TLNEXTENTRY@ references.\nContinuing as is.\n";
      else {
	ofstream ntfnew(notetmpfile+".tlurl_cleaned");
	if (!ntfnew) WARN << "make_note(): Unable to create " << notetmpfile << ".tlurl_cleaned.\nContinuing as is.\n";
	else {
	  while (ntf) {
	    if ((res=copy_until_line(&ntf,&ntfnew,"@TLURL@|@TLNEXTENTRY@",lbuf,LLEN))==1) {
	      tlentryinit = lbuf;
	      tlentryinit.gsub(BigRegex("[<][Aa][ 	]+[Hh][Rr][Ee][Ff][ 	]*=[ 	]*\"[^\"]*\\(@TLURL@\\|@TLNEXTENTRY@\\)[^\"]*\"[ 	]*[>][^<]*[<]/[Aa][>]"),"");
	      tlentryinit.gsub(BigRegex("[ 	]+[Hh][Rr][Ee][Ff][ 	]*=[ 	]*\"[^\"]*\\(@TLURL@\\|@TLNEXTENTRY@\\)[^\"]*\"[ 	]*"),"");
	      ntfnew << tlentryinit << '\n';
	    } else break;
	  }
	  ntfnew.close();
	  if (res<0) WARN << "make_note(): Unable to find and remove spurious @TLURL@ and @TLNEXTENTRY@ references.\nContinuing as is.\n";
	  else if (rename(notetmpfile+".tlurl_cleaned",notetmpfile)<0) WARN << "make_note(): Unable to rename " << notetmpfile << ".tlurl_cleaned to " << notetmpfile << ".\nContinuing as is.\n";
	}
	ntf.close();
      }
      
    } else {

      // if a Task Log entry was made then fill in @TLURL@ and @TLNEXTENTRY@ in the note
      ntf.open(notetmpfile);
      if (!ntf) WARN << "make_note(): Unable to open " << notetmpfile << "\nto fill in @TLURL@ and @TLNEXTENTRY@ references.\nContinuing as is.\n";
      else {
	ofstream ntfnew(notetmpfile+".tlurl");
	if (!ntfnew) WARN << "make_note(): Unable to create " << notetmpfile << ".tlurl.\nContinuing as is.\n";
	else {
	  String tlurl(relurl(notedst,get_TL_head()));
	  while (ntf) {
	    if ((res=copy_until_line(&ntf,&ntfnew,"@TLURL@|@TLNEXTENTRY@",lbuf,LLEN))==1) {
	      tlentryinit = lbuf;
	      tlentryinit.gsub("@TLURL@",tlurl);
	      tlentryinit.gsub("@TLNEXTENTRY@",newTLID);
	      ntfnew << tlentryinit << '\n';
	    } else break;
	  }
	  ntfnew.close();
	  if (res<0) WARN << "make_note(): Unable to find and fill in @TLURL@ and @TLNEXTENTRY@ references.\nContinuing as is.\n";
	  else if (rename(notetmpfile+".tlurl",notetmpfile)<0) WARN << "make_note(): Unable to rename " << notetmpfile << ".tlurl to " << notetmpfile << ".\nContinuing as is.\n";
	}
	ntf.close();
      }
    }
  }

  // Process the note, put it into its destination file
  if (!process_note(notetmpfile,notedst)) {
    ERROR << "make_note(): Unable to process note, leaving note in " << notetmpfile << '\n';
    return false;
  }

  // Move the temporary file to enable subsequent queued access
  if (rename(notetmpfile,notetmpfile+".bak")<0) if (errno!=ENOENT) {
    WARN << "make_note(): Unable to rename " << notetmpfile << " to " << notetmpfile+".bak.\n";
  }	
  return true;
}
