# @(#) Makefile 96/10/18 1.124
#

# To make a dipir, you must specify a target architecture and a configuration.
# These can be specified by setting DIPIR_ARCH and DIPIR_CONFIG in the
# environment.  If they are not set, the default is to build a set of
# default configurations and architectures.

# Resulting objects and binaries are put in a
# subdirectory $DIPIR_ARCH/$DIPIR_CONFIG.

UNREGISTERED_FILES =	ppc_clib.lib dipir.cd.enc.c dipir.cd.unenc.c
NO_RELOCS =		1
UNCOMPRESSED_MODULE =	1
ELFSIZE =		${HOSTOSBIN}/elfsize

EXTRA_SRC =	${SRC}/folios/compression/decompress.c
INCDIRS =	${SRC}/folios/compression

ifeq (${BUILD_PCDEBUGGER},YES)
BUILD_DEBUGGER = NO
TEMP := $(OPTIONS_FLAGS)
OPTIONS_FLAGS = $(subst -DBUILD_DEBUGGER,,$(TEMP))
endif

############################################################################
# Target architectures and configurations.
#

# All possible architectures and configurations.
ARCHS=			m2 m2.16 m2.flash

# Default architectures and configurations.
DEFAULT_ARCHS=		m2 m2.16 m2.flash

# Now select the list we will actually be building

ifndef DIPIR_ARCH
ARCH_LIST =	${DEFAULT_ARCHS}
else
ARCH_LIST =	${DIPIR_ARCH}
endif

ifeq (1, ${words ${ARCH_LIST}})
ifndef DIPIR_ARCH
DIPIR_ARCH = ${ARCH_LIST}
endif
endif


############################################################################
# Compiler flags dependent on the architecture and configuration.
#

# Architectures (based on DIPIR_ARCH)
DEFINES_m2 =		-DNUM_PCMCIA_SLOTS=2 \
			-DNUM_MICRO_SLOTS=4 \
			-DPCMCIA_TUPLES=1 \
			-DBUILD_PC16550=1
LFLAGS_m2 =

DEFINES_m2.16 =		${DEFINES_m2}
LFLAGS_m2.16 =		${LFLAGS_m2}

DEFINES_m2.flash =	${DEFINES_m2}
LFLAGS_m2.flash =	${LFLAGS_m2}


############################################################################

ifeq (${DIPIR_ARCH}, m2.16)
BUILD_16M = YES
endif

ifeq (${DIPIR_ARCH}, m2.flash)
BUILD_FLASH = YES
endif

include ${CODEMGR_WS}/ws_root/variables/generic
include ${CODEMGR_WS}/ws_root/variables/memorymap

############################################################################
# Defintions of some constants.
#

DIPIRSTACKSIZE =	3072

# These should match DIPIRBUFSTART in variables/memorymap.
DDD_ADDR =		0x40004000 # 16K
  DDD_SIZE =			16384
DDD_DATAADDR =		0x40008000 #  8K
  DDD_DATASIZE =		8192
DEVDIPIR_ADDR =		0x4000a000 # 12K
  DEVDIPIR_SIZE =		12288
DEVDIPIR_DATAADDR =	0x4000d000 #  8K
  DEVDIPIR_DATASIZE =		8192

# Make sure they match the definitions in variables/memorymap.
ifneq (${strip ${DDD_ADDR}},${strip ${DIPIRBUFSTART}})
@@@@ Ack! Foo! The above definitions need to be changed to match memorymap!
endif

############################################################################
# Compiler flags
#

GENDEFINES =		${MAP_ALL_DEFINES} \
			${DEBUG_DEFS} \
			-DDIPIRSTACKSIZE=${DIPIRSTACKSIZE} \
			-DNO_PORTFOLIO

# Compiler flags for ASM
ADEFINES =		${GENDEFINES}

# Compiler flags for C
CDEFINES =		${GENDEFINES} \
			-Xstrings-in-text -Xstmw-fast
# Linker flags
LFLAGS =		-g


############################################################################
# Directories and files.
#
OBJDIR =		objects/${BUILD_TYPE}
ROMDIR =		${OBJDIR}/${DIPIR_ARCH}
DRVDIR =		${OBJDIR}/drivers
DIPDIR =		${OBJDIR}/dipirs
DIPIR_CLIB =		ppc_clib.lib
DIPIR_DIPLIB =		${DIPDIR}/diplib.a

# List of drivers to build.
DIPIR_DRIVERS :=	${DRVDIR}/dev.lccd
ifeq (${BUILD_DEBUGGER}, YES)
DIPIR_DRIVERS +=	${DRVDIR}/dev.host \
			${DRVDIR}/dev.hostcd
endif

# List of device-dipirs to build
DEVICE_DIPIRS :=	${DIPDIR}/dipir.microcard \
			${DIPDIR}/dipir.cd.unenc \
			${DIPDIR}/dipir.cd.enc \
			${DIPDIR}/dipir.sysromapp \
			${DIPDIR}/dipir.bootromapp \
			${DIPDIR}/dipir.romapp \
			${DIPDIR}/dipir.visa \
			${DIPDIR}/dipir.cart \
			${DIPDIR}/dipir.pc16550 \
			${DIPDIR}/dipir.pcdevcard \
			${DIPDIR}/dipir.tstation \
			${DIPDIR}/dipir.mediadbg
ifeq (${BUILD_DEBUGGER}, YES)
DEVICE_DIPIRS +=	${DIPDIR}/dipir.host
endif


# List of everything to build
ifdef DIPIR_ARCH
HDW=			${ROMDIR}/romdipir
LOCALS=			${DEVICE_DIPIRS} ${DIPIR_DRIVERS}
endif


############################################################################
# Implicit rules for building objects.
#

#  Rules for standard (ROM dipir) objects.
${ROMDIR}/%.so: %.s
	${AS} ${AINCLUDES} ${ADEFINES} ${AFLAGS} ${DEFINES_${DIPIR_ARCH}} -o $@ $<
${ROMDIR}/%.co: %.c
	${CC} -c -o $@ ${CDEFINES} ${CINCLUDES} ${CFLAGS} ${DEFINES_${DIPIR_ARCH}} $<

#  Rules for building driver objects.
${DRVDIR}/%.so: %.s
	${AS} ${AINCLUDES} ${ADEFINES} ${AFLAGS} -o $@ $<
${DRVDIR}/%.co: %.c
	${CC} -c -o $@ ${CDEFINES} ${CINCLUDES} ${CFLAGS} $<

#  Rules for building device-dipir objects.
${DIPDIR}/%.so: %.s
	${AS} ${AINCLUDES} ${ADEFINES} ${AFLAGS} -o $@ $<
${DIPDIR}/%.co: %.c
	${CC} -c -o $@ ${CDEFINES} ${CINCLUDES} ${CFLAGS} $<


############################################################################
# How to build a device dipir.
#
define build_device_dipir
	${LD} -D -e DeviceDipir -Hname=`basename $@` -o $@ \
	        -Bt=${DEVDIPIR_ADDR} -Bd=${DEVDIPIR_DATAADDR} ${LFLAGS} ${^:${DIPIR_DIPLIB}=-l${DIPIR_DIPLIB}}
	@if [ `${ELFSIZE} -t $@` -gt ${DEVDIPIR_SIZE} ]; then \
		echo "### $@ too big! `${ELFSIZE} -t $@` > ${DEVDIPIR_SIZE}" ; \
		rm $@; exit 1 ; fi
	@if [ `${ELFSIZE} -db $@` -gt ${DEVDIPIR_DATASIZE} ]; then \
		echo "### $@ too big! `${ELFSIZE} -db $@` > ${DEVDIPIR_DATASIZE}" ; \
		rm $@; exit 1 ; fi
endef

############################################################################
# How to build a dipir device driver (DDD).
#
define build_dipir_driver
	${LD} -D -e InitDriver -Hname=`basename $@` -o $@ \
	        -Bt=${DDD_ADDR} -Bd=${DDD_DATAADDR} ${LFLAGS} $^
	@if [ `${ELFSIZE} -t $@` -gt ${DDD_SIZE} ]; then \
		echo "### $@ too big! `${ELFSIZE} -t $@` > ${DDD_SIZE}" ; \
		rm $@; exit 1 ; fi
	@if [ `${ELFSIZE} -db $@` -gt ${DDD_DATASIZE} ]; then \
		echo "### $@ too big! `${ELFSIZE} -db $@` > ${DDD_DATASIZE}" ; \
		rm $@; exit 1 ; fi
endef

############################################################################
# "default", "all" and "production"
#
default: all

ifeq (1, ${words ${ARCH_LIST}})

all ftp production: makedirs.here ${HDW} ${LOCALS}

FTP_TRANSFER =

else

all ftp production:
	@for arch in ${ARCH_LIST}; do \
	    echo "===== building $$arch =====" ; \
	    ${MAKE} DIPIR_ARCH=$$arch $@ ; \
	done

endif


############################################################################
# Make subdirectories (generic handles ${OBJDIR})
#

ifeq (1, ${words ${ARCH_LIST}})

makedirs.here: ${ROMDIR} ${DRVDIR} ${DIPDIR}

${ROMDIR}:
	@[ -d $@ ] || ${MD} $@
${DRVDIR}:
	@[ -d $@ ] || ${MD} $@
${DIPDIR}:
	@[ -d $@ ] || ${MD} $@

endif

############################################################################
# ppc_clib.lib: Dipirs clib (not really a lib, its an obj file.  I lib'd)
#
${DIPIR_CLIB}: ppc_clib.s
	${AS} -I${SRC}/includes ${AINCLUDES} ${AFLAGS} -o $@ $<

############################################################################
# romdipir: The dipir that sits in the system ROM.
#
dipir_SSRC =	m2dipir.s rsa_subs.s m2ecc32.s \
		memcpy.s memset.s
dipir_CSRC =	m2dipir.c m2sysdep.c \
		alloc.c boot.c dbuffer.c ddd.c display.c hwdev.c \
		icons.c loader.c md5c.c ntgdigest.c print.c rand.c \
		rommisc.c romtag.c rsa.c rsadipir.c tiny.c \
		dev.chan.c decompress.c \
		chan.powerbus.c chan.lccd.c chan.bridgit.c \
		chan.microcard.c chan.pcmcia.c chan.sysmem.c \
		chan.misc.c chan.host.c chan.hostcd.c \
		va_arg.c

ROMDIPIR_OBJS = \
	${dipir_SSRC:%.s=${ROMDIR}/%.so} \
	${dipir_CSRC:%.c=${ROMDIR}/%.co}

#### romdipir

${ROMDIR}/romdipir: ${ROMDIPIR_OBJS} ${DIPIR_CLIB}
	${echoconfig}
	${LD} -e DipirEntry -Hname=romdipir -o $@ \
	        -Bt=${DIPIRCODESTART} -Bd=${DIPIRDATASTART} \
		${LFLAGS_${DIPIR_ARCH}} ${LFLAGS} $^


############################################################################
# diplib.a: Library for device dipirs.
#
DIPLIB_SRC = \
	diplib.banner.c \
	diplib.boot.c \
	diplib.cdmode.c \
	diplib.comp.c \
	diplib.devperm.c \
	diplib.icon.c \
	diplib.list.c \
	diplib.loados.c \
	diplib.reloc.c \
	diplib.replace.c \
	diplib.scratch.c \
	diplib.verify.c \
	diplib.wdata.c

${DIPIR_DIPLIB}: ${DIPLIB_SRC:%.c=${DIPDIR}/%.co}
	rm -rf ${DIPIR_DIPLIB}
	${AR} ${AR_OPTIONS} $@ $^

############################################################################
# dipir.cd.unenc: The dipir that sits on a development title CD.
#
${DIPDIR}/dipir.cd.unenc_CSRC = dipir.cd.unenc.c ddd.cd.c
${DIPDIR}/dipir.cd.unenc: ${${DIPDIR}/dipir.cd.unenc_CSRC:%.c=${DIPDIR}/%.co} ${DIPIR_CLIB} ${DIPIR_DIPLIB}
	${build_device_dipir}
dipir.cd.unenc.c: dipir.cd.c
	rm -rf $@
	cat dipir.cd.c > $@

############################################################################
# dipir.cd.enc: The dipir that sits on a title CD.
#
${DIPDIR}/dipir.cd.enc_CSRC = dipir.cd.enc.c ddd.cd.c
${DIPDIR}/dipir.cd.enc: ${${DIPDIR}/dipir.cd.enc_CSRC:%.c=${DIPDIR}/%.co} ${DIPIR_CLIB} ${DIPIR_DIPLIB}
	${build_device_dipir}
dipir.cd.enc.c: dipir.cd.c
	rm -rf $@
	echo '#define ENCRYPT' > $@
	echo '#define STRICT_DIPIR' >> $@
	cat dipir.cd.c >> $@

############################################################################
# dipir.romapp: The dipir that validates a RomApp cartridge.
#
${DIPDIR}/dipir.romapp_CSRC = dipir.romapp.c
${DIPDIR}/dipir.romapp: ${${DIPDIR}/dipir.romapp_CSRC:%.c=${DIPDIR}/%.co} ${DIPIR_CLIB} ${DIPIR_DIPLIB}
	${build_device_dipir}

############################################################################
# dipir.bootromapp: The dipir that loads a RomApp OS.
#
${DIPDIR}/dipir.bootromapp_CSRC = dipir.bootromapp.c
${DIPDIR}/dipir.bootromapp: ${${DIPDIR}/dipir.bootromapp_CSRC:%.c=${DIPDIR}/%.co} ${DIPIR_CLIB} ${DIPIR_DIPLIB}
	${build_device_dipir}

############################################################################
# dipir.sysromapp: The dipir that loads the RomApp OS from the system ROM.
#
${DIPDIR}/dipir.sysromapp_CSRC = dipir.sysromapp.c
${DIPDIR}/dipir.sysromapp: ${${DIPDIR}/dipir.sysromapp_CSRC:%.c=${DIPDIR}/%.co} ${DIPIR_CLIB} ${DIPIR_DIPLIB}
	${build_device_dipir}

############################################################################
# dipir.visa: dipir for VISA cards with "tiny" ROMS.
#
${DIPDIR}/dipir.visa_CSRC = dipir.visa.c
${DIPDIR}/dipir.visa: ${${DIPDIR}/dipir.visa_CSRC:%.c=${DIPDIR}/%.co} ${DIPIR_CLIB} ${DIPIR_DIPLIB}
	${build_device_dipir}

############################################################################
# dipir.host: The dipir that loads the OS from the host.
#
${DIPDIR}/dipir.host_CSRC = dipir.host.c  ddd.host.c
${DIPDIR}/dipir.host: ${${DIPDIR}/dipir.host_CSRC:%.c=${DIPDIR}/%.co} ${DIPIR_CLIB} ${DIPIR_DIPLIB}
	${build_device_dipir}

############################################################################
# dipir.k: Kludge dipir for testing only.
#
${DIPDIR}/dipir.k_CSRC = dipir.k.c
${DIPDIR}/dipir.k: ${${DIPDIR}/dipir.k_CSRC:%.c=${DIPDIR}/%.co} ${DIPIR_CLIB} ${DIPIR_DIPLIB}
	${build_device_dipir}

############################################################################
# dipir.cart: Kludge dipir for testing only.
#
${DIPDIR}/dipir.cart_CSRC = dipir.cart.c
${DIPDIR}/dipir.cart: ${${DIPDIR}/dipir.cart_CSRC:%.c=${DIPDIR}/%.co} ${DIPIR_CLIB} ${DIPIR_DIPLIB}
	${build_device_dipir}

############################################################################
# dipir.microcard: Dipir for Microcards with "tiny" ROMs.
#
${DIPDIR}/dipir.microcard_CSRC = dipir.microcard.c
${DIPDIR}/dipir.microcard: ${${DIPDIR}/dipir.microcard_CSRC:%.c=${DIPDIR}/%.co} ${DIPIR_CLIB} ${DIPIR_DIPLIB}
	${build_device_dipir}

############################################################################
# dipir.pc16550: The dipir that validates the prototype PC16550 card.
#
${DIPDIR}/dipir.pc16550_CSRC = dipir.pc16550.c
${DIPDIR}/dipir.pc16550: ${${DIPDIR}/dipir.pc16550_CSRC:%.c=${DIPDIR}/%.co} ${DIPIR_CLIB} ${DIPIR_DIPLIB}
	${build_device_dipir}

############################################################################
# dipir.pcdevcard: The dipir that validates the PC developer card.
#
${DIPDIR}/dipir.pcdevcard_CSRC = dipir.pcdevcard.c
${DIPDIR}/dipir.pcdevcard: ${${DIPDIR}/dipir.pcdevcard_CSRC:%.c=${DIPDIR}/%.co} ${DIPIR_CLIB} ${DIPIR_DIPLIB}
	${build_device_dipir}

############################################################################
# dipir.tstation: Dipir for the test station card.
#
${DIPDIR}/dipir.tstation_CSRC = dipir.tstation.c
${DIPDIR}/dipir.tstation: ${${DIPDIR}/dipir.tstation_CSRC:%.c=${DIPDIR}/%.co} ${DIPIR_CLIB} ${DIPIR_DIPLIB}
	${build_device_dipir}

############################################################################
# dipir.mediadbg: A debugging dipir for media testing.
#
${DIPDIR}/dipir.mediadbg_CSRC = dipir.mediadbg.c ddd.cd.c
${DIPDIR}/dipir.mediadbg: ${${DIPDIR}/dipir.mediadbg_CSRC:%.c=${DIPDIR}/%.co} ${DIPIR_CLIB} ${DIPIR_DIPLIB}
	${build_device_dipir}




############################################################################
# dev.lccd: The low cost cd driver for M2's internal CDE drive
#
${DRVDIR}/dev.lccd_CSRC = dev.lccd.c lccd_cde.c
${DRVDIR}/dev.lccd: ${${DRVDIR}/dev.lccd_CSRC:%.c=${DRVDIR}/%.co} ${DIPIR_CLIB}
	${build_dipir_driver}

############################################################################
# dev.host: Driver for Host files.
#
${DRVDIR}/dev.host_CSRC = dev.hostfs.c hostfscmd.c hostutil.c
${DRVDIR}/dev.host: ${${DRVDIR}/dev.host_CSRC:%.c=${DRVDIR}/%.co} ${DIPIR_CLIB}
	${build_dipir_driver}

############################################################################
# dev.hostcd: Driver for the Host CD-ROM emulator.
#
${DRVDIR}/dev.hostcd_CSRC = dev.hostcd.c hostcdcmd.c hostutil.c
${DRVDIR}/dev.hostcd: ${${DRVDIR}/dev.hostcd_CSRC:%.c=${DRVDIR}/%.co} ${DIPIR_CLIB}
	${build_dipir_driver}



############################################################################
# other stuff
#
clean:
	/bin/rm -rf ${OBJDIR} dipir.cd.enc.c dipir.cd.unenc.c\
		decompress.c lzss.h

include ${CODEMGR_WS}/ws_root/rules/generic
include ${CODEMGR_WS}/ws_root/rules/extrasrc
