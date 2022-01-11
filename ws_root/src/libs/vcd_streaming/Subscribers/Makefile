# @(#) Makefile 96/06/04 1.1

TARGET_DIRECTORY  =	${LINK_LIB_DIR}
LIBRARY_IS_PUBLIC =	1
TARGET_NAME =		libvcdsubscriber.a
INCDIRS =		${SRC}/includes

include ${CODEMGR_WS}/ws_root/variables/linklib
include ${CODEMGR_WS}/ws_root/rules/subtree

SCSRC =			${foreach dir,${SUBDIRS},${wildcard ${dir}/*.c}}
SCSRCD =		${dir ${SCSRC}}
SCSRCN =		${notdir ${SCSRC}}
SCOBJN =		${SCSRCN:%.c=${OBJDIR}/%.co}
SCOBJ =			${join ${SCSRCD},${SCOBJN}}

OBJS =			${SCOBJ}

include ${CODEMGR_WS}/ws_root/rules/linklib
