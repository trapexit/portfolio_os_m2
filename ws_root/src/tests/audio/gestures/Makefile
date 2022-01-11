# @(#) Makefile 96/07/08 1.7

ifdef MEMDEBUG
    CDEFINES +=		-DMEMDEBUG
endif

TARGET_DIRECTORY =	${REMOTE_DIR}/Tests/Audio

DELIVER_FILES_TO =      ${TARGET_DIRECTORY}
DELIVER_THESE_FILES =   ${wildcard *.mp}

LINK_LIBS =		audiodemo music clt spmath eventbroker c
MODULES =		audio audiopatchfile font graphics gstate frame2d

include ${CODEMGR_WS}/ws_root/variables/multilink
include ${CODEMGR_WS}/ws_root/rules/multilink
