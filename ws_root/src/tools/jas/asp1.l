%{
/* @(#) asp1.l 95/03/24 1.4 */

#include <string.h>
#include <ctype.h>

#define MAX_M4_ARGS 9 /* m4 has a limit of 9 on macro arguments. */
static char ma[MAX_M4_ARGS][128];
static int nma;
static char mn[512][128];
static int nmn;
static struct {
	char name[128 - sizeof(int)];
	int number;
    } ll[512];
static int nll, llc;
static int in_mb, in_mi= -1;
static int lineno= 1;
static char currentfile[256];

static void asp1_error(char const* s);
static void bump_lineno(void);

/* local label and macro label format strings */
char const*const l_label_f= ".L.%d.L%s";
char const*const m_label_f= ".L.%s__ii__.M%s";
%}

ws		[ \t]+
ident		[_]*[A-Za-z][0-9A-Za-z_]*
lchar		\\.
comment		\;[^\n]*
preproc		#[^\n]*
qchar		\'([^\\]|\\.)\'
sqstring	\'([^\n\']|(\\\'))*[\'\n]
qstring		\"([^\n\"]|(\\\"))*[\"\n]

%s MACRO MN MB MI

%%
^#[^\n]*\n	{
		    ECHO;
		    strtok(yytext, " ");
		    lineno= atoi(strtok(0, " "));
		    strcpy(currentfile, strtok(0, " "));
		}
{qstring}	|
{qchar}		|
{lchar}		ECHO;

^({ws})?\.macro	BEGIN MACRO;
<MACRO>{ident}	{
		    nma= 0;
		    strcpy(mn[nmn++], yytext);
		    fprintf(yyout, "define(%s,`", yytext);
		    BEGIN MN;
		}
<MN>{ws}	|
<MN>","		;
<MN>"&"{ident}	{
		    if(nma == MAX_M4_ARGS) asp1_error("too many macro args");
		    else strcpy(ma[nma++], yytext);
		}
<MN>\n		{
		    fprintf(yyout, " define(%s__ii__,__ii__)\n", mn[nmn-1]);
		    BEGIN MB;
		    in_mb= 1;
		    bump_lineno();
		}
<MI>"%%"	|
<MB>"%%"	;
<MB>"&"{ident}	|
<MI>"&"{ident}	{
		    int i;

		    for(i= 0; i < nma; i++) if(!strcmp(ma[i], yytext))
		    {
			fprintf(yyout, "$%d", i + 1);
			break;
		    }
		    if(i == nma)
		    {
			asp1_error("undefined parameter:");
			asp1_error(yytext);
		    }
		}
<MB>^"%"{ident}:	fprintf(yyout, m_label_f, mn[nmn - 1], yytext + 1);
<MB>"%"{ident}	{
		/* Don't use __ii__ since calling another macro will
		 * increment __ii__ and cause the label to mismatch.
		 * e.g.
			macro
			A ...
			endm
			macro
			B ...
			bl	%exit		<-- __ii__ here
			A	; call A
		   %exit			<-- is different from __ii__
						    here after call to A.
			endm
		 */
		fprintf(yyout, m_label_f, mn[nmn-1], yytext + 1);
		}
<MB>\.endm	{
		    fprintf(yyout, "undefine(`%s__ii__')", mn[nmn-1]);
		    fputs(" define(`__ii__',incr(__ii__))')", yyout);
		    BEGIN INITIAL;
		    in_mb= 0;
		}

<MI>{ws}	;
<MI>\n		{
		    fputs(")\n", yyout);
		    if(in_mb) BEGIN MB;
		    else BEGIN INITIAL;
		    in_mi= -1;
		    bump_lineno();
		}

"%"{ident}	{
		    /* Munge local labels to prevent macro/label name clash. */
		    /* E.g.:  %stop when there's a stop macro. */
		    int i;

		    if(in_mi > -1)
		    {
			fprintf(yyout, m_label_f, mn[in_mi], yytext + 1);
			break;
		    }

		    /* Determine if this local label has been defined. */
		    for(i= 0; i < nll; i++) if(!strcmp(ll[i].name, yytext))
		    {
			/* Found it.  Don't enter it. */
			break;
		    }
		    if(i == nll)
		    {
			/* Didn't find it.  Enter it. */
			strcpy(ll[i].name, yytext);
			ll[i].number= llc++;
			nll++;
		    }
		    fprintf(yyout, l_label_f, ll[i].number, ll[i].name + 1);
		}
^"."{ident}	|
^{ident}	{
		    /* Assume a macro invocation won't appear in column */
		    /* zero.  Since I encountered a non-local label, */
		    /* reset the local label table. */
		    ECHO;
		    nll= 0;
		}
{ident}		{
		    int i;

		    for(i= 0; i < nmn; i++) if(!strcmp(mn[i], yytext))
		    {
			ECHO;
			putc('(', yyout);
			in_mi= i;
			BEGIN MI;
			break;
		    }
		    if(i == nmn) ECHO;
		}
\n		{
		    ECHO;
		    bump_lineno();
		}

%%

static int rv;

main()
{
    fputs("define(__ii__,1)", yyout);
    return yylex() || rv;
}

static void asp1_error(char const* s)
{
    fflush(yyout);
    fprintf(stderr, "asp1: Error: %s in %s line %d\n", s, currentfile, lineno);
    rv= 1;
}

static void bump_lineno(void)
{
    lineno++;
    if(!(lineno % 10)) fprintf(yyout, "# %d %s\n", lineno, currentfile);
}
