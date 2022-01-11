# @(#) Makefile 96/05/23 1.60
# Build audiofolio

TARGET_DIRECTORY =	${MODULE_DIR}
LOADERTOOL_FLAGS =	-Htype=4 -e main
PRIVILEGED_MODULE =	1
NO_CHDIR_MODULE =	1
MODULE_CALLBACKS =	1
SHOW_MODULE_INFO =	1

MODULES =		iff
LINK_LIBS =		audioprivate dspptouch music c
# music is necessary for <audio/music_iff.h> stuff

include ${CODEMGR_WS}/ws_root/variables/linkexec

# Conditionally turn on DSPP disassembler.
ifdef DSPP_DISASM
    CDEFINES +=		-DDSPP_DISASM
endif

# Enable simulator if requested.
ifdef SIMULATE_DSPP
    CDEFINES +=		-DSIMULATE_DSPP
endif

# Enable memdebug if requested.
ifdef MEMDEBUG
    CDEFINES +=		-DMEMDEBUG
endif

include ${CODEMGR_WS}/ws_root/rules/subtree
include ${CODEMGR_WS}/ws_root/rules/linkexec
