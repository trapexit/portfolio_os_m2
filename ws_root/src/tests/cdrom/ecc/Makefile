# @(#) Makefile 96/03/21 1.1

TARGET_DIRECTORY =	${REMOTE_DIR}/Tests/CD-ROM

REQUIRE_HIERARCHICAL_INCLUDES = 1

LOADERTOOL_FLAGS := -Hstack=32768

EXTRA_SRC += \
	${CODEMGR_WS}/ws_root/src/others/dipir/m2ecc32.i \
	${CODEMGR_WS}/ws_root/src/others/dipir/m2ecc32.s

include ${CODEMGR_WS}/ws_root/variables/linkexec
include ${CODEMGR_WS}/ws_root/rules/linkexec
include ${CODEMGR_WS}/ws_root/rules/extrasrc

