#include "ifflib.h"
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "M2TXTypes.h"
#include "os.h"
#include <assert.h>
#include "lws.i"
#include "lws.h"

extern int CommentLevel;
LWS *cur_lws;

/***
 *
 * Token parsing data
 *
 ***/
int		tokenType = 0;		/* type of input token */
static	bool	gotToken = FALSE;	/* TRUE if token has been read ahead */
char	token[256];			/* input token buffer */

static char *token_types[] =	/* token names for error messages */
{
	"end",
	"keyword",
	"number",
	"{",
	"}",
	"(",
	")",
	"|",
	",",
	"[",
	"]",
	"bitfield",
};
static int	max_token_types = sizeof(token_types) / sizeof(char *);

/****
 *
 * Read a floating point number from the current LWS file and save
 * its value in the given location. Print an error message if
 * a number cannot be parsed.
 *
 * returns:
 *	TRUE if successfully read, otherwise FALSE
 *
 ****/
bool
lws_read_float(gfloat* f)
{
	assert(f);
	if (lws_get_token() != T_NUMBER)
	   {
		lws_bad_token(T_NUMBER);
		return(FALSE);
	   }
	else
	   {
		sscanf(token, "%f", f);
		return(TRUE);
	   }
}

/****
 *
 * Read an integer from the current LWS file and save
 * its value in the given location. Print an error message if
 * a number cannot be parsed.
 *
 * returns:
 *	TRUE if successfully read, otherwise FALSE
 *
 ****/
bool
lws_read_int(int32* i)
{
  if (lws_get_token() != T_NUMBER)
    {
      lws_bad_token(T_NUMBER);
      return(FALSE);
    }
  else
    {
      if ((token[1]=='x') || (token[1]=='X'))
      	sscanf(token, "%lx", i);
      else
	sscanf(token, "%ld", i);
      return(TRUE);
    }
}

bool
lws_read_uint(uint32* i)
{

  unsigned long value;

  if (lws_get_token() != T_NUMBER)
    {
      lws_bad_token(T_NUMBER);
      return(FALSE);
    }
  else
    {
      if ((token[1]=='x') || (token[1]=='X'))
	sscanf(token, "%lx", &value);
      else
	sscanf(token, "%lu", &value);
      *i = value;
      return(TRUE);
    }
}

/****
 *
 * Read a name from the input stream. A name can be either a
 * string of alphanumeric characters or a quoted string.
 * Names are converted to lower case. An error message is
 * printed if a name does not follow in the input stream.
 *
 * returns:
 *	-> first character in name (null-terminated string)
 *	NULL if no name can be read
 *
 ****/
char* lws_read_name(char* s)
{
	if (!s)									/* token already parsed? */
	   {
		if (!lws_check_token(T_KEYWORD))	/* parse the name/string */
			return(NULL);
		s = token;
	   }
	while (*s)								/* convert to lower case */
		*s++ = tolower(*s);
	return(token);
}

/****
 *
 * Read a string from the input stream. A name can be either a
 * string of alphanumeric characters or a quoted string. We do
 * not do any translation of the characters. An error message is
 * printed if a name does not follow in the input stream.
 * String are not converted to lower case.
 *
 * returns:
 *	-> first character in name (null-terminated string)
 *	NULL if no name can be read
 *
 ****/
char*
lws_read_string()
{
	if (lws_get_token() != T_KEYWORD)
	   {
		lws_bad_token(T_KEYWORD);
		return(NULL);
	   }
	return(token);
}

/****
 *
 * Read a character from the input stream and update the LWS
 * line number if a newline is encountered. Because we run on
 * Mac, PC and Unix, an end of line character can have different
 * meanings:
 *	Unix	newline			\n
 *	Mac		carriage return	\r
 *	PC		carriage return and newline \r\n
 *
 * returns:
 *   character read
 *
 ****/
static int
GetChar(void)
{
	int32 c, d;

	c = K9_GetChar(cur_lws->Stream);
	if (c == '\n')					/* Unix end of line? */
	   {
		cur_lws->Lines++;			/* bump line number and exit */
		return(c);
	   }
	if (c != '\r')					/* not Mac end of line? */
		return(c);					/* just return the char */
	c = '\n';						/* always convert to newline */
	cur_lws->Lines++;				/* bump line number */
	d = K9_GetChar(cur_lws->Stream);
	if (d != '\n')					/* PC end of line? */
		K9_UngetChar(cur_lws->Stream, d);
	return(c);
}

/****
 *
 * Put back the given input character. If it is a newline,
 * update the LWS line number
 *
 ****/
static void
UngetChar(int c)
{
	if (c == '\n')
		cur_lws->Lines--;
	K9_UngetChar(cur_lws->Stream, c);
}


/****
 *
 * Get the next input token and return its type.
 *
 * returns:
 *	token	  -> first non-space character of input token
 *	tokenType = type of token
 *		T_END		end of file
 *		T_KEYWORD	name or quoted string
 *		T_NUMBER	integer or floating point number
 *		T_OR		|
 *		T_COMMA		,
 *		T_LBRACK	[
 *		T_RBRACK	]
 *		T_LPAREN	(
 *		T_RPAREN	)
 *
 ****/
int32
lws_get_token(void)
{
	int i;
	int c;

	if (gotToken)					/* did we already get one? */
	   {
		gotToken = FALSE;			/* indicate it has been used */
		return(tokenType);			/* just return it */
	   }
	for (;;)						/* find next token */
	   {
		while ((c = GetChar()) != EOF && isspace(c))
			{ ; }					/* consume space characters */

		if (c == EOF)				/* stop if end of file */
			return (tokenType = T_END);

/*		if (c == '#' || c == '/') */	/* consume comments */
		if (c == '#')	                /* consume comments */
		   {
			while ((c = GetChar()) != EOF && (c != '\n'))
				{ ; }
			continue;
		   }
		if (c == '"')				/* consume quoted strings */
		   {
			i = 0;
			while ((c = GetChar()) != EOF && c != '"')
			   {
				token[i] = c;
				i++;
			   }
			token[i] = '\0';
			return (tokenType = T_KEYWORD);
		   }
		if (c == '|')
		   {
			token[0] = c;
			token[1] = '\0';
			return (tokenType = T_OR);
		   }
		if (c == ',')
		   {
			token[0] = c;
			token[1] = '\0';
			return (tokenType = T_COMMA);
		   }
		if (c == '[')
		   {
			token[0] = c;
			token[1] = '\0';
			return (tokenType = T_LBRACK);
		   }
		if (c == ']')
		   {
			token[0] = c;
			token[1] = '\0';
			return (tokenType = T_RBRACK);
		   }
		if (c == '(')
		   {
			token[0] = c;
			token[1] = '\0';
			return (tokenType = T_LPAREN);
		   }
		if (c == ')')
		   {
			token[0] = c;
			token[1] = '\0';
			return (tokenType = T_RPAREN);
		   }
		if (c == '{')
		   {
			token[0] = c;
			token[1] = '\0';
			return (tokenType = T_LBRACE);
		   }
		if (c == '}')
		   {
			token[0] = c;
			token[1] = '\0';
			return (tokenType = T_RBRACE);
		   }
		/* extract keyword token */
		/*		if (c != EOF && (isalpha(c) || c == '_')) */
		if (c != EOF && (isalpha(c) || c == '_' || c == '/' || c == '\\' || c == ':'))
		  {
		    /*			for (i = 0; c != EOF && (isalnum(c) || c == '_' || c == '.'); i++) */
		    for (i = 0; c != EOF && (isalnum(c) || c == '_' || c == '/' || c == '\\' || c == ':' || c == '.' || c =='-'); i++)
			   {
				token[i] = c;
				c = GetChar();
			   }
			token[i] = '\0';
			UngetChar(c);
			return (tokenType = T_KEYWORD);
		   }
		else if (c != EOF && isdigit(c) ||
				 c == '.' ||
				 c == '+' ||
				 c == '-' ||
				 c == 'e' ||
				 c == 'E')
		   {
		     for (i = 0;
			  c != EOF &&
			    (isdigit(c) ||
			     isxdigit(c) ||
			     ((i==1) && (c=='x')) ||
			     ((i==1) && (c=='X')) ||
			     c == '.' ||
			     c == '+' ||
			     c == '-' ||
			     c == 'e' ||
			     c == 'E');
			  i++)
		       {
			 token[i] = c;
			 c = GetChar();
		       }
		     token[i] = '\0';
		     UngetChar(c);
		     return (tokenType = T_NUMBER);
		   }
		else
		  {
		    /*			GLIB_WARNING(("<< %c >>\n", c)); */
		    fprintf(stderr,"WARNING:<< %c >> \n",c);
		    lws_bad_token(T_UNKNOWN);
		    tokenType = T_END;
		  }
	   }

	/* can't get here */
	return (tokenType = T_END);
}



/****
 *
 * Print a diagnostic message indicating a bad token was parsed
 *
 ****/
void
lws_bad_token(int32 t)
{
  char *t_name;
  
  if ((t > 0) && (t < max_token_types))
    {
      t_name = token_types[t];
      fprintf(stderr, "WARNING:unexpected token '%s', was expecting %s\n", token, t_name);
    }
  else
    fprintf(stderr,"WARNING:unexpected token '%s'", token);
}


/****
 *
 * Put the current token back
 *
 ****/
void
lws_unget_token(void)
{
	assert(!gotToken);
	gotToken = TRUE;
}

/****
 *
 * Get the next token and check to see if it is of the given type.
 * Print an error message if not.
 *
 * returns:
 *   TRUE if next token is of the given type, else FALSE
 *
 ****/
bool
lws_check_token(int32 t)
{
  if (lws_get_token() == t)
    return(TRUE);
  lws_bad_token(t);
  return(FALSE);
}

void BadToken(char *string, char *token)
{
  /*  if (CommentLevel>5) */
  fprintf(stderr,"ERROR:%s \"%s\" at line %d\n", string, token,
	    cur_lws->Lines);
}
