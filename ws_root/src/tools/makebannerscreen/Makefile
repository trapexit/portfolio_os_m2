# @(#) Makefile 96/08/13 1.2

TARGET_DIRECTORY =	${ROOT}/release/mac/cdrommaster
TARGET =		${TARGET_DIRECTORY}/makebannerscreen

include ${CODEMGR_WS}/ws_root/variables/generic
include ${CODEMGR_WS}/ws_root/rules/generic

${TARGET}: makebannerscreen
	${FIXCOPYR} < $< | ${FIXEOL} > $@

all.post: ${TARGET}
clean:
	-${RM} -rf ${TARGET}
