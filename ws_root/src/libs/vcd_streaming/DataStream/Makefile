# @(#) Makefile 96/06/04 1.1

TARGET_DIRECTORY  	=	${LINK_LIB_DIR}
LIBRARY_IS_PUBLIC 	=	1
TARGET_NAME 		=		libvcdds.a
REQUIRE_HIERARCHICAL_INCLUDES = 1
INCDIRS				=	${SRC}/includes ${VCD}/includes
ifdef DEBUG_STREAMER
CDEFINES =              -DREAD_TRIGGER_BIT=1 -DDEBUG -DEXTERNAL_RELEASE
SRCDEBUG =		1
else
CDEFINES =              -DEXTERNAL_RELEASE
endif

include ${CODEMGR_WS}/ws_root/variables/linklib
include ${CODEMGR_WS}/ws_root/rules/linklib
