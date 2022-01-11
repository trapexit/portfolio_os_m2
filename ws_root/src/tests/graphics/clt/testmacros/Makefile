# @(#) Makefile 95/09/25 1.1

include ${CODEMGR_WS}/ws_root/variables/linkexec

CCOPTIONS =	-Xstring-align=1 -Xno-double -Xdouble-error -Ximport \
			-Xstrict-ansi -Xunsigned-char -XO -Xunroll=1 \
			-Xtest-at-bottom -Xextend-args=0 -Xforce-prototypes \
			-Xuse-float -Xinline=5 -Xno-libc-inlining \
			-Xno-recognize-lib -Xno-bss=2 -Xlint=0x80 -Xsmall-data=0 \
			-Xsmall-const=0 -Xstrings-in-text -Xuse-.init=0 \
			-Xtrace-table=0

include ${CODEMGR_WS}/ws_root/rules/linkexec
