/* @(#) towers.c 95/10/21 1.2 */

#include <stdio.h>

#define maxcells	18
#define stackrange	3

struct    element {
	    int discsize;
	    int next;
	};

int    		stack[stackrange+1];
struct element  cellspace[maxcells+1];
int    		freelist, movesdone;

    /*  Program to Solve the Towers of Hanoi */

Error (char *emsg)
{
	printf(" Error in Towers: %s\n",emsg);
}

Makenull (int s)
{
	stack[s]=0;
}

int Getelement (void)
{
	int temp;
	if ( freelist>0 ) {
	    temp = freelist;
	    freelist = cellspace[freelist].next;
	}
	else
	{
	    Error("out of space   ");
	    temp = -1;
	}
	return (temp);
}

void Push(int i,int s)
{
	int errorfound, localel;
	errorfound=false;
	if ( stack[s] > 0 )
    	if ( cellspace[stack[s]].discsize<=i ) {
		errorfound=true;
		Error("disc size error");
	}
	if ( ! errorfound ) {
	    localel=Getelement();
	    cellspace[localel].next=stack[s];
	    stack[s]=localel;
	    cellspace[localel].discsize=i;
	}
}

void Init (int s,int n)
{
	int discctr;
	Makenull(s);
	for ( discctr = n; discctr >= 1; discctr-- )
    		Push(discctr,s);
}

int Pop (int s)
{
 	int temp, temp1;
	if ( stack[s] > 0 )
    	{
	    temp1 = cellspace[stack[s]].discsize;
	    temp = cellspace[stack[s]].next;
	    cellspace[stack[s]].next=freelist;
	    freelist=stack[s];
	    stack[s]=temp;
	    return (temp1);
    	}

  	Error("nothing to pop ");
    	return -1;
}

void MoveIt (int s1,int s2)
{
	Push(Pop(s1),s2);
	movesdone=movesdone+1;
}

void tower(int i,int j,int k)
{
	int other;

	if ( k==1 )
    		MoveIt(i,j);
	else {
	    	other=6-i-j;
	    	tower(i,other,k-1);
	    	MoveIt(i,j);
	    	tower(other,j,k-1);
	}
}


void Towers (void)
{
    int i;
    for ( i=1; i <= maxcells; i++ )
	cellspace[i].next=i-1;
    freelist=maxcells;
    Init(1,14);
    Makenull(2);
    Makenull(3);
    movesdone=0;
    tower(1,2,14);
    if ( movesdone != 16383 )
	printf (" Error in Towers.\n");
}
