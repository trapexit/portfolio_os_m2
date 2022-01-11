# @(#) Makefile 96/03/11 1.3
# Build testmpegaudio

TARGET_DIRECTORY =  ${REMOTE_DIR}/Tests/MPEG/TestMPEGAudio
LINK_LIBS =		music eventbroker c
MODULES	=		audio mpegaudiodecoder
NO_CHDIR_MODULE =   1

ifdef MEMDEBUG
	 CDEFINES +=     -DMEMDEBUG
endif

ifdef DEBUG
	CDEFINES +=		-DDEBUG
	SRCDEBUG =		1
	NO_OPTIMIZER =	1
else
ifdef DEBUG_TESTMPEGAUDIO
	CDEFINES +=		-DEBUG -DDEBUG_AUDIO
	SRCDEBUG =		1
	NO_OPTIMIZER =	1
endif
endif

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
	 CDEFINES +=	-DDEBUG_PRINT
endif

include ${CODEMGR_WS}/ws_root/rules/subtree
include ${CODEMGR_WS}/ws_root/variables/linkexec
include ${CODEMGR_WS}/ws_root/rules/linkexec
