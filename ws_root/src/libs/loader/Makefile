# @(#) Makefile 96/08/02 1.18

TARGET_DIRECTORY =	${LINK_LIB_DIR}

EXTRA_SRC =	${SRC}/folios/compression/decompress.c \
		${SRC}/folios/compression/simpledecompress.c

INCDIRS =	${SRC}/folios/compression

CDEFINES +=	-DKERNEL

include ${CODEMGR_WS}/ws_root/variables/linklib
include ${CODEMGR_WS}/ws_root/rules/linklib
include ${CODEMGR_WS}/ws_root/rules/extrasrc
