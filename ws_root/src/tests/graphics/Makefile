# @(#)Makefile 96/01/25 1.5

include ${CODEMGR_WS}/ws_root/variables/generic

ifeq (${HOSTOS}, IRIX)
DO_NOT_BUILD_IN =      portfolio benchmark morebench pqa test
BUILD_IN_FIRST =  	unix
else
DO_NOT_BUILD_IN =      unix demo test
endif

include ${CODEMGR_WS}/ws_root/rules/subtree
include ${CODEMGR_WS}/ws_root/rules/generic
