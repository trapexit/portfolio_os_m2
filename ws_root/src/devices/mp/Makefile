# @(#) Makefile 96/05/31 1.3

TARGET_NAME =		${DIRNAME}.driver
TARGET_DIRECTORY =	${REMOTE_DIR}/System.m2/Drivers
DDF_DIRECTORY =		${REMOTE_DIR}/System.m2/Drivers/Descriptions
LOADERTOOL_FLAGS =	-Htype=13 -e main
PRIVILEGED_MODULE =	1
NO_CHDIR_MODULE =	1
SHOW_MODULE_INFO =	1

include ${CODEMGR_WS}/ws_root/variables/linkexec
include ${CODEMGR_WS}/ws_root/rules/linkexec

all.here: ${DDF_DIRECTORY} ${DDF_DIRECTORY}/mp.ddf

${DDF_DIRECTORY}/mp.ddf: ${OBJDIR}/mp.ddf
	${hardlinkme}

${DDF_DIRECTORY}:
	${MD} $@
