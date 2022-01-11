# @(#) Makefile 96/06/05 1.1

REQUIRE_HIERARCHICAL_INCLUDES = 1
INCDIRS	= 	${VCD}/includes
# Set this environment variable to build a debug version of this subscriber.
ifdef DEBUG_SAUDIOSUBSCRIBER
CDEFINES +=		-DDEBUG				\
		-DSAUDIO_TRACE_MAIN=1		\
		-DSAUDIO_TRACE_BUFFERS=1	\
		-DSAUDIO_TRACE_CHANNELS=1	\
		-DSAUDIO_TRACE_SUPPORT=1	\
		-DSAUDIO_TRACE_TEMPLATES=1

SRCDEBUG =			1
NO_OPTIMIZER =		1
endif

ifdef DEBUG_PRINT
CDEFINES +=		-DDEBUG -DDEBUG_PRINT
endif
INCDIRS		= $(SRC)/includes $(VCD)/includes
include ${CODEMGR_WS}/ws_root/variables/linklib
include ${CODEMGR_WS}/ws_root/rules/generic

all.post: ${OBJS}
