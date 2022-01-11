# @(#) Makefile 96/11/22 1.36

include ${CODEMGR_WS}/ws_root/variables/generic
include ${CODEMGR_WS}/ws_root/variables/memorymap
include ${CODEMGR_WS}/ws_root/rules/generic

ARCHIVE = /thdo/ssw/m2/examples_data/systemrom
VER = 18Oct96

ifeq (${BUILD_PCDEBUGGER},YES)
BUILD_DEBUGGER = NO
TEMP := $(OPTIONS_FLAGS)
OPTIONS_FLAGS = $(subst -DBUILD_DEBUGGER,,$(TEMP))
endif

default : all

ifeq (${BOOT_ARCH},)

# BOOT_ARCH not defined.
# Re-invoke MAKE with BOOT_ARCH and BOOT_FLAVOR set.
all :
ifeq (${BUILD_DEBUGGER},YES)
	${MAKE} $@ BOOT_ARCH=m2       BOOT_FLAVOR=BOOT_DEBUG
	${MAKE} $@ BOOT_ARCH=m2.16    BOOT_FLAVOR=BOOT_DEBUG  BUILD_16M=YES
	${MAKE} $@ BOOT_ARCH=m2.flash BOOT_FLAVOR=BOOT_DEBUG  BUILD_FLASH=YES
else
ifeq (${BUILD_PCDEBUGGER},NO)
	${MAKE} $@ BOOT_ARCH=m2       BOOT_FLAVOR=BOOT_DEMO
	${MAKE} $@ BOOT_ARCH=m2       BOOT_FLAVOR=BOOT_UNENC
	${MAKE} $@ BOOT_ARCH=m2       BOOT_FLAVOR=BOOT_ENC
	${MAKE} $@ BOOT_ARCH=m2       BOOT_FLAVOR=BOOT_ES4DEMO
	${MAKE} $@ BOOT_ARCH=m2       BOOT_FLAVOR=BOOT_ES4UNENC
else
	${MAKE} $@ BOOT_ARCH=m2       BOOT_FLAVOR=BOOT_PCDEV
endif
endif # ${BUILD_DEBUGGER}

else # ${BOOT_ARCH} defined

SYSTEM_RECIPE =	system.recipe
SYSTEM_NAME_STUB = ROM.${BOOT_ARCH}
ifeq (${BOOT_FLAVOR},BOOT_DEBUG)
SYSTEM_NAME = 	${SYSTEM_NAME_STUB}.debug
else
ifeq (${BOOT_FLAVOR},BOOT_ENC)
SYSTEM_NAME = 	${SYSTEM_NAME_STUB}.enc
else
ifeq (${BOOT_FLAVOR},BOOT_UNENC)
SYSTEM_NAME = 	${SYSTEM_NAME_STUB}.unenc
else
ifeq (${BOOT_FLAVOR},BOOT_DEMO)
SYSTEM_NAME = 	${SYSTEM_NAME_STUB}.demo
else
ifeq (${BOOT_FLAVOR},BOOT_PCDEV)
SYSTEM_NAME = 	${SYSTEM_NAME_STUB}.pcdev
else
ifeq (${BOOT_FLAVOR},BOOT_ES4DEMO)
SYSTEM_NAME = 	${SYSTEM_NAME_STUB}.es4demo
else
ifeq (${BOOT_FLAVOR},BOOT_ES4UNENC)
SYSTEM_NAME = 	${SYSTEM_NAME_STUB}.es4unenc
else
@@@ Error - unrecognized BOOT_FLAVOR
endif
endif
endif
endif
endif
endif
endif
SYSTEM_FILE =	${RELEASE_DIR}/${SYSTEM_NAME}

COOKOPTS =	-q -L ${OBJDIR}/cooklog.${SYSTEM_NAME}
COOKDEFS =	-DROOT=${ROOT}			\
		-DSRC=${SRC}			\
		-DRELEASE_DIR=${RELEASE_DIR}	\
		-DHOSTOSBIN=${HOSTOSBIN}	\
		-DOBJDIR=${OBJDIR}		\
		-DOPTIONS_PATH=${OPTIONS_PATH}	\
		-DBOOT_ARCH=${BOOT_ARCH}	\
		-DSYSTEM_NAME=${SYSTEM_NAME}	\
		-DSYSTEM_FILE=${SYSTEM_FILE}	\
		-D${BOOT_FLAVOR}=${BOOT_FLAVOR}	\
		${OPTIONS_FLAGS}		\
		${COMMAND_DEFINES}		\
		${MAP_GLOBAL_DEFINES}		\
		${MAP_ROM_DEFINES}

DO_COOK =	${COOK} ${$@_RECIPE} ${COOKOPTS} ${COOKDEFS}

IMAGES =	SYSTEM
BINARIES =	${SYSTEM_FILE}

FTP_TRANSFER =	${BINARIES}

all :		${OBJDIR} ${RELEASE_DIR} datafiles ${IMAGES}

endif # ${BOOT_ARCH}


.PHONY:		${IMAGES}

${IMAGES}:
	@echo "===== Building ${SYSTEM_NAME} ====="
	${DO_COOK}

${OBJDIR}/%:	%.r64
	${R64DEC} < $(@F).r64 > $@

clean.post:
	${RM} ${BINARIES}


datafiles:	
	${RM} ${OBJDIR}/staticscreen
	${RM} ${OBJDIR}/characterdata
ifeq (${BOOT_FLAVOR},BOOT_PCDEV)
	${SLN} ${ARCHIVE}/${VER}/pcstaticscreen ${OBJDIR}/staticscreen
else	
	${SLN} ${ARCHIVE}/${VER}/staticscreen ${OBJDIR}/staticscreen
endif
	${SLN} ${ARCHIVE}/${VER}/characterdata ${OBJDIR}/characterdata

