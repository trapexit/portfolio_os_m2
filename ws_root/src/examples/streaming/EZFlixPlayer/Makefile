# @(#) Makefile 96/03/04 1.12
# EZFlixPlayer example

EXAMPLE_CLASS =		Streaming/EZFlixPlayer
LOADERTOOL_FLAGS =      -Hstack=0x2000
LINK_LIBS	=	subscriber ds dsutils ezflixdecoder music eventbroker c
MODULES		=	audio graphics
INCDIRS	=	${SRC}/includes ${SRC}/examples/streaming

ifdef DEBUG_EZFLIXPLAYER
CDEFINES =		-DDEBUG -DEXTERNAL_RELEASE
SRCDEBUG =		1
NO_OPTIMIZER =		1
else
CDEFINES =		-DEXTERNAL_RELEASE
endif

include ${CODEMGR_WS}/ws_root/variables/makeexamples
include ${CODEMGR_WS}/ws_root/rules/subtree
include ${CODEMGR_WS}/ws_root/variables/linkexec
include ${CODEMGR_WS}/ws_root/rules/linkexec
