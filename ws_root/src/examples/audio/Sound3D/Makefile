# @(#) Makefile 96/03/20 1.7

EXAMPLE_CLASS =		Audio/Sound3D
LINK_LIBS =		music eventbroker spmath c
MODULES =		audio audiopatch audiopatchfile iff

ifdef MEMDEBUG
    CDEFINES +=		-DMEMDEBUG
endif

include ${CODEMGR_WS}/ws_root/variables/makeexamples
include ${CODEMGR_WS}/ws_root/variables/multilink
include ${CODEMGR_WS}/ws_root/rules/subtree
include ${CODEMGR_WS}/ws_root/rules/makeexamples
include ${CODEMGR_WS}/ws_root/rules/multilink
