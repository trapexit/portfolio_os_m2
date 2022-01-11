/*
**	File:		AM_Model.h	
**
**	Contains:	Header file for articulated model data 
**
**	Written by:	Ravindar Reddy
**
**	Copyright:	© 1996 by The 3DO Company. All rights reserved.
**	          	This material constitutes confidential and proprietary
**	          	information of the 3DO Company and shall not be used by
**	          	any Person or for any purpose except as expressly
**	          	authorized in writing by the 3DO Company.
**
**	Last Modified:	 96/05/10	version		1.17	
*/

#ifndef AM_Model_INC
#define AM_Model_INC	1

#include "mercury.h"

typedef struct 
{
	float x, y, z, w;
} Quaternion;

typedef enum 
{
	ePopNode,
	ePushNode
} AmNodeType;

/*
	Flags to control the individual nodes in the hierarchy
	hideTheModel        : hide the model pods by removing
	                      them from the display list
*/
#define hideTheModel          (1 << 0)

/*
** A node in the articulated model hierarchy
** flags      : disable display 
** type       : Pop, Push
** n          : child count - 1, for "push" node. 
**              level of stack pop for "pop" node
** pivot      : pivot position in space 
** podCount   : number of geometry Pods refered by this node
** podOffset  : starting Pod offset in the AmModel Pod list
*/
typedef struct 
{
	unsigned int	type      : 1;
	unsigned int	flags     : 15;
	unsigned int	n         : 16;
	Vector3D		pivot;
	unsigned int    podCount  : 16; 
	unsigned int    podOffset : 16; 
} AmNode;

/*
	Flags to control the animation of an articulated model
	
	updateTheAnim        : set the flag to update the animation. This 
	                       enables/disables the animation engine and traverser	
	updateToRealFPS      : adjust the frame increment based on the current
	                       FPS. If this flag is not set then the frame
	                       increment will be constant. 
	cycleTheAnim         : play the animation from "timeStart" to "timeEnd" 
	                       repeatedly
	stopTheAnim          : stop at it's position at it's "timeEnd" and remain
	resetTheAnim         : go back to it's position at the "timeStart" and remain
	pingPongTheAnim      : when the animation hits "timeEnd", it plays backwards 
	                       to "timeStart"
*/
#define updateTheAnim          (1 << 0)
#define updateToRealFPS        (1 << 1)
#define cycleTheAnim           (1 << 2)
#define stopTheAnim            (1 << 3)
#define resetTheAnim           (1 << 4)
#define pingPongTheAnim        (1 << 5)

/*
** To control a single node level control parameters like :
** flags       : status flags as explained above
** beginFrame  : start frame of the animation definition
** endFrame    : end frame of the animation definition
** lockedFPS   : lock the animation to this speed
** timeStart   : start time for the animation play
** timeEnd     : end time for the animation play 
** curTime     : current time where the animation is at
** curInc      : current time increment constanr/variable 
**               based on the real frame frame rate
*/
typedef struct 
{
	uint32		flags;
	float		beginFrame;
	float		endFrame;
	float		lockedFPS;
	float		timeStart;
	float		timeEnd;
	float		curTime;
	float		curInc;
} AmControl;

/*
** Articulated Model Animation. This contains :
** proutine    : case routine that munges on the data to produce
**               local transform. This routine and the data can be
**               replaced by user to create procedural animation 
** Data encoding for different cases :
** pcase         : AM_EvalTransform
** animData    : Matrix     local matrix data
**
** pcase         : AM_EvalSpline
** animData    :
** CONSTANT :
**	position track :
**	uint32					flags	
**	Vector3D				postion 
**
**	rotation track :
**	uint32					flags	
**	Quaternion				rotational quaternion
**	
** CUBIC :
**	position track :
**	uint32					flags	
**	uint32					number of spline segments ( nsegs )
**	float[nsegs+1]			time vector
**	TransCubic[nsegs]		postion spline segment repeated "nsegs" times
**	
**	rotation track :
**	uint32				    flags		
**	uint32					number of spline segments ( nsegs )
**	float[nsegs+1]			time vector
**	RotCubic[nsegs]			rotational spline segment repeated "nsegs" times
*/
typedef struct AmAnim 
{
	void 		(*proutine)( Vector3D *, Vector3D *, uint32 *, float, Matrix * );
	uint32		*animData;
} AmAnim;

/*
** Articulated Model. This contains :
** control   : animation controls for this AmModel 
** numNodes  : number of nodes in given articulated model
** numPods   : number of geometry Pods in this AmModel 
** pods      : geometry PODs refered in this AmModel
** textures  : texture pages refered in this AmModel
** hierarchy : flattened hierarchy array
** animation : animation array 
** matrices  : characters matrix array. Each matrix in this array 
**             corresponds to a single node in the hierarchy.
** userData  : not used ( can be used to store model Pod list ) 
** next      : a link to the next AmModel
**
** Sizes of all the three arrays ( hierarchy, matrices, animation )
** are exactly same
*/
typedef struct AmModel 
{
	AmControl		control;	
	uint32			numNodes;
	uint32			numPods;
	Pod				*pods;
	PodTexture		*textures;
	Matrix			*matrices;
	AmNode			*hierarchy;				
	AmAnim			*animation;
	uint32			*userData;
	struct AmModel	*next;	
} AmModel;

/* 
** Flattened hierarchy that can be traversed as a 
** matrix stack for concatenation 
*/

typedef struct
{
	unsigned int	flags     : 1;
	unsigned int	type      : 15;
	unsigned int	n         : 16;
	Vector3D *pivot;
	AmAnim	*anim;
	Matrix	*mat;
} AmStack;	

/*
** Articulated Model global data:
** flags     : unused
** const1    : model traversal phase 
** const2    : constants for quat->matrix
*/

typedef struct 
{
	uint32		flags;
	float		const1;
	float		const2;
	AmStack		*stack;
} AmCloseData;

/*
** Function prototypes
*/
AmCloseData	*AM_Init( unsigned int max_model_depth );
void		AM_End( AmCloseData *block );
void		AM_Traverse( AmCloseData *block, AmModel *mdls );
void		AM_Evaluate( AmCloseData *block, AmModel *mdls, uint32 count, float fps );
Pod			*AM_GetPodList( AmModel *mdls, uint32 mdlCount, uint32 *podCount );
AmModel		*AM_Duplicate( AmModel *list, uint32 *amCount, AmModel *srcMdl, uint32 dupCount );

#endif
