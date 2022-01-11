# @(#) Makefile 96/08/19 1.2
# basic CLT example with clipping

EXAMPLE_CLASS =         Graphics/CLT/CLTspinclip
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


