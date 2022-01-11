# @(#) Makefile 96/02/14 1.8

ifdef MEMDEBUG
    CDEFINES +=		-DMEMDEBUG
endif

TARGET_DIRECTORY =	${REMOTE_DIR}/Tests/Audio
LINK_LIBS =		spmath c
MODULES =		audio
PRIVILEGED_MODULE =	1

include ${CODEMGR_WS}/ws_root/variables/linkexec
include ${CODEMGR_WS}/ws_root/rules/linkexec
