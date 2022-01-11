# @(#) Makefile 96/03/06 1.10

TARGET_DIRECTORY  =	${LINK_LIB_DIR}
LIBRARY_IS_PUBLIC =	1
TARGET_NAME =		libezflixdecoder.a
INCDIRS =       	${SRC}/includes ${SRC}/libs/streaming/Subscribers/EZFlixSubscriber/EZFlixDecoder

CDEFINES =		-DEXTERNAL_RELEASE

EXTRA_SRC   = ${SRC}/mac/tools/streaming/EZFlixChunkifier/EZFlixEncoder/EZFlixCodec.h \
	${SRC}/mac/tools/streaming/EZFlixChunkifier/Includes/EZFlixXPlat.h

BULLET = "__"
TOOL_CLASS =            StreamTools/Source/EZFlixChunkifier/${BULLET}EZFlixDecoder

DELIVER_FILES_TO =      ${PLATFORM_DIR}/Sources/Streaming/${TOOL_CLASS}
DELIVER_THESE_FILES =   ${wildcard *.c *.cp *.h}


include ${CODEMGR_WS}/ws_root/variables/linklib
include ${CODEMGR_WS}/ws_root/rules/linklib
include ${CODEMGR_WS}/ws_root/rules/extrasrc
