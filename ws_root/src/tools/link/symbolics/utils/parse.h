/*  @(#) parse.h 96/07/25 1.11 */

#ifndef __PARSE_H__
#define __PARSE_H__

#ifndef USE_DUMP_FILE     
#include "utils.h"
#endif                  

#ifndef macintosh    
	#define TAB_SIZE 4
#else
	#define TAB_SIZE 8
#endif

//======================================
class Parse
//======================================
{
	const char* _buf;
	int32 _sz;
	const char* _file;
	int _line;
	int _col;
protected:
	Boolean advance_buf(int i);
	int _next_token_idx;	//index to next token
public:
	enum Token {
		tkn_unknown,
		tkn_number,
		tkn_symbol,
		tkn_equals,			// =
		tkn_plus,			// +
		tkn_dash,			// -
		tkn_asterisk,		// *
		tkn_fwdslash,		// /
		tkn_bwdslash,		// \ 
		tkn_openpren,		// (
		tkn_closepren,		// )
		tkn_openbracket,	// [
		tkn_closebracket,	// ]
		tkn_openbrace,		// {
		tkn_closebrace,		// }
		tkn_quote,			// '
		tkn_backquote,		// `
		tkn_dblquote,		// "
		tkn_undscore,		// _
		tkn_comma,			// ,
		tkn_period,			// .
		tkn_colon,			// :
		tkn_semicolon,		// ;
		tkn_at,				// @
		tkn_and,			// &
		tkn_bar,			// |
		tkn_exclaimation,	// !
		tkn_question,		// ?
		tkn_pound,			// #
		tkn_dolar,			// $
		tkn_percent,		// %
		tkn_carrot,			// ^
		tkn_eof				// EOF
		};
	int32 _num;
	char* _str;
	char* _token_buf;
	Token _token;

	Parse(const char* buf, int32  sz, const char* file=0);
	static Boolean iswhite(char x) { return (Boolean) (x==' '||x=='\0'||x==NEW_LINE||x=='\t'); }
	static Boolean ischar(char x) { return (Boolean) (x>='a'&&x<='z'||x>='A'&&x<='Z'); }
	static Boolean isnum(char x) { return (Boolean) (x>='0'&&x<='9'); }
	Boolean isspecial(char x) { return (Boolean) (x=='_'); }	
	Boolean more() { return (Boolean) (_sz>0); }
	const char* file() { return _file; }
	int line() { return _line; }
	int col() { return _col; }
	void skip_white();
	void skip_line();
	Token next_token();
	Token get_token();
}; // class Parse
	
#endif /* __PARSE_H__ */
