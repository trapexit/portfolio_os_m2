# @(#) Makefile 96/07/09 1.3
# audiomon

ifdef MEMDEBUG
    CDEFINES +=		-DMEMDEBUG
endif

TARGET_DIRECTORY =	${REMOTE_DIR}/System.m2/Programs
LINK_LIBS =		audiodemo clt c
MODULES =		audio font graphics gstate frame2d
NO_CHDIR_MODULE =	1
LOADERTOOL_FLAGS =	-Hpri=110

include ${CODEMGR_WS}/ws_root/variables/linkexec
include ${CODEMGR_WS}/ws_root/rules/linkexec
