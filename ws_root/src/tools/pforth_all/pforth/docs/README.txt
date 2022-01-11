/* @(#) README.txt 96/06/03 1.12 */

README for pForth - a Portable ANS-like Forth written in ANSI 'C'

by Phil Burk
with Larry Polansky, David Rosenboom and Darren Gibbs.

Last updated: 5/28/96  V10

NOTE: This version of pForth is only for EVALUATION and TESTING.
As a courtesy, please do not distribute pForth until we release
the first official version.

Please direct feedback, bug reports, and suggestions to: phil@3do.com

-- LEGAL NOTICE -----------------------------------------

The pForth software code is dedicated to the public domain,
and any third party may reproduce, distribute and modify
the pForth software code or any derivative works thereof
without any compensation or license.  The pForth software
code is provided on an "as is" basis without any warranty
of any kind, including, without limitation, the implied
warranties of merchantability and fitness for a particular
purpose and their equivalents under the laws of any jurisdiction.

-- ANS Compatibility ---------------------------------------

This Forth is intended to be ANS compatible.  I will not claim
that it is compatible until more people bang on it.  If you find
areas where it deviates from the standard, please let me know.

Word sets supported include:
    FLOAT
    LOCAL with support for { lv1 lv2 | lv3 -- } style locals
    EXCEPTION but standard throw codes not implemented
    FILE ACCESS
    MEMORY ALLOCATION

Here are the areas that I know are not compatible:

The ENVIRONMENT queries are not implemented.

Word sets NOT supported include:
    BLOCK - a matter of religion
    SEARCH ORDER - coming soon
    PROGRAMMING TOOLS - only has .S ? DUMP WORDS BYE
    STRING - only has CMOVE CMOVE> COMPARE
    DOUBLE NUMBER - but cell is 32 bits

When I run the coretest.fth, it fails for the following tests:
    INCORRECT RESULT: { max-uint max-uint UM* max-uint UM/MOD -> 0 max-uint }
    INCORRECT RESULT: { BL WORD

-- The Origins of pForth --------------------------------

PForth began as a JSR threaded 68000 Forth called HForth that was used
to support HMSL, the Hierarchical Music Specification Language.
HMSL was a music experimentation language developed by Phil Burk,
Larry Polansky and David Rosenboom while working at the Mills College
Center for Contemporary Music.  Phil moved from Mills to the 3DO Company
where he ported the Forth kernel to 'C'.  It was used at 3DO as a
tool for verifying ASIC design and for bringing up new hardware
platforms.  At 3DO, the Forth had to run on many systems including
SUN, SGI, Macintosh, Amiga, the 3DO ARM based Opera system, and the 3DO
PowerPC based M2 system.

(Phil Burk also worked on the JForth package with Mike Haas when he was
a partner at Delta Research.)

-- pForth Design Goals ----------------------------------

PForth has been designed with portability as the primary design goal.
As a result, pForth avoids any fancy UNIX calls.  pForth also
avoids using any clever and original ways of constructing the Forth
dictionary.  It just compiles its kernel from ANSI compatible 'C'
code then loads ANS compatible Forth code to build the dictionary.
Very boring but very likely to work on almost any platform. (Note:
pForth has only been tested on systems with normal byte ordering.
Apparently some old fashioned computers based on early 1980's technology
use backwards byte ordering and may not work with pForth. :-)

The dictionary files that can be saved from pForth are host independant.
They can be compiled on one processor, and then run on another processor.

PForth can be used to bring up minimal hardware systems that have
very few system services implemented. It is possible to compile pForth
for systems that only support routines to send and receive a single
character.  If malloc() and free() are not available, equivalent
functions are available in standard 'C' code. If file I/O is not available,
the dictionary can be saved as a static data array in 'C' source format
on a host system. The dictionary in 'C' source form is then compiled
with a custom pForth kernel to avoid having to read the dictionary
from disk.

-- How To Build pForth ----------------------------------

UNIX:
    1) cd to top directory of pForth
    2) Enter in shell:   gmake

MACINTOSH:
    1) cd to top directory of pForth
    2) Perform full build of target "pforth"

-- How to run PForth ------------------------------------

Once you have compiled and built the dictionary, just enter:
     pforth

To verify that PForth is working, enter:
     3 4 + .
It should print "7  ok"

To compile source code files use:    INCLUDE filename

To create a custom dictionary enter in pForth:
	c" newfilename.dic" SAVE-FORTH
The name must end in ".dic".

To run PForth with the new dictionary enter in the shell:
	pforth newfilename.dic

To run PForth and automatically include a forth file:
	pforth myprogram.fth

You can test the Forth without loading a dictionary.
Enter:   pforth -i
In pForth, enter:    3 4 + .
In pForth, enter:    loadsys
In pForth, enter:    10  0  do i . loop

-- How to build a BootStrap pForth ----------------------

You may want to create a version of pForth that can be run on
a small system that does not support file I/O.  This is useful
when bringing up new computer systems.  Here are the steps to
create an embedded pForth.

1) Compile your custom Forth words on a host development system.

2) Compile the pForth utulity "utils/savedicd.fth".

3) Enter in pForth:  SDAD

4) SDAD will generate a file called "pfdicdat.h" that contains your
dictionary in source code form.

5) Rewrite the character primitives sdOutputChar() and sdInputChar()
defined in pf_host.h to use your new computers communications port.

6) Compile a new version of pForth for your target machine
with PF_NO_FILEIO defined. The file "pfdicdata.h" will be
compiled into this executable and your dictionary will thus be
included in the pForth executable as a static array.

7) Run the new pForth on your target machine.

-- Version History --------------------------------------

V1 - 5/94
	- built pForth from various Forths including HMSL
	
V2 - 8/94
	- made improvements necessary for use with M2 Verilog testing
	
V3 - 3/1/95
	- Added support for embedded systems: PF_NO_FILEIO
	and PF_NO_MALLOC.
	- Fixed bug in dictionary loader that treated HERE as name relative.

V4 - 3/6/95
	- Added smart conditionals to allow IF THEN DO LOOP etc.
	  outside colon definitions.
	- Fixed RSHIFT, made logical.
	- Added ARSHIFT for arithmetic shift.
	- Added proper M*
	- Added <> U> U<
	- Added FM/MOD SM/REM /MOD MOD */ */MOD
	- Added +LOOP EVALUATE UNLOOP EXIT
	- Everything passes "coretest.fth" except UM/MOD FIND and WORD

V5 - 3/9/95
	- Added pfReportError()
	- Fixed problem with NumPrimitives growing and breaking dictionaries
	- Reduced size of saved dictionaries, 198K -> 28K in one instance
	- Funnel all terminal I/O through ioKey() and ioEmit()
	- Removed dependencies on printf() except for debugging
	
V6 - 3/16/95
	- Added floating point
	- Changed NUMBER? to return a numeric type
	- Support double number entry, eg.   234.  -> 234 0
	
V7 - 4/12/95
	- Converted to 3DO Teamware environment
	- Added conditional compiler [IF] [ELSE] [THEN], use like #if
	- Fixed W->S B->S for positive values
	- Fixed ALLOCATE FREE validation.  Was failing on some 'C' compilers.
	- Added FILE-SIZE
	- Fixed ERASE, now fills with zero instead of BL

V8 - 5/1/95
	- Report line number and line dump when INCLUDE aborts
	- Abort if stack depth changes in colon definition. Helps
	  detect unbalanced conditionals (IF without THEN).
	- Print bytes added by include.  Helps determine current file.
	- Added RETURN-CODE which is returned to caller, eg. UNIX shell.
	- Changed Header and Code sizes to 60000 and 150000
	- Added check for overflowing dictionary when creating secondaries.
	
V9 - 10/13/95
	- Cleaned up and documented for alpha release.
	- Added EXISTS?
	- compile floats.fth if F* exists
	- got PF_NO_SHELL working
	- added TURNKEY to build headerless dictionary apps
	- improved release script and rlsMakefile
	- added FS@ and FS! for FLPT structure members
	
V10 - 3/21/94
	- Close nested source files when INCLUDE aborts.
	- Add PF_NO_CLIB option to reduce OS dependencies.
	- Add CREATE-FILE, fix R/W access mode for OPEN-FILE.
	- Use PF_FLOAT instead of FLOAT to avoid DOS problem.
	- Add PF_HOST_DOS for compilation control.
	- Shorten all long file names to fit in the 8.3 format
	  required by some primitive operating systems. My
	  apologies to those with modern computers who suffer
	  as a result.  ;-)

-- Custom Compilation of pForth kernel -------------------

There are several versions of PForth that can be built.
By default, the full kernel will be built.  For custom builds,
define the following options in the Makefile before compiling
the 'C' code:

    PF_NO_INIT   = don't compile the code used to initially build
                   the dictionary.

    PF_NO_SHELL  = don't compile the outer interpreter and Forth compiler.
	
    PF_NO_MALLOC = system has no malloc() function so define our own.

    PF_NO_CLIB   = replace 'C' lib calls like toupper and memcpy with
                   local version.  This is useful for embedded systems.

    PF_MEM_POOL_SIZE = size of array in bytes used by custom allocator.

    PF_NO_FILEIO = system has no fileio so load static dictionary.
                   Use "save_dic_as_data.fth" to save a dictionary
                   as 'C' source code in a file called "pf_data_dic.h".
    
    PF_SUPPORT_FP = compile ANSI floating point support

The following defines are set automatically in the appropriate Makefiles:

    PF_HOST_MACINTOSH = compile for Macintosh systems
    PF_HOST_DOS = compile for PC DOS systems
	
For example, to build a system that only runs turnkey or cloned binaries:

    -DPF_NO_INIT -DPF_NO_SHELL

To build a system that only runs in an embedded system using
only getc and putc:

    -DPF_NO_INIT -DPF_NO_MALLOC -DPF_NO_FILEIO

-- DESIGN -----------------------------------------------

This Forth supports multiple dictionaries.
Each dictionary consists of a header segment and a seperate code segment.
The header segment contains link fields and names.
The code segment contains tokens and data.
The headers, as well as some entire dictionaries such as the
compiler support words, can be discarded
when creating a stand-alone app.

[NOT IMPLEMENTED] Dictionaries can be split so that the compile time words can
be placed above the main dictionary.  Thus they can use
the same relative addressing but be discarded when turnkeying.

Execution tokens are either an index of a primitive ( n < NUM_PRIMITIVES),
or the offset of a secondary in the code segment. ( n >= NUM_PRIMITIVES )

Issues: ------------------
How should we handle USER variables???

-- DICTIONARY DATA STRUCTURES  --------------------------

The NAME HEADER portion of the dictionary contains a structure
for each named word in the dictionary.  It contains the following
fields:

 bytes    
	4	Link Field	   relative address of previous name header
	4	Code Pointer   relative address of corresponding code
	n	Name Field     name as counted string
	
Headers are quad byte aligned.

The CODE portion of the dictionary consists of
the following structures:

	PRIMITIVE - no code
		
	SECONDARY
		4*n	Parameter Field   execution tokens
		4	ID_NEXT = 0       terminates secondary
		
	CREATE DOES>
		4	ID_CREATE_P       token 
		4	Token for optional DOES> code, OR ID_NEXT = 0
		4	ID_NEXT = 0
		n	Body = arbitrary data
		
	DEFERRED
		4	ID_DEFER_P    same action as ID_NOOP, identifies deferred words
		4	Execution Token of word to execute.
		4	ID_NEXT = 0
		
	C_CALL
		4	ID_CALL_C
		4	Pack C Call Info
				Bits 0-15  = Function Index
				Bits 16-23 = FunctionTable Index (Unused)
				Bits 24-30 = NumParams
				Bit  31    = 1 if function returns value
		4	ID_NEXT = 0

-- INNER INTERPRETER ----------------

The inner interpreter is implemented in the function ExecuteToken()
which is in pf_inner.c.

	void pfExecuteToken( ExecToken XT );

It is passed an execution token the same as EXECUTE would accept.
It handles threading of secondaries and also has a large switch()case
statement to interpret primitives.  It is in one huge routine to take
advantage of register variables, and to reduce calling overhead.

-- 'C' INTERFACE --------------------

You can call the pForth interpreter as an embedded tool in a 'C'
application.  For an example of this, see the file pf_main.c. This
application does nothing but load the dictionary and call the pForth
interpreter.

You can call 'C' from pForth by adding your own custom 'C' functions
to a dispatch table, and then adding Forth words to the dictionary
that call those functions.  See the file pfcustom.c for more
information.

-- Description of Files --------------------

ansilocs.fth    = support for ANSI standard (LOCAL) word
c_struct.fth    = 'C' like data structures
case.fth        = CASE OF ENDOF ENDCASE
catch.fth       = CATCH and THROW
condcomp.fth   = [IF] [ELSE] [THEN] conditional compiler
floats.fth      = floating point support
forget.fth      = FORGET [FORGET] IF.FORGOTTEN
loadp4th.fth    = loads basic dictionary
locals.fth      = { } style locals using (LOCAL)
math.fth        = misc math words
member.fth      = additional 'C' like data structure support
misc1.fth       = miscellaneous words
misc2.fth       = miscellaneous words
numberio.fth    = formatted numeric input/output
quit.fth        = QUIT EVALUATE INTERPRET in high level
smart_if.fth    = allows conditionals outside colon definition
strings.fth     = string support
system.fth      = bootstraps pForth dictionary

csrc/pf_cglue.c    = glue for pForth calling 'C'
csrc/pfcompil.c    = pForth compiler support
csrc/pf_core.c     = primary words called from 'C' app that embeds pForth
csrc/pfcustom.c    = example of 'C' functions callable from pForth
csrc/pfinnrfp.h    = float extensions to interpreter
csrc/pf_inner.c    = inner interpreter
csrc/pf_guts.h     = primary include file, define structures
csrc/pf_io.c       = input/output
csrc/pf_main.c     = basic application for standalone pForth
csrc/pf_mem.c      = optional malloc() implementation
csrc/pf_save.c     = save and load dictionaries
csrc/pf_host.c     = host system interface
csrc/pf_text.c     = string tools
csrc/pf_tools.c    = miscellaneous tools
csrc/pf_words.c    = miscellaneous pForth words implemented
csrc/pforth.h      = include this in app that embeds pForth

Enjoy,
Phil Burk
