# @(#) Makefile 96/08/30 1.22
# MPEG VideoPlayer example

EXAMPLE_CLASS =		Streaming/VideoPlayer
LOADERTOOL_FLAGS =	-Hstack=0x2000
LINK_LIBS	=	subscriber ds dsutils eventbroker music c 
MODULES		=	audio graphics mpegaudiodecoder
INCDIRS	=		${SRC}/includes ${SRC}/examples/streaming

ifdef DEBUG_VIDEOPLAYER
CDEFINES =              -DDEBUG -DEXTERNAL_RELEASE
SRCDEBUG =		1
NO_OPTIMIZER =		1
else
CDEFINES =              -DEXTERNAL_RELEASE
endif

CDEFINES +=		-DSTARTUP_WORKAROUND		# defined ==> avoid startup flicker
#CDEFINES +=		-DFIELD_TO_DISPLAY_FIRST=0	# defined ==> in 480-line mode, source is interlaced, this field first

include ${CODEMGR_WS}/ws_root/variables/makeexamples
include ${CODEMGR_WS}/ws_root/rules/subtree
include ${CODEMGR_WS}/ws_root/variables/linkexec
include ${CODEMGR_WS}/ws_root/rules/makeexamples
include ${CODEMGR_WS}/ws_root/rules/linkexec


