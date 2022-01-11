# @(#) Makefile 96/01/12 1.7

REQUIRE_HIERARCHICAL_INCLUDES = 1

EXTRA_SRC   = EZFlixDecoder/EZFlixDecoder.h
DELIVER_FILES_TO =      ${PLATFORM_DIR}/Sources/Streaming/Libs/Subscribers/EZFlixSubscriber
DELIVER_THESE_FILES =   EZFlixDecoder.h		\
			EZFlixSubscriber.c	\
			EZFlixSubscriber.make

include ${CODEMGR_WS}/ws_root/rules/subtree
include ${CODEMGR_WS}/ws_root/variables/linklib
include ${CODEMGR_WS}/ws_root/rules/generic
include ${CODEMGR_WS}/ws_root/rules/extrasrc

ifdef DEBUG_EZFLIX
CDEFINES =              -DDEBUG -DEXTERNAL_RELEASE
SRCDEBUG =              1
else
CDEFINES =              -DEXTERNAL_RELEASE
endif

all.post: ${OBJS}
