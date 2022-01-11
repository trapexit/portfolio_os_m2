# @(#) Makefile 96/06/25 1.3

TARGET_NAME =		${DIRNAME}
TARGET_DIRECTORY =	${REMOTE_DIR}/Tests
DDF_DIRECTORY =		${REMOTE_DIR}/System.m2/Drivers/Descriptions

include ${CODEMGR_WS}/ws_root/variables/linkexec
include ${CODEMGR_WS}/ws_root/rules/linkexec

all.here: ${DDF_DIRECTORY} ${DDF_DIRECTORY}/proxyfile.ddf

${DDF_DIRECTORY}/proxyfile.ddf: ${OBJDIR}/proxyfile.ddf
	${hardlinkme}

${DDF_DIRECTORY}:
	${MD} $@
