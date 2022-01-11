# @(#) Makefile 96/03/28 1.13

REQUIRE_HIERARCHICAL_INCLUDES = 1

DELIVER_FILES_TO =      ${PLATFORM_DIR}/Sources/Streaming/Libs/Subscribers/SAudioSubscriber
DELIVER_THESE_FILES =	${wildcard *.h *.c *.make}

# Set this environment variable to build a debug version of this subscriber.
ifdef DEBUG_SAUDIOSUBSCRIBER
CDEFINES +=		-DDEBUG				\
#		-DSAUDIO_TRACE_MAIN=1		\
#		-DSAUDIO_TRACE_BUFFERS=1	\
#		-DSAUDIO_TRACE_CHANNELS=1	\
#		-DSAUDIO_TRACE_SUPPORT=1	\
#		-DSAUDIO_TRACE_TEMPLATES=1

SRCDEBUG =			1
NO_OPTIMIZER =		1
endif

ifdef DEBUG_PRINT
CDEFINES +=		-DDEBUG -DDEBUG_PRINT
endif

include ${CODEMGR_WS}/ws_root/variables/linklib
include ${CODEMGR_WS}/ws_root/rules/generic

all.post: ${OBJS}
