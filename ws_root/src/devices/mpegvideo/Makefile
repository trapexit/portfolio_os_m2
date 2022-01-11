# @(#) Makefile 96/12/11 1.17

TARGET_NAME =		${DIRNAME}.driver
TARGET_DIRECTORY =	${REMOTE_DIR}/System.m2/Drivers
DDF_DIRECTORY =		${REMOTE_DIR}/System.m2/Drivers/Descriptions
LOADERTOOL_FLAGS =	-Htype=13 -Hstack=1000 -e main
PRIVILEGED_MODULE =	1
NO_CHDIR_MODULE =	1
SHOW_MODULE_INFO =	1

ifdef DEBUG_MPEGVIDEODRIVER
SRCDEBUG =		1
endif

include ${CODEMGR_WS}/ws_root/variables/linkexec
include ${CODEMGR_WS}/ws_root/rules/linkexec

# Useful debugging defines you can add to CDEFINES to turn on various printf categories:
#
#	-DDEBUG_PRINT_DEV		- Device create/delete/open/close.
#	-DDEBUG_PRINT_UNIT		- Unit detail (hardware regs, etc).
#	-DDEBUG_PRINT_IORW		- I/O detail (reads, writes).
#	-DDEBUG_PRINT_IOCNTL	- I/O detail (control cmds).
#	-DDEBUG_PRINT_BUFALLOC	- Buffer-related allocation stuff.
#	-DDEBUG_PRINT_PARSE		- Happenings in mpVideoParse.c
#	-DDEBUG_PRINT_DECODE	- Happenings in mpVideoDecode.c
#	-DDEBUG_PRINT_ACTIONS	- Happenings in mpVideoActions.c

CDEFINES = -DTHREEDO_M2

ifeq (${BUILD_STRINGS}, YES)
CDEFINES +=	-DDEBUG		# so that debug.h PRNT and PERR macros do something.
endif

all.here: ${DDF_DIRECTORY} ${DDF_DIRECTORY}/mpegvideo.ddf

${DDF_DIRECTORY}/mpegvideo.ddf: ${OBJDIR}/mpegvideo.ddf
	${hardlinkme}

${DDF_DIRECTORY}:
	${MD} $@
