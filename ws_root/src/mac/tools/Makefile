# @(#) Makefile 95/09/22 1.4
# Mac tools Makefile

# Don't build autodepends here or down - avoids generating a ton of meaningless
# errors from trying to build depends on mac code. If anything beneath here actually
# needs to be compiled as part of the OS build, then this line should be removed.
DO_NOT_BUILD_DEPENDS =	1

DO_NOT_BUILD_IN = tcl

include ${CODEMGR_WS}/ws_root/variables/generic
include ${CODEMGR_WS}/ws_root/rules/subtree
include ${CODEMGR_WS}/ws_root/rules/generic
