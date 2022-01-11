# @(#) Makefile 96/09/12 1.9

TARGET_DIRECTORY =	${MODULE_DIR}
LOADERTOOL_FLAGS =	-Htype=4 -e main
LINK_LIBS =		clt c
MODULES =		font gstate iff fsutils graphics audio international frame2d audiopatchfile date savegame
NO_CHDIR_MODULE =	1
SIGNED_MODULE =		1
SHOW_MODULE_INFO =	1

include ${CODEMGR_WS}/ws_root/variables/linkexec
include ${CODEMGR_WS}/ws_root/rules/subtree
include ${CODEMGR_WS}/ws_root/rules/linkexec
