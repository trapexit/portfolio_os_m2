#! /bin/sh

# elfsize [-tdb] [-v] file...

# Prints the decimal size of the sections of an ELF file.
# The -t -d and -b flags control which sections are examined:
#	-t  Text
#	-d  Data
#	-b  BSS
# If more than one flag is specified, the sum of the sizes 
# is printed.  For example "elfsize -db file" prints the sum
# of the sizes of the data and bss sections.
# The -v flag causes the name of the file to be printed along
# with the size.

USAGE="Usage: elfsize [-tdb] [-v] file..."

ADD_TEXT=0
ADD_DATA=0
ADD_BSS=0
VERBOSE=0
while getopts "tdbv" ch
do
	case $ch in
	t) ADD_TEXT=1 ;;
	d) ADD_DATA=1 ;;
	b) ADD_BSS=1  ;;
	v) VERBOSE=1  ;;
	*) echo "$USAGE" ; exit 2 ;;
	esac
done
shift `expr $OPTIND - 1`

if [ $# -gt 1 ]; then VERBOSE=1; fi

elf_size $* | tail +2 | while read TEXT DATA BSS hTEXT hDATA FILE
do
	SIZE=0
	if [ $ADD_TEXT = 1 ]; then SIZE=`expr $SIZE + $TEXT`; fi
	if [ $ADD_DATA = 1 ]; then SIZE=`expr $SIZE + $DATA`; fi
	if [ $ADD_BSS  = 1 ]; then SIZE=`expr $SIZE + $BSS` ; fi
	if [ $VERBOSE = 1 ]; then
		echo "$SIZE $FILE"
	else
		echo "$SIZE"
	fi
done
