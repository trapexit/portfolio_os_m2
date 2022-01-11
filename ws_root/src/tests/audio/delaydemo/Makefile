# @(#) Makefile 96/09/17 1.3
# delay line template demo

TARGET_DIRECTORY =	${REMOTE_DIR}/Tests/Audio/DelayDemo
TARGET_NAMES =		${R64FILES:%.R64=%}
DELIVER_FILES_TO =	${TARGET_DIRECTORY}
DELIVER_THESE_FILES =	${filter %.mp %.pimap %.script,${ALLFILES}}
FTP_TRANSFER =		${DELIVER_THESE_FILES:%=${DELIVER_FILES_TO}/%}

include ${CODEMGR_WS}/ws_root/variables/datafiles
include ${CODEMGR_WS}/ws_root/rules/datafiles
