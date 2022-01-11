##############################################################################
#                                                                            #
# @(#) Makefile 96/11/19 1.5                                                            #
#                                                                            #
#           Makefile for use by API developers                               #
#                                                                            #
##############################################################################

#
# If the location of the Alias libraries and header files are
# different from $ALIAS_LOCATION, set it here.
#
# ALIAS_LOCATION=/home/sixpack1/rroopred/conv
# ALIAS_LOCATION=/home/dilbert5/atai/conv
SHELL = /bin/sh

ALIAS_LOCATION=..
WIRE_LOCATION=$(ALIAS_LOCATION)/OpenModel

PLATFORM = SGI
HGRELEASEDIR = ../../release/$(PLATFORM)
CP = /bin/cp

EXAMPLES = $(CEXAMPLES) 

CC = cc
CPLUSPLUS = CC
INCLUDES = -I. -I$(WIRE_LOCATION)/include
LFLAGS = -g -L$(WIRE_LOCATION)/lib

#
# Note that with Alias V5.1 OpenModel is supplied as an Irix5 (ELF)
# library only.
#
# LIBS = -lalias_api -lm -lsun
LIBS = -lalias_api -lapi_memory -lapi_image -lm -lsun

CLIBS = $(LIBS)

#
# Some code is conditionally compiled because it produces a lot of output.
# (see AlPrint.c++ for example) Uncomment the following line if you would
# like to see all the output.
#
# DEBUG = -DDEBUG

CFLAGS = -xansi -signed -prototypes $(INCLUDES) $(DEBUG)
CPLUSPLUSFLAGS = $(CFLAGS)

#
# Rules for building.
#
.c++.o:
	$(CPLUSPLUS) -c -g $(CPLUSPLUSFLAGS) $*.c++

%.o : %.c++
	$(CPLUSPLUS) -c -g $(CPLUSPLUSFLAGS) $<

.c.o:
	$(CC) -c $(CFLAGS) $*.c

.o:
	$(CPLUSPLUS) $(LFLAGS) -o $@ $? $(CLIBS)

.c.i:
	 $(CC) -E $(CFLAGS) $*.c | sed -e '/^[    ]*$$/d' > $*.i

.c++.i:
	$(CPLUSPLUS) -E $(CPLUSPLUSFLAGS) $*.c++ | sed -e '/^[  ]*$$/d' > $*.i

default: $(EXAMPLES)

#
# Rules for building the C++ executables.
#
altosdf:		Main.o		PrintSDF.o
	$(CPLUSPLUS) $(LFLAGS) -o $@ Main.o PrintSDF.o $(CLIBS)
	$(CP) $@ $(HGRELEASEDIR)
clean:
	/bin/rm -f Main.o PrintSDF.o
clobber:
	/bin/rm -f altosdf
