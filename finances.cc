// finances.cc
// Randal A. Koene, 20020905

#include "dil2al.hh"

/*
  These functions provide formalizer support for finances planning,
  administration and accounting.

  See <A HREF="dil2al.hh">dil2al.hh</A>.
*/

int Financial_Note::get(String & s, time_t & t, int sloc) {
  // get a financial note item from a string
  BigRegex trx("[[]\\([^]]+\\)[]]");
  String tr,trp;
  sloc = HTML_get_table_row(s,sloc,trp,tr);
  trp.upcase();
  handled = !trp.contains("BGCOLOR");
  StringList tc; int tclen = 0;
  for (int j = 0; j>=0; ) {
    j = HTML_get_table_cell(tr,j,trp,tc[tclen]);
    if (j>=0) tclen++;
  }
  int i = 0;
  if ((tclen>2) && (tc[0].index(BRXint)>=0)) {
    // get a new date
    String datestr(tc[0].sub(BRXint,0)+"0000");
    t = time_stamp_time_date(datestr.before(12));
    i++;
  }
  date = t;
  note = tc[i];
  expense = 0.0;
  int e = -1, l = 0;
  for (int eend = 0; eend>=0; ) { // get last double in note (expenses must contain a decimal point)
    eend = note.index(BRXdouble,String::SEARCH_END,eend);
    if (eend>=0) if (note.sub(BRXdouble,0).contains('.')) { e = BRXdouble.subpos(); l = BRXdouble.sublen(); }
  }
  if (e>=0) {
    expense = atof(String(note.at(e,l)));
    if (e>0) if (note[e-1]=='$') { e--; l++; }
    note.del(e,l);
  }
  if (note.index(trx)<0) type = "unknown";
  else {
    type = note.sub(trx,1);
    note.del(trx.subpos(),trx.sublen());
  }
  while ((!note.empty()) && (note.lastchar()==' ')) note.del((int) note.length()-1,1);
  i++;
  if (tclen>i) comment=tc[i];
  while ((!comment.empty()) && (comment.lastchar()=='\n')) comment.del((int) comment.length()-1,1);
  return sloc;
}

void Financial_Note::link_sorted(PLLRoot<Financial_Note> & r) {
  // link this financial note to r in order sorted by date
  PLL_LOOP_FORWARD(Financial_Note,r.head(),1) {
    if (e->Date()>Date()) {
      r.insert_before(e,this); // sorted by date
      return;
    }
  }
  r.link_before(this); // append at end
}

String Financial_Note::put() const {
  const String hstr(" BGCOLOR=\"#FF5F5F\"");
  const String nullstr("");
  const String * rp;
  if (handled) rp=&nullstr; else rp=&hstr;
  String datecell("");
  if ((!Prev()) || ((Prev()->Date()!=Date()))) {
    long notesondate = 1;
    PLL_LOOP_FORWARD(Financial_Note,Next(),Date()==e->Date()) notesondate++;
    datecell = HTML_put_table_cell(" ROWSPAN="+String(notesondate),time_stamp("%Y%m%d",Date()),true)+'\n';
  }
  return HTML_put_table_row(*rp,datecell+HTML_put_table_cell("",note+" $"+String(expense,"%.2f")+" ["+type+']',true)+HTML_put_table_cell("",comment,true),true)+'\n';
}

bool read_financial_notes_file(PLLRoot<Financial_Note> & fnotes, String & finnotestop, String & finnotesend) {
  String finnotesstr;
  // read financial notes file
  if (!read_file_into_String(finnotesfile,finnotesstr)) return false;
  // extract financial notes table
  if (verbose) INFO << "Parsing Financial Notes file\n";
  int i, j;
  if ((i=finnotesstr.index("<!-- dil2al: start items -->",String::SEARCH_END,0))<0) {
    ERROR << "read_financial_notes_file(): Missing start of financial notes items table.\n";
    return false;
  }
  if ((j=finnotesstr.index("<!-- dil2al: end items -->",0))<0) {
    ERROR << "read_financial_notes_file(): Missing end of financial notes items table.\n";
    return false;
  }
  finnotestop = finnotesstr.before(i)+'\n';
  finnotesend = finnotesstr.from(j);
  finnotesstr.del(j,finnotesstr.length()-j);
  finnotesstr.del(0,i); // finnotesstr contains table
  // extract financial notes items while keeping track of dates and sorting by date
  time_t notedate = Time(NULL);
  for (i=0; i>=0; ) {
    Financial_Note * fn = new Financial_Note(finnotesstr,notedate,i);
    if (i<0) delete fn;
    else fn->link_sorted(fnotes);
  }
  return true;
}

bool write_financial_notes_file(PLLRoot<Financial_Note> & fnotes, String & finnotestop, String & finnotesend) {
  PLL_LOOP_FORWARD(Financial_Note,fnotes.head(),1) finnotestop += e->put();
  finnotestop += finnotesend;
  return write_file_from_String(finnotesfile,finnotestop,"Financial notes");
}

bool financial_notes() {
  // Edit notes in <A HREF="../../doc/html/financial-notes.html">financial-notes.html</A>.
  
  PLLRoot<Financial_Note> fnotes; String finnotestop, finnotesend;
  if (!read_financial_notes_file(fnotes,finnotestop,finnotesend)) return false;
  
  // add new notes (currently just by hand)
  Financial_Note * f;
  time_t d; double e; bool h; String dstr(curdate), t, n, c;
  d = time_stamp_time_date(dstr+"0000");
  while (1) {
    n = input->request("date ["+dstr+"] (done=exit): ",false);
    if (!n.empty()) {
      if (n=="done") break;
      dstr = n;
      n += "0000"; n.del(12,n.length()-12);
      d = time_stamp_time_date(n);
    }
    e = atof(input->request("expense: ",false));
    t = input->request("type [variable]: ",false);
    if (t.empty()) t = "variable";
    if (t==String("variable")) h = true;
    else h = !confirmation("handled [n]: ",'y',"NO","Yes");
    n = input->request("note: ",false);
    c = input->request("comment: ",false);
    f = new Financial_Note(d,e,t,h,n,c);
    f->link_sorted(fnotes);
  }
  
  // write financial notes file
  return write_financial_notes_file(fnotes,finnotestop,finnotesend);
}

Financial_Monthly_SubCategory * Financial_Monthly_SubCategory::Find_SubCategory(String s) const {
  if (subcategory==s) return const_cast<Financial_Monthly_SubCategory *>(this);
  if (Next()) return Next()->Find_SubCategory(s);
  return NULL;
}

Financial_Monthly_Category * Financial_Monthly_Category::Find_Category(String c) const {
  if (category==c) return const_cast<Financial_Monthly_Category *>(this);
  if (Next()) return Next()->Find_Category(c);
  return NULL;
}

void Financial_Monthly_Category::Copy_SubCategories(Financial_Monthly_Category * c) {
  if (c==this) return; // no other category to copy from
  PLL_LOOP_FORWARD(Financial_Monthly_SubCategory,c->subcategories.head(),1) {
    Financial_Monthly_SubCategory * fs = new Financial_Monthly_SubCategory(e->SubCategory());
    subcategories.link_before(fs);
  }
}

void Financial_Monthly_Category::add(String s, double e, Financial_Monthly & month) {
  // add to result and by subcategory
  
  result += e;
  if (s.index(BRXidentifier)<0) s = "general";
  else s = s.sub(BRXidentifier,0);
  Financial_Monthly_SubCategory * fs = NULL;
  if (subcategories.head()) fs = subcategories.head()->Find_SubCategory(s);
  if (!fs) { // add subcategory in this category in all months
    PLL_LOOP_FORWARD(Financial_Monthly,month.head(),1) {
      Financial_Monthly_Category * fc = e->categories.head()->Find_Category(category);
      fs = new Financial_Monthly_SubCategory(s);
      fc->subcategories.link_before(fs);
    }
    fs = subcategories.tail(); // retrieve subcategory in this month and category
  }
  
  // add to subcategory result
  fs->add(e);
}

Financial_Monthly * Financial_Monthly::Find_Month(String m) const {
  if (month==m) return const_cast<Financial_Monthly *>(this);
  if (Next()) return Next()->Find_Month(m);
  return NULL;
}

void Financial_Monthly::Copy_Categories(Financial_Monthly * h) {
  if (h==this) return; // no other month to copy from
  PLL_LOOP_FORWARD(Financial_Monthly_Category,h->categories.head(),1) {
    Financial_Monthly_Category * fc = new Financial_Monthly_Category(e->Category());
    categories.link_before(fc);
    fc->Copy_SubCategories(e);
  }
}

void Financial_Monthly::add(const Financial_Note & fn) {
  // add by category
  
  String cstr = fn.Type();
  if (cstr.index(BRXidentifier)<0) cstr = "variable";
  else cstr = cstr.sub(BRXidentifier,0);
  Financial_Monthly_Category * fc = NULL;
  if (categories.head()) fc = categories.head()->Find_Category(cstr);
  if (!fc) { // add category in all months
    PLL_LOOP_FORWARD(Financial_Monthly,head(),1) {
      fc = new Financial_Monthly_Category(cstr);
      e->categories.link_before(fc);
    }
    fc = categories.tail(); // retrieve category in this month
  }
  
  // add to category result and by subcategory
  fc->add(fn.Type().after(','),fn.Expense(),*this);
}

void Financial_Monthly_Root::add(const Financial_Note & fn) {
  // add by month
  
  String mstr = time_stamp("%Y%m",fn.Date());
  Financial_Monthly * fm = NULL;
  if (head()) fm = head()->Find_Month(mstr);
  if (!fm) { // add month
      fm = new Financial_Monthly(mstr);
      link_before(fm);
      fm->Copy_Categories(head());
  }
  
  // check for new categories and subcategories
  // add by category and subcategory within month
  fm->add(fn);
}

String Financial_Monthly_Root::put_results() const {
  String res;
  PLL_LOOP_FORWARD_NESTED(Financial_Monthly,head(),1,m) { // month
    res += "Month: " + m->Month() + '\n';
    PLL_LOOP_FORWARD_NESTED(Financial_Monthly_Category,m->categories.head(),1,c) { // category
      res += "<TABLE BORDER=1 WIDTH=\"100%\">\n<TR>";
      // headings
      PLL_LOOP_FORWARD(Financial_Monthly_SubCategory,c->subcategories.head(),1) { // subcategory
	res += "<TH>" + e->SubCategory();
      }
      res += "<TH>" + c->Category() + "\n<TR>";
      // results
      PLL_LOOP_FORWARD(Financial_Monthly_SubCategory,c->subcategories.head(),1) { // subcategory
	res += "<TD>" + String(e->Result(),"%.2f");
      }
      res += "<TH><B>" + String(c->Result(),"%.2f") + "</B>\n</TABLE>\n";
    }
  }
  
  return res;
}

bool financial_monthly() {
  // Compute results from <A HREF="../../doc/html/financial-notes.html">financial-notes.html</A>.
  
  PLLRoot<Financial_Note> fnotes; String finnotestop, finnotesend;
  if (!read_financial_notes_file(fnotes,finnotestop,finnotesend)) return false;
  
  // compute sums of categorized expenses that have not previously been handled
  Financial_Monthly_Root fmonthly;
  if (verbose) INFO << "If you wish to rely on monthly results from the Financial Notes, please\nverify that all non-variable expenses listed in bank statements are\naccounted for in the Financial Notes.\n\n";
  PLL_LOOP_FORWARD(Financial_Note,fnotes.head(),1) {
    if (!e->Handled()) fmonthly.add(*e);
  }
  INFO << fmonthly.put_results();
  
  return true;
}
