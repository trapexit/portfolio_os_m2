# @(#) Makefile 96/03/11 1.33

ifdef MEMDEBUG
    CDEFINES +=		-DMEMDEBUG
endif

TARGET_DIRECTORY =	${REMOTE_DIR}/Tests/Audio
LINK_LIBS =		music audioprivate dspptouch eventbroker spmath c
MODULES =		audiopatchfile audiopatch audio iff script
NO_CHDIR_MODULE =	1
# LOADERTOOL_FLAGS =	-N

include ${CODEMGR_WS}/ws_root/variables/multilink
include ${CODEMGR_WS}/ws_root/rules/subtree
include ${CODEMGR_WS}/ws_root/rules/multilink
