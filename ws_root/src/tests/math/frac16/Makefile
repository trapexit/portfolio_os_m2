# @(#) Makefile 95/09/18 1.3

TARGET_DIRECTORY =	${REMOTE_DIR}/Tests/Math
REQUIRE_HIERARCHICAL_INCLUDES = 1

# !!! this silly link order is required to get strtof() (in c, but requires stuff in spmath) to link properly
LINK_LIBS =		spmath c spmath c

include ${CODEMGR_WS}/ws_root/variables/multilink
include ${CODEMGR_WS}/ws_root/rules/multilink
