/*  @(#) option.cpp 96/09/09 1.24 */

#include <stdlib.h>
#include <stdio.h>
#include "option.h"
#include "utils.h"
#include "debug.h"

#pragma segment utils
//==========================================================================
// class GetOpts - parse command options

//-------------------------------------------
GetOpts::GetOpts(int ac, char **av, Opts* options)
//-------------------------------------------
{ 
    totalAs = ac; //How many arguments total do we have ?
	create_opts(options);
	create_cmdargs(ac,av);
}
    
//-------------------------------------------
GetOpts::option::~option()
//-------------------------------------------
{
	if (_optarg)
	{
		optarg* current = _optarg;
		while (current)
		{
			optarg* next = current->_chain;
			delete current;
			current = next;
		}
	}
}

//-------------------------------------------
GetOpts::~GetOpts()
//-------------------------------------------
{
	if (_opts)
	{
#if 0
	// The delete [] below does this anyway.
		for (int i=0; i<=_nopts; i++)
		{
			if (_opts[i]._optarg)
				DELETE(_opts[i]._optarg);
		}
#endif
		DELETE_ARRAY(_opts);
	}
	if (_cmdargs)
		FREE(_cmdargs);
}
	
#define LINES_PER_SCREEN 20
void GetOpts::usage(Opts* options,char *pname,FILE* fp) {
	if (!fp) return;
	if (!options) {
    	fprintf(fp,
        	"\nNo options defined for program %s\n", pname?pname:"");
        return;
        }
    fprintf(fp,
        "Usage:\n%s [options] files...\n", pname);
	char *s, *p;
	int nline = 0; 
	char buf[1024];
	for (int i=0; options[i]._ind!=END_OPTS; i++)
	{
		s = options[i]._description; 
		if (s)
			fprintf(fp,"    -%c",options[i]._id);
		//pause if we've filled up the screen
		while (s)
		{
			p = strchr(s,'\n');
			if (p)
			{
				memcpy(buf,s,(p-s+1));
				buf[p-s+1]=0;
				fprintf(fp,buf);
				s = ++p;
				nline++;
#ifndef macintosh
				if ((fp==stdout || fp==stderr) && (nline%LINES_PER_SCREEN)==0) {
					fprintf(fp,"--more--");
					if (getchar()=='q') break;
					//getchar();	//get return
					fprintf(fp,"\r");
					}
#endif
			}
			else
				s = 0;
		}
	}
}
    
void GetOpts::create_opts(Opts* options)
{
	int i,j;
	_options = options;
	_nopts = 0;
	if (!options) return;
    for (i=0; options[i]._ind!=END_OPTS; i++)
    	_nopts++;
	_opts = (option*)NEW(option[_nopts+1]);
	if (!_opts) { 
		DBG_ERR(("outamem! can't create _opts!\n")); 
		return; 
		}
	for (i=0; i<_nopts; i++) {
		j = options[i]._ind;
		if (j<0 || j>=_nopts) { 
			DBG_ERR(("invalid option index %d\n",j)); 
			continue; 
			}
		_opts[j]._id=options[i]._id;	//set char to identify  this opt
		_opts[j]._isset=false;
		_opts[j]._optarg=0;
		_opts[j]._nargs=0;
		}
	_opts[_nopts]._id=USER_OPT;	//last one for user's opts
	_opts[_nopts]._isset=false;
	_opts[_nopts]._optarg=0;
	_opts[_nopts]._nargs=0;
}
	
//-------------------------------------------
void GetOpts::create_cmdargs(int ac,char **av)
//-------------------------------------------
{
    _cmdargs = (char**)MALLOC(sizeof(char*)*(ac+1));
	if (!_cmdargs) { 
		DBG_ERR(("outamem! can't create _cmdargs!\n")); 
		return; 
		}
	if (ac<=1) { 
		DBG_WARN(("no options!\n")); 
		return; 
		}
	int argind=0;
	_av = &av[1];
	_p = &av[1];
	//_av = &av[0];
	//_p = &av[0];
	int i; // ANSI standard requires it to be here, rather than in the "for" - jrm 95/12/20
	for (i=0; argind<ac && _p&&_p[0]&&_p[0][0]; i++) {
		_cmdargs[i] = _p[0];
		//if useropt, assign args
		DBG_(INIT,("argind=0x%X, opt=av[argind][1]=%c, arg=av[argind]=%s\n",argind,av[argind][1],av[argind]));
		int ind;
		if (_p[0][0]!='-') {	//user opt
			ind = _nopts;
			set(ind,rawcmdarg(),rawcmdasn());
			}
		else {
			int minargs = cmdminargs(_p[0][1]);
			ind = optind(_p[0][1]);
			DBG_(INIT,("option ind=%d, _opts[ind]._id=%c, minargs=%d, opt=av[argind][1]=%c\n",ind,_opts[ind]._id,minargs,av[argind][1]));
			set(ind,rawcmdarg(),rawcmdasn());
			if (minargs>0) {
				//if useropt has manditory arg(s), skip args
    			for (int j=nargs(ind); j<minargs; j++) {
					argind++;	//skip args for this opt
					_p++;
					if (!(argind<ac && _p&&_p[0]&&_p[0][0])) {
						break;
						}
					DBG_(INIT,("adding minargs: argind=0x%X, opt=av[argind][1]=%c, arg=av[argind]=%s\n",argind,av[argind][1],av[argind]));
					set(ind,rawcmdarg(),rawcmdasn());
					}
				}
			}
		_p++;
		argind++;		
		}
	_ncmdopts = i;
    _cmdargs[i] = 0;
}

//return index of option id 
#if 0
int GetOpts::optind(unsigned char c)
{
	int i;
	//set up opts
	for (i=0; i<=_nopts; i++)
	{
		if (c==_opts[i]._id)
			return i;
	}
	return 0;
   }
#endif

int GetOpts::optind(unsigned char c)
{
	int i;
	//set up opts
	for (i=0; i<=_nopts; i++)
	{
		if (c==_options[i]._id)
		{	
			int j = _options[i]._ind;
			DBG_ASSERT(_opts[j]._id==c);
			return j;
		}
	}
	return 0;
}
        
//-------------------------------------------
int GetOpts::cmdminargs(unsigned char c)
//return minimum arguments for option id for parsing command line
//-------------------------------------------
{
	int i;
	//set up opts
	for (i=0; i<=_nopts; i++)
	{
		if (c==_options[i]._id)
			return _options[i]._minargs;
	}
	return 0;
}
        
//get user arg/asn value
uint32 GetOpts::argval(int i, int j)
{
	if (i==USER_OPT) i=_nopts;
	DBG_ASSERT(i>=0 && i<=_nopts);
	if (i<0 || i>_nopts) return 0;
	return ch2val(arg(i,j));
	}
uint32 GetOpts::asnval(int i, int j) {
	if (i==USER_OPT) i=_nopts;
	DBG_ASSERT(i>=0 && i<=_nopts);
	if (i<0 || i>_nopts) return 0;
	return ch2val(asn(i,j));
	}
Boolean GetOpts::isset(int i) { 
	if (i==USER_OPT) 
		i=_nopts; 
	return (i>=0&&i<=_nopts) 
		? _opts[i]._isset 
		: false; 
	}
void GetOpts::set(int i) { 
	if (i==USER_OPT) i=_nopts;
	DBG_ASSERT(i>=0 && i<=_nopts);
	if (i>=0 && i<= _nopts)
		_opts[i]._isset=true; 
	}
void GetOpts::clear(int i) { 
	if (i==USER_OPT) i=_nopts;
	DBG_ASSERT(i>=0 && i<=_nopts);
	if (i>=0 && i<= _nopts)
		_opts[i]._isset=false; 
	}
void GetOpts::set(int i, char* arg,char *asn) { 
	DBG_ENT("set");
	if (i==USER_OPT) i=_nopts;
	DBG_ASSERT(_opts);
	DBG_ASSERT(i>=0 && i<= _nopts);
	if (i>=0 && i<= _nopts) {
		_opts[i]._isset=true; 
		DBG_(INIT,("option %c set\n",(char)_opts[i]._id));
		if (arg || asn) {
			DBG_(INIT,("adding option %c arg=%s, asn=%s\n",(char)_opts[i]._id,arg?arg:"-none-",asn?asn:"-none-"));
			if (!NEW(optarg(_opts[i]._optarg,arg,asn))) {
				DBG_ERR(("# Unable to add new option!\n"));
				return;
				}
			if (!_opts[i]._optarg) { 
				DBG_ERR(("outamem! can't create _opts[i]._arg!\n")); 
				return; 
				}
			_opts[i]._nargs++;
			}
		}
	}
char* GetOpts::arg(int i, int j) {
	int k; optarg* o; 
	if (i==USER_OPT) i=_nopts;
	DBG_ASSERT(i>=0 && i<= _nopts);
	if (i>=0 && i<= _nopts) {
		//NOTE: _chain is upsidedown! (it's a stack)
		for (k=nargs(i)-1, o=_opts[i]._optarg; o && k>=0; k--, o=o->_chain) {
			if (k==j) return o->_arg;
			}
		}
	return 0;
	}

char* GetOpts::asn(int i, int j)
{ 
	int k; optarg* o; 
	if (i==USER_OPT) i=_nopts;
	DBG_ASSERT(i>=0 && i<= _nopts);
	if (i>=0 && i<= _nopts) {
		//NOTE: _chain is upsidedown! (it's a stack)
		for (k=nargs(i)-1, o=_opts[i]._optarg; o && k>=0; k--, o=o->_chain) {
			if (k==j) return o->_asn;
			}
		}
	return 0;
}

//-------------------------------------------
unsigned char GetOpts::cmdid()
//iterators (for itterating thru options in order)
//get user opts
//-------------------------------------------
{
    if (!_p || !_p[0]) return 0;
    if (_p[0][0]=='-') 
		return _p[0][1];	//return char if not recognized
	return USER_OPT;
}

//-------------------------------------------
char* GetOpts::rawcmdarg()
//if _p is an opt with minargs, we'll need to skip spaces if necessary
//to get to args
//-------------------------------------------
{
	char* a;
    if (!_p || !_p[0]) return 0;
	unsigned char optid=cmdid(); 
    if (!optid || optid==(unsigned char)UNKNOWN_OPT) return 0;
    if (optid==USER_OPT)
    	a = &(_p[0][0]);
	else
		a = &(_p[0][2]);
	if (!*a && cmdminargs(optid) > 0)
	{
		//need to get next arg
		//SOMEDAY:  need to fix cmdasn so doesn't ++ twice!
		_p++;
    	a = &(_p[0][0]);
	}
	if (!*a) return 0;
	return a;
}

//-------------------------------------------
char* GetOpts::rawcmdasn()
//-------------------------------------------
{
	char *p,*optarg=cmdarg(); 
	if (optarg && (p=strchr((const char*)optarg,'='),p&&*p)) {
		*p=0;
		return p+1;
		}
    return 0;
}
	
//-------------------------------------------
char* GetOpts::cmdarg()
//if _p is an opt with minargs, we'll need to skip spaces if necessary
//to get to args
//-------------------------------------------
{
	_argind=0;
	char* a;
    if (!_p || !_p[0]) return 0;
	unsigned char optid=cmdid(); 
    if (!optid || optid==UNKNOWN_OPT) return 0;
    if (optid==USER_OPT) a = &(_p[0][0]);
	else a = &(_p[0][2]);
	if (!*a && cmdminargs(optid)>0) {	//need to get next arg
		_p++;
    	a = &(_p[0][0]);
		}
	if (!*a) return 0;
	_argind=1;
	return a;
}

//-------------------------------------------
char* GetOpts::nextcmdarg()
//to get to next arg
//-------------------------------------------
{
	char* a;
    if (!_p || !_p[0]) return 0;
    if (!_optid || _optid==UNKNOWN_OPT) return 0;
	if (cmdminargs(_optid)-_argind<=0)
	{
		//need to get next arg
		return 0;
	}
	_p++;
    a = &(_p[0][0]);
	_argind++;
	return a;
}

//-------------------------------------------
char* GetOpts::cmdasn()
//-------------------------------------------
{
	char *p,*optarg=cmdarg(); 
	if (optarg && (p=strchr((const char*)optarg,'='),p&&*p))
	{
		*p=0;
		return p+1;
	}
    return 0;
}
	
//======================================
//iterators (for iterating thru options in order)
//======================================

//-------------------------------------------
void GetOpts::initopts()
//-------------------------------------------
{
    _p = _ncmdopts>0?&_av[0]:0;	//mac doesn't get 1st arg if running as an app
}

//-------------------------------------------
int GetOpts::firstrawopt()
//get first raw user opt
//-------------------------------------------
{
    initopts();
   	return cmdid();
}

//-------------------------------------------
int GetOpts::firstopt()
//get first user opt
//-------------------------------------------
{
   	return testopt(firstrawopt());
}

//-------------------------------------------
int GetOpts::testopt(unsigned char c)
//test user opt
//-------------------------------------------
{
	if (!c) return 0;
	for (int i=0; i<=_nopts; i++)
	{
		if (c==_opts[i]._id)
			return c;
	}
   	return UNKNOWN_OPT;
}

//-------------------------------------------
int GetOpts::nextrawopt()
//get next user opt id
//-------------------------------------------
{
    if (!_p || !_p[0]) return 0;
   	_p++;
   	return cmdid();
}

//-------------------------------------------
int GetOpts::nextopt()
//get next user opt
//-------------------------------------------
{
	cmdarg();	//make sure we get past any user args
   	return testopt(nextrawopt());
}
