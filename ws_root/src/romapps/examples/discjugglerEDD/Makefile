# @(#) Makefile 96/03/08 1.8

TARGET_DIRECTORY = ${REMOTE_DIR}/RomApps
LINK_LIBS     = clt romapp c
MODULES	      = font graphics gstate frame2d
SIGNED_MODULE = 1

EXTRA_SRC += \
	${CODEMGR_WS}/ws_root/src/romapps/examples/discjuggler/discjuggler.c \
	${CODEMGR_WS}/ws_root/src/romapps/examples/discjuggler/event.c

CDEFINES += -DBUILD_EXPECTDATADISC

include ${CODEMGR_WS}/ws_root/variables/linkexec
include ${CODEMGR_WS}/ws_root/rules/linkexec
include ${CODEMGR_WS}/ws_root/rules/extrasrc
