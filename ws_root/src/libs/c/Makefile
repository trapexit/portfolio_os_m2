# @(#) Makefile 96/02/07 1.11

TARGET_DIRECTORY  =	${LINK_LIB_DIR}
LIBRARY_IS_PUBLIC =	1
REQUIRE_HIERARCHICAL_INCLUDES = 1

include ${CODEMGR_WS}/ws_root/variables/linklib
include ${CODEMGR_WS}/ws_root/rules/subtree

SCSRC =			${foreach dir,${SUBDIRS},${wildcard ${dir}/*.c}}
SCSRCD =		${dir ${SCSRC}}
SCSRCN =		${notdir ${SCSRC}}
SCOBJN =		${SCSRCN:%.c=${OBJDIR}/%.co}
SCOBJ =			${join ${SCSRCD},${SCOBJN}}

SSSRC =			${filter-out _%.s,${foreach dir,${SUBDIRS},${wildcard ${dir}/*.[Ss]}}}
SSSRCD =		${dir ${SSSRC}}
SSSRCN =		${notdir ${SSSRC}}
SSOBJN =		${SSSRCN:%.s=${OBJDIR}/%.so}
SS2OBJN =		${SSSRCN:%.S=${OBJDIR}/%.so}
SSOBJ =			${join ${SSSRCD},${SSOBJN}} ${join ${SSSRCD},${SS2OBJN}}

SR64SRC =		${foreach dir,${SUBDIRS},${wildcard ${dir}/*.r64}}
SR64SRCD =		${dir ${SR64SRC}}
SR64SRCN =		${notdir ${SR64SRC}}
SR64OBJN =		${SR64SRCN:%.r64=${OBJDIR}/%}
SR64OBJ =		${join ${SR64SRCD},${SR64OBJN}}

OBJS =			${SCOBJ} ${SR64OBJ} ${SSOBJ}

include ${CODEMGR_WS}/ws_root/rules/linklib
