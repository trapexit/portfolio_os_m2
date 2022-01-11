# @(#) Makefile 96/09/10 1.1
# playmf

ifdef MEMDEBUG
    CDEFINES +=		-DMEMDEBUG
endif

TARGET_DIRECTORY =	${REMOTE_DIR}/System.m2/Programs
LINK_LIBS =		music eventbroker spmath c
MODULES =		audio audiopatchfile iff
NO_CHDIR_MODULE =	1
LOADERTOOL_FLAGS =	-Hpri=150

include ${CODEMGR_WS}/ws_root/variables/linkexec
include ${CODEMGR_WS}/ws_root/rules/linkexec
