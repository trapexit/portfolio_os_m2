# @(#) Makefile 95/12/08 1.15

TARGET_DIRECTORY  =	${LINK_LIB_DIR}
LIBRARY_IS_PUBLIC =	1
TARGET_NAME =		libds.a
REQUIRE_HIERARCHICAL_INCLUDES = 1

ifdef DEBUG_STREAMER
CDEFINES =              -DDEBUG -DEXTERNAL_RELEASE
SRCDEBUG =		1
else
CDEFINES =              -DEXTERNAL_RELEASE
endif

include ${CODEMGR_WS}/ws_root/variables/linklib

DELIVER_FILES_TO =      ${PLATFORM_DIR}/Sources/Streaming/Libs/DataStream
DELIVER_THESE_FILES =   ${wildcard *.c libDS.a.make} # Don't forget to include libDSShuttle.a.make later (after alpha release).

include ${CODEMGR_WS}/ws_root/rules/linklib
