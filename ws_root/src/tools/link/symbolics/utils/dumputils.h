/*  @(#) dumputils.h 96/07/25 1.9 */

//====================================================================
// dumputils.h  -  utilities for dumping 
//

#ifndef __DUMPUTILS_H__
#define __DUMPUTILS_H__

#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "predefines.h"
#include "loaderty.h"

    
//==========================================================================
//defines for class Outstrstuffs
#ifdef _MW_BUG_	//crazy!  Strstuffs is private, yet MW thinks it's accessable when inheritted (complains about ambiguous funcs)
class Outstrstuffs {
#else
class Outstrstuffs : Strstuffs {
#endif
private:
    FILE *_outfp;
    int _col;
	int _indent;
	Boolean _ok_to_close;
public:
    Outstrstuffs(const char* out_file=0);
    Outstrstuffs(FILE* dump_fp);
    ~Outstrstuffs();
    void outhex(int,unsigned char*);
    void outhex(unsigned short);
    void outdec(unsigned short);
    void outhex(short);
    void outdec(short);
    void outhex(unsigned long);
    void outdec(unsigned long);
    void outhex(long);
    void outdec(long);
    void outstr(const char *, ...);
    void outstr(int, const char *, ...);
    char* cvstr(const char *, va_list); //col counts for output
    void pad2(int); //pad output
	void dumphex(unsigned char *cp, unsigned long cnt, long off);
	void dumpbytes(int n, unsigned char* buf);
	void indent();
	void unindent();
	void outnl();
    };

//==========================================================================
//defines for class Outy

class Outy {
	FILE* _fp;
	char _spaces[1024];	/* array for spacing using tabs */
	int _tabs;	/* tab count */
	Boolean _ok_to_close;
public:
	Outy(FILE* dump_fp=0);
	~Outy() { if (_fp && _ok_to_close) fclose(_fp); }
	void print_indent() { fprintf(_fp,"%s",_spaces); }
	void indent() { 
		if (_tabs>1024) { return; }
		_spaces[_tabs++]='\t';	/* increment tab count */ 
		_spaces[_tabs]='\0';	
		}
	void unindent() { 
		if (_tabs<=0) { _tabs=0; return; }
		_spaces[--_tabs]='\0';	/* decrement tab count */ 
		}
	void print(char *fmt,...);
	void print_cont(char *fmt,...);
	};
	
#endif /* __DUMPUTILS_H__ */
