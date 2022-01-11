# @(#) Makefile 95/05/26 1.6

TARGET_DIRECTORY  =	${LINK_LIB_DIR}
LIBRARY_IS_PUBLIC =	1
TARGET_NAME =		libsubscriber.a
INCDIRS =		${SRC}/includes

DELIVER_FILES_TO =      ${PLATFORM_DIR}/Sources/Streaming/Libs/Subscribers
DELIVER_THESE_FILES =   BuildSubscriberLib # BuildSubscriberSLib

include ${CODEMGR_WS}/ws_root/variables/linklib
include ${CODEMGR_WS}/ws_root/rules/subtree

SCSRC =			${foreach dir,${SUBDIRS},${wildcard ${dir}/*.c}}
SCSRCD =		${dir ${SCSRC}}
SCSRCN =		${notdir ${SCSRC}}
SCOBJN =		${SCSRCN:%.c=${OBJDIR}/%.co}
SCOBJ =			${join ${SCSRCD},${SCOBJN}}

OBJS =			${SCOBJ}

include ${CODEMGR_WS}/ws_root/rules/linklib
