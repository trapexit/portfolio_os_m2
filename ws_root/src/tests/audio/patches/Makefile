# @(#) Makefile 96/06/20 1.3
# test patch scripts for makepatch

DELIVER_FILES_TO =	${REMOTE_DIR}/Tests/Audio/Patches
DELIVER_THESE_FILES =	${wildcard *.mp} ${wildcard *.pimap}
FTP_TRANSFER =		${DELIVER_THESE_FILES:%=${DELIVER_FILES_TO}/%}

include ${CODEMGR_WS}/ws_root/variables/generic
include ${CODEMGR_WS}/ws_root/rules/generic
