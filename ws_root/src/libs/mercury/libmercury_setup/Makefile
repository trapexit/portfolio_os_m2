# @(#) Makefile 96/10/02 1.15

ifdef STATISTICS
STATDEF = -DSTATISTICS=1
endif

include ${CODEMGR_WS}/ws_root/variables/generic
include ${CODEMGR_WS}/ws_root/variables/linklib

OBJS = \
	${OBJDIR}/M_SetupDynLit.co \
	${OBJDIR}/M_SetupDynLitFog.co \
	${OBJDIR}/M_SetupDynLitTex.co \
	${OBJDIR}/M_SetupDynLitFogTex.co \
	${OBJDIR}/M_SetupDynLitSpecTex.co \
	${OBJDIR}/M_SetupDynLitEnv.co \
	${OBJDIR}/M_SetupDynLitFogEnv.co \
	${OBJDIR}/M_SetupDynLitSpecEnv.co \
	${OBJDIR}/M_SetupPreLit.co \
	${OBJDIR}/M_SetupPreLitFog.co \
	${OBJDIR}/M_SetupPreLitTex.co \
	${OBJDIR}/M_SetupPreLitFogTex.co \
	${OBJDIR}/M_SetupDynLitTrans.co \
	${OBJDIR}/M_SetupDynLitFogTrans.co \
	${OBJDIR}/M_SetupDynLitTransTex.co \
	${OBJDIR}/M_SetupDynLitTransEnv.co \
	${OBJDIR}/M_SetupPreLitTrans.co \
	${OBJDIR}/M_SetupPreLitFogTrans.co \
	${OBJDIR}/M_SetupPreLitTransTex.co \
	${OBJDIR}/M_SetupDynLitAA.co \
	${OBJDIR}/M_SetupDynLitTexAA.co \
	${OBJDIR}/M_SetupPreLitAA.co \
	${OBJDIR}/M_SetupPreLitTexAA.co 

all: ${OBJDIR} ${OBJS}

DELIVER_FILES_TO =	${PLATFORM_DIR}/Examples/Mercury/lib/libmercury_setup
DELIVER_THESE_FILES =	${wildcard *.c} ${wildcard *.s} ${wildcard *.make}

include ${CODEMGR_WS}/ws_root/variables/makeexamples
include ${CODEMGR_WS}/ws_root/rules/makeexamples
include ${CODEMGR_WS}/ws_root/rules/generic

CINCLUDES += -I../include -I../../include -I${SRC}/includes/mercury -DGSTATE_PRIVATE ${STATDEF}

AS = ppcas
SFLAGS = -I../include -I../../include -I${SRC}/includes/mercury ${STATDEF}

