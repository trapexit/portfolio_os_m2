#  @(#) Makefile 96/04/23 1.33

.KEEP_STATE:

#==========================
MDIR=./misc/
TDIR=./symtest/
NDIR=./symbolics/
UDIR=./symbolics/utils/
DDIR=./symbolics/in_the_dumps/
#should merge these files with kernel's!
KDIR=./symbolics/from_the_kernel
LDIR=./symbolics/linker/
#XDIR=./symbolics/xcoff/
#ADIR=./symbolics/arm/
RDIR=./symbolics/ar/
EDIR=./symbolics/elf/
#SDIR=./symbolics/xsym/
CDIR= ../../folios/compression/
HINCDIRS    = ${SRC}/includes

#GCC=/usr/local/bin/gcc-2.5.8
GCC=gcc -D_NEW_GCC

#DEBUG=-g -DDEBUG -DTEST
#DEBUG=-g -DDEBUG
#ODIR=./sun4gd/
#DEBUG=-g
#ODIR=./sun4g/
DEBUG=
ODIR=./sun4/


LINK=link3do
DUMP=dump3do
#DEFS=-DLINKER -DDUMPER -D_SUN4 -D_GCC -DOLDLIB
DEFS=-DLINKER -DDUMPER -D_SUN4 -D_GCC

UINCS= $(UDIR)utils.h $(UDIR)dumputils.h $(UDIR)option.h $(UDIR)parse.h
DINCS= $(DDIR)dumpopts.h $(DDIR)symdump.h $(DDIR)3dodump.pch++
KINCS= $(KDIR)loaderty.h $(KDIR)nodes.h $(KDIR)header3do.h
LINCS= $(LDIR)linkopts.h $(LDIR)3dolink.pch++
SINCS= $(NDIR)symapi.h $(NDIR)symnet.h $(NDIR)symnodes.h
MINCS= $(MDIR)debug.h $(MDIR)predefines.h
RINCS= $(RDIR)ar.h $(EDIR)elf_ar.h
EINCS= $(EDIR)dwarf.h $(EDIR)elf.h $(EDIR)elf_d.h $(EDIR)elf_l.h \
	$(EDIR)elf_r.h $(EDIR)elf_3do.h $(EDIR)elf_utils.h

UOBJS= $(ODIR)dump_utils.o $(ODIR)option.o $(ODIR)parse.o $(ODIR)utils.o\
	$(ODIR)misc.o
DOBJS= $(ODIR)dump.o
LOBJS= $(ODIR)link.o
SOBJS= $(ODIR)sym_api.o $(ODIR)sym_net.o \
	$(ODIR)sym_nodes.o $(ODIR)sym_utils.o $(ODIR)sym_dump.o $(ODIR)ppc_disasm.o
EOBJS= $(ODIR)elf_attrs.o $(ODIR)elf_d.o $(ODIR)elf_l.o \
	$(ODIR)elf_r.o $(ODIR)elf_tags.o $(ODIR)elf_utils.o \
	$(ODIR)elf_libs.o $(ODIR)elf_objs.o $(ODIR)elf_3do.o
ROBJS= $(ODIR)ar_d.o $(ODIR)elf_ar.o
COBJS= $(ODIR)compress.o

INCS= -I$(DDIR) -I$(LDIR) -I$(MDIR) -I$(UDIR) -I$(TDIR) -I$(NDIR) -I$(EDIR) -I$(KDIR) -I$(RDIR) -I$(CDIR)
#INCS= -I$(DDIR) -I$(LDIR) -I$(MDIR) -I$(UDIR) -I$(TDIR) -I$(NDIR) -I$(EDIR) -I$(KDIR) -I$(RDIR) -I/usr/include -I/usr/include/sys

DUMPOBJS= $(DOBJS) $(UOBJS) $(SOBJS) $(EOBJS) $(ROBJS) $(COBJS)
LINKOBJS= $(LOBJS) $(UOBJS) $(SOBJS) $(EOBJS) $(ROBJS) $(COBJS)

CC=$(GCC) $(DEFS) $(DEBUG) -c $(INCS)
CCP=$(GCC) -x c++ $(DEFS) $(DEBUG) -c $(INCS)

LD=$(GCC) $(DEBUG) -lgcc -lobjc

#==========================
all:	$(ODIR) $(LINK) $(DUMP)

default: all

depends:

#==========================
#dump3do:

$(DUMP):	$(DUMPOBJS)
	$(LD) $(DUMPOBJS) -o $@
	@strip $@

$(ODIR)dump.o: $(DDIR)dump.cpp
	$(CCP) -c $(DDIR)dump.cpp -o $@

$(ODIR)sym_dump.o: $(DDIR)sym_dump.cpp
	$(CCP) -c $(DDIR)sym_dump.cpp -o $@

$(ODIR)ppc_disasm.o: $(DDIR)ppc_disasm.cpp
	$(CCP) -c $(DDIR)ppc_disasm.cpp -o $@

#==========================
#link3do:

$(LINK): $(LINKOBJS) Makefile
	$(LD) $(LINKOBJS) -o $@
	@strip $@

$(ODIR)link.o: $(LDIR)link.cpp
	$(CCP) -c $(LDIR)link.cpp -o $@

#==========================
#sym:

$(ODIR)GetSymbolics.o: $(NDIR)GetSymbolics.cp
	$(CCP) -c $(NDIR)GetSymbolics.cp -o $@

$(ODIR)sym_api.o: $(NDIR)sym_api.cpp
	$(CCP) -c $(NDIR)sym_api.cpp -o $@

$(ODIR)sym_net.o: $(NDIR)sym_net.cpp
	$(CCP) -c $(NDIR)sym_net.cpp -o $@

$(ODIR)sym_nodes.o: $(NDIR)sym_nodes.cpp
	$(CCP) -c $(NDIR)sym_nodes.cpp -o $@

$(ODIR)sym_utils.o: $(NDIR)sym_utils.cpp
	$(CCP) -c $(NDIR)sym_utils.cpp -o $@

#==========================
#utils:

$(ODIR)dump_utils.o: $(UDIR)dump_utils.cpp
	$(CCP) -c $(UDIR)dump_utils.cpp -o $@

$(ODIR)option.o: $(UDIR)option.cpp
	$(CCP) -c $(UDIR)option.cpp -o $@

$(ODIR)parse.o: $(UDIR)parse.cpp
	$(CCP) -c $(UDIR)parse.cpp -o $@

$(ODIR)utils.o: $(UDIR)utils.cpp
	$(CCP) -c $(UDIR)utils.cpp -o $@

$(ODIR)misc.o: $(UDIR)misc.cpp
	$(CCP) -c $(UDIR)misc.cpp -o $@

#==========================
#ar:

$(ODIR)elf_ar.o: $(EDIR)elf_ar.cpp
	$(CCP) -c $(EDIR)elf_ar.cpp -o $@

$(ODIR)ar_d.o: $(RDIR)ar_d.cpp
	$(CCP) -c $(RDIR)ar_d.cpp -o $@

#==========================
#compress:

$(ODIR)compress.o: $(CDIR)compress.c
	$(CC) -DNO_PORTFOLIO -I$(HINCDIRS) -c $(CDIR)compress.c -o $@

#==========================
#elf:

$(ODIR)elf_attrs.o: $(EDIR)elf_attrs.cpp
	$(CCP) -c $(EDIR)elf_attrs.cpp -o $@

$(ODIR)elf_d.o: $(EDIR)elf_d.cpp
	$(CCP) -c $(EDIR)elf_d.cpp -o $@

$(ODIR)elf_l.o: $(EDIR)elf_l.cpp
	$(CCP) -c $(EDIR)elf_l.cpp -o $@

$(ODIR)elf_r.o: $(EDIR)elf_r.cpp
	$(CCP) -c $(EDIR)elf_r.cpp -o $@

$(ODIR)elf_tags.o: $(EDIR)elf_tags.cpp
	$(CCP) -c $(EDIR)elf_tags.cpp -o $@

$(ODIR)elf_utils.o: $(EDIR)elf_utils.cpp
	$(CCP) -c $(EDIR)elf_utils.cpp -o $@

$(ODIR)elf_3do.o: $(EDIR)elf_3do.cpp
	$(CCP) -c $(EDIR)elf_3do.cpp -o $@

$(ODIR)elf_libs.o: $(EDIR)elf_libs.cpp
	$(CCP) -c $(EDIR)elf_libs.cpp -o $@

$(ODIR)elf_objs.o: $(EDIR)elf_objs.cpp
	$(CCP) -c $(EDIR)elf_objs.cpp -o $@

#==========================
test:
	$(TDIR)FAKE.A
	$(TDIR)etest.dat
	$(TDIR)link.elf
	$(TDIR)link.map
	$(TDIR)link.out
	$(TDIR)stest.dat
	$(TDIR)sym_natest.cpp
	$(TDIR)sym_natest_all.cpp
	$(TDIR)t.c
	$(TDIR)t.o
	$(TDIR)t.out
	$(TDIR)t2.c
	$(TDIR)t2.o
	$(TDIR)test.c
	$(TDIR)test2.c
	$(TDIR)xstest.dat
	$(TDIR)xtest.dat

$(ODIR):
	-mkdir $(ODIR)

#==========================
clean:
	rm -rf $(ODIR)*

#==========================
orphans:
	@echo "!!! target 'orphans' not implemented in src/tools/link."




