# @(#) Makefile 96/08/19 1.6

include ${CODEMGR_WS}/ws_root/variables/generic
include ${CODEMGR_WS}/ws_root/variables/memorymap

DMNUM =			${HOSTOSBIN}/dmnum

TARGETS =		${RELEASE_DIR}/dbinfo.8M \
			${RELEASE_DIR}/dbinfo.16M \
			${RELEASE_DIR}/dbinfo.flash

FTP_TRANSFER =		${TARGETS}

include ${CODEMGR_WS}/ws_root/rules/generic

clean:
	-@${RM} ${TARGETS}

ifeq (${DBINFO},)

all.post: ${TARGETS}

${RELEASE_DIR}/dbinfo.8M: Makefile
	${MAKE} DBINFO=$@ HIMEM=0x407f DBG_FLAGS=0 $@
${RELEASE_DIR}/dbinfo.16M: Makefile
	${MAKE} DBINFO=$@ HIMEM=0x40ff BUILD_16M=YES DBG_FLAGS=0 $@
${RELEASE_DIR}/dbinfo.flash: Makefile
	${MAKE} DBINFO=$@ HIMEM=0x407f BUILD_FLASH=YES DBG_FLAGS=0 $@

else

${DBINFO}: Makefile
	rm -rf $@
	@echo "Building $@"
	@${DMNUM} -w4 0xE36CA3C8	>> $@	# dbg_Magic
	@${DMNUM} -w4 3			>> $@	# dbg_Version
	@${DMNUM} -w1 0 0 ${OS_VERSION} ${OS_REVISION} >> $@ # dbg_OSVersion
	@${DMNUM} -w4 0x`date +%y%d%m`	>> $@	# dbg_OSTime
	@${DMNUM} -w4 ${HIMEM}ffc0	>> $@	# dbg_CommInPtr
	@${DMNUM} -w4 0x20		>> $@	# dbg_CommInSize
	@${DMNUM} -w4 ${HIMEM}ffe0	>> $@	# dbg_CommOutPtr
	@${DMNUM} -w4 0x20		>> $@	# dbg_CommOutSize
	@${DMNUM} -w4 ${HIMEM}ff80	>> $@	# dbg_PrintfPtr
	@${DMNUM} -w4 0x40		>> $@	# dbg_PrintfSize
	@${DMNUM} -w4 ${HIMEM}fd00	>> $@	# dbg_MonInPtr
	@${DMNUM} -w4 0x200		>> $@	# dbg_MonInSize
	@${DMNUM} -w4 ${HIMEM}fb00	>> $@	# dbg_MonOutPtr
	@${DMNUM} -w4 0x200		>> $@	# dbg_MonOutSize
	@${DMNUM} -w4 ${SYSROMIMAGE}	>> $@	# dbg_ResetVector
	@${DMNUM} -w4 0			>> $@	# dbg_DBVersion
	@${DMNUM} -w2 0 0		>> $@	# dbg_HostType,DBType
	@${DMNUM} -w4 ${DBG_FLAGS}	>> $@	# dbg_Flags
endif

