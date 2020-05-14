// tlfilter.cc
// Randal A. Koene, 20000304

#include "dil2al.hh"

//#define DEBUG_PPEXTRACT

String generate_Figures_Directory_entry(String figid) {
  String figlabel, figtype,res;
  figlabel = figid.before('.',-1);
  figtype = figid.after('.',-1);
  figid = figlabel; figlabel.del(BigRegex("^.*/"));
  figid = relurl(figuresdirectory,figid);
  res = "<LI><!-- @Begin FIGURE DATA: [fig:"+figlabel+"] "+figid+".eps@ -->\n";
  res += "[<!-- @Figure label@ -->fig:"+figlabel+"]<BR>\n";
  if (figtype!="eps") res += "(*)";
  res += "Figure: <A HREF=\""+figid+".eps\"><!-- @Figure file@ -->"+figid+".eps</A><BR>\n";
  if (figtype!="fig") res += "(*)";
  res += "Source: <A HREF=\""+figid+".fig\"><!-- @Figure source@ -->"+figid+".fig</A><BR>\n";
  if (figtype!="ps") res += "(*)";
  res += "PostScript: <A HREF=\""+figid+".ps\"><!-- @Figure PostScript@ -->"+figid+".ps</A><BR>\n";
  res += "Caption: <!-- @Figure caption@ --><BR>\nPSfrag conversions: <PRE><!-- @Begin Figure psconversions@ -->\n<!-- @End Figure psconversions@ --></PRE>\n";
  res += "Created with:\n<!-- @End FIGURE DATA@ -->\n\n";
  return res;
}

void get_figure_data(String & figfile, String & figlabel, String & figcaption, String & figintext, String & psconversions, StringList & srcf, StringList & srcfname, int & srcfnum) {
// obtains missing figure data from the Figures Directory
// this function can be called with data in either figfile or figlabel
  const BigRegex figintextrx("%[ 	]*@Begin INTEXT@[ \t\n]*\\(.*\\)%[ 	]*@End INTEXT@");
  int srcidx;
  if ((srcidx = get_file_in_list(figuresdirectory,srcf,srcfname,srcfnum))<0)
    WARN << "get_figure_data(): Some figure data may be missing.\nContinuing as is.\n";
  else {
    // find figure data
    String findid;
    if (!figfile.empty()) findid = relurl(figuresdirectory,figfile);
    else if (!figlabel.empty()) findid = figlabel;
    else WARN << "get_figure_data(): No figure file or label, some figure data may be missing.\ncontinuing as is.\n";
    if (!findid.empty()) {
      int figididx;
      if ((figididx = srcf[srcidx].index(BigRegex("[<]!--[ 	]@Begin FIGURE DATA:[^@]*"+RX_Search_Safe(findid)+"[^@]*@[ 	]*--[>]")))<0) {
	WARN << "get_figure_data(): Figure " << findid << " not found.\nContinuing as is.\n";
	if (!figfile.empty()) figfile = findid;
	if (figlabel.empty()) {
	  figlabel = figfile.after('/',-1);
	  if (figlabel.empty()) figlabel = figfile;
	  if (figlabel.index('.',-1)>=0) figlabel = figlabel.before('.',-1);
	  figlabel.prepend("fig:");
	}
      } else {
	int figidend;
	if ((figidend = srcf[srcidx].index(BigRegex("[<]!--[ 	]*@End FIGURE DATA@[ 	]*--[>]"),figididx))<0)
	  WARN << "get_figure_data(): Figure " << findid << " End marker missing.\nContinuing as is.\n";
	else {
	  String figdata = srcf[srcidx].at(figididx,figidend-figididx);
	  // get figure label
	  figlabel = figdata.at(BigRegex("[<]!--[ 	]*@Figure label@[ 	]*--[>][ 	]*[^]< 	\n]+"));
	  figlabel = figlabel.after(BigRegex("[<]!--[ 	]*@Figure label@[ 	]*--[>][ 	]*"));
	  // get figure file
	  figfile = figdata.at(BigRegex("[<]!--[ 	]*@Figure file@[ 	]*--[>][ 	]*[^< 	\n]+"));
	  figfile = figfile.after(BigRegex("[<]!--[ 	]*@Figure file@[ 	]*--[>][ 	]*"));
	  int fci = figdata.index(BigRegex("[<]!--[ 	]*@Figure caption@[ 	]*--[>][ 	]*"),String::SEARCH_END);
	  // get figure caption
	  figcaption = ""; figintext = "";
	  if (fci<0) WARN << "get_figure_data(): Figure " << findid << " ``@Figure caption@'' marker missing in.\nContinuing as is.\n";
	  else {
	    int fce = figdata.index(BigRegex("[<][Bb][Rr][>][ \t\n]*PSfrag"),fci);
	    if (fce<0) WARN << "et_figure_data(): Figure " << findid << " ``<BR>\\nPSfrag'' caption end code missing.\nContinuing as is.\n";
	    else {
	      figcaption = figdata.at(fci,fce-fci);
	      if (figcaption.index(figintextrx)>=0) { // identify ``INTEXT'' figure descriptions
		figintext = "% @Begin INTEXT@\n" + figcaption.sub(figintextrx,1) + "% @End INTEXT@\n"; // place new markers to ensure line ends
		fci = figintextrx.subpos(0); fce = figintextrx.subpos(0)+figintextrx.sublen(0);
		if (fci>0) if (figcaption.elem(fci-1)=='\n') fci--;
		// note that figcaption.length() loses value ranges here due to conversion to signed int
		if (fce<(int) figcaption.length()) if (figcaption.elem(fce)=='\n') fce++;
		figcaption.del(fci,fce-fci);
	      }
	      // remove troublesome characters from figure caption (cannot use the convert_item_content_to_TeX() function,
	      // since content is already assumed to be in TeX format with codes such as $math-stuff$
	      if (figcaption.gsub(BigRegex("\\([^\\]\\)[%<>]"),"_1X",'_')>0) if (verbose) INFO << "TeX Correction: Changed special characters in figure caption to `X'\n";
	      if (figcaption.gsub(BigRegex("^[%<>]"),"X")>0) if (verbose) INFO << "TeX Correction: Changed special characters in figure caption to `X'\n";
	    }
	  }
	  // get PS conversions
	  int pscidx;
	  if ((pscidx = figdata.index(BigRegex("[<]!--[ 	]*@Begin Figure psconversions@[ 	]*--[>][ 	]*"),String::SEARCH_END))>=0) {
	    int pscend;
	    if ((pscend = figdata.index(BigRegex("[<]!--[ 	]*@End Figure psconversions@[ 	]*--[>]"),pscidx))>=0)
	      psconversions = figdata.at(pscidx,pscend-pscidx);
	    if (psconversions=="\n") psconversions = "";
	  }
	}
      }
    }
  }
  if (figfile.empty()) figfile = homedir+"doc/tex/common-eps/placeholder.eps";
  if (figlabel.empty()) figlabel = "fig:unknown";
  if (figcaption.empty()) figcaption = "\\relax";
  if (psconversions.empty()) psconversions = "\\relax";
}

bool get_Paper_Plans_Section_IDs(StringList & ppsectionids, StringList & ppsectiontitles, StringList & ppsectiontemplates) {
/* obtain registered IDs of paper Sections as
   listed in the
   <A HREF="../../doc/html/lists/paper-plans.html#PST">
   Paper Plans
   </A> file */
	String ppfstr;
	if (!read_file_into_String(paperplansfile,ppfstr)) return false;
	int pstidx;
	if ((pstidx = ppfstr.index(BigRegex("[<]A[ 	]+[Nn][Aa][Mm][Ee][ 	]*=[ 	]*\"PST\"")))<0) {
		EOUT << "dil2al: Unable to find Paper Section Titles in Paper Plans document in get_Paper_Plans_Section_IDs()\n";
		return false;
	}
	if ((pstidx = ppfstr.index("<UL>",pstidx))<0) {
		EOUT << "dil2al: Section IDs list not found in Paper Plans document in get_Paper_Plans_Section_IDs()\n";
		return false;
	}
	int pstend = ppfstr.index("</UL>",pstidx);
	int pstnum = 0;
	String pstid, psttitle, psttemplate;
	while ((pstidx = ppfstr.index("<LI>",pstidx+1))>=0) {
		if ((unsigned int) pstidx < (unsigned int) pstend) {
			if ((pstid = ppfstr.at(BigRegex("[[]\\([<][^>]*[>]\\)?[^]]+"),pstidx))!="") {
				pstid.del("["); pstid.gsub(BigRegex("[<][^>]*[>]"),""); pstid.gsub(BigRegex("[ 	]+"),"");
				if ((psttitle = ppfstr.at(BigRegex("[]][^\n]+"),pstidx))!="") {
					psttitle = psttitle.after(BigRegex("[]][ 	]*"));
					psttemplate = psttitle.after(BigRegex("[<]A[ 	]+[^>]*[Hh][Rr][Ee][Ff][ 	]*=[ 	]*\""));
					psttemplate = psttemplate.before("\"");
					if (psttemplate!="") psttemplate = absurl(paperplansfile,psttemplate);
					psttitle.gsub(BigRegex("[<][^>]*[>]"),"");
					psttitle.del(BigRegex("[ 	]*$"));
					psttitle.upcase();
					ppsectionids[pstnum] = pstid;
					ppsectiontitles[pstnum] = psttitle;
					ppsectiontemplates[pstnum] = psttemplate;
					pstnum++;
				}
			}
		} else break;
	}
	return true;
}

// #define DEBUG_DO_NOT_CREATE_NEW_OUTLINE

void extract_required_item_to_paper(String & ostrtext, String refurl, Novelty_Marker & nm, String & sectitle, String & requireditems, String & pprevtext) {
  // generates output for a Required item with HTML tagged .TeX comments
  // indicating item qualifiers, quantifiers and additional marked text
  // additionally keeps track of a list of required items to be updated
  // in the paper plan and used for AL priority DIL entries
  
#ifdef DEBUG_PPEXTRACT
  cout << 'M'; cout.flush();
#endif
  String contextconcat = nm.context.concatenate(", ");
  String impconcat;
  char fc[10];
  if (nm.imp) {
    sprintf(fc,"%4.2f",nm.imp[0]);
    impconcat = fc;
    for (int i=1; i<nm.impnum; i++) {
      sprintf(fc,"%4.2f",nm.imp[i]);
      impconcat += ", "; impconcat += fc;
    }
  }
  // test if REWRITTEN
  int previdx;
  if ((previdx = pprevtext.index(BigRegex("\n[ 	]*%[ 	]*@Begin[ 	]+[^@]*REWRITTEN[^@]*[ 	]+REQUIRED:[ 	]*[[]"+RX_Search_Safe(sectitle)+"[]][ 	]*"+RX_Search_Safe(contextconcat)+"[ 	]*@")))>=0) {
    int prevend;
    if ((prevend = pprevtext.index(BigRegex("\n[ 	]*%[ 	]*@End[ 	]+[^@]*REWRITTEN[^@]*[ 	]+REQUIRED:[ 	]*[[]"+RX_Search_Safe(sectitle)+"[]][ 	]*"+RX_Search_Safe(contextconcat)+"[ 	]*@"),String::SEARCH_END,previdx))>=0) {
      ostrtext += pprevtext.at(previdx,prevend-previdx);
      if (verbose) { INFO << "+R"; VOUT.flush(); }
      if ((!askextractconcat) || confirmation("Append newly extracted version of REQUIRED ``"+contextconcat+"''? (y/N) ",'y')) return;
      contextconcat += " (newly extracted version)";
    } else WARN << "extract_required_item_to_paper(): Missing @End REWRITTEN REQUIRED: [section] concatenated-context@.\nExtracting from source.\n";
  }
  // include required in outline
  ostrtext += "\n% @Begin REQUIRED: ["+sectitle+"] "+contextconcat+"@\n% Context reference: ";
  ostrtext += refurl;
  if (nm.imp) ostrtext += "\n% Importance: " + impconcat;
  sprintf(fc,"%4.2f",nm.len);
  ostrtext += "\n% Intended length: "; ostrtext += fc;
  ostrtext += " pages\n\\editnote{REQUIRED: " + contextconcat;
  if (nm.marktext!="") ostrtext += " (" + nm.marktext + ')';
  ostrtext += "}\n% @End REQUIRED: ["+sectitle+"] "+contextconcat+"@\n\n";
  // collect required items for addition to paper plan
  sprintf(fc,"%5.2f",nm.len);
  if (nm.marktext!="") contextconcat.prepend("<I>"+nm.marktext+";</I> ");
  requireditems += "<LI>["+sectitle+"] "+contextconcat+" (imp: "+((impconcat+", len: ")+fc)+")\n";
}

// #define _ASK_APPEND_TO_REWRITTEN_OUT_OF_PLACE

int TeX_get_scope(String & textstr, int start, String & scopecontent) {
  // get the text between brackets starting at start
  // returns location after closing bracket or -1 if there is no closing bracket
  
  const BigRegex brck1rx("[\\{\\}]"); // \\{ rather than { because of regex-gnu.h/BigRegex.hh setting
  const BigRegex brck2rx("[][]");
  const BigRegex brck3rx("[()]");
  const BigRegex * brckrx = NULL;
  char bracket = textstr.elem(start);
  switch (bracket) {
  case '{': brckrx = &brck1rx; break;
  case '[': brckrx = &brck2rx; break;
  case '(': brckrx = &brck3rx; break;
  }
  int depth = 1, i = start+1;
  while ((depth>0) && ((i=textstr.index(*brckrx,i))>=0)) {
    if (i>0) if (textstr.elem(i-1)=='\\') { i++; continue; } // escaped bracket
    if (textstr.elem(i)==bracket) depth++; else depth--;
    i++;
  }
  if (i>=0) scopecontent = textstr.at(start+1,i-(start+2));
  return i;
}

int TeX_get_argument(String & textstr, int start, String & argstr, char argtype, int * argstart) {
  // get argument text between argtype brackets after start
  // returns location after closing bracket or -1 if there is no such argument
  // white space characters may precede the opening bracket
  
  const BigRegex nonwsrx("[^ \n\t\r\v\f]");
  if ((start=textstr.index(nonwsrx,start))<0) return -1;
  if (textstr.elem(start)!=argtype) return -1;
  if (argstart) *argstart = start;
  return TeX_get_scope(textstr,start,argstr);
}

int TeX_get_command(String & textstr, int start, String & cmdstr) {
  // get command and arguments text (\command[]{}{}{}{}{}{}{}{}) at start
  // returns location
  // *** if you need function that find commands or other chunks after rather than
  //     at a certain location, in the same manner as the HTML utilities, then
  //     either add a paramter that controls such an option, or create wrapper
  //     functions that search for a chunk and then call these functions
  
  const BigRegex TeXcmd("\\(\\\\[A-Za-z]+\\).*"); // TeX command identifier
  if (!textstr.matches(TeXcmd,start)) return -1;
  // get command
  int cend = TeXcmd.subpos(1)+TeXcmd.sublen(1);
  String argstr; int j, argsleft = 9;
  // find possible optional argument
  if ((j=TeX_get_argument(textstr,cend,argstr,'['))>=0) {
    cend = j; argsleft--;
  }
  // find and convert possible mandatory arguments
  while ((argsleft>0) && ((j=TeX_get_argument(textstr,cend,argstr,'{'))>=0)) {
    cend = j; argsleft--;
  }
  cmdstr = textstr.at(start,cend-start);
  return cend;
}

String TeX_safe_text(String & title) {
  // produces a version of title in proper TeX format from plain text
  // no attempt is made to convert '\', since it is assumed to precede
  // TeX commands that are frequently embedded in notes
  // there are two forms of recognized embedded TeX bracketing:
  // (1) foo\somecommand [text0] {text1} {text2} {text3} bar (with optional space)
  // (2) foo{\somecommand text}bar
  // all other curly brackets and hanging curly brackets are converted
  
  const BigRegex TeXchar("[#$%&_]"); // TeX special characters
  const BigRegex TeXcmdctx("[\\{}]"); // TeX command context indicators
  const BigRegex TeXcmd("\\(\\\\[A-Za-z]+\\).*"); // TeX command identifier
  const BigRegex TeXscopecmd("{[ \n\t\r\v\f]*\\\\[A-Za-z]+.*"); // TeX scope limited command identification
  int convertedto = 0, i = 0;
  String convtitle;
  while ((i=title.index(TeXcmdctx,convertedto))>=0) {
    convtitle += title.at(convertedto,i-convertedto);
    switch (title.elem(i)) {
    case '\\': if (title.matches(TeXcmd,i)) {
	// find and convert command
	convtitle += title.sub(TeXcmd,1);
	convertedto = i + TeXcmd.sublen(1);
	String argstr; int argstart = 0, j, argsleft = 9;
	// find and convert possible optional argument
	if ((j=TeX_get_argument(title,convertedto,argstr,'[',&argstart))>=0) {
	  convtitle += title.at(convertedto,argstart-convertedto) + '[' + TeX_safe_text(argstr) + ']';
	  convertedto = j; argsleft--;
					}
	// find and convert possible mandatory arguments
	while ((argsleft>0) && ((j=TeX_get_argument(title,convertedto,argstr,'{',&argstart))>=0)) {
	  convtitle += title.at(convertedto,argstart-convertedto) + '{' + TeX_safe_text(argstr) + '}';
	  convertedto = j; argsleft--;
	}
      } else {
	convtitle += "$\\backslash$"; convertedto = i+1;
      } break;
    case '{': if (title.matches(TeXscopecmd,i)) {
	String scopecontent;
	int j = TeX_get_scope(title,i,scopecontent);
	if (j<0) {
	  convtitle += "\\{"; convertedto = i+1;
	} else {
	  convtitle += '{' + TeX_safe_text(scopecontent) + '}'; convertedto = j;
	}
      } else {
	convtitle += "\\{"; convertedto = i+1;
      } break;
    case '}': convtitle += "\\}"; convertedto = i+1; break;
    }
  }
  convtitle += title.from(convertedto);
  convtitle.gsub(TeXchar,"\\_0",'_');
  return convtitle;
}

void TeX_format_corrections(String & ostrtext, bool estimate) {
  // corrections applied to a generated TeX text
  // rather than assuming a plain text source as above, this function assumes
  // a TeX draft
  
  const BigRegex TeXerr_refrx("\\\\\\(ref\\|cite\\)\\([{]\\)[^}]*\\([{\\]\\)");
  const BigRegex TeXnon_refrx("[^-0-9A-Za-z:._+]");
  const BigRegex TeXnon_citerx("[^-0-9A-Za-z:._+,]");
  const BigRegex TeXerr_reflblrx("\\\\ref[{]\\([^}]*[^-0-9A-Za-z:._+}][^}]*\\)[}]");
  const BigRegex TeXerr_citelblrx("\\\\cite[{]\\([^}]*[^-0-9A-Za-z:._+,}][^}]*\\)[}]");
  const BigRegex TeXfighererx("\\\\[A-za-z]*figurehere");
  int i,j,k; String movestr, reststr;
  // a) \ref{ \command[]{}{} } or \ref{ {  } } or \cite{ \command[]{}{} } or \cite{ {  } }
  while ((i=ostrtext.index(TeXerr_refrx))>=0) { // iteratively applied to the whole text
    if ((k=TeX_get_command(ostrtext,i,reststr))>=0) {
      switch (ostrtext.elem(TeXerr_refrx.subpos(3))) {
      case '\\': // move interfering command after \ref{} or \cite{}
	if ((j=TeX_get_command(ostrtext,TeXerr_refrx.subpos(3),movestr))>=0) {
	  reststr = ostrtext.at(j,k-j)+movestr+ostrtext.from(k);
	  ostrtext = ostrtext.before(TeXerr_refrx.subpos(3));
	  ostrtext += reststr;
	  if (verbose) INFO << "TeX Correction: Moved TeX command out of " << String(ostrtext.at(i,(k-movestr.length())-i)) << '\n';
	} else { // loose `\'
	  ostrtext[TeXerr_refrx.subpos(3)] = '-';
	  if (verbose) INFO << "TeX Correction: Changed `\\' to `-' in " << String(ostrtext.at(i,k-i)) << '\n';
	}
	break;
      case '{': // move interfereing scope after \ref{} or \cite{}
	if ((j=TeX_get_scope(ostrtext,TeXerr_refrx.subpos(3),movestr))>=0) {
	  reststr = ostrtext.at(j,k-j)+'{'+movestr+'}'+ostrtext.from(k);
	  ostrtext = ostrtext.before(TeXerr_refrx.subpos(3));
	  ostrtext += reststr;
	  if (verbose) INFO << "TeX Correction: Moved TeX scope out of " << String(ostrtext.at(i,(k-(movestr.length()+2))-i)) << '\n';
	} else { // loose `{'
	  ostrtext[TeXerr_refrx.subpos(3)] = '+';
	  if (verbose) INFO << "TeX Correction: Changed `{' to `+' in " << String(ostrtext.at(i,k-i)) << '\n';
	}
	break;
      }
    } else { // missing `}'
      if ((j=ostrtext.index(TeXnon_refrx,TeXerr_refrx.subpos(2)+1))>=0) {
	reststr = ostrtext.from(j);
	ostrtext = ostrtext.before(j);
	ostrtext += '}' + reststr;
      } else ostrtext += '}';
      if (verbose) INFO << "TeX Correction: Added missing } for " << String(ostrtext.at(i,j-i)) << '\n';
    }
  }
  // b) \ref{ non-label-characters }
  i = 0;
  while ((i=ostrtext.index(TeXerr_reflblrx,i))>=0) { // single pass
    movestr = ostrtext.sub(TeXerr_reflblrx,1);
    movestr.gsub(TeXnon_refrx,"X");
    if (verbose) INFO << "TeX Correction: Changed non-label characters to `X' in \\ref{" << movestr << "}\n";
    if (estimate) { // attempt to estimate the intended reference label
      j = 0; String argstr;
      while ((j=ostrtext.index(TeXfighererx,String::SEARCH_END,j))>=0) {
	if ((k=TeX_get_argument(ostrtext,j,argstr,'{'))>=0) {
	  if ((k=TeX_get_argument(ostrtext,k,argstr,'{'))>=0) {
	    if (movestr.matches(BigRegex("X*"+RX_Search_Safe(argstr)+"X*"))) {
	      if (verbose) VOUT << "TeX Correction: Assuming that \\ref{" << movestr << "} actually refers to ``" << argstr << "''\n";
	      movestr = argstr;
	      break;
	    }
	    j = k;
	  }
	}
      }
      
    }
    ostrtext.at(TeXerr_reflblrx.subpos(1),TeXerr_reflblrx.sublen(1)) = movestr;
  }
  // c) \cite{ non-label-characters }
  // *** can compare with all known bibliography keys
  i = 0;
  while ((i=ostrtext.index(TeXerr_citelblrx,i))>=0) { // single pass
    movestr = ostrtext.sub(TeXerr_citelblrx,1);
    movestr.gsub(TeXnon_citerx,"X");
    if (verbose) INFO << "TeX Correction: Changed non-label characters to `X' in \\cite{" << movestr << "}\n";
    ostrtext.at(TeXerr_citelblrx.subpos(1),TeXerr_citelblrx.sublen(1)) = movestr;
  }
}

void extract_hierarchy_depth_increase_to_paper(String & ostrtext, Novelty_Marker & nm, StringList & cc, LongList & ccr, String & sectitle, StringList & srcf, StringList & srcfname, int & srcfnum, String & pprevtext) {
  // generates output for a Topical Context Hierarchy depth increase, suggesting
  // subtopics and indicating the TCH title in .TeX comments
  
  const BigRegex shrrx("\\(Significance Hierarchy Rank:\\)[ 	]*[0-9]+[.][0-9]+");
  // test if REWRITTEN
#ifdef DEBUG_PPEXTRACT
  cout << 'N'; cout.flush();
#endif
  int previdx; String contextconcat = nm.context.concatenate(", ");
  if ((previdx = pprevtext.index(BigRegex("\n[ 	]*%[ 	]*@Begin[ 	]+[^@]*REWRITTEN[^@]*[ 	]+SUBTOPIC:[ 	]*[[]depth[^],]*,[ 	]*"+RX_Search_Safe(sectitle)+"[]][ 	]*"+RX_Search_Safe(contextconcat)+"[ 	]*@")))>=0) {
    int prevend, prevheadend;
    if ((prevend = pprevtext.index(BigRegex("\n[ 	]*%[ 	]*@End[ 	]+[^@]*REWRITTEN[^@]*[ 	]+SUBTOPIC:[ 	]*[[]depth[^],]*,[ 	]*"+RX_Search_Safe(sectitle)+"[]][ 	]*"+RX_Search_Safe(contextconcat)+"[ 	]*@"),String::SEARCH_END,previdx))>=0) {
      if (((prevheadend = pprevtext.index(BigRegex("\n[ 	]*%[ 	]*@[^ 	]*SubSection HEADEND[^@]*@[^\n]*"),String::SEARCH_END,previdx))>=0) && (prevheadend<prevend)) {
	ostrtext += pprevtext.at(previdx,prevheadend-previdx); // full template head
      } else {
	if (((prevheadend = pprevtext.index(BigRegex(".*\\([^ 	}]subsection[{][^\n]*\\|[\\]paragraph[{][^\n]*\\|[\\][\\]\\)\n\n"),String::SEARCH_END,previdx))>=0) && (prevheadend<prevend)) {
	  ostrtext += pprevtext.at(previdx,prevheadend-previdx); // subtopic title
	} else {
	  WARN << "extract_hierarchy_depth_increase_to_paper(): No template header or subtopic heading found.\nExtracting from source.\n";
	  prevheadend = -1;
	}
      }
      if (prevheadend>=0) {
	if (verbose) { INFO << "+RI"; VOUT.flush(); }
#ifdef _ASK_APPEND_TO_REWRITTEN_OUT_OF_PLACE
	if ((!askextractconcat) || confirmation("Append newly extracted version of SUBTOPIC Head ``"+contextconcat+"''? (y/N) ",'y')) return;
	contextconcat += " (newly extracted version)";
#else
	return;
#endif
      }
    } else WARN << "extract_hierarchy_depth_increase_to_paper(): Missing @End REWRITTEN SUBTOPIC: [depth n, section] concatenated-context@.\nExtracting from source.\n";
  }
  // place subtopic head
  int templateidx, tidx, tend;
  ostrtext += "\n% @Begin SUBTOPIC: [depth " + (String((long) nm.hierdepth) + (", "+sectitle+"] " + contextconcat + "@\n"));
  switch (nm.hierdepth) {
  case 0:	ostrtext += "% (Unexpected hierarchy depth at ``\\section'' level.)\n\n";
    WARN << "extract_hierarchy_depth_increase_to_paper(): Unexpected hierarchy depth at ``\\section'' level. Continuing.\n";
    break;
  case 1: 
    // load template if not already loaded
    if ((templateidx = get_file_in_list(templatearticlesubsection,srcf,srcfname,srcfnum))<0) {
      WARN << "\nContinuing as is.\n";
      ostrtext += "\\subsection{" + TeX_safe_text(contextconcat) + "}\n\n"; // no template
    } else {
      // fill in estimated significance rank
      int shridx = cc.iselement(contextconcat), r = 0;
      if (shridx>=0) r = ccr[shridx];
      String shrrepl("_1 "+String((double) r,"%.1f"));
      srcf[templateidx].gsub(shrrx,shrrepl,'_');
      // fill in subtopic information
      tidx = srcf[templateidx].index(BigRegex("%[ 	]*@Begin SUBTOPIC:[^@]*@[^\n]*\n"),String::SEARCH_END);
      tend = srcf[templateidx].index(BigRegex("\n[\\]subsection[{]"),String::SEARCH_END,tidx);
      if ((tidx<0) || (tend<0)) {
	WARN << "extract_hierarchy_depth_increase_to_paper(): Missing SUBTOPIC at depth 1, ``" << contextconcat << "''.\nContinuing as is.\n";
	ostrtext += "\\subsection{";
      } else ostrtext += srcf[templateidx].at(tidx,tend-tidx);
      ostrtext += TeX_safe_text(contextconcat) + "}\n% --><H3>"+contextconcat+"</H3><!--\n\n";
      tidx = srcf[templateidx].index(BigRegex("[}][ 	]*\n[ 	]*\n"),String::SEARCH_END,tend);
      tend = srcf[templateidx].index(BigRegex("\n[ 	]*%[ 	]*@SubSection HEADEND[^@]*@[^\n]*"),String::SEARCH_END,tidx);
      if ((tidx<0) || (tend<0)) WARN << "extract_hierarchy_depth_increase_to_paper(): Missing HEADEND at depth 1, ``" << contextconcat << "''.\nContinuing as is.\n";
      else ostrtext += srcf[templateidx].at(tidx,tend-tidx);
    }
    break;
  case 2:
    // load template if not already loaded
    if ((templateidx = get_file_in_list(templatearticlesubsubsection,srcf,srcfname,srcfnum))<0) {
      WARN << "\nContinuing as is.\n";
      ostrtext += "\\subsubsection{" + TeX_safe_text(contextconcat) + "}\n\n"; // no template
    } else {
      // fill in estimated significance rank
      int shridx = cc.iselement(contextconcat), r = 0;
      if (shridx>=0) r = ccr[shridx];
      String shrrepl("_1 "+String((double) r,"%.1f"));
      srcf[templateidx].gsub(shrrx,shrrepl,'_');
      // fill in subtopic information
      tidx = srcf[templateidx].index(BigRegex("%[ 	]*@Begin SUBTOPIC:[^@]*@[^\n]*\n"),String::SEARCH_END);
      tend = srcf[templateidx].index(BigRegex("\n[\\]subsubsection[{]"),String::SEARCH_END,tidx);
      if ((tidx<0) || (tend<0)) {
	WARN << "extract_hierarchy_depth_increase_to_paper(): Missing SUBTOPIC at depth 2, ``" << contextconcat << "''.\nContinuing as is.\n";
	ostrtext += "\\subsubsection{";
      } else ostrtext += srcf[templateidx].at(tidx,tend-tidx);
      ostrtext += TeX_safe_text(contextconcat) + "}\n% --><H4>"+contextconcat+"</H4><!--\n\n";
      tidx = srcf[templateidx].index(BigRegex("[}][ 	]*\n[ 	]*\n"),String::SEARCH_END,tend);
      tend = srcf[templateidx].index(BigRegex("\n[ 	]*%[ 	]*@SubSubSection HEADEND[^@]*@[^\n]*"),String::SEARCH_END,tidx);
      if ((tidx<0) || (tend<0)) WARN << "extract_hierarchy_depth_increase_to_paper(): Missing HEADEND at depth 2, ``" << contextconcat << "''.\nContinuing as is.\n";
      else ostrtext += srcf[templateidx].at(tidx,tend-tidx);
    }
    break;
  case 3: ostrtext += "\\paragraph{" + TeX_safe_text(contextconcat) + "}\n\n"; break;
  case 4: ostrtext += "\\\\\n\n"; break;
  default:	ostrtext += "% (Hierarchy depth exceeds all \\LaTeX subsection levels.)\n\n";
    WARN << "extract_hierarchy_depth_increase_to_paper(): Hierarchy depth exceeds all \\LaTeX subsection levels. Continuing.\n";
  }
}

void extract_hierarchy_depth_decrease_to_paper(String & ostrtext, Novelty_Marker & nm, String & sectitle, StringList & srcf, StringList & srcfname, int & srcfnum, String & pprevtext) {
// generates output for a Topical Context Hierarchy depth decrease, suggesting
// subtopics and indicating the TCH title in .TeX comments
#ifdef DEBUG_PPEXTRACT
	cout << 'O'; cout.flush();
#endif
	// get corresponding subtopic context
	String depthstr = String((long) nm.hierdepth+1); String subtopicid, contextconcat;
	int ostridx;
//*** may have to allow for REWRITTEN in front of SUBTOPIC here
	if ((ostridx = ostrtext.index(BigRegex("\n[ 	]*%[ 	]*@Begin SUBTOPIC:[ 	]*[[]depth[ 	]+"+depthstr+"[ 	]*,[ 	]*"+RX_Search_Safe(sectitle)+"[]]"),-1))<0) {
		EOUT << "dil2al: Missing @Begin SUBTOPIC: [depth n, section] concatenated-context@ in extract_hierarchy_depth_decrease_to_paper(), continuing as is\n";
		ostrtext += "\n% @WARNING: Missing @Begin SUBTOPIC: [depth n, section] concatenated-context@\n";
		return;
	} else {
		// get identification string
		subtopicid = ostrtext.at(BigRegex("[^@]*@[^@]+@"),ostridx);
		contextconcat = subtopicid.after("@Begin SUBTOPIC:");
		subtopicid.gsub("@Begin ","@End ");
		subtopicid += "\n\n";
		contextconcat.del('@');
		contextconcat = contextconcat.after(']');
		contextconcat.del(BigRegex("^[ 	]*"));
	}
#ifdef DEBUG_PPEXTRACT
	cout << 'P'; cout.flush();
#endif
	// test if REWRITTEN
	int previdx; //String contextconcat = nm.context.concatenate(", ");
	if ((previdx = pprevtext.index(BigRegex("\n[ 	]*%[ 	]*@Begin[ 	]+[^@]*REWRITTEN[^@]*[ 	]+SUBTOPIC:[ 	]*[[]depth[^],]*,[ 	]*"+RX_Search_Safe(sectitle)+"[]][ 	]*"+RX_Search_Safe(contextconcat)+"[ 	]*@")))>=0) {
		int prevend, prevtailbegin;
		if ((prevend = pprevtext.index(BigRegex("\n[ 	]*%[ 	]*@End[ 	]+[^@]*REWRITTEN[^@]*[ 	]+SUBTOPIC:[ 	]*[[]depth[^],]*,[ 	]*"+RX_Search_Safe(sectitle)+"[]][ 	]*"+RX_Search_Safe(contextconcat)+"[ 	]*@"),String::SEARCH_END,previdx))>=0) {
			if (((prevtailbegin = pprevtext.index(BigRegex("\n[ 	]*%[^@\n]*@[^ 	]*SubSection TAILBEGIN[^@]*@"),previdx))>=0) && (prevtailbegin<prevend)) {
				ostrtext += pprevtext.at(prevtailbegin,prevend-prevtailbegin); // full template tail
			} else {
				if (((prevtailbegin = pprevtext.index(BigRegex("\n[ 	]*%[ 	]*@End[ 	]+[^@]*REWRITTEN[^@]*[ 	]+SUBTOPIC:[ 	]*[[]depth[^],]*,[ 	]*"+RX_Search_Safe(sectitle)+"[]][ 	]*"+RX_Search_Safe(contextconcat)+"[ 	]*@"),previdx))>=0) && (prevtailbegin<prevend)) {
					ostrtext += pprevtext.at(prevtailbegin,prevend-prevtailbegin); // subtopic end tag
				} else {
					EOUT << "dil2al: No template tail or subtopic end tag found in extract_hierarchy_depth_decrease_to_paper(), extracting from source\n";
					prevtailbegin = -1;
				}
			}
			if (prevtailbegin>=0) {
				if (verbose) { VOUT << "+RD"; VOUT.flush(); }
#ifdef _ASK_APPEND_TO_REWRITTEN_OUT_OF_PLACE
				if ((!askextractconcat) || confirmation("Append newly extracted version of SUBTOPIC Tail ``"+contextconcat+"''? (y/N) ",'y')) return;
				subtopicid = subtopicid.before('@',-1)+" (newly extracted version)@\n\n";
#else
				return;
#endif
			}
		} else EOUT << "dil2al: Missing @End REWRITTEN SUBTOPIC: [depth n, section] concatenated-context@ in extract_hierarchy_depth_decrease_to_paper(), extracting from source\n";
	}
#ifdef DEBUG_PPEXTRACT
	cout << 'Q'; cout.flush();
#endif
	// place subtopic tail
	int templateidx, tidx, tend;
	switch (nm.hierdepth+1) {
		case 0: ostrtext += "% (Unexpected hierarchy depth at ``\\section'' level.)\n\n";
				EOUT << "dil2al: Unexpected hierarchy depth at ``\\section'' level in extract_hierarchy_depth_decrease_to_paper(), continuing\n";
				break;
		case 1: 
#ifdef DEBUG_PPEXTRACT
			cout << 'R'; cout.flush();
#endif
			// load template if not already loaded
			if ((templateidx = get_file_in_list(templatearticlesubsection,srcf,srcfname,srcfnum))<0) {
				EOUT << "continuing as is\n"; // no template
			} else {
				tidx = srcf[templateidx].index(BigRegex("\n%[^@\n]*@SubSection TAILBEGIN[^@]*@"));
				tend = srcf[templateidx].index(BigRegex("\n%[ 	]*@End SUBTOPIC:[^@]*@"),tidx);
				if ((tidx<0) || (tend<0)) EOUT << "dil2al: Missing TAILBEGIN at depth 1, ``" << contextconcat << "'' in extract_hierarchy_depth_decrease_to_paper(), continuing as is\n";
				else ostrtext += srcf[templateidx].at(tidx,tend-tidx);
			}
			break;
		case 2:
#ifdef DEBUG_PPEXTRACT
			cout << 'S'; cout.flush();
#endif
			// load template if not already loaded
			if ((templateidx = get_file_in_list(templatearticlesubsubsection,srcf,srcfname,srcfnum))<0) {
				EOUT << "continuing as is\n"; // no template
			} else {
				tidx = srcf[templateidx].index(BigRegex("\n%[^@\n]*@SubSubSection TAILBEGIN[^@]*@"));
				tend = srcf[templateidx].index(BigRegex("\n%[ 	]*@End SUBTOPIC:[^@]*@"),tidx);
				if ((tidx<0) || (tend<0)) EOUT << "dil2al: Missing TAILBEGIN at depth 2, ``" << contextconcat << "'' in extract_hierarchy_depth_decrease_to_paper(), continuing as is\n";
				else ostrtext += srcf[templateidx].at(tidx,tend-tidx);
			}
			break;
		case 3: break;
		case 4: break;
		default:	ostrtext += "% (Hierarchy depth exceeds all \\LaTeX subsection levels.)\n\n";
				EOUT << "dil2al: Hierarchy depth exceeds all \\LaTeX subsection levels in extract_hierarchy_depth_decrease_to_paper(), continuing\n";
	}
	ostrtext += subtopicid;
#ifdef DEBUG_PPEXTRACT
	cout << 'T'; cout.flush();
#endif
}

/* The following functions incorporate rules used to identify and obtain
   research novelty item content (see <A HREF="../../doc/html/planning/material.html#preparing-results>
   Paper Plans: preparing results
   </A>) */

bool item_content_between_range_tags(Novelty_Marker & nm, int nidx, String & src, String & ic) {
	// 1. search for content range tags
	int nstartidx = 0, nendidx = 0;
	if (((nstartidx = src.index(BigRegex("[<]!--[ 	]*@Begin[ 	]+"+RX_Search_Safe(nm.id)+"@[ 	]*--[>]"),nidx-src.length()))<0)
	 || ((nendidx = src.index(BigRegex("[<]!--[ 	]*@End[ 	]+"+RX_Search_Safe(nm.id)+"@[ 	]*--[>]"),nidx))<0)) return false;
	ic = src.at(nstartidx,nendidx-nstartidx);
	return true;
}

bool item_content_within_TL_chunk(int nidx, String & src, String & ic) {
	// 4. content range when within Task Log chunk
	int nstartidx = 0, nendidx = 0;
	if ((!src.contains(BigRegex("[<]TITLE[>]Task Log ([^)]*)[<]/TITLE[>]")))
	 || ((nstartidx = src.index(BigRegex("[<]!--[ 	]*chunk Begin[ 	]*--[>]"),nidx-src.length()))<0)
	 || ((nendidx = src.index(BigRegex("[<]!--[ 	]*chunk End[ 	]*--[>]"),nidx))<0)) return false;
	if ((nstartidx = src.index("<P>",nstartidx))<0) return false;
	nstartidx+=3;
	ic = src.at(nstartidx,nendidx-nstartidx);
	// remove TL entry headers
	ic.gsub(BigRegex("[<]!--[ 	]*entry [^ 	]+[ 	]*--[>][^\n]*\n"),"");
	return true;
}

bool item_content_within_DIL_entry(int nidx, String & src, String & ic) {
	// 5. content range when within DIL entry
	int nstartidx = 0, nendidx = 0;
	if ((!src.contains(BigRegex("[<]!--[ 	]*dil2al:[ 	]*DIL begin[ 	]*--[>]")))
	 || ((nstartidx = src.index(BigRegex("[<]TR[^>]*[>]"),nidx-src.length()))<0)) return false;
	if ((nstartidx = src.index(BigRegex("[<]TD COLSPAN[^>]*[>]"),nstartidx))<0) return false;
	if ((nendidx = src.index(BigRegex("\\([<]TR[^>]*[>]\\|[<]!--[ 	]*dil2al:[ 	]*DIL end[ 	]*--[>]\\)"),nstartidx))<0) return false;
	ic = src.at(nstartidx,nendidx-nstartidx);
	return true;
}

bool item_content_within_paragraph(int nidx, String & src, String & ic) {
  // 6. content range between <P>, empty lines, beginning and end of file
  int nstartidx = 0, nendidx = 0;
  if ((nstartidx = src.index(BigRegex("\\(\n\n\\|[<][Pp][>]\\)"),nidx-src.length()))>=0) {
    if (src[nstartidx]=='<') nstartidx+=3;
    else nstartidx+=2;
  } else nstartidx=0; // from empty line or beginning
  if ((nendidx = src.index(BigRegex("\\(\n\n\\|[<][Pp][>]\\)"),nidx))<0) nendidx = src.length(); // to empty line or end
  ic = src.at(nstartidx,nendidx-nstartidx);
  return true;
}

int Text_find_line_end(String & textstr, int start, char lend       ) {
// returns next location of lend or string length
// note that textstr.length() loses some of its value range here due to
// conversion to signed int, a conversion that is done here instead of
// declaring start as unsigned int in case this function is to be used
// with a search from the end of the string
  
  if (start>=(int) textstr.length()) return -1;
  int i = textstr.index(lend,start);
  if (i>=0) return i;
  return textstr.length();
}

void Text_limit_line_length(String & textstr, int optlinemax, int abslinemax, char lend) {
// limits text lines in textstr using optlinemax as optimal line maximum and
// abslinemax as absolute line maximum
	const BigRegex sptabrx("[ 	]");
	int i = 0, j, istart; String itembuf;
	istart = optlinemax - 50; if (istart<0) istart = 0;
	while ((j=Text_find_line_end(textstr,i,lend))>=0) {
		if ((j-i)>optlinemax) {
			if ((j=textstr.index(sptabrx,i+istart))<0) {
				j = i+optlinemax;
				itembuf = textstr.from(j); textstr.del(j,textstr.length()-j);
				textstr += lend + itembuf;
				EOUT << "dil2al: Unable to locate space for line-break, forcing break at column " << optlinemax << " in convert_item_content_to_TeX(), continuing as is\n";
			} else if (j>(i+abslinemax)) {
				j = i+optlinemax;
				itembuf = textstr.from(j); textstr.del(j,textstr.length()-j);
				textstr += lend + itembuf;
				EOUT << "dil2al: Space for line-break too distant, forcing break at column " << optlinemax << " in convert_item_content_to_TeX(), continuing as is\n";
			} else {
				textstr[j] = lend; // replace rather than insert
				if (verbose) VOUT << "TeX Correction: breaking long line at column " << (j-i) << '\n';
			}
		}
		i = j + 1;
	}
}

void convert_item_content_to_TeX(String & itemcontent) {
	// conversions to TeX (also ensures that lines do not exceed the TeX buffer size)
	//	- remove Novelty Marker tags
	itemcontent.gsub(BigRegex("[<]A[ 	]+[Nn][Aa][Mm][Ee][ 	]*=[ 	]*\"NOVELTY-[^>]*[>][[][^]]*N[^]]*[]]"),"");
	//	- identify content hyperlink references
	itemcontent.gsub(BigRegex("[<]A[ 	]+[^>]*[Hh][Rr][Ee][Ff][ 	]*=[ 	]*\"\\([^\"]+\\)\"[^>]*[>]"),"@HRef::_1@",'_');
	//	- cite, but do not recurse into bibliography references
	itemcontent.gsub(BigRegex("\\([[][^]]*\\)?@HRef::[^@]*"+RX_Search_Safe(bibindexfilename)+"#\\([^@]+:[^@]+\\)@\\([^]]*[]]\\)?"),"\\cite{_2}",'_');
	//	- sometimes citations do not contain an HRef, warn if this appears to be the case and apply citation tentatively
	int citeerrnum = itemcontent.gsub(BigRegex("[[]\\([A-Z]+:[A-Z]+[,A-Z:]*\\)[]]"),"\\cite{_1}",'_');
	if (citeerrnum>0) EOUT << "dil2al: " << citeerrnum << " apparent citations without HRef encountered in convert_item_content_to_TeX(), continuing\n";
	//	- convert paragraph and line-break codes
	itemcontent.gsub("\n<P>\n","\n\n");
	itemcontent.gsub("<P>","\n");
	itemcontent.gsub(BigRegex("\\([^\n]\\)[\n]?[<][Bb][Rr][>]"),"_1\\\\",'_');
//*** the following two lines can be removed when Thesis Log has been converted to the Task Log format
	itemcontent.gsub(BigRegex("[<]!--[ 	]*entry Begin[ 	]*--[>][^]]+[]][ 	]*[<]/FONT[>]"),"");
	itemcontent.gsub(BigRegex("[<]!--[ 	]*chunk End[ 	]*--[>][ 	]*[<][Ii][>][^<]+[<]/[Ii][>]"),"");
//*** possibly remove the following conversions
	//	- convert text attribute codes
	itemcontent.gsub(BigRegex("[<]\\([Bb]\\|[Ss][Tt][Rr][Oo][Nn][Gg]\\)[>]"),"{\\bf ");
	itemcontent.gsub(BigRegex("[<]\\([Ii]\\|[Ee][Mm]\\)[>]"),"{\\em ");
	itemcontent.gsub(BigRegex("[<]/\\([Bb]\\|[Ss][Tt][Rr][Oo][Nn][Gg]\\|[Ii]\\|[Ee][Mm]\\)[>]"),"}");
//*** possibly remove conversions above
	//	- remove remaining HTML tags
	itemcontent.gsub(BigRegex("[<][^>]*[>]"),"");
	//	- convert HTML character codes
	itemcontent.gsub("&lt;","<");
	itemcontent.gsub("&gt;",">");
	itemcontent.gsub("&amp;","\\&");
	//	- escape LaTeX special characters
	itemcontent.gsub(BigRegex("\\([^\\]\\)\\([#&_%]\\)"),"_1\\_2",'_');
//*** convert GESNlib variable names to GESN convention standard labels as found in nse.label-equivalence.tex
//*** could do that to the source once instead
	// insure lines are at most around 1000 characters long
	Text_limit_line_length(itemcontent,1000,1500);
}

void unconvert_item_content_from_TeX(String & itemcontent) {
	// conversions from TeX
	itemcontent.gsub(BigRegex("[\\]\\([#&_%]\\)"),"_1",'_');
}

void extraction_put_figure(String & ostrtext, String * figcaps, String * figps, String & figfile, StringList & srcf, StringList & srcfname, int & srcfnum, String & poutline) {
	const BigRegex nonalpharx("[^A-Za-z]");
	String figlabel, figcaption, figintext, psconversions;
	figfile = figfile.through('.',-1) + "eps"; // use .eps not .fig
	// obtain label, caption and psfrag conversions
	get_figure_data(figfile,figlabel,figcaption,figintext,psconversions,srcf,srcfname,srcfnum);
	if (ostrtext.contains('{'+figlabel+'}')) {
		if (verbose) VOUT << "Figure with duplicate label ``" << figlabel << "'' linked to file ``" << figfile << "'' omitted\n";
	} else { // include figure
//*** perhaps use a template here
//*** intelligently choose type of figure command
		ostrtext += "\n\\rrfigurehere";
		if (!psconversions.empty()) {
			if (figps!=NULL) { // add to FIGURE PSCONVERSIONS section is present
				String figpslabel(figlabel);
				figpslabel.del("fig:",0); figpslabel.gsub(nonalpharx,""); figpslabel.prepend("\\figps");
				if (psconversions.lastchar()=='\n') psconversions.del((int) psconversions.length()-1,1);
				(*figps) += "\\newcommand{" + figpslabel + "}{" + psconversions + "}\n";
				ostrtext += '{'+figpslabel+'}';
			} else ostrtext += '{'+psconversions+'}';
		}
		ostrtext += '{' + figlabel + "}{" + relurl(poutline,figfile) + "}{";
		if (figcaps!=NULL) { // add to FIGURE CAPTIONS section is present
			String figcapslabel(figlabel);
			figcapslabel.del("fig:",0); figcapslabel.gsub(nonalpharx,""); figcapslabel.prepend("\\figcap");
			if (figcaption.lastchar()=='\n') figcaption.del((int) figcaption.length()-1,1);
			(*figcaps) += "\\newcommand{" + figcapslabel + "}{" + figcaption + "}\n";
			ostrtext += figcapslabel;
		} else ostrtext += figcaption;
		ostrtext += "}{4.2in}{0}\n"+figintext;
	}
}

void extract_recursively(String & ostrtext, String * figcaps, String * figps, Novelty_Marker & nm, String & sectitle, StringList & srcf, StringList & srcfname, int & srcfnum, int depth, Novelty_Marker_List & visited, String & poutline, String & pprevtext) {
#ifdef DEBUG_PPEXTRACT
	cout << '(' << nm.id << ')'; cout.flush();
#endif
	// keep track of places already visited to avoid infinite loops
	if (visited.iselement(nm)>=0) {
		EOUT << "dil2al: Avoiding repetition of " << nm.source << '#' << nm.id << " in extract_recursively(), continuing as is\n";
		ostrtext += "% @WARNING: Repeated recursion into " + nm.source + '#' + nm.id + "avoided@\n";
		return;
	}
#ifdef DEBUG_PPEXTRACT
	cout << 'A'; cout.flush();
#endif
	int visitedidx = visited.length();
	if (nm.nmtype==Novelty_Marker::NM_NOVELTY_ITEM) visitedidx--; // use first list element
	visited[visitedidx].source = nm.source;
	visited[visitedidx].id = nm.id;
	visited[visitedidx].nmtype = nm.nmtype;
	// keep track of depth
	depth++;
	if (depth>MAX_RECURSIVE_PP2PD_EXTRACTION_DEPTH) {
		EOUT << "dil2al: Maximum extraction recursion depth (" << MAX_RECURSIVE_PP2PD_EXTRACTION_DEPTH << ")exceeded in extract_recursively(), continuing as is\n";
		ostrtext = ostrtext + "% @WARNING: Maximum extraction recursion depth (" + String((long) MAX_RECURSIVE_PP2PD_EXTRACTION_DEPTH) + ")exceeded@\n";
		return;
	}
#ifdef DEBUG_PPEXTRACT
	cout << 'B'; cout.flush();
#endif
	String srcref = relurl(poutline,nm.source)+'#'+nm.id;
	// test if REWRITTEN
	int previdx = -1;
	if ((nm.nmtype==Novelty_Marker::NM_NOVELTY_ITEM)
	 && ((previdx = pprevtext.index(BigRegex("\n[ 	]*%[ 	]*@Begin[ 	]+[^@]*REWRITTEN[^@]*[ 	]+EXTRACTED:[ 	]*[[]"+RX_Search_Safe(sectitle)+"[]][ 	]*"+RX_Search_Safe(srcref)+"[ 	]*@")))>=0)) {
		int prevend;
		if ((prevend = pprevtext.index(BigRegex("\n[ 	]*%[^@\n]*@End[ 	]+[^@]*REWRITTEN[^@]*[ 	]+EXTRACTED:[ 	]*[[]"+RX_Search_Safe(sectitle)+"[]][ 	]*"+RX_Search_Safe(srcref)+"[ 	]*@"),String::SEARCH_END,previdx))>=0) {
			ostrtext += pprevtext.at(previdx,prevend-previdx);
			if (verbose) { VOUT << "+R"; VOUT.flush(); }
			if ((!askextractconcat) || confirmation("Append newly extracted version of EXTRACTED ``"+srcref+"''? (y/N) ",'y')) return;
			srcref += " (newly extracted version)";
		} else EOUT << "dil2al: Missing @End REWRITTEN EXTRACTED: [section] source-reference@ in extract_recursively(), extracting from source\n";
	}
	// load source document if not already loaded
	int srcidx;
	if ((srcidx = get_file_in_list(nm.source,srcf,srcfname,srcfnum))<0) {
		EOUT << "continuing as is\n";
		ostrtext += "% @WARNING: Recursion was unable to load source document " + nm.source + "@\n";
		return;
	}
#ifdef DEBUG_PPEXTRACT
	cout << 'C'; cout.flush();
#endif
	// include content in outline
	if (nm.nmtype==Novelty_Marker::NM_NOVELTY_ITEM) {
		ostrtext += "\n% @Begin EXTRACTED: [" + sectitle + "] " + srcref + "@\n% Context: ";
		ostrtext += nm.context.concatenate(", ") + "\n% Importance: ";
		char fc[10];
		if (nm.imp) {
			sprintf(fc,"%4.2f",nm.imp[0]);
			ostrtext += fc;
			for (int j=1; j<nm.impnum; j++) {
				sprintf(fc,"%4.2f",nm.imp[j]);
				ostrtext += ", "; ostrtext += fc;
			}
		}
		sprintf(fc,"%4.2f",nm.len);
		ostrtext = ostrtext + "\n% Intended length: " + fc + " pages -->\n";
	} else ostrtext += "\n% @Begin CONTENT HREF: (depth " + (String((long) depth) + (") " + srcref + "@\n"));
	int nidx;
	if ((nidx = srcf[srcidx].index(BigRegex("[<]A[ 	]+[Nn][Aa][Mm][Ee][ 	]*=[ 	]*\""+RX_Search_Safe(nm.id)+'"')))>=0) {
#ifdef DEBUG_PPEXTRACT
		cout << 'D'; cout.flush();
#endif
		// content range detection (independent of file type)
//*** add remaining rules for content ranges
		String itemcontent;
		if (!item_content_between_range_tags(nm,nidx,srcf[srcidx],itemcontent))
		 if (!item_content_within_TL_chunk(nidx,srcf[srcidx],itemcontent))
		  if (!item_content_within_DIL_entry(nidx,srcf[srcidx],itemcontent))
		   item_content_within_paragraph(nidx,srcf[srcidx],itemcontent);
		if (nm.source.contains(BigRegex("[.][Tt][Ee][Xx]$"))) {
			// TeX files require no conversion but can contain \input{} references
			int inputidx = -1;
			while ((inputidx = itemcontent.index("\\input{",inputidx+1))>=0) {
				String inputref = itemcontent.at(BigRegex("[^ }]*"),inputidx+7);
				inputref = absurl(nm.source,inputref);
				inputref = relurl(poutline,inputref);
				String afterinput = itemcontent.from("}",inputidx+7);
				itemcontent = itemcontent.before(inputidx+7) + inputref + afterinput;
			}
		} else convert_item_content_to_TeX(itemcontent);
		// locate content hyperlink references
		int istart=0, iend=0; Novelty_Marker nmhref;
		// note that itemcontent.length() loses some value range here
		// due to conversion to signed int, a conversion that is done
		// since iend must be signed to contain valid output of the
		// itemcontent.index() search below
		while (iend<(int) itemcontent.length()) {
#ifdef DEBUG_PPEXTRACT
			cout << 'E'; cout.flush();
#endif
			if ((iend = itemcontent.index("@HRef::",istart))<0) {
				iend = itemcontent.length();
				ostrtext += itemcontent.at(istart,iend-istart) + '\n';
			} else {
				ostrtext += itemcontent.at(istart,iend-istart); //*** could add a `\n` here
				// recurse content extraction for @HREF: <document>@ codes
//*** perhaps recursively extracted content should be placed
//*** after the following punctuation mark
				nmhref.source = itemcontent.at(BigRegex("[^@]+"),iend+7);
				// convert HRef back to non-TeX format
				unconvert_item_content_from_TeX(nmhref.source);
				nmhref.id = nmhref.source.after('#');
				if (nmhref.id!="") nmhref.source = nmhref.source.before('#');
				nmhref.source = absurl(srcfname[srcidx],nmhref.source);
				// recognize figures
				if (nmhref.source.contains(BigRegex("[.]\\([Ff][Ii][Gg]\\|[Ee]?[Pp][Ss]\\)$"))) {
					extraction_put_figure(ostrtext,figcaps,figps,nmhref.source,srcf,srcfname,srcfnum,poutline);
				} else {
					nmhref.nmtype = Novelty_Marker::NM_CONTENT_HREF;
					extract_recursively(ostrtext,figcaps,figps,nmhref,sectitle,srcf,srcfname,srcfnum,depth,visited,poutline,pprevtext);
				}
				if ((istart = itemcontent.index("@",iend+1))<0) istart = iend;
				else istart++;
// *** There are some references without ID
			}
		}
	} else {
		EOUT << "dil2al: " << nm.source << '#' << nm.id << " not found in extract_paper_plan_to_paper(), continuing as is\n";
		ostrtext += "% (Item ID " + nm.id + " not found in content source file " + relurl(poutline,nm.source) + ") \n";
	}
	if (nm.nmtype==Novelty_Marker::NM_NOVELTY_ITEM) ostrtext += "% <!-- @End EXTRACTED: [" + sectitle + "] " + srcref + "@\n\n";
	else ostrtext += "% @End CONTENT HREF: " + srcref + "@\n";
#ifdef DEBUG_PPEXTRACT
	cout << 'F'; cout.flush();
#endif
}

void extract_novelty_item_to_paper(String & ostrtext, String * figcaps, String * figps, Novelty_Marker & nm, String & sectitle, StringList & srcf, StringList & srcfname, int & srcfnum, String & poutline, String & pprevtext) {
#ifdef DEBUG_PPEXTRACT
	cout << 'G'; cout.flush();
#endif
	if (nm.source=="") {
		EOUT << "dil2al: Missing Novelty item source reference for item with text ``" << nm.marktext << "'' in extract_paper_plan_to_paper(), continuing as is\n";
		return;
	}
#ifdef DEBUG_PPEXTRACT
	cout << 'H'; cout.flush();
#endif
	Novelty_Marker_List visited;
	extract_recursively(ostrtext,figcaps,figps,nm,sectitle,srcf,srcfname,srcfnum,0,visited,poutline,pprevtext);
}

void include_article_head_template(String & pout, String & ostrtext, String & pprevioustext) {
// detects @Begin REWRITTEN ARTICLE HEAD: specs@
	String templatetext;
	if (!read_file_into_String(articleheadtemplate,templatetext)) {
		EOUT << "dil2al: Unable to read " << articleheadtemplate << " in include_article_head_template(), continuing as is\n";
		return;
	}
	int previdx;
	if ((previdx = pprevioustext.index(BigRegex("%[ 	]*@Begin[ 	]*[^@]*REWRITTEN[^@]*ARTICLE HEAD:[^@]*@")))>=0) {
		// copy rewritten from previous
		int prevend;
		if ((prevend = pprevioustext.index(BigRegex("\n[ 	]*%[ 	]*@End ARTICLE HEAD[^@]*@"),String::SEARCH_END,previdx))>=0) {
			ostrtext += pprevioustext.at(previdx,prevend-previdx);
			//*** if (askextractconcat) doesn't make a lot of sense here
			return;
		} else EOUT << "dil2al: Missing @End ARTICLE HEAD@ in include_article_head_template(), including from template\n";
	}
	// substitute tentative information for known place holders
	String texname(pout.after('/',-1)), texdir(pout.before('/',-1));
	if (texname.length()==0) texname = pout;
	if (texname.index('.',-1)>0) texname = texname.before('.',-1); // > instead of >= for file names that begin with `.'
	if (texdir.length()==0) texdir = "/";
	else if (texdir.index('/',-1)>=0) texdir = texdir.after('/',-1);
	if (verbose) VOUT << "Assuming that " << texname << " is main document and " << texdir << " is its directory\n";
	templatetext.gsub(" filename.tex",' '+texname+".tex");
	templatetext.gsub("article-tex-directory",texdir);
	templatetext.gsub("main-doc.",texname+'.');
	templatetext.gsub("@ARTICLE START DATE: date@","@ARTICLE START DATE: "+curtime+'@');
	// include from template
	ostrtext += templatetext;
}

void include_article_tail_template(String & pout, String & ostrtext, String & pprevioustext) {
// detects @Begin REWRITTEN ARTICLE TAIL: specs@
	String templatetext;
	if (!read_file_into_String(articletailtemplate,templatetext)) {
		EOUT << "dil2al: Unable to read " << articletailtemplate << " in include_article_tail_template(), continuing as is\n";
		return;
	}
	int previdx;
	if ((previdx = pprevioustext.index(BigRegex("\n[ 	]*%[ 	]*@Begin[ 	]*[^@]*REWRITTEN[^@]*ARTICLE TAIL[^@]*@")))>=0) {
		// copy rewritten from previous
		int prevend;
		if ((prevend = pprevioustext.index(BigRegex("\n[ 	]*%[ 	]*@End ARTICLE TAIL[^@]*@"),String::SEARCH_END,previdx))>=0) {
			ostrtext += pprevioustext.at(previdx,prevend-previdx);
				//*** if (askextractconcat) doesn't make a lot of sense here
			return;
		} else EOUT << "dil2al: Missing @End ARTICLE TAIL@ in include_article_tail_template(), including from template\n";
	}
	// substitute tentative information for known place holders
	String texname(pout.after('/',-1)), texdir(pout.before('/',-1));
	templatetext.gsub("article-tex-directory",texdir);
	templatetext.gsub("main-doc.",texname+'.');
	texname = time_stamp("%c"); texname.gsub("\n","");
	templatetext.gsub("article-creation-date",texname);
	// include from template
	ostrtext += templatetext;
}

void include_article_section_head_template(String & templatefname, String & templatetext, String & ostrtext, String & pprevioustext) {
// detects @Begin REWRITTEN SECTION: SECTION-TITLE@
	if (!read_file_into_String(templatefname,templatetext)) {
		EOUT << "dil2al: Unable to read " << templatefname << " in include_article_section_head_template(), continuing as is\n";
		return;
	}
	String sectitle = templatetext.at(BigRegex("%[ 	]*@Begin SECTION:[ 	]*[^@]+@"));
	sectitle = sectitle.after(BigRegex(":[ 	]*")); sectitle = sectitle.before('@');
	int previdx;
	if ((previdx = pprevioustext.index(BigRegex("\n[ 	]*%[ 	]*@Begin[ 	]*[^@]*REWRITTEN[^@]*SECTION:[ 	]*"+RX_Search_Safe(sectitle)+"[ 	]*@")))>=0) {
		// copy rewritten from previous
		int prevend, prevheadend;
		if ((prevend = pprevioustext.index(BigRegex("\n[ 	]*%[ 	]*@End SECTION:[ 	]*"+RX_Search_Safe(sectitle)+"[ 	]*@"),String::SEARCH_END,previdx))>=0) {
			if (((prevheadend = pprevioustext.index(BigRegex("\n[ 	]*%[ 	]*@Section HEADEND[^@]*@"),String::SEARCH_END,previdx))>=0)  && (prevheadend<prevend)) {
				ostrtext += pprevioustext.at(previdx,prevheadend-previdx);
				//*** if (askextractconcat) doesn't make a lot of sense here
				return;
			} else EOUT << "dil2al: Missing @Section HEADEND@ in include_article_section_head_template(), including from template\n";
		} else EOUT << "dil2al: Missing @End SECTION " << sectitle << "@ in include_article_section_head_template(), including from template\n";
	}
	// include from template
	ostrtext += templatetext.through(BigRegex("\n[ 	]*%[ 	]*@Section HEADEND[^@]*@"));
}

void include_article_section_tail_template(String & templatetext, String & ostrtext, String & pprevioustext) {
// detects @Begin REWRITTEN SECTION: SECTION-TITLE@
	String sectitle = templatetext.at(BigRegex("%[ 	]*@Begin SECTION:[ 	]*[^@]+@"));
	sectitle = sectitle.after(BigRegex(":[ 	]*")); sectitle = sectitle.before('@');
	int previdx;
	if ((previdx = pprevioustext.index(BigRegex("\n[ 	]*%[ 	]*@Begin[ 	]*[^@]*REWRITTEN[^@]*SECTION:[ 	]*"+RX_Search_Safe(sectitle)+"[ 	]*@")))>=0) {
		// copy rewritten from previous
		int prevend;
		if ((prevend = pprevioustext.index(BigRegex("\n[ 	]*%[ 	]*@End SECTION:[ 	]*"+RX_Search_Safe(sectitle)+"[ 	]*@"),String::SEARCH_END,previdx))>=0) {
			if (((previdx = pprevioustext.index(BigRegex("\n[ 	]*%[ 	]*@Section TAILBEGIN[^@]*@"),previdx))>=0)  && (previdx<prevend)) {
				ostrtext += pprevioustext.at(previdx,prevend-previdx);
				//*** if (askextractconcat) doesn't make a lot of sense here
				return;
			} else EOUT << "dil2al: Missing @Section TAILBEGIN@ in include_article_section_tail_template(), including from template\n";
		} else EOUT << "dil2al: Missing @End SECTION " << sectitle << "@ in include_article_section_tail_template(), including from template\n";
	}
	// include from template
	ostrtext += templatetext.from(BigRegex("\n[ 	]*%[ 	]*@Section TAILBEGIN[^@]*@"));
}

void add_protected_text(int ostridx, int protidx, int protend, String & ostrtext, String & pprevioustext) {
	String ostrtail;
	if (ostridx<0) ostridx = ostrtext.index(BigRegex("\n[  ]*%[ 	]*@Begin ARTICLE TAIL[^@]*@"),-1);
	if (ostridx<0) {
		ostridx = ostrtext.length();
		ostrtail = "";
	} else {
		ostrtail = ostrtext.from(ostridx);
		ostrtext = ostrtext.before(ostridx);
	}
	ostrtext += pprevioustext.at(protidx,protend-protidx) + ostrtail;
}

void include_protected_text(String & ostrtext, String & pprevioustext) {
// @Begin PROTECTED: [Section,[before,after,topic]]@, @End PROTECTED@
	int protidx, protend = 0;
	while ((protidx = pprevioustext.index(BigRegex("\n[ 	]*%[ 	]*@Begin PROTECTED[^@]*@"),protend))>=0) {
		if ((protend = pprevioustext.index(BigRegex("\n[ 	]*%[ 	]*@End PROTECTED[^@]*@"),String::SEARCH_END,protidx))<0) {
			EOUT << "dil2al: @End PROTECTED@ missing in include_protected_text(), continuing as is\n";
			protend = protidx+1;
		} else {
			String secid = pprevioustext.at(BigRegex("\n[ 	]*%[ 	]*@Begin PROTECTED[^@]*@"),protidx);
			secid = secid.after("@Begin PROTECTED:");
			secid = secid.before("@");
			String secloc = secid.after(",");
			if (secloc!="") secid = secid.before(",");
			secloc.del(BigRegex("[ 	]$")); secloc.del(BigRegex("^[ 	]"));
			secid.del(BigRegex("[ 	]$")); secid.del(BigRegex("^[ 	]"));
			secid.upcase();
			int ostridx = -1; String ostrtail;
			if (secid!="") {
				if (secloc=="before") { // before section
					if ((ostridx = ostrtext.index(BigRegex("\n[ 	]*%[ 	]*@Begin SECTION:[ 	]*"+RX_Search_Safe(secid)+"[ 	]*@")))<0) {
						EOUT << "dil2al: Unable to find section " << secid << " in include_protected_text(), adding to end of document\n";
					}
					add_protected_text(ostridx,protidx,protend,ostrtext,pprevioustext);
				} else if (secloc=="after") { // after section
					if ((ostridx = ostrtext.index(BigRegex("\n[ 	]*%[ 	]*@End SECTION:[ 	]*"+RX_Search_Safe(secid)+"[ 	]*@"),String::SEARCH_END))<0) {
						EOUT << "dil2al: Unable to find section " << secid << " in include_protected_text(), adding to end of document\n";
					}
					add_protected_text(ostridx,protidx,protend,ostrtext,pprevioustext);
				} else { // in section
					if ((secloc=="") || ((ostridx = ostrtext.index(BigRegex("\n[ 	]*%[ 	]*@Begin SUBTOPIC:[ 	]*[[]depth[^,]*,[ 	]*"+RX_Search_Safe(secid)+"[^]]*[]][ 	]*"+RX_Search_Safe(secloc)+"[ 	]*@")))<0)) {
						if ((ostridx = ostrtext.index(BigRegex("\n[ 	]*%[ 	]*@Begin SECTION:[ 	]*"+RX_Search_Safe(secid)+"[ 	]*@")))>=0) {
							if ((ostridx = ostrtext.index(BigRegex("\n[ 	]*%[ 	]*@Section HEADEND[^@]*@"),String::SEARCH_END,ostridx))<0) {
								EOUT << "dil2al: Unable to find section (" << secid << ") head end in include_protected_text(), adding to end of document\n";
							}
						} else {
							EOUT << "dil2al: Unable to find section " << secid << " in include_protected_text(), adding to end of document\n";
						}
					} else {
						int subsecend,subheadend;
						if ((subsecend = ostrtext.index(BigRegex("\n[ 	]*%[ 	]*@End SUBTOPIC:[ 	]*[[]depth[^,]*,[ 	]*"+RX_Search_Safe(secid)+"[^]]*[]][ 	]*"+RX_Search_Safe(secloc)+"[ 	]*@"),ostridx))<0) {
							EOUT << "dil2al: Missing @End SUBTOPIC@ in include_protected_text(), adding to end of document\n";
						} else {
							if (((subheadend = ostrtext.index(BigRegex("\n[ 	]*%[ 	]*@[^@]*SubSection HEADEND[^@]*@"),String::SEARCH_END,ostridx))>=0) && (subheadend<subsecend)) {
								ostridx = subheadend;
							}
						}
					}
					add_protected_text(ostridx,protidx,protend,ostrtext,pprevioustext);
				}
			} else { // add protected text to end of document
				add_protected_text(-1,protidx,protend,ostrtext,pprevioustext);
			}
		}
	}
}

bool extract_paper_plan_to_paper(String ppname, String pout, String pprevious) {
// uses information in a paper plan to generate paper outline and content
// (see DIL#20000315155709.1)
// preparation:
// - a sorted paper plan including required items
// - an optional previous (version of the) paper, containing
//   stylistically rewritten items or arbitrary ``protected'' text
//   for inclusion in the paper draft
  const BigRegex fighererx("[\\][A-Za-z]*figurehere[{]");
  String pptext, pprevtext, ostrtext, _figcaps, _figps, * figcaps = NULL, * figps = NULL;
  if (!read_file_into_String(ppname,pptext)) return false;
  if (pprevious!="") if (!read_file_into_String(pprevious,pprevtext)) return false; // safe in case new paper draft would overwrite pprevious
  ofstream ostr(pout+".new");
  if (!ostr) {
    EOUT << "dil2al: Unable to create " << pout << ".new in extract_paper_plan_to_paper()\n";
    return false;
  }
  include_article_head_template(pout,ostrtext,pprevtext);
  // determine if FIGURE CAPTIONS and FIGURE PSCONVERSIONS sections are present
  if (ostrtext.contains("@Begin FIGURE CAPTIONS@")) figcaps = &_figcaps;
  if (ostrtext.contains("@Begin FIGURE PSCONVERSIONS@")) figps = &_figps;
  // read all Novelty items from paper plan into list
  Novelty_Marker_List nml; int nmend = 0, nmnum = 0, hdepth = 0;
  if ((nmend = pptext.index(BigRegex("[<]A[ 	]*[Nn][Aa][Mm][Ee][ 	]*=[ 	]*\"tch\""),nmend))<0) {
    EOUT << "dil2al: No Topical Context Hierarchy found in Paper Plan in extract_paper_plan_to_paper()\n";
    return false;
  }
#ifdef DEBUG_PPEXTRACT
  cout << "Paper Plan text length = " << pptext.length() << " Novelty Markers at positions: ";
#endif
  while ((nmend = nml[nmnum].Get_Novelty_from_Paper_Plan(pptext,nmend,hdepth))>=0) {
#ifdef DEBUG_PPEXTRACT
    if (nmnum>0) cout << ',';
    cout << nmend;
#endif
    hdepth = nml[nmnum].hierdepth; // track Topical Context Hierarchy depth
    if (hdepth<0) break;
    if (nml[nmnum].source!="") nml[nmnum].source = absurl(ppname,nml[nmnum].source);
#ifdef DEBUG_PPEXTRACT
    if (nml[nmnum].source!="") cout << '(' << nml[nmnum].source << ')';
    else cout << '(' << nml[nmnum].context[0] << ')';
#endif
    nmnum++;
  }
#ifdef DEBUG_PPEXTRACT
  cout << '\n';
#endif
  StringList contextconcats; LongList ccranks;
  nml.Get_Context_Ranks(contextconcats,ccranks);
  if (verbose) VOUT << nmnum << " Novelty Items, Topic Openings and Closings\n";
  // read Section IDs
  StringList ppsectionids, ppsectiontitles, ppsectiontemplates;
  if (!get_Paper_Plans_Section_IDs(ppsectionids,ppsectiontitles,ppsectiontemplates)) return false;
  // test for unknown section IDs in Novelty Items
  for (int i = 0; i<nmnum; i++) if (nml[i].nmtype==Novelty_Marker::NM_NOVELTY_ITEM)
    for (unsigned int s = 0; s<nml[i].section.length(); s++) if (ppsectionids.iselement(nml[i].section[s])<0)
      EOUT << "dil2al: Unknown section type `" << nml[i].section[s] << "' for " << nml[i].id << " in extract_paper_plan_to_paper(), continuing as is\n";
  //*** generate tentative Abstract, as specified in
  //*** DIL#20000315155709.1
  //*** use information about paper type to modify main template or section templates
  //*** according to background template
  // per section
  unsigned int secidx;
  int srcfnum = 0;
  StringList srcf, srcfname; String requireditems,sectiontemplate;
  for (secidx=0; secidx<ppsectionids.length(); secidx++) if (ppsectionids[secidx]!="ABS") {
#ifdef DEBUG_PPEXTRACT
    cout << 'I'; cout.flush();
#endif
    // get section title
    String sectitle = ppsectiontitles[secidx];
    sectitle.upcase();
    if (verbose) VOUT << "Generating paper outline section " << sectitle << '\n';
    include_article_section_head_template(ppsectiontemplates[secidx],sectiontemplate,ostrtext,pprevtext);
    // parse items in section
    String refurl = relurl(pout,ppname);
    for (int i=0; i<nmnum; i++)
#ifdef DEBUG_PPEXTRACT
      { cout << 'J'; cout.flush();
#endif
      if ((nml[i].nmtype==Novelty_Marker::NM_TCH_DEPTH_INC)
	  || (nml[i].nmtype==Novelty_Marker::NM_TCH_DEPTH_DEC)
	  || (nml[i].section.iselement(ppsectionids[secidx])>=0)) {
	if (nml[i].nmtype==Novelty_Marker::NM_REQUIRED_ITEM) extract_required_item_to_paper(ostrtext,refurl,nml[i],sectitle,requireditems,pprevtext);
	else if (nml[i].nmtype==Novelty_Marker::NM_TCH_DEPTH_INC) extract_hierarchy_depth_increase_to_paper(ostrtext,nml[i],contextconcats,ccranks,sectitle,srcf,srcfname,srcfnum,pprevtext);
	else if (nml[i].nmtype==Novelty_Marker::NM_TCH_DEPTH_DEC) extract_hierarchy_depth_decrease_to_paper(ostrtext,nml[i],sectitle,srcf,srcfname,srcfnum,pprevtext);
	else if (nml[i].nmtype==Novelty_Marker::NM_NOVELTY_ITEM) extract_novelty_item_to_paper(ostrtext,figcaps,figps,nml[i],sectitle,srcf,srcfname,srcfnum,pout,pprevtext);
	else EOUT << "dil2al: Unknown Novelty_Marker::nmtype in extract_paper_plan_to_paper(), continuing as is\n";
      }
      // close section
#ifdef DEBUG_PPEXTRACT
      } cout << 'L'; cout.flush();
#endif
      include_article_section_tail_template(sectiontemplate,ostrtext,pprevtext);
  }
#ifdef DEBUG_PPEXTRACT
  cout << 'K'; cout.flush();
#endif
  include_article_tail_template(pout,ostrtext,pprevtext);
  // add FIGURE CAPTIONS and FIGURE PSCONVERSIONS sections if present
  if (figcaps!=NULL) ostrtext.gsub(BigRegex("@Begin FIGURE CAPTIONS@[^\n]*\n"),"@Begin FIGURE CAPTIONS@\n"+_figcaps);
  if (figps!=NULL) ostrtext.gsub(BigRegex("@Begin FIGURE PSCONVERSIONS@[^\n]*\n"),"@Begin FIGURE PSCONVERSIONS@\n"+_figps);
  // add \figureloose commands
  int ofigidx;
  if ((ofigidx = ostrtext.index(fighererx))>=0) {
    // find loose figures location in outline and copy to there
    int iloosefigidx, ifigend = -1; String ostrtail;
    if (((iloosefigidx = ostrtext.index(BigRegex("\n%[ 	]*@Begin FIGURES AT END@"),-1))>=0)
	&& ((ifigend = ostrtext.index(BigRegex("\n%[ 	]*@End FIGURES AT END@[^\n]*\n"),String::SEARCH_END,-1))>=0)) {
      ostrtail = ostrtext.from(ifigend);
      ostrtext = ostrtext.before(iloosefigidx);
    } else {
      if ((iloosefigidx = ostrtext.index("\\end{document}",-1))>=0) {
	ostrtail = ostrtext.from(iloosefigidx);
	ostrtext = ostrtext.before(iloosefigidx);
      } else {
	EOUT << "dil2al: No \\end{document} found in outline in extract_paper_plan_to_paper(), continuing as is\n";
	ostrtail = "";
      }
    }
    // copy \figurehere data to \figureloose commands
    ostrtext += "\n% @Begin FIGURES AT END@\n\n";
    while (ofigidx>=0) {
      String figdata;
      if ((ifigend=TeX_get_command(ostrtext,ofigidx,figdata))>=0) {
	figdata.gsub(BigRegex("^\\([\\][A-Za-z]*\\)figurehere[{]"),"_1figureloose{",'_');
	ostrtext += figdata + "\n\n";
      } else {
	EOUT << "dil2al: Incomplete figure command found for " << ostrtext.sub(fighererx,0) << " in extract_paper_plan_to_paper(), continuing as is\n";
	ifigend = ofigidx+1;
      }
      ofigidx = ostrtext.index(fighererx,ifigend);
    }
    ostrtext += "% @End FIGURES AT END@\n"+ostrtail;
  }
  // include protected text
  include_protected_text(ostrtext,pprevtext);
  // TeX format corrections, safeguards for TeX compilation
  TeX_format_corrections(ostrtext,true);
  //*** add new Context items to contexts file here (DIL#20010412121145.1)
  cerr << "Add automatic call to add Context items to contexts file here (DIL#20010412121145.1)...\n";
  ostr << ostrtext; // write paper draft file
  ostr.close();
#ifndef DEBUG_DO_NOT_CREATE_NEW_OUTLINE
  if (!backup_and_rename(pout,"Paper Draft")) return false;
  // find required items list in paper plan, create if not found
  if (requireditems.length()>0) {
    int riidx;
    if ((riidx = pptext.index(BigRegex("[<]!--[ 	]*@Begin REQUIRED ITEMS@[ 	]*--[>]\n"),String::SEARCH_END))<0) {
      // create list of required items
      if ((riidx = pptext.index(BigRegex("\\([<][Pp][>][ 	]*\n[<][Hh][Rr][>].*\\)?[<]/[Bb][Oo][Dd][Yy][>]")))<0)
	EOUT << "dil2al: Unable to create list of required items in extract_paper_plan_to_paper(), continuing as is\n";
      else {
	String pptextrest = pptext.at(riidx,pptext.length()-riidx);
	pptext = pptext.before(riidx)
	  + "<H2><A HNAME=\"required-items\">Required Items</A></H2>\n\n<UL>\n<!-- @Begin REQUIRED ITEMS@ -->\n<!-- @End REQUIRED ITEMS@ -->\n</UL>\n\n"
	  + pptextrest;
	riidx += 92; // move index to start of <!-- @End REQUIRED ITEMS@ --> tag
      }
    }
    if (riidx>=0) {
      // update required items list
      int riend;
      if ((riend = pptext.index(BigRegex("[<]!--[ 	]*@End REQUIRED ITEMS@[ 	]*--[>]"),riidx))<0) {
	EOUT << "dil2al: Missing @End REQUIRED ITEMS@ tag in extract_paper_plan_to_paper(), placing tag\n";
	String pptextrest = pptext.at(riidx,pptext.length()-riidx);
	pptext = pptext.before(riidx) + "<!-- @End REQUIRED ITEMS@ -->\n" + pptextrest;
	riend = riidx;
      }
      String pptextrest = pptext.at(riend,pptext.length()-riend);
      pptext = pptext.before(riidx) + requireditems + pptextrest;
      // store, backup and rename updated paper plan
      ostr.open(ppname+".new");
      if (!ostr) {
	EOUT << "dil2al: Unable to create " << ppname << ".new to update required items list in extract_paper_plan_to_paper(), continuing as is\n";
	return false;
      }
      ostr << pptext;
      if (!backup_and_rename(ppname,"Paper Plan"))
	EOUT << "dil2al: Unable to update required items list extract_paper_plan_to_paper(), continuing as is\n";
    }
  } else if (verbose) VOUT << "No Required items to list\n";
  return true;
#endif
}

bool cmdline_extract_paper_plan_to_paper(String ppfile) {
// Commandline interface to extract_paper_plan_to_paper()
	String ppname, poutline, pprevious;
	ppname = ppfile.before("+");
	if (ppname == "") {
		ppname = ppfile;
		EOUT << "dil2al: Paper extraction without specific paper outline not yet supported\n";
		return false;
	} else {
		poutline = ppfile.after("+");
		if (poutline == "") {
			EOUT << "dil2al: Paper extraction without specific paper outline not yet supported\n";
			return false;
		} else {
			pprevious = poutline.after("+");
			if (pprevious != "") {
				poutline = poutline.before("+");
				if (pprevious[0]!='/') pprevious.prepend(homedir);
			}
			if (ppfile[0]!='/') ppfile.prepend(homedir);
			if (poutline[0]!='/') poutline.prepend(homedir);
			return extract_paper_plan_to_paper(ppname,poutline,pprevious);
		}
	}
}

bool Assess_Paper_Correction_Time(String ppfile) {
// Obtain quantitative information about guidelines requiring
// checking to complete the correction of a paper.
	// *** this may be improved by having a class or structure
	//     that reads in lists of guidelines in an outline context
	//     and the text to which they apply
	String pptext;
	if (!read_file_into_String(ppfile,pptext)) return false;
	// prune document head and tail
	pptext.del(BigRegex(".*\n[^%]*[\\]begin[{]document[}]"),0);
	pptext.del(BigRegex("%[ 	]+@Begin[ 	]+ARTICLE TAIL@.*"));
	// prune comments
	String checklistid("CHL"); checklistid = RX_Search_Safe(generate_tag(checklistid).through(':'));
	checklistid.gsub(" ","[ 	]+");
	String checkmarkid("CHM"); checkmarkid = RX_Search_Safe(generate_tag(checkmarkid).through(':'));
	checkmarkid.gsub(" ","[ 	]+");
	pptext.gsub(BigRegex(checklistid),"@@@Begin CHECKLIST:");
	pptext.gsub(BigRegex(checkmarkid),"@@@Begin CHECKMARK:");
	checklistid.gsub("@Begin","@End");
	checkmarkid.gsub("@Begin","@End");
	pptext.gsub(BigRegex(checklistid),"@@@End CHECKLIST:");
	pptext.gsub(BigRegex(checkmarkid),"@@@End CHECKMARK:");
	pptext.gsub(BigRegex("\\([^\\]\\)%[^\n]*"),"_1",'_');
	int pploc = 0, ppnext, totwc = 0; String tagtext, contextstr, papercontext;
	while ((pploc=D2A_get_tag(pptext,pploc,"End CHECKLIST",tagtext))>=0) {
		if ((pploc=pptext.index('\n',pploc))>=0) { // from start of next line
			// to next tag or end of file
			pploc++;
			if ((ppnext=pptext.index("@@@Begin CHECKLIST:",pploc))>=0) {
				contextstr = pptext.at(pploc,ppnext-pploc);
				pploc = ppnext;
			} else contextstr = pptext.from(pploc);
			// count number of words
			int wc = BigRegex_freq(contextstr,BRXwhite);
			totwc += wc;
			if (tagtext.contains("Paper")) papercontext = tagtext;
			else VOUT << tagtext << ": " << wc << " words\n";
		}
	}
	if (totwc>0) VOUT << papercontext << ": " << totwc << " words\n";
	return true;
}

bool Quantitative_Assessment(char qatype, String ppfile) {
// Carry out quantitative assessments that can aid in tasks,
// e.g. obtaining statistics about a paper that help in the
// scheduling of DIL entries with appropriate time requirements.
  
  switch (qatype) {
  case 'c': // assess time required for paper corrections
    return Assess_Paper_Correction_Time(ppfile);
  default:
    ERROR << "Quantitative_Assessment(): Unknown assessment request `" << qatype << ".\n";
    return false;
  }
  return true;
}
