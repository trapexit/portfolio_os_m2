# @(#) Makefile 95/12/08 1.10

TARGET_DIRECTORY  =	${LINK_LIB_DIR}
LIBRARY_IS_PUBLIC =	1
TARGET_NAME =		libdsutils.a
REQUIRE_HIERARCHICAL_INCLUDES = 1

CDEFINES =              -DEXTERNAL_RELEASE

DELIVER_FILES_TO =      ${PLATFORM_DIR}/Sources/Streaming/Libs/DSUtils
DELIVER_THESE_FILES =   ${wildcard *.c libDSUtils.a.make}

include ${CODEMGR_WS}/ws_root/variables/linklib
include ${CODEMGR_WS}/ws_root/rules/linklib
