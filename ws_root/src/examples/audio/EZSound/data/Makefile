# @(#) Makefile 96/09/05 1.2
# EZSound patches

DELIVER_FILES_TO =	${REMOTE_DIR}/Examples/Audio/EZSound
DELIVER_THESE_FILES =	${wildcard *.mp} ${wildcard *.script}
FTP_TRANSFER =		${DELIVER_THESE_FILES:%=${DELIVER_FILES_TO}/%}

include ${CODEMGR_WS}/ws_root/variables/generic
include ${CODEMGR_WS}/ws_root/rules/generic
