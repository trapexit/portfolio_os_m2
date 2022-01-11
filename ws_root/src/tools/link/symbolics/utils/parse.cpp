/*  @(#) parse.cpp 96/07/25 1.13 */

#include "parse.h"

#pragma segment utils
//==========================================================================
// class Parse - parse ascii buffer

Parse::Parse(const char* buf, int32  sz, const char* file) { 
	_buf = buf; _sz = sz; _file=file; _line=1; _col=0; _next_token_idx=0; 
	}
	
Boolean Parse::advance_buf(int i) {
		_buf+=i;
		_sz-=i;
		if (_sz<=0) return false;
		return true;
		}
		
Parse::Token Parse::get_token() {
	Token tkn=next_token();
	advance_buf(_next_token_idx);
	return tkn;
	}

Parse::Token Parse::next_token() {
	int i=0;
	_num=0;
	_str=0;
	_token_buf=0;
	_token = tkn_unknown;
	
	if (!more()) return tkn_eof;
	skip_white();
	if (!more()) return tkn_eof;
	//parse _buf
	if (isnum(_buf[i])) { 
		while (isnum(_buf[i]) && i < _sz) { i++; _col++; }
		_token_buf = Strstuffs::str(i,_buf);
		_num = strtoul(_token_buf,0,10);
		_token = tkn_number;
		}
	else if (ischar(_buf[i]) || isspecial(_buf[i])) { 
		while ((ischar(_buf[i])||isnum(_buf[i])||isspecial(_buf[i])) && i < _sz) { i++; _col++; }
		_token_buf = _str = Strstuffs::str(i,_buf);
		_token =  tkn_symbol;
		}
	else {
		//if it wasn't a symbol or a number, what is it?
		switch (_buf[i]) {
			case '=':	i=1; _token = tkn_equals;			break;
			case '+':	i=1; _token = tkn_plus;			break;
			case '-':	i=1; _token = tkn_dash;			break;
			case '*':	i=1; _token = tkn_asterisk;		break;
			case '/':	i=1; _token = tkn_fwdslash;		break;
			case '\\':	i=1; _token = tkn_bwdslash;		break;
			case '(':	i=1; _token = tkn_openpren;		break;
			case ')':	i=1; _token = tkn_closepren;		break;
			case '[':	i=1; _token = tkn_openbracket;		break;
			case ']':	i=1; _token = tkn_closebracket;	break;
			case '{':	i=1; _token = tkn_openbrace;		break;
			case '}':	i=1; _token = tkn_closebrace;		break;
			case '\'':	i=1; _token = tkn_quote;			break;
			case '`':	i=1; _token = tkn_backquote;		break;
			case '\"':	i=1; _token = tkn_dblquote;		break;
			case '_':	i=1; _token = tkn_undscore;		break;
			case ',':	i=1; _token = tkn_comma;			break;
			case '.':	i=1; _token = tkn_period;			break;
			case ':':	i=1; _token = tkn_colon;			break;
			case ';':	i=1; _token = tkn_semicolon;		break;
			case '@':	i=1; _token = tkn_at;				break;
			case '&':	i=1; _token = tkn_and;				break;
			case '|':	i=1; _token = tkn_bar;				break;
			case '!':	i=1; _token = tkn_exclaimation;	break;
			case '?':	i=1; _token = tkn_question;		break;
			case '#':	i=1; _token = tkn_pound;			break;
			case '$':	i=1; _token = tkn_dolar;			break;
			case '%':	i=1; _token = tkn_percent;			break;
			case '^':	i=1; _token = tkn_carrot;			break;
			case (char) EOF:	i=1; _token = tkn_eof;		break;
			default:
				while (!iswhite(_buf[i]) && i < _sz) { i++; _col++; }
				if (i >= _sz) _token = tkn_eof;
				else _token = tkn_unknown;
			}
		}
	_token_buf = Strstuffs::str(i,_buf);
	_next_token_idx = i;
	return _token;
	}
	
void Parse::skip_white()
{
	while (iswhite(*_buf) && more())
	{
		if (*_buf==NEW_LINE)
		{
			_line++;
			_col=0;
		}
		if (*_buf=='\t')
			_col += TAB_SIZE-1;
		advance_buf(1);
		_col++;
	}
}
	
void Parse::skip_line()
{
	while (*_buf != NEW_LINE && more())
		advance_buf(1);
	if (*_buf == NEW_LINE)
	{
		advance_buf(1);
		_line++;
		_col=0;
	}
	_token_buf = 0;
}
