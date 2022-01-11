#  @(#) Makefile 96/02/21 1.4

EXAMPLE_CLASS =         EventBroker
LINK_LIBS =             eventbroker c

EXTRA_SRC = ${SRC}/tests/eventbroker/cpdump.c \
            ${SRC}/tests/eventbroker/focus.c \
            ${SRC}/tests/eventbroker/lookie.c \
	    ${SRC}/tests/eventbroker/luckie.c \
	    ${SRC}/tests/eventbroker/maus.c

include ${CODEMGR_WS}/ws_root/variables/makeexamples
include ${CODEMGR_WS}/ws_root/variables/multilink
include ${CODEMGR_WS}/ws_root/rules/subtree
include ${CODEMGR_WS}/ws_root/rules/makeexamples
include ${CODEMGR_WS}/ws_root/rules/multilink
include ${CODEMGR_WS}/ws_root/rules/extrasrc
