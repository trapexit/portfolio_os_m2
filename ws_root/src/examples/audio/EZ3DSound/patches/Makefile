# @(#) Makefile 96/09/06 1.3
# EZ3DSound patches

DELIVER_FILES_TO =	${REMOTE_DIR}/Examples/Audio/EZ3DSound
DELIVER_THESE_FILES =	${filter %.mp,${ALLFILES}} ${filter %.script,${ALLFILES}}
FTP_TRANSFER =		${DELIVER_THESE_FILES:%=${DELIVER_FILES_TO}/%}

include ${CODEMGR_WS}/ws_root/variables/generic
include ${CODEMGR_WS}/ws_root/rules/generic
