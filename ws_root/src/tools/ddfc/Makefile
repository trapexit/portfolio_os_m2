# @(#) Makefile 95/12/01 1.5

default_all: all

include ${CODEMGR_WS}/ws_root/variables/hostexecs
include ${CODEMGR_WS}/ws_root/rules/hostexecs

all: ${HOSTOSBIN}/ddfc ${HOSTOSBIN}/ddf2c

${HOSTOSBIN}/ddfc: ddfc.sh ${HOSTOSBIN}/ddfc1
	${CP} $< $@
	chmod 755 $@

${HOSTOSBIN}/ddf2c: ddf2c.sh
	${CP} $< $@
	chmod 755 $@

${HOSTOSBIN}/ddfc1: ${OBJDIR}/ddfc.yo
	${HOSTCC} -o $@ $^ ${LINK_LIBS_DIR:%=-L%}

${OBJDIR}/%.yo: %.y
	yacc $<
	mv y.tab.c $*.c
	${HOSTCC} -c -o $@ ${HOSTCFLAGS} -I. $*.c
	-@${RM} $*.c

clean:
	-@${RM} ${HOSTOSBIN}/ddfc1
