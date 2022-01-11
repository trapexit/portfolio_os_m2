# @(#) Makefile 96/04/01 1.15
# M2 native pForth tool.

TARGET_DIRECTORY =	${REMOTE_DIR}/System.m2/Programs
LINK_LIBS =		c
PRIVILEGED_MODULE =     1

PFORTH_DIR       = ${SRC}/tools/pforth_all/pforth

EXTRA_SRC += \
    ${PFORTH_DIR}/pf_cglue.c \
    ${PFORTH_DIR}/pfcompil.c \
    ${PFORTH_DIR}/pf_core.c \
    ${PFORTH_DIR}/pf_inner.c \
    ${PFORTH_DIR}/pf_io.c \
    ${PFORTH_DIR}/pf_mem.c \
    ${PFORTH_DIR}/pf_save.c \
    ${PFORTH_DIR}/pf_text.c \
    ${PFORTH_DIR}/pf_tools.c \
    ${PFORTH_DIR}/pf_words.c \
    ${PFORTH_DIR}/pf_cglue.h \
    ${PFORTH_DIR}/pfcompil.h \
    ${PFORTH_DIR}/pf_core.h \
    ${PFORTH_DIR}/pfcompfp.h \
    ${PFORTH_DIR}/pfinnrfp.h \
    ${PFORTH_DIR}/pf_all.h \
    ${PFORTH_DIR}/pf_guts.h \
    ${PFORTH_DIR}/pf_io.h \
    ${PFORTH_DIR}/pf_mem.h \
    ${PFORTH_DIR}/pf_save.h \
    ${PFORTH_DIR}/pf_text.h \
    ${PFORTH_DIR}/pf_tools.h \
    ${PFORTH_DIR}/pf_words.h \
    ${PFORTH_DIR}/pforth.h

CDEFINES +=	-DPF_HOST_3DO -DPF_3DO_FILEIO

ifdef PF_CDE_SERIAL
	CDEFINES +=	-DPF_CDE_SERIAL
endif
ifdef PF_INIT_MODE
	CDEFINES +=	-DPF_INIT_MODE
endif
ifdef PF_NO_FILEIO
	CDEFINES +=	-DPF_NO_FILEIO
endif

include ${CODEMGR_WS}/ws_root/variables/linkexec
INCDIRS            += ${PFORTH_DIR}
include ${CODEMGR_WS}/ws_root/rules/linkexec
include ${CODEMGR_WS}/ws_root/rules/extrasrc
