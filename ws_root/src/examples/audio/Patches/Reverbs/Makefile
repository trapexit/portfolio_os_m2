# @(#) Makefile 95/12/14 1.2
# Reverb examples

DELIVER_FILES_TO =	${REMOTE_DIR}/Examples/Audio/Patches/Reverbs
DELIVER_THESE_FILES =	${wildcard *.mp} ${wildcard *.script}
FTP_TRANSFER =		${DELIVER_THESE_FILES:%=${DELIVER_FILES_TO}/%}

include ${CODEMGR_WS}/ws_root/variables/generic
include ${CODEMGR_WS}/ws_root/rules/generic
