%{
/* @(#) asp2.l 95/09/01 1.3 */

#include <string.h>
#include <ctype.h>
#include "asp2y.h"

int lineno= 1;
int no_double;
%}

ws		[ \t]+
dotident	[\._]*[A-Za-z][0-9\.A-Za-z_]*(@)?
ident		[_]*[A-Za-z][0-9A-Za-z_]*
hex		0x[0-9A-Fa-f]+
number		[0-9]+
lchar		\\.
comment		\;[^\n]*
preproc		#[^\n]*
qchar		\'([^\\]|\\.)\'
qstring		\"([^\n\"]|(\\\"))*[\"\n]
reg		[FfRr][0-9]+

%a 8000
%o 8000
%p 8000
%s RSS

%%
{qstring}	|
{qchar}		|
{lchar}		ECHO;
^#[^\n]*\n	{ ECHO; lineno++; }

fadd/[^s]	|
fdiv/[^s]	|
fmadd/[^s]	|
fmsub/[^s]	|
fmul/[^s]	|
fnmadd/[^s]	|
fnmsub/[^s]	|
fsub/[^s]	{
		    if(no_double)
		    {
			fprintf(stderr, "warning: double-precision"
				" instruction `%s' changed"
				" to `%ss'\n", yytext, yytext);
			ECHO;
			putc('s', yyout);
		    }
		    else ECHO;
		}

\.struct	{ BEGIN RSS; return RECORD; }
<RSS>\.ends	{ BEGIN INITIAL; return ENDR; }
<RSS>\.byte	{ yylval.number= 1; return DS; }
<RSS>\.long	{ yylval.number= 4; return DS; }
<RSS>\.word	{ yylval.number= 2; return DS; }
<RSS>". ="	return ORG;
<RSS>{ident}	{ strcpy(yylval.string, yytext); return IDENT; }
<RSS>{hex}	{ yylval.number= strtol(yytext, 0, 0); return NUMBER; }
<RSS>{number}	{ yylval.number= atoi(yytext); return NUMBER; }
<RSS>{ws}	;
<RSS>\n		lineno++;
<RSS>.		return *yytext;

\.end		;

srr0		|
SRR0		fputs("26", yyout);
srr1		|
SRR1		fputs("27", yyout);
SP		|
sp		fputs("r1", yyout);
{dotident}	ECHO;
\n		{ putc('\n', yyout); lineno++; }
%%
