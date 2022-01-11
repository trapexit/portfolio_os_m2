#
# @(#) Makefile 96/11/19 1.2
#
SHELL = /bin/sh

HGRELEASEDIR = ../../release/$(PLATFORM)

CP = cp
RM = /bin/rm -f
M2TXLIB = M2TXlib.a
IFFLIB = IFFlib.a
LIBS = ../../lib/$(M2TXLIB) ../../lib/$(IFFLIB) -lm
INCLUDES = -I../../application/include  -I../../application/include/IFF -I../../conv
CC = gcc
CCFLAGS = -ansi -O -g -DTEXTOOLS_VERISION="\"2.0d2\"" $(INCLUDES)

RELEASE = \
	 utfpipcat utfpipsub utfquantmany utfsplit utfunpage utfcat

$(RELEASE):
	$(CC) $(CCFLAGS) $(INCLUDES) -o bin/$@ $@.c ${LIBS}
	$(CP) bin/$@ $(HGRELEASEDIR)

utfpage: futil.o SDFTexBlend.o utfpage.c
	$(CC) $(CCFLAGS) $(INCLUDES) -o bin/$@ $@.c SDFTexBlend.o futil.o ${LIBS}
	$(CP) bin/$@ $(HGRELEASEDIR)

all:  $(RELEASE) utfpage

depend:
	makedepend -- $(INCLUDES)

.c.o:   $*.c
	$(CC) $(CCFLAGS) $(INCLUDES) -c $*.c 

clean:
	$(RM) $(RELEASE) utfpage

# DO NOT DELETE THIS LINE -- make depend depends on it.








