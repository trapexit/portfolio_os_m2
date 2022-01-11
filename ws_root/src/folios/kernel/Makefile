# @(#) Makefile 96/10/21 1.62

TARGET_NAME =		kernel
TARGET_DIRECTORY =	${REMOTE_DIR}/System.m2/Boot
LDFLAGS =		-e main
LINK_LIBS =		loader c
NOSIG_PRIVED_MODULE =	1
STD_MODULES =		filesystem kernel.init
NO_CHDIR_MODULE =	1
SHOW_MODULE_INFO =	1

include ${CODEMGR_WS}/ws_root/variables/linkexec
include ${CODEMGR_WS}/ws_root/variables/version

CDEFINES = -DKERNEL -DOS_VERSION=${OS_VERSION} -DOS_REVISION=${OS_REVISION}

ifeq (${BUILD_DEBUGGER}, YES)
CDEFINES += -DBUILD_IODEBUG
endif

include ${CODEMGR_WS}/ws_root/rules/linkexec
