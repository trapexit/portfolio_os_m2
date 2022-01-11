# @(#) Makefile 96/04/03 1.1

TARGET_DIRECTORY =	${REMOTE_DIR}/Tests/CD-ROM
PRIVILEGED_MODULE =     1

REQUIRE_HIERARCHICAL_INCLUDES = 1

LINK_LIBS =	clt eventbroker c
MODULES = 	frame2d gstate font graphics
LOADERTOOL_FLAGS := -Hstack=32768

include ${CODEMGR_WS}/ws_root/variables/linkexec
include ${CODEMGR_WS}/ws_root/rules/linkexec

