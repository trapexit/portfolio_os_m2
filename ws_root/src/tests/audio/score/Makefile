# @(#) Makefile 96/03/01 1.6

ifdef MEMDEBUG
    CDEFINES +=		-DMEMDEBUG
endif

TARGET_DIRECTORY =	${REMOTE_DIR}/Tests/Audio
LINK_LIBS =		music spmath c
MODULES =		audio audiopatchfile iff

include ${CODEMGR_WS}/ws_root/variables/multilink
include ${CODEMGR_WS}/ws_root/rules/multilink
