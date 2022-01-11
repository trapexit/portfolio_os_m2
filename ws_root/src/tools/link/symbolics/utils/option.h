/*  @(#) option.h 96/09/09 1.16 */

#ifndef __OPTION_H__
#define __OPTION_H__

#define END_OPTS 0xff
#define UNKNOWN_OPT 0xfe
#define USER_OPT END_OPTS

#ifndef USE_DUMP_FILE
#include <stdio.h>
#include "utils.h"
#endif

#include "loaderty.h"

struct Opts {
	unsigned char _ind;
	unsigned char _id;
	int _minargs;
    char* _description;
    };
	
class GetOpts {
	struct optarg {
		optarg* _chain;
		char* _arg;	//arg list for opt
		char* _asn;	//asn list for arg list
		optarg(optarg*& chain, char* arg, char* asn) {
			_chain = chain;
			chain = this;
			_arg = arg;
			_asn = asn;
			}
		};
	struct option {
		Boolean _isset;	
		int _nargs;
		unsigned char _id;		//char to identify opt
		optarg* _optarg;		//linked arg list for opt
		option() { 
			_isset=false;
			_nargs=0;
			_id=0;
			_optarg=0;
			};
		~option();
		};
	int _argind;
	unsigned char _optid;
		
    char **_cmdargs;
    char **_p;
    char **_av;
    option* _opts;
	Opts* _options;
	int totalAs; //just to keep track of how many arguments
    int _nopts;
    int _ncmdopts;
	void sort_args(char **av);
	void create_opts(Opts* options);
	int optind(unsigned char c);
	int cmdminargs(unsigned char c);
	void create_cmdargs(int ac,char **av);
    char* rawcmdarg();	//get user arg 
    char* rawcmdasn();	//get user asn 
public:
    GetOpts(int ac, char **av, Opts* options);
	~GetOpts();
	static void usage(Opts* options,char *pname,FILE* fp);
    //for parsing thru the user's opts
	int firstrawopt();		//get first raw user opt
	int firstopt();			//get first user opt
	void initopts();	//set initial opt pointer
    int nextrawopt();	//get next raw user opt
    int nextopt();		//get next user opt
    //char* optarg();	//get user arg for opt
    int totalArgs(){ return totalAs;}    //Just to find out the number of opts mmh 96/08/16
    unsigned char cmdid();		//get user opt id 
    char* cmdarg();	//get user arg 
    char* cmdasn();	//get user asn 
    char* nextcmdarg();		//get next user arg for current opt
	//for querying the opts & args later
    int ncmdopts() { return _ncmdopts; }
    int nargs(int i) { if (i==USER_OPT) i=_nopts; return (i>=0&&i<=_nopts) ? _opts[i]._nargs : 0; }
    unsigned char id(int i) { if (i==USER_OPT) i=_nopts; return (i>=0&&i<=_nopts) ? _opts[i]._id : 0; };
    char* arg(int i, int j=0);
    char* asn(int i, int j=0);
    uint32 argval(int i, int j=0);
    uint32 asnval(int i, int j=0);
    Boolean isset(int i);
	void set(int i);
	void clear(int i);
	void set(int i, char* arg,char *asn=0);
	int testopt(unsigned char c);
    };

#endif /* __OPTION_H__ */
