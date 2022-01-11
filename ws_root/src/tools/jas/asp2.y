%{
/* @(#) asp2.y 95/09/01 1.3 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct assoc_t
{
    char* s;
    void* d;
}
assoc_t;

typedef struct context_t
{
    assoc_t* l;
    int n;
}
context_t;

#define new_context() ((context_t*)calloc(1, sizeof(context_t)))
static int add_assoc(context_t* context, char const* id, void* data);
static void* get_data(context_t* context, char const* id);

typedef struct node_t
{
    context_t* recmem; /* 0 for non-record record members. */
    int offset;
    int size;
}
node_t;

static void recblow(context_t* assoc, char* name, int offset);

extern FILE* yyout;
extern int lineno;
extern int no_double;

static char base[256], recname[88];
static char* curbase;
static int size;
static context_t* a;
static context_t* cr;
%}
 
%union
{
#undef STR_LEN
#define STR_LEN 256
    char string[STR_LEN];
    int number;
}

%token RECORD ENDR ORG
%token <string> IDENT
%token <number> NUMBER DS

%%

records	:	/* Nothing */
	|	records record
	;

record	:	RECORD
	    {
		cr= new_context();
	    }
		IDENT
	    {
		add_assoc(cr, "*name", strdup($3));
		strcpy(recname, $3);
		add_assoc(a, $3, cr);
		size= 0;
		putchar('\n');
	    }
		fieldlist ENDR
	    {
		add_assoc(cr, "*size", (void*)size);
		fprintf(yyout, "%s\t.equ\t%d\n", recname, size);
	    }
	;

fieldlist:	/* Nothing */
	|	fieldlist field
	;

field	:	IDENT
	    {
		/* Assume a single long. */
		node_t* p= (node_t*)malloc(sizeof(node_t));

		p->recmem= 0;
		p->offset= size;
		p->size= 4;
		add_assoc(cr, $1, p);
		fprintf(yyout, "%s.%s\t.equ\t%d\n", recname, $1, size);
		size += p->size;
	    }
	|	IDENT DS NUMBER
	    {
		node_t* p= (node_t*)malloc(sizeof(node_t));

		p->recmem= 0;
		p->offset= size;
		p->size= $2 * $3;
		add_assoc(cr, $1, p);
		fprintf(yyout, "%s.%s\t.equ\t%d\n", recname, $1, size);
		size += p->size;
	    }
	|	IDENT DS IDENT
	    {
		node_t* p= (node_t*)malloc(sizeof(node_t));

		p->recmem= (context_t*)get_data(a, $3);
		if(!p->recmem)
		{
		    /* Assume it's a label for an equate. */
		    fprintf(yyout, "%s.%s\t.equ\t%s\n", recname, $1, $3);
		    free(p);
		}
		else
		{
		    p->offset= size;
		    p->size= (int)get_data(p->recmem, "*size");
		    add_assoc(cr, $1, p);
		    fprintf(yyout, "%s.%s\t.equ\t%d\n", recname, $1, size);
		    strcpy(base, recname);
		    strcat(base, ".");
		    curbase= base + strlen(base);
		    recblow(p->recmem, $1, size);
		    *base= '\0';
		    size += p->size;
		}
	    }
	|	ORG NUMBER
	    {
		size= $2;
	    }
	;

%%

int main(int argc)
{
    no_double= argc != 1;
    a= new_context();
    curbase= base;
    return yyparse();
}

static void recblow(context_t* context, char* fname, int offset)
{
    char* p;
    int i;
    assoc_t* a;

    for(a= context->l, i= context->n; i--; a++)
    {
	node_t* n;
	char* p= a->s;
	if(*p == '*') continue;
	n= (node_t*)a->d;
	if(n->recmem)
	{
	    /* Recurse. */
	    char* s= curbase;
	    strcpy(s, fname);
	    strcat(s, ".");
	    curbase= s + strlen(s);
	    recblow(n->recmem, p, offset + n->offset);
	    *s= '\0';
	}
	else fprintf(yyout, "%s%s.%s\t.equ\t%d\n", base, fname, p, offset + n->offset);
    }
}

yyerror(s)
char* s;
{
    fprintf(stderr, "asp2 error: \"%s\" in line %d.\n", s, lineno);
}

static int add_assoc(context_t* context, char const* id, void* data)
{
    assoc_t* l;

    if(get_data(context, id)) return 1;

    /* Extend the association list for this context. */
    context->n++;
    context->l= context->l /* Does realloc() accept a nil pointer? */
	? (assoc_t*)realloc(context->l, sizeof(assoc_t) * context->n)
	: (assoc_t*)malloc(sizeof(assoc_t) * context->n);
    if(!context->l)
    {
	perror("add_assoc");
	exit(1);
    }

    /* Add the new association to the end of the list. */
    context->l[context->n - 1].s= strdup(id);
    context->l[context->n - 1].d= data;
    return 0;
}

static void* get_data(context_t* context, char const* id)
{
    int i;

    for(i= 0; i < context->n; i++)
    {
	if(!strcmp(id, context->l[i].s)) return context->l[i].d;
    }
    return 0;
}
