\ @(#) log_to_file.fth 96/03/21 1.3
\ $Id: dspp_asm.fth,v 1.25 1995/03/14 07:10:19 phil Exp $
echo off
\ DSPP RPN Assembler
\
\ by Phil Burk
\ Copyright 1992,93,94 3DO
\ All rights reserved.
\ Proprietary and confidential.

anew task-log_to_file.fth

\ log output to a file
\ This is used to create .emu file for the emulator

variable log-fid

: log.type  ( addr cnt -- )
	log-fid @ ?dup  \ is a file open
	if write-file drop  \ then write to it
	else type       \ otherwise type to screen
	then
;

variable emit-file-scratch
: emit-file ( char fid -- ior )
	swap emit-file-scratch c!
	emit-file-scratch 1 rot write-file
;

: log.cr ( -- )
	log-fid @ ?dup
	if $ 0A swap emit-file drop
	else cr
	then
;
: log.emit ( char -- )
	log-fid @ ?dup
	if  emit-file drop
	else emit
	then
;


: log" ( <string"> -- print string to log )
	[compile] p"
	compile count compile log.type
; immediate

: }log ( -- stop logging )
	log-fid @ ?dup
	if
		close-file drop
		log-fid off
	then
;

: $log{ ( $filename -- , open log file )
	}log
	count r/w create-file
	abort" log.start - Couldn't open file!"
	log-fid !
;

: log{ ( -- )
	p" dspp.log" $log{
;
