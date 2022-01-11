# @(#) Makefile 96/03/20 1.3
# Markov Music example

EXAMPLE_CLASS =		Audio/MarkovMusic
LINK_LIBS =		music eventbroker spmath c
MODULES =		audio iff

ifdef MEMDEBUG
    CDEFINES +=		-DMEMDEBUG
endif

include ${CODEMGR_WS}/ws_root/variables/makeexamples
include ${CODEMGR_WS}/ws_root/variables/linkexec
include ${CODEMGR_WS}/ws_root/rules/subtree
include ${CODEMGR_WS}/ws_root/rules/makeexamples
include ${CODEMGR_WS}/ws_root/rules/linkexec
