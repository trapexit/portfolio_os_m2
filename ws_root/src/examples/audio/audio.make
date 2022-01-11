#####################################
##
##  @(#) audio.make 96/03/14 1.11
##
#####################################


#####################################
#	Symbol definitions
#####################################

App			= Audio
ObjectDir	= :Objects:
Apps_Data	= {3DORemote}Examples:{App}:
ModuleDir	= {3DORemote}System.m2:Modules:
CC			= dcc
LINK		= link3do
3DOAutoDup	= -y
M2Libs		= {3DOLibs}{m2librelease}:
DebugFlag	= 0

#####################################
#	Default compiler options
#####################################

#
# This is the entire nested hierarchy of 3DO header files the GfxUtils folder under
# the Examples.
#

Includes	=	-I"{3doincludes}"													¶
				-I"{3doincludes}audio:"												¶
				-I"{3doincludes}device:"											¶
				-I"{3doincludes}file:"												¶
				-I"{3doincludes}graphics:"											¶
				-I"{3doincludes}graphics:clt:"										¶
				-I"{3doincludes}graphics:frame:"									¶
				-I"{3doincludes}graphics:frame2d:"									¶
				-I"{3doincludes}graphics:pipe:"										¶
				-I"{3doincludes}international:"										¶
				-I"{3doincludes}kernel:"											¶
				-I"{3doincludes}loader:"											¶
				-I"{3doincludes}misc:"												¶
				-I"{3doincludes}streaming:"											¶
				-I"{3doincludes}ui:"												¶
				-I"{3dofolder}Examples:{3DORelease}:Graphics:GfxUtils:Includes:"

DccOpts		=	-c -WDDTARGET=PPC602 -Xstring-align=1 -Xdouble-warning -Xuse-float 			¶
				-Ximport -Xstrict-ansi -Xunsigned-char -Xextend-args=0 -Xforce-prototypes	¶
				-Xno-libc-inlining -Xno-recognize-lib -Xno-bss=2 -Xlint=0x10				¶
				-Xtrace-table=0 -Xsmall-data=0 -Xsmall-const=0

LinkOpts	=	-D -r -Htime=now -Hsubsys=1 -Hname={default} -Htype=5 -Hstack=32768

#####################################
#	Object files
#####################################

Objects		=	"{ObjectDir}tone.c.o"					¶
				"{ObjectDir}cacophony.c.o"				¶
				"{ObjectDir}capture_audio.c.o"			¶
				"{ObjectDir}minmax_audio.c.o"			¶
				"{ObjectDir}playmf.c.o"					¶
				"{ObjectDir}playsample.c.o"				¶
				"{ObjectDir}sfx_score.c.o"				¶
				"{ObjectDir}simple_envelope.c.o"		¶
				"{ObjectDir}ta_attach.c.o"				¶
				"{ObjectDir}ta_customdelay.c.o"			¶
				"{ObjectDir}ta_envelope.c.o"			¶
				"{ObjectDir}ta_pitchnotes.c.o"			¶
				"{ObjectDir}ta_spool.c.o"				¶
				"{ObjectDir}ta_sweeps.c.o"				¶
				"{ObjectDir}ta_timer.c.o"				¶
				"{ObjectDir}ta_tuning.c.o"				¶
				"{ObjectDir}windpatch.c.o"

Libs		=	-lmusic			¶
				-leventbroker	¶
				-lfile			¶
				-lc

Modules		=	"{ModuleDir}audio" "{ModuleDir}iff"

#####################################
#	Default build rules
#####################################

All			Ä	{App}

{App}		Ä	Directories {App}.make {OBJECTS}

Directories	Ä
	if ( !{DebugFlag} )
		Set MoreDccOpts		"-XO -Xunroll=1 -Xinline=5 -Xtest-at-bottom"
		Set MoreLinkOpts	""
		Set COPY			"move"
	else
		Set MoreDccOpts		"-g"
		Set MoreLinkOpts	"-g"
		Set COPY			"duplicate"
	end
	If Not "`Exists -d "{ObjectDir}"`"
		NewFolder "{ObjectDir}"
	End
	If Not "`Exists -d "{3DORemote}Examples:"`"
		NewFolder "{3DORemote}Examples:"
	End
	If Not "`Exists -d "{3DORemote}Examples:Audio:"`"
		NewFolder "{3DORemote}Examples:Audio:"
	End

{ObjectDir}	Ä	:

.c.o		Ä	.c
	{CC} {Includes} {DccOpts} {MoreDccOpts} {depDir}{default}.c -o {targDir}{default}.c.o
	{LINK} -o {default}					¶
		{LinkOpts}						¶
		{targDir}{default}.c.o			¶
		-L{M2Libs} {Modules} {LIBS}		¶
		> {targDir}{default}.map
	SetFile {default} -c '3DOD' -t 'PROJ'
	{COPY} {3DOAutoDup} {default} "{Apps_Data}"
