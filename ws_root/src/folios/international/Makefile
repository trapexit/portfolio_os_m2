# @(#) Makefile 96/05/23 1.34

TARGET_DIRECTORY =	${MODULE_DIR}
LOADERTOOL_FLAGS =	-Htype=4 -e main
LINK_LIBS =		c
MODULES =		jstring date batt
PRIVILEGED_MODULE =	1
NO_CHDIR_MODULE =	1
MODULE_CALLBACKS =	1
SHOW_MODULE_INFO =	1

CDEFINES =		-DINTLFOLIO_PRIVATE

include ${CODEMGR_WS}/ws_root/variables/linkexec
include ${CODEMGR_WS}/ws_root/rules/linkexec
