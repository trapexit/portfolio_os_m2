# @(#) Makefile 95/10/27 1.1

IMPORTED_FILES = iff.h iff.c io.c
UNREGISTERED_FILES = ${IMPORTED_FILES}

include ${CODEMGR_WS}/ws_root/variables/generic
include ${CODEMGR_WS}/ws_root/rules/generic

MAC_FIXINCLUDES = 	${HOSTOSBIN}/fix_includes_mac
MAC_FIXEOL =	  	${HOSTOSBIN}/fix_eol_mac

iff.h:
	cat ${SRC}/includes/misc/iff.h | ${MAC_FIXINCLUDES} | ${MAC_FIXEOL} > $@

iff.c:
	cat ${SRC}/folios/iff/iff.c | ${MAC_FIXINCLUDES} | ${MAC_FIXEOL} > $@

io.c:
	cat ${SRC}/folios/iff/io.c | ${MAC_FIXINCLUDES} | ${MAC_FIXEOL} > $@

all.post: ${IMPORTED_FILES}

clean:
	${RM} ${IMPORTED_FILES}
