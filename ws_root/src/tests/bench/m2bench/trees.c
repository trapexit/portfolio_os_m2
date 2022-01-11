/* @(#) trees.c 95/10/21 1.2 */

#include <stdio.h>
#include <stdlib.h>

#define nil 0L
#define sortelements 5000
#define SEED 74755

static int    sortlist[sortelements+1],
    biggest, littlest;

    struct node {
	struct node *left,*right;
	int val;
    };

struct node *tree;


    /* Sorts an array using treesort */

void tInitarr(void )
	{
	int i, temp;
	srand(SEED);
	biggest = 0; littlest = 0;
	for ( i = 1; i <= sortelements; i++ )
	    {
	    temp = rand();
	    sortlist[i] = temp - (temp/100000L)*100000L - 50000L;
	    if ( sortlist[i] > biggest ) biggest = sortlist[i];
	    else if ( sortlist[i] < littlest ) littlest = sortlist[i];
	    }
	}

	void CreateNode (struct node **t, int n)
	{
		*t = (struct node *)malloc(sizeof(struct node));
		(*t)->left = nil; (*t)->right = nil;
		(*t)->val = n;
	}

    void Insert(int n, struct node *t)
	/* insert n into tree */
    {
	   if ( n > t->val )
		if ( t->left == nil ) CreateNode(&t->left,n);
		else Insert(n,t->left);
    	   else if ( n < t->val )
		if ( t->right == nil ) CreateNode(&t->right,n);
		else Insert(n,t->right);
    }

    int Checktree(struct node *p)
    /* check by inorder traversal */
    {
    int result;
        result = true;
		if ( p->left != nil )
		   if ( p->left->val <= p->val ) result=false;
		   else result = Checktree(p->left) && result;
		if ( p->right != nil )
		   if ( p->right->val >= p->val ) result = false;
		   else result = Checktree(p->right) && result;
	return( result);
    } /* checktree */

void Trees(void)
{
    int i;
    tInitarr();
    tree = (struct node *)malloc(sizeof(struct node));
    tree->left = nil; tree->right=nil; tree->val=sortlist[1];
    for ( i = 2; i <= sortelements; i++ ) Insert(sortlist[i],tree);
    if ( ! Checktree(tree) ) printf ( " Error in Tree.\n");
}
