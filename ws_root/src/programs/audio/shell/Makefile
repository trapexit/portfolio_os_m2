# @(#) Makefile 96/09/10 1.10
# non-graphic single-source programs

ifdef MEMDEBUG
    CDEFINES +=		-DMEMDEBUG
endif

TARGET_DIRECTORY =	${REMOTE_DIR}/System.m2/Programs
LINK_LIBS =		music c
MODULES =		audio audiopatchfile
NO_CHDIR_MODULE =	1

include ${CODEMGR_WS}/ws_root/variables/multilink
include ${CODEMGR_WS}/ws_root/rules/multilink
