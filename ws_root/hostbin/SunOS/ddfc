#!/bin/sh

# @(#) ddfc.sh 95/11/09 1.7

if [ $# -ne 3 ]
then
    echo usage: `basename $0` -o out.ddf in.D >&2
    exit 2
fi
B=$CODEMGR_WS/ws_root/hostbin/`uname -s`
if [ ! -x $B/ddfc1 ]
then
    if [ ! -x /usr/software/bin/ddfc1 ]
    then
	echo ddfc1 is not in $B or /usr/software/bin. >&2
	exit 1
    fi
    B=/usr/software/bin
fi
T=/tmp/ddfc.$$
/usr/lib/cpp -undef -B -I$CODEMGR_WS/ws_root/src/includes $3 > $T
$B/ddfc1 $1 $2 $T; ret=$?
#mv $T `pwd`/tmpfile			# for debugging
rm -f $T
exit $ret
