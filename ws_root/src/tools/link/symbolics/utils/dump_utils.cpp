/*  @(#) dump_utils.cpp 96/07/25 1.11 */

//====================================================================
// dumputils.cpp  -  utilities for dumping 
//

#ifndef USE_DUMP_FILE
#include "predefines.h"

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "utils.h"
#endif /* USE_DUMP_FILE */

#include "dumputils.h"
#include "debug.h"

#pragma segment utils

//==========================================================================
//defines for class Outstrstuffs
//Outstrstuffs fstdout(stdout);

Outstrstuffs::Outstrstuffs(FILE* fp) {
	_indent=0;
	_outfp=fp;
	_ok_to_close=false;
    }
Outstrstuffs::Outstrstuffs(const char* file_out) {
	_indent=0;
    if (file_out) {
       if ((_outfp = fopen(file_out,"w+")),_outfp) {
          printf("can't open output file '%s'\n", file_out);
          }
        }
    else _outfp=stdout;
	_ok_to_close=true;
    }
Outstrstuffs::~Outstrstuffs() {
    fflush(_outfp);
    if (_outfp!=stdout && _ok_to_close)
        fclose(_outfp);
    }
void Outstrstuffs::pad2(int n) {
    while (_col < n) {
        fputc(' ',_outfp);
        _col += 1;
        }
    }
void Outstrstuffs::indent() {
	_indent+=4;
    }
void Outstrstuffs::unindent() {
	_indent-=4;
	DBG_ASSERT(_indent>=0);
    }
char* Outstrstuffs::cvstr(const char *fmt, va_list ap) {
    // this does column counting for formatted output.
    char *t = Strstuffs::vfmt_str(fmt,ap);
    char *p = t;
    while (*p) {
        if (*p == NEW_LINE)
            _col = 0;
        else if (*p == '\t') {
            int oldcol = _col;
            _col = ALIGN(_col,8);
            if (_col == oldcol) _col += 8;
            }
        else _col++;
        p++;
        }
    return t;
    }
void Outstrstuffs::outnl() {
	outstr("\n");
    	#ifdef DEBUG
			fflush(_outfp);
		#endif
	pad2(_indent);
	}
void Outstrstuffs::outstr(const char *fmt, ...) {
	busy_cursor();
    va_list ap;
    va_start(ap,fmt);
    fprintf(_outfp, "%s", cvstr(fmt,ap));
    va_end(ap);
    }
void Outstrstuffs::outstr(int n, const char *fmt, ...) {
    va_list ap;
    va_start(ap,fmt);
    char *s = cvstr(fmt,ap);
    va_end(ap);
    for (int i=0; i < n && s[i]; i++) {
        fputc(s[i],_outfp);
        _col += 1;
        }
    }
void Outstrstuffs::outhex(unsigned short i) { outstr("0x%04X", i); }
void Outstrstuffs::outhex(short i) { outstr("0x%04X", i); }
void Outstrstuffs::outhex(unsigned long i) { outstr("0x%08X", i); }
void Outstrstuffs::outhex(long i) { outstr("0x%08X", i); }
void Outstrstuffs::outhex(int n,unsigned char* m) { 
	int i;
	outstr("0x"); 
	for (i=0; i<n; i++) {
		outstr("%X", m[i]); 
		}
	}
	
//dump hex bytes and ascii text for cnt bytes 
void Outstrstuffs::dumphex(unsigned char *cp, unsigned long cnt, long off) {
    outstr("%6lX: ", off);
    int i;

	//start dumping at an aligned offset
    int base = (off & ~0xf);
    int pad = (off - base);
    int pad_0 = pad, pad_1 = 0, pad_2;
    if (pad > 7) {
        pad_0 = 8;
        pad_1 = pad - 8;
        }
    pad_2 = 16 - (pad + cnt);
    for (i=0; i < pad_0; i++) 
		outstr("   ");

	//print hex bytes
    int n = pad_0;
    for (i = 0; i < cnt; i++) {
        if (n == 8) {	//print middle bar
            outstr("| ");
            pad2(pad_1);
            for (int j=0; j < pad_1; j++) {
                //was: outstr(" ");
				outstr("   ");
                n++;
                }
            }
        outstr("%02X ", cp[i]);	//print hex value
        n++;
        }
    for (i=0; i < pad_2; i++) outstr("   ");
    if (pad_2 > 7) outstr("  ");
    outstr(" ");

	//print ascii text for hex bytes
    for (i=0; i < pad; i++) outstr(" ");
    for (i = 0; i < cnt; i++) {
        outstr("%c", isprint(cp[i])?cp[i]:'.');
        }
    outstr("\n");
    }
    
//dump n bytes at buf in hex
void Outstrstuffs::dumpbytes(int n, unsigned char* buf) {
		int i,j;	
		for (i=0;i<n;i+=32) {
			 for (j=i; j<i+32 && j<n; j++) {	/* print a line */	
			 	if ((j%4)==0) outstr(" ");	
			 	outstr("%02x",(unsigned char)buf[j]);	
			 	}	
			}	
		}

//==========================================================================
//defines for class Outy

Outy::Outy(FILE* dump_fp) { 
	_tabs=0; 
	_spaces[0]=0;
	if (dump_fp) {
		_fp = dump_fp;
		_ok_to_close = false;
		}
	else {
		if ((_fp=fopen("symdump.out","w+"))==NULL) {
			printf("#Unable to open symdump.out\n");
			exit(0);
			}
		_ok_to_close = true;
		}
	}
		
//-------------------------------------------
void Outy::print(char *fmt,...)
//-------------------------------------------
{
	busy_cursor();
	print_indent();
	va_list ap;
	va_start(ap, fmt);
	vfprintf(_fp, fmt, ap);
	va_end(ap);
	fflush(_fp);
	}
void Outy::print_cont(char *fmt,...) {
	busy_cursor();
	va_list ap;
	va_start(ap, fmt);
	vfprintf(_fp, fmt, ap);
	va_end(ap);
	fflush(_fp);
	}
	
