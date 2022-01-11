# @(#) Makefile 96/03/25 1.4
# cltsurface CLT example

EXAMPLE_CLASS =         Graphics/CLT/CLTSurface
LINK_LIBS =             spmath eventbroker clt c
MODULES =               graphics gstate

ifdef MEMDEBUG
    CDEFINES +=         -DMEMDEBUG
endif

include ${CODEMGR_WS}/ws_root/variables/makeexamples
include ${CODEMGR_WS}/ws_root/variables/linkexec
include ${CODEMGR_WS}/ws_root/rules/subtree
include ${CODEMGR_WS}/ws_root/rules/makeexamples
include ${CODEMGR_WS}/ws_root/rules/linkexec


