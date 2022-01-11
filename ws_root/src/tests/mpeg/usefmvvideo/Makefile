# @(#) Makefile 96/01/26 1.10

TARGET_DIRECTORY =	${REMOTE_DIR}/Tests/MPEG
UNREGISTERED_FILES = usefmvvideo usefmvvideo.elf

LINK_LIBS =		eventbroker c
MODULES =		graphics

ifdef DEBUG_FMV
SRCDEBUG = 1
CDEFINES = -DDEBUG_PRINT
endif

include ${CODEMGR_WS}/ws_root/variables/linkexec
include ${CODEMGR_WS}/ws_root/rules/linkexec
