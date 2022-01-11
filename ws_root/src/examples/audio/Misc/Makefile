# @(#) Makefile 96/07/31 1.5
# Misc audio examples

ifdef MEMDEBUG
    CDEFINES +=		-DMEMDEBUG
endif

EXAMPLE_CLASS =		Audio/Misc
LINK_LIBS =		music eventbroker spmath c
MODULES =		audio iff
NO_CHDIR_MODULE =	1

include ${CODEMGR_WS}/ws_root/variables/makeexamples
include ${CODEMGR_WS}/ws_root/variables/multilink
include ${CODEMGR_WS}/ws_root/rules/makeexamples
include ${CODEMGR_WS}/ws_root/rules/multilink
