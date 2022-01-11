# @(#) Makefile 96/08/06 1.52

TARGET_DIRECTORY =	${REMOTE_DIR}/System.m2/Boot
LOADERTOOL_NAME =	operator
LOADERTOOL_FLAGS =	-Hpri=200 -Hstack=2048
STD_MODULES =		operator.init filesystem kernel
EXTRA_SRC =		operator.ddf.c
NOSIG_PRIVED_MODULE =	1
NO_CHDIR_MODULE =	1
SHOW_MODULE_INFO =	1

include ${CODEMGR_WS}/ws_root/variables/linkexec
include ${CODEMGR_WS}/ws_root/rules/linkexec

clean.post:
	${RM} ${EXTRA_SRC}

operator.ddf.c: ${OBJDIR}/operator.ddf
	${HOSTOSBIN}/ddf2c -n operatorDDF -o operator.ddf.c $<
