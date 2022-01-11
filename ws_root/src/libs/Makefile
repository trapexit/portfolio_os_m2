# @(#)Makefile 96/03/21 1.6

include ${CODEMGR_WS}/ws_root/variables/generic

ifeq (${HOSTOS},IRIX)

GfxLibs =            pbsdf bsdf clt cpp frame frame_utils pipe gfxsim sdf tcl_frame
DO_NOT_BUILD_IN =    $(filter-out $(GfxLibs),$(wildcard *))
BUILD_IN_FIRST =     ${SRC}/includes/sgi

else

ifdef BUILD_GFXSIM
ifeq (${BUILD_GFXSIM},YES)
DO_NOT_BUILD_IN =    cpp sdf tcl_frame
else
DO_NOT_BUILD_IN =    gfxsim cpp sdf tcl_frame
endif
else
DO_NOT_BUILD_IN =    gfxsim cpp sdf tcl_frame
endif

endif

include ${CODEMGR_WS}/ws_root/rules/subtree
include ${CODEMGR_WS}/ws_root/rules/generic
