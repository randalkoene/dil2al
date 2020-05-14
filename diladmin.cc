// diladmin.cc
// Randal A. Koene, 20000304

#include "dil2al.hh"
#include <dirent.h>

// Debug definitions:

//#define DEBUGTIER1
//#define DEBUGTIER2
//#define DEBUG_HIERARCHY_EXPRESSIONS
//#define DEBUG_PLAN_ENTRY
//#define DEBUG_PERIODIC_TASKS

// External variables:

const char periodictaskchar[pt_nonperiodic+1] = {'d','D','w','b','m','M','y','s','\0'};
const char periodictaskstr[pt_nonperiodic+1][16] = {
  "daily",
  "workdays",
  "weekly",
  "bi-weekly",
  "monthly",
  "eomoffset",
  "yearly",
  "OLD_span",
  "non-periodic"
};

// Local variables:

time_t _periodicnearesttdate = (MAXTIME_T);

// Topical_DIL_Files Class member files (see dil2al.hh)

void Topical_DIL_Files::Set_File(String file, int n) {
	if (!n) { ft.file = file; updated=true; return; }
	if (n>0) {
		if (Next()) Next()->Set_File(file,n-1);
		else {
			Topical_DIL_Files * tdf = new Topical_DIL_Files();
			if (Root()->link_before(tdf)) Next()->Set_File(file,n-1);
		}
	} else {
		if (Prev()) Prev()->Set_File(file,n+1);
		else {
			Topical_DIL_Files * tdf = new Topical_DIL_Files();
			if (Root()->link_after(tdf)) Prev()->Set_File(file,n+1);
		}
	}
}

void Topical_DIL_Files::Set_Title(String title, int n) {
	if (!n) { ft.title = title; updated=true; return; }
	if (n>0) {
		if (Next()) Next()->Set_Title(title,n-1);
		else {
			Topical_DIL_Files * tdf = new Topical_DIL_Files();
			if (Root()->link_before(tdf)) Next()->Set_Title(title,n-1);
		}
	} else {
		if (Prev()) Prev()->Set_Title(title,n+1);
		else {
			Topical_DIL_Files * tdf = new Topical_DIL_Files();
			if (Root()->link_after(tdf)) Prev()->Set_Title(title,n+1);
		}
	}
}

int Topical_DIL_Files::Refresh() {
  // Clear and reload Topical DIL files list
  // This may be called when Topical_DIL_Files_Updated_Access is used,
  // which happens during calls to Topical_DIL_Files:: File(), Title(),
  // Topic() and operator[].
	if (Prev()) return Prev()->Refresh(); // seek head of list
	delete Next(); // clear list
	ft.file=""; ft.title="";
	const int LLEN = 10240, SLEN = 1024;
	char lbuf[LLEN], sbuf[SLEN];
	ifstream lf(listfile);
	regex_t re;
	regmatch_t rm[3];
	int mres;
	numDILs = 0;
	if (lf) {
		// find DIL header
		if (!find_line(&lf,"<H1><A NAME=\"DIL\">",lbuf,LLEN)) return 0;
		// find <UL>
		if (!find_line(&lf,"<UL>",lbuf,LLEN)) return 0;
		// get DILs
		if (regcomp(&re,"<LI><A +HREF=\"([^\"]*)\">([^<]*)<", REG_EXTENDED) != 0) {
			EOUT << "dil2al: Unable to compile regular expression in get_topical_DILs()\n";
			return 0;
		}
		do {
			if (lf.eof()) {
				regfree(&re);
				return numDILs;
			}
			lf.getline(lbuf,LLEN);
			mres = regexec(&re,lbuf, (size_t) 3, rm, 0);
			if ((mres != 0) && (mres != REG_NOMATCH)) {
				EOUT << "dil2al: Internal error while matching in get_topical_DILs()\n";
				regfree(&re);
				return 0;
			}
			if (!mres) {
				if (rm[2].rm_so==-1) EOUT << "dil2al: Wrong number of substring matches in get_topical_DILs()\n";
				else {
					strncpy(sbuf,&lbuf[rm[1].rm_so],rm[1].rm_eo-rm[1].rm_so);
					sbuf[rm[1].rm_eo-rm[1].rm_so]='\0';
					Set_File(absurl(listfile,sbuf),numDILs);
					strncpy(sbuf,&lbuf[rm[2].rm_so],rm[2].rm_eo-rm[2].rm_so);
					sbuf[rm[2].rm_eo-rm[2].rm_so]='\0';
					Set_Title(sbuf,numDILs);
					numDILs++;
				}
			}
		} while (!pattern_in_line("</UL>",lbuf));
		regfree(&re);
		return numDILs;
	} else {
		EOUT << "dil2al: Error - " << listfile << " not found\n";
		return 0;
	}
}

bool DIL_Visualize_with_Tabs::Visualize_Element(DIL_entry * de, int depth, int width, DIL_entry * prede) {
	if (depth>0) s += replicate('\t',depth);
	s += "DIL#" + String(de->chars()) + '\n';
	content += "\nDIL#" + String(de->chars()) + ":\n";
	String * ctxt = de->Entry_Text();
	if (ctxt) {
		String ptxtcontent(*ctxt);
		content += (*HTML_remove_tags(ptxtcontent));
	}
	content += '\n';
	return true;
}

void DIL_Visualize_with_Tabs::Visualize_NotShown(int numdependencies, int depth, int width, DIL_entry * prede) {
  if (numdependencies>0) {
    if (depth>0) s += replicate('\t',depth);
    s += '['+String((long) numdependencies)+" dependencies...]\n";
  }
}

bool DIL_Visualize_with_HTML_Tabs::Visualize_Element(DIL_entry * de, int depth, int width, DIL_entry * prede) {
  String ptxtcontent;
  content += "\nDIL#" + String(de->chars()) + ":\n";
  String * ctxt = de->Entry_Text();
  if (ctxt) {
    ptxtcontent = (*ctxt);
    content += (*HTML_remove_tags(ptxtcontent));
    Elipsis_At(ptxtcontent,hierarchyexcerptlength);
    ptxtcontent.gsub('\n',' ');
  } else ptxtcontent = "<!-- excerpt requires content file option -->";
  content += '\n';
  if (depth>0) s += replicate('\t',depth);
  s += HTML_put_href((de->Topics(0)->dil.file+'#')+de->chars(),"DIL#" + String(de->chars())) + ": " + ptxtcontent + '\n';
  return true;
}

void DIL_Visualize_with_HTML_Tabs::Visualize_NotShown(int numdependencies, int depth, int width, DIL_entry * prede) {
  if (numdependencies>0) {
    if (depth>0) s += replicate('\t',depth);
    s += '['+String((long) numdependencies);
    if (reversehierarchy) s += " superiors...]\n";
    else s += " dependencies...]\n";
  }
}

bool DIL_Visualize_with_FORM_Tabs::Visualize_Plan_Entry(DIL_entry * de) {
  // returns false mostly just to indicate when a regular content editing
  // link should be added to s
  if ((!de) || (!Level_Data())) return false;
  if (hierarchyplanparse) { // additional parsing requested
    if (de->Is_Plan_Entry()==PLAN_ENTRY_TYPE_ACTION) {
#ifdef DEBUG_PLAN_ENTRY
      s += '[' + String((long) de->Is_Plan_Entry()) + ']';
#endif
      if (Level_Data()->Prev()) {
	DIL_entry * pe = Level_Data()->Prev()->Entry();
	if (pe->Is_Plan_Entry()==PLAN_ENTRY_TYPE_GOAL)
	  if (pe->content->Get_Plan_Entry_Content()->Decision()==(*de)) {
	    s += "<B>{!}</B>";
	  }
      }
    }
#ifdef DEBUG_PLAN_ENTRY
    else s += '{' + String((long) de->Is_Plan_Entry()) + '}';
#endif
  }
  if (Level_Data()->likelihood>DILSUPS_REL_UNSPECIFIED) {
    if (cumlikelihood==0.0) cumlikelihood = Level_Data()->likelihood;
    else cumlikelihood *= Level_Data()->likelihood;
    s += " (";
    if (de->content) {
      s += "<A HREF=\"file:///cgi-bin/dil2al?dil2al=MEI&DILID="+de->str()+"\">"; // open link to modification interface
      double desirability = de->content->PLAN_OUTCOME_DESIRABILITY;
      if (desirability>0.0) {
	s += "<B>" + String(desirability,"%3.1f") + "</B>,";
	if (Level_Data()->Prev()) Level_Data()->Prev()->benefit += (Level_Data()->likelihood*desirability);
      } else {
	s += String(desirability,"%3.1f") + ',';
	if (Level_Data()->Prev()) Level_Data()->Prev()->risk += (Level_Data()->likelihood*(-desirability));
      }
      s += "</A>"; //close link to modification interface
    } 
    s += String(Level_Data()->likelihood,"%4.2f") + '|' + String(cumlikelihood,"%5.3f") + ')';
  } else return false;
  return true;
}

bool DIL_Visualize_with_FORM_Tabs::Visualize_Element(DIL_entry * de, int depth, int width, DIL_entry * prede) {
  const char haslocalflag[2] = {'L','F'};
  const char specialnonlocalflag[1] = {'S'};
  String ptxtcontent;
  content += "\nDIL#" + String(de->chars()) + ":\n";
  String * ctxt = de->Entry_Text();
  if (ctxt) {
    ptxtcontent = (*ctxt);
    content += (*HTML_remove_tags(ptxtcontent));
    Elipsis_At(ptxtcontent,hierarchyexcerptlength);
    ptxtcontent.gsub('\n',' ');
  } else ptxtcontent = "<!-- excerpt requires content file option -->";
  content += '\n';
  if (depth>0) s += replicate('\t',depth);
  else cumlikelihood = 0.0;
  // differentiation of display depending on completion status
  bool pending = true;
  if (de->content) pending = ((de->content->completion>=0.0) && (de->content->completion<1.0));
  // show checkbox and DIL ID reference
  s += "<INPUT type=\"checkbox\" name=\"DILIDCHKBX"+String(de->chars())+"\" value=\"extract\">";
  if (pending) s += HTML_put_href((de->Topics(0)->dil.file+'#')+de->chars(),"<B>DIL#" + String(de->chars()) + "</B>");
  else s += HTML_put_href((de->Topics(0)->dil.file+'#')+de->chars(),"DIL#" + String(de->chars()));
  // optionally show local target date
  if (hierarchysorting) {
    bool isfromlocal; int haslocal, numpropagating;
    time_t tdate = de->Target_Date_Info(isfromlocal,haslocal,numpropagating);
    s += " TD=";
    if (isfromlocal) s += "<B>";
    if (tdate<(MAXTIME_T)) s += time_stamp("%Y%m%d%H%M,",tdate);
    else s += "unspecified,";
    if (isfromlocal) s += "</B>";
    if (haslocal>0) s += haslocalflag[haslocal-1];
    if (haslocal<0) {
      s += "<B>";
      s += specialnonlocalflag[2+haslocal];
      s += "</B>";
    }
    s += String((long) numpropagating);
  }
  // show level-specific data
  if (Level_Data()) { // visualization that depends on level-specific data
    if (!Visualize_Plan_Entry(de)) if (de->content) s += "[<A HREF=\"file:///cgi-bin/dil2al?dil2al=MEI&DILID="+de->str()+"\">edit</A>]";
  }
  // show DIL entry text content
  s += ": " + ptxtcontent + '\n';
  return true;
}

void DIL_Visualize_with_FORM_Tabs::Visualize_NotShown(int numdependencies, int depth, int width, DIL_entry * prede) {
  if (numdependencies>0) {
    if (depth>0) s += replicate('\t',depth);
    s += '['+String((long) numdependencies);
    if (reversehierarchy) s += " superiors...]\n";
    else s += " dependencies...]\n";
  }
  if (Level_Data())
    if (Level_Data()->has_benefit_risk()) {
      if (depth>0) s += replicate('\t',depth);
      s += "&lt;" + String(Level_Data()->benefit,"%3.1f") + '/' + String(Level_Data()->risk,"%3.1f") + "&gt;\n";
    }
}

void URL_sections(String & txt, int width) {
  // Use ')' characters to achieve a width restriction
  // of URL text displayed by acroread on mouse-over.
  int wcnt = 0, last_i = -1;
  for (int i = 0; i<((int) txt.length()); i++) {
    if (wcnt>=width) {
      if (i<=last_i) break;
      last_i = i;
      for (int j = i; j>0; j--) if (txt[j]=='_') {
	  txt[j] = ')';
	  wcnt = i-j;
	  break;
	}
    } else wcnt++;
  }
}

void OOP_Status::get(DIL_entry * de) {
  // Finds information such as the status of OOP detailing
  // See TL#200907050757.
  PLL_LOOP_FORWARD(DIL_entry,de->head(),1) if (e->Is_Direct_Dependency_Of(de)) {
    String * ctxt = e->Entry_Text();
    if (ctxt) { // detect detailing task
      if (ctxt->index("Detail further objectives")>=0) {
	has_detailing_task = true;
	identified_problems = (ctxt->index("[DONE] Identify the underlying problem")>=0);
	identified_solutions = (ctxt->index("[DONE] Then apply problem solving")>=0);
	OOP_hierarchy_set = (ctxt->index("[DONE] Create a hierarchy")>=0);
	break;
      }
    }
  }
}

bool DIL_Visualize_GRAPH::Visualize_Element(DIL_entry * de, int depth, int width, DIL_entry * prede) {
  // (**not yet used here**) const char haslocalflag[2] = {'L','F'};
  // (**not yet used here**) const char specialnonlocalflag[1] = {'S'};
  // 1) The first time 'de' is encountered, semaphore is 0: Then all is
  //    visualized and return values are true.
  //    When Show_DIL_Hierarchy() is the calling function, this leads to
  //    following connections from 'de'.
  // 2) The next times 'de' is encountered, semaphore is not 0: Then
  //    connections are visualized (or prepared for visualization), the
  //    node is not and return values are false.
  //    When Show_DIL_Hierarchy() is the calling function, this leads to
  //    no further exploration along the branch 'de' is on.
  // Note 1: In Graphviz (`dot`), the "color" parameter defines the color of
  // the box outline.
  // Note 2: The contentfile is not used in the call that w3minterface makes
  // to dil2al for the .dot visualized hierarchy. The contentfile is an
  // option to create a separate file with the full textual content of a
  // task for those ways of displaying the hierarchy that do not store that
  // data in the same file (e.g. tabbed visualization, form visualization).
  // The 'content' variable is only used for text intended for such a
  // contentfile.

  // 1.) Apply Filters
  // If we are running into a hierarchy search width restriction
  if (width>maxdotgraphwidth) return false; // avoid unconnected parts by processing deeper when not processing this wide (see TL#200907091823.3)
  bool first_visit = (de->Get_Semaphore()==0);
  de->Set_Semaphore(1); // Mark as visited // *** this was commented out!
  /* if (width>maxdotgraphwidth) return first_visit;*/
  // *** was: de->Set_Semaphore(2);
  String ptxtcontent;
  String * ctxt = de->Entry_Text();
  bool isobjective = false, ismilestone = false, isproblem = false, ispossiblesolution = false;
  int planentrytype = de->Is_Plan_Entry();
  // Detect OOP types
  if (planentrytype==PLAN_ENTRY_TYPE_OBJECTIVE) isobjective = true;
  else if (planentrytype==PLAN_ENTRY_TYPE_MILESTONEOBJECTIVE) {
    isobjective = true;
    ismilestone = true;
  }
  else if (planentrytype==PLAN_ENTRY_TYPE_PROBLEM) isproblem = true;
  else if (planentrytype==PLAN_ENTRY_TYPE_POSSIBLESOLUTION) ispossiblesolution = true;
  // If we are _only_ looking for DIL entries with a specific @label:LABEL@
  if (!hierarchylabel.empty()) { // include only DIL entries with a specific @label:LABEL@
    if (!ctxt) return first_visit;
    if (!(*ctxt).contains(hierarchylabel)) return first_visit;
  }
  // If we are _only_ looking for Objectives (or specified 'hierarchyaddnonobjectives')
  if ((hierarchyexcerptlength<1) && (!isobjective) && (!hierarchyaddnonobjectives)) return first_visit;

  // 2.) Collect Special OOP Node Data
  //     If this is an Objective then a.) possibly store another
  //     Objectives pair to connect and b.) push() this new Objective
  //     to 'objectivesstack'.
  if (isobjective) {
    if (objectivesstack.tail()) {
      // Add if this pair does not already exist in the list
      bool foundsame = false;
      PLL_LOOP_FORWARD(DIL_entry_Pair,objectivespairs.head(),1) if (e->Same(objectivesstack.tail()->Entry(),de)) { foundsame=true; break; }
      if (!foundsame) objectivespairs.push(new DIL_entry_Pair(objectivesstack.tail()->Entry(),de));
    }
    objectivesstack.push(new DIL_entry_Pointer(de));
  }

  // 3.) Collect Node Data for Vizualisation
  de->Set_Semaphore(2); // Only counts as "shown" if passes depth and width criteria
  String destr(de->chars()); if (graphtype=='D') destr.gsub('.','_');
  if (first_visit) {
    const BigRegex nonalphanum("[^0-9A-Za-z.]+");
    String textcontent, labelcontent, tooltipcontent;
    // content: possibly collect the DIL ID for a contentfile
    if (isobjective) content += '\n';
    else content += "\nDIL#" + String(de->chars()) + ":\n";
    if (ctxt) { // description text is available
      if ((isobjective) || (isproblem)) { // Collect descriptive text and possible Label for OOP Objective/Problem
	// content: add nothing (newline)
	// ptxtcontent: receives the Objective/Problem Label or "(OBJ/PROB)"
	// labelcontent: receives the Objective/Problem Label or empty (with & appended for URL if .dot output)
	// textcontent: receives entry text description (if .dot output, then sanitized and prepared for appending to URL)
	// tooltipcontent: receives the Objective/Problem Label plus entry text description and special characters replaced
	ptxtcontent = (*ctxt).after("@TEXT@ -->");
	labelcontent = ptxtcontent.before(']'); // empty if no [Label]
	labelcontent = labelcontent.after('[');
	tooltipcontent = labelcontent;
	if (labelcontent.empty()) textcontent = ptxtcontent;
	else textcontent = ptxtcontent.after(']');
	textcontent = textcontent.before("<!-- @end");
	HTML_remove_tags(textcontent);
	ptxtcontent = labelcontent;
	if (ptxtcontent.empty()) ptxtcontent = "(OBJ/PROB)";
	tooltipcontent += ": "+textcontent;
	tooltipcontent.gsub('"','\'');
	tooltipcontent.gsub('\n',' ');

	if (graphtype=='C') { // .xgmml
	  ptxtcontent.gsub('"','\'');
	  textcontent.gsub('"','\'');
	} else if (graphtype=='J') { // .json
	  ptxtcontent.gsub('"','\'');
	  textcontent.gsub('"','\'');
	} else { // .dot
	  textcontent.gsub(nonalphanum,'_');
	  URL_sections(textcontent,56);
	  labelcontent += '&';
	}

      } else { // Collect descriptions from entry without explicit Label
	// content: receives the entry text description without HTML tags
	// ptxtcontent: receives limited length entry text description without HTML tags, newlines or quotation marks, and some OOP specific text abbreviated
	// labelcontent: empty (with & appended for URL if .dot output)
	// textcontent: if 'hierarchyaddnonobjectives' then receives entry text description (if .dot output, then sanitized and prepared for appending to URL)
	// tooltipcontent: receives entry text description with DIL ID appended and special characters replaced
	ptxtcontent = (*ctxt);
	content += (*HTML_remove_tags(ptxtcontent));
	if (hierarchyaddnonobjectives) textcontent = ptxtcontent;
	tooltipcontent = ptxtcontent;
	tooltipcontent += " [DIL#"+String(de->chars())+']';
	tooltipcontent.gsub('"','\'');
	tooltipcontent.gsub('\n',' ');
	switch (planentrytype) {
	case PLAN_ENTRY_TYPE_POSSIBLESOLUTION: ptxtcontent.gsub("POSSIBLE SOLUTION","PS:"); break;;
	case PLAN_ENTRY_TYPE_SEEKPOSSIBLESOLUTIONS: ptxtcontent.gsub("SEEK POSSIBLE SOLUTIONS","SPS:"); ptxtcontent.gsub("for the ",""); break;;
	case PLAN_ENTRY_TYPE_PEERREVIEWSOLUTIONS: ptxtcontent.gsub("PEER REVIEW SOLUTIONS","PRS:"); ptxtcontent.gsub("for the ",""); break;;
	}
	Elipsis_At(ptxtcontent,hierarchyexcerptlength);
	ptxtcontent.gsub('"','\'');

	if (graphtype=='C') { // .xgmml
	  textcontent.gsub('"','\'');
	} else if (graphtype=='J') { // .json
	  textcontent.gsub('"','\'');
	} else { // .dot
	  ptxtcontent.gsub('\n',' ');
	  labelcontent = "&"; // empty
	  textcontent.gsub(nonalphanum,'_');
	  URL_sections(textcontent,56);
	}

     }
    } else ptxtcontent = "(no description text extracted)";

    content += '\n';

    // 4.) Add DIL Entry Data to 's' for Visualization
    //
    // GraphViz .dot format: OOP node styles and regular node styles
    //  "shape=polygon,sides=5,style=\"filled,rounded\",", // objective, with or without detailing task
    //  "shape=polygon,sides=5,style=filled,", // objective, with detailing, problems identified
    //  "shape=polygon,sides=6,style=filled,", // milestone objective
    const char oopshape[][50] = {
      "shape=box,style=filled,", // not objective
      "shape=hexagon,style=filled,", // problem
      "shape=pentagon,style=\"filled,rounded\",", // objective, with or without detailing task
      "shape=pentagon,style=filled,", // objective, with detailing, problems identified
      "shape=house,style=filled,", // milestone objective
      "shape=point,", // non-objective with hierarchyaddnonobjectives
      "shape=oval,style=filled,", // periodic non-objective
      "shape=invhouse,style=filled,", // objective detailing
      "shape=box,style=\"filled,rounded\"," // possible solution
    };
    const char oopproblems[][15] = {
      "magenta,", // incomplete, without solutions, without OOP hierarchy
      "Yellow,", // incomplete, with OOP hierarchy
      "blue,", // completed
      "gray,", // different
      "red,", // incomplete, without even a detailing task!
      "Orange,", // incomplete, solutions identified
      "cyan," // Problem overcome (completed)
    };
    const char regularcolors[][15] = {
      "white,", // completion==0
      "palegreen,", // completion>0 && completion < 1
      "limegreen,", // completion>=1
      "gray," // inactive task, other state (completion<0)
    };

    // Cytoscape .xgmml format
    if (graphtype=='C') {
      // *** For now just using ptxtcontent and tooltipcontent
      s += "  <node id=\"" + destr + "\" label=\"" + ptxtcontent + "\">\n";
      s += "    <att name=\"name\" value=\"" + ptxtcontent + "\" type=\"string\"/>\n";
      s += "    <att name=\"progress\" type=\"integer\"/>\n";
      s += "    <att name=\"targetdate\" value=\"" + time_stamp("%Y-%m-%d",de->Target_Date()) + "\" type=\"string\"/>\n";
      s += "    <att name=\"significance\" type=\"integer\"/>\n";
      s += "    <att name=\"abbrev\" value=\"" + String(ptxtcontent.before(5)) + "\" type=\"string\"/>\n";
      s += "    <att name=\"details\" value=\"" + tooltipcontent + "\" type=\"string\"/>\n";
      s += "    <att name=\"obsolete\" value=\"";
      if (de->Completion_State()<0.0) s += '1';
      else s += '0';
      s += "\" type=\"boolean\"/>\n";
      // *** The color should be improved, see the .dot example below!
      s += "    <att name=\"color\" value=\"#ff00ff\" type=\"string\"/>\n";
      s += "    <att name=\"type\" value=\"";
      if (de->tdperiod()==pt_nonperiodic) {
	switch (planentrytype) {
	case PLAN_ENTRY_TYPE_OBJECTIVE: s += "objective"; break;
	case PLAN_ENTRY_TYPE_MILESTONEOBJECTIVE: s += "milestone objective"; break;
	case PLAN_ENTRY_TYPE_PROBLEM: s += "problem"; break;
	case PLAN_ENTRY_TYPE_POSSIBLESOLUTION: s += "solution"; break;
	default: s += "normal task";
	}
      } else s += "periodic task";
      s += "\" type=\"string\"/>\n";
      s += "    <att name=\"URL1\" value=\"\" type=\"string\"/>\n";
      s += "    <att name=\"URL2\" value=\"\" type=\"string\"/>\n";
      s += "  </node>\n";
      return first_visit;
    }

    // JSON format
    if (graphtype=='J') {
      s += "\n        {\"name\": \"";
      s += ptxtcontent;
      s += "\", \"abbrev\": \"";
      s += destr; // ***this is not really in use yet
      s += "\", \"type\": \"";
      if (isobjective) {
	if (ismilestone) s += "<problem>"; // *** SHOULD BE MILESTONE
	else s += "<objective>";
      } else s += "<task>";
      s += "\", \"id\": ";
      s += destr;
      s += ", \"significance\": ";
      double v = de->Valuation();
      s += String(v,"%.1f");
      s += ", \"targetdate\": \"";
      s += time_stamp("%Y-%m-%d",de->Target_Date());
      s += "\"},";
      return first_visit;
    }

    // GraphViz .dot format
    // Build a Node Statement (node_stmt in http://www.graphviz.org/doc/info/lang.html)
    // A - node identifier (node_id)
    s += "DIL"+destr+" [";
    // B - node attributes (attr_list)
    // B.1 - node shape
    OOP_Status oop;
    if (isobjective) {
      if (ismilestone) { // MILESTONE OBJECTIVE
	// ** for some reason OOP work on milestones is not yet detected and shown
	s += oopshape[4]; // five-sided filled polygon in house shape
      } else { // OBJECTIVE
	oop.get(de);
	if (oop.identified_problems) s += oopshape[3]; // five-sided filled polygon
	//else if (oop.has_detailing_task) s += oopshape[2]; // five-sided filled rounded polygon
	else s += oopshape[2]; // five-sided rounded polygon (was: oopshape[1])
      }
      s += "margin=\"0.11,0.055\",";
    } else { // OTHER DIL ENTRY
      if (hierarchyaddnonobjectives) {
	s += oopshape[5]; // point
      } else {
	if (isproblem) {
	  s += oopshape[1]; // hexagon
	} else if (ispossiblesolution) {
	  s += oopshape[8]; // filled rounded box
	} else {
	  if (de->tdperiod()==pt_nonperiodic) {
	    int oopshapeid = 0;
	    if (ctxt) if (ctxt->contains("Detail further objectives")) oopshapeid = 7;
	    s += String(oopshape[oopshapeid]); // filled box
	  } else s += String(oopshape[6]); // oval
	}
      }
    }
    // B.2 - node fillcolor
    if (isobjective) {
      if (ismilestone) { // MILESTONE OBJECTIVE
	// ** for some reason OOP work on milestones is not yet detected and shown
	s += "fillcolor=";
	if (de->Completion_State()>=1.0) s += oopproblems[2];
	else if (de->Completion_State()<0.0) s += oopproblems[3];
	else s += oopproblems[0];
      } else { // OBJECTIVE
	s += "fillcolor=";
	if (de->Completion_State()>=1.0) { s += oopproblems[2]; s += "fontcolor=white,"; }
	else if (de->Completion_State()<0.0) s += oopproblems[3];
	else if (oop.OOP_hierarchy_set) s += oopproblems[1];
	else if (oop.identified_solutions) s += oopproblems[5];
	else if (oop.has_detailing_task) s += oopproblems[0];
	else s += oopproblems[4];
      }
    } else if (isproblem) { // PROBLEM
      s += "fillcolor=";
      if (de->Completion_State()<0.0) s += oopproblems[3];
      else if (de->Completion_State()>=1.0) s += oopproblems[6];
      else s += oopproblems[1];
    } else { // OTHER DIL ENTRY
      s += "fillcolor=";
      if (de->Completion_State()==0.0) { if (ispossiblesolution) s += oopproblems[4]; else s += regularcolors[0]; }
      else if (de->Completion_State()>=1.0) { s += regularcolors[2]; s += "fontsize=8,"; }
      else if (de->Completion_State()<0.0) { s += regularcolors[3]; s += "fontsize=8,"; }
      else s += regularcolors[1];
      //else s += String(de->Completion_State(),"\"limegreen;%.2f:palegreen\","); *** POSSIBLE WITH dot -v > 2013 06/29
    }
    // B.3 - node URL
    s += "URL=\"file://"+idfile+'#'+String(de->chars());
    if ((isobjective) || (hierarchyaddnonobjectives)) s += "?dil2alURLLABEL="+labelcontent+"dil2alURLTEXT=)"+textcontent;
    s += '"';
    // B.4 - possible tooltip
    if (!tooltipcontent.empty()) s += ",tooltip=\""+tooltipcontent+'"';
    // B.5 - possible label and additional attributes
    //if (width>=maxdotgraphwidth) s += ",color=red";
    if ((isobjective) || (!hierarchyaddnonobjectives)) {
      //if (!isobjective) s += ",label=\"DIL#"+String(de->chars());
      //if (isobjective)
      s += ",label=\"" + ptxtcontent;
      // We cannot search for all target date information here, because unfortunately
      // the functions Target_Date() and Targe_Date_Info() all use SetSemaphore as well.
      // ** Note that Propagated_Target_Date_Origin() does not appear to use the
      // semaphore!
      // The rule here is: Look for the locally specified target dates connected with
      // the Superior 'prede' (if 'prede' is not NULL). If it is not set then don't
      // show a target date. If it is set locally then show that date. At least this
      // way you can see for the tree structure of the graph generated if the order of
      // dates makes sense.
      if ((de->Completion_State()>=0.0) && (de->Completion_State()<1.0)) {
	DIL_Superiors * dsup = de->Projects(0);
	time_t tdate = MAXTIME_T;
	if (prede) { // Show a local target date that takes precedence if there is one
	  PLL_LOOP_FORWARD(DIL_Superiors,dsup,1) if (e->Superiorbyid()==prede) { if (e->targetdate()>=0) tdate=e->targetdate(); break; }
	} else { // Show the smallest locally defined target date if there is one
	  PLL_LOOP_FORWARD(DIL_Superiors,dsup,1) if ((e->targetdate()>=0) && (e->targetdate()<tdate)) tdate=e->targetdate();
	}
	if (tdate<MAXTIME_T) s += time_stamp(" %Y%m%d",tdate);
      }
      /*bool isfromlocal; int haslocal, numpropagating; // Unfortunately this messes with things due to SetSemaphore!
      time_t pretdate = MAXTIME_T;
      if (prede) pretdate = prede->Target_Date();
      time_t tdate = de->Target_Date_Info(isfromlocal,haslocal,numpropagating);
      if ((tdate<MAXTIME_T) && (tdate!=pretdate)) s += time_stamp(" %Y%m%d",tdate);*/
      s += '"';
      //else s += ":\\n" + ptxtcontent + '"';
      if (!prede) s += ",peripheries=2";
    }
    s += "];\n";
  }
  /*if (prede) {
    String predestr(prede->chars()); predestr.gsub('.','_');
    if (reversehierarchy) s += "DIL"+predestr+" -> DIL"+destr+" [";
    else s += "DIL"+predestr+" -> DIL"+destr+" [dir=back,";
    DIL_Superiors * suppars = de->Superior_Parameters(prede);
    if (suppars) {
      if (suppars->relevance>=1.0) s += "color=blue";
      else s += "color=black";
    } else s += "color=black";
    s += "];\n";
    }*/
  return first_visit;
}

void DIL_Visualize_GRAPH::Add_Connections(DIL_entry * de) {
    // This function is normally called for every DIL entry in the list.
    // For all pairs of 'de' and one of its superior DIL entries, if both
    // have been marked (with the Semaphore) to appear in the visualized
    // graph, then draw a connection between the two.
    // Beware of the oddity with 'reversehierarchy' and "dir=back"!
    if (de->Get_Semaphore()<2) return;
    String destr(de->chars());
    if (graphtype=='D') destr.gsub('.','_');
    DIL_AL_List * deproj = de->Projects(0);
    DIL_entry * supde = NULL;
    if (deproj) PLL_LOOP_FORWARD(DIL_AL_List,deproj,1) if ((supde=e->Superiorbyid())!=NULL) {
	if (supde->Get_Semaphore()>=2) {
	  String supdestr(supde->chars());

	  if (graphtype=='C') {
	    // id is not *required* according to http://cgi7.cs.rpi.edu/research/groups/pb/punin/public_html/XGMML/DOC/xgmml_schema.html#element_edge_Link01C919E0
	    // Also see: https://code.google.com/p/dynnetwork/wiki/DynamicXGMML
	    s += "  <edge label=\"" + destr + '-' + supdestr + "\" source=\"" + destr + "\" target=\"" + supdestr + " cy:directed=\"1\">\n";
	    DIL_Superiors * suppars = de->Superior_Parameters(supde);
	    if (suppars) {
	      if (suppars->relevance>=1.0) s += "    <graphics width=\"2\"/>\n";
	      else s += "    <graphics width=\"1\"/>\n";
	    } else s += "    <graphics width=\"1\"/>\n";
	    s += "  </edge>\n";
	    // *** For more attributes, see: http://wiki.cytoscape.org/XGMML
	    // *** Beware of the oddity with 'reversehierarchy' and "dir=back"!
	  } if (graphtype=='J') {
	    s += "{\"source\":" + destr + ", \"target\": " + supdestr + "},\n";
	    // *** For more work on the JSON version, see http://166.78.102.229/roadmap.json
	    // *** Beware of the oddity with 'reversehierarchy' and "dir=back"!
	  } else { 
	    supdestr.gsub('.','_');
	    if (reversehierarchy) s += "DIL" + destr + " -> DIL" + supdestr + " [";
	    else s += "DIL" + supdestr + " -> DIL" + destr + " [dir=back,";
	    DIL_Superiors * suppars = de->Superior_Parameters(supde);
	    if (suppars) {
	      if (suppars->relevance>=1.0) s += "color=blue";
	      else s += "color=black";
	    } else s += "color=black";
	    s += "];\n";
	  }
	  
	}
      }
}

void DIL_Visualize_GRAPH::Add_Abstract_Connections() {
  // *** For more work on the JSON version, see http://166.78.102.229/roadmap.json
  // Add visualizations of abstract (in)direct connections between pairs of Objectives.
  PLL_LOOP_FORWARD(DIL_entry_Pair,objectivespairs.head(),1) if ((e->First()) && (e->Second())) {
    String firststr(e->First()->chars());
    String secondstr(e->Second()->chars());
    
    if (graphtype=='C') {
      s += "  <edge label=\"OOP" + firststr + '-' + secondstr + "\" source=\"" + firststr + "\" target=\"" + secondstr + " cy:directed=\"1\">\n";
      s += "    <graphics width=\"3\" fill=\"#FF0000\" transparency=\"50\"/>\n";
      s += "  </edge>\n";      
      // *** Beware of the oddity with 'reversehierarchy' and "dir=back"!
    } else if (graphtype=='J') {
      s += "{\"source\":" + firststr + ", \"target\": " + secondstr + "},\n";
      // *** For more work on the JSON version, see http://166.78.102.229/roadmap.json
      // *** Beware of the oddity with 'reversehierarchy' and "dir=back"!
    } else {
      firststr.gsub('.','_');
      secondstr.gsub('.','_');
      if (reversehierarchy) s += "DIL" + firststr + " -> DIL" + secondstr + " [";
      else s += "DIL" + secondstr + " -> DIL" + firststr + " [dir=back,";
      s += "color=red,weight=5];\n";
    }

  }
}

long GRAPHtermnodenum = 0;

void DIL_Visualize_GRAPH::Visualize_NotShown(int numdependencies, int depth, int width, DIL_entry * prede) {
  if (width>maxdotgraphwidth) return;
  // If this entry (here referenced by prede) is an Objective then it needs to be popped from objectivesstack
  if ((prede) && (objectivesstack.tail())) if (objectivesstack.tail()->Entry()==prede) { DIL_entry_Pointer * dilep = objectivesstack.pop(); delete dilep; }
  if ((hierarchyexcerptlength<1) && (!hierarchyaddnonobjectives)) return; // SPECIAL CASE, looking for Objectives!
  if (numdependencies>0) {

    // Cytoscape .xgmml format
    if (graphtype=='C') {
      // *** For now just using ptxtcontent and tooltipcontent
      s += "  <node id=\"term" + String((long) GRAPHtermnodenum) + "\" label=\"term" + String((long) GRAPHtermnodenum) + "\">\n";
      s += "    <att name=\"name\" value=\"term" + String((long) GRAPHtermnodenum) + "\" type=\"string\"/>\n";
      s += "    <att name=\"progress\" type=\"integer\"/>\n";
      s += "    <att name=\"targetdate\" value=\"-1\" type=\"string\"/>\n";
      s += "    <att name=\"significance\" type=\"integer\"/>\n";
      s += "    <att name=\"abbrev\" value=\"term" + String((long) GRAPHtermnodenum) + "\" type=\"string\"/>\n";
      s += "    <att name=\"details\" value=\"" + String((long) numdependencies);
      if (hierarchyexcerptlength<1) s += "\" type=\"string\"/>\n";
      else {
	if (reversehierarchy) s += " superiors\" type=\"string\"/>\n";
	else s += " dependencies\" type=\"string\"/>\n";
      }
      s += "    <att name=\"obsolete\" value=\"0\" type=\"boolean\"/>\n";
      // *** The color should be improved, see the .dot example below!
      s += "    <att name=\"color\" value=\"#ff00ff\" type=\"string\"/>\n";
      s += "    <att name=\"type\" value=\"not-shown\" type=\"string\"/>\n";
      s += "    <att name=\"URL1\" value=\"\" type=\"string\"/>\n";
      s += "    <att name=\"URL2\" value=\"\" type=\"string\"/>\n";
      s += "  </node>\n";
      if (prede) {
	String predestr(prede->chars());
	s += "  <edge label=\"DIL" + predestr + "-term" + String((long) GRAPHtermnodenum) + "\" source=\"" + predestr + "\" target=\"term" + String((long) GRAPHtermnodenum) + " cy:directed=\"1\">\n";
	s += "    <graphics width=\"1\" fill=\"#FF0000\"/>\n";
	s += "  </edge>\n";
      }      
      GRAPHtermnodenum++;
      return;
    }
    // JSON format
    if (graphtype=='J') {
      s += "\n        {\"name\": \"";
      s += "term" + String((long) GRAPHtermnodenum);
      s += "\", \"abbrev\": \"";
      s += "term" + String((long) GRAPHtermnodenum);
      s += "\", \"type\": \"<not-shown>\", \"id\": ";
      s += "term" + String((long) GRAPHtermnodenum);
      s += ", \"significance\": 1.0, \"targetdate\": \"-1\"},";
      if (prede) {
	String predestr(prede->chars());
	s += "{\"source\":" + predestr + ", \"target\": term" + String((long) GRAPHtermnodenum) + "},\n";
      }
      GRAPHtermnodenum++;
      return;
    }
    // GraphViz .dot format
    if (depth>0) s += replicate('\t',depth); // *** I don't think this is actually relevant to .dot output
    s += "term" + String((long) GRAPHtermnodenum); 
    s += " [shape=plaintext,";
    //if (width>=maxdotgraphwidth) s += "fontcolor=red,";
    s += "label=\""+String((long) numdependencies);
    if (hierarchyexcerptlength<1) s += "\"]\n";
    else {
      if (reversehierarchy) s += " superiors\"]\n";
      else s += " dependencies\"]\n";
    }
    if (prede) {
      String predestr(prede->chars()); predestr.gsub('.','_');
      if (reversehierarchy) s += "DIL"+predestr+" -> term" + String((long) GRAPHtermnodenum)+";\n";
      else s += "DIL"+predestr+" -> term" + String((long) GRAPHtermnodenum)+" [dir=back];\n"; //"term" + String((long) GRAPHtermnodenum)+" -> DIL"+predestr+";\n";
    }
    GRAPHtermnodenum++;

  }
}

void DIL_Visualize_GRAPH::Visualize_BeyondWidth(int width, DIL_entry * de) {
  if (width<=maxdotgraphwidth) return;
  width -= maxdotgraphwidth;
  String destr(de->chars());

  // add node (WIDEdestr) and an edge between WIDEdestr and destr
  // Cytoscape .xgmml format
  if (graphtype=='C') {
    s += "  <node id=\"WIDE" + destr + "\" label=\"" + String((long) width) + "\">\n";
    s += "    <att name=\"name\" value=\"" + String((long) width) + "\" type=\"string\"/>\n";
    s += "    <att name=\"progress\" type=\"integer\"/>\n";
    s += "    <att name=\"targetdate\" value=\"-1\" type=\"string\"/>\n";
    s += "    <att name=\"significance\" type=\"integer\"/>\n";
    s += "    <att name=\"abbrev\" value=\"" + String((long) width) + "\" type=\"string\"/>\n";
    s += "    <att name=\"details\" value=\"" + String((long) width) + "\" type=\"string\"/>\n";
    s += "    <att name=\"obsolete\" value=\"0\" type=\"boolean\"/>\n";
    // *** The color should be improved, see the .dot example below!
    s += "    <att name=\"color\" value=\"#ff0000\" type=\"string\"/>\n";
    s += "    <att name=\"type\" value=\"not-shown\" type=\"string\"/>\n";
    s += "    <att name=\"URL1\" value=\"\" type=\"string\"/>\n";
    s += "    <att name=\"URL2\" value=\"\" type=\"string\"/>\n";
    s += "  </node>\n";
    s += "  <edge label=\"DIL" + destr + "-WIDE" + destr + "\" source=\"" + destr + "\" target=\"WIDE" + destr + " cy:directed=\"1\">\n";
    s += "    <graphics width=\"1\" fill=\"#FF0000\"/>\n";
    s += "  </edge>\n";      
    return;
  }
  // JSON format
  if (graphtype=='J') {
    s += "\n        {\"name\": \"";
    s += "WIDE" + destr;
    s += "\", \"abbrev\": \"";
    s += String((long) width);
    s += "\", \"type\": \"<not-shown>\", \"id\": ";
    s += String((long) width);
    s += ", \"significance\": 1.0, \"targetdate\": \"-1\"},";
    s += "{\"source\":" + destr + ", \"target\": WIDE" + destr + "},\n";
    return;
  }
  // GraphViz .dot format
  destr.gsub('.','_');
  s += "WIDE" + destr + " [shape=trapezium,color=red,fontcolor=green,label=\"" + String((long) width) + "\"];\n";
  if (reversehierarchy) s += "DIL"+destr+" -> WIDE" + destr +";\n";
  else s += "DIL"+destr+" -> WIDE" + destr +" [dir=back];\n"; // "WIDE" + destr +" -> DIL"+destr+";\n";
}

// DIL administration functions

bool get_active_DIL_IDs(StringList & idfileids, unsigned long * resnumidfileids) {
// obtains all DIL IDs from the DIL-by-ID file
// optionally adds the number of IDs found to resnumidfileids
	const int LLEN = 10240;
	char lbuf[LLEN];
	String dilid[2];
	unsigned long numidfileids = 0;
	int res;
	ifstream df(idfile);
	if (df) {
		if (!find_line(&df,"[<]!-- *dil2al: *DIL ID *begin *--[>]",lbuf,LLEN)) {
			EOUT << "dil2al: Tag <!-- dil2al: DIL ID begin --> not found in " << idfile << "in get_active_DIL_IDs()\n";
			return false;
		}
		do {
			if ((res=find_in_line(&df,"[<]TR[>]",0,NULL,"[<]!-- *dil2al: *DIL ID *end *--[>]",lbuf,LLEN))==1) {
				res = find_in_line(&df,"[<]TD[^>]*[>][<]A +NAME=\"([.0-9]+)\"",2,dilid,"[<]TR[>]|[<]!-- *dil2al: *DIL ID *end *--[>]",lbuf,LLEN);
				if (res<-1) return false;
				idfileids[numidfileids++] = dilid[1];
			} else {
				if (res>=-1) res=-1;
				else return false;
			}
		} while ((res>0) || ((res==0) && (!pattern_in_line("[<]!-- *dil2al: *DIL ID *end *--[>]",lbuf))));
		df.close();
	} else {
		EOUT << "dil2al: Unable to open DIL ID file " << idfile << "in get_active_DIL_IDs()\n";
		return false;
	}
	if (resnumidfileids) (*resnumidfileids) += numidfileids;
	return true;
}

String superior_reference_info(StringList * supinfoptr) {
// add superior DIL entry references to DIL-by-ID entry
	String res("</A> (");
	if (supinfoptr) {
		int suprefsadded = 0;
		res += "1.0)\n<TD>";
		for (unsigned int sr=0; sr<supinfoptr->length(); sr++) if ((*supinfoptr)[sr]!="") {
			if (suprefsadded>0) res += ", ";
			res += (*supinfoptr)[sr];
			suprefsadded++;
		}
		res += "\n\n";
		if (verbose) INFO << suprefsadded << " superior DIL entry references added\n";
	} else res += "?.?)\n<TD>&nbsp;\n\n";
	return res;
}

bool add_new_to_DIL(String dilfile, StringList * supinfoptr) {
// Adds all new DIL entry ID references to the DIL-by-ID file,
// with superior DIL entry information if provided in supinfoptr
  // 1. - The dilfile
  // get name of DIL file
  String dilfilename;
  int idx = dilfile.index('/',-1);
  if (idx<0) dilfilename = dilfile;
  else dilfilename = dilfile.after(idx);
  if (dilfilename.empty()) {
    EOUT << "dil2al: Unable to parse name from DIL file path '" << dilfile << "' in add_new_to_DIL()\n";
    return false;
  }
  dilfile=basedir+RELLISTSDIR+dilfilename; // clean reconstruct, just in case
  if (verbose) INFO << "Adding new DIL ID from " << dilfile << '\n';
  // load the whole DIL file
  String dfstr;
  if (!read_file_into_String(dilfile,dfstr)) return false;
  // get title of DIL file
  String dilfiletitle;
  int titlestart = dfstr.index("<TITLE>",0);
  if (titlestart<0) {
    ERROR << "add_new_to_DIL(): Unable to find title of DIL file.\n";
    return false;
  }
  titlestart += 7;
  if ((idx = dfstr.index('<',titlestart))<0) {
    ERROR << "add_new_to_DIL(): Unable to parse title from title line of DIL file.\n";
    return false;
  }
  dilfiletitle = dfstr.at(titlestart,idx-titlestart);
  dilfiletitle.gsub(BigRegex(" *DIL: *"),"");
  // put DIL IDs into StringList in order
  if ((idx = dfstr.index(BigRegex("[<]!-- *dil2al: *DIL *begin *--[>]"),idx))<0) {
    ERROR << "add_new_to_DIL(): Tag <!-- dil2al: DIL begin --> not found in " << dilfile << ".\n";
    return false;
  }
  if (verbose) INFO << "Reading DIL IDs from " << dilfiletitle << " DIL file\n";
  StringList dilfileids; long numdilfileids = 0; String dilid;
  while ((idx = dfstr.index("<TR BGCOLOR=\"#00005F",idx))>=0) { // could use BigRegex("[<]TR +BGCOLOR=\"#00005F") if less predictable
    if ((idx = dfstr.index("<TD WIDTH=\"15%\"><A NAME=\"",idx+20))>=0) { // could use BigRegex("[<]TD[^>]*[>][<]A +NAME=\"([.0-9]+)\"") if less predictable
      dilid = dfstr.at(idx+25,16); // *** we are ASSUMING a valid DIL ID follows the NAME reference!
      if (numdilfileids==0) dilfileids[0] = dilid;
      else {
	if (dilfileids[numdilfileids-1]<=dilid) dilfileids[numdilfileids] = dilid;
	else {
	  long i;
	  for (i = numdilfileids-2; i>=0; i--) if (dilfileids[i]<=dilid) {
	      dilfileids.insert(i+1,dilid);
	      break;
	    }
	  if (i<0) dilfileids.insert(0,dilid);
	}
      }
      numdilfileids++;
    }
  }
  // 2. - The idfile
  // compare with all DIL IDs in detailed-items-by-ID.html, copy and insert/add
  if (verbose) INFO << "Comparing " << numdilfileids << " DIL IDs with DIL IDs in " << idfile << " for possible addition\n";
  if (!read_file_into_String(idfile,dfstr)) return false; // now dfstr contains the detailed-items-by-ID.html file
  if ((idx = dfstr.index("<!-- dil2al: DIL ID begin -->",idx))<0) {
    ERROR << "dil2al: Tag <!-- dil2al: DIL ID begin --> not found in " << idfile << '\n';
    return false;
  }
  long i = 0, numidsadded = 0; // comparing with dilfileids[i]
  int copyidx = 0; // copied all up to, but not including, dfstr[copyidx]
  String newidstr, iddilid;
  // find first <TR> and copy up to it
  if ((idx = dfstr.index("<TR>",idx+29))<0) { // special case, this must be a brand new empty idfile!
    if ((idx = dfstr.index("<!-- dil2al: DIL ID end -->",idx+29))<0) {
      EOUT << "add_new_to_DIL(): Tag <!-- dil2al: DIL ID end --> not found in " << idfile << ".\n";
      return false;
    }
    newidstr = dfstr.before(idx); // jump to adding of dilfileids[] and copying to end of idfile
    copyidx = idx;
  } else { // idx is pointing to the start of <TR>
    newidstr = dfstr.before(idx); // copied up to first <TR>
    copyidx = idx;
    while ((i<numdilfileids) && ((idx = dfstr.index("<TD><A NAME=\"",copyidx))>=0)) { // valid DIL IDs in dilfile and idfile
      iddilid = dfstr.at(idx+13,16); // *** we are ASSUMING a valid DIL ID follows the NAME reference!
      int c = 0;
      while ((i<numdilfileids) && (c=compare(dilfileids[i],iddilid))<0) { // dilfileids[i] belongs here
	newidstr += "<TR>\n<TD><A NAME=\"" + dilfileids[i] + "\">" + dilfileids[i] + "</A>\n<TD><A HREF=\"" + dilfilename + '#' + dilfileids[i] + "\">" + dilfiletitle + superior_reference_info(supinfoptr);
	numidsadded++;
	i++;
      }
      if (i<numdilfileids) { // got here due to c>=0, not because of i>=numdilfileids
	if (c==0) i++; // DIL ID already exists in idfile
	if ((idx = dfstr.index("<TR>",idx+29))<0) { // already at last entry in idfile
	  if ((idx = dfstr.index("<!-- dil2al: DIL ID end -->",idx+29))<0) {
	    ERROR << "add_new_to_DIL(): Tag <!-- dil2al: DIL ID end --> not found in " << idfile << ".\n";
	    return false;
	  }
	  newidstr += dfstr.at(copyidx,idx-copyidx); // copy to end marker
	  copyidx = idx;
	  break; // stop searching for NAME references
	}
	newidstr += dfstr.at(copyidx,idx-copyidx); // copy to next <TR>
	copyidx = idx;
      }
    }
    // here, if all in dilfileids[] belonged before the last entry in idfile there may be some of idfile left, if new DIL IDs need to be added to the end of the table then all entries already in idfile will have been walked through
  }
  // add all remaining dilfileids[]
  for (; i<numdilfileids; i++) {
    newidstr += "<TR>\n<TD><A NAME=\"" + dilfileids[i] + "\">" + dilfileids[i] + "</A>\n<TD><A HREF=\"" + dilfilename + '#' + dilfileids[i] + "\">" + dilfiletitle + superior_reference_info(supinfoptr);
    numidsadded++;
  }
  // copy to end of idfile, save with backup
  newidstr += dfstr.from(copyidx);
  if (!write_file_from_String(idfile,newidstr,"DIL ID")) return false;
  // report number of new DIL IDs
  if (verbose) INFO << "Inserted or added " << numidsadded << " new IDs to " << idfile << '\n';
  return true;
}

bool create_new_DIL(String dilfile) {
  // Creates a new DIL file with the file name dilfile.
  // dilfile may be a relative or absolute file path.
  // returns true if successful.
  // returns false if canceled or if an error was encountered.
 
  // get name of DIL file regardless of whether it was a relative or absolute path
  String dilid[2];
  if (!substring_from_line("([^/]+)$",2,dilid,(const char *) dilfile)) {
    ERROR << "create_new_DIL(): Unable to parse name from DIL file path.\n";
    return false;
  }
  String dilfilename(dilid[1]);
  dilfile=basedir+RELLISTSDIR+dilfilename;
  
  if (verbose) INFO << "Creating new DIL file " << dilfile << '\n';
  ifstream df(dilfile);
  if (df) {
    df.close();
    if (!confirmation("DIL file"+dilfilename+" already exists, overwrite? (y/N) ",'y',"NO","Yes")) return false;
  }
  
  ofstream ndf(dilfile);
  if (!ndf) {
    ERROR << "create_new_DIL(): Unable to create file " << dilfile << ".\n";
    return false;
  }
  
  if (verbose) INFO << "Initializing new DIL file\n";
  String diltitle(input->request("DIL title for "+dilfilename+": ",true));
  if ((diltitle.empty()) && (yad_canceled)) return false;
  
  ndf << "<HTML>\n<HEAD>\n<TITLE>DIL: " << diltitle << "</TITLE>\n";
  ndf << "</HEAD>\n<BODY BGCOLOR=\"#000000\" TEXT=\"#F0E0A0\" LINK=\"#AFAFFF\" VLINK=\"#7F7FFF\">\n";
  ndf << "<H1><FONT COLOR=\"#00FF00\">DIL: " << diltitle << "</FONT></H1>\n\n";

  String keywords(input->request("DIL keywords and relevance, e.g. neuron (1.0), synapse (1.0): ",true));
  if ((keywords.empty()) && (yad_canceled)) return false;
  
  ndf << "<B>Topic Keywords, k_{top} (and relevance in [0,1]):</B> " << keywords << "\n<P>\n\n";
  ndf << "<TABLE WIDTH=\"100%\" CELLPADDING=3 BORDER=2>\n<!-- dil2al: DIL begin -->\n\n<!-- dil2al: DIL end -->\n</TABLE>\n\n\n";
  ndf << "<LI><A HREF=\"detailed-items-by-ID.html\">Detailed Items by ID</A>\n<LI><A HREF=\"../lists.html#DIL\">Topical Detailed Items Lists</A>\n<P>\n";
  ndf << "<HR>~/doc/<A HREF=\"../maincont.html\">html</A>/<A HREF=\"../lists.html\">lists</A>/"
      << dilfilename << " <FONT COLOR=\"#AFAFAF\"><I>&nbsp;&nbsp;&nbsp;(created " << curdate << ", Randal A. Koene)</I></FONT>\n\n</BODY>\n";
  ndf.close();

  // Add DIL title and reference to list.html
  df.open(listfile);
  if (!df) {
    ERROR << "create_new_DIL(): Unable to open " << listfile << ".\n";
    return false;
  }
  ndf.open(listfile+".new");
  if (!ndf) {
    ERROR << "create_new_DIL(): Unable to create " << listfile << ".new.\n";
    return false;
  }

  if (verbose) INFO << "Adding new DIL to " << listfile << '\n';
  const int LLEN = 10240;
  char lbuf[LLEN];
  if (copy_until_line(&df,&ndf,"<H1><A NAME=\"DIL\">",lbuf,LLEN)<=0) {
    ERROR << "create_new_DIL(): No DIL header ``<H1><A NAME=\"DIL\">'' found.\n";
    return false;
  }
  ndf << lbuf << endl;
  if (copy_until_line(&df,&ndf,"</UL>",lbuf,LLEN)<=0) {
    ERROR << "create_new_DIL(): No DIL list end ``</UL>'' found.\n";
    return false;
  }
  ndf << "<LI><A HREF=\"" << RELLISTSDIR << dilfilename << "\">" << diltitle << "</A>\n";
  ndf << lbuf << endl;
  ndf << df.rdbuf();
  df.close();
  ndf.close();
  
  if (!backup_and_rename(listfile,"lists index")) return false;
  return true;
}

bool send_dil_to_DIL(String dilfile) {
  
  // read entry data into string buffer (to avoid confusion in case stdin and create file requests)
  String dentrybuf;
  if (verbose) INFO << "Reading DIL entry content\n";
  read_data_from_stream(DIN,dentrybuf,"@End of DIL entry@");

  // get inline tag information and remove those inline tags
  String supinfotag("DESUP"); supinfotag = RX_Search_Safe(generate_tag(supinfotag)); // get standardized tag
  supinfotag.gsub(BigRegex(":[ 	]*"),":[%_%]*\\(");
  supinfotag.gsub(" ","[ 	]+");
  supinfotag.gsub("%_%"," 	");
  supinfotag.gsub("SUPid","[.0-9]+");
  supinfotag.gsub(BigRegex("SI[^,)]*"),"[^,)]*");
  supinfotag.gsub(BigRegex("\\(.\\)@"),"_1\\)@",'_');
  BigRegex si(supinfotag); StringList supinfo; int supnum = 0;
  while (dentrybuf.index(si)>=0) {
    supinfo[supnum] = dentrybuf.at(si.subpos(1),si.sublen(1)); // get superior id and information
    dentrybuf.del(si.subpos(0),si.sublen(0)); // remove tag
    supnum++;
  }
  
  // get name of DIL file regardless of whether it was a relative or absolute path
  String dilid[2];
  if (!substring_from_line("([^/]+)$",2,dilid,(const char *) dilfile)) {
    ERROR << "dil2alsend_dil_to_DIL(): Unable to parse name from DIL file path.\n";
    return false;
  }

  String dilfilename(dilid[1]);
  dilfile=basedir+RELLISTSDIR+dilfilename;
  
  if (verbose) INFO << "Sending DIL entry to " << dilfile << '\n';
  ifstream df(dilfile);
  if (!df) {
    // optionally create new DIL file
    if (confirmation("DIL file "+dilfilename+" does not exist, create? (y/N) ",'y',"NO","Yes")) return false;
    if (!create_new_DIL(dilfilename)) {
      ERROR << "send_dil_to_DIL(): Unable to create new DIL file " << dilfilename << ".\n";
      return false;
    }
    df.open(dilfile);
  }

  if (!df) {
    ERROR << "dil2alsend_dil_to_DIL(): Unable to open new DIL file " << dilfilename << ".\n";
    return false;
  }
  
  ofstream ndf(dilfile+".new");
  if (!ndf) {
    ERROR << "dil2alsend_dil_to_DIL(): Unable to create " << dilfile << ".new.\n";
    return false;
  }
  
  if (verbose) INFO << "Copying DIL to " << dilfile << ".new and adding new entry\n";
  // copy content up to bottom of DIL table
  const int LLEN = 10240;
  char lbuf[LLEN];
  if (copy_until_line(&df,&ndf,"[<]!-- *dil2al: *DIL *end *--[>]",lbuf,LLEN)<=0) {
    ERROR << "send_dil_to_DIL(): End of DIL table ``<!-- dil2al: DIL end -->'' not found.\n";
    return false;
  }
  // add new DIL content
  ndf << dentrybuf;
  // place end of DIL table marker
  ndf << lbuf << endl;
  // copy rest of DIL file
  ndf << df.rdbuf();
  df.close();
  ndf.close();
  
  if (!backup_and_rename(dilfile,"DIL")) return false;
  
  StringList * supinfoptr = NULL;
  if (supinfo[0]!="") supinfoptr = &supinfo;
  if (!add_new_to_DIL(dilfile,supinfoptr)) {
    ERROR << "send_dil_to_DIL(): Unable to add new item from " << dilfile << " to DIL.\n";
    return false;
  }
  
  return true;
}

String get_new_DIL_ID() {
  // generates a new DIL ID based on the current time
  // and the next single decimal available with that
  // time stamp (in the unlikely event that there are
  // already 9 items with that time stamp, it will
  // try an ID based on another time stamp)
  
  // obtain all existing DIL IDs
  StringList idfileids;
  unsigned long numidfileids = 0;
  if (!get_active_DIL_IDs(idfileids,&numidfileids)) {
    ERROR << "get_new_DIL_ID(): Unable to obtain DIL IDs from " << idfile << ".\n";
    return "";
  }

  // generate a new time stamped DIL ID
  String newdilid;
  for (int loopescape=60; (loopescape); loopescape--) { 
    String idtimestamp = time_stamp("%Y%m%d%H%M%S");
    // try ID decimal indices
    for (char decimalindex = '1'; decimalindex <= '9'; decimalindex++) {
      newdilid = idtimestamp + '.' + decimalindex;
      for (StringList * idfileentry = &idfileids; (idfileentry); idfileentry=idfileentry->get_Next()) {
	if (newdilid==(*idfileentry)[0]) {
	  newdilid="";
	  break;
	}
      }
      if (newdilid!="") break;
    }
    if (newdilid!="") break;
    if (loopescape>1) {
      sleep(1); /* usleep(1000000L); 1000000 microsec */
    }
  }

  return newdilid;
}

void generate_modify_element_FORM_interface_DIL_entry_content(DIL_entry * de) {
  // generates FORM components specific to editing the content elements of
  // a DIL entry
  VOUT << "<FORM METHOD=\"POST\" ACTION=\"/cgi-bin/dil2al\"><INPUT type=\"hidden\" name=\"dil2al\" value=\"MEi\">\n<P>\n";
  // ID
  VOUT << "<A HREF=\"";
  if (!de->parameters) VOUT << idfile << '#' << de->chars();
  else if (!de->parameters->topics.head()) VOUT << idfile << '#' << de->chars();
  else VOUT << de->parameters->topics.head()->dil.file << '#' << de->chars();
  VOUT << "\"><B>DIL ID# " << de->chars() << "</B> Content</A> <INPUT type=\"hidden\" name=\"DILID\" value=\"" << de->chars() << "\">([2,ESC,SHIFT+M,l] to edit)\n<P>\n";
  if (de->content) {
    VOUT << "<TABLE>\n";
    // valuation
    if (de->Is_Plan_Entry()==PLAN_ENTRY_TYPE_OUTCOME) VOUT << "<TR><TD ALIGN=RIGHT><B>desirability</B> = <TD>" << String((double) de->content->PLAN_OUTCOME_DESIRABILITY,"%.2f") << " <TD><INPUT type=\"text\" name=\"valuation\" maxlength=5><TD>&nbsp;\n<P>\n";
    else VOUT << "<TR><TD ALIGN=RIGHT>valuation = <TD>" << String((double) de->content->valuation,"%.2f") << " <TD><INPUT type=\"text\" name=\"valuation\" maxlength=5><TD>&nbsp;\n<P>\n";
    // completion ratio
    String completionstr((double) de->content->completion,"%.2f");
    VOUT << "<TR><TD ALIGN=RIGHT>completion ratio = <TD>" << completionstr << " <TD><INPUT type=\"text\" name=\"completion\" maxlength=5><TD>special completion values:\n<UL>\n<LI>-1 = obsolete\n<LI>-2 = replaced\n<LI>-3 = done differently\n<LI>-4 = no longer possible / did not come to pass\n</UL>\n<P>\n";
    // time required
    String reqstr((double) (de->content->required/3600.0),"%.2f");
    String completedreq((double) (de->content->completion*de->content->required/3600.0),"%.3f");
    VOUT << "<TR><TD ALIGN=RIGHT>time required = <TD>" << reqstr << " <TD><INPUT type=\"text\" name=\"required\" maxlength=5><TD>Set this task <B>completed</B> <INPUT type=\"checkbox\" name=\"setcompleted\" value=" << completedreq << "> at " << completedreq << " hours\n<BR>Add <INPUT type=\"text\" name=\"addrequired\" maxlength=5> hours to time required.<INPUT type=\"hidden\" name=\"reqtoaddto\" value=" << reqstr << "><INPUT type=\"hidden\" name=\"completiontoadjust\" value=" << completionstr << ">\n<P>\n";
  } else VOUT << "<B>No content found!</B>\n<P>\n";
  // text
  if (de->Entry_Text()) {
    String textstr(""),hrefurl,hreftext; int hrefstart,hrefend = 0,hrefnextend; 
    while ((hrefnextend=HTML_get_href(*(de->Entry_Text()),hrefend,hrefurl,hreftext,&hrefstart))>=0) {
      if (hrefstart>hrefend) textstr += de->Entry_Text()->at(hrefend,hrefstart-hrefend);
      hrefurl = absurl(basedir+RELLISTSDIR,hrefurl);
      textstr += HTML_put_href(hrefurl,hreftext);
      hrefend = hrefnextend;
    }
    if (((int) (de->Entry_Text()->length()))>hrefend) textstr += de->Entry_Text()->at(hrefend,de->Entry_Text()->length()-hrefend);
    // Identify possible delegation of task work to a working group (WG).
    // If that is the case then time required represents time required to
    // manage the task that is being carried out by the working group.
    // Note: This is currently embedded in DIL entry text, so that it is
    // not a required variable for all DIL entries.
    int wgidx = -1;
    if ((wgidx=textstr.index("{WG="))>=0) {
      int wgend = -1;
      if ((wgend=textstr.index('}',wgidx))<wgidx) {
	VOUT << "<TR><TD ALIGN=RIGHT>working group = <TD><B>INCOMPLETE DATA</B><TD>&nbsp;<TD>&nbsp;\n<P>\n";
      } else {
	String wg(textstr.at(wgidx+4,wgend-(wgidx+4)));
	VOUT << "<TR><TD ALIGN=RIGHT>working group = <TD><B>" << wg << "</B><TD>(time required is management time)<TD>&nbsp;\n<P>\n";
	textstr.del(wgidx,(wgend+1)-wgidx);
      }
    }
    // *** Could put here: Identify possible relevant contacts specific to this task (RC).
    VOUT << "</TABLE>\n" << textstr << "\n<P>\n";
    //VOUT << "<TEXTAREA name=\"contenttext\" rows=\"20\" cols=\"80\">\n" << *(de->Entry_Text()) << "\n</TEXTAREA>\n<P>\n";
  } else VOUT << "</TABLE>\n";
  // close FORM
  VOUT << "<BR><INPUT type=\"submit\" value=\"Process Content\"> <INPUT type=\"reset\"></FORM>\n";
}

bool testing_semaphore = false;

void generate_modify_element_FORM_interface_DIL_Superiors(DIL_entry * sup, String dilstr, bool isplanoutcome = false, DIL_Superiors * dilsup = NULL) {
  // shows superior data (where available) and provides input tags for data
  // entry
  if (!sup) VOUT << "*WARNING - This does not appear to be a valid reference!*\n";
  else {
    if (sup->Entry_Text()) {
      String suptext(*(sup->Entry_Text()));
      HTML_remove_tags(suptext);
      remove_whitespace(suptext);
      Elipsis_At(suptext,hierarchyexcerptlength);
      VOUT << suptext << '\n';
    } else VOUT << '\n';
  }
  VOUT << "<BR><TABLE>\n<TR><TD>";
  if (isplanoutcome) VOUT << "likelihood<TD>= ";
  else VOUT << "dependency<TD>= "; // was relevance
  if (dilsup) VOUT << String((double) dilsup->relevance,"%.2f ");
  VOUT << "<TD><INPUT type=\"text\" name=\"" << dilstr << ".relevance\" maxlength=5><TD>(0.0 = unspecified, 1.0 = absolute)\n<TR><TD>unbounded importance<TD>= ";
  if (dilsup) VOUT << String((double) dilsup->unbounded,"%.2f ");
  VOUT << "<TD><INPUT type=\"text\" name=\"" << dilstr << ".unbounded\" maxlength=5><TD>(0.0 = unspecified; <B>significance</B> for AL/OOP monitoring/updating)\n<TR><TD>bounded importance<TD>= ";
  if (dilsup) VOUT << String((double) dilsup->bounded,"%.2f ");
  VOUT << "<TD><INPUT type=\"text\" name=\"" << dilstr << ".bounded\" maxlength=5><TD>(0.0 = unspecified)\n<TR><TD>target date<TD>= ";
  // Note: Radio buttons are more simplified than property flags. Only one radiotd* variable can be true at a time,
  // even though exact implies the fixed property.
  bool radiotdvariable = true;
  bool radiotdfixed = false;
#ifdef INCLUDE_EXACT_TARGET_DATES
  bool radiotdexact = false;
#endif
  if (dilsup) {
    radiotdfixed = (dilsup->tdproperty() & DILSUPS_TDPROP_FIXED);
    radiotdvariable = !radiotdfixed;
#ifdef INCLUDE_EXACT_TARGET_DATES
    radiotdexact = dilsup->tdexact();
    if (radiotdexact) { radiotdfixed = false; radiotdvariable = false; }
#endif
    if (dilsup->targetdate()==DILSUPS_TD_UNSPECIFIED) VOUT << "unspecified";
    else VOUT << time_stamp("%Y%m%d%H%M",dilsup->targetdate());
  }
  if (radiotdvariable) VOUT << "<TD><INPUT type=\"text\" name=\"" << dilstr << ".targetdate\" maxlength=12>";
  else VOUT << "<TD>(FIXED)";
  VOUT << "<TD>( -1 = unspecified)\n<TR><TD>TD property<TD>" << HTML_put_form_radio(dilstr+".fixed","fixed","fixed",radiotdfixed);
  VOUT << "<TD>" << HTML_put_form_radio(dilstr+".fixed","variable","variable",radiotdvariable); VOUT.flush();
#ifdef INCLUDE_EXACT_TARGET_DATES
  VOUT << "<TD>" << HTML_put_form_radio(dilstr+".fixed","exact","exact",radiotdexact);
#else
  VOUT << "<TD>&nbsp;";
#endif
#ifdef INCLUDE_EXACT_TARGET_DATES
  bool radiotdperiodic[pt_nonperiodic+1];
  for (unsigned int i=0; i<=pt_nonperiodic; i++) radiotdperiodic[i] = false;
  if (dilsup) {
    radiotdperiodic[pt_nonperiodic] = !(dilsup->tdproperty() & DILSUPS_TDPROP_PERIODIC);
    if (!radiotdperiodic[pt_nonperiodic]) radiotdperiodic[dilsup->tdperiod()] = true;
  }
  VOUT << "\n<TR><TD>[ if fixed/exact: ";
  VOUT << "<TD>" << HTML_put_form_radio(dilstr+".period","non-periodic","non-periodic",radiotdperiodic[pt_nonperiodic]);
  VOUT << "<TD COLSPAN=2>every <INPUT type=\"text\" name=\"" << dilstr << ".every\"";
  if (dilsup) VOUT << " value=\"" << dilsup->tdevery() << '"';
  VOUT << " maxlength=4>";
  VOUT << "\n<TR><TD>&nbsp;<TD>" << HTML_put_form_radio(dilstr+".period","daily","days",radiotdperiodic[pt_daily]);
  VOUT << "<TD>" << HTML_put_form_radio(dilstr+".period","workdays","workdays",radiotdperiodic[pt_workdays]);
  VOUT << "<TD>" << HTML_put_form_radio(dilstr+".period","weekly","weeks",radiotdperiodic[pt_weekly]);
  VOUT << "\n<TR><TD>&nbsp;<TD>" << HTML_put_form_radio(dilstr+".period","bi-weekly","fortnights",radiotdperiodic[pt_biweekly]);
  VOUT << "<TD>" << HTML_put_form_radio(dilstr+".period","monthly","months",radiotdperiodic[pt_monthly]);
  VOUT << "<TD>" << HTML_put_form_radio(dilstr+".period","eomoffset","offsets from end of month",radiotdperiodic[pt_endofmonthoffset]);
  VOUT << "\n<TR><TD>&nbsp;<TD>" << HTML_put_form_radio(dilstr+".period","yearly","years",radiotdperiodic[pt_yearly]);
  VOUT << "<TD COLSPAN=2>span <INPUT type=\"text\" name=\"" << dilstr << ".span\"";
  if (dilsup) VOUT << " value=\"" << dilsup->tdspan() << '"';
  VOUT << " maxlength=4> (0=unlimited repetitions)]";
#endif
  VOUT << "\n<TR><TD>computed urgency<TD>= ";
  if (dilsup) VOUT << String((double) dilsup->urgency,"%.2f ");
  VOUT << "<TD><INPUT type=\"text\" name=\"" << dilstr << ".urgency\" maxlength=5><TD>(0.0 = unspecified/effect-non-p, &lt; 0.0 effect-other, &gt; 0 effect-p)\n<TR><TD>computed priority<TD>= ";
  if (dilsup) VOUT << String((double) dilsup->priority,"%.2f ");
  VOUT << "<TD><INPUT type=\"text\" name=\"" << dilstr << ".priority\" maxlength=5><TD>(0.0 = unspecified)\n";
  VOUT << "</TABLE>\n";
}

void generate_modify_element_FORM_interface_DIL_entry_parameters(DIL_entry * de) { //, Detailed_Items_List & dilist) {
  // generates FORM components specific to editing the parameter elements of
  // a DIL entry
  bool isplanoutcome = (de->Is_Plan_Entry()==PLAN_ENTRY_TYPE_OUTCOME);
  VOUT << "<FORM METHOD=\"POST\" ACTION=\"/cgi-bin/dil2al\"><INPUT type=\"hidden\" name=\"dil2al\" value=\"MEi\">\n<P>\n";
  // ID
  VOUT << "<B>DIL ID# " << de->chars() << "</B> Parameters <INPUT type=\"hidden\" name=\"DILID\" value=\"" << de->chars() << "\">\n<P>\n";
  // ***I can add an interface to edit topic information here
  // Current superiors/projects that may be removed
  DIL_Superiors * dilsup = de->Projects(0);
  if (!dilsup) VOUT << "No current superior DIL entry references.\n<P>\n";
  else {
    VOUT << "<U>Current superior DIL ID references</U>:\n<P>\n<OL>\n";
    for (; (dilsup); dilsup = dilsup->Next()) {
      VOUT << "<LI>(remove <INPUT type=\"checkbox\" name=\"suprm__EQ__" << dilsup->al.title << "\">) " << HTML_put_href(dilsup->al.file,dilsup->al.title) << ": ";
      generate_modify_element_FORM_interface_DIL_Superiors(dilsup->Superiorbyid(),dilsup->al.title,isplanoutcome,dilsup);
    }
    VOUT << "</OL>\n<P>\nSelect references above to remove.\n<P>\n";
  }
  VOUT.flush();
  // Possible superiors to add
  String dilrefsupstr;
  if (!read_file_into_String(dilref,dilrefsupstr,false)) VOUT << "No superior DIL entries in " << dilref << " for possible addition.\n<P>\n";
  else {
    VOUT << "<U>DIL IDs referenced in " << dilref << "</U>:\n<P>\n<OL>\n"; VOUT.flush();
    dilrefsupstr.gsub(BigRegex("Current[^f]*file..."),"");
    StringList dilrefsups(dilrefsupstr,'\n'); String supid;
    for (unsigned int i=0; i<dilrefsups.length(); i++) {
      supid = dilrefsups[i].after('#');
      DIL_ID did(supid);
      if (did.valid()) {
	VOUT << "<LI>" << HTML_put_href(dilrefsups[i],supid) << ": "; VOUT.flush();
	testing_semaphore = true;
	generate_modify_element_FORM_interface_DIL_Superiors(de->elbyid(did),supid,isplanoutcome);
	testing_semaphore = false;
      }
    }
    VOUT << "</OL>\n<P>\n<INPUT type=\"checkbox\" name=\"dilrefsups\"> Add these as superior DIL entries.\n<P>\n";VOUT.flush();
  }
  // Link to seek out Objective DIL entries
  VOUT << "Associate an OOP <A HREF=\"file:///cgi-bin/dil2al?dil2al=D&DILIDCHKBX20050307091430.1=extract&processcmd=newhierarchy&maxdepth=8&setpar__EQ__hierarchyexcerptlength=80&setpar=noreversehierarchy&expressions=%40begin%3A%20PLAN%20DIL%20TEMPLATE\">Objective</A>.<P>";
  // For periodic tasks provide the possibility to specify an automatic update
  if (de->tdperiod()!=pt_nonperiodic) VOUT << "Periodic: Automatically update past current date and time <INPUT type=\"checkbox\" name=\"periodicautoupdate\">.<BR>\nPeriodic: Skip one instance, advance to next target date <INPUT type=\"checkbox\" name=\"periodicskipone\">";
  // close FORM
  VOUT << "<BR><INPUT type=\"submit\" value=\"Process Parameters\"> <INPUT type=\"reset\"></FORM>\n";
}

/* ENABLE MULTI-SKIP FORWARD/BACKWARD:
  // For periodic tasks provide the possibility to specify an automatic update
  if (de->tdperiod()!=pt_nonperiodic) VOUT << "Periodic: Automatically update past current date and time <INPUT type=\"checkbox\" name=\"periodicautoupdate\">.<BR>\nPeriodic: Skip <INPUT type=\"text\" name=\"periodicskipnum\" value=\"1\" maxlength=2> instances, advance to next target date <INPUT type=\"checkbox\" name=\"periodicskipone\">";
  // close FORM
 */

bool generate_modify_element_FORM_interface(StringList * qlist) {
// generates an HTML FORM interface that enables modification of elements
// of a DIL entry. The DIL entry is given by a DILID variable in qlist.
  int dilididx = qlist->contains("DILID");
  if (dilididx<0) return false;
  DIL_ID dilid((*qlist)[dilididx].after('='));
  if (!dilid.valid()) return false;
  if (calledbyforminput) VOUT << "</PRE><!-- "; // clean up HTML output (dil2al processing output can still be inspected by looking at the HTML source)
  Detailed_Items_List dilist;
  if (dilist.Get_All_DIL_ID_File_Parameters()<0) return false;
  if (!dilist.list.head()) return false;
  if (dilist.Get_All_Topical_DIL_Parameters(true)<0) return false;
  DIL_entry * de = dilist.list.head()->elbyid(dilid);
  if (!de) return false;
  VOUT << "<!-- comment out process output -->\n";
  generate_modify_element_FORM_interface_DIL_entry_content(de);
  VOUT << "<P>&nbsp;<P>\n<HR>\n<P>\n";
  generate_modify_element_FORM_interface_DIL_entry_parameters(de); //,dilist);
  VOUT << "<P>&nbsp;<P>\n";
  return true;
}

void modify_element_through_FORM_interface_DIL_Superiors(int & dilididx, StringList * qlist, String supid, DIL_Superiors * dilsup) {
  // test if specific parameters of a DIL_Superiors object should be
  // modified and make the modification if possible
  // Note that according to the reference at
  // http://www.w3.org/TR/REC-html40/interact/forms.html#successful-controls
  // an empty text input field may or may not be treated as a successful
  // control by the user agent (e.g. w3m), so that particular user agents
  // or their particular versions may behave differently. For that reason,
  // even if the QUERY_STRING contains some of the parameter tags tested
  // below, there must also be a test for a valid value.
  
  if ((!qlist) || (!dilsup)) return;
  
  bool pautoupdate = false, pskipone = false;
#ifdef INCLUDE_PERIODIC_TASKS
  if (dilsup->tdproperty() & DILSUPS_TDPROP_PERIODIC) {
    pautoupdate = (qlist->contains("periodicautoupdate=")>=0);
    if (!pautoupdate) pskipone = (qlist->contains("periodicskipone=")>=0);
/* ENABLE MULTI-SKIP FORWARD/BACKWARD:
    int pskipnum = 1, pskipnumidx;
    bool validint;
    if ((pskipnumidx=qlist->contains("periodicskipnum="))>=0) {
      pskipnum = get_int((*qlist)[pskipnumidx].after('='),1,&validint);
    }
*/
    if (pskipone || pautoupdate) {
      // advance the target date according to the periodic task's periodicity
      if (verbose) {
	if (pautoupdate) INFO << "Auto-update of periodic task reference.\n";
	else INFO << "Skipping one instance of periodic task reference.\n";
/* ENABLE MULTI-SKIP FORWARD/BACKWARD:
        else VOUT << "Skipping << pskipnum << " instances of periodic task reference.\n";
*/
      }
      time_t tdate = dilsup->targetdate();
      _DEBUG_TIME_STAMP_TIME_CALLER(__PRETTY_FUNCTION__);
      time_t t = time_stamp_time(curtime); // *** Probably correct with noexit=false (20190127).
      periodictask_t p = dilsup->tdperiod();
      int span = dilsup->tdspan(); // 0 means unlimited
      int every = dilsup->tdevery();
      if (pautoupdate) { // update to next occurrence after the current date
	while ((span!=1) && (tdate<t)) { // for unlimited (span<1) or while days left in span (span>1)
	  tdate=Add_to_Date(tdate,p,every); // (see alcomp.cc)
	  if (span>1) span--;
	}
      } else { // skip one occurrence
/* ENABLE MULTI-SKIP FORWARD/BACKWARD:
// NOTE THAT Subtract_from_Date() STILL NEEDS TO BE IMPLEMENTED!
        else VOUT << "Skipping << pskipnum << " instances of periodic task reference.\n";
	while ((span!=1) && (pskipnum!=0) { // for unlimited (span<1) or while days left in span (span>1)
          if (pskipnum>0) {
            tdate=Add_to_Date(tdate,p,every); // (see alcomp.cc)
            if (span>1) span--;
            pskipnum--;
	  } else {
	    tdate=Subtract_from_Date(tdate,-p,every); // (see alcomp.cc)
            if (span>1) span++;
            pskipnum++;
	  }
        }
*/
	if (span!=1) { // for unlimited (span<1) or while days left in span (span>1)
	  tdate=Add_to_Date(tdate,p,every); // (see alcomp.cc)
	  if (span>1) span--;
	}
      }
      if (span==1) { // if a span was reduced to 1 by the update
	p = pt_nonperiodic;
	dilsup->set_tdspan(0); // [***NOTE] Currently updating spans to their last incidence retains no memory of prior "span" setting other than in the task log.
	dilsup->set_tdperiod(p);
	dilsup->set_tdproperty(dilsup->tdproperty() & (~DILSUPS_TDPROP_PERIODIC));
	if (verbose) {
	  INFO << "  " << supid << ".tdspan = " << dilsup->tdspan() << '\n';
	  INFO << "  " << supid;
	  if (dilsup->tdproperty() & DILSUPS_TDPROP_EXACT) INFO << ".tdproperty = fixed | exact\n  ";
	  else INFO << ".tdproperty = fixed\n  ";
	  INFO << supid << ".tdperiod = non-periodic\n";
	}
      }
      dilsup->force_targetdate(tdate);
      if (tdate<_periodicnearesttdate) _periodicnearesttdate = tdate;
      if (verbose) INFO << "  " << supid << ".targetdate = " << time_stamp("%Y%m%d%H%M\n",dilsup->targetdate());
      if (span>1) {
	dilsup->set_tdspan(span);
	if (verbose) INFO << "  " << supid << ".tdspan = " << dilsup->tdspan() << '\n';
      }
    }
  }
#endif
  bool validvalue;
  if ((dilididx=qlist->contains(supid+".relevance="))>=0) {
    dilsup->relevance = get_double((*qlist)[dilididx].after('='),dilsup->relevance,&validvalue);
    if (validvalue && verbose) INFO << "  " << supid << ".relevance = " << String((double) dilsup->relevance,"%.2f\n");
  }
  if ((dilididx=qlist->contains(supid+".unbounded="))>=0) {
    dilsup->unbounded = get_double((*qlist)[dilididx].after('='),dilsup->unbounded,&validvalue);
    if (validvalue && verbose) INFO << "  " << supid << ".unbounded = " << String((double) dilsup->unbounded,"%.2f\n");
  }
  if ((dilididx=qlist->contains(supid+".bounded="))>=0) {
    dilsup->bounded = get_double((*qlist)[dilididx].after('='),dilsup->bounded,&validvalue);
    if (validvalue && verbose) INFO << "  " << supid << ".bounded = " << String((double) dilsup->bounded,"%.2f\n");
  }
  if ((dilididx=qlist->contains(supid+".targetdate="))>=0) {
    String newtargetdate = (*qlist)[dilididx].after('=');
    if ((!newtargetdate.empty()) && (newtargetdate.matches(BRXint))) {
      _DEBUG_TIME_STAMP_TIME_CALLER(__PRETTY_FUNCTION__);
      if (!dilsup->set_targetdate(time_stamp_time(newtargetdate))) INFO << "Unable to modify targetdate, targetdate property=FIXED.\n"; // *** Probably correct with noexit=false (20190127).
      else if (verbose) INFO << "  " << supid << ".targetdate = " << time_stamp("%Y%m%d%H%M\n",dilsup->targetdate());
    }
  }
  // Note below: Fixed and variable are possible without exact, but exact is possible only with fixed.
  if ((dilididx=qlist->contains(supid+".fixed=fixed"))>=0) {
#ifdef INCLUDE_EXACT_TARGET_DATES
    dilsup->set_tdproperty((dilsup->tdproperty() | DILSUPS_TDPROP_FIXED) & (~DILSUPS_TDPROP_EXACT));
    dilsup->set_tdexact(false);
#else
    dilsup->set_tdproperty(DILSUPS_TDPROP_FIXED);
#endif
    if (verbose) INFO << "  " << supid << ".tdproperty = fixed\n";
#ifdef INCLUDE_EXACT_TARGET_DATES
    if (verbose) INFO << "  " << supid << ".tdexact = false\n";
#endif
  }
  if ((dilididx=qlist->contains(supid+".fixed=variable"))>=0) {
#ifdef INCLUDE_EXACT_TARGET_DATES
    dilsup->set_tdproperty(dilsup->tdproperty() & (~DILSUPS_TDPROP_FIXED) & (~DILSUPS_TDPROP_EXACT));
    dilsup->set_tdexact(false);
#else
    dilsup->set_tdproperty(0);
#endif
    if (verbose) INFO << "  " << supid << ".tdproperty = variable\n";
#ifdef INCLUDE_EXACT_TARGET_DATES
    if (verbose) INFO << "  " << supid << ".tdexact = false\n";
#endif
  }
#ifdef INCLUDE_EXACT_TARGET_DATES
  if ((dilididx=qlist->contains(supid+".fixed=exact"))>=0) {
    dilsup->set_tdproperty(dilsup->tdproperty() | DILSUPS_TDPROP_FIXED | DILSUPS_TDPROP_EXACT);
    dilsup->set_tdexact(true);
    if (verbose) INFO << "  " << supid << ".tdproperty = fixed | exact\n  " << supid << ".tdexact = true\n";
  }
  if ((dilididx=qlist->contains(supid+".period="))>=0) {
    String supidperiod((*qlist)[dilididx].after('='));
#ifdef DEBUG_PERIODIC_TASKS
    VOUT << "DEBUG_PERIODIC_TASKS mark 1: tdperiod=" << periodictaskstr[dilsup->tdperiod()] << "(#" << (long) dilsup->tdperiod() << ")\n";
    VOUT << "  QLIST period=" << supidperiod << '\n';
    VOUT << "  QLIST=" << qlist->concatenate(", ") << '\n'; VOUT.flush();
#endif
    if (((dilsup->tdproperty() & DILSUPS_TDPROP_FIXED)==0) && (!supidperiod.matches(periodictaskstr[pt_nonperiodic]))) {
      INFO << "Periodic specification ignored! Periodic tasks currently require 'fixed' or 'exact' target dates.\n";
    } else {
      periodictask_t periodicity = pt_nonperiodic; int i = pt_nonperiodic;
      for (; i>=0; i--) if (supidperiod.matches(periodictaskstr[i])) {
	periodicity = (periodictask_t) i;
	dilsup->set_tdperiod(periodicity);
	break;
      }
      if (i<0) WARN << "  modify_element_through_FORM_interface_DIL_Superiors(): Unknown periodicity (" << supidperiod << ").\nNot modifying periodicity data.\n";
      else {
	if (periodicity!=pt_nonperiodic) {
	  // An every indicator may specify multiples of the periodicity between occurrences.
	  int everyidx = -1;
	  if ((!pautoupdate) && (!pskipone) && ((everyidx=qlist->contains(supid+".every="))>=0)) {
	    String everystr((*qlist)[everyidx].after('='));
	    int everyval = atoi(everystr.chars());
	    if (everyval<1) {
	      WARN << "  INVALID every " << supid << ".every = " << everystr << ", no every<1 not allowed, every unchanged\n";
	    } else dilsup->set_tdevery(everyval);
	  }
	  // A span may specify a limit to the number of periodic cycles.
	  int spanidx = -1;
	  if ((!pautoupdate) && (!pskipone) && ((spanidx=qlist->contains(supid+".span="))>=0)) {
	    String spanstr((*qlist)[spanidx].after('='));
	    int spanval = atoi(spanstr.chars());
	    if (spanval<0) {
	      WARN << "  INVALID span " << supid << ".span = " << spanstr << ", no negative spans allowed, span unchanged\n";
	    } else if (spanval==1) {
	      WARN << "  INVALID span " << supid << ".span = " << spanstr << ", try setting 'non-periodic', span unchanged\n";
	    } else dilsup->set_tdspan(spanval);
	  }/* else {
	    VOUT << "  MISSING span, setting to Unlimited (0)\n";
	    dilsup->set_tdspan(0);
	    }*/
	}
	if (dilsup->tdperiod()!=pt_nonperiodic) dilsup->set_tdproperty(dilsup->tdproperty() | DILSUPS_TDPROP_PERIODIC);
	// Was: if (dilsup->tdperiod()!=pt_nonperiodic) dilsup->set_tdproperty(dilsup->tdproperty() | DILSUPS_TDPROP_FIXED | DILSUPS_TDPROP_EXACT | DILSUPS_TDPROP_PERIODIC);
	if (verbose) {
	  if (dilsup->tdperiod()!=pt_nonperiodic) {
	    if (dilsup->tdproperty() & DILSUPS_TDPROP_EXACT) INFO << "  " << supid << ".tdproperty = fixed | exact | periodic\n  ";
	    else INFO << "  " << supid << ".tdproperty = fixed | periodic\n  ";
	    INFO << supid << ".tdevery = " << dilsup->tdevery() << "\n  "
		 << supid << ".tdperiod = " << periodictaskstr[dilsup->tdperiod()] << "\n  "
		 << supid << ".tdspan = " << dilsup->tdspan() << '\n';
	  }
	}
      }
    }
#ifdef DEBUG_PERIODIC_TASKS
    VOUT << "DEBUG_PERIODIC_TASKS mark 2: tdperiod=" << periodictaskstr[dilsup->tdperiod()] << "(#" << (long) dilsup->tdperiod() << ")\n"; VOUT.flush();
#endif
  }
#endif
  if ((dilididx=qlist->contains(supid+".urgency="))>=0) {
    dilsup->urgency = get_double((*qlist)[dilididx].after('='),dilsup->urgency,&validvalue);
    if (validvalue && verbose) INFO << "  " << supid << ".urgency = " << String((double) dilsup->urgency,"%.2f\n");
  }
  if ((dilididx=qlist->contains(supid+".priority="))>=0) {
    dilsup->priority = get_double((*qlist)[dilididx].after('='),dilsup->priority,&validvalue);
    if (validvalue && verbose) INFO << "  " << supid << ".priority = " << String((double) dilsup->priority,"%.2f\n");
  }
}

bool modify_element_through_FORM_interface(StringList * qlist, bool periodicautoupdate_stopTLchunk) {
// receives information about element modifications in a DIL entry through
// an HTML FORM interface. The DIL entry is given by a DILID variable in qlist.
// See TL#200610270457.1 about periodicautoupdate.
  if (!qlist) return false;
  //for (int i = 0; i<((int) qlist->length()); i++) VOUT << (*qlist)[i] << '\n';
  int dilididx = qlist->contains("DILID");
  if (dilididx<0) return false;
  DIL_ID dilid((*qlist)[dilididx].after('='));
  if (!dilid.valid()) return false;
  // content modifications
  double valuation = 0.0, completion = 0.0, required = 0.0;
  bool updateval = false, updatecomp = false, updatereq = false, setcompleted = false;
  String valstr;
#ifdef INCLUDE_PERIODIC_TASKS
  // detect automatic update of periodic task
  bool allowfastreturn = true;
  if (!periodicautoupdate_stopTLchunk) {
    if (qlist->contains("periodicautoupdate=")>=0) {
      INFO << "Auto-update of periodic task\n";
      completion = 0.0; updatecomp = true;
      allowfastreturn = false;
    } else if (qlist->contains("periodicskipone=")>=0) {
      INFO << "Skip one of periodic task\n";
      completion = 0.0; updatecomp = true;
      allowfastreturn = false;
    }
  }
#endif
  //VOUT << qlist->concatenate(", ") << '\n'; VOUT.flush();
  if ((dilididx=qlist->contains("valuation="))>=0) valuation=get_double((*qlist)[dilididx].after('='),valuation,&updateval); // modify valuation
  if ((dilididx=qlist->contains("completion="))>=0) completion=get_double((*qlist)[dilididx].after('='),completion,&updatecomp); // modify completion ratio
  if (!updatecomp) if ((dilididx=qlist->contains("setcompleted="))>=0) { // set completion ratio to 1.0, adjust time required accordingly
    setcompleted = true;
    required=get_double((*qlist)[dilididx].after('='),required,&updatereq); // get value to set required to if completed is changed to 1.0
    completion=1.0;
    if (updatereq) updatecomp=true;
    else {
      WARN << "Unable to set completed.\n";
      return false;
    }
  }
  if ((dilididx=qlist->contains("required="))>=0) required=get_double((*qlist)[dilididx].after('='),required,&updatereq); // modify time required
  if (!updatereq) updatereq = setcompleted; // in case it was reset by the preceding query
  if (!updatereq) if ((dilididx=qlist->contains("addrequired="))>=0) { // add to time required, adjust completed accordingly
    double addtorequired=get_double((*qlist)[dilididx].after('='),0.0,&updatereq);
    if (updatereq) {
      if ((dilididx=qlist->contains("reqtoaddto="))>=0) required=get_double((*qlist)[dilididx].after('='),required,&updatereq); // get required time to add to
      if (updatereq) {
	if ((dilididx=qlist->contains("completiontoadjust="))>=0) completion=get_double((*qlist)[dilididx].after('='),completion,&updatecomp); // get required time to add to
	if (updatecomp) {
	  double timeworked = required*completion;
	  required += addtorequired;
	  if (required==0.0) {
	    WARN << "Unable to use 'add to required time' to set required time to zero.\n";
	    return false;
	  }
	  completion = timeworked/required;
	} else {
	  updatereq = false;
	  WARN << "Unable to add to required time.\n";
	  return false;
	}
      }
    } // else addrequired was not specified, continue below
  }
  if (updateval || updatecomp || updatereq) {
    if (!update_DIL_entry_elements(dilid.chars(),valuation,updateval,completion,updatecomp,required,updatereq)) {
      WARN << "Unable to modify content elements of DIL entry\n";
      return false;
    }
#ifdef INCLUDE_PERIODIC_TASKS
    if (allowfastreturn) return true;
#else
    return true;
#endif
  }
  // parameters modifications
  Detailed_Items_List dilist; int entrynum;
  DIL2AL_CONDITIONAL_ERROR_RETURN((entrynum=dilist.Get_All_DIL_ID_File_Parameters())<=0,"No DIL entries found in "+idfile+" in modify_element_through_FORM_interface()\n");
  DIL_entry * de;
  DIL2AL_CONDITIONAL_ERROR_RETURN(!(de = dilist.list.head()->elbyid(dilid)),"The DIL entry "+dilid.str()+" does not appear to exist in modify_element_through_FORM_interface()\n");
  PLLRoot<DIL_Superiors> addsups;
  // detect add
  if ((dilididx=qlist->contains("dilrefsups="))>=0) { // add superiors specified in dilref
    String dilrefsupstr;
    if (!read_file_into_String(dilref,dilrefsupstr,false)) VOUT << "No superior DIL entries in " << dilref << " for possible addition.\n";
    else {
      dilrefsupstr.gsub(BigRegex("Current[^f]*file..."),"");
      StringList dilrefsups(dilrefsupstr,'\n'); String supid;

      for (unsigned int i=0; i<dilrefsups.length(); i++) {
	supid = dilrefsups[i].after('#');
	DIL_ID did(supid);
	if (did.valid()) {
	  DIL_entry * sup = dilist.list.head()->elbyid(did);
	  if (sup) {
	    DIL_Superiors * dilsup = new DIL_Superiors(*de);
	    dilsup->al.title = supid;
	    dilsup->al.file = absurl(idfile,'#'+supid);
	    modify_element_through_FORM_interface_DIL_Superiors(dilididx,qlist,supid,dilsup);
	    addsups.link_before(dilsup);
	    continue;
	  }
	}
	WARN << supid << "does not appear to be a valid reference!\n";
      }
    }
  }
  // detect remove
  StringList removesups; int removesupsnum = 0;
  for (StringList * qlistptr = qlist->List_Index(qlist->contains("suprm=")); (qlistptr); ) {
    removesups[removesupsnum] = (*qlistptr)[0].after('=');
    if (removesups[removesupsnum].contains('=')) removesups[removesupsnum] = removesups[removesupsnum].before('=');
    removesupsnum++;
    if (!(qlistptr=qlistptr->get_Next())) break;
    qlistptr = qlistptr->List_Index(qlistptr->contains("suprm="));
  }
  // detect modify
  for (DIL_Superiors * dilsup = de->Projects(0); dilsup; dilsup = dilsup->Next())
    modify_element_through_FORM_interface_DIL_Superiors(dilididx,qlist,dilsup->al.title,dilsup);
  if (!update_DIL_entry_parameter_elements(dilist,dilid,addsups,&removesups,removesupsnum)) {
    WARN << "Unable to modify parameter elements of DIL entry\n";
    return false;
  }
  return true;
}


bool periodic_advance(String dilid, DIL_entry * de, bool periodicautoupdate_stopTLchunk) {
  // *** IMPROVE THIS! If a periodic task is to be updated, but the work that leads to the update takes place more
  // than one periodic interval before the target date then only autoupdate and not skipone should be used.
  // If a periodic task has an exact target date then always autoupdate rather than skipone.
  if ((periodicadvancebyskip) && (!de->tdexact())) {
    StringList qlistA("DILID="+dilid+"&periodicskipone=on",'&');
    _periodicnearesttdate = (MAXTIME_T);
    if (!modify_element_through_FORM_interface(&qlistA,periodicautoupdate_stopTLchunk)) return false;
    _DEBUG_TIME_STAMP_TIME_CALLER(__PRETTY_FUNCTION__);
    time_t t = time_stamp_time(curtime); // *** Probably correct with noexit=false (20190127).
    if (_periodicnearesttdate > t) return true;
  }
  StringList qlistB("DILID="+dilid+"&periodicautoupdate=on",'&');
  return modify_element_through_FORM_interface(&qlistB,periodicautoupdate_stopTLchunk);
}

bool Update_Unlimited_Periodic_Target_Dates() {
  // Finds all of the periodic target date tasks with unlimited periodicity, which have
  // periodic intervals smaller than one year. Sends those to receive a periodicautoupdate.
  // This can be very helpful when updating the AL.
  
  Detailed_Items_List dilist;
  DIL_entry ** dep = dilist.Sort_by_Target_Date(true);
  if (!dep[0]) {
    ERROR << "Update_Unlimited_Periodic_Target_Dates(): Empty DIL.\n";
    return false;
  }
  int deplen = dep[0]->fulllength();
  // For each entry until target dates are at least at the current date
  _DEBUG_TIME_STAMP_TIME_CALLER(__PRETTY_FUNCTION__);
  time_t t = time_stamp_time(curtime); // *** Probably correct with noexit=false (20190127).
  for (int i = 0; (i<deplen) && (dep[i]->Target_Date()<t); i++) {
    // Detect if the entry is periodic with unlimited periodicity and periodicity smaller than one year
    // If completion ratio is smaller than zero, do not update. If completion ratio is 1.0 or larger, then mark non-updated and mention.
    if ((dep[i]->tdperiod()!=pt_nonperiodic) && (dep[i]->tdspan()==0)) {
      if ((dep[i]->Completion_State()>=0.0) && (dep[i]->Completion_State()<1.0)) {
	if (dep[i]->tdperiod()<pt_yearly) {
	  // Have it updated
	  INFO << "\nUpdating unlimited periodic task <A HREF=\"file://"+idfile+'#'+dep[i]->str()+"\">DIL#" << dep[i]->str() << "</A><BR>\n";
	  StringList qlist("DILID="+dep[i]->str()+"&periodicautoupdate=on",'&');
	  if (!modify_element_through_FORM_interface(&qlist)) {
	    delete[] dep;
	    return false;
	  }
	} else {
	  INFO << "\nNot automatically updating <A HREF=\"file://"+idfile+'#'+dep[i]->str()+"\">DIL#" << dep[i]->str() << "</A> as it occurs once a year or less frequently<BR>\n";
	}
      } else {
	if (dep[i]->Completion_State()>=1.0) {
	  INFO << "\nNot automatically updating <A HREF=\"file://"+idfile+'#'+dep[i]->str()+"\">DIL#" << dep[i]->str() << "</A> due to completion ratio >= 1.0 (suggest checking this!)<BR>\n";
	}
      }
    }
  }
  delete[] dep;
  return true;
  // now build the form interface button to this	
}

/* IMPLEMENTATION NOTE FOR TASK TRIAGE (see TL#201106051031.1) - BASED ON Update_Unlimited_Periodic_Target_Dates()

This is very useful whenever regular tasks need to make way to allow important tasks to be done on time.

1.) Add to the CTR form a "Task Triage Date" text entry box.
2.) Add to the CTR form selector boxes for each task that allow the task to be selected for triage.
3.) Based on Update_Unlimited_Periodic_Target_Dates(), for each triaged task:
    a.) Check if the task target date is already greater than the triage date.
    b.) If not, then if the task is a periodic task, use the same procedure as above to skip ahead until the target date is greater than the triage date.
        Or, if the task is not a periodic task then set the target date to the triage date. Do this, even if the task had a fixed target date (this is relatively safe, since you have to manually select tasks to be triaged.)

*/

bool Skip_Unlimited_Periodic_Target_Dates(StringList * exclude) {
  // Finds all of the periodic target date tasks with unlimited periodicity, which have
  // periodic intevals smaller than one year. Uses the (emulated) current Time if provided.
  // Checks to see if the task was marked as excluded from being skipped. Sends those
  // not excluded to receive a periodicautoupdate.
  // This can be very helpful when seeking to resolve high task density in the AL
  // (see TL#201108281520.1).
  // If exclude==NULL then there are no tasks to exclude from being skipped.
  Detailed_Items_List dilist;
  DIL_entry ** dep = dilist.Sort_by_Target_Date(true);
  if (!dep[0]) {
    ERROR << "Skip_Unlimited_Periodic_Target_Dates(): Empty DIL.\n";
    return false;
  }
  int deplen = dep[0]->fulllength();
  // For each entry until target dates are at least at the current date
  _DEBUG_TIME_STAMP_TIME_CALLER(__PRETTY_FUNCTION__);
  time_t t = time_stamp_time(curtime); // *** Probably correct with noexit=false (20190127).
  for (int i = 0; (i<deplen) && (dep[i]->Target_Date()<t); i++) {
    // Detect if the entry is periodic with unlimited periodicity and periodicity smaller than one year
    // If completion ratio is smaller than zero, do not update. If completion ratio is 1.0 or larger, then mark non-updated and mention.
    if ((dep[i]->tdperiod()!=pt_nonperiodic) && (dep[i]->tdspan()==0)) {
      if ((dep[i]->Completion_State()>=0.0) && (dep[i]->Completion_State()<1.0)) {
	if (dep[i]->tdperiod()<pt_yearly) {
	  // Test for presence in the exclude list
	  String depstr(dep[i]->str());
	  if (exclude) if (exclude->iselement(depstr)>=0) {
	      INFO << "Unlimited periodic task <A HREF=\"file://"+idfile+'#'+depstr+"\">DIL#" << depstr << "</A> was explicitly EXCLUDED from skipping<BR>\n";
	      continue;
	    }
	  // Have it updated
	  INFO << "\nSkipping unlimited periodic task <A HREF=\"file://"+idfile+'#'+depstr+"\">DIL#" << depstr << "</A><BR>\n";
	  StringList qlist("DILID="+depstr+"&periodicautoupdate=on",'&');
	  if (!modify_element_through_FORM_interface(&qlist)) {
	    delete[] dep;
	    return false;
	  }
	} else {
	  INFO << "\nNot automatically skipping <A HREF=\"file://"+idfile+'#'+dep[i]->str()+"\">DIL#" << dep[i]->str() << "</A> as it occurs once a year or less frequently<BR>\n";
	}
      } else {
	if (dep[i]->Completion_State()>=1.0) {
	  INFO << "\nNot automatically skipping <A HREF=\"file://"+idfile+'#'+dep[i]->str()+"\">DIL#" << dep[i]->str() << "</A> due to completion ratio >= 1.0 (suggest checking this!)<BR>\n";
	}
      }
    }
  }
  delete[] dep;
  return true;
}

bool update_DIL_entry_elements(String dilid, double valuation, bool updateval, double completion, bool updatecomp, double required, bool updatereq, time_t tdiff, bool periodicautoupdate_stopTLchunk) {
  // update the elements of all instances of dilid, as
  // referenced in the DIL-by-ID file
  // When tdiff>=0, this is used to indicate new time spent on the DIL
  // entry during a task chunk instead of the value in completion.
  // The simplest update uses the difference between current time
  // and the start time of the most recent task log chunk to compute
  // the update. More sophisticated updates apply the rules
  // specified in dil2al.html#20000815153735.1.
  // get DIL references of DIL entry
  // See TL#200610270457.1 about periodicautoupdate.
  
  Detailed_Items_List dilist; int entrynum;
  if ((entrynum=dilist.Get_All_DIL_ID_File_Parameters(&dilid))<=0) {
    ERROR << "update_DIL_entry_completion_ratio(): DIL entry " << dilid << " not found in " << idfile << ".\nContinuing as is.\n";
    return false;
  }
  if (entrynum>1) WARN << "DIL entry with ID " << dilid << " occurs " << entrynum << " times in " << idfile << ".\n";
  DIL_entry * de = dilist.list.head(); int dilidx;
  PLL_LOOP_FORWARD(DIL_Topical_List,de->Topics(0),1) {
    // get DIL file and update completion ratio of entry
    if ((dilidx=get_file_in_list(e->dil.file,dilist.tf,dilist.tfname,dilist.tfnum))<0)
      WARN << "update_DIL_entry_elements(): DIL file " << e->dil.file << " not found. Ccontinuing as is.\n";
    else {
      // find DIL ID
      String & dilstr = dilist.tf[dilidx];
      int dilpos, dilentryend; String cellparams, cellcontent;
      if ((dilpos=dilstr.index(BigRegex("[<]!--[ 	]*dil2al:[ 	]*DIL begin[ 	]*--[>]"),String::SEARCH_END))<0) {
        WARN << "update_DIL_entry_elements(): Missing DIL begin marker in DIL file " << e->dil.file << ".\nContinuing as is.\n";
        continue;
      }
      if ((dilpos=dilstr.index(BigRegex(("[<][Aa][ 	]+[Nn][Aa][Mm][Ee][ 	]*=[ 	]*\""+RX_Search_Safe(dilid))+'"'),dilpos))<0) {
        WARN << "update_DIL_entry_elements(): DIL entry with ID " << dilid << " not found in DIL file " << e->dil.file << ".\nContinuing as is.\n";
        continue;
      }
      if ((dilentryend=HTML_get_table_cell(dilstr,dilpos,cellparams,cellcontent,&dilpos))<0) {
	WARN << "update_DIL_entry_elements(): Missing data at DIL entry with ID " << dilid << " in DIL file " << e->dil.file << ".\nContinuing as is.\n";
	continue;
      }
      if (verbose) INFO << "Setting:\n";
      if (updateval) { // modify valuation
	String vstr(valuation,"_1 %4.2f");
	if (verbose) INFO << "  valuation = " << vstr << '\n';
	cellcontent.gsub(BigRegex("\\(valuation:[^-0-9 	]*\\)[ 	]*\\([0-9]+[.][0-9]+\\|-\\)"),vstr,'_');
      }
      if (updatecomp) { // modify completion ratio
	String crstr;
	if (tdiff>=0) {
	  if (!updatereq) {
	    cellparams = cellcontent.after(BigRegex("time[ 	]+required[^: 	]*[: 	]")); HTML_remove_tags(cellparams);
	    required = atof(cellparams);
	  }
	  crstr = cellparams.after(BigRegex("completion[ 	]+ratio[^: 	]*[: 	]"));
	  completion = atof(crstr);
	  if (completion<0.0) WARN << "update_DIL_entry_elements(): DIL entry with ID " << dilid << " in\nDIL file " << e->dil.file << " has a completion\nstatus with special meaning that cannot be updated. Continuing as is.\n";
	  else {
	    if (required==0.0) WARN << "update_DIL_entry_elements(): No time required indicated for DIL entry\nwith ID " << dilid << " in DIL file " << e->dil.file << ".\nContinuing as is.\n";
	    else completion += (((double) tdiff)/(ceil(required*60.0)*60.0));
	  }
	  if (tltrackperformance) track_performance(dilid,tdiff,completion,required); // update performance file
#ifdef INCLUDE_PERIODIC_TASKS
	  if ((periodicautoupdate_stopTLchunk) && (completion >= 1.0) && (de->tdperiod()!=pt_nonperiodic)) {
	    completion = 0.0;
	    if (!periodic_advance(dilid,de,periodicautoupdate_stopTLchunk)) WARN << "Unable to auto-update periodic task DIL#" << dilid << ".\n";
	  }
#endif
	}
	crstr = String(completion,"_1 %4.2f");
	if (verbose) INFO << "  completion ratio = " << crstr << '\n';
	cellcontent.gsub(BigRegex("\\(completion[ 	]+ratio:[^-0-9 	]*\\)[ 	]*\\([0-9]+[.][0-9]+\\|-\\)"),crstr,'_');
      }
      if (updatereq) { // modify required time
	String rstr(required,"_1 %4.2f");
	if (verbose) INFO << "  required = " << rstr << '\n';
	cellcontent.gsub(BigRegex("\\(time[ 	]+required:[^-0-9 	]*\\)[ 	]*\\([0-9]+[.][0-9]+\\|-\\)"),rstr,'_');
      }
      // paste modified DIL entry into topical DIL file
      String newdilstr(dilstr.before(dilpos));
      newdilstr += HTML_put_table_cell("",cellcontent,true) + dilstr.from(dilentryend);
      // write modified DIL file
      if (!write_file_from_String(e->dil.file,newdilstr,"DIL ``"+e->dil.title+"''")) return false;
    }
  }
#ifdef DEBUG
  INFO << "Completed DIL updates\n";
#endif
  return true;
}

bool update_DIL_entry_parameter_elements(Detailed_Items_List & dilist, DIL_ID dilid, PLLRoot<DIL_Superiors> & addsups, StringList * removesups, int removesupsnum) {
  // update elements of the DIL-by-ID parameters file
  // target date modification is currently a special case, as described in
  // the modify_DIL_entry_target_date() and modify_DIL_group_target_dates()
  // and is therefore not included in this function
  int entrynum = dilist.list.length();
  if (entrynum<1) return false;
  DIL_entry * de = dilist.list.head()->elbyid(dilid);
  if (!de) {
    ERROR << "Detailed_Items_List in update_DIL_entry_parameter_elements(): The DIL entry with ID " << dilid.chars() << "was not found.\n";
    return false;
  }
  // Add superiors referenced in dilref
  if (addsups.head()) {
    // modify project data to add the superiors
    if (!de->parameters) {
      ERROR << "update_DIL_entry_parameter_elements(): DIL entry " << dilid << " parameters not found in " << idfile << ".\n";
      return false;
    }
    if (verbose) INFO << "Adding superior references from " << dilref << ":\n";
    for (DIL_Superiors * dilsup = addsups.head(); dilsup; dilsup = dilsup->Next()) {
      if (!de->parameters->project_append(dilsup->al.file,dilsup->al.title,dilsup->relevance,dilsup->unbounded,dilsup->bounded,dilsup->targetdate(),dilsup->tdproperty(),
#ifdef INCLUDE_EXACT_TARGET_DATES
					  dilsup->tdexact(),dilsup->tdperiod(),dilsup->tdspan(),dilsup->tdevery(),
#endif
					  dilsup->urgency,dilsup->priority)) WARN << "Unable to append a superior reference with ID " << dilsup->al.title << '\n';
      else if (verbose) INFO << "     " << dilsup->al.title << " added\n";
    }
  }
  // Remove superiors indicated in removesups
  if ((removesupsnum>0) && (removesups)) {
    if (verbose) {
      INFO << "Removing superior references:\n";
      for (int i = 0; i<removesupsnum; i++) INFO << "     " << (*removesups)[i] << '\n';
    }
    // modify project data to remove the superiors
    if (!de->Projects(0)) {
      INFO << "The DIL entry with ID " << dilid.chars() << " has no superiors, therefore none can be removed.\n";
      return false;
    }
    for (int i = 0; i<removesupsnum; i++) {
      PLL_LOOP_FORWARD(DIL_Superiors,de->Projects(0),1) if (e->al.title==(*removesups)[i]) {
	e->remove();
	if (verbose) INFO << "     " << (*removesups)[i] << " removed\n";
	break;
      }
    }
  }
  return dilist.Write_to_File();
}

bool modify_DIL_entry_target_date(DIL_ID dilid, time_t tdate, Detailed_Items_List & dilist) {
// modify the target date of dilid in all self- or superior-
// references (dilid can be initialized from a String)
// Target dates are modified locally, without regard to target
// dates of superiors. If no target date was previously given,
// a set of parameters is created. The target date -1 indicates
// that target dates should be set to "?", i.e. unspecified.
// If dilist is empty, Get_All_DIL_ID_File_Parameters() is called.
// Modifications are made only if tdate differs from Target_Date().
// (Note: Use `modify_DIL_group_target_date()' to update the
// target date with minimal disturbances of relationships
// between dependencies and superiors within a group of DIL
// entries.)
// *** perhaps there should be a way to `force' modification if
//     it is used to ensure that the target date is local
  
  // get DIL parameter sets
  int entrynum;
#ifdef DEBUGTIER1
  VOUT << "dil2al: [DEBUGTIER1] Modifying for DIL#" << dilid.chars() << " to tdate=" << tdate << " (" << time_stamp("%Y%m%d%H%M",tdate) << ")\n";
#endif
  if (!dilist.list.head()) entrynum=dilist.Get_All_DIL_ID_File_Parameters();
  else entrynum=dilist.list.length();
  if (entrynum<=0) {
    ERROR << "modify_DIL_entry_target_date(): No DIL entries found in " << idfile << ".\nContinuing as is.\n";
    return false;
  }
#ifdef DEBUGTIER2
  VOUT << "dil2al: [DEBUGTIER2] Modification progress: 1";
#endif
  // get first entry in list
  DIL_entry * de = dilist.list.head();
  // if (!de) return false; // should never happen when entrynum>0
  DIL_entry * did = de->elbyid(dilid);
  if (!did) {
    ERROR << "modify_DIL_entry_target_date(): DIL entry " << dilid << " not found in " << idfile << ".\nContinuing as is.\n";
    return false;
  }
#ifdef DEBUGTIER2
  VOUT << ", 2";
#endif
  if (did->Target_Date()!=tdate) { // modify only if different
    if (!did->parameters) {
      ERROR << "modify_DIL_entry_target_date(): DIL entry " << dilid << " parameters not found in " << idfile << ".\nContinuing as is.\n";
      return false;
    }
#ifdef DEBUGTIER2
    VOUT << ", 3\n";
#endif
    // modify dilid target dates
    if (did->Projects(0)) PLL_LOOP_FORWARD(DIL_AL_List,did->Projects(0),1) {
#ifdef DEBUGTIER2
	VOUT << "e->targetdate = tdate\n";
#endif
	if (!e->set_targetdate(tdate)) WARN << "Unable to modify FIXED target date of ref.#" << dilid << "->#" << e->al.title << '\n';
      } else if (tdate>=0) { // create self-reference for specified target date
      // *** Modify this if the data format changes
#ifdef DEBUGTIER2
      VOUT << "did->parameters->project_append\n";
#endif
      did->parameters->project_append(idfile+String('#')+did->chars(),did->chars(),DILSUPS_REL_UNSPECIFIED,DILSUPS_UNB_UNSPECIFIED,DILSUPS_BND_UNSPECIFIED,tdate,0,
#ifdef INCLUDE_EXACT_TARGET_DATES
				      false,pt_nonperiodic,0,1,
#endif
				      DILSUPS_URG_UNSPECIFIED,DILSUPS_PRI_UNSPECIFIED);
    }
#ifdef DEBUGTIER1
    VOUT << "dil2al: [DEBUGTIER1] Modification performed\n";
#endif
  }
#ifdef DEBUGTIER2
  else VOUT << ", target date already correct\n";
#endif
#ifdef DEBUG
  EOUT << "Modified DIL entry target date\n";
#endif
  return true;
}

bool modify_DIL_entry_target_date(DIL_ID dilid, time_t tdate) {
// modify the target date of dilid in all self- or superior-
// references (dilid can be initialized from a String) and
// write to DIL ID file
// Target dates are modified locally, without regard to target
// dates of superiors. If no target date was previously given,
// a set of parameters is created. The target date -1 indicates
// that target dates should be set to "?", i.e. unspecified.
// Modifications are made only if tdate differs from Target_Date().
// (Note: Use `modify_DIL_group_target_date()' to update the
// target date with minimal disturbances of relationships
// between dependencies and superiors within a group of DIL
// entries.)

  // modify dilid target dates
  Detailed_Items_List dilist;
  if (!modify_DIL_entry_target_date(dilid,tdate,dilist)) return false;
  // place new data and copy rest
#ifdef VERBOSEDEBUG
  PLL_LOOP_FORWARD(DIL_entry,dilist.list.head(),1) {
    EOUT << e->chars();
    if (e->Projects(0)) EOUT << '\t' << e->Projects(0)->targetdate();
    EOUT << '\n';
  }
#endif
#ifdef DEBUG
  bool res = dilist.Write_to_File();
  if (res) EOUT << "Modified DIL entry target date and stored to DIL ID file\n";
  return res;
#else
  return dilist.Write_to_File();
#endif
}

bool modify_DIL_group_target_dates(Detailed_Items_List & dilgroup, bool minfragmentation) {
// Adjust the target dates of DIL entries listed in dilgroup,
// utilizing propagation rules to minimize the number of
// locally specified target dates where superiors can be
// modified and are members of the group. Desired target dates
// are specified in dilgroup.list[i]->Projects(0)->_targetdate,
// so that they can be obtained through dilgroup.list[i]->Target_Date().
// A target date is not changed if it is the same as the
// entry's current Target_Date() or if -2 was specified.
// A target date specification of -1 implies that it will be
// set to `DILSUPS_TD_UNSPECIFIED'.
#ifdef DEBUG_REPORT_DEVIATIONS
  int tdmodifdeviations = 0;
#endif
  Detailed_Items_List dilist; DIL_entry * did = NULL; time_t tdate;
#ifdef DEBUGTIER2
  time_t * oldtargetdate;
  if (!dilist.list.head()) dilist.Get_All_DIL_ID_File_Parameters();
  oldtargetdate = new time_t[dilist.list.length()];
  for (int i=0; i<dilist.list.length(); i++) oldtargetdate[i] = dilist.list.el(i)->Local_Target_Date(0);
#endif
  if (minfragmentation) { // apply minimization rules
#ifdef DEBUGTIER2
    VOUT << "dil2al: [DEBUGTIER2] Applying rules to minimize fragmentation\n";
#endif
    int entrynum=dilist.Get_All_DIL_ID_File_Parameters();
    if (entrynum<=0) {
      WARN << "modify_DIL_group_target_dates(): No DIL entries found in " << idfile << ".\nContinuing as is.\n";
      return false;
    }
    DIL_entry * de = dilist.list.head();
    // sort dilgroup in reverse order of desired target dates
    // - thus superiors with the latest target date are updated first (minimizes insulations)
    DIL_entry ** e = dilgroup.Sort_by_Target_Date(); // traverse in reverse order, highest to lowest
#define MODIFY_DIL_GROUP_TARGET_DATES_RETURN_WITH_CLEANUP(res) delete[] e; return res
    for (int i = dilgroup.list.length()-1; i>=0; i--) if ((tdate=e[i]->Local_Target_Date(0))>=-1) { // iterate through dilgroup members
      did = de->elbyid(*(e[i]));
      if (!did) {
	WARN << "modify_DIL_group_target_dates(): DIL entry " << e[i]->chars() << " not found in " << idfile << ".\nContinuing as is.\n";
	MODIFY_DIL_GROUP_TARGET_DATES_RETURN_WITH_CLEANUP(false);
      }
      if (did->Projects(0)) PLL_LOOP_FORWARD_NESTED(DIL_Superiors,did->Projects(0),1,pe) { // iterate through corresponding dilist member superiors
	// Note: Do this even if did->Target_Date()==tdate to insure that
	//       did->Target_Date()==tdate will also be true after the
	//       target dates of superiors may have been modified.
	// *** can set local target date if pe refers to self
	//     once superior references are pointer parameters
	// set `local' target date specifications to desired target date
	if (pe->targetdate()>=0) { 
	  if (!pe->set_targetdate(tdate)) VOUT << "Unable to modify FIXED target date of ref.#" << e[i]->chars() << "->#" << pe->al.title << '\n';
	} else if (tdate>=0) { // if original TD is propagated from superior
	  if (pe->IsSelfReference()) {
	    WARN << "modify_DIL_group_target_dates(): Correcting self-reference without local target date.\n";
	    if (!pe->set_targetdate(tdate)) WARN << "modify_DIL_group_target_dates(): Unable to modify FIXED target date of ref.#" << e[i]->chars() << "->#" << pe->al.title << ".\n";
	  } else {
	    // 1 - find the new target date that this superior is to be set to
	    DIL_ID peid(pe->al.title);
	    DIL_entry * peg = dilgroup.list.head()->elbyid(peid); // can't use Superiorbyid() here, since getting in dilgroup
	    time_t petdate = -1;
	    if (peg) petdate = peg->Local_Target_Date(0); // to insure that code -1 is detected
	    // 2 - does this superior take care of the intended target date?
	    DIL_entry * sup = pe->Superiorbyid();
	    if (!sup) {
	      WARN << "modify_DIL_group_target_dates(): DIL entry superior with ID #" << pe->al.title << " not found.\nContinuing as is.\n";
	      continue;
	    }
	    // if superior pe of e is in dilgroup with same desired TD
	    // then pe will take care of update (beware of loops)
	    if (petdate==tdate) continue;
	    // 3 - to avoid propagation of earlier TD, set locally
	    // if pe->Target_Date() is earlier than desired
	    // then set desired locally (don't alter superior, since
	    // that could affect other dependencies of that superior)
	    if (sup->Target_Date()<tdate) { // insulate from superior's target date
	      if (!pe->set_targetdate(tdate)) WARN << "modify_DIL_group_target_dates(): Unable to modify FIXED target date of ref.#" << e[i]->chars() << "->#" << pe->al.title << ".\n";
	    } else {
	      // 4 - if the dependency is highly relevant, completing the
	      //     superior earlier does not make sense
	      // if pe in dilgroup and pe desired < e desired
	      // and relevance not 1.0 or unspecified then set locally
	      // else warn incompatibility for fully relevant dependency
	      if ((peg) && (petdate<tdate)) {
		if ((pe->relevance<1.0) && (pe->relevance!=DILSUPS_REL_UNSPECIFIED)) {
		  // set locally, because superior will be set to earlier
		  if (!pe->set_targetdate(tdate)) WARN << "modify_DIL_group_target_dates(): Unable to modify FIXED target date of ref.#" << e[i]->chars() << "->#" << pe->al.title << ".\n";
		  continue;
		} else if (petdate>=0) WARN << "modify_DIL_group_target_dates(): Earlier target date requested for superior with ID #" << pe->al.title << " than for dependency with ID # " << e[i]->chars() << ". Continuing.\n";
	      }
	      // 5 - if superior is not being changed or petdate>tdate then
	      //     if propagated TD is still too late then set it locally
	      // if pe is last superior and Target_Date() is still
	      // > desired then set locally
	      if (!pe->Next()) if (did->Target_Date()>tdate) {
		if (!pe->set_targetdate(tdate)) WARN << "modify_DIL_group_target_dates(): Unable to modify FIXED target date of ref.#" << e[i]->chars() << "->#" << pe->al.title << ".\n";
	      }
	    }
	  }
	}
      } else if (tdate>=0) { // unless setting to unspecified, make a self-reference
	// *** Modify this if the data format changes
	if (!did->parameters) did->parameters = new DIL_entry_parameters(*did);
	did->parameters->project_append(idfile+'#'+did->chars(),did->chars(),DILSUPS_REL_UNSPECIFIED,DILSUPS_UNB_UNSPECIFIED,DILSUPS_BND_UNSPECIFIED,tdate,0,
#ifdef INCLUDE_EXACT_TARGET_DATES
					false,pt_nonperiodic,0,1,
#endif
					DILSUPS_URG_UNSPECIFIED,DILSUPS_PRI_UNSPECIFIED);
      }
    }
    // Double-check to insure that target dates are detected as requested
    for (int i = dilgroup.list.length()-1; i>=0; i--) if ((tdate=e[i]->Local_Target_Date(0))>=0) { // iterate through dilgroup members
      did = de->elbyid(*(e[i]));
      if (did->Target_Date()!=tdate) {
#ifdef DEBUG_REPORT_DEVIATIONS
	tdmodifdeviations++; // useful when testing functional or rule-based approach above
#endif
	if (did->Projects(0)) {
	  if (did->Target_Date()>tdate) {
	    if (!did->Projects(0)->set_targetdate(tdate)) WARN << "modify_DIL_group_target_dates(): Unable to modify FIXED target date of ref.#" << e[i]->chars() << "->#" << did->Projects(0)->al.title << ".\n";
	    else WARN << "Forcing target date adjustment at DIL entry #" << did->chars() << " superior reference " << did->Projects(0)->al.title << '\n';
	  } else PLL_LOOP_FORWARD_NESTED(DIL_Superiors,did->Projects(0),1,pe) { // iterate through corresponding dilist member superiors
	    if (pe->targetdate() < tdate) {
	      if (!pe->set_targetdate(tdate)) WARN << "modify_DIL_group_target_dates(): Unable to modify FIXED target date of ref.#" << e[i]->chars() << "->#" << pe->al.title << ".\n";
	      else WARN << "Forcing target date adjustment at DIL entry #" << did->chars() << " superior reference " << pe->al.title << ".\n";
	    }
	  }
	}
      }
    }
    // Final test of detected target dates achieved
    for (int i = dilgroup.list.length()-1; i>=0; i--) if ((tdate=e[i]->Local_Target_Date(0))>=0) // iterate through dilgroup members
      if (de->elbyid(*(e[i]))->Target_Date()!=tdate) WARN << "modify_DIL_group_target_dates(): Unable to set target date for DIL#" << did->chars() << ".\nContinuing as is.\n";
    delete[] e;
  } else PLL_LOOP_FORWARD(DIL_entry,dilgroup.list.head(),1) { // modify target dates directly
    tdate = e->Local_Target_Date(0); // not using Target_Date() to insure -1 and -2 codes are detected
    if (tdate>=-1) if (!modify_DIL_entry_target_date(*e,tdate,dilist)) return false;
  }
#ifdef DEBUGTIER2
  int i = 0;
  PLL_LOOP_FORWARD(DIL_entry,dilist.list.head(),1) {
    if (e->Local_Target_Date(0)!=oldtargetdate[i]) cout << "dil2al: [DEBUGTIER2] Changed DIL entry #" << e->chars() << ", old=" << oldtargetdate[i] << " (" << time_stamp("%Y%m%d%H%M",oldtargetdate[i]) << "), new=" << e->Local_Target_Date(0) << " (" << time_stamp("%Y%m%d%H%M",e->Local_Target_Date(0)) << ") TD=" << time_stamp("%Y%m%d%H%M",e->Target_Date()) << '\n';
    i++;
  }
  delete[] oldtargetdate;
#endif
#ifdef DEBUG_REPORT_DEVIATIONS
  EOUT << "dil2al: [DEBUG_REPORT_DEVIATIONS] Target Date Modification Deviations = " << tdmodifdeviations << '\n';
#endif
#ifdef DEBUG
  bool res = dilist.Write_to_File();
  if (res) EOUT << "Modified DIL group target dates and stored to DIL ID file\n";
  return res;
#else
  return dilist.Write_to_File();
#endif
/*
An alternative approach
-----------------------
Note that the concept below needs to be worked out for the case of muiltiple
superiors at each level, i.e. when tracking through the superiors structure,
all path possibilities must be marked/remembered, instead of just using pe as
a roadblock. Due to this, and the depth-search itself, it is unclear if this
functional approach is more or less efficient than the rule-based one. It also
remains to be seen if this approach follows all the same rules as the rule-based
approach.

From furthest TD to nearest TD in list of DIL entries:

isgroupsuperior()
	if (!pe) return false;
	e = dilgroup.list.head()->elbyid(*pe);
	if (!e) return false;
	if (e->Target_Date()!=tdate) return false;
	return true;

while Target_Date()!=tdate
	pe = NULL
	while (!isgroupsuperior(pe)) pe = Target_Date_Specified(pe);
	if (pe->Projects(0)) pe->Projects(0)->_targetdate = tdate;
	else pe->parameters->add_project(...,tdate,...);
*/
}

bool modify_DIL_group_target_dates_cmd(bool minfragmentation, StringList * qlist) {
  // Command line interface to modify_DIL_group_target_dates().
  // Reads pairs of DIL entry IDs and desired target dates (with
  // optional -2 code to indicate no target date modification, and
  // -1 to unspecify local target date parameters).
  // Data is read from the DIN stream.
  // If qlist is not NULL then HTML form data is obtained from qlist.
  
  const BigRegex td("^TD\\([0-9]+[.][0-9]+\\)=");
  const BigRegex tdchkbx("^TDCHKBX\\([0-9]+[.][0-9]+\\)=");
  const BigRegex skipchkbx("^SKIPCHKBX\\([0-9]+[.][0-9]+\\)=");
  const BigRegex pairsep("[ \t,:;]+");
  const BigRegex nonnumerals("[^-0-9]");
  String modifdata;
  if (qlist) { // HTML form interface
    if (qlist->contains("skip=Skip")>=0) { // Actually a call to Skip_Unlimited_Periodic_Target_Dates()
      StringList exclude; int excludenum = 0;
      int qnum = qlist->length()-1; // *** empty element at end of StringList
      for (int i = 0; i<qnum; i++) {
	if ((*qlist)[i].contains(skipchkbx)) {
	  String chkbxid = (*qlist)[i].at(skipchkbx.subpos(1),skipchkbx.sublen(1));
	  String tdstr = (*qlist)[i].after('=');
	  if (tdstr=="noskip") { // collect as non-skippable
	    exclude[excludenum] = chkbxid;
	    excludenum++;
	  }
	}
      }
      return Skip_Unlimited_Periodic_Target_Dates(&exclude);
    } else {
      StringList exclude; int excludenum = 0;
      int qnum = qlist->length()-1; // *** empty element at end of StringList
      for (int i = 0; i<qnum; i++) {
	// *** Beware: This code assumes that DIL entry IDs do not
	//     occur as subsets of other DIL entry IDs, i.e. it
	//     assumes that a move to IDs with more digits would imply
	//     prepending older IDs with 0s. This could of course be
	//     handled by a preprocessing loop.
#ifdef DEBUGTIER2
	cout << '$' << (*qlist)[i] << '\n';
#endif
	if ((*qlist)[i].contains("TDupdate=")) {
	  if ((*qlist)[i].after('=')=="direct") minfragmentation = false;
	  else minfragmentation = true;
	} else if ((*qlist)[i].contains(td)) {
	  modifdata += (*qlist)[i].at(td.subpos(1),td.sublen(1)) + ' ';
	  String tdstr = (*qlist)[i].after('='); if (tdstr.length()==0) tdstr = "-2";
	  modifdata += tdstr + '\n';
	} else if ((*qlist)[i].contains(tdchkbx)) {
	  String chkbxid = (*qlist)[i].at(tdchkbx.subpos(1),tdchkbx.sublen(1));
	  String tdstr = (*qlist)[i].after('=');
	  if (tdstr=="noupdate") { // set corresponding desired target date to code -2
	    exclude[excludenum] = chkbxid;
	    excludenum++;
	  }
	}
      }
      for (int i = 0; i<excludenum; i++) modifdata.gsub(BigRegex(RX_Search_Safe(exclude[i])+" [-0-9]*"),exclude[i]+" -2");
    }
  } else {
    // *** This should be handled by a UI_ equivalent, where the UI_CLASSIC version
    //     probably does exactly what is below up to and including the
    //     read_data_from_stream() call.
    //     This is hardly ever used though, so it is momentarily a waste of time to
    //     update the implementation.
    if (useansi) VOUT << ANSI_BOLD_ON;
    VOUT << "Enter group of DIL entries to consider for target date modification.\n";
    if (useansi) VOUT << ANSI_BOLD_OFF;
    VOUT << "Rows of DIL entry ID and desired target date pairs.\n\
Special desired target date codes:\n					\
-1 = set to `unspecified', -2 = no desired target date (but modifiable).\n\
Separate pairs with any number of spaces, tabs, commas, colons or semicolons.\n	\
End of File (Ctrl+D) or a line stating `EOF' completes data entry.\n\n";
    if (useansi) VOUT << ANSI_UNDERLINE_ON;
    VOUT << "DIL entry ID#\tdesired target date\n";
    if (useansi) VOUT << ANSI_UNDERLINE_OFF;
    read_data_from_stream(DIN,modifdata,"EOF");
  }
  StringList modiflist(modifdata,'\n');
  int groupsize = modiflist.length()-1; // *** last entry empty when initializing from string with separator
  Detailed_Items_List dilgroup; DIL_entry * de; String did; time_t tdate;
#ifdef DEBUGTIER2
  cout << "Content-Type: text/plain\n\ndil2al: [DEBUGTIER2]\n";
  for (int j=0; j<groupsize; j++) {
    String & s = modiflist[j];
    cout << s << '\n';
  }
  cout << "+++\n";
#endif
  for (int i=0; i<groupsize; i++) {
    did = modiflist[i].after(pairsep); if (did.index(nonnumerals)>=0) did = did.before(nonnumerals);
    _DEBUG_TIME_STAMP_TIME_CALLER(__PRETTY_FUNCTION__);
    tdate = time_stamp_time(did); // *** Probably correct with noexit=false (20190127).
    did = modiflist[i].before(pairsep);
    de = new DIL_entry(dilgroup,did);
    de->parameters = new DIL_entry_parameters(*de);
    de->parameters->project_append('#'+did,did,DILSUPS_REL_UNSPECIFIED,DILSUPS_UNB_UNSPECIFIED,DILSUPS_BND_UNSPECIFIED,tdate,0,
#ifdef INCLUDE_EXACT_TARGET_DATES
				   false,pt_nonperiodic,0,1,
#endif
				   DILSUPS_URG_UNSPECIFIED,DILSUPS_PRI_UNSPECIFIED);
    dilgroup.list.link_before(de);
  }
#ifdef DEBUGTIER1
  if (minfragmentation) cout << "dil2al: [DEBUGTIER1] Minimum fragmentation\n"; else cout << "dil2al: [DEBUGTIER1] Direct\n";
#endif
#ifdef DEBUGTIER2
  for (int i=0; i<groupsize; i++) {
    cout << "dil2al: [DEBUGTIER2] " << dilgroup.list.el(i)->chars() << '\t';
    if (dilgroup.list.el(i)->Local_Target_Date(0)<0) cout << dilgroup.list.el(i)->Local_Target_Date(0) << '\n';
    else cout << time_stamp("%Y%m%d%H%M",dilgroup.list.el(i)->Local_Target_Date(0)) << '\n';
  }
#endif
  return modify_DIL_group_target_dates(dilgroup,minfragmentation);
}

bool extract_DIL_IDs_from_form(StringList * qlist) {
  // Form interface that extracts DIL IDs that correspond to checked
  // checkboxes in any form.
  // Known variable names in form: dil2al, DILIDCHKBX<DIL_ID>, processcmd,
  // maxdepth, hierarchyfile.
  // Returns the DIL IDs to INFO and also writes them to the dilref file.
  //
  // NOTE: In practice, this function is used only in select places, such
  // as in the hierarchy form (hierarchy.html). The w3minterface script
  // has its own implementation.
  
  if (!qlist) return false;
  
  const BigRegex dilidchkbx("^DILIDCHKBX\\([0-9]+[.][0-9]+\\)=");
  StringList dilids; int dilidsnum = 0;
  
  // Find checked checkboxes that contain DIL IDs
  int qnum = qlist->length()-1; // *** empty element at end of StringList
  for (int i = 0; i<qnum; i++) {
    // *** Beware: The same caveat about the assumed length of DIL IDs applies
    //     as in the modify_DIL_group_target_dates_cmd() function above.
#ifdef DEBUGTIER2
    cout << '$' << (*qlist)[i] << '\n';
#endif
    if ((*qlist)[i].contains(dilidchkbx)) {
      String chkbxid = (*qlist)[i].at(dilidchkbx.subpos(1),dilidchkbx.sublen(1));
      String chkbxval = (*qlist)[i].after('=');
      if (chkbxval=="extract") {
	dilids[dilidsnum] = chkbxid;
	dilidsnum++;
      }
    }
  }
  
  // Check if extract or make new hierarchy
  int processcmdidx = qlist->contains("processcmd");
  if (processcmdidx>0) if (String((*qlist)[processcmdidx].after('='))=="newhierarchy") {
    int depthidx = qlist->contains("maxdepth"), maxdepth = 50;
    if (depthidx>0) maxdepth = atol((const char *) String((*qlist)[depthidx].after('=')));
    int hierarchyfileidx = qlist->contains("hierarchyfile"); String hierarchyfile("");
    if (hierarchyfileidx>0) hierarchyfile = '@'+String((*qlist)[hierarchyfileidx].after('='))+'@';
    int expressionsidx = qlist->contains("expressions"); String expressions("");
    if (expressionsidx>0) expressions = '#'+URI_unescape(String((*qlist)[expressionsidx].after('=')));
    int contentfileidx = qlist->contains("contentfile"); String contentfile("");
    if (contentfileidx>0) contentfile = 'c'+String((*qlist)[contentfileidx].after('='));
    if (dotgraph) return DIL_Hierarchy_cmd(hierarchyfile+"GRAPH"+dilids.concatenate('+')+contentfile+expressions,maxdepth);
    else return DIL_Hierarchy_cmd(hierarchyfile+"FORM"+dilids.concatenate('+')+contentfile+expressions,maxdepth);
  }
  
  // Write the DIL IDs to the dilref file
  if (dilidsnum==0) return false;
  INFO << dilids.concatenate("\n"); // VOUT, because this is meant to be output on stdout.
  
  String dilidsstr;
  for (int i = 0; i<dilidsnum; i++) dilidsstr += "Current URL      file://"+idfile+'#'+dilids[i]+'\n';
  return write_file_from_String(dilref,dilidsstr,"Checked DIL IDs");
}

void Show_DIL_Hierarchy(DIL_entry * topde, DIL_Visualize & dv, const int maxdepth, int depth, int width, DIL_entry * prede, double likelihood) {
  // Create a visualizable representation of a DIL hierarchy from a DIL entry.
  // 'topde' must be part of a valid Detailed_Items_List or at least part
  // of a valid PLLHandle<DIL_entry> list.
  // 'prede' contains a pointer to the calling DIL entry or NULL at the top.
  // The visualizable structure is collected in 'dv', which can be overloaded
  // to cater to special requirements (e.g. of FORM and GRAPH types).
  
#ifdef DEBUG_PLAN_ENTRY
  EOUT << "Show " << topde->chars() << '\n'; EOUT.flush();
#endif
  DIL_Hierarchy_Level_Data ld(*topde,likelihood);
  dv.ldroot.link_before(&ld); // link into list of level data in this depth first process
  dv.Attach_Level_Data(ld);
  // Visualize data for this node
  bool recurse = dv.Visualize_Element(topde,depth,width,prede);
  dv.Detach_Level_Data();
  depth++;
  int numdependencies = 0; double thislevelcumlikelihood = dv.get_cumlikelihood();
  // Look for connected nodes
  if (recurse) {
    if (hierarchysorting) {
      DIL_entry_ptr * dep = 0; int n = 0;
      if (reversehierarchy) {
	DIL_AL_List * topdeproj = topde->Projects(0);
	if (topdeproj) dep = new DIL_entry_ptr[topdeproj->length()]; 
	if (topdeproj) PLL_LOOP_FORWARD(DIL_AL_List,topdeproj,1) if (!(e->Superiorbyid()==topde)) {
	    if (maxdepth>=depth) dep[n++] = e->Superiorbyid();
	    else numdependencies++;
	  }
      } else {
	if (topde->head()) dep = new DIL_entry_ptr[topde->head()->length()]; 
	PLL_LOOP_FORWARD(DIL_entry,topde->head(),1) if (e->Is_Direct_Dependency_Of(topde)) {
	  if (maxdepth>=depth) dep[n++] = e;
	  else numdependencies++;
	}
      }
      if (n>0) {
	qsort(dep,n,sizeof(DIL_entry_ptr),DIL_entry_target_date_qsort_compare);
	for (int i=0; i<n; i++) {
	  // [***INCOMPLETE] This implementation does not take into account
	  // e->PLAN_OUTCOME_LIKELIHOOD when hierarchysorting is true.
	  dv.set_cumlikelihood(thislevelcumlikelihood);
	  Show_DIL_Hierarchy(dep[i],dv,maxdepth,depth,i,topde);
	}
      }
      delete[] dep;
    } else {
      int width = 0;
      if (reversehierarchy) {
	DIL_AL_List * topdeproj = topde->Projects(0);
	if (topdeproj) PLL_LOOP_FORWARD(DIL_AL_List,topdeproj,1) if (!(e->Superiorbyid()==topde)) {
	    if (maxdepth>=depth) {
	      dv.set_cumlikelihood(thislevelcumlikelihood);
	      Show_DIL_Hierarchy(e->Superiorbyid(),dv,maxdepth,depth,width,topde,e->PLAN_OUTCOME_LIKELIHOOD);
	      width++;
	    } else numdependencies++;
	  }
      } else { // Regular, unsorted, top-down
	// Always goes through all dependencies (within depth and width limits) and
	// heads down witn 'e' as the new node and 'topde' as the current one (e.g. the
	// 'e' from the level above). All decisions on what is actually visualized or
	// not are done within dv.Visualize_Element().
	PLL_LOOP_FORWARD(DIL_entry,topde->head(),1) if (e->Is_Direct_Dependency_Of(topde)) {
	  if (maxdepth>=depth) {
	    Show_DIL_Hierarchy(e,dv,maxdepth,depth,width,topde); // Depth-first search
	    width++;
	  } else numdependencies++;
	}
      }
      dv.Visualize_BeyondWidth(width,topde);
    }
  }
  // the following is used to convey benefit/risk information for PLAN
  // entries and if maxdepth was reached and there are further dependencies
  dv.Attach_Level_Data(ld);
  dv.Visualize_NotShown(numdependencies,depth,width,topde);
  dv.Detach_Level_Data();
  // ld is automatically removed from list of level data in this depth first process
}

String Tabbed_DIL_Hierarchy(String dilidstr, int maxdepth) {
  // visualize a DIL hierarchy starting at dilid with tabs indicating depth
  
  DIL_Visualize_with_Tabs dv;
  // 3. Get optional expressions that must occur in DIL entry content text
  String expressions(dilidstr.after('#'));
  if (!expressions.empty()) dilidstr = dilidstr.before('#');
  // 4. Get optional ContentFile reference
  String contentfile(dilidstr.after('c'));
  if (!contentfile.empty()) dilidstr = dilidstr.before('c');
  DIL_ID dilid(dilidstr);
  if (!dilid.valid()) return String("");
  Detailed_Items_List dilist;
  dilist.Get_All_DIL_ID_File_Parameters();
  if (!contentfile.empty()) dilist.Get_All_Topical_DIL_Parameters(true);
  if (dilist.list.head()) Show_DIL_Hierarchy(dilist.list.head()->elbyid(dilid),dv,maxdepth);
  if (!contentfile.empty()) if (!write_file_from_String(contentfile,dv.Output_Content(),"DIL Hierarchy Content")) WARN << "Tabbed_DIL_Hierarchy(): Unable to write " << contentfile << ".\nContinuing as is.\n";
  
  return dv.Output();
}

String Tabbed_HTML_DIL_Hierarchy(String dilidstr, int maxdepth) {
  // visualize a DIL hierarchy starting at dilid with tabs indicating depth
  // including HREFs and other HTML code, while HTML head and tail code are
  // left to the content in which the extracted hierarchy is embedded
  // Note that since standard output does not know which file the text may
  // be embedded in, HREF file references are absolute. Post-processing may
  // convert those to relative references.
  
	DIL_Visualize_with_HTML_Tabs dv;
  	// 3. Get optional expressions that must occur in DIL entry content text
  	String expressions(dilidstr.after('#'));
  	if (!expressions.empty()) dilidstr = dilidstr.before('#');
  	// 4. Get optional ContentFile reference
	String contentfile(dilidstr.after('c'));
	if (!contentfile.empty()) dilidstr = dilidstr.before('c');
	DIL_ID dilid(dilidstr);
	if (!dilid.valid()) return String("");
	Detailed_Items_List dilist;
	dilist.Get_All_DIL_ID_File_Parameters();
	if (!contentfile.empty()) dilist.Get_All_Topical_DIL_Parameters(true);
	if (dilist.list.head()) Show_DIL_Hierarchy(dilist.list.head()->elbyid(dilid),dv,maxdepth);
	if (!contentfile.empty()) if (!write_file_from_String(contentfile,dv.Output_Content(),"DIL Hierarchy Content")) WARN << "Tabbed_HTML_DIL_Hierarchy(): Unable to write " << contentfile << ".\nContinuing as is.\n";
	
	return "<PRE>\n"+dv.Output()+"</PRE>\n";
}

void Tabbed_FORM_DIL_Hierarchy_full(Detailed_Items_List & dilist, DIL_Visualize_with_FORM_Tabs & dv, const int maxdepth, String & expressions) {

  // Show full hierarchy with possible restriction to roots containing certain expressions
  StringList expressionlist(expressions,'+');
  int expressionnum = expressionlist.length()-1; // since an empty extra item is initialized in the StringList
  for (int i = 0; i<expressionnum; i++) expressionlist[i] = URI_unescape(expressionlist[i]);
#ifdef DEBUG_HIERARCHY_EXPRESSIONS
  for (int i = 0; i<expressionnum; i++) VOUT << "&&& expressionlist[" << i << "] = " << expressionlist[i] << '\n';
#endif
  if (dilist.list.head()) dilist.list.head()->Set_Semaphores(1); // initialize all as valid to show
  
  // if specific expressions are selected, test for their presence
  if (expressionnum>0) PLL_LOOP_FORWARD(DIL_entry,dilist.list.head(),1) {
    String * cptr = e->Entry_Text();
    if (!cptr) e->Set_Semaphore(0);
    else if (expressionlist.first_contained_in(*cptr)<0) e->Set_Semaphore(0);
  }
  
  // show the desired entries of the full DIL hierarchy
  if (hierarchysorting) {
    DIL_entry_ptr * dep = new DIL_entry_ptr[dilist.list.length()]; int n = 0;
    if (reversehierarchy) PLL_LOOP_FORWARD(DIL_entry,dilist.list.head(),1) {
      if ((e->Get_Semaphore()>0) && (!dilist.Entry_Has_Dependencies(e,true))) dep[n++] = e;
    } else PLL_LOOP_FORWARD(DIL_entry,dilist.list.head(),1) if ((e->Get_Semaphore()>0) && (!e->Has_Superiors(true))) dep[n++] = e;
    if (n>0) {
      qsort(dep,n,sizeof(DIL_entry_ptr),DIL_entry_target_date_qsort_compare);
      for (int i=0; i<n; i++) Show_DIL_Hierarchy(dep[i],dv,maxdepth);
    }
    delete[] dep;
  } else {
    if (reversehierarchy) PLL_LOOP_FORWARD(DIL_entry,dilist.list.head(),1) {
      if ((e->Get_Semaphore()>0) && (!dilist.Entry_Has_Dependencies(e,true))) Show_DIL_Hierarchy(e,dv,maxdepth);
    } else PLL_LOOP_FORWARD(DIL_entry,dilist.list.head(),1) if ((e->Get_Semaphore()>0) && (!e->Has_Superiors(true))) Show_DIL_Hierarchy(e,dv,maxdepth);
  }
}

String Tabbed_FORM_DIL_Hierarchy(String dilidstr, int maxdepth) {
  // Visualize a DIL hierarchy starting at dilid with tabs indicating depth
  // including HREFs and other HTML code, while HTML head and tail code are
  // left to the content in which the extracted hierarchy is embedded.
  // A comment is included that enables commenting out of process output simply
  // by appending "<!-- " to the head code.
  // Note that since standard output does not know which file the text may
  // be embedded in, HREF file references are absolute. Post-processing may
  // convert those to relative references.
  // This output presents FORM check boxes and a "submit" button that extracts
  // checked DIL IDs with the aid of dil2al form processing.
  // There is also a "new hierarchy" button that uses the checked boxes as
  // the top items in new hierarchies to visualize.
  
#ifdef DEBUG_HIERARCHY_EXPRESSIONS
  VOUT << "&&& Tabbed_FORM_DIL_Hierarchy: dilidstr = " << dilidstr << '\n';
#endif
  DIL_Visualize_with_FORM_Tabs dv;
  
  // 3. Get optional expressions that must occur in DIL entry content text
  String expressions(dilidstr.after('#'));
  if (!expressions.empty()) dilidstr = dilidstr.before('#');
  
  // 4. Get optional ContentFile reference
  String contentfile(dilidstr.after('c'));
  if (!contentfile.empty()) dilidstr = dilidstr.before('c');
  
  // dilidstr may now contain no, one or several DIL IDs
  Detailed_Items_List dilist;
  dilist.Get_All_DIL_ID_File_Parameters();
  dilist.Get_All_Topical_DIL_Parameters(true); // always get for FORM hierarchy
  if (!dilist.list.head()) return String("");
  if (dilidstr.empty()) { // full hierarchy
    INFO << "No specific DIL ID, creating full hierarchy\n";
    // *** Is implemented in ~/doc/html/lists.html: maxdepth = 5; // reduced depth for full hierarchy
    Tabbed_FORM_DIL_Hierarchy_full(dilist,dv,maxdepth,expressions);
  } else {
	StringList dilidlist(dilidstr,'+'); int dilidnum = dilidlist.length();
	DIL_ID dilid;
	for (int i = 0; i<dilidnum; i++) {
		dilid = dilidlist[i];
		if (dilid.valid()) {
			Show_DIL_Hierarchy(dilist.list.head()->elbyid(dilid),dv,maxdepth);
			dv.Append("<HR>\n");
		}
	}
	if (!expressions.empty()) Tabbed_FORM_DIL_Hierarchy_full(dilist,dv,maxdepth,expressions);
  }
  if (!contentfile.empty()) if (!write_file_from_String(contentfile,dv.Output_Content(),"DIL Hierarchy Content")) WARN << "Tabbed_HTML_DIL_Hierarchy(): Unable to write " << contentfile << ".\nContinuing as is.\n";
  
  // prepare the FORM output
  String formoutputstr("<!-- comment out process output --><B>Maximum Depth = ");
  formoutputstr += String((long) maxdepth)+"</B>\n<FORM METHOD=\"GET\" ACTION=\"/cgi-bin/dil2al\"><INPUT type=\"hidden\" name=\"dil2al\" value=\"D\">\n<PRE>\n"+dv.Output()+"</PRE>\n<P>\n<INPUT type=\"radio\" name=\"processcmd\" value=\"extract\"> Extract\n<BR><INPUT type=\"radio\" name=\"processcmd\" value=\"newhierarchy\" checked> New Hierarchy with depth: <INPUT type=\"text\" name=\"maxdepth\" value=\"" + String((long) maxdepth) + "\" maxlength=5> and excerpt length: <INPUT type=\"text\" name=\"setpar__EQ__hierarchyexcerptlength\" value=\""+String((long) hierarchyexcerptlength)+"\" maxlength=5> <INPUT type=\"radio\" name=\"setpar\" value=\"nohierarchysorting\"";
  if (!hierarchysorting) formoutputstr += " checked";
  formoutputstr += "> unsorted <INPUT type=\"radio\" name=\"setpar\" value=\"hierarchysorting\"";
  if (hierarchysorting) formoutputstr += " checked";
  formoutputstr += "> sorted  <INPUT type=\"radio\" name=\"setpar\" value=\"noreversehierarchy\"";
  if (!reversehierarchy) formoutputstr += " checked";
  formoutputstr += "> top-down <INPUT type=\"radio\" name=\"setpar\" value=\"reversehierarchy hierarchyplanparse\"";
  if (reversehierarchy) formoutputstr += " checked";
  formoutputstr += "> bottom-up <INPUT type=\"checkbox\" name=\"expressions\" value=\"%40begin%3A%20PLAN%20DIL%20TEMPLATE\"";
  if (!expressions.empty()) formoutputstr += " checked";
  formoutputstr += "> PLAN entries only\n<BR><INPUT type=\"submit\" value=\"Process\"> <INPUT type=\"reset\"></FORM>\n";
  
  return formoutputstr;
}

String GRAPH_DIL_Hierarchy(String dilidstr, int maxdepth, char graphtype) {
  // Visualize a DIL hierarchy starting at 'dilidstr' by creating a graph file
  // that can be processed by a graph language processor indicated by the
  // 'graphtype', e.g. the "dot" language processor of GraphViz ('D'),
  // Java graph output expressed in a JSON file ('J'), or Cytoscape ('C').
  // The output of this function is collected in DIL_Hierarchy_cmd() where
  // it can be written to a file such as a .dot, .json or .xgmml file.
  //
  // NOTE: To add additional GRAPH output types, see the notes in
  // ~/src/dil2al/DIL-hierarchy-output-notes.txt.
  
#ifdef DEBUG_HIERARCHY_EXPRESSIONS
  VOUT << "&&& GRAPH_DIL_Hierarchy: dilidstr = " << dilidstr << '\n';
#endif
  DIL_Visualize_GRAPH dv;
  dv.graphtype = graphtype;
  
  // 3. Get optional expressions that must occur in DIL entry content text
  String expressions(dilidstr.after('#'));
  if (!expressions.empty()) dilidstr = dilidstr.before('#');
  
  // 4. Get optional ContentFile reference
  String contentfile(dilidstr.after('c'));
  if (!contentfile.empty()) dilidstr = dilidstr.before('c');
  
  // dilidstr may now contain no, one or several DIL IDs
  Detailed_Items_List dilist;
  dilist.Get_All_DIL_ID_File_Parameters();
  dilist.Get_All_Topical_DIL_Parameters(true); // always get for GRAPH hierarchy
  if (!dilist.list.head()) return String("");
  dilist.list.head()->Set_Semaphores(0); // Avoid duplicates in this type of hierarchy output!
  if (dilidstr.empty()) { // full hierarchy
    WARN << "GRAPH_DIL_Hierarchy(): GRAPH of full hierarchy (without specified DILID) not yet implemented!\n";
    //VOUT << "No specific DIL ID, creating full hierarchy\n";
    // *** Is implemented in ~/doc/html/lists.html: maxdepth = 5; // reduced depth for full hierarchy
    //Tabbed_FORM_DIL_Hierarchy_full(dilist,dv,maxdepth,expressions);
  } else {
#ifdef DEBUG_PLAN_ENTRY
    VOUT << "&&& valid DIL ID\n";
#endif
    StringList dilidlist(dilidstr,'+'); int dilidnum = dilidlist.length();
    DIL_ID dilid;
    for (int i = 0; i<dilidnum; i++) {
#ifdef DEBUG_PLAN_ENTRY
      VOUT << "&&& ID: " << dilidlist[i] << '\n';
#endif
      dilid = dilidlist[i];
      if (dilid.valid()) {
	Show_DIL_Hierarchy(dilist.list.head()->elbyid(dilid),dv,maxdepth);
	//dv.Append("<HR>\n");
      }
    }
    //if (!expressions.empty()) Tabbed_FORM_DIL_Hierarchy_full(dilist,dv,maxdepth,expressions);
  }
  
  // Separately place connection arrows if source and destination were both shown
  PLL_LOOP_FORWARD(DIL_entry,dilist.list.head(),1) dv.Add_Connections(e);
  
  // Place abstract connections between Objectives
  dv.Add_Abstract_Connections();
  
  // Write an optional "Content" file (indicated with the '[cContentFile]' option of dil2al -H)
  if (!contentfile.empty()) if (!write_file_from_String(contentfile,dv.Output_Content(),"DIL Hierarchy Content")) WARN << "GRAPH_HTML_DIL_Hierarchy(): Unable to write " << contentfile << ".\nContinuing as is.\n";
  
  // prepare the GRAPH output to deliver back to DIL_Hierarchy_cmd()
  String graphoutputstr;
  if (graphtype=='C') { // Cytoscape format
    graphoutputstr = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n<graph id=\"52\" label=\"Roadmap\" directed=\"1\" cy:documentVersion=\"3.0\" xmlns:dc=\"http://purl.org/dc/elements/1.1/\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\" xmlns:cy=\"http://www.cytoscape.org\" xmlns=\"http://www.cs.rpi.edu/XGMML\">\n  <att name=\"networkMetadata\">\n    <rdf:RDF>\n      <rdf:Description rdf:about=\"http://www.cytoscape.org/\">\n        <dc:type>Protein-Protein Interaction</dc:type>\n        <dc:description>N/A</dc:description>\n        <dc:identifier>N/A</dc:identifier>\n        <dc:date>2014-02-25 20:48:20</dc:date>\n        <dc:title>Roadmap</dc:title>\n        <dc:source>http://www.cytoscape.org/</dc:source>\n        <dc:format>Cytoscape-XGMML</dc:format>\n      </rdf:Description>\n    </rdf:RDF>\n  </att>\n  <att name=\"shared name\" value=\"Roadmap\" type=\"string\"/>\n  <att name=\"name\" value=\"Roadmap\" type=\"string\"/>\n  <att name=\"selected\" value=\"1\" type=\"boolean\"/>\n  <att name=\"__Annotations\" type=\"list\">\n    <att name=\"__Annotations\" value=\"\" type=\"string\"/>\n  </att>\n  <att name=\"layoutAlgorithm\" value=\"Hierarchical Layout\" type=\"string\" cy:hidden=\"1\"/>\n";
    graphoutputstr += dv.Output();
    graphoutputstr += "\n</graph>\n";
  } else if (graphtype=='J') { // JSON format
    graphoutputstr = "{\n    \"nodes\": [\n";
    graphoutputstr += dv.Output();
    graphoutputstr += "}\n";
  } else { // DOT GraphViz format
    graphoutputstr = "digraph G {\nconcentrate=true;\ncharset=latin1;\nsize=\"7.5,10\";\ngraph [rankdir=\"RL\"];\nnode [margin=\"0.055,0.0275\"];\n";
    graphoutputstr += dv.Output();
    graphoutputstr += "}\n";
  }
  
  return graphoutputstr;
}

bool DIL_Hierarchy_cmd(String dilidstr, int maxdepth) {
  // visualize a DIL hierarchy
  
#ifdef DEBUG_HIERARCHY_EXPRESSIONS
  VOUT << "&&& DIL_Hierarchy_cmd: dilidstr = " << dilidstr << '\n';
#endif
  String hierarchyfile("");
  
  // 1. Get optional @f@ hierarchyfile reference
  if ((dilidstr.length()>=2) && (dilidstr[0]=='@')) {
    dilidstr.del(0,1);
    hierarchyfile = dilidstr.before('@');
    dilidstr = dilidstr.after('@');
  }
#ifdef DEBUG_HIERARCHY_EXPRESSIONS
  VOUT << "&&& DIL_Hierarchy_cmd: hierarchyfile = " << hierarchyfile << " dilidstr = " << dilidstr << '\n';
#endif
  
  // 2. Get optional type identifier
  if ((dilidstr.length()>=4) && (String(dilidstr.before(4))=="FORM")) {
    dilidstr = dilidstr.from(4);
#ifdef DEBUG_HIERARCHY_EXPRESSIONS
    VOUT << "&&& DIL_Hierarchy_cmd: type is FORM, dilidstr = " << dilidstr << '\n';
#endif
    if ((calledbyforminput) && (hierarchyfile.empty())) INFO << "</PRE><!-- "; // clean up HTML output (dil2al processing output can still be inspected by looking at the HTML source)
    dilidstr = Tabbed_FORM_DIL_Hierarchy(dilidstr,maxdepth);
    // *** If I want to give hierarchy.html a proper head and tail then I
    //     can detect "if ((!hierarchyfile.empty()) && (!dilidstr.empty()))"
    //     here and prepend the head and append the tail to dilidstr if true.
  } else if ((dilidstr.length()>4) && (String(dilidstr.before(4))=="HTML")) {
    dilidstr = dilidstr.from(4);
    dilidstr = Tabbed_HTML_DIL_Hierarchy(dilidstr,maxdepth);
  } else if ((dilidstr.length()>5) && (String(dilidstr.before(5))=="GRAPH")) {
    dilidstr = dilidstr.from(5);
    char graphtype = 'D'; // .dot
    if ((dilidstr.length()>4) && (String(dilidstr.before(4))=="JSON")) {
      graphtype = 'J'; // .json
      dilidstr = dilidstr.from(4);
    } else if ((dilidstr.length()>4) && (String(dilidstr.before(4))=="CYTO")) {
      graphtype = 'C'; // .xgmml
      dilidstr = dilidstr.from(4);
    }
    // Fill a string with new .dot, .json or .xgmml file content.
    dilidstr = GRAPH_DIL_Hierarchy(dilidstr,maxdepth,graphtype);
  } else dilidstr = Tabbed_DIL_Hierarchy(dilidstr,maxdepth);
  if (dilidstr.empty()) return false;
  if (!hierarchyfile.empty()) {
    INFO << "Writing DIL Hierarchy to file " << hierarchyfile << " (<A HREF=\"file://" << hierarchyfile << "\">" << hierarchyfile << "</A>\n";
    return write_file_from_String(hierarchyfile,dilidstr,"DIL Hierarchy");
  }
  INFO << dilidstr;
  
  return true;
}

enum Explicit_OOP_Type {freemind_oop, compendium_oop, no_explicit_oop};

const char Explicit_OOP_Type_str[3][11] = {"Freemind","Compendium","none"};

class OOP_Metrics: public PLLHandle<OOP_Metrics>  {
protected:
  DIL_entry * de;
  String shortidstr;
  Explicit_OOP_Type ooptype;
  int numdependencies;		// How many different actionables connect directly with this? (Has OOP led to any work differentiation, actionable scheduling?)
  int numdirectdependencies;	// How many different actionables have been scheduled for this? (Has OOP led to granular understanding, scheduling?)
  double directTreq;		// directTreq & directTdone give an impression of the breadth across branches of actionables over which progress is made
  double directTdone;
  double Treq;			// Treq & Tdone give the best estimate (for the current level of OOP completeness) of progress on this
  double Tdone;
  time_t t_intended;		// The original target date for this, as intended
  time_t t_allpartsscheduled;	// The earliest possible target date according to all the actionable parts scheduled
public:
  OOP_Metrics(DIL_entry * eptr, String s): de(eptr), shortidstr(s), ooptype(no_explicit_oop), numdependencies(0), numdirectdependencies(0), t_intended(eptr->Target_Date()), t_allpartsscheduled(-1) {}
  ~OOP_Metrics() {}
  void set_Short_ID(String s) { shortidstr = s; }
  void set_OOP_Type(Explicit_OOP_Type o) { ooptype = o; }
  void set_Num_Dependencies(int ndep) { numdependencies = ndep; }
  void set_Num_Direct_Dependencies(int ndep) {numdirectdependencies = ndep; }
  DIL_entry * DE() { return de; }
  String ShortID() { return shortidstr; }
  Explicit_OOP_Type OOPType() { return ooptype; }
  int NumDependencies() { return numdependencies; }
  int NumDirectDependencies() { return numdirectdependencies; }
  double DirectTReq() { return directTreq; }
  double DirectTDone() { return directTdone; }
  double DirectCompletionRatio() { if (directTreq<=0.0) return 1.0; return directTdone/directTreq; }
  double TReq() { return Treq; }
  double TDone() { return Tdone; }
  double CompletionRatio() { if (Treq<=0.0) return 1.0; return Tdone/Treq; }
  time_t TargetDate_Intended() { return t_intended; }
  time_t TargetDate_AllPartsScheduled() { return t_allpartsscheduled; }
  void Check_Dependencies();
  String * OutputHTML(String & outputstring);
};

int OMCDentries = 0;
int OMCDdilchecks = 0;
time_t t_unspecified;

void OOP_Metrics::Check_Dependencies() {
  // Collects some information about dependencies
  
  OMCDentries++;
  numdependencies=0;
  numdirectdependencies=0;
  directTreq=0.0;
  directTdone=0.0;
  Treq=0.0;
  Tdone=0.0;
  time_t t = Time(NULL);
  t_allpartsscheduled=t;
  // Make sure to watch the de->Completion_State() as well!
  double thiscratio = de->Completion_State();
  if ((thiscratio<0.0) || (thiscratio>=1.0)) return;
  PLL_LOOP_FORWARD(DIL_entry,de->head(),1) {
    OMCDdilchecks++;
    double treq = e->Time_Required(), cratio = e->Completion_State();
    if ((cratio>=0.0) && (treq>0.0)) {
      // Note that I will not count zero-time-required tasks here, because they do not indicate OOP actionables.
      if (e->Is_Direct_Dependency_Of(de)) {
	// add to dependency count and add to completion ratio
	// *** It seems like we should use the values of the "dependency" parameters somehow!!
	numdirectdependencies++;
	double tdone = cratio*treq;
	if (cratio>1.0) directTreq += tdone;
	else directTreq += treq;
	directTdone += tdone;
      }
      if (e->Is_Dependency_Of(de)) {
	// add to dependency count, update earliest target date and add to completion ratio
	numdependencies++;
	double tdone = cratio*treq;
	if (cratio>1.0) Treq += tdone;
	else Treq += treq;
	Tdone += tdone;
	if (cratio<1.0) {
	  double tleft = treq*(1.0-cratio); // already in seconds
	  time_t tdate = e->Target_Date();
	  if ((tdate<(t+tleft)) || (tdate>t_unspecified)) tdate = t+tleft;
	  if (tdate>t_allpartsscheduled) t_allpartsscheduled = tdate;
	}
      }
    }
  }
  // add time scheduled for this DIL entry itself
  double thistreq = de->Time_Required();
  if (thistreq>0.0) {
    double tdone = thistreq*thiscratio;
    directTreq += thistreq;
    directTdone += tdone;
    Treq += thistreq;
    Tdone += tdone;
    if (t_intended>t_allpartsscheduled) t_allpartsscheduled = t_intended;
  }
}

// *** BEWARE OF HARDCODED PATHS!!!
String * OOP_Metrics::OutputHTML(String & outputstring) {
  String rowstring = HTML_put_table_cell("",HTML_put_href(idfile+'#'+DE()->str(),DE()->str()))+HTML_put_table_cell("","<B>"+ShortID()+"</B>");
  switch (OOPType()) {
  case freemind_oop: rowstring += HTML_put_table_cell(" BGCOLOR=\"#00FF7F\"",HTML_put_href("/home/randalk/doc/html/planning/"+ShortID()+".mm",Explicit_OOP_Type_str[OOPType()])); break;;
  case compendium_oop: rowstring += HTML_put_table_cell(" BGCOLOR=\"#00FF7F\"",Explicit_OOP_Type_str[OOPType()]); break;;
  default: rowstring += HTML_put_table_cell(" BGCOLOR=\"#FF7F00\"",Explicit_OOP_Type_str[OOPType()]); break;;
  }
  String bgcolorstr("");
  if (NumDirectDependencies()<1) bgcolorstr=" BGCOLOR=\"#FF0000\"";
  rowstring += HTML_put_table_cell(bgcolorstr,String((long) NumDependencies()))+HTML_put_table_cell(bgcolorstr,String((long) NumDirectDependencies()));
  bgcolorstr="";
  if ((TReq()>0.0) && (CompletionRatio()>=1.0)) bgcolorstr=" BGCOLOR=\"#FF0000\"";
  rowstring += HTML_put_table_cell(bgcolorstr,String(TReq()/3600.0,"%.1f"))+HTML_put_table_cell(bgcolorstr,String(DirectTReq()/3600.0,"%.1f"))+
    HTML_put_table_cell(bgcolorstr,String(CompletionRatio()*100.0,"%.1f"))+HTML_put_table_cell(bgcolorstr,String(DirectCompletionRatio()*100.0,"%.1f"));
  bgcolorstr="";
  _DEBUG_TIME_STAMP_TIME_CALLER(__PRETTY_FUNCTION__);
  time_t reasonablecurrenthorizon = time_stamp_time("205111070000"); // *** Probably correct with noexit=false (20190127).
  time_t t = Time(NULL);
  if ((TargetDate_Intended()>reasonablecurrenthorizon) || (TargetDate_AllPartsScheduled()>reasonablecurrenthorizon) || (TargetDate_Intended()<t)) bgcolorstr=" BGCOLOR=\"#FF7F00\"";
  if (TargetDate_AllPartsScheduled()>TargetDate_Intended()) bgcolorstr=" BGCOLOR=\"#FF0000\"";
  rowstring += HTML_put_table_cell(bgcolorstr,time_stamp("%Y%m%d%H%M",TargetDate_Intended()))+HTML_put_table_cell(bgcolorstr,time_stamp("%Y%m%d%H%M",TargetDate_AllPartsScheduled()));
  bgcolorstr="";
  if ((TargetDate_AllPartsScheduled()>TargetDate_Intended()) && (DE()->tdfixed())) bgcolorstr=" BGCOLOR=\"#7F0000\"";
  String entrytext(*(DE()->Entry_Text()));
  HTML_remove_comments(entrytext);
  HTML_remove_tags(entrytext);
  remove_whitespace(entrytext);
  Elipsis_At(entrytext,80);
  rowstring += HTML_put_table_cell("",entrytext);
  outputstring += HTML_put_table_row(bgcolorstr,rowstring)+'\n';
	
  return &outputstring;
}

class OOP_Metrics_Set: public PLLRoot<OOP_Metrics> {
public:
  OOP_Metrics_Set() {}
};

#define COMPENDIUMMAP 2
#define DERBYVALUE_NODETYPE 4
#define DERBYVALUE_NODENAME 7
// *** BEWARE OF HARDCODED PATHS!!!
void Get_Metrics(Detailed_Items_List & dilist, String & simplestring, const int maxdepth, String expressions) {
  // parse the DIL and other sources of data to collect metrics
  DIL_entry ** dep = dilist.Sort_by_Target_Date(true);
  long deplen = dep[0]->fulllength();
  _DEBUG_TIME_STAMP_TIME_CALLER(__PRETTY_FUNCTION__);
  t_unspecified = time_stamp_time("900001010000"); // *** Probably correct with noexit=false (20190127).
  // cut out possible special expressions
  double treq_threshold = MAXDOUBLE;
  double uimp_threshold = MAXDOUBLE;
  while (1) {
    String firstexpression(expressions.before('+'));
    //VOUT << firstexpression << '\n';
    if (firstexpression.contains("TREQ>")) {
      treq_threshold = atof(String(firstexpression.after('>')))*3600.0;
      INFO << "Including tasks with time required greater than " << treq_threshold << " hours\n";
      expressions = expressions.after('+');
      continue;
    }
    if (firstexpression.contains("UIMP>")) {
      uimp_threshold = atof(String(firstexpression.after('>')));
      INFO << "Including tasks with unbounded importance greater than " << uimp_threshold << '\n';
      expressions = expressions.after('+');
      continue;
    }
    break;
  }
  StringList expressionlist(expressions,'+');
  int expressionnum = expressionlist.length()-1; // since an empty extra item is initialized in the StringList
  for (int i = 0; i<expressionnum; i++) expressionlist[i] = URI_unescape(expressionlist[i]);
  //if (dilist.list.head()) dilist.list.head()->Set_Semaphores(1); // initialize all as valid to show
  // if specific expressions are selected, test for their presence
  OOP_Metrics_Set oopmetrics;
  if (expressionnum>0) for (long i=0; i<deplen; i++) { // PLL_LOOP_FORWARD(DIL_entry,dilist.list.head(),1)
      String * cptr = dep[i]->Entry_Text();
      //if (!cptr) e->Set_Semaphore(0);
      //else 
      if (expressionlist.first_contained_in(*cptr)>=0) {
	String objectivestr(cptr->after('['));
	objectivestr = objectivestr.before(']');
	HTML_remove_tags(objectivestr);
	oopmetrics.link_before(new OOP_Metrics(dep[i],objectivestr)); // <0) e->Set_Semaphore(0);
	continue;
      }
      if (dep[i]->Completion_State()<0.0) continue;
      if (dep[i]->Completion_State()>=1.0) continue;
      if (dep[i]->Time_Required()>=treq_threshold) {
	oopmetrics.link_before(new OOP_Metrics(dep[i],""));
	continue;
      }
      if (dep[i]->Unbounded()>=uimp_threshold) {
	oopmetrics.link_before(new OOP_Metrics(dep[i],""));
	continue;
      }
    }
  // collect existing OOP 'freemind' mind maps
  DIR *mmdir = opendir("/home/randalk/doc/html/planning");
  if (!mmdir) {
    ERROR << "Get_Metrics(): Unable to read OOP 'freemind' mind maps directory.\n";
    delete[] dep;
    return;
  }
  struct dirent * mmdirentry;
  const BigRegex mmfilerx("[.]mm$");
  while ((mmdirentry = readdir(mmdir))!=NULL) {
    String mmfilename(mmdirentry->d_name);
    if (mmfilename.contains(mmfilerx)) {
      mmfilename.del((_BIGSTRING_SIZE) mmfilename.length()-3,(_BIGSTRING_SIZE) 3);
      PLL_LOOP_FORWARD(OOP_Metrics,oopmetrics.head(),1) if (mmfilename==e->ShortID()) { e->set_OOP_Type(freemind_oop); break; }
    }
  }
  // collect the Compendium backup
  String compendium;
  if (!read_file_into_String("/home/randalk/local/bin/Compendium/Backups/formalizer-whiteboard.sql",compendium)) {
    ERROR << "Get_Metrics(): Unable to read OOP maps in Compendium database backup.\n";
    delete[] dep;
    return;
  }
  const BigRegex derbyinsertrx("\nINSERT INTO Node ");
  int nextinsert = -1;
  while ((nextinsert = compendium.index(derbyinsertrx,nextinsert+1))>=0) {
    int nextnewline = compendium.index('\n',nextinsert+1);
    String derbyvalues(compendium.at(nextinsert,nextnewline-nextinsert));
    derbyvalues = derbyvalues.after("VALUES (");
    StringList derbyvaluelist(derbyvalues,',');
    int compendiumnodetype = atoi(derbyvaluelist[DERBYVALUE_NODETYPE]);
    if (compendiumnodetype==COMPENDIUMMAP) {
      String mapname(derbyvaluelist[DERBYVALUE_NODENAME].after('\'')); mapname.del((_BIGSTRING_SIZE) mapname.length()-1,(_BIGSTRING_SIZE) 1);
      PLL_LOOP_FORWARD(OOP_Metrics,oopmetrics.head(),1) if (mapname==e->ShortID()) {
	if (e->OOPType()==no_explicit_oop) e->set_OOP_Type(compendium_oop);
	else WARN << "Detected more than one explicit OOP for " << e->ShortID() << "!\n";
	break;
      }
    }
  }
  // *** and then, I will probably also want to be able to parse Task Logs to collect metrics...
  // show the desired metrics of the full DIL hierarchy
  INFO << oopmetrics.length() << ':'; VOUT.flush();
  int iun_state = 0;
  PLL_LOOP_FORWARD(OOP_Metrics,oopmetrics.head(),1) {
    e->Check_Dependencies();
    switch (iun_state) {
    case 0: if (e->TargetDate_Intended()>ImmediateTD) {
	simplestring += "<TR><TD COLSPAN=9><B>IMMEDIATE TASKS ^^^^^^^^^^</B></TD><TD COLSPAN=3><B>"+time_stamp("%Y%m%d%H%M",ImmediateTD)+"</B></TD></TR>";
	iun_state++;
      }
      break;;
    case 1: if (e->TargetDate_Intended()>UrgentTD) {
	simplestring += "<TR><TD COLSPAN=9><B>URGENT TASKS ^^^^^^^^^^</B></TD><TD COLSPAN=3><B>"+time_stamp("%Y%m%d%H%M",UrgentTD)+"</B></TD></TR>";
	iun_state++;
      }
      break;;
    case 2: if (e->TargetDate_Intended()>NearTermTD) {
	simplestring += "<TR><TD COLSPAN=9><B>NEAR TERM TASKS ^^^^^^^^^^</B></TD><TD COLSPAN=3><B>"+time_stamp("%Y%m%d%H%M",NearTermTD)+"</B></TD></TR>";
	iun_state++;
      }
      break;;
    }
    e->OutputHTML(simplestring);
    INFO << OMCDentries << '.'; VOUT.flush();
  }
  // *** It would be good to know how reliable the Treqs are... i.e. how much of the OOP detailing had been done.
  // This should be reflected in the visualization.
  delete[] dep;
}

String Metrics_List(String dilidstr, int maxdepth) {
  // Produce a list (csv format) with metrics.
  
  // DIL_Visualize_GRAPH dv; *** I probably want to use a Metrics class here that stores the metrics data and connects with other such objects.
  String simplestring;
  
  // STEP ONE: Let's just find Objectives first! (See TL#201104181348.1.) - for this provide OBJECTIVE in an expression.
  // 3. Get optional expressions that must occur in DIL entry content text
  String expressions(dilidstr.after('#'));
  if (!expressions.empty()) dilidstr = dilidstr.before('#');
  
  // 4. Get optional ContentFile reference
  String contentfile(dilidstr.after('c'));
  if (!contentfile.empty()) dilidstr = dilidstr.before('c');
  
  // dilidstr may now contain no, one or several DIL IDs
  Detailed_Items_List dilist;
  dilist.Get_All_DIL_ID_File_Parameters();
  dilist.Get_All_Topical_DIL_Parameters(true); // always get for FORM hierarchy *** does this apply here too?
  if (!dilist.list.head()) return String("");
  INFO << dilist.list.length() << '\n';
  dilist.list.head()->Set_Semaphores(0); // Avoid duplicates
  if (dilidstr.empty()) { // full hierarchy
    Get_Metrics(dilist,simplestring,maxdepth,expressions);
    // EOUT << "Full Metrics List not yet implemented!\n";
  } else {
    StringList dilidlist(dilidstr,'+'); int dilidnum = dilidlist.length();
    DIL_ID dilid;
    for (int i = 0; i<dilidnum; i++) {
      dilid = dilidlist[i];
      if (dilid.valid()) {
	Get_Metrics(dilist,simplestring,maxdepth,expressions); // ,dv,maxdepth);
      }
    }
  }
  
  // Separately place connection arrows if source and destination were both shown
  // PLL_LOOP_FORWARD(DIL_entry,dilist.list.head(),1) dv.Add_Connections(e);
  // Write the resulting .dot language file
  // if (!contentfile.empty()) if (!write_file_from_String(contentfile,dv.Output_Content(),"DIL Hierarchy Content")) EOUT << "dil2al: Unable to write " << contentfile << "in Tabbed_HTML_DIL_Hierarchy(), continuing as is\n";
  // prepare the output
  // String formoutputstr("digraph G {\nconcentrate=true;\ncharset=latin1;\nsize=\"7.5,10\";\n"); //("<!-- comment out process output --><B>Maximum Depth = ");
  // formoutputstr += dv.Output();
  // formoutputstr += "}\n";
  
  return simplestring;
}

bool DIL_Metrics_cmd(String dilidstr, int maxdepth) {
  // This is a simple implementation of a few metrics to improve oversight of AL/OOP work.
  
  String metricsfile("");
  
  // 1. Get optional @f@ metricsfile reference
  if ((dilidstr.length()>=2) && (dilidstr[0]=='@')) {
    dilidstr.del(0,1);
    metricsfile = dilidstr.before('@');
    dilidstr = dilidstr.after('@');
  }
  String outstr("<HTML>\n<BODY>\n<TABLE>\n<TR><TH>DIL#</TH><TH>ShortID</TH><TH>OOP</TH><TH>#deps</TH><TH>#direct</TH><TH>Treq</TH><TH>Tdirect</TH><TH>%comp</TH><TH>%direct</TH><TH>t_intended</TH><TH>t_allparts</TH><TH>Excerpt</TH></TR>\n");
  
  // 2. Get optional type identifier
  if ((dilidstr.length()>=4) && (String(dilidstr.before(4))=="LIST")) {
    dilidstr = dilidstr.from(4);
    if ((calledbyforminput) && (metricsfile.empty())) INFO << "</PRE><!-- "; // clean up HTML output (dil2al processing output can still be inspected by looking at the HTML source)
    outstr += Metrics_List(dilidstr,maxdepth);
  } else outstr += Metrics_List(dilidstr,maxdepth); // For now, there is only one type!
  //if (outstr.empty()) return false;
  outstr += "</TABLE>\n</BODY>\n</HTML>\n";
  INFO << '\n' << OMCDentries << '\n';
  INFO << OMCDdilchecks << '\n';
  INFO << IDOentered << '\n';
  INFO << IDOpassed << '\n';
  if (!metricsfile.empty()) {
    INFO << "Writing Metrics to file " << metricsfile << " (<A HREF=\"file://" << metricsfile << "\">" << metricsfile << "</A>\n";
    return write_file_from_String(metricsfile,outstr,"DIL Hierarchy");
  }
  INFO << outstr;
  return true;
}

