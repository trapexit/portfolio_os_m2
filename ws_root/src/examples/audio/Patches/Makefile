# @(#) Makefile 96/03/20 1.6
# Patch examples

DELIVER_FILES_TO =	${REMOTE_DIR}/Examples/Audio/Patches
DELIVER_THESE_FILES =	${wildcard *.mp} ${wildcard *.script}
FTP_TRANSFER =		${DELIVER_THESE_FILES:%=${DELIVER_FILES_TO}/%}
DO_NOT_BUILD_DEPENDS =	1

include ${CODEMGR_WS}/ws_root/variables/generic
include ${CODEMGR_WS}/ws_root/rules/subtree
include ${CODEMGR_WS}/ws_root/rules/generic
