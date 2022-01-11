//  @(#) debug.h 96/03/12 1.15
//=======================================================================================
// debug.h - macros for generating formatted debug output based on debug level
//
//	DBG_SET_LEVEL(lvl) 	-	set debug level to DBG_LVL_<lvl> where lvl is:
//							(the DBG_LVL_ prefix is assumed to reduce wordiness and prevent namespace conflicts)
//	 	NONE	0	- no debugging
// 		ERR		1	- errors
// 		ASSERT	2	- errors, failed assertions
// 		WARN	3	- errors, failed assertions, warnings
// 		CALL	4	- errors, failed assertions, warnings, entrance to functions
// 		TRACE	5	- errors, failed assertions, warnings, entrance to functions, exits from functions
// 		<lvl>	?	- user defined level & levels below 
//		ALL	  100	- all debugging output
//
// 	DBG_INIT(file) 		- 	set debug output file
// 	DBG_TERM()			- 	close debug output file
// 	DBG_ENT(func) 		- 	enter function and increase tab
//	DBG(msg)			-	print msg (indented) if debug level >= DBG_LVL_DEFAULT
//	DBG_(lvl,msg)		-	customised version of DBG; print msg (indented) if debug level >= DBG_LVL_<lvl>
//							(the DBG_LVL_ prefix is assumed to reduce wordiness and prevent namespace conflicts)
//							eg. for DBG_LVL_ME call DBG_(ME,("my msg\n"))
//	DBG_ASSERT(expr) 	- 	if expr is false, prints error message
// 	DBG_ERR(errmsg)		- 	prints errmsg
// 	DBG_WARN(warnmsg)	-	prints warnmsg
//
// NOTE: this file can be included more than once to toggle debug output:
// 		#include "debug.h"	// initial include
//			...
//		(no debug output)
//			...
// 		#define DEBUG
// 		#include "debug.h"	// include again to redefine debug macros
//			...
//		(debug output)
//			...
// 		#undef DEBUG
// 		#include "debug.h"	// include again to redefine debug macros
//			...
//		(no debug output)
//=======================================================================================

//=======================================================================================
// debug levels - add new levels here
//=======================================================================================
#ifndef DEBUG_DCLS
//debug_levels:		(call DBG_SET_LVL(<lvl>) to set; the DBG_LVL_ prefix is automatically concatinated to prevent namespace conflicts)
#define DBG_LVL_NONE		0	// no debugging
#define DBG_LVL_ASSERT		1	// failed assertions
#define DBG_LVL_ERR			2	// JRM 96/02/06: was 2.  failed assumptions, errors
#define DBG_LVL_WARN		3	// errors, failed assertions, warnings
#define DBG_LVL_FIXME 		4	// fixme output & levels below
#define DBG_LVL_CALL		5	// errors, failed assertions, warnings, entrance to functions
#define DBG_LVL_TRACE		6	// errors, failed assertions, warnings, entrance to functions, exits from functions
#define DBG_LVL_DEFAULT	   50	// generic debugging output
#define DBG_LVL_ALL		  100	// all debugging output
//new levels:
#define DBG_LVL_IMP   	20	// important output & levels below
#define DBG_LVL_INIT  	30	// informational output & levels below
#define DBG_LVL_INFO  	55	// informational output & levels below
#define DBG_LVL_PICKY 	90	// picky details output & levels below
#define DBG_LVL_SYMS  	70	// symbols output & levels below
#define DBG_LVL_SYMAPI	80	// symbolics API output & levels below
#define DBG_LVL_SYMNET	80	// symbolics network output & levels below
#define DBG_LVL_ARMSYM	85	// ARM symbolics output & levels below
#define DBG_LVL_XCOFF 	85	// XCOFF symbolics output & levels below
#define DBG_LVL_XSYM  	85	// XSYM symbolics output & levels below
#define DBG_LVL_ELF  	85	// XSYM symbolics output & levels below
#define DBG_LVL_ALLSYMS 86	// all symbolics output & levels below
#define DBG_LVL_BRKS  	60	// breakpoints output & levels below
#define DBG_LVL_EXPR   	60	// expression parsing output & levels below
#define DBG_LVL_LOAD	60	// loading output & levels below
#define DBG_LVL_WIN    	60	// windows output & levels below
#endif /* DEBUG_DCLS */

//=======================================================================================
//undefs for reincluding debug.h in the middle of a file
//useful for turning off & on debugging within a file 
#ifdef DEBUG_H
#undef DBG_SET_LEVEL
#undef DBG_IS_ACTIVE
#undef DEBUG_H
#undef DBG_ENT
#undef DBG_INIT
#undef DBG_TERM
#undef DBG_ASSERT
#undef DBG_WARN
#undef DBG_FIXME
#undef DBG_ERR
#undef DBG_DUMP
#undef DBG_DUMP_
#undef DBG
#undef DBG_
#undef DBG_OUT
#undef DBG_OUT_
#endif /* DEBUG_H */

#ifndef DEBUG_H
#define DEBUG_H 1

#ifdef DEBUG
//=======================================================================================
// debug.h definitions and declarations
//=======================================================================================
#ifndef DEBUG_DCLS
//these things should only get included once no matter how many times debug.h is included...
#include <stdio.h>
#include <stdarg.h>
#ifdef _MAC
	#include <Types.h>
#endif

extern int _debug_level;	
extern FILE *_debug_fp;
extern Boolean _debug_active;
extern void _debug_printf(char *fmt,...);
extern void _debug_dump(int n, unsigned char* buf);
extern char _debug_spaces[1024];	/* array for spacing using tabs */
extern int _debug_call_level;	/* tab count */
extern _debug_out(char* msg, char* x,char* file,int line);
inline void _debug_indent() { if (_debug_fp) fprintf(_debug_fp,"%s",_debug_spaces); }
inline void set_debug_level(int level) { _debug_level = level; }
#define MAX_NAME 31

class _debug {
	//char* _debug_func;
	char _debug_func[MAX_NAME+1];
public:
	_debug(char* f,char* file,int line);
	~_debug();
	};
#endif /* DEBUG_DCLS */

#ifdef DEBUG_DEFS
#ifndef DEBUG_DEF_DCLS
#define DEBUG_DEF_DCLS 1
	int _debug_level=DBG_LVL_ALL;	
	FILE *_debug_fp=stdout;
	Boolean _debug_active=false;
	char _debug_spaces[1024]="";	/* array for spacing using tabs */
	int _debug_call_level=0;	/* tab count */
	void _debug_printf(char *fmt,...) {
		if (!_debug_fp) return;
		va_list ap;
		va_start(ap, fmt);
		vfprintf(_debug_fp, fmt, ap);
		va_end(ap);
		fflush(_debug_fp);
		}
		
	void _debug_dump(int n, unsigned char* buf) {
		int i,j;	
		if (!_debug_fp) return;
		for (i=0;i<n;i+=32) {
			_debug_indent();	
			 for (j=i; j<i+32 && j<n; j++) {	/* print a line */	
			 	if ((j%4)==0) _debug_printf("  ");	
			 	_debug_printf("%02x",(unsigned char)buf[j]);	
			 	}	
			_debug_printf("\n");	
			}	
		fflush(_debug_fp);	
		}
				
	_debug::_debug(char* f,char* file,int line) { 
		//_debug_func=NEW(char[strlen(f)]);
		//strcpy(_debug_func,f);
		int n = min(strlen(f),MAX_NAME);
		strncpy(_debug_func,f,n);
		_debug_func[n]=0;
		if (_debug_fp && _debug_level>=DBG_LVL_CALL) {	
			_debug_indent();
			fprintf(_debug_fp,"==>ENTER(%s:%d:%s){\n",file,line,_debug_func);	
			fflush(_debug_fp);	
			}
		_debug_spaces[_debug_call_level]='\t';	/* increment tab count */ 
		_debug_spaces[++_debug_call_level]='\0';	
		}
	_debug::~_debug() {
		if (_debug_fp && _debug_level>=DBG_LVL_TRACE) {	
			_debug_indent();	
			fprintf(_debug_fp,"<==EXIT(%s)}\n",_debug_func);	
			fflush(_debug_fp);	
			}	
		//DELETE(_debug_func);
		_debug_spaces[--_debug_call_level]='\0';	/* decrement tab count */ 
		}

#endif /* DEBUG_DEF_DCLS */
#endif /* DEBUG_DEFS */


#ifndef DEBUG_DCLS
#define DEBUG_DCLS 1

#define _DBG_OUT(msg,x,file,line) 	\
	if (_debug_fp) {	\
		fprintf(_debug_fp,"%s(%s:%d):::",msg,file,line); 	\
		_debug_printf x;	\
		fflush(_debug_fp);	\
		}
				
#define _DBG_STR(x) #x

#endif /* DEBUG_DCLS */


//=======================================================================================
// external macros for generating debug output
//=======================================================================================
#define DBG_SET_LEVEL(lvl) set_debug_level(DBG_LVL_##lvl);
#define DBG_IS_ACTIVE _debug_active

//if env var DEBUG_LEVEL is not set,
//still do debug checks, but don't write any output
//else set the debug level to the environment variable value
#define DBG_INIT(x) { \
	if (getenvnum("DEBUG_LEVEL")) {	\
		_debug_level = getenvnum("DEBUG_LEVEL");	\
		_debug_fp = 0;	\
		_debug_active=true;	\
		}	\
	else if ((_debug_fp=fopen(x,"w+"))==NULL) { \
		printf("error opening trace file %s.\n",x);	\
		_debug_fp=stdout; 	\
		_debug_active=false;	\
		}	\
	else _debug_active=true;	\
	}
	
#define DBG_TERM() { \
	if (_debug_fp && _debug_fp!=stdout)  { fclose(_debug_fp); _debug_fp=0; }	\
	_debug_active=false;	\
	_debug_level=0;		\
	}
	
#define DBG_ENT(x) \
	_debug _dbg(x,__FILE__,__LINE__);	
	
//macro for detecting null pointers
#define DBG_ASSERT(x)  \
	if (!(x) && _debug_level>=DBG_LVL_ASSERT) { \
		_DBG_OUT("ASSERTION FAILED!! ",("(%s)\n",_DBG_STR(x)),__FILE__,__LINE__);	\
		printf("ASSERTION FAILED!! --(%s) at %s#%d--",_DBG_STR(x),__FILE__,__LINE__);	\
		printf("abort?? (y/n) ");	\
		if (getchar()!='n') {	\
			exit(0);	\
			}	\
		}
		
//note: indentation requires that _dbg be constructed; must call DBG_ENT first
//	DBG_(lvl,msg) - customised version of DBG; print msg (indented) if debug level >= DBG_LVL_<lvl> 
#define DBG(x)  DBG_(DEFAULT,x)
#define DBG_(lvl,x)  \
	if (_debug_level>=DBG_LVL_##lvl)	{ \
		_debug_indent();	\
		_DBG_OUT("",x,__FILE__,__LINE__);	\
		}	
#define DBG_OUT(x)  DBG_OUT_(DEFAULT,x)
#define DBG_OUT_(lvl,x)  \
	if (_debug_level>=DBG_LVL_##lvl)	{ \
		_debug_printf x;	\
		}	
		
#define DBG_ERR(x)  \
	if (_debug_level>=DBG_LVL_ERR)	 \
		_DBG_OUT("# ERROR. ",x,__FILE__,__LINE__);	
#define DBG_WARN(x)  \
	if (_debug_level>=DBG_LVL_WARN)	 \
		_DBG_OUT("# WARNING. ",x,__FILE__,__LINE__);

#define DBG_FIXME(x)  \
	if (_debug_level>=DBG_LVL_FIXME)	 \
		_DBG_OUT("# FIXME. ",x,__FILE__,__LINE__);

#define DBG_DUMP(n,buf)	DBG_DUMP_(DEFAULT,n,buf)
#define DBG_DUMP_(lvl,n,buf)	\
	if (_debug_level>=DBG_LVL_##lvl)	{ \
		_debug_indent();	\
		_DBG_OUT("",("dumping %d bytes at x%04x:\n",n,buf),__FILE__,__LINE__);	\
		_debug_dump(n,buf);	\
		}

#else
#define DBG_SET_LEVEL(x)
#define DBG_IS_ACTIVE
#define DBG_INIT(x) 
#define DBG_TERM()
#define DBG_ENT(x) 
#define DBG(x)
#define DBG_(lvl,x)
#define DBG_ASSERT(x) 
#define DBG_ERR(x) printf x ;
#define DBG_WARN(x)
#define DBG_FIXME(x)
#define DBG_DUMP(n,x)
#define DBG_DUMP_(lvl,n,x)
#define DBG_OUT(x)
#define DBG_OUT_(lvl,x) printf x ;
#endif /* DEBUG */

#endif /* DEBUG_H */
