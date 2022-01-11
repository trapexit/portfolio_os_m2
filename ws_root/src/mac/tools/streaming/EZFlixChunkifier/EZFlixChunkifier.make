#####################################
##
##      @(#) EZFlixChunkifier.make 96/03/18 1.7
##
#####################################
#
#	File:		EZFlixChunkifier.make
#
#	Contains:	Main Make for EZFlixChunkifier
#				Builds a FAT tool using Make and MakeMake
#
#	Written by:	Donn Denman and Greg Wallace
#
#	To Do:
#
#
#APPL			= EZFlixChunkifier
#
#{APPL} �� {APPL}.68K.Make
#	if ! `exists :Objects:`
#		newfolder :Objects:
#	end
#	Make -f {APPL}.68K.Make > Makeout; Makeout; Delete Makeout;
#	
#{APPL} �� {APPL}.makemake {APPL}.defaultRules
#	if ! `exists :Objects:`
#		newfolder :PPCObjects:
#	end
#	if !`exists {APPL}.xcoff.Make` �
#			|| `newer {APPL}.makemake {APPL}.xcoff.Make` �
#			|| `newer {APPL}.defaultRules {APPL}.xcoff.Make`
#		MakeMake {APPL}.makemake
#	end
#	Make -f {APPL}.xcoff.Make > Makeout; Makeout; Delete Makeout;

Destination 	= "{3DOMPWTools}"
APPL			= EZFlixChunkifier

ALL					� {Destination}{APPL}
{APPL}				� {Destination}{APPL}

{Destination}{APPL}	� $OutOfDate
	BuildEZFlixChunkifier update
	
{Destination} 		� : :Includes: :EZFlixEncoder:
