#ifndef BSDF_Anim_INC 
#define BSDF_Anim_INC 1

#include "kf_eng.h"
#include "kf_spline.h"


/* bit positions within key flag set */
#define TRANS_MAT (1 << 4)		/* 4 x 3 local transform as anim data */
#define TRANS_CONST (1 << 3)	/* Translation portion is one key */
#define TRANS_CUBIC (1 << 2)	/* Translation portion is cubic spline */

#define ROT_CONST (1 << 1)		/* Rotation portion is one key */
#define ROT_CUBIC (1 << 0)		/* Rotation portion is cubic spline */

#define MAX_HIER_NODES 200

/* typedef struct _SDFBObject SDFBObject; */

/*
** Node type in the tree
*/
typedef enum 
{
    ePopNode,
    ePushNode
} AmNodeType;

/*
** Node in the tree
*/
typedef struct 
{
    unsigned int    type : 24;
    unsigned int    n    : 8;
    Point3          pivot;
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
    uint32      flags;
    float       beginFrame;
    float       endFrame;
    float       lockedFPS;
    float       timeStart;
    float       timeEnd;
    float       curTime;
    float       curInc;
} AmControl;

/*
** Character Tree Information
*/
typedef struct 
{
	uint32 numNodes;
	uint32 treeDepth;
	uint32 numPods;
	AmControl control;	
} TreeInfo;


#endif
