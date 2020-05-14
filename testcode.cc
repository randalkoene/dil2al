// testcode.cc
// Randal A. Koene, 20190208

#include "dil2al.hh"

bool test_UI_Entry_Yad() {
  String res(input->request("Testing UI_Entry_Yad\nPlease enter text string here: ",true));
  cout << "res = " << res << '\n';
  cout << "yad_canceled = " << yad_canceled << '\n';
  cout << "yad_pclose_retval = " << yad_pclose_retval << '\n';
  return true;
}

bool test_String_matching() {
  // String matching tests
  String mixedcase("ThIsIsAmIxEdCaSe\n");
  cout << mixedcase;
  cout << downcase(mixedcase);
  cout << mixedcase;
  String bufstr("");
  cout << "(" << bufstr << ") matches BRXint " << bufstr.matches(BRXint) << '\n';
  bufstr = "123";
  cout << "(" << bufstr << ") matches BRXint " << bufstr.matches(BRXint) << '\n';
  bufstr = "hello";
  cout << "(" << bufstr << ") matches BRXint " << bufstr.matches(BRXint) << '\n';
  bufstr = "123.1";
  cout << "(" << bufstr << ") matches BRXint " << bufstr.matches(BRXint) << '\n';
  bufstr = "";
  cout << "(" << bufstr << ") matches BRXdouble " << bufstr.matches(BRXdouble) << '\n';
  bufstr = "123";
  cout << "(" << bufstr << ") matches BRXdouble " << bufstr.matches(BRXdouble) << '\n';
  bufstr = "hello";
  cout << "(" << bufstr << ") matches BRXdouble " << bufstr.matches(BRXdouble) << '\n';
  bufstr = "123.1";
  cout << "(" << bufstr << ") matches BRXdouble " << bufstr.matches(BRXdouble) << '\n';
  return true;
}

bool test_UI_Confirm_multi_request() {
  // UI_Confirm::multi_request() test
  StringList mrdescriptors("Start;End;The Middle",';');
  VOUT << "UI_Confirm::multi_request() = " << confirm->multi_request("Choose [s]tart, [E]nd, or [m]iddle. ","sem",mrdescriptors) << '\n';
  return true;
}

bool test_UI_Progress() {
  // UI_Progress() test
  progress->start();
  progress->update(0,"Progress: ");
  for (int i=0; i<100; i += 4) {
    progress->update(i);
    sleep(1);
  }
  progress->update(100);
  int stopval = progress->stop();
  cout << "progress->stop() returned " << stopval << '\n';
  cout << "Let's wait for input now, so that we can check if the yad process\nis still running even though the pipe was closed.\nPress ENTER...";
  const int LLEN = 16;
  char lbuf[LLEN];
  cin.getline(lbuf,LLEN);
  return true;
}

bool test_UI_Confirm() {
  // UI_Confirm() test
  VOUT << confirm->request("Retry or override? ",'o',"Retry","Override") << '\n';
  if (confirmation("Wanna retry, or (o)verride? ",'o')) VOUT << "YES\n";
  else VOUT << "NO\n";
  return true;
}

bool test_UI_Info() {
  // UI_Info() test
  // UI_Info * info = new UI_Info_Classic();
  INFO.write("A static const char string.\n");
  String s("initialized string\n");
  INFO.write(s.chars()); // INFO.write(s);
  INFO.write(String("Character string plus "+s).chars()); // INFO.write("Character string plus "+s);
  INFO.write(String("This locally defined string\n").chars()); // INFO.write(String("This locally defined string\n"));
  INFO << "Sent as a stream\n";
  //INFO.flush();
  long l = 12345;
  s = "with a string inside";
  INFO << "Sent as a stream " << s << " and a number " << l << '\n';
  //INFO.flush();
  WARN << "And here is a warning about " << (double) 0.35 << " value.\n\nComposed of two lines.\n";
  ERROR << "This is an error message.\nWhile we also want to collect those,\nan effort needs to be made to make sure\nthat these are flushed even if there is\nan early exit().\n";
  return true;
}

bool test_time_stamp_time() {
  // time_stamp_time() test
  VOUT << "time_stamp_time('19990101') test\n";
  time_t ttt = time_stamp_time("19990101");
  VOUT << "ttt = " << ttt << '\n';
  VOUT << "You can only get here when stricttimestamps=false.\n";
  VOUT << "time_stamp_time(' 201901270829') test\n";
  ttt = time_stamp_time(" 201901270829");
  VOUT << "ttt = " << ttt << '\n';
  VOUT << "time_stamp('%Y%m%d%H%M',ttt) = " << time_stamp("%Y%m%d%H%M",ttt) << '\n';
  VOUT << "time_stamp_time('-2') test\n";
  ttt = time_stamp_time("-2");
  VOUT << "ttt = " << ttt << '\n';
  return true;
}

bool test_Get_DIL_ID_File_Superior_TargetDate() {
  // Reading target date codes as presently defined is relatively
  // complex. This test can be used to try the function with
  // any coded target date string. Examples:
  //   Fwe2_201901102330
  //   Ede4_201903122330
  //   Ew201902161700
  //   F201712012300
  //   201712112259
  //   20NONSENSE59
  const char ptstr[][20] = {"daily","workdays","weekly","biweekly","monthly","endofmonthoffset","yearly","oldspan","nonperiodic"};

  VOUT << "Please enter a string. (See examples in testcode.c.)\n";
  VOUT << "Target date coded string to test: ";
  
  const int LLEN = 50;
  char lbuf[LLEN];
  cin.getline(lbuf,LLEN);
  String bufstr(lbuf);

  time_t targetdate = 0;
  int tdprop = 0;
  bool tdex = false;
  periodictask_t periodicity = pt_nonperiodic;
  int span = 0;
  int every = 0;
  int retval = Get_DIL_ID_File_Superior_TargetDate(bufstr,0,bufstr.length(),targetdate,tdprop,tdex,periodicity,span,every);

  VOUT << "\nretval = " << retval;
  if (retval<0) VOUT << " [ERROR]\n";
  else VOUT << " [SUCCESS]\n\n";
  VOUT << "targetdate = " << targetdate << " [" << time_stamp("%Y%m%d%H%M",targetdate) << "]\n";
  VOUT << "tdprop = " << tdprop << " [";
  if (tdprop & DILSUPS_TDPROP_FIXED) {
    if (tdprop & DILSUPS_TDPROP_EXACT) {
      VOUT << "Exact";
    } else {
      VOUT << "Fixed";
    }
  }
  if (tdprop & DILSUPS_TDPROP_PERIODIC) {
    VOUT << ",Periodic";
  }
  VOUT << "]\n";
  if (tdex) VOUT << "tdex = true\n";
  else VOUT << "tdex = false\n";
  VOUT << "periodicity = " << periodicity;
  if ((periodicity>0) && (periodicity<=pt_nonperiodic)) VOUT << " [" << ptstr[periodicity] << "]\n";
  else VOUT << " [UNDEFINED]\n";
  VOUT << "span = " << span << '\n';
  VOUT << "every = " << every << "\n\n";
  return true;
}

bool test_code() {
  // Put any code here that needs to be tested in the context of
  // initialized functions and parameters.

  cout << "Select test:\n";
  cout << "  (1) - String matching\n";
  cout << "  (2) - UI_Confirm_multi_request()\n";
  cout << "  (3) - UI_Progress()\n";
  cout << "  (4) - UI_Confirm()\n";
  cout << "  (5) - UI_Info()\n";
  cout << "  (6) - time_stamp_time()\n";
  cout << "  (7) - test_Get_DIL_ID_File_Superior_TargetDate()\n";
  cout << "  (8) - test UI_Entry_Yad()\n";
  cout << "\nYour selection: ";
  
  const int LLEN = 16;
  char lbuf[LLEN];
  cin.getline(lbuf,LLEN);
  String choice(lbuf);
  delete_all_whitespace(choice);
  if (choice.empty()) {
    cout << "Unrecognized selection\n";
    return false;
  }
  if (!choice.matches(BRXint)) {
    cout << "Unrecognized selection\n";
    return false;
  }
  int selection = atoi(choice);
  switch (selection) {
  case 1:
    test_String_matching();
    break;
  case 2:
    test_UI_Confirm_multi_request();
    break;
  case 3:
    test_UI_Progress();
    break;
  case 4:
    test_UI_Confirm();
    break;
  case 5:
    test_UI_Info();
    break;
  case 6:
    test_time_stamp_time();
    break;
  case 7:
    test_Get_DIL_ID_File_Superior_TargetDate();
    break;
  case 8:
    test_UI_Entry_Yad();
    break;
  default:
    cout << "Unrecognized selection\n";
    return false;
  }
  return true;
}


