# @(#) Makefile 96/03/01 1.13

REQUIRE_HIERARCHICAL_INCLUDES = 1

DELIVER_FILES_TO =      ${PLATFORM_DIR}/Sources/Streaming/Libs/Subscribers/MPEGVideoSubscriber
DELIVER_THESE_FILES =   fmvdriverinterface.c	\
			fmvdriverinterface.h \
			mpegbufferqueues.c	\
			mpegvideochannels.c	\
			mpegvideochannels.h	\
			mpegvideosubscriber.c	\
			mpegvideosupport.c	\
			mpegvideosupport.h	\
			MPEGVideoSubscriber.make

ifdef DEBUG_MPEGSUBSCRIBER
CDEFINES =		-DDEBUG -DEXTERNAL_RELEASE
#CDEFINES =		-DDEBUG -DEXTERNAL_RELEASE -DMPVD_TRACE_MAIN=1 -DMPVD_TRACE_SUPPORT=1
SRCDEBUG =		1
else
CDEFINES =              -DEXTERNAL_RELEASE
endif

include ${CODEMGR_WS}/ws_root/variables/linklib
include ${CODEMGR_WS}/ws_root/rules/generic

all.post: ${OBJS}
