# @(#) Makefile 96/09/10 1.4
# score player examples

EXAMPLE_CLASS =		Audio/Score
LINK_LIBS =		music eventbroker spmath c
MODULES =		audio audiopatchfile iff
NO_CHDIR_MODULE =	1

EXTRA_SRC =		${SRC}/programs/audio/playmf/playmf.c

include ${CODEMGR_WS}/ws_root/variables/makeexamples
include ${CODEMGR_WS}/ws_root/variables/multilink
include ${CODEMGR_WS}/ws_root/rules/subtree
include ${CODEMGR_WS}/ws_root/rules/makeexamples
include ${CODEMGR_WS}/ws_root/rules/multilink
include ${CODEMGR_WS}/ws_root/rules/extrasrc
