# @(#) Makefile 96/10/21 1.12
#
# Makefile to create ROM images.
# Current targets are:
#	cdrom	Builds a CD-ROM image.
#	cart	Builds a bootable cartridge.
#	romapp	Builds a RomApp cartridge.
# These must be specified explicitly on the command line (e.g. gmake cdrom).
# The default build (gmake) does nothing.
#

include ${CODEMGR_WS}/ws_root/variables/generic
include ${CODEMGR_WS}/ws_root/variables/memorymap

ifeq (${BUILD_PCDEBUGGER},YES)
BUILD_DEBUGGER = NO
TEMP := $(OPTIONS_FLAGS)
OPTIONS_FLAGS = $(subst -DBUILD_DEBUGGER,,$(TEMP))
endif

default: all

ifndef IMAGE

all:
ifeq (${BUILD_DEBUGGER},YES)
	${MAKE} IMAGE=remote
	mv ${RELEASE_DIR}/remote.image ${RELEASE_DIR}/remote/System.m2/Boot/remotevol
endif
ifeq (${BUILD_CD},YES)
	${MAKE} IMAGE=test.cdrom
endif
	@echo "Done"

cdrom:
	${MAKE} IMAGE=cdrom
cart:
	${MAKE} IMAGE=cart
romapp:
	${MAKE} IMAGE=romapp
bootromapp:
	${MAKE} IMAGE=bootromapp


else # IMAGE defined

ENC =			unenc
TARGET_NAME =		${IMAGE}.image
TARGET_DIRECTORY =	${RELEASE_DIR}
OBJDIR =		objects/${ENC}/${BUILD_TYPE}

LOCALS =		${OBJDIR}/${TARGET_NAME}

COOKDEFS =		-DROOT=${ROOT} \
			-DOBJDIR=${OBJDIR} \
			-DRELEASE_DIR=${RELEASE_DIR} \
			-DHOSTOSBIN=${HOSTOSBIN} \
			-DOPTIONS_PATH=${OPTIONS_PATH} \
			-DENC=${ENC} \
			-DIMAGE=${IMAGE} \
			${MAP_GLOBAL_DEFINES} \
			${COMMAND_DEFINES}


all ftp production: makedirs.here ${LOCALS} ${TARGET_DIRECTORY}/${TARGET_NAME}

FTP_TRANSFER =	${TARGET_DIRECTORY}/${TARGET_NAME}

${TARGET_DIRECTORY}/${TARGET_NAME}: ${OBJDIR}/${TARGET_NAME}
	${hardlinkme}

${OBJDIR}/${TARGET_NAME}: all.here
	${COOK} ${IMAGE}.recipe -q -L ${OBJDIR}/cook.${IMAGE}.log ${COOKDEFS}

endif # IMAGE

clean:
	rm -rf objects laytools

include ${CODEMGR_WS}/ws_root/rules/generic
