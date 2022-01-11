#! /bin/sh

#
# ddf2c [-n name] [-o output] input
#
# Creates a C source file in the output file which, when compiled,
# creates two variables:
#	name_Buffer[]	a buffer containing the contents of the input file
#	name_Len	an int32 initialized to the length of the input file

USAGE="$0 [-n name] [-o output] file.ddf"

NAME=DDF
while getopts n:o: ch
do
	case $ch in
	n)	NAME=$OPTARG ;;
	o)	OUTPUT=$OPTARG ;;
	*)	echo "$USAGE" ; exit 1 ;;
	esac
done
shift `expr $OPTIND - 1`

if [ $# != 1 ]; then
	echo $USAGE; exit 1
fi

(
	echo '#include "kernel/types.h"'
	echo "char ${NAME}_Buffer" '[] = {'
	od -vbw1 $1 | sed '
		s/^.......//
		s/[0-7][0-7]*/0&,/
	' ;
	echo '};'
	echo "int32 ${NAME}_Len = sizeof(${NAME}_Buffer);"
) > $OUTPUT
