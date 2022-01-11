# @(#) Makefile 96/04/18 1.43

include ${CODEMGR_WS}/ws_root/variables/generic
include ${CODEMGR_WS}/ws_root/variables/memorymap

ifdef DO_STARTUPSCRIPT
SDIR =			${REMOTE_DIR}/System.m2/Scripts
else
SDIR =
endif

QDIR = 			${REMOTE_DIR}/QuickLook

TARGETS =		${RELEASE_DIR}/debugger.scr \
			${RELEASE_DIR}/debugger.16M.scr \
			${RELEASE_DIR}/debugger.flash.scr \
			${RELEASE_DIR}/flash.scr \
			${QDIR}/quicklook \
			${QDIR}/longlook

ifdef DO_STARTUPSCRIPT
TARGETS += ${SDIR}/SystemStartup.m2
endif

FTP_TRANSFER =		${TARGETS}

include ${CODEMGR_WS}/ws_root/rules/generic

makedirs.here: ${QDIR} ${SDIR}

${QDIR} ${SDIR}:
	[ -d $@ ] || ${MD} $@

clean:
	-@${RM} ${TARGETS}

${SDIR}/SystemStartup.m2: SystemStartup.m2
	${FIXCOPYR} < $< | ${FIXEOL} > $@

${QDIR}/quicklook: quicklook
	${FIXCOPYR} < $< | ${FIXEOL} > $@

${QDIR}/longlook: longlook
	${FIXCOPYR} < $< | ${FIXEOL} > $@

${RELEASE_DIR}/flash.scr: Makefile
	@(echo "releasenupup") | ${FIXEOL} > $@

ifeq (${DEBUGGER_SCRIPT},)

all.post: ${TARGETS}

${RELEASE_DIR}/debugger.scr: Makefile
	${MAKE} DEBUGGER_SCRIPT=$@ DEVROM=ROM.m2.debug DBINFO=dbinfo.8M $@

${RELEASE_DIR}/debugger.16M.scr: Makefile
	${MAKE} DEBUGGER_SCRIPT=$@ DEVROM=ROM.m2.16.debug DBINFO=dbinfo.16M BUILD_16M=YES $@

${RELEASE_DIR}/debugger.flash.scr: Makefile
	${MAKE} DEBUGGER_SCRIPT=$@ DBINFO=dbinfo.flash BUILD_FLASH=YES $@

else

${DEBUGGER_SCRIPT}: Makefile
	@(if [ x${DEVROM} = x ]; then : ; else			  \
	    echo "download ${DEVROM} @${SYSROMIMAGE}"; fi	; \
	  echo "download ${DBINFO} @${DEBUGGERREGION}" 		; \
	  echo "" 						; \
	  echo "releasenupup") | ${FIXEOL} > $@
endif
