# @(#) Makefile 95/04/13 1.6
# compile "pforth.dic" - machine independent dictionary for pforth

include ${CODEMGR_WS}/ws_root/variables/generic
include ${CODEMGR_WS}/ws_root/rules/subtree

# pforth and pforth.dic live in a host specific bin directory
PFORTHBIN =		${HOSTOSBIN}
PFORTH =		${PFORTHBIN}/pforth
PFORTH_DIC =		pforth.dic
PFORTH_TARGET =		${PFORTHBIN}/${PFORTH_DIC}

# all .fth files in this directory are used in building pforth.dic
PFORTH_DIC_SRCS =	${wildcard *.fth}

TARGET_NAME =		${PFORTH_TARGET}

include ${CODEMGR_WS}/ws_root/rules/generic

all.post: ${PFORTH_TARGET}

${PFORTH_TARGET}: ${PFORTH_DIC_SRCS} ${PFORTH} Makefile
	${RM} ${PFORTH_DIC}
	${PFORTH} -i system.fth
	${MV} ${PFORTH_DIC} ${PFORTH_TARGET}

clean:
	${RM} ${PFORTH_TARGET}
