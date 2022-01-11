# @(#) Makefile 96/02/08 1.17

# don't build autodepends here or down - saves a bit of time during gmake depends.
DO_NOT_BUILD_DEPENDS =	1

include ${CODEMGR_WS}/ws_root/variables/generic

ifneq (${HOSTOS}, IRIX)
DO_NOT_BUILD_IN =	sgi
endif

DELIVER_FILES_TO =	${PLATFORM_DIR}/includes
DELIVER_THESE_FILES =	assert.h				\
			ctype.h					\
			float.h					\
			limits.h				\
			math.h					\
			setjmp.h				\
			stdarg.h				\
			stddef.h				\
			stdio.h					\
			stdlib.h				\
			string.h				\
			strings.h				\
			varargs.h

include ${CODEMGR_WS}/ws_root/rules/subtree
include ${CODEMGR_WS}/ws_root/rules/generic
