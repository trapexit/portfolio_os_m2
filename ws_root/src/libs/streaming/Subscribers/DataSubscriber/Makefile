# @(#) Makefile 96/02/29 1.2

REQUIRE_HIERARCHICAL_INCLUDES = 1

CDEFINES =              -DEXTERNAL_RELEASE

DELIVER_FILES_TO =      ${PLATFORM_DIR}/Sources/Streaming/Libs/Subscribers/DataSubscriber
DELIVER_THESE_FILES =   datasubscriber.c	\
                    	datachannels.c		\
                    	datachannels.h		\
                    	datasubscriber.make

ifdef DEBUG_DATASUBSCRIBER
CDEFINES =		-DDEBUG -DEXTERNAL_RELEASE
#CDEFINES =		-DDEBUG -DEXTERNAL_RELEASE -DTRACE_DATA_SUBSCRIBER=1 -DDATA_TRACE_LEVEL=3
SRCDEBUG =		1
else
CDEFINES =		-DEXTERNAL_RELEASE
endif

include ${CODEMGR_WS}/ws_root/variables/linklib
include ${CODEMGR_WS}/ws_root/rules/generic

all.post: ${OBJS}
