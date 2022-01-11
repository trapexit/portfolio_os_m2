# @(#) Makefile 95/10/27 1.1

IMPORTED_FILES = compress.c decompress.c lzss.h simplecompress.c simpledecompress.c
UNREGISTERED_FILES = ${IMPORTED_FILES}

include ${CODEMGR_WS}/ws_root/variables/generic
include ${CODEMGR_WS}/ws_root/rules/generic

MAC_FIXINCLUDES = 	${HOSTOSBIN}/fix_includes_mac
MAC_FIXEOL =	  	${HOSTOSBIN}/fix_eol_mac

compress.c:
	cat ${SRC}/folios/compression/compress.c | ${MAC_FIXINCLUDES} | ${MAC_FIXEOL} > $@

decompress.c:
	cat ${SRC}/folios/compression/decompress.c | ${MAC_FIXINCLUDES} | ${MAC_FIXEOL} > $@

lzss.h:
	cat ${SRC}/folios/compression/lzss.h | ${MAC_FIXINCLUDES} | ${MAC_FIXEOL} > $@

simplecompress.c:
	cat ${SRC}/folios/compression/simplecompress.c | ${MAC_FIXINCLUDES} | ${MAC_FIXEOL} > $@

simpledecompress.c:
	cat ${SRC}/folios/compression/simpledecompress.c | ${MAC_FIXINCLUDES} | ${MAC_FIXEOL} > $@

all.post: ${IMPORTED_FILES}

clean:
	${RM} ${IMPORTED_FILES}
