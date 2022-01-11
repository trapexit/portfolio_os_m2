# @(#) Makefile 96/09/23 1.45

TARGET_NAME =		filesystem
TARGET_DIRECTORY =	${REMOTE_DIR}/System.m2/Boot
NOSIG_PRIVED_MODULE =	1
LOADERTOOL_NAME =	filesystem
LOADERTOOL_FLAGS =	-Hstack=3072 -Hpri=210 -Htype=4 -e main
STD_MODULES =		filesystem.init kernel
NO_CHDIR_MODULE =	1
SHOW_MODULE_INFO =	1

CDEFINES =		-DDODAEMON		\
			-DDOCACHE

EXTRA_SRC =		${SRC}/drivers/file/acrobat/acroutil.c

include ${CODEMGR_WS}/ws_root/variables/linkexec
include ${CODEMGR_WS}/ws_root/rules/linkexec
include ${CODEMGR_WS}/ws_root/rules/extrasrc
