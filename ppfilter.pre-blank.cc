// tlfilter.cc
// Randal A. Koene, 20000304

#include "dil2al.hh"

int get_file_in_list(String & fname, StringList & srcf, StringList &srcfname, int & srcfnum) {
// load a file if it is not already in a list of file-strings
	int srcidx;
	if ((srcidx = srcfname.iselement(fname))<0) {
		if (!read_file_into_String(fname,srcf[srcfnum])) {
			EOUT << "dil2al: Unable to load source document " << fname << " in get_file_in_list()\n";
			return -1;
		}
		srcfname[srcfnum] = fname;
		srcidx = srcfnum;
		srcfnum++;
	}
	return srcidx;
}

void get_figure_data(String & figfile, String & figlabel, String & figcaption, String & psconversions, StringList & srcf, StringList & srcfname, int & srcfnum) {
// obtains missing figure data from the Figures Directory
	int srcidx;
	if ((srcidx = get_file_in_list(figuresdirectory,srcf,srcfname,srcfnum))<0)
		EOUT << "dil2al: Some figure data may be missing in get_figure_data(), continuing as is\n";
	else {
		// find figure data
		String findid;
		if (figfile!="") findid = relurl(figuresdirectory,figfile);
		else if (figlabel!="") findid = figlabel;
		else EOUT << "dil2al: No figure file or label, some figure data may be missing in get_figure_data(), continuing as is\n";
		if (findid!="") {
			int figididx;
			if ((figididx = srcf[srcidx].index(BigRegex("[<]!--[ 	]@Begin FIGURE DATA:[^@]*"+findid+"[^@]*@[ 	]*--[>]")))<0)
				EOUT << "dil2al: Figure " << findid << " not found in get_figure_data(), continuing as is\n";
			else {
				int figidend;
				if ((figidend = srcf[srcidx].index(BigRegex("[<]!--[ 	]*@End FIGURE DATA@[ 	]*--[>]"),figididx))<0)
					EOUT << "dil2al: Figure " << findid << " End marker missing in get_figure_data(), continuing as is\n";
				else {
					String figdata = srcf[srcidx].at(figididx,figidend-figididx);
					figlabel = figdata.at(BigRegex("[<]!--[ 	]*@Figure label@[ 	]*--[>][ 	]*[^]< 	\n]+"));
					figlabel = figlabel.after(BigRegex("[<]!--[ 	]*@Figure label@[ 	]*--[>][ 	]*"));
					figfile = figdata.at(BigRegex("[<]!--[ 	]*@Figure file@[ 	]*--[>][ 	]*[^< 	\n]+"));
					figfile = figfile.after(BigRegex("[<]!--[ 	]*@Figure file@[ 	]*--[>][ 	]*"));
					figcaption = figdata.at(BigRegex("[<]!--[ 	]*@Figure caption@[ 	]*--[>][ 	]*[^< 	\n]+"));
					figcaption = figcaption.after(BigRegex("[<]!--[ 	]*@Figure caption@[ 	]*--[>][ 	]*"));
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
	if (figfile=="") figfile = homedir+"doc/tex/common-eps/placeholder.eps";
	if (figlabel=="") figlabel = "fig:unknown";
	if (figcaption=="") figcaption = "\\relax";
}

bool get_Paper_Plans_Section_IDs(StringList & ppsectionids, StringList & ppsectiontitles) {
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
	String pstid, psttitle;
	while ((pstidx = ppfstr.index("<LI>",pstidx+1))>=0) {
		if ((unsigned int) pstidx < (unsigned int) pstend) {
			if ((pstid = ppfstr.at(BigRegex("[[]\\([<][^>]*[>]\\)?[^]]+"),pstidx))!="") {
				pstid.del("["); pstid.gsub(BigRegex("[<][^>]*[>]"),""); pstid.gsub(BigRegex("[ 	]+"),"");
				if ((psttitle = ppfstr.at(BigRegex("[]][^\n]+"),pstidx))!="") {
					psttitle = psttitle.after(BigRegex("[]][ 	]*"));
					psttitle.del(BigRegex("[ 	]*$"));
					psttitle.upcase();
					ppsectionids[pstnum] = pstid;
					ppsectiontitles[pstnum] = psttitle;
					pstnum++;
				}
			}
		} else break;
	}
	return true;
}

// #define DEBUG_DO_NOT_CREATE_NEW_OUTLINE

bool clean_paper_outline(String & pouttext) {
// removes content from previous paper extractions to make room for a possible
// new subsection hierarchy
//*** removal of sections may be added if sections are automatically placed by
//*** checking Section qualifiers in Novelty items (see extract_paper_plan_to_paper
//*** usage comments)
//*** How about positioning protected parts in places other than at the top of
//*** a section? How about manually added subtopics or subtopics without tags?
	String poutold(pouttext);
	pouttext = "";
	int nextextr, nextreq, nextsubstart, nextsubend, nextprot, p = 0, pp = 0, x = 0, r = 0, h = 0, t = 0, P = 0, depth = 0;
	if (verbose) { VOUT << "Cleaning outline: "; VOUT.flush(); }
	while (pp>=0) {
		// find nearest identifier
		nextextr = poutold.index(BigRegex("\n[ 	]*%[ 	]*@Begin[ 	]+\\(REWRITTEN[ 	]+\\)?EXTRACTED[^@]*:[^@]*@"),p);
		pp = nextextr;
		nextreq = poutold.index(BigRegex("\n[ 	]*%[ 	]*@Begin[ 	]+\\(REWRITTEN[ 	]+\\)?REQUIRED[^@]*@"),p);
		if ((nextreq>=0) && ((nextreq<pp) || (pp<0))) pp = nextreq;
		nextsubstart = poutold.index(BigRegex("\n[ 	]*%[ 	]*@Begin[ 	]+SUBTOPIC[^@]*:[^@]*@"),p);
		if ((nextsubstart>=0) && ((nextsubstart<pp) || (pp<0))) pp = nextsubstart;
		nextsubend = poutold.index(BigRegex("\n[ 	]*%[ 	]*@End[ 	]+\\([^ 	\n]*SUBSECTION[ 	]+CONTENT\\|SUBTOPIC[^@]*:\\)[^@]*@"),p);
		if ((nextsubend>=0) && ((nextsubend<pp) || (pp<0))) pp = nextsubend;
		nextprot = poutold.index(BigRegex("\n[ 	]*%[ 	]*@Begin[ 	]+PROTECTED[^@]*@"),p);
		if ((nextprot>=0) && ((nextprot<pp) || (pp<0))) pp = nextprot;
		if (pp>=0) {
			// copy up to identifier location
			if (pp>p) pouttext += poutold.at(p,pp-p);
			// process identifier
			if (pp==nextextr) { // clean up EXTRACTED
				int endextr;
				if ((endextr = poutold.index(BigRegex("\n[ 	]*%[ 	]*@End[ 	]+\\(REWRITTEN[ 	]+\\)?EXTRACTED[^@]*:[^@]*@"),String::SEARCH_END,pp))<0) {
					EOUT << "dil2al: @End EXTRACTED...@ not found in clean_paper_outline()\n";
					return false;
				}
				p = endextr; x++;
				if (verbose) { VOUT << "-x"; VOUT.flush(); }
			} else if (pp==nextreq) { // clean up REQUIRED
				int endreq;
				if ((endreq = poutold.index(BigRegex("\n[ 	]*%[ 	]*@End[ 	]+\\(REWRITTEN[ 	]+\\)?REQUIRED[^@]*@"),String::SEARCH_END,pp))<0) {
					EOUT << "dil2al: @End REQUIRED@ not found in clean_paper_outline()\n";
					return false;
				}
				p = endreq; r++;
				if (verbose) { VOUT << "-r"; VOUT.flush(); }
			} else if (pp==nextsubstart) { // clean up SUBTOPIC head
				int headeridx;
//*** no subtopic begins or ends allowed between begin and this line... how about
//*** other codes?
/*
- have to be able to rewrite topic headers as well, in which case you somehow have
  to make sure that the old header with its template content is reused, both at
  the head and the tail
- when cleaning, make sure the head and tail of a template are removed, but make
  sure they really belong to the topic begin or topic end
- why bother cleaning if you clean away basically everything before filling everything
  back in again??? Why not start blank and fill in all the rewritten information?
  
YES: start blank, use all available templates, place what is indicated by sections,
topics and items, before placing something (anything) check if a rewritten version
is already available (make sure the indicators are very clear, i.e. section+context
and begin and end markers with IDs), add in arbitrary text that was marked protected

1. add section title to each extracted ID
2. add subtopic context and section to the End markers of all subtopics
3. add section to the Begin markers of all subtopics
4. add tests for REWRITTEN versions of *everything* to the functions that place parts
5. add section detection from paper plan
6. add clear IDs to templates
7. add placement functions for file header and footer

*/
				if ((headeridx = poutold.index(BigRegex("\n[ 	]*%[ 	]*@Begin[ 	]+[^ 	]*SUBSECTION[ 	]+CONTENT[^@]*@[^)]*)"),String::SEARCH_END,pp))<0)
					if ((headeridx = poutold.index(BigRegex("\n[\\][^ 	\n]*\\(subsection\\|paragraph\\)[{][^\n]*[}]\n"),String::SEARCH_END,pp))<0) {
						EOUT << "dil2al: Subtopic header not found in clean_paper_outline()\n";
						return false;
					}
cerr << ">>>" << poutold.at(pp,headeridx-pp) << "<<<\n";
				p = headeridx; h++; depth++;
				if (verbose) { VOUT << "-h"; VOUT.flush(); }
			} else if (pp==nextsubend) { // clean up SUBTOPIC tail
				int tailidx;
//*** no subtopic begins allowed between the lines
				if ((tailidx = poutold.index(BigRegex("\n[ 	]*%[ 	]*@End[ 	]+SUBTOPIC[^@]*:[^@]*@"),String::SEARCH_END,pp))<0) {
					EOUT << "dil2al: Subtopic tail not found in clean_paper_outline()\n";
					return false;
				}
cerr << ">>>" << poutold.at(pp,tailidx-pp) << "<<<\n";
				p = tailidx; t++; depth--;
				if (verbose) { VOUT << "-t"; VOUT.flush(); }
			} else { // retain PROTECTED
				int endprot;
				if ((endprot = poutold.index(BigRegex("\n[ 	]*%[ 	]*@End[ 	]+PROTECTED[^@]*@"),String::SEARCH_END,pp))<0) {
					EOUT << "dil2al: @End PROTECTED@ not found in clean_paper_outline(), continuing as is\n";
					endprot = poutold.length();
				}
				pouttext += poutold.at(pp,endprot-pp);
				p = endprot; P++;
				if (verbose) { VOUT << "+P"; VOUT.flush(); }
			}
		} else pouttext += poutold.from(p); // copy remaining text
	}
	pouttext.gsub(BigRegex("\n[ 	]*\n[ 	\n]+[ 	]*\n"),"\n\n\n"); // clean up spurious '\n'
	if (verbose) VOUT << ", (x=" << x << ",r=" << r << ",h=" << h << ",t=" << t << ",P=" << P << ",h-t=" << depth << ") done\n";
	return true;
}

void extract_required_item_to_paper(String & ostrtext, String refurl, Novelty_Marker & nm, String & sectitle, String & requireditems, String & pprevtext) {
// generates output for a Required item with HTML tagged .TeX comments
// indicating item qualifiers, quantifiers and additional marked text
// additionally keeps track of a list of required items to be updated
// in the paper plan and used for AL priority DIL entries
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
//*** check if in pprevtext
//*** - some novelty items were extracted to multiple sections, but may have
//***   been rewritten in several ways, while perhaps not having been rewritten
//***   in some sections, so that it is important to know that the right section
//***   is being addressed when a rewritten version is found
//***
//*** * fix clean up... find extra tail removed by outputting cleaned up version
//***   and comparing with version generated from jliltm.tex.outline
//*** * figure out how to differentiate between same-id novelty items in multiple
//***   sections
//*** * add use of pprevtext for extracted novelty and required items
	ostrtext += "% @Begin REQUIRED@\n% Context: ";
	ostrtext += contextconcat;
	if (nm.imp) ostrtext += "\n% Importance: " + impconcat;
	sprintf(fc,"%4.2f",nm.len);
	ostrtext += "\n% Intended length: " + (fc + (" pages\n% Section: " + sectitle));
	ostrtext += "\n% [" + refurl + "]";
	ostrtext += "\n\\editnote{REQUIRED: " + contextconcat;
	if (nm.marktext!="") {
		ostrtext += " (" + nm.marktext + ')';
		contextconcat.prepend("<I>"+nm.marktext+";</I> ");
	}
	ostrtext += "}\n% @End REQUIRED@\n\n";
	// collect required items for addition to paper plan
	sprintf(fc,"%5.2f",nm.len);
	requireditems += "<LI>["+sectitle+"] "+contextconcat+" (imp: "+((impconcat+", len: ")+fc)+")\n";
}

void extract_hierarchy_depth_increase_to_paper(String & ostrtext, Novelty_Marker & nm) {
// generates output for a Topical Context Hierarchy depth increase, suggesting
// subtopics and indicating the TCH title in .TeX comments
	ostrtext += "\n% @Begin SUBTOPIC: [depth " + (dec(nm.hierdepth) + ("] " + nm.context.concatenate(", ") + "@\n"));
	switch (nm.hierdepth) {
		case 0:	ostrtext += "% (Unexpected hierarchy depth at ``\\section'' level.)\n\n";
				EOUT << "dil2al: Unexpected hierarchy depth at ``\\section'' level in extract_hierarchy_depth_increase_to_paper(), continuing\n";
				break;
		case 1: ostrtext += "\\subsection{" + nm.context.concatenate(", ") + "}\n\n"; break;
		case 2: ostrtext += "\\subsubsection{" + nm.context.concatenate(", ") + "}\n\n"; break;
		case 3: ostrtext += "\\paragraph{" + nm.context.concatenate(", ") + "}\n\n"; break;
		case 4: ostrtext += "\\\\\n\n"; break;
		default:	ostrtext += "% (Hierarchy depth exceeds all \\LaTeX subsection levels.)\n\n";
				EOUT << "dil2al: Hierarchy depth exceeds all \\LaTeX subsection levels in extract_hierarchy_depth_increase_to_paper(), continuing\n";
	}
}

void extract_hierarchy_depth_decrease_to_paper(String & ostrtext, Novelty_Marker & nm) {
// generates output for a Topical Context Hierarchy depth decrease, suggesting
// subtopics and indicating the TCH title in .TeX comments
		ostrtext = ostrtext + "\n% @End SUBTOPIC: [depth " + dec(nm.hierdepth+1) + "]@\n\n";
}

/* The following functions incorporate rules used to identify and obtain
   research novelty item content (see <A HREF="../../doc/html/planning/material.html#preparing-results>
   Paper Plans: preparing results
   </A>) */

bool item_content_between_range_tags(Novelty_Marker & nm, int nidx, String & src, String & ic) {
	// 1. search for content range tags
	int nstartidx, nendidx;
	if (((nstartidx = src.index(BigRegex("[<]!--[ 	]*@Begin[ 	]+"+nm.id+"@[ 	]*--[>]"),nidx-src.length()))<0)
	 || ((nendidx = src.index(BigRegex("[<]!--[ 	]*@End[ 	]+"+nm.id+"@[ 	]*--[>]"),nidx))<0)) return false;
	ic = src.at(nstartidx,nendidx-nstartidx);
	return true;
}

bool item_content_within_TL_chunk(int nidx, String & src, String & ic) {
	// 4. content range when within Task Log chunk
	int nstartidx, nendidx;
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
	int nstartidx, nendidx;
	if ((!src.contains(BigRegex("[<]!--[ 	]*dil2al:[ 	]*DIL begin[ 	]*--[>]")))
	 || ((nstartidx = src.index(BigRegex("[<]TR[^>]*[>]"),nidx-src.length()))<0)) return false;
	if ((nstartidx = src.index(BigRegex("[<]TD COLSPAN[^>]*[>]"),nstartidx))<0) return false;
	if ((nendidx = src.index(BigRegex("\\([<]TR[^>]*[>]\\|[<]!--[ 	]*dil2al:[ 	]*DIL end[ 	]*--[>]\\)"),nstartidx))<0) return false;
	ic = src.at(nstartidx,nendidx-nstartidx);
	return true;
}

bool item_content_within_paragraph(int nidx, String & src, String & ic) {
	// 6. content range between <P>, empty lines, beginning and end of file
	int nstartidx, nendidx;
	if ((nstartidx = src.index(BigRegex("\\(\n\n\\|[<][Pp][>]\\)"),nidx-src.length()))>=0) {
		if (src[nstartidx]=='<') nstartidx+=3;
		else nstartidx+=2;
	} else nstartidx=0; // from empty line or beginning
	if ((nendidx = src.index(BigRegex("\\(\n\n\\|[<][Pp][>]\\)"),nidx))<0) nendidx = src.length(); // to empty line or end
	ic = src.at(nstartidx,nendidx-nstartidx);
	return true;
}

void convert_item_content_to_TeX(String & itemcontent) {
	// conversions to TeX
	//	- remove Novelty Marker tags
	itemcontent.gsub(BigRegex("[<]A[ 	]+[Nn][Aa][Mm][Ee][ 	]*=[ 	]*\"NOVELTY-[^>]*[>][[][^]]*N[^]]*[]]"),"");
	//	- identify content hyperlink references
	itemcontent.gsub(BigRegex("[<]A[ 	]+[^>]*[Hh][Rr][Ee][Ff][ 	]*=[ 	]*\"\\([^\"]+\\)\"[^>]*[>]"),"@HRef::_1@",'_');
	//	- cite, but do not recurse into bibliography references
	itemcontent.gsub(BigRegex("\\([[][^]]*\\)?@HRef::[^@]*"+bibindexfilename+"#\\([^@]+:[^@]+\\)@\\([^]]*[]]\\)?"),"",'_');
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
	itemcontent.gsub(BigRegex("\\([^\\]\\)\\([#&_]\\)"),"_1\\_2",'_');
//*** convert GESNlib variable names to GESN convention standard labels as found in nse.label-equivalence.tex
//*** could do that to the source once instead
}

void extract_recursively(String & ostrtext, Novelty_Marker & nm, StringList & srcf, StringList & srcfname, int & srcfnum, int depth, Novelty_Marker_List & visited, String & poutline, String & pprevtext) {
	// keep track of places already visited to avoid infinite loops
	if (visited.iselement(nm)>=0) {
		EOUT << "dil2al: Avoiding repetition of " << nm.source << '#' << nm.id << " in extract_recursively(), continuing as is\n";
		ostrtext += "% @WARNING: Repeated recursion into " + nm.source + '#' + nm.id + "avoided@\n";
		return;
	}
	int visitedidx = visited.length();
	if (nm.nmtype==Novelty_Marker::NM_NOVELTY_ITEM) visitedidx--; // use first list element
	visited[visitedidx].source = nm.source;
	visited[visitedidx].id = nm.id;
	visited[visitedidx].nmtype = nm.nmtype;
	// keep track of depth
	depth++;
	if (depth>MAX_RECURSIVE_PP2PD_EXTRACTION_DEPTH) {
		EOUT << "dil2al: Maximum extraction recursion depth (" << MAX_RECURSIVE_PP2PD_EXTRACTION_DEPTH << ")exceeded in extract_recursively(), continuing as is\n";
		ostrtext = ostrtext + "% @WARNING: Maximum extraction recursion depth (" + dec(MAX_RECURSIVE_PP2PD_EXTRACTION_DEPTH) + ")exceeded@\n";
		return;
	}
	// load source document if not already loaded
	int srcidx;
	if ((srcidx = get_file_in_list(nm.source,srcf,srcfname,srcfnum))<0) {
		EOUT << "continuing as is\n";
		ostrtext += "% @WARNING: Recursion was unable to load source document " + nm.source + "@\n";
		return;
	}
	// include content in outline
	String srcref = relurl(poutline,nm.source)+'#'+nm.id;
	if (nm.nmtype==Novelty_Marker::NM_NOVELTY_ITEM) {
//*** detect if REWRITTEN, if so copy from rewritten version and optionally
//*** ask if new extraction should be concatenated with a marker
//*** if (askextractconcat)
		ostrtext += "% @Begin EXTRACTED: " + srcref + "@\n% Context: ";
		ostrtext += nm.context.concatenate(", ");
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
		ostrtext = ostrtext + "\n% Intended length: " + fc + " pages\n";
	} else ostrtext += "\n% @Begin CONTENT HREF: (depth " + (dec(depth) + (") " + srcref + "@\n"));
	int nidx;
	if ((nidx = srcf[srcidx].index(BigRegex("[<]A[ 	]+[Nn][Aa][Mm][Ee][ 	]*=[ 	]*\""+nm.id+'"')))>=0) {
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
		while (iend<itemcontent.length()) {
			if ((iend = itemcontent.index("@HRef::",istart))<0) {
				iend = itemcontent.length();
				ostrtext += itemcontent.at(istart,iend-istart) + '\n';
			} else {
				ostrtext += itemcontent.at(istart,iend-istart); //*** could add a `\n` here
				// recurse content extraction for @HREF: <document>@ codes
				//*** perhaps recursively extracted content should be placed
				//*** after the following punctuation mark
				//*** buglets:
				//*** getting content from C++ files, file names
				//*** such as synapse_AMPA_NMDA.hh affected by
				//*** conversions
				nmhref.source = itemcontent.at(BigRegex("[^@]+"),iend+7);
				nmhref.id = nmhref.source.after('#');
				if (nmhref.id!="") nmhref.source = nmhref.source.before("\\#");
				nmhref.source = absurl(srcfname[srcidx],nmhref.source);
				// recognize figures
				if (nmhref.source.contains(BigRegex("[.]\\([Ff][Ii][Gg]\\|[Ee]?[Pp][Ss]\\)$"))) {
					nmhref.source = nmhref.source.through('.',-1) + "eps"; // use .eps not .fig
					// obtain label, caption and psfrag conversions
					String figlabel, figcaption, psconversions;
					get_figure_data(nmhref.source,figlabel,figcaption,psconversions,srcf,srcfname,srcfnum);
					// include figure
//*** perhaps use a template here
//*** intelligently choose type of figure command
					ostrtext += "\n\\figurehere";
					if (psconversions!="") ostrtext += "{psconversions}";
					ostrtext += '{' + figlabel + "}{" + relurl(poutline,nmhref.source) + "}{"+figcaption+"}\n";
				} else {
					nmhref.nmtype = Novelty_Marker::NM_CONTENT_HREF;
					extract_recursively(ostrtext,nmhref,srcf,srcfname,srcfnum,depth,visited,poutline,pprevtext);
				}
				if ((istart = itemcontent.index("@",iend+1))<0) istart = iend;
				else istart++;
// *** There are some references without ID
			}
		}
	} else {
		EOUT << "dil2al: " << nm.id << " not found in source file in extract_paper_plan_to_paper(), continuing as is\n";
		ostrtext += "% (Item ID " + nm.id + " not found in content source file " + relurl(poutline,nm.source) + ") \n";
	}
	if (nm.nmtype==Novelty_Marker::NM_NOVELTY_ITEM) ostrtext += "% @End EXTRACTED: " + srcref + "@\n\n";
	else ostrtext += "% @End CONTENT HREF: " + srcref + "@\n";
}

void extract_novelty_item_to_paper(String & ostrtext, Novelty_Marker & nm, StringList & srcf, StringList & srcfname, int & srcfnum, String & poutline, String & pprevtext) {
	if (nm.source=="") {
		EOUT << "dil2al: Missing Novelty item source reference for item with text ``" << nm.marktext << "'' in extract_paper_plan_to_paper(), continuing as is\n";
		return;
	}
	Novelty_Marker_List visited;
	extract_recursively(ostrtext,nm,srcf,srcfname,srcfnum,0,visited,poutline,pprevtext);
}

bool extract_paper_plan_to_paper(String ppname, String poutline, String pprevious) {
// uses information in a paper plan to generate paper content in a paper outline
// (see DIL#20000315155709.1)
// preparation:
// - a sorted paper plan including required items
// - a paper outline including paper-type specific background
//   templates (content that is not added during extraction
//   may already be included)
// - outline contains sections designated to items in
//   the paper plan (can be automatically created by parsing
//   paper plan and using section template)
// - an optional previous (version of the) paper, containing
//   stylistically rewritten items
	String pptext, pprevtext, ostrtext;
	if (!read_file_into_String(ppname,pptext)) return false;
	if (pprevious!="") if (!read_file_into_String(pprevious,pprevtext)) return false; // safe in case new paper draft would overwrite pprevious
	ofstream ostr(poutline+".new");
	if (!ostr) {
		EOUT << "dil2al: Unable to create " << poutline << ".new in extract_paper_plan_to_paper()\n";
		return false;
	}
	String istr; int iread = 0;
	if (!read_file_into_String(poutline,istr)) return false;
//WE NOW START BLANK!	if (!clean_paper_outline(istr)) return false; // clean paper outline
	// read all Novelty items from paper plan into list
	Novelty_Marker_List nml; int nmend = 0, nmnum = 0, hdepth = 0;
	if ((nmend = pptext.index(BigRegex("[<]A[ 	]*[Nn][Aa][Mm][Ee][ 	]*=[ 	]*\"tch\""),nmend))<0) {
		EOUT << "dil2al: No Topical Context Hierarchy found in Paper Plan in extract_paper_plan_to_paper()\n";
		return false;
	}
	while ((nmend = nml[nmnum].Get_Novelty_from_Paper_Plan(pptext,nmend,hdepth))>=0) {
		hdepth = nml[nmnum].hierdepth; // track Topical Context Hierarchy depth
		if (hdepth<0) break;
		if (nml[nmnum].source!="") nml[nmnum].source = absurl(ppname,nml[nmnum].source);
		nmnum++;
	}
	// read Section IDs
	StringList ppsectionids, ppsectiontitles;
	if (!get_Paper_Plans_Section_IDs(ppsectionids,ppsectiontitles)) return false;
//*** generate tentative Abstract, as specified in
//*** DIL#20000315155709.1
	// per section
	// read outline, find section headings that match Section IDs
	int secidx, srcfnum = 0; StringList srcf, srcfname; String requireditems;
	while ((secidx = istr.index("\\section",iread))>=0) {
		// get section title
		String sectitle = istr.at(BigRegex("[^}]+"),secidx+8);
		sectitle = sectitle.after(BigRegex("[ 	]*[{][ 	]*"));
		sectitle.del(BigRegex("[ 	]*$"));
		sectitle.upcase();
		if (verbose) VOUT << "Parsing paper outline section " << sectitle << '\n';
		// copy outline up to \section command
		ostrtext += istr.at(iread,secidx-iread); iread = secidx;
		// copy section header
		if ((secidx = istr.index(BigRegex("%[^\n]*@[ 	]*End[ 	]+SECTION[ 	]+CONTENT[ 	]*@"),iread))>=0) {
			ostrtext += istr.at(iread,secidx-iread); iread = secidx;
		} else {
			EOUT << "dil2al: Missing @End SECTION CONTENT@ tag in extract_paper_plan_to_paper(), content placement may be incorrect, continuing\n";
			secidx = istr.index("\n",iread); secidx++;
			ostrtext += istr.at(iread,secidx-iread); iread = secidx;
		}
		// parse items in section
		int seclistidx; String refurl = relurl(poutline,ppname);
		if ((sectitle!="") && ((seclistidx = ppsectiontitles.iselement(sectitle))>=0)) {
			for (int i=0; i<nmnum; i++)
			 if ((nml[i].nmtype==Novelty_Marker::NM_TCH_DEPTH_INC)
			  || (nml[i].nmtype==Novelty_Marker::NM_TCH_DEPTH_DEC)
			  || (nml[i].section.iselement(ppsectionids[seclistidx])>=0)) {
				if (nml[i].nmtype==Novelty_Marker::NM_REQUIRED_ITEM) extract_required_item_to_paper(ostrtext,refurl,nml[i],sectitle,requireditems,pprevtext);
				else if (nml[i].nmtype==Novelty_Marker::NM_TCH_DEPTH_INC) extract_hierarchy_depth_increase_to_paper(ostrtext,nml[i]);
				else if (nml[i].nmtype==Novelty_Marker::NM_TCH_DEPTH_DEC) extract_hierarchy_depth_decrease_to_paper(ostrtext,nml[i]);
				else if (nml[i].nmtype==Novelty_Marker::NM_NOVELTY_ITEM) extract_novelty_item_to_paper(ostrtext,nml[i],srcf,srcfname,srcfnum,poutline,pprevtext);
			}
		}
	}
	// add \figureloose commands
	int ofigidx;
	if ((ofigidx = ostrtext.index(BigRegex("[\\][A-Za-z]*figurehere[{]")))>=0) {
		// find loose figures location in outline and copy to there
		int iloosefigidx, ifigend;
		if (((iloosefigidx = istr.index(BigRegex("\n%[ 	]*@Begin FIGURES AT END@"),iread))>=0)
		 && ((ifigend = istr.index(BigRegex("\n%[ 	]*@End FIGURES AT END@[^\n]*\n"),String::SEARCH_END,iread))>=0)) {
			ostrtext += istr.at(iread,iloosefigidx-iread); iread = ifigend;
		} else {
			if ((iloosefigidx = istr.index("\\end{document}",iread))>=0) {
				ostrtext += istr.at(iread,iloosefigidx-iread); iread = iloosefigidx;
			} else {
				EOUT << "dil2al: No \\end{document} found in outline in extract_paper_plan_to_paper(), continuing as is\n";
				ostrtext += istr.from(iread); iread = istr.length();
			}
		}
		// copy \figurehere data to \figureloose commands
		ostrtext += "\n% @Begin FIGURES AT END@\n\n";
		while (ofigidx>=0) {
			if ((ifigend = ostrtext.index("\n",ofigidx))<0) ifigend = ostrtext.length();
			String figdata = ostrtext.at(ofigidx,ifigend-ofigidx);
			figdata.gsub(BigRegex("^\\([\\][A-Za-z]*\\)figurehere[{]"),"_1figureloose{",'_');
			ostrtext += figdata + "\n\n";
			ofigidx = ostrtext.index(BigRegex("[\\][A-Za-z]*figurehere[{]"),ifigend);
		}
		ostrtext += "% @End FIGURES AT END@\n";
	}
	// copy remainder of outline
	if (iread<istr.length()) ostrtext += istr.from(iread);
//*** add new Context items to plan-plans.html file here
cerr << "Add automatic call to add Context items to plan-plans.html file here...\n";
	ostr << ostrtext; // write paper draft file
	ostr.close();
#ifndef DEBUG_DO_NOT_CREATE_NEW_OUTLINE
	if (!backup_and_rename(poutline,"Paper Draft")) return false;
	// find required items list in paper plan, create if not found
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
