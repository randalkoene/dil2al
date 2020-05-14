#
# Makefile for dil2al tests
# (by Randal A. Koene, randalk@marina.psych.mcgill.ca)
#
# Instructions to insure a successful make:
# 1. Insure that LIB_PATH points to the directory in which
#    -X11 and -Xext libraries can be found
# 2. Edit the include paths (e.g. -I/usr/openwin/include) in
#    CPPFLAGS to refelect the correct location of g++, -X11
#    and -Xext include files
# 3. Make sure the dynamic shared libraries can be located by
#    ld.so.1 at runtime (e.g. define an environment variable
#    setenv LD_LIBRARY_PATH "/usr/openwin/lib")
#
#ARCH=-DNON_INTEL # Specify non-Intel platform to avoid __asm instructions

#if you have a Pentium computer, then tell me how fast it is
# MHZ=133000000 # Irrelevant on Sun Solaris

# PATH for -X11 -Xext
LIB_PATH=#-L/usr/openwin/lib


# if you do not have a pentium comment that out !!
# MHZ is necessary for correct timing. 
# Assign the value 1.0/133000000.0 for a 133 MHZ Pentium
# RDTSC= -DPENTIUM -DMHZ=1.0/133000000.0 # Irrelevant on Sun Solaris

# what compiler ?
CC=gcc
CCPP=g++ 

# aurora:
# settings for use with rx.h
#ALT_REGEX=rx.o
#ALT_REGEX_H=
# settings for use with regex-gnu (from sed)
ALT_REGEX=regex-gnu.o
ALT_REGEX_H=\"regex-gnu.h\"
# settings for use with generic GNU regex
#ALT_REGEX=regex.o
#ALT_REGEX_H=regex.h
# kenji:
# settings for use of system <regex.h>
#ALT_REGEX=
#ALT_REGEX_H=

#### compiler switches
# aurora:
# use regex-gnu.h
CFLAGS= -g -D__DIL2AL__ -DSTDC_HEADERS
# use rx.h
#CFLAGS= -g -D__DIL2AL__
# with malloc, realloc and free instead of alloca
#CFLAGS= -g -D__DIL2AL__ -DREGEX_MALLOC
# kenji:
# use rx.h with Sun specific features
#CFLAGS= -g -DSUN_RX -I/opt/FSFlibg++/lib/g++-include
# use system <regex.h> with Sun specific features
#CFLAGS= -g -DSYSTEM_RX -DSUN_RX -I/opt/FSFlibg++/lib/g++-include
# aurora:
# use system <regex.h>
#CPPFLAGS= -g -DSYSTEM_RX -D__DIL2AL__ -D_BIGREGEX_SAFE_MATCHES -felide-constructors -fnonnull-objects -I/opt/FSFlibg++/lib/g++-include
# use rx.h
#CPPFLAGS= -g -D__DIL2AL__ -D_BIGREGEX_SAFE_MATCHES -felide-constructors -fnonnull-objects -I/opt/FSFlibg++/lib/g++-include
# use C++ compatible regex-gnu.h or other alternative
CPPFLAGS= -g -D_ALT_REGEX_H=$(ALT_REGEX_H) -D_USE_ALT_REGEX -D_CPP_REGEX -D__DIL2AL__ -D_BIGREGEX_SAFE_MATCHES -felide-constructors -fnonnull-objects -I/opt/FSFlibg++/lib/g++-include
# use regex-gnu.h or other alternative
#CPPFLAGS= -g -D_ALT_REGEX_H=$(ALT_REGEX_H) -D_USE_ALT_REGEX -D__DIL2AL__ -D_BIGREGEX_SAFE_MATCHES -felide-constructors -fnonnull-objects -I/opt/FSFlibg++/lib/g++-include
# use rx.h without special Regex safety precautions
#CPPFLAGS= -g -felide-constructors -fnonnull-objects -I/opt/FSFlibg++/lib/g++-include
# optimized
#CPPFLAGS= -O3 -felide-constructors -fnonnull-objects -I/opt/FSFlibg++/lib/g++-include
# kenji:
# use rx.h
#CPPFLAGS= -g -D__DIL2AL__ -D_BIGREGEX_SAFE_MATCHES -D_BIGREGEX_HAS_RM_SPEP -DSUN_RX -felide-constructors -fnonnull-objects -I/opt/FSFlibg++/lib/g++-include
# optimized with debug setting
#CPPFLAGS= -g -O6 -I/opt/FSFlibg++/lib/g++-include -I/usr/openwin/include
# generate profile information for use with gprof
#CPPFLAGS= -pg -g -I/opt/FSFlibg++/lib/g++-include -I/usr/openwin/include

JOYSTICK= #-DJOYSTICK
# Turn on HICOLOR by uncommenting below, if your X color class
# supports color depth greater than 8 bit.
COLORS= #-DHICOLOR

# FIREBUTTON=-DCONTROL_SHOOTS

# below there is no need to change

# TIMER= #-DTIMER -DCPS="1.0/($(MHZ))"  #-DPLAY
# Timer uses RDTSC (Intel specific!), therefore it is only available for Pentium or higher computers

all: regextest regextestc

BigRegex.o: BigRegex.cc BigRegex.hh rx.h regex.h regex-gnu.h
	$(CCPP) $(CPPFLAGS) $(COLORS) -c BigRegex.cc -o BigRegex.o

regex.o: regex.c regex.h
	$(CC) $(CFLAGS) -c regex.c -o regex.o

regex-gnu.o: regex-gnu.c regex-gnu.h
	$(CCPP) $(CFLAGS) -c regex-gnu.c -o regex-gnu.o

rx.o: rx.c rx.h
	$(CC) $(CFLAGS) $(COLORS) -c rx.c -o rx.o

regextest.o: regextest.c BigRegex.hh rx.h regex.h regex-gnu.h
	$(CCPP) $(CPPFLAGS) -c regextest.c -o regextest.o

regextestc.o: regextestc.c BigRegex.hh rx.h regex.h regex-gnu.h
	$(CC) $(CFLAGS) -c regextestc.c -o regextestc.o

#ranlib.o: ranlib.c ranlib.h
#	$(CCPP) $(CPPFLAGS) $(COLORS) -c ranlib.c -o ranlib.o

#spiker: ranlib.o spiker.o main.o
#	$(CCPP) $(CPPFLAGS) main.o spiker.o ranlib.o -o spiker $(LIB_PATH) -lX11 -lXext

regextest: regextest.o BigRegex.o rx.o regex.o regex-gnu.o
	$(CCPP) $(CPPFLAGS) regextest.o $(ALT_REGEX) BigRegex.o -o regextest $(LIB_PATH)
#	$(CCPP) $(CPPFLAGS) regextest.o $(ALT_REGEX) BigRegex.o -o regextest $(LIB_PATH)

regextestc: regextestc.o rx.o regex.o regex-gnu.o
	$(CC) $(CFLAGS) regextestc.o $(ALT_REGEX) -o regextestc $(LIB_PATH)

clean:
	rm -r -f *.o $(NAME)
#	rm -r -f html

doc++:
	rm -r -f html
	doc++ -d html spiker.dxx

