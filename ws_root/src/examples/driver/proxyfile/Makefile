#  @(#) Makefile 96/02/21 1.4

EXAMPLE_CLASS =         Driver
LINK_LIBS =             c

EXTRA_SRC = ${SRC}/tests/proxyfile/proxyfile.c \
            ${SRC}/tests/proxyfile/proxyfile.D

include ${CODEMGR_WS}/ws_root/variables/makeexamples
include ${CODEMGR_WS}/ws_root/variables/multilink
include ${CODEMGR_WS}/ws_root/rules/subtree
include ${CODEMGR_WS}/ws_root/rules/makeexamples
include ${CODEMGR_WS}/ws_root/rules/multilink
include ${CODEMGR_WS}/ws_root/rules/extrasrc
