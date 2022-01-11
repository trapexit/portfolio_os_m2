# @(#) Makefile 96/10/18 1.48

NO_RELOCS = 1
UNCOMPRESSED_MODULE = 1

include ${CODEMGR_WS}/ws_root/variables/linkexec

ifeq (${BUILD_PCDEBUGGER},YES)
BUILD_DEBUGGER = NO
TEMP := $(OPTIONS_FLAGS)
OPTIONS_FLAGS = $(subst -DBUILD_DEBUGGER,,$(TEMP)) 
endif

TARGET_DIRECTORY =	${OBJDIR}
LFLAGS +=		-eExceptionTable -Bt=${BOOTCODESTART} -Bd=${BOOTDATASTART} -g

# This contains the Exception vector, so it _must_ be linked first
FIRST_SRC =	bootvectors.s
FIRST_OBJ =	$(FIRST_SRC:%.s=$(OBJDIR)/%.so)
OBJS :=  $(filter-out $(FIRST_OBJ), $(OBJS))
OBJS := $(FIRST_OBJ) $(OBJS)

ADEFINES = ${MAP_ALL_DEFINES} ${DEBUG_DEFS}
CDEFINES = ${MAP_ALL_DEFINES} ${DEBUG_DEFS} -Xstmw-fast

ifeq (${OBJSUBDIR},)

default: all
all ftp production depends clean orphans autodoc:
	${MAKE} $@ OBJSUBDIR=/m2
ifeq (${BUILD_PCDEBUGGER},NO)
	${MAKE} $@ OBJSUBDIR=/m2.16 BUILD_16M=YES
	${MAKE} $@ OBJSUBDIR=/m2.flash BUILD_FLASH=YES
endif
else
include ${CODEMGR_WS}/ws_root/rules/linkexec
include ${CODEMGR_WS}/ws_root/variables/memorymap
include ${CODEMGR_WS}/ws_root/rules/extrasrc
endif

