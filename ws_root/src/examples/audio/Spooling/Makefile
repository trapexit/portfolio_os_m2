# @(#) Makefile 96/03/20 1.16
# spooling and sound player examples

EXAMPLE_CLASS =		Audio/Spooling
LINK_LIBS =		music eventbroker spmath c
MODULES =		audio iff

ifdef MEMDEBUG
    CDEFINES +=		-DMEMDEBUG
endif

include ${CODEMGR_WS}/ws_root/variables/makeexamples
include ${CODEMGR_WS}/ws_root/variables/multilink
include ${CODEMGR_WS}/ws_root/rules/subtree
include ${CODEMGR_WS}/ws_root/rules/makeexamples
include ${CODEMGR_WS}/ws_root/rules/multilink
