# @(#) Makefile 96/07/09 1.8
# Single-source test programs that use portable graphics

ifdef MEMDEBUG
    CDEFINES +=		-DMEMDEBUG
endif

TARGET_DIRECTORY =	${REMOTE_DIR}/Tests/Audio
LINK_LIBS =		audiodemo music clt spmath eventbroker c
MODULES =		audio font graphics gstate frame2d

include ${CODEMGR_WS}/ws_root/variables/multilink
include ${CODEMGR_WS}/ws_root/rules/multilink
