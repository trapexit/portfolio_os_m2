
LINK_LIBS = clt eventbroker blitter_utils clt spmath c
MODULES =  graphics gstate compression fsutils icon iff blitter frame2d audiopatchfile audiopatch font

#EXAMPLE_CLASS =	Mercury/Examples/

include ${CODEMGR_WS}/ws_root/variables/makeexamples
include ${CODEMGR_WS}/ws_root/variables/linkexec

include ${CODEMGR_WS}/ws_root/rules/subtree
include ${CODEMGR_WS}/ws_root/rules/makeexamples
include ${CODEMGR_WS}/ws_root/rules/linkexec


ALL_LINK_LIBS += \
		${REFERENCE_ROOT}/libs/${OPTIONS_PATH}/libspmath.a \
		${LINK_LIB_DIR}/libc.a

CINCLUDES += -I../../include -DGSTATE_PRIVATE

AS = ppcas
SFLAGS = -m602 -I../../include 
OPTIONS_FLAGS += -DVERBOSE








