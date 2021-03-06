#
# Makefile for dil2al
# (by Randal A. Koene, randalk@minduploading.org)
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

##########################################################
## Compiler
## --------
CC=gcc
CCPP=g++ 
##########################################################

INCLUDES= -I$(HOME)/src/include

##########################################################
## Machine selection
## -----------------
## aurora (x86):
MACHSPEC=
GCCTUNE=$(if $(shell gcc -v 2>&1 | grep "^gcc version[^0-9]*[4-9][.]"),-mtune=pentium4)
MACHOPT=
#MACHOPT=$(GCCTUNE) -mfpmath=sse
##
## kenji (SUN Ultra):
#MACHSPEC= -DSUN_RX
#MACHSTR=SUN
##
##########################################################

### Attempting a temporary fix for gcc 4.x compilation of dil2al
### (This should be replaced by actual source code modification.)
### Also, attempting a temporary fix for gcc 7.1+ compilation of dil2al
### (This should also be replaced by actual source code modification.)
CPPTEMPWORKAROUND=-fno-access-control -Wno-format-truncation
#RXSPECIAL=-Wno-error

##########################################################
## Regular Expression Library
## --------------------------
## (Suggestion: Using regex-gnu.h guarantees the greatest
## amount of compatibility and identical behaviour on all
## platforms, since it is included with the dil2al source
## code and has been adapted for reliable use with C++.)
##
## regex-gnu.h adapted for integration with C++:
GCC3FXTRA=$(if $(shell gcc -v 2>&1 | grep "^gcc version[^0-9]*[3-9][.]"),-Wno-unused-function)
ALT_REGEX=regex-gnu.o
ALT_REGEX_H=\"regex-gnu.h\"
CFXTRA= -D__DIL2AL__ -DSTDC_HEADERS -pedantic -Wall -Werror -Wno-char-subscripts $(GCC3FXTRA)
RXCFXTRA= -D__DIL2AL__ -DSTDC_HEADERS -pedantic -Wall $(GCC3FXTRA)
CPPXTRA= -D__DIL2AL__ -D_ALT_REGEX_H=$(ALT_REGEX_H) $(MACHOPT) -mieee-fp -ffast-math -D_USE_ALT_REGEX -D_CPP_REGEX -DDEFAULTHOMEDIR=\"$(HOME)\" -pedantic -Wall -Werror $(GCC3FXTRA) $(CPPTEMPWORKAROUND)
REGEXSTR=regex-gnu_for_C++
##
## rx.h:
#ALT_REGEX=rx.o
#ALT_REGEX_H=
#CFXTRA= -D__DIL2AL__
#CPPXTRA= -D__DIL2AL__ -D_BIGREGEX_HAS_RM_SPEP
#REGEXSTR=rx
##
## generic GNU regex (Linux/GNU specific):
#ALT_REGEX=regex.o
#ALT_REGEX_H=regex.h
#CFXTRA=
#CPPXTRA= -D_ALT_REGEX_H=$(ALT_REGEX_H) -D_USE_ALT_REGEX -D__DIL2AL__ -D_BIGREGEX_HAS_RM_SPEP
#REGEXSTR=regex
##
## system <regex.h> (SUN specific):
#ALT_REGEX=
#ALT_REGEX_H=
#CFXTRA= -DSYSTEM_RX
#CPPXTRA= -D__DIL2AL__ -DSYSTEM_RX
#REGEXSTR=system
##########################################################

##########################################################
## Safe Regular Expressions
## ------------------------
## assume '\0' within String length and rm structure has
## no sp/ep pointers:
#SAFEREGEX=
## make no assumptions:
SAFEREGEX= -D_BIGREGEX_SAFE_MATCHES
##########################################################

##########################################################
## Compiler Options
## ----------------
## debugging information (updates 2019-02-01):
#COMPOPT= -g -Og
#OPTSTR=editing_optimized_for_debugging
## generate profile information for use with gprof:
#COMPOPT= -g -O6
#OPTSTR=profiling
## optimized (USUALLY USE THIS ONE!):
COMPOPT= -O3
OPTSTR=optimized
## optimized + debugging info (as per GDB recommendations 2019-02-01)
#COMPOPT= -g -O3
#OPTSTR=production_optimized_with_dbg_symbols
##########################################################

##########################################################
## C++ Specific Compiler Options
## -----------------------------
## debugging information:
#CPPOPT=
## generate profile information for use with gprof:
#CPPOPT=
## optimized:
CPPOPT= -felide-constructors
## uncomment the following optimization option only if
## you have an older C++ compiler that does not optimize
## return values unless explicitly told to do so with
## the now deprecated `named return value' extension:
#CPPOPT= $(CPPOPT) -D_USE_NAMED_RETURN_VALUE_EXTENSION
##########################################################

CFLAGS= $(COMPOPT) $(MACHSPEC) $(CFXTRA)
RXCFLAGS= $(COMPOPT) $(MACHSPEC) $(RXCFXTRA)
CPPFLAGS= $(COMPOPT) $(CPPOPT) $(MACHSPEC) $(CPPXTRA) $(SAFEREGEX) $(INCLUDES)

.INIT:
	@echo '-------------------------------------------------------------------'
	@echo 'Compilation options: $(MACHSTR), $(OPTSTR), $(REGEXSTR)'
	@echo '-------------------------------------------------------------------'

all: dil2al

dil2al.o: dil2al.cc dil2al.hh BigString.hh BigRegex.hh rx.h regex.h regex-gnu.h
	$(CCPP) $(CPPFLAGS) $(COLORS) -c dil2al.cc -o dil2al.o

alcomp.o: alcomp.cc dil2al.hh BigString.hh BigRegex.hh rx.h regex.h regex-gnu.h
	$(CCPP) $(CPPFLAGS) $(COLORS) -c alcomp.cc -o alcomp.o

diladmin.o: diladmin.cc dil2al.hh BigString.hh BigRegex.hh rx.h regex.h regex-gnu.h
	$(CCPP) $(CPPFLAGS) $(COLORS) -c diladmin.cc -o diladmin.o

utilities.o: utilities.cc dil2al.hh BigString.hh BigRegex.hh rx.h regex.h regex-gnu.h
	$(CCPP) $(CPPFLAGS) $(COLORS) -c utilities.cc -o utilities.o

interface.o: interface.cc dil2al.hh BigString.hh BigRegex.hh rx.h regex.h regex-gnu.h
	$(CCPP) $(CPPFLAGS) $(COLORS) -c interface.cc -o interface.o

tladmin.o: tladmin.cc dil2al.hh BigString.hh BigRegex.hh rx.h regex.h regex-gnu.h
	$(CCPP) $(CPPFLAGS) $(COLORS) -c tladmin.cc -o tladmin.o

note.o: note.cc dil2al.hh BigString.hh BigRegex.hh rx.h regex.h regex-gnu.h
	$(CCPP) $(CPPFLAGS) $(COLORS) -c note.cc -o note.o

controller.o: controller.cc dil2al.hh BigString.hh BigRegex.hh rx.h regex.h regex-gnu.h
	$(CCPP) $(CPPFLAGS) $(COLORS) -c controller.cc -o controller.o

tlfilter.o: tlfilter.cc dil2al.hh BigString.hh BigRegex.hh rx.h regex.h regex-gnu.h
	$(CCPP) $(CPPFLAGS) $(COLORS) -c tlfilter.cc -o tlfilter.o

ppfilter.o: ppfilter.cc dil2al.hh BigString.hh BigRegex.hh rx.h regex.h regex-gnu.h
	$(CCPP) $(CPPFLAGS) $(COLORS) -c ppfilter.cc -o ppfilter.o

search.o: search.cc dil2al.hh BigString.hh BigRegex.hh rx.h regex.h regex-gnu.h
	$(CCPP) $(CPPFLAGS) $(COLORS) -c search.cc -o search.o

finances.o: finances.cc dil2al.hh BigString.hh BigRegex.hh rx.h regex.h regex-gnu.h
	$(CCPP) $(CPPFLAGS) $(COLORS) -c finances.cc -o finances.o

wizard.o: wizard.cc dil2al.hh BigString.hh BigRegex.hh rx.h regex.h regex-gnu.h
	$(CCPP) $(CPPFLAGS) $(COLORS) -c wizard.cc -o wizard.o

testcode.o: testcode.cc dil2al.hh BigString.hh BigRegex.hh rx.h regex.h regex-gnu.h
	$(CCPP) $(CPPFLAGS) $(COLORS) -c testcode.cc -o testcode.o

cksum.o: cksum.cc cksum.hh BigString.hh rx.h regex.h regex-gnu.h
	$(CCPP) $(CPPFLAGS) $(COLORS) -c cksum.cc -o cksum.o

usage.o: usage.cc dil2al.hh BigString.hh BigRegex.hh rx.h regex.h regex-gnu.h
	$(CCPP) $(CPPFLAGS) $(COLORS) -c usage.cc -o usage.o

BigString.o: BigString.cc BigString.hh
	$(CCPP) $(CPPFLAGS) $(COLORS) -c BigString.cc -o BigString.o

BigRegex.o: BigRegex.cc BigRegex.hh rx.h regex.h regex-gnu.h
	$(CCPP) $(CPPFLAGS) $(COLORS) -c BigRegex.cc -o BigRegex.o

regex.o: regex.c regex.h
	$(CC) $(RXCFLAGS) -c regex.c -o regex.o

regex-gnu.o: regex-gnu.c regex-gnu.h
	$(CCPP) $(RXCFLAGS) -c regex-gnu.c -o regex-gnu.o

rx.o: rx.c rx.h
	$(CC) $(RXCFLAGS) $(COLORS) -c rx.c -o rx.o

dil2al: dil2al.o alcomp.o diladmin.o utilities.o interface.o tladmin.o note.o controller.o tlfilter.o ppfilter.o search.o finances.o wizard.o testcode.o cksum.o usage.o BigString.o BigRegex.o regex-gnu.o regex.o rx.o
	$(CCPP) $(CPPFLAGS) dil2al.o alcomp.o diladmin.o utilities.o interface.o tladmin.o note.o controller.o tlfilter.o ppfilter.o search.o finances.o wizard.o testcode.o cksum.o usage.o $(ALT_REGEX) BigString.o BigRegex.o -o dil2al $(LIB_PATH)

clean:
	cp -f dil2al dil2al.bak
	rm -r -f *.o $(NAME)

doc++:
	rm -r -f html
	doc++ -d html spiker.dxx

