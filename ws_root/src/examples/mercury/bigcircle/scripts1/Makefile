# @(#) Makefile 96/09/24 1.1
# Get archived mercury data to the remote folder.

DELIVER_THESE_FILES =	regress regress.mp regress.cube regress.hires32
DELIVER_FILES_TO =	${PLATFORM_DIR}/Examples/Mercury/Examples/bigcircle
EXAMPLE_CLASS =		Mercury/Examples/bigcircle
EXTRA_SRC =		../regress \
			../regress.mp \
			../regress.cube \
			../regress.hires32

include ${CODEMGR_WS}/ws_root/variables/datafiles
include ${CODEMGR_WS}/ws_root/variables/makeexamples
include ${CODEMGR_WS}/ws_root/rules/extrasrc
include ${CODEMGR_WS}/ws_root/rules/makeexamples
include ${CODEMGR_WS}/ws_root/rules/generic

