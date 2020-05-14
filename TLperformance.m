% TLperformance.m
% Randal A. Koene, 20070702

clear;
source("~/.dil2al-TLperformance.m");
printf("Date of performance data  : %d\n",datadate);
printf("Day time interval measured: %.3f hours (i.e. %d minutes)\n",daytelapsed/60,daytelapsed);

hoursworked = dayworked/60;
printf("Day time worked measured  : %.3f hours (i.e. %d minutes)\n\n",hoursworked,dayworked);

% Or use octave's "time()" function and related functions
dataymd = floor(datadate/10000);
gmt = gmtime(time());
compdate=(gmt.year+1900)*10000 + (gmt.mon+1)*100 + gmt.mday;

if (compdate>dataymd)
  dayofweek = gmt.wday-1;
  if (dayofweek<0)
    dayofweek = 6;
  endif
else
  dayofweek = gmt.wday;
endif

if ((dayofweek>=1) && (dayofweek<6))
  typeofdaystr = "workday";
  expectedhours = 8;
else
  typeofdaystr = "weekend";
  expectedhours = 2;
endif

workedratio = hoursworked/expectedhours;

intervalratio = dayworked/daytelapsed;

printf("Hours worked     : %.3f (%d hours per day expected on a %s)\n",hoursworked,expectedhours,typeofdaystr);
printf("Ratio of expected: %.3f = %.3f percent\n",workedratio,workedratio*100);
printf("Ratio of interval: %.3f = %.3f percent\n",intervalratio,intervalratio*100);
