# @(#) Makefile 96/06/25 1.8

TARGET_DIRECTORY =	${MODULE_DIR}
LOADERTOOL_FLAGS =	-Htype=4 -e main
MODULES =		audiopatch audio iff
LINK_LIBS =		music c
NO_CHDIR_MODULE =	1
SIGNED_MODULE =		1
SHOW_MODULE_INFO =	1

# this folio requires all of its MODULES all of the time, so consequently it's
# depending on them being opened automatically by the loader when this module
# is loaded.

ifdef MEMDEBUG
    CDEFINES +=		-DMEMDEBUG
endif

include ${CODEMGR_WS}/ws_root/variables/linkexec
include ${CODEMGR_WS}/ws_root/rules/linkexec
