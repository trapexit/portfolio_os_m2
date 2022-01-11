# @(#) Makefile 96/09/05 1.3
# EZSound example

EXAMPLE_CLASS =		Audio/EZSound
LINK_LIBS =		music eventbroker spmath c
MODULES =		audiopatchfile audiopatch audio iff

ifdef MEMDEBUG
    CDEFINES +=		-DMEMDEBUG
endif

include ${CODEMGR_WS}/ws_root/variables/makeexamples
include ${CODEMGR_WS}/ws_root/variables/linkexec
include ${CODEMGR_WS}/ws_root/rules/subtree
include ${CODEMGR_WS}/ws_root/rules/makeexamples
include ${CODEMGR_WS}/ws_root/rules/linkexec
