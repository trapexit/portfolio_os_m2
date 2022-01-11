# @(#) Makefile 96/03/25 1.4
# M2 native pForth tool.
# This library is for private, internal consumption.
# It is designed to be linked with the bootcode to provide
# a debug monitor for debugging hardware.

TARGET_DIRECTORY  =	${LINK_LIB_DIR}

PFORTH_DIR       = ${SRC}/tools/pforth_all/pforth
PFORTH_3DO_DIR       = ${SRC}/programs/pforth

# PF_NO_FILEIO means get dictionary from static array in pf_dic_data.h
# which is generated using "save_dic_as_data.fth".
# Use CDE serial port for character I/O.
CDEFINES +=  -DPF_NO_FILEIO -DPF_CDE_SERIAL \
	-DPF_HOST_3DO -DPF_NO_CLIB \
	 -DPF_NO_MALLOC  -DPF_MEM_POOL_SIZE=500000 \
	-DPF_MALLOC_ADDRESS=0x40380000

EXTRA_SRC += \
    ${PFORTH_DIR}/pfcustom.c \
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
    ${PFORTH_DIR}/pforth.h \
    ${PFORTH_3DO_DIR}/pf_host.h \
    ${PFORTH_3DO_DIR}/pf_host_m2.c


include ${CODEMGR_WS}/ws_root/variables/linklib
INCDIRS            += ${PFORTHDIR}
include ${CODEMGR_WS}/ws_root/rules/linklib
include ${CODEMGR_WS}/ws_root/rules/extrasrc
