# @(#) Makefile 96/02/20 1.3

default_all: all

include ${CODEMGR_WS}/ws_root/variables/hostexecs
include ${CODEMGR_WS}/ws_root/rules/hostexecs

all: ${HOSTOSBIN}/elfsize

${HOSTOSBIN}/elfsize: elfsize.sh
	${RM} -f $@ ; ${CP} elfsize.sh $@
	chmod +x $@

#clean:
#	${RM} -f ${HOSTOSBIN}/elfsize
