// usage.cc
// Randal A. Koene, 20190214

/*
  This source file provides the usage/help text displayed by dil2al/formalizer
  programs.

  NOTE: It's tempting to use something like usage_dil2al.txt for a pure text
  file and to convert that to a .cc with xxd in the Makefile, but that is not
  as flexible as the \n\ delimited char arrays below, because it would not be
  possible as easily to dynamically compile in definitions for OWNER,
  DILBYIDFILE and DEFAULTEDITOR.
 */

#include "dil2al.hh"

const char * _dil2al_usage = "\
Usage: dil2al [-s] [-o <owner>] [-c <DIL-filename>] [-n <DIL-filename>]\n\
              [-t<tag-id>] [-N] [-i[tc|te] [-I]] [-f <data-file>]\n\
              [-O <verbose-output-file>] [-A[c][S][Whh:mm][Tmaxt|numtcs]] [-m[note-filename]]\n\
              [-E <editor>] [-C[autocommands]] [-S[autocommands]] [-u<n|y|a>]\n\
              [-V[flag command]] [-a <DIL ID>] [-l <DIL ID>] [-x <file+plan>]\n\
              [-X <plan+outline+previous>] [-F <figure<.eps|.fig|.ps>>] [-q<x>]\n\
              [-M<modification>] [-U] [-p<param>=<value>] [-T <YYYYmmddHHMM>] [-Q]\n\
              [-H[@f@][type]<DIL ID>[cContentFile][#expressions]]\n\
              [-e[@f@][type]<DIL ID>[cContentFile][#expressions]] [-P]\n\
              [-D<diagnostic>] [-g <+search-key+target>]\n\
              [-b <DIL ID>@<output file>] [-z[label]] [-1] [-2]\n\
              [-j <DIL ID>]\n\
       dil2al [-s] [-o <owner>] [-c <DIL-filename>] [-d <DIL-filename>]\n\
              [-D<diagnostic>]\n\
              [-t<tag-id>] [-N] [-i[tc|te] [-I]] [-f <data-file>]\n\
              [-O <verbose-output-file>] [-A[c][S][Whh:mm][Tmaxt|numtcs]] [-m[note-filename]]\n\
              [-E <editor>] [-C[autocommands]] [-S[autocommands]] [-u<n|y|a>]\n\
              [-V[flag command]] [-a <DIL ID>] [-l <DIL ID>] [-x <file+plan>]\n\
              [-X <plan+outline+previous>] [-F <figure<.eps|.fig|.ps>>] [-q<x>]\n\
              [-M<modification>] [-U] [-p<param>=<value>] [-T <YYYYmmddHHMM>] [-Q]\n\
              [-H[@f@][type]<DIL ID>[cContentFile][#expressions]]\n\
              [-e[@f@][type]<DIL ID>[cContentFile][#expressions]] [-P]\n\
              [-D<diagnostic>] [-g <+search-key+target>]\n\
              [-b <DIL ID>@<output file>] [-z[label]] [-1] [-2]\n\
              [-j <DIL ID>]\n\
       dil2al -h|-?\n\
                      -s = silent, run non-verbosely\n\
                      -o = specify DIL owner\n\
                           (compiled default: " OWNER ")\n\
                      -c = create new DIL (e.g. ltp.html)\n\
                      -n = add new DIL entries to DIL-by-ID file\n\
                           (" DILBYIDFILE ")\n\
                      -d = send a DIL entry to a DIL file\n\
                      -t = generate standard tag\n\
                      -N = generate a new DIL ID\n\
                      -i = initialize a new DIL entry\n\
                           tc = initialize a new TL chunk\n\
                           te = initialize a new TL entry\n\
                      -I = include content input in initialized entry\n\
                      -f = obtain content data from file instead of stdin\n\
                      -O = send verbose and error output to file\n\
                      -A = update Active Lists with DILs (and # of TCs)\n\
                           (-A0 = complete AL, -Acx = show cumulative time\n\
                           required, -ASx = strict, -AEx = expand\n\
                           WHH:MM = worked, TYYYYmmddHHMM = max time\n\
                           or n = number of TCs to geneate where 0 is\n\
                           unlimited)\n\
                      -P = create pool for plan-led rescheduling\n\
                      -m = make ``note'' (for TL or elsewhere)\n\
                           optionally include text from note-filename\n\
                      -E = specify a text editor\n\
                           (compiled default: " DEFAULTEDITOR ")\n\
                      -C = task controller (with optional command string)\n\
                      -S = scheduled task controller (and command string)\n\
                      -u = automatic task update strategy (no/yes/ask)\n\
                      -V = visual and audible flag\n\
                      -a = generate AL entry\n\
                      -l = generate full AL entry\n\
                      -x = extract NOVELTY tagged items from ``file'' to\n\
                           ``plan'' (if ``plan''==``'' to stdout)\n\
                      -X = extract NOVELTY content into ``outline''\n\
                           according to ``plan'', retaining elements\n\
                           already rewritten in ``previous''\n\
                      -F = generate Figures Directory entry\n\
                      -q = carry out quantitative assessment `x'\n\
                      -M = modify element\n\
                           (-MEcXXXXXXXXXXXXXX.Y=R.r = entry completion ratio,\n\
                           -MEcXXXXXXXXXXXXXX.Y=V.v = entry valuation,\n\
                           -MEtXXXXXXXXXXXXXX.Y=YYYYmmddHHMM = target date,\n\
                           -MEtgroup[direct] = group target dates)\n\
                      -U = update unlimited periodic target dates\n\
                      -K = skip unlimited periodic tasks to (emulated) date\n\
                      -p = set parameter (multiple permitted)\n\
                      -T = ``current'' time to emulate\n\
                      -Q = refresh Quick Load Cache\n\
                      -H = visualize DIL hierarchy, type=FORM, HTML, GRAPH,\n\
                           GRAPHJSON, GRAPHCYTO or none (==plain text) tabbed\n\
                           format, `DILID'c`contentfile', @f@=hierarchyfile\n\
                           (DILID+DILID+DILID optional with FORM type),\n\
                           #expressions that must occur in DIL entries\n\
                      -e = extract DIL metrics, type=LIST,\n\
                           none (meaning LIST) format,\n\
                           `DILID'c`contentfile', @f@=metricsfile\n\
                           (DILID+DILID+DILID optional with FORM type),\n\
                           #expressions that must occur in DIL entries\n\
                      -g = grep target for search-key (2* escapes separator)\n\
                           set followlinks and searchprogressmeter with -p\n\
                           can use @<regex>@ at end of target, multiple\n\
                           targets indicated by separator\n\
                      -1 = generate a first call page (for dil2al -T? -S)\n\
                      -2 = generate a 5 min intervals first call page\n\
                      -z = generate metrics per day per category CSV\n\
                           or for the @label=X.Y@ values found\n\
                      -b = build a task history page\n\
                      -D = diagnostic test\n\
                             cache = test caching\n\
                             test  = test code\n\
                      -j = JSON request API, return Node data for <DIL ID>\n\
\n\
More information at ~/doc/html/lists.html and ~/doc/html/style-guide.html.\n\
Further information is currently only provided by the in-source comments for\n\
each function. As the documentation effort proceeds, more information will\n\
be collected in the Google doc Formalizer.\n\
Another good place to start is the README file in the source directory.\n\
\n\
Also runnable as `formalizer` when link exists, see `formalizer -h`.\n\
\n\
The order of command line arguments can be relevant. For example:\n\
\n\
  dil2al -p \"UI_Info=UI_CLASSIC\" -h\n\
\n\
sends usage/help information to the classic output handler, while:\n\
\n\
  dil2al -h -p \"UI_Info=UI_CLASSIC\"\n\
\n\
produces usage/help output before changing the UI_Info parameter.\n\
This is only relevant for a select few commands. Most commands are collected\n\
before being executed in an order specified in dil2al.cc, and will modify\n\
parameters as requested before the commands are carried out.\n\
\n\
When dil2al is called from an HTML FORM, the following commands are\n\
available (through either GET or POST interfaces):\n\
\n\
     ?dil2al=<command>\n\
\n\
     where <command> is an available command specifier.\n\
\n\
     Command specifiers:\n\
\n\
     A[cSE][W<HH:MM>][T<maxtime>|<numtcs>] = generate AL\n\
              c = show cumulative time required\n\
              S = strict, no automatic expansion\n\
              E = strict, automatic expansion\n\
              W = time worked on current day, HH hours, MM minutes\n\
              T = maximum date-time to generate AL to (YYYYmmddHHMM)\n\
              <numtcs> = a number of task chunks to generate\n\
     D = extract all checked DIL IDs to dilref file\n\
     M[E<t[group]Ii>] = modify DIL entry(ies)\n\
              E =  target date or element\n\
                   t = target date, possibly of group\n\
                   I = element via FORM interface\n\
                   i = element as specified by FORM data\n\
     U = update unlimited periodic target date tasks\n\
     K = skip unlimited periodic target date tasks\n\
     P = Objective Oriented Planning\n\
     g = RegEx search\n\
\n\
     &<argument-label>__EQ__<argument-data>\n\
\n\
     where <argument-label> is an available command and\n\
     <argument-data> specifies the arguments for that command.\n\
\n\
     Argument labels:\n\
\n\
     setpar = modify a configurable parameter (like -p above)\n\
              E.g:\n\
                   &setpar__EQ__ctrshowall\n\
                   &setpar__EQ__&setpar__EQ__alwide=999999\n\
";

const char * _formalizer_usage = "\
Usage: formalizer [-f<n|m>]\n\
       formalizer -h|-?\n\
                      -f = finances options\n\
                           n = financial note (financial-notes.html)\n\
                           m = monthly results (financial-notes.html)\n\
                      -w = wizard guide options\n\
                           m = morning System wizard\n\
\n\
Also runnable as `dil2al` when link exists, see `dil2al -h`.\n\
\n\
See the dil2al usage information for more.\n\
";
