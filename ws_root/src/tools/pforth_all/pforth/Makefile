# @(#) Makefile 96/03/21 1.9
#
# Makefile for pforth 'C' based kernel

include ${CODEMGR_WS}/ws_root/variables/hostexec

# Build embedded version?
ifdef PF_NO_FILEIO
    HOSTCFLAGS +=         -DPF_NO_FILEIO
endif

HOSTCFLAGS +=  -DPF_SUPPORT_FP
LINK_LIBS  += m

include ${CODEMGR_WS}/ws_root/rules/hostexec

