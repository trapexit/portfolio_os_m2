%{

/*
 * DDF compiler.
 * @(#) ddfc.y 96/04/29 1.13
 */

#include "ddfc.h"
#define	MAXNAME		32	/* Max characters in a word */
#define	MAXCHUNKSIZE	1024	/* Max size of an IFF chunk */

#define	new(type)	(type *) malloc(sizeof(type))

static int errors = 0;

void OutputFileHeader(void);
void OutputFileTrailer(void);
void OutputDriverName(char *, char *, char *);
void OutputNeedHeader(void);
void OutputNeedTrailer(void);
void OutputProvideHeader(void);
void OutputProvideTrailer(void);
void OutputIcon(char *);
void OutputString(char *);
void OutputInt(signed long);
void OutputKeyword(unsigned int);
void OutputOp(unsigned int);

%}

%union {
	signed long num;
	char *str;
	unsigned long op;
}

%token		DRIVER END ICON MODULE NEEDS OR PROVIDES USES VERSION
%token <str>	NAME
%token <num>	INTEGER

%left '|'
%left '^'
%left '&'
%left SHL SHR
%left '+' '-'
%left '*' '/'
%right '!' '~'
%nonassoc UMINUS

%type  <op>	op
%type  <num>	expr
%type  <str>	module uses


%%

descfile:	{ OutputFileHeader(); }
		VERSION version desclist
		{
			OutputFileTrailer();
		}
	;

version:	INTEGER '.' INTEGER subversion
		{
			OutputVersion($1, $3);
		}
	;

subversion:	/* nothing */
	|	'.' INTEGER	/* sub-versions are ignored. */
	;

desclist:	desc
	|	desclist desc
	;

desc:		DRIVER NAME module uses
		{
			OutputDriverName($2,$3,$4);
		}
		sectionlist
		END DRIVER
	;

module:		/* nothing */	{ $$ = ""; }
	|	MODULE NAME	{ $$ = $2; }
	;

uses:		/* nothing */	{ $$ = ""; }
	|	USES NAME	{ $$ = $2; }
	;

sectionlist:	section
	|	sectionlist section
	;

section:	needsection
	|	providesection
	|	iconsection
	;

providesection:	PROVIDES
		{
			OutputProvideHeader();
		}
		provlist END PROVIDES
		{
			OutputProvideTrailer();
		}

needsection:	NEEDS
		{
			OutputNeedHeader();
		}
		orlist END NEEDS
		{
			OutputNeedTrailer();
		}

iconsection:	ICON NAME
		{
			OutputIcon($2);
		}

provlist:	prov
	|	provlist prov
	;

prov:		NAME
		{
			OutputString($1);
		}
		':' valuelist
		{
			OutputKeyword(K_END_PROVIDE);
		}
	;

orlist:		needlist
	|	orlist OR { OutputKeyword(K_OR); } needlist
	;

needlist:	need
	|	needlist need
	;

need:		NAME
		{
			OutputString($1);
		}
		op
		{
			OutputOp($3);
		}
		valuelist
		{
			OutputKeyword(K_END_NEED);
		}
	;

valuelist:	ivaluelist
	|	svaluelist
	;

svaluelist:	svalue
	|	svaluelist ',' svalue
	;

ivaluelist:	ivalue
	|	ivaluelist ',' ivalue
	;

svalue:		NAME
		{
			OutputString($1);
		}
	|	'*'
		{
			OutputString("*");
		}
	;

ivalue:		expr
		{
			OutputInt($1);
		}
	;

expr:		INTEGER		{ $$= $1; }
	|	'(' expr ')'	{ $$= $2; }
	|	'-' expr %prec UMINUS { $$ = - $2; }
	|	'+' expr %prec UMINUS { $$ = $2; }
	|	'!' expr	{ $$ = ! $2; }
	|	'~' expr	{ $$ = ~ $2; }
	|	expr '*' expr	{ $$ = $1 * $3; }
	|	expr '/' expr	{ $$ = $1 / $3; }
	|	expr '+' expr	{ $$ = $1 + $3; }
	|	expr '-' expr	{ $$ = $1 - $3; }
	|	expr SHL expr	{ $$= $1 << $3; }
	|	expr SHR expr	{ $$= $1 >> $3; }
	|	expr '&' expr	{ $$ = $1 & $3; }
	|	expr '^' expr	{ $$ = $1 ^ $3; }
	|	expr '|' expr	{ $$= $1 | $3; }
	;

op:		'='	{ $$ = OP_EQ; }
	|	'>'	{ $$ = OP_GT; }
	|	'<'	{ $$ = OP_LT; }
	|	'!' '='	{ $$ = OP_NOT | OP_EQ; }
	|	'<' '='	{ $$ = OP_NOT | OP_GT; }
	|	'>' '='	{ $$ = OP_NOT | OP_LT; }
	;

%%

#include <stdio.h>

FILE *infile;
FILE *outfile;
int linenum;

/*
 * Print error message.
 */
	static void
yyerror(char *s)
{
	fprintf(stderr, "parsing error near line %d: %s\n", linenum, s);
	errors++;
}

	static int
lgetc(void)
{
	int ch;

	ch = getc(infile);
	if (ch == '\n')
		linenum++;
	return ch;
}

	static void
unlgetc(int ch)
{
	ungetc(ch, infile);
}

/*
 * Lexical parser to return tokens to the yacc grammer.
 */
	static int
yylex(void)
{
	int ch;
	char *p;
	int v;
	char word[MAXNAME+1];

again:
	/* Skip white space. */
	do {
		ch = lgetc();
	} while (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r');

	if (ch == EOF)
		return 0;

	if (ch >= '0' && ch <= '9')
	{
		/* Integer. */
		v = 0;
		do {
			word[v++] = ch;
			ch = lgetc();
		} while ((ch >= '0' && ch <= '9') ||
			 (ch >= 'a' && ch <= 'f') ||
			 (ch >= 'A' && ch <= 'F') ||
			 ch == 'x' || ch == 'X');
		word[v] = '\0';
		unlgetc(ch);
		yylval.num = strtol(word, &p, 0);
		if (*p != '\0')
		{
			fprintf(stderr, "badly formed number\n");
			errors++;
		}
		return INTEGER;
	}
	if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_')
	{
		/* Word. */
		v = 0;
		do {
			word[v++] = ch;
			ch = lgetc();
		} while ((ch >= 'a' && ch <= 'z') ||
			 (ch >= 'A' && ch <= 'Z') ||
			 (ch >= '0' && ch <= '9') ||
			 ch == '_' ||
			 ch == '.' ||
			 ch == '$');
		word[v] = '\0';
		unlgetc(ch);
		if (strcmp(word, "driver") == 0)
			return DRIVER;
		if (strcmp(word, "end") == 0)
			return END;
		if (strcmp(word, "icon") == 0)
			return ICON;
		if (strcmp(word, "module") == 0)
			return MODULE;
		if (strcmp(word, "needs") == 0)
			return NEEDS;
		if (strcmp(word, "or") == 0)
			return OR;
		if (strcmp(word, "provides") == 0)
			return PROVIDES;
		if (strcmp(word, "uses") == 0)
			return USES;
		if (strcmp(word, "version") == 0)
			return VERSION;
		yylval.str = malloc(strlen(word) +1);
		strcpy(yylval.str, word);
		return NAME;
	}
	if (ch == '"')
	{
		/* Quoted word. */
		v = 0;
		while ((ch = lgetc()) != '"')
		{
			word[v++] = ch;
			if (ch == EOF || ch == '\n' || ch == '\r')
			{
				fprintf(stderr, "unterminated string\n");
				errors++;
				return END;
			}
		}
		word[v] = '\0';
		yylval.str = malloc(strlen(word) +1);
		strcpy(yylval.str, word);
		return NAME;
	}
	if (ch == '#')
	{
		/* Skip comments. */
		do
			ch = lgetc();
		while (ch != '\n' && ch != EOF);
		goto again;
	}
	if (ch == '<')
	{
		ch = lgetc();
		if (ch == '<') return SHL;
		unlgetc(ch);
		ch = '<';
	}
	if (ch == '>')
	{
		ch = lgetc();
		if (ch == '>') return SHR;
		unlgetc(ch);
		ch = '>';
	}
	return ch;
}

/*
 * Print usage message.
 */
	static void
usage(void)
{
	fprintf(stderr, "usage: ddc [-o output] file\n");
	exit(1);
}

/*
 * Entry point.
 */
main(int argc, char *argv[])
{
       int ch;

	extern char *optarg;
	extern int optind;
	char *infilename;
	char *outfilename = NULL;

	while ((ch = getopt(argc, argv, "o:")) != EOF)
	switch (ch)
	{
	case 'o':
		outfilename = optarg;
		break;
	default:
		usage();
		exit(1);
	}

	if (optind != argc-1)
		usage();
	infilename = argv[optind];
	infile = fopen(infilename, "r");
	if (infile == NULL)
	{
		perror(infilename);
		exit(1);
	}

	if (outfilename == NULL)
	{
		outfilename = malloc(strlen(infilename) + 5);
		strcpy(outfilename, infilename);
		strcat(outfilename, ".bin");
	}
	outfile = fopen(outfilename, "w");
	if (outfile == NULL)
	{
		perror(outfilename);
		exit(1);
	}
	linenum = 1;
	yyparse();
	if (errors)
	{
		fprintf(stderr, "%d errors\n", errors);
		unlink(outfilename);
		exit(1);
	}
	return 0;
}

#define	MAKE_ID(c1,c2,c3,c4)	(((c1)<<24)|((c2)<<16)|((c3)<<8)|(c4))
char buffer[MAXCHUNKSIZE];
char *pbuf;
unsigned long totalBytes;

/*
 * Output a byte to the chunk buffer.
 */
	void
OutputByte(unsigned char n)
{
	*pbuf++ = n;
	totalBytes++;
}

/*
 * Output a longword to the chunk buffer.
 */
	void
OutputLong(unsigned long n)
{
	OutputByte(n >> 24);
	OutputByte(n >> 16);
	OutputByte(n >> 8);
	OutputByte(n);
}

/*
 * Write the chunk buffer to the output file.
 */
	void
FlushChunk(void)
{
	unsigned long size;

	size = pbuf - buffer;
	((unsigned long *)buffer)[1] = size - 8;

        if (size & 1)
        {
        	size++;
		OutputByte(0);
	}

	fwrite(buffer, 1, size, outfile);
	pbuf = buffer;
}

/*
 * Begin the output file.
 */
	void
OutputFileHeader(void)
{
	pbuf = buffer;
	totalBytes = 0;
	OutputLong(MAKE_ID('F','O','R','M'));
	OutputLong(0); /* Will be corrected in OutputFileTrailer */
	OutputLong(MAKE_ID('D','D','F',' '));
	FlushChunk();
}

/*
 * Finish up the output file.
 */
	void
OutputFileTrailer(void)
{
	totalBytes -= 8;
	fseek(outfile, 4, 0);
	fwrite(&totalBytes, 1, sizeof(totalBytes), outfile);
}

/*
 * Output the version chunk.
 */
	void
OutputVersion(int version, int revision)
{
int           i;
unsigned char buffer[20];

	OutputLong(MAKE_ID('!','V','e','r'));
	OutputLong(0);

	sprintf(buffer, "%u.%u", version, revision);
	i = 0;
	while (buffer[i])
	    OutputByte(buffer[i++]);

	FlushChunk();
}

/*
 * Output the driver name chunk.
 */
	void
OutputDriverName(char *name, char *module, char *driver)
{
	int i;

	OutputLong(MAKE_ID('N','A','M','E'));
	OutputLong(0);
	do {
		OutputByte(*name);
	} while (*name++ != '\0');
	do {
		OutputByte(*module);
	} while (*module++ != '\0');
	do {
		OutputByte(*driver);
	} while (*driver++ != '\0');

	/* Add another 6 null bytes for future expansion. */
	for (i = 0;  i < 6;  i++)
		OutputByte(0);

	FlushChunk();
}

/*
 * Begin the NEEDS chunk.
 */
	void
OutputNeedHeader(void)
{
	OutputLong(MAKE_ID('N','E','E','D'));
	OutputLong(0);
}

/*
 * Finish up the NEEDS chunk.
 */
	void
OutputNeedTrailer(void)
{
	OutputKeyword(K_END_NEED_SECTION);
	FlushChunk();
}

/*
 * Begin the PROVIDES chunk.
 */
	void
OutputProvideHeader(void)
{
	OutputLong(MAKE_ID('P','R','O','V'));
	OutputLong(0);
}

/*
 * Finish up the PROVIDES chunk.
 */
	void
OutputProvideTrailer(void)
{
	OutputKeyword(K_END_PROVIDE_SECTION);
	FlushChunk();
}

/*
 * Output an ICON section.
 */
	void
OutputIcon(char *filename)
{
	FILE *f;
	int ch;

	if ((f = fopen(filename, "r")) == NULL)
	{
		perror(filename);
		exit(1);
	}
	OutputLong(MAKE_ID('I','C','O','N'));
	OutputLong(0);
	while ((ch = getc(f)) != EOF)
		OutputByte(ch);
	fclose(f);
	FlushChunk();
}

/*
 * Output a keyword token.
 */
	void
OutputKeyword(unsigned int k)
{
	OutputByte((TOK_KEYWORD<<4)|k);
}

/*
 * Output an operator token.
 */
	void
OutputOp(unsigned int k)
{
	OutputByte((TOK_OP<<4)|k);
}

/*
 * Output a string token.
 */
	void
OutputString(char *s)
{
	OutputByte(TOK_STRING<<4);
	do {
		OutputByte(*s);
	} while (*s++ != '\0');
}

/*
 * Output an integer token.
 */
	void
OutputInt(signed long v)
{
	int neg = (v < 0) ? TOK_INT_NEG : 0;

	if (neg) v = -v;
	if (v < (1<<3))
	{
		OutputByte((TOK_INT1<<4)|neg|v);
	} else if (v < (1<<11))
	{
		OutputByte((TOK_INT2<<4)|neg|(v>>8));
		OutputByte(v&0xFF);
	} else if (v < (1<<19))
	{
		OutputByte((TOK_INT3<<4)|neg|(v>>16));
		OutputByte((v>>8)&0xFF);
		OutputByte(v&0xFF);
	} else if (v < (1<<27))
	{
		OutputByte((TOK_INT4<<4)|neg|(v>>24));
		OutputByte((v>>16)&0xFF);
		OutputByte((v>>8)&0xFF);
		OutputByte(v&0xFF);
	} else
	{
		OutputByte((TOK_INT5<<4)|neg|0);
		OutputByte((v>>24)&0xFF);
		OutputByte((v>>16)&0xFF);
		OutputByte((v>>8)&0xFF);
		OutputByte(v&0xFF);
	}
}
