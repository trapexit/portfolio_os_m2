# @(#) Makefile 96/05/23 1.6
# Build Beep Folio

TARGET_DIRECTORY =	${MODULE_DIR}
PRIVILEGED_MODULE =	1
LOADERTOOL_FLAGS =	-Htype=4 -e main
LINK_LIBS =		dspptouch c
NO_CHDIR_MODULE =	1
MODULE_CALLBACKS =	1
SHOW_MODULE_INFO =	1

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
