/* @(#) dumpelf.c 95/04/06 1.2 */

#include <stdio.h>
#include <string.h>

#define LL 1024

static FILE* fin;
static FILE* fout;

#define lgets(s) fgets((s), LL, fin)

int main(int argc, char *argv[])
{
    static char b1[LL];
    static char b2[LL];
    char* first= b1;
    char* next= b2;

    if(argc < 2 || argc > 3)
    {
	fprintf(stderr, "usage: %s elfin [asmout]\n", *argv);
	return 2;
    }
    sprintf(b1, "objdump -dr %s", argv[1]);
    fin= popen(b1, "r");
    if(!fin)
    {
	perror("popen(objdump)");
	return 1;
    }
    if(argc == 3)
    {
	fout= fopen(argv[2], "w");
	if(!fout)
	{
	    perror("fopen($2, w)");
	    return 1;
	}
    }
    else fout= stdout;

    while(first= lgets(first), first)
    {
	while(next= lgets(next), next)
	{
	    char* t;

	    if(!strncmp("\t\tRELOC: ", next, 9))
	    {
		char* p;
		char* q;
		char* r= strchr(strchr(next, ' ') + 10, ' ') + 1;

		if((p= strstr(first, " bl\t"), p) && !strncmp(first, p + 4, 8))
		{
		    strstr(first, " bl\t")[4]= '\0';
		    fprintf(fout, "%s%s", first, r);
		    break;
		}
		if((p= strstr(first, " b\t"), p) && !strncmp(first, p + 3, 8))
		{
		    strstr(first, " b\t")[3]= '\0';
		    fprintf(fout, "%s%s", first, r);
		    break;
		}
		q= strstr(first + 14, ",0");
		if(q)
		{
		    if(strstr(first, " lis\t") || strstr(first, " addis\t"))
		    {
			q[1]= '\0';
			*strchr(r, '\n')= '\0';
			fprintf(fout, "%s%s@ha\n", first, r);
			break;
		    }
		    if(strstr(first, " stw\t")
			|| strstr(first, " lwz\t")
			|| strstr(first, " addi\t")
			|| strstr(first, " lbz\t")
			|| strstr(first, " lfs\t"))
		    {
			q[1]= '\0';
			q += 2;
			*strchr(r, '\n')= '\0';
			fprintf(fout, "%s%s@l%s", first, r, q);
			break;
		    }
		}
	    }

	    fputs(first, fout);
	    t= first;
	    first= next;
	    next= t;
	}
	if(!next && first) fputs(first, fout);
    }
    return 0;
}
