# @(#) Makefile 95/12/12 1.10
# This library is for private, internal consumption.
# It bangs on the DSP hardware.
# It is shared by the audiofolio, beepfolio, and DSP hardware tests.

TARGET_DIRECTORY  =	${LINK_LIB_DIR}

REQUIRE_HIERARCHICAL_INCLUDES = 1

include ${CODEMGR_WS}/ws_root/variables/linklib

# The build in this directory is for BDA Pass 2
# CDEFINES +=     -DAF_BDA_PASS=2

# Enable simulator if requested.
ifdef SIMULATE_DSPP
    CDEFINES +=		-DSIMULATE_DSPP
endif

include ${CODEMGR_WS}/ws_root/rules/linklib
