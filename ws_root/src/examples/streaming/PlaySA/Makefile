# @(#) Makefile 96/03/29 1.18
# PlaySA example

EXAMPLE_CLASS		=	Streaming/PlaySA
LOADERTOOL_FLAGS	=  -Hstack=0x2000
LINK_LIBS			=	subscriber ds dsutils music eventbroker c
MODULES				=	audio graphics mpegaudiodecoder
INCDIRS				=	${SRC}/includes ${SRC}/examples/streaming

ifdef DEBUG_PLAYSA
CDEFINES		=	-DDEBUG
SRCDEBUG 		=	1
NO_OPTIMIZER	=	1
endif

include ${CODEMGR_WS}/ws_root/variables/makeexamples
include ${CODEMGR_WS}/ws_root/rules/subtree
include ${CODEMGR_WS}/ws_root/variables/linkexec
include ${CODEMGR_WS}/ws_root/rules/makeexamples
include ${CODEMGR_WS}/ws_root/rules/linkexec
