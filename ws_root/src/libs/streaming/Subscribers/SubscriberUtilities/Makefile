# @(#) Makefile 95/12/08 1.8

REQUIRE_HIERARCHICAL_INCLUDES = 1

CDEFINES =              -DEXTERNAL_RELEASE

DELIVER_FILES_TO =      ${PLATFORM_DIR}/Sources/Streaming/Libs/Subscribers/SubscriberUtilities
DELIVER_THESE_FILES =   subscribertraceutils.c		\
                    	subscriberutils.c		\
			subscribertrace.dict		\
                    	SubscriberUtilities.make

include ${CODEMGR_WS}/ws_root/variables/linklib
include ${CODEMGR_WS}/ws_root/rules/generic

all.post: ${OBJS}
