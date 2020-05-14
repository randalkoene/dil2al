// wizard.cc
// Randal A. Koene, 20190907

// A collection of wizard guide functions that can be used to take a user through
// common Formalizer System processes.
// See the Formalizer Google Doc and associated documents, such as the
// System - Improving Metrics & AL update support Google Doc.

#include "dil2al.hh"

time_t today_last_year() {
  long daydate_integer = atol(curdate);
  if (daydate_integer<20000000) {
    ERROR << "today_last_year(): Weird date.\n";
    return -1;
  }
  daydate_integer -= 10000; // last year
  return time_stamp_time(String(daydate_integer)+"0000");
}

bool self_eval_metrics() {
  // Integrated implementation of the helper function originally scripted in
  // self-eval-metrics.sh.

  // set some parameters
  mainmetricscategoriesfile=basedir+"lists/formalizer.self-evaluation-categories.rc";
  metricsdayspage=homedir+"tmp/formalizer.self-evaluation-days.csv";
  metricsearliestday=today_last_year();
  // ??? UI_Info * info_backup = info; // keep for later
  // ??? info = new UI_Info_Classic(vout,false);
  // call metrics parsing (-zMAP)
  if (!metrics_parse_task_log("MAP")) {
    ERROR << "self_eval_metrics(): Unable to generate metrics per day per category CSV file.\n";
    return false;
  }
  // ??? delete info;
  // ??? info = info_backup; // restore
  String mapfile(homedir+"tmp/formalizer.metrics-days-categories.map");
  struct stat statbuf;
  if (stat(mapfile,&statbuf)<0) {
    ERROR << "self_eval_metrics(): Unable to stat() MAP file at " << mapfile << ".\n";
    return false;
  }
  long mapsize = statbuf.st_size;
  long numdays = mapsize/1440;
  INFO << "MAP file size : " << mapsize << '\n';
  INFO << "Number of days: " << numdays << " (1440 minutes mapped for each)\n";
  String pngmap(homedir+"tmp/formalizer.metrics-days-categories.png");
  String convertcmd("convert -size 1440x"+String(numdays)+" -depth 2 CMYK:"+mapfile+" "+pngmap);
  if (System_Call(convertcmd)<0) {
    ERROR << "self_eval_metrics(): Unable to compress PNG of MAP file.\n";
    return false;
  }
  INFO << "Compressed PNG of MAP file at " << pngmap << '\n';
  INFO << "To save space, removing MAP file.\n";
  unlink(mapfile);
  String showmetricscmd("{ head -n 1 "+systemselfevaluationdata+"; tail "+systemselfevaluationdata+"; } | column -t -s ',' | zenity --text-info --no-wrap --width=800 --height=400 --font='DejaVu Sans Mono' --title='Self Evaluation Metrics (recent 10 days)' > /dev/null 2>&1 &");
  if (System_Call(showmetricscmd)<0) {
    ERROR << "self_eval_metrics(): Unable to show self evaluation metrics.\n";
    return false;
  }
  String windowcontrolcmd("sleep 2; wmctrl -r 'Self Evaluation Metrics (recent 10 days)' -b add,above");
  if (System_Call(windowcontrolcmd)<0) {
    ERROR << "self_eval_metrics(): Unable to set self evaluation data window on top.\n";
    return false;
  }
  
  return true;
}

bool System_Metrics_Time_Tracking_wizard() {
  // Wizard guide stage called by System_Metrics_AL_morning_wizard().
  // See the System - Improving Metrics & AL update support Google Doc.

  // 1. Make sure the Task Log is up to date before updating System Metrics.
  Task_Log TaskLog;
  TL_entry_content * TLec = NULL;
  if (!(TLec = TaskLog.get_previous_task_log_entry())) {
    ERROR << "System_Metrics_Time_Tracking_wizard(): No previous Task Log entry found.\n";
    return false;
  }
  time_t cursecond = time_stamp_time(curtime);
  if ((cursecond-TLec->_chunkstarttime)>1800) { // Last entry more than 30 minutes ago
    INFO << "Most recent Task Log chunk entry made at " << time_stamp("%Y%m%d%H%M",TLec->_chunkstarttime) << " is more than 30 minutes ago.\nPlease catch up the Task Log for proper System Metrics.\n";
    return false;
  } else {
    INFO << "The Task Log appears to be up to date.\n";
  }

  // 2. Make sure that Metrics and Scores should be recorded for yesterday.
  if (!confirmation("Was yesterday a vacation day (no metrics) (y/N)? ",'y',"yes","NO")) {
    INFO << "...Please manually mark yesterday's Time Tracking as vacation (no data).\n";
    return true;
  }

  // 3. Progress hours values from Task Log as per @ACTTYPE=@ entries.
  INFO << "Progress hours values (@ACTTYPE=@ tags) are not yet extracted automatically.\nPlease do so manually.\n";
  
  // 4. Metrics values from Task Log as per formalizer.self-evaluation-categories.rc.
  INFO << "Showing the Time Tracking table is not yet implemented (and the table still needs to be converted to HTML).\nPlease consult it manually.\n";
  // *** The simple System call approach...
  String selfevalcmd("self-eval-show.sh");
  if (System_Call(selfevalcmd)<0) {
    ERROR << "System_Metrics_Time_Tracking_wizard(): Unable to run and show self evaluation metrics.\n";
    return false;
  }
  // *** if (!self_eval_metrics()) return false;
    
  INFO << "The wizard steps for the Time Tracking stage are not yet implemented.\nPlease carry them out manually.\nSee the Google Doc at https://docs.google.com/document/d/1aIOppQP08vq_0_uiGhPbAXLeaWuOPmHUcH5Qqw3uyks/edit#heading=h.d4sz9rsru89s.\n";

  return true;
}

bool System_Metrics_Daily_wizard() {
  // Wizard guide stage called by System_Metrics_AL_morning_wizard().
  // See the System - Improving Metrics & AL update support Google Doc.

  INFO << "The wizard steps for the Daily stage are not yet implemented.\nPlease carry them out manually.\nSee the Google Doc at https://docs.google.com/document/d/1aIOppQP08vq_0_uiGhPbAXLeaWuOPmHUcH5Qqw3uyks/edit#heading=h.d4sz9rsru89s.\n";

  return true;
}

bool System_Metrics_Nonlinear_BSA_wizard() {
  // Wizard guide stage called by System_Metrics_AL_morning_wizard().
  // See the System - Improving Metrics & AL update support Google Doc.

  INFO << "The wizard steps for the Nonlinear BSA stage are not yet implemented.\nPlease carry them out manually.\nSee the Google Doc at https://docs.google.com/document/d/1aIOppQP08vq_0_uiGhPbAXLeaWuOPmHUcH5Qqw3uyks/edit#heading=h.d4sz9rsru89s.\n";

  return true;
}

bool System_Metrics_Update_Communications_wizard() {
  // Wizard guide stage called by System_Metrics_AL_morning_wizard().
  // See the System - Improving Metrics & AL update support Google Doc.

  INFO << "The wizard steps for the Update Communications stage are not yet implemented.\nPlease carry them out manually.\nSee the Google Doc at https://docs.google.com/document/d/1aIOppQP08vq_0_uiGhPbAXLeaWuOPmHUcH5Qqw3uyks/edit#heading=h.d4sz9rsru89s.\n";

  return true;
}

bool System_Metrics_Update_Outcomes_wizard() {
  // Wizard guide stage called by System_Metrics_AL_morning_wizard().
  // See the System - Improving Metrics & AL update support Google Doc.

  INFO << "The wizard steps for the Update Outcomes stage are not yet implemented.\nPlease carry them out manually.\nSee the Google Doc at https://docs.google.com/document/d/1aIOppQP08vq_0_uiGhPbAXLeaWuOPmHUcH5Qqw3uyks/edit#heading=h.d4sz9rsru89s.\n";

  return true;
}

bool System_Metrics_Update_Positives_wizard() {
  // Wizard guide stage called by System_Metrics_AL_morning_wizard().
  // See the System - Improving Metrics & AL update support Google Doc.

  INFO << "The wizard steps for the Update Positives stage are not yet implemented.\nPlease carry them out manually.\nSee the Google Doc at https://docs.google.com/document/d/1aIOppQP08vq_0_uiGhPbAXLeaWuOPmHUcH5Qqw3uyks/edit#heading=h.d4sz9rsru89s.\n";

  return true;
}

bool System_Morning_AL_Simple_AL_update_wizard() {
  // Wizard guide stage called by System_Metrics_AL_morning_wizard().
  // See the System - Improving Metrics & AL update support Google Doc.

  INFO << "The wizard steps for the Simple AL update stage are not yet implemented.\nPlease carry them out manually.\nSee the Google Doc at https://docs.google.com/document/d/1aIOppQP08vq_0_uiGhPbAXLeaWuOPmHUcH5Qqw3uyks/edit#heading=h.d4sz9rsru89s.\n";

  return true;
}

bool System_Morning_AL_Scheduled_Communications_wizard() {
  // Wizard guide stage called by System_Metrics_AL_morning_wizard().
  // See the System - Improving Metrics & AL update support Google Doc.

  INFO << "The wizard steps for the Scheduled Communications stage are not yet implemented.\nPlease carry them out manually.\nSee the Google Doc at https://docs.google.com/document/d/1aIOppQP08vq_0_uiGhPbAXLeaWuOPmHUcH5Qqw3uyks/edit#heading=h.d4sz9rsru89s.\n";

  return true;
}

bool System_Metrics_AL_morning_wizard() {
  // A wizard function to guide a Formalizer System user through standard
  // morning processes for Metrics and AL updates.
  // See the System - Improving Metrics & AL update support Google Doc.

  // 1) System Metrics: Time Tracking
  if (!System_Metrics_Time_Tracking_wizard()) return false;

  if (!System_Metrics_Daily_wizard()) return false;

  if (!System_Metrics_Nonlinear_BSA_wizard()) return false;

  if (!System_Metrics_Update_Communications_wizard()) return false;

  if (!System_Metrics_Update_Outcomes_wizard()) return false;

  if (!System_Metrics_Update_Positives_wizard()) return false;

  if (!System_Morning_AL_Simple_AL_update_wizard()) return false;

  if (!System_Morning_AL_Scheduled_Communications_wizard()) return false;
  
  return true;
}
