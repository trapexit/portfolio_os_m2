# @(#) Makefile 95/11/03 1.12

UNREGISTERED_FILES = asp2y.h

default_all: all

include ${CODEMGR_WS}/ws_root/variables/hostexecs
include ${CODEMGR_WS}/ws_root/rules/hostexecs

# For my convenience...
T=${HOSTOSBIN}

# For now we just put asp1 and asp2 in /usr/software/bin.
# Later we will get jas there as well.
#MORE_TARGS=$T/jas $T/jas.debug $T/asp1 $T/asp2
MORE_TARGS=$T/jas $T/jas.debug

all: ${MORE_TARGS}

$T/jas: jas
	echo '#!/bin/sh' > $@
	echo 'B=/usr/software/bin' >> $@
	cat $< >> $@
	chmod +x $@

$T/jas.debug: jas.debug
	echo '#!/bin/sh' > $@
	echo 'B=/usr/software/bin' >> $@
	cat $< >> $@
	chmod +x $@

$T/asp1: ${OBJDIR}/asp1.lo
	${HOSTCC} -o $@ $< -ll

$T/asp2: ${OBJDIR}/asp2.yo ${OBJDIR}/asp2.lo
	${HOSTCC} -o $@ $^ -ly -ll

${OBJDIR}/%.yo: %.y
	yacc -d $<
	mv y.tab.h $(*F)y.h
	${HOSTCC} -c -o $@ ${HOSTCFLAGS} y.tab.c
	rm -f y.tab.c

${OBJDIR}/%.lo: %.l
	lex $<
	${HOSTCC} -c -o $@ ${HOSTCFLAGS} lex.yy.c
	rm -f lex.yy.c

clean:
	-@${RM} asp2y.h ${MORE_TARGS}
