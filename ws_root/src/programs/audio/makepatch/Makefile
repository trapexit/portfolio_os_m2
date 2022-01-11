# @(#) Makefile 96/03/01 1.7
# makepatch

ifdef MEMDEBUG
    CDEFINES +=		-DMEMDEBUG
endif

TARGET_DIRECTORY =	${REMOTE_DIR}/System.m2/Programs
LINK_LIBS =		music spmath c
MODULES =		audio audiopatch audiopatchfile iff
NO_CHDIR_MODULE =	1

include ${CODEMGR_WS}/ws_root/variables/linkexec
include ${CODEMGR_WS}/ws_root/rules/linkexec
