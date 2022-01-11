/*
 * RenderThread.h
 *
 * Handles all render-related cmds from the main task
 *
 * Copyright (c) 1996, The 3DO Company
 */


#ifndef __RENDERTHREAD__
#define __RENDERTHREAD__

/* ---------- Constants */


#define	kRenderQSemaphoreName	"RenderQ"


/* ---------- Types */


typedef enum QCmd {
/*  cmd                 Param1         Param2         Param3         RetValAddr */
/*  ---                 ------         ------         ------         ---------- */
    cmdInitGraphics, /* FBWid,FBHeight FBDepth        CmdListBufSize 0 */
    cmdCopySnippet,  /* CltSnippet     <unused>       <unused>       0 */
    cmdSortPods,     /* PodCount       PPodBuffer     <unused>       &FirstPodPtr */
    cmdDrawPods,     /* FirstPod       CloseData      <unused>       &PodPipeRet */
    cmdSwapPods,
    cmdSwapScreens,  /* <unused>       <unused>       <unused>       0 */
    cmdSync,         /* <unused>       <unused>       <unused>       0 */
    cmdExit,         /* CloseData      <unused>       <unused>       0 */

    cmdNoMore
} QCmd;

typedef struct RenderQNode {
    Node	rq;
    QCmd	rq_Cmd;
    uint32	rq_Param1;
    uint32	rq_Param2;
    uint32	rq_Param3;
    uint32*	rq_RetValAddr;
} RenderQNode;


/* ---------- Prototypes */

Err RenderThreadInit(void);
void RenderQ(QCmd cmd, uint32 p1, uint32 p2, uint32 p3, uint32* retValAddr);
void RenderSync(void);
void RenderThread(void);

#define RenderQ_M(cmd, p1, p2, p3, ret) \
		RenderQ(cmd, (uint32)p1, (uint32)p2, (uint32)p3, (uint32*)ret)

#endif	/* __RENDERTHREAD__ */

