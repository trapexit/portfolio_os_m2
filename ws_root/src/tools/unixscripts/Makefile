# @(#)Makefile 95/02/27 1.2

TARGET_DIRECTORY =	${HOSTOSBIN}
TARGETS =		${SHSRC:%.sh=${TARGET_DIRECTORY}/%}

include ${CODEMGR_WS}/ws_root/variables/generic
include ${CODEMGR_WS}/ws_root/rules/generic

${TARGET_DIRECTORY}/%: %.sh
	@${RM} $@
	${CP} $< $@
	${CHMOD} a+x $@

all.post: ${TARGETS}

clean:
	${RM} ${TARGETS}
