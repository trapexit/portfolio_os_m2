# @(#) Makefile 96/05/31 1.6
TARGET_DIRECTORY =      ${REMOTE_DIR}/System.m2/Drivers
TARGET_NAME =           ${DIRNAME}.driver
DDF_DIRECTORY =         ${REMOTE_DIR}/System.m2/Drivers/Descriptions
LOADERTOOL_FLAGS =      -Htype=13 -e main
PRIVILEGED_MODULE =     1
NO_CHDIR_MODULE =       1
SHOW_MODULE_INFO =      1
include ${CODEMGR_WS}/ws_root/variables/linkexec
include ${CODEMGR_WS}/ws_root/rules/linkexec
ifeq (${BUILD_PCDEBUGGER},YES)

all.here: ${DDF_DIRECTORY} ${DDF_DIRECTORY}/${DIRNAME}.ddf

${DDF_DIRECTORY}/${DIRNAME}.ddf: ${OBJDIR}/${DIRNAME}.ddf
	${hardlinkme}

${DDF_DIRECTORY}:
	${MD} $@
else
all:
endif
