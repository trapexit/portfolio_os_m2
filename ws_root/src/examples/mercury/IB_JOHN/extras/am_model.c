/*
**	File:		AM_Model.c
**
**	Contains:	Articulated model hierarchy traverser and animation code
**
**	Written by:	Ravindar Reddy
**
**	Copyright:	© 1996 by The 3DO Company. All rights reserved.
**	          	This material constitutes confidential and proprietary
**	          	information of the 3DO Company and shall not be used by
**	          	any Person or for any purpose except as expressly
**	          	authorized in writing by the 3DO Company.
**
**	Last Modified:	 96/05/08	version		1.23
*/

#ifdef MACINTOSH
#include <stdio.h>
#include <string.h>
#include <:kernel:mem.h>
#else
#include <stdio.h>
#include <string.h>
#include <kernel/mem.h>
#endif

#include "AM_Model.h"
#include "AM_Anim.h"

/*
** Initialise the traversal stack based on the maximun
** tree depth
*/
AmCloseData *
AM_Init(
	unsigned int tree_depth
	)
{
	AmCloseData *amc;

	amc = ( AmCloseData *) AllocMemAligned( sizeof( AmCloseData ),
											MEMTYPE_NORMAL,32 );
	if( !amc )
	{
		printf("ERROR: Out of memory in AM_Init\n");
	} else {
		amc->stack = ( AmStack *) AllocMemAligned(
						sizeof( AmStack ) * tree_depth,
						MEMTYPE_NORMAL|MEMTYPE_TRACKSIZE, 32 );

		if( !amc->stack )
			printf("ERROR: Out of memory in AM_Init\n");
	}

	return( amc );
}

/*
** cleanup the stack and closedata
*/
void
AM_End(
	AmCloseData *amc
	)
{
	FreeMem( amc->stack, TRACKED_SIZE );
	FreeMem( amc,sizeof( AmCloseData ) );
}

/*
** Given a single AmModel update it's position at the
** given time - mdl->control.curTime
*/
void
AM_Traverse(
	AmCloseData *cls,
	AmModel *mdl
	)
{
	/* points to parent and current transform obj */
	uint32	top, cur;
	uint32 j;
	AmStack	*stk = cls->stack;
	Vector3D proot = { 0.0, 0.0, 0.0 };
	float *a, *b, t[3];

	/* update the articulated model */
	top = 0;

	/* load the top node of the hierarchy */
	stk[top].flags = mdl->hierarchy[top].flags;
	stk[top].type = mdl->hierarchy[top].type;
	stk[top].n = mdl->hierarchy[top].n;
	stk[top].pivot = &mdl->hierarchy[top].pivot;
	stk[top].anim = &mdl->animation[top];
	stk[top].mat = &mdl->matrices[top];

	/* stuff the top node matrix with it's local matrix */
	mdl->animation[top].proutine( &proot,
									stk[top].pivot,
									stk[top].anim->animData,
									mdl->control.curTime,
									stk[top].mat );

	for( j = 1; j < mdl->numNodes; j++ )
	{
		/* set the current stack element data */
		cur = top + 1;
		stk[cur].n = mdl->hierarchy[j].n;
		stk[cur].type = mdl->hierarchy[j].type;
		stk[cur].pivot = &mdl->hierarchy[j].pivot;
		stk[cur].anim = &mdl->animation[j];
		stk[cur].mat = &mdl->matrices[j];

		/* stuff the top node matrix with it's local matrix */
		mdl->animation[j].proutine(	stk[top].pivot,
									stk[cur].pivot,
									stk[cur].anim->animData,
									mdl->control.curTime,
									stk[cur].mat );

		/* calculate the total matrix of this node. pre-multiply
		** child's matrix with cumulative parent's matrix
		** calculate the matrices in place rather calling a
		** function for efficiency
		*/
		a = (float *)stk[cur].mat;
		b = (float *)stk[top].mat;

		t[0] = *a * *b + *(a+1) * *(b+3) + *(a+2) * *(b+6);
		t[1] = *a * *(b+1) + *(a+1) * *(b+4) + *(a+2) * *(b+7);
		t[2] = *a * *(b+2) + *(a+1) * *(b+5) + *(a+2) * *(b+8);
		*a = t[0]; *(a+1) = t[1]; *(a+2) = t[2];

		t[0] = *(a+3) * *b + *(a+4) * *(b+3) + *(a+5) * *(b+6);
		t[1] = *(a+3) * *(b+1) + *(a+4) * *(b+4) + *(a+5) * *(b+7);
		t[2] = *(a+3) * *(b+2) + *(a+4) * *(b+5) + *(a+5) * *(b+8);
		*(a+3) = t[0]; *(a+4) = t[1]; *(a+5) = t[2];

		t[0] = *(a+6) * *b + *(a+7) * *(b+3) + *(a+8) * *(b+6);
		t[1] = *(a+6) * *(b+1) + *(a+7) * *(b+4) + *(a+8) * *(b+7);
		t[2] = *(a+6) * *(b+2) + *(a+7) * *(b+5) + *(a+8) * *(b+8);
		*(a+6) = t[0]; *(a+7) = t[1]; *(a+8) = t[2];

		t[0]  = *(a+9) * *(b+0) + *(a+10) * *(b+3) + *(a+11) * *(b+6) + *(b+9);
		t[1] = *(a+9) * *(b+1) + *(a+10) * *(b+4) + *(a+11) * *(b+7) + *(b+10);
		t[2] = *(a+9) * *(b+2) + *(a+10) * *(b+5) + *(a+11) * *(b+8) + *(b+11);
		*(a+9) = t[0]; *(a+10) = t[1]; *(a+11) = t[2];

		/*
		** based on the node type (Push, Pop) adjust the stack
		** for Pop nodes the 'n' value is equal to the stack jump value
		*/
		if( stk[cur].type == ePushNode )
			top++;
		else
			top -= stk[cur].n;
	}
}

/*
** Given a list of AmModels update the position at the
** given time and animation controls. This function can be
** modified to fit to the application needs
*/
void
AM_Evaluate(
	AmCloseData *cls,
	AmModel *mdls,
	uint32 count,
	float realFPS
	)
{
	AmModel	*mdl = mdls;
	register uint32 i;

	/* update all the models in the articulated model list */
	for( i = 0; i < count; i++ )
	{
		if( mdl->control.flags & updateTheAnim )
		{
			/* traverse the model to update the positions */
			AM_Traverse( cls, mdl );

			/* calculate the increment based on the frame rate */
			if( mdl->control.flags & updateToRealFPS )
				mdl->control.curInc = mdl->control.lockedFPS / realFPS;

			/* increment the current time */
			mdl->control.curTime += mdl->control.curInc;

			/* cycle the animation */
			if( mdl->control.curTime > mdl->control.timeEnd )
			{
				if( mdl->control.flags & cycleTheAnim )
					mdl->control.curTime = mdl->control.timeStart +
								mdl->control.curTime - mdl->control.timeEnd;
				else {
					mdl->control.flags &= ~updateTheAnim;
					if( mdl->control.flags & stopTheAnim )
						mdl->control.curTime = mdl->control.timeEnd;
					else if ( mdl->control.flags & resetTheAnim )
						mdl->control.curTime = mdl->control.timeStart;
				}
			}
		}

		mdl = mdl->next;
	}

}
/*
** Link the all the Pods from different AmModels into a
** single linked list for sorting and rendering
*/
Pod*
AM_GetPodList(
	AmModel *mdls,
	uint32 mdlCount,
	uint32 *podCount
	)
{
	register uint32	i, j, k;
	AmModel	*mdl;
	AmNode *amdl;
	Pod *pods = NULL;
	Pod *pods_head = NULL;

	mdl = mdls;
	*podCount = 0;
	if( mdlCount <= 0 ) return NULL;

	/* collect the podlist from all of the articulated models */
	for( i = 0; i < mdlCount; i++ )
	{
		/* connect all the Pods in the AmModel */
		for( j = 0; j < mdl->numNodes; j++ )
		{
			amdl = ( mdl->hierarchy + j );
			if( !( amdl->flags & hideTheModel ) && amdl->podCount )
			{
				*podCount += amdl->podCount;

				/* Pods list head */
				if( pods_head == NULL )
					pods_head = &mdl->pods[amdl->podOffset];
				else pods->pnext = &mdl->pods[amdl->podOffset];

				pods = &mdl->pods[amdl->podOffset];

				for( k = 0; k < ( amdl->podCount - 1 ); k++ )
				{
					pods->pnext = pods + 1;
					pods++;
				}
			}
		}
		mdl = mdl->next;
	}

	return ( pods_head );
}

/*
** Duplicate the source AmModel and insert that into the
** given linked list
*/
AmModel *
AM_Duplicate(
	AmModel *list,
	uint32 *amCount,
	AmModel *srcMdl,
	uint32 dupCount
	)
{
	AmModel *mdl_cur;
	AmModel *m;
	uint32 i, j, len;
	static float pos_inc = 0.0;
	Matrix *mb, *nb;
	float inc = 15.0;
	float center = inc * dupCount / 2.0;

	mdl_cur = list;

	for( i = 1; i < (*amCount); i++ )
	{
		mdl_cur = mdl_cur->next;
	}

	for( i = 0; i < dupCount; i++ )
	{
		m = ( AmModel *) AllocMem( sizeof( AmModel ), MEMTYPE_NORMAL );
		m->control = srcMdl->control;
		m->numNodes = srcMdl->numNodes;
		m->numPods = srcMdl->numPods;
		m->textures = srcMdl->textures;
		m->userData = srcMdl->userData;
		m->next = NULL;

		/* model matrix data */
		len = sizeof( Matrix ) * srcMdl->numNodes;
		m->matrices = ( Matrix *)AllocMem( len, MEMTYPE_NORMAL );
		memcpy( m->matrices, srcMdl->matrices, len );

		/* flattened hierarchy data */
		len = sizeof( AmNode ) * srcMdl->numNodes;
		m->hierarchy = ( AmNode *)AllocMem( len, MEMTYPE_NORMAL );
		memcpy( m->hierarchy, srcMdl->hierarchy, len );

		/* animation data */
		len = sizeof( AmAnim ) * srcMdl->numNodes;
		m->animation = ( AmAnim *)AllocMem( len, MEMTYPE_NORMAL );
		memcpy( m->animation, srcMdl->animation, len );

		/* model pod data */
		len = sizeof( Pod ) * srcMdl->numPods;
		m->pods = ( Pod *)AllocMem( len, MEMTYPE_NORMAL );
		memcpy( m->pods, srcMdl->pods, len );

		/* make the pods point to the new matrix buffer */
		mb = srcMdl->matrices;
		nb = m->matrices;
		for( j = 0; j < m->numPods; j++ )
		{
			m->pods[j].pmatrix = (Matrix *)((uint32 )nb +
								(uint32 )(srcMdl->pods[j].pmatrix) -
								(uint32 )mb );
		}

		/* reposition the object in space */
		pos_inc += 15.0;
		m->control.curTime += pos_inc / 5.0;
		m->hierarchy[0].pivot.x += ( pos_inc - center);

		/* insert the model in the list */
		mdl_cur->next = m;
		mdl_cur = m;
		(*amCount) += 1;
	}

	return( list );
}
