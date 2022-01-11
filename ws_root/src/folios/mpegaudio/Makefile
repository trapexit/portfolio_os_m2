# @(#) Makefile 96/06/25 1.7
# Build mpegaudiodecoder

TARGET_NAME =   	mpegaudiodecoder
TARGET_DIRECTORY =      ${MODULE_DIR}
LOADERTOOL_FLAGS =      -Htype=4 -e main
NO_CHDIR_MODULE =	1
SIGNED_MODULE =		1
SHOW_MODULE_INFO =	1

ifdef PERFTEST
    CDEFINES +=         -DPERFTEST
endif

ifdef SOUNDFILE
    CDEFINES +=         -DSOUNDFILE
endif

ifdef COMPARE
    CDEFINES +=         -DCOMPARE
endif

ifdef DEBUG_PRINT
    CDEFINES +=         -DDEBUG_PRINT
endif

ifdef DEBUG
    CDEFINES +=         -DDEBUG
	SRCDEBUG =			1
	NO_OPTIMIZER =		1
else
ifdef DEBUG_AUDIO
    CDEFINES +=         -DDEBUG -DDEBUG_AUDIO
	SRCDEBUG =			1
	NO_OPTIMIZER =		1
endif
endif

# Enable memdebug if requested.
ifdef MEMDEBUG
    CDEFINES +=         -DMEMDEBUG
endif

include ${CODEMGR_WS}/ws_root/variables/linkexec
include ${CODEMGR_WS}/ws_root/rules/linkexec
