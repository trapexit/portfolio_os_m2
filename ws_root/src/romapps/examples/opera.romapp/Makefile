# @(#) Makefile 96/04/30 1.1

TARGET_DIRECTORY = ${REMOTE_DIR}/RomApps
LINK_LIBS     = clt romapp c
MODULES	      = font graphics gstate frame2d
SIGNED_MODULE = 1

EXTRA_SRC += \
	${CODEMGR_WS}/ws_root/src/romapps/examples/discjuggler/event.c

include ${CODEMGR_WS}/ws_root/variables/linkexec
include ${CODEMGR_WS}/ws_root/rules/linkexec
include ${CODEMGR_WS}/ws_root/rules/extrasrc
