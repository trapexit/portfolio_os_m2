# @(#) Makefile 96/07/08 1.12
# Single-source programs that use portable graphics

ifdef MEMDEBUG
    CDEFINES +=		-DMEMDEBUG
endif

TARGET_DIRECTORY =	${REMOTE_DIR}/System.m2/Programs
LINK_LIBS =		audiodemo music clt spmath eventbroker c
MODULES =		audio audiopatchfile font graphics gstate iff frame2d
NO_CHDIR_MODULE =	1

include ${CODEMGR_WS}/ws_root/variables/multilink
include ${CODEMGR_WS}/ws_root/rules/multilink
