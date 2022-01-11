#  @(#) Makefile 96/09/18 1.5

ifdef STATISTICS
STATDEF = -DSTATISTICS=1
endif

LIBRARY_IS_PUBLIC	=	1

include ${CODEMGR_WS}/ws_root/variables/linklib
include ${CODEMGR_WS}/ws_root/variables/generic

TARGET_NAME		=	libmercury_mpm.a

all: ${OBJDIR} ${OBJS}

DELIVER_FILES_TO =	${PLATFORM_DIR}/Examples/Mercury/lib/libmercury_mpm
DELIVER_THESE_FILES =	${wildcard *.c} ${wildcard *.s} ${wildcard *.make}

include ${CODEMGR_WS}/ws_root/variables/makeexamples
include ${CODEMGR_WS}/ws_root/rules/makeexamples
include ${CODEMGR_WS}/ws_root/rules/linklib

CINCLUDES += -I../include -I../../include -I${SRC}/includes/mercury -DGSTATE_PRIVATE ${STATDEF}

AS = ppcas
SFLAGS = -I../include -I../../include -I${SRC}/includes/mercury ${STATDEF}
