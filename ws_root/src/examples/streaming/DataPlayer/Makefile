# @(#) Makefile 96/07/08 1.2

EXAMPLE_CLASS =		Streaming/DataPlayer
LOADERTOOL_FLAGS =	-Hstack=0x2000
LINK_LIBS	=	subscriber ds dsutils eventbroker music clt c clt c
MODULES		=	audio graphics gstate compression font
INCDIRS	=		${SRC}/includes

ifdef DEBUG_DATAPLAYER
CDEFINES =			-DDEBUG -DEXTERNAL_RELEASE
SRCDEBUG =			1
NO_OPTIMIZER =		1
else
CDEFINES =			-DEXTERNAL_RELEASE
endif

include ${CODEMGR_WS}/ws_root/variables/makeexamples
include ${CODEMGR_WS}/ws_root/rules/subtree
include ${CODEMGR_WS}/ws_root/variables/linkexec
include ${CODEMGR_WS}/ws_root/rules/makeexamples
include ${CODEMGR_WS}/ws_root/rules/linkexec
