# @(#) Makefile 96/07/03 1.11

TARGET_NAME =		kernel.init
TARGET_DIRECTORY =	${REMOTE_DIR}/System.m2/Boot
LOADERTOOL_NAME =	kernel.init
STD_MODULES =		kernel
NOSIG_PRIVED_MODULE =	1
NO_CHDIR_MODULE =	1

include ${CODEMGR_WS}/ws_root/variables/linkexec
include ${CODEMGR_WS}/ws_root/rules/linkexec

CDEFINES =		-DBUILD_PATH=\"${CODEMGR_WS}\"
