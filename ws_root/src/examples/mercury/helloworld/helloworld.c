/*
 * @(#) helloworld.c 96/09/18 1.16
 *
 * Basic Mercury example.  This example initializes the display, bitmaps, and
 * GState, in preparation for rendering (in graphicsenv.c), creates some
 * simple geometry (in data.c) for two walls and a floor, then reads in a
 * model (in filepod.c), and displays a single frame of data containing both
 * pieces of data (in helloworld.c).  The camera can be positioned anywhere
 * via command line args, and, since the camera looks at the center of the
 * geometry, the scene will always be visible.  When a control pad button is
 * pressed, the program exits.  The program uses a standard texture reader
 * (in tex_read.c) and bsdf reader (in bsdf_read.c), which can read data from
 * the 3DO graphics conversion tools.
 *
 * Formatted with 8 space Tab stops
 */

#ifdef MACINTOSH
#include <kernel:types.h>
#include <kernel:mem.h>
#include <graphics:graphics.h>
#include <graphics:view.h>
#include <graphics:clt:gstate.h>
#include <misc:event.h>
#else
#include <kernel/types.h>
#include <kernel/mem.h>
#include <graphics/graphics.h>
#include <graphics/view.h>
#include <graphics/clt/gstate.h>
#include <misc/event.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "graphicsenv.h"
#include "mercury.h"
#include "matrix.h"
#include "data.h"
#include "bsdf_read.h"
#include "filepod.h"


/* Carries around all the program's data */
typedef struct AppData {
    CloseData* close;
    Matrix*    skewMatrix;
} AppData;


/* Mercury limits we have arbitrarily set -- lower if tight on mem */
#define kMaxCmdListWords    400

#define kDefaultCamLocX     200.0
#define kDefaultCamLocY     100.0
#define kDefaultCamLocZ     300.0
#define kDefaultFilename    "spacefig"


/* Prototypes for routines only used in this file */
void ParseArgs(int argc, char* argv[], Vector3D* camLoc, char* filename);
void Program_Init(GraphicsEnv* genv, AppData** app, Vector3D* camLocation,
		  char* filename, uint32* modelPodCount);
void Program_Begin(GraphicsEnv* genv, AppData* app, uint32 modelPodCount);


/* --------------------------------------------------------------------- */


int main(int argc, char* argv[])
{
    Err            err;
    GraphicsEnv*   genv;
    AppData*       appData;
    uint32         modelPodCount;
    Vector3D       desiredCamLocation;
    char           filename[32];
    

    /* Print usage */

    printf("\n");
    printf("Usage: helloworld [-c x y z] [-f <filename>\n");
    printf("  -c x y z will position the camera in world coords. at x,y,z\n");
    printf("  -f <filename> will use <filename>.bsf and <filename>.utf.\n");
    printf("     Do not append \".bsf\" or \".utf\" to <filename>\n");
    printf("     If a filename is not specified, the default is \"%s\"\n",
	   kDefaultFilename);
    printf("\n");

    ParseArgs(argc, argv, &desiredCamLocation, filename);

    /* Setup Display, GState */

    genv = GraphicsEnv_Create();
    if (genv == NULL) {
	printf("Couldn't allocate graphics environment.  Exiting.\n");
	exit(1);
    }
    err = GraphicsEnv_Init(genv, 0, 0, 0);  /* "Default" environment */
    if (err < 0) {
	PrintfSysErr(err);
	exit(err);
    }

    /* Setup the EventBroker */

    err = InitEventUtility(1, 0, 0);
    if (err < 0) {
	PrintfSysErr(err);
	exit(err);
    }

    /* Load & initialize data, then begin */

    Program_Init(genv, &appData, &desiredCamLocation, filename, &modelPodCount);
    Program_Begin(genv, appData, modelPodCount);

    /* Cleanup */

    printf("Program exiting.\n");
    GraphicsEnv_Delete(genv);

    return 0;
}


/* --------------------------------------------------------------------- */


void ParseArgs(int argc, char* argv[], Vector3D* camLoc, char* filename)
{
    int32 i;

    strcpy(filename, kDefaultFilename);
    /* Use this location as default camera position */
    camLoc->x = kDefaultCamLocX;
    camLoc->y = kDefaultCamLocY;
    camLoc->z = kDefaultCamLocZ;

    for (i=1; i<argc; i++) {
	if (strcmp(argv[i], "-f") == 0) {
	    if (i+1 < argc) {
		/* Be sure we have a filename after the "-f" */
		strcpy(filename, argv[i+1]);
		printf("Reading from files \"%s.bsf\" and \"%s.utf\"\n",
		       filename, filename);
	    } else {
		printf("Warning: No filename specified\n");
	    }
	}
	if (strcmp(argv[i], "-c") == 0) {
	    if (i+3 < argc) {
		/* Be sure we have 3 args to use as x, y, z */
		camLoc->x = (float)atoi(argv[i+1]);
		camLoc->y = (float)atoi(argv[i+2]);
		camLoc->z = (float)atoi(argv[i+3]);
		printf("Camera position set to (%g, %g, %g)\n",
		       camLoc->x, camLoc->y, camLoc->z);
	    } else {
		printf("Warning: -c needs X, Y, Z specified after it\n");
	    }
	}
    }
}


/* --------------------------------------------------------------------- */


void Program_Init(GraphicsEnv* genv, AppData** app, Vector3D* camLocation,
		  char* filename, uint32* modelPodCount)
{
    ViewPyramid  vp;
    Vector3D     lookAtPt;
    AppData*     appData;
    uint32       maxPodVerts;
    Matrix       camMtx = {
	{
	    1.0, 0.0, 0.0,
	    0.0, 1.0, 0.0,
	    0.0, 0.0, 1.0,
	    0.0, 0.0, 0.0
	}
    };

    /* Create an instance of AppData */

    *app = AllocMem(sizeof(AppData), MEMTYPE_NORMAL);
    if (*app == NULL) {
	printf("Couldn't create AppData structure.  Exiting\n");
	exit(1);
    }
    appData = *app;

    /* Load Model from disk */
    Model_LoadFromDisk(filename, modelPodCount, &maxPodVerts);

    /* Initialize Mercury */

    maxPodVerts = (maxPodVerts + 3) & ~3;
    appData->close = M_Init(maxPodVerts, kMaxCmdListWords, genv->gs);
    if (appData->close == NULL) {
	printf("Couldn't init Mercury.  Exiting\n");
	exit(1);
    }
    appData->close->fwclose = 1.01;
    appData->close->fwfar = 100000.0;
    appData->close->fscreenwidth = genv->d->width;
    appData->close->fscreenheight = genv->d->height;
    appData->close->depth = genv->d->depth;

    /* Setup camera */

    appData->skewMatrix = AllocMem(sizeof(Matrix), MEMTYPE_NORMAL);
    if (appData->skewMatrix == NULL) {
	printf("Couldn't allocate skew matrix.  Exiting\n");
	exit(1);
    }

    vp.left = -1.0;
    vp.right = 1.0;
    vp.top = 0.75;      /* Basic 4/3 aspect ratio, with positive Y UP */
    vp.bottom = -0.75;
    vp.hither = 1.2;
    Matrix_Perspective(appData->skewMatrix, &vp, 0.0, (float)(genv->d->width),
		       0.0, (float)(genv->d->height), 1.0/gHitherPlane);

	Matrix_SetTranslationByVector(&camMtx, camLocation );
	Vector3D_Zero( &lookAtPt );

	Matrix_LookAt(&camMtx, &lookAtPt, camLocation, 0.0);

	M_SetCamera( appData->close, &camMtx, appData->skewMatrix );


}


/* --------------------------------------------------------------------- */


void Program_Begin(GraphicsEnv* genv, AppData* app, uint32 modelPodCount)
{
    ControlPadEventData   cped;
    Pod                   *firstPod;

    /* First setup DBlender so that the clear will also clear Z, etc. */
    GS_Reserve(genv->gs, M_DBInit_Size);
    M_DBInit(GS_Ptr(genv->gs), 0, 0, genv->d->width, genv->d->height);

    /* Clear screen and Z-buffer */
    CLT_ClearFrameBuffer(genv->gs, 0.9, 0.9, 0.92, 1.0, TRUE, TRUE);

    /* Now reset state, since CLT_ClearFB changed the state of the DBlender */
    GS_Reserve(genv->gs, M_DBInit_Size);
    M_DBInit(GS_Ptr(genv->gs), 0, 0, genv->d->width, genv->d->height);
    
    /* Draw the model */
    firstPod = M_Sort(modelPodCount, gBSDF->pods, NULL);
    M_Draw(firstPod, app->close);

    /* Now draw the background.  We could have just appended them together
     * above, but this is how to do it when you need to render multiple
     * pod lists
     */
    M_Draw(&gCornerPod, app->close);

    /* Here's where the cmd list is sent to the Triangle Engine */
    genv->gs->gs_SendList(genv->gs);

    /* Once it's done being rendered, show the bitmap */
    GS_WaitIO(genv->gs);
    ModifyGraphicsItemVA(genv->d->view,
			 VIEWTAG_BITMAP, GS_GetDestBuffer(genv->gs),
			 TAG_END);

    printf("Hit anything on the Control Pad to exit...\n");
    GetControlPad(1, TRUE, &cped);
}

