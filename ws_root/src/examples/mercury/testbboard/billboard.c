/*
 * @(#) viewer.c 96/05/10 1.14
 *
 * Basic Mercury example.  This example initializes the display, bitmaps, and
 * GState, in preparation for rendering (in graphicsenv.c), creates some
 * simple geometry (in data.c) for two walls and a floor, then reads in a
 * model (in filepod.c), and displays frames of data containing both
 * pieces of data (in viewer.c).  The camera can be moved around in world
 * coordinates, and the Model read in from disk can be rotated and moved 
 * around (in viewer.c).  Hitting Start resets the scene.  Hitting stop quits.
 * The program uses a standard texture reader (in tex_read.c) and bsdf reader
 * (in bsdf_read.c), which can read data from the 3DO graphics conversion
 * tools.
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
#include "controller.h"

/* Carries around all the program's data */
typedef struct AppData {
    CloseData   *close;
    Matrix      *matrixSkew;
    Matrix      *matrixCamera;

    uint32     curRenderScreen;
    float      rx, ry, rz;        /* rotation of object */
    float      tx, ty, tz;        /* location of object */

    CONTROLLER  controls;

} AppData;


/* Mercury limits we have arbitrarily set -- lower if tight on mem */
#define kMaxCmdListWords    400

#define Pi                  	3.141592653589793
#define kRadian             	(Pi/180.0)
#define kIncrement          	(2.5 * kRadian)
#define kMoveIncrement          1.0

#define kTurnCameraIncrement	(2.5f * kRadian)
#define kMoveCameraIncrement    (7.5f)

#define kDefaultCamLocX     0.0
#define kDefaultCamLocY     0.0
#define kDefaultCamLocZ     400.0

/* Prototypes for routines only used in this file */
void ParseArgs(int argc, char* argv[], Vector3D* camLoc);
void Program_Init(GraphicsEnv* genv, AppData** app, Vector3D* camLocation);
void Program_Begin(GraphicsEnv* genv, AppData* app);
static void Program_UserControl(AppData* appData, Matrix* matrixCamera);


Vector3D startCamLocation;


/* --------------------------------------------------------------------- */


int main(int argc, char* argv[])
{
    Err            err;
    GraphicsEnv*   genv;
    AppData*       appData;

    /* Print usage */

    printf("\n");
    printf("Usage: viewer [-c x y z]\n");
    printf("  -c x y z will position the camera in world coords. at x,y,z\n");
    printf("\n");
    printf("Interactive controls:\n");
    printf("   UP:         - Rotate camera up.\n");
    printf("   DOWN:       - Rotate camera down.\n");
    printf("   LEFT:       - Rotate camera left.\n");
    printf("   RIGHT:      - Rotate camera right.\n");
    printf("   RIGHT-SHIFT:- Twist to the right.\n");
    printf("   LEFT-SHIFT: - Twist to the left.\n");
    printf("   A:          - Move towards object.\n");
    printf("   B:          - Move away from object.\n");
    printf("   C:  		   - Look at object and zero twist.\n");
    printf("   RIGHT-SHIFT+C: - Move towards object.\n");
    printf("   RIGHT-SHIFT+C: - Move away from object.\n");
    printf("   PAUSE:      - Reset view to start-up condition.\n");
    printf("   STOP:       - Exit program.\n");
    printf("\n");
    printf("Red = BillboardX\n");
    printf("Green = BillboardY\n");
    printf("Blue = BillboardZ\n");

    ParseArgs(argc, argv, &startCamLocation);

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

    Program_Init(genv, &appData, &startCamLocation);
    Program_Begin(genv, appData);

    /* Cleanup */

    printf("Program exiting.\n");
    GraphicsEnv_Delete(genv);

    return 0;
}


/* --------------------------------------------------------------------- */


void ParseArgs(int argc, char* argv[], Vector3D* camLoc)
{
    int32 i;

    /* Use this location as default camera position */
    camLoc->x = kDefaultCamLocX;
    camLoc->y = kDefaultCamLocY;
    camLoc->z = kDefaultCamLocZ;

    for (i=1; i<argc; i++) {
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


void Program_Init(GraphicsEnv* genv, AppData** app,  Vector3D* camLoc)
{
    ViewPyramid  vp;
    AppData*     appData;
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

    /* Initialize Mercury */
    appData->close = M_Init(4, kMaxCmdListWords, genv->gs);
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
    appData->matrixSkew = AllocMem(sizeof(Matrix), MEMTYPE_NORMAL);
    if (appData->matrixSkew == NULL) {
        printf("Couldn't allocate skew matrix.  Exiting\n");
        exit(1);
    }

    vp.left = -1.0;
    vp.right = 1.0;
    vp.top = 0.75;      /* Basic 4/3 aspect ratio, with positive Y UP */
    vp.bottom = -0.75;
    vp.hither = 1.2;
    Matrix_Perspective(appData->matrixSkew, &vp, 0.0, (float)(genv->d->width),
		       0.0, (float)(genv->d->height), 1.0/10.0);


    /*
     * Additional camera setup
     */
    printf("Additional camera setup...\n");
    appData->matrixCamera = Matrix_Construct();
    if (appData->matrixCamera == NULL) {
        printf("Couldn't allocate camera matrix.  Exiting\n");
        exit(1);
    }
    {
        Vector3D   center;
        Vector3D   position;

        Vector3D_Set(&center, 0.0f, 0.0f, 0.0f);
        Matrix_Identity(appData->matrixCamera);
        Matrix_Translate(appData->matrixCamera,
			 camLoc->x, camLoc->y, camLoc->z  );
        Matrix_GetTranslation(appData->matrixCamera, &position);
        Matrix_LookAt(appData->matrixCamera, (Point3D*)&center,
		      (Point3D*)&position, 0.0f);
    }

    M_SetCamera(appData->close, appData->matrixCamera, appData->matrixSkew);

    /*
     * Setup the controller
     */
    printf("Setup controller...\n");
    Controller_Construct( &appData->controls );

}

/* --------------------------------------------------------------------- */


static void Program_UserControl(AppData* appData, Matrix* matrixCamera)
{

    Vector3D   moveCamera;
    Vector3D   turnCamera;
    Vector3D   move;
    Vector3D   turn;
    Vector3D   center;
    Vector3D   position;
    Vector3D   delta;
    float		length;
    uint32	buttons = appData->controls.currentButtons;

    Vector3D_Zero(&moveCamera);
    Vector3D_Zero(&turnCamera);
    Vector3D_Zero(&move);
    Vector3D_Zero(&turn);
    Vector3D_Zero(&delta);

    /*
     * Turn deltas for camera
     */
    if ( buttons & ControlUp )
        turnCamera.x = kTurnCameraIncrement;
    if ( buttons & ControlDown )
        turnCamera.x = -kTurnCameraIncrement;

    if ( buttons & ControlLeft )
        turnCamera.y = kTurnCameraIncrement;
    if ( buttons & ControlRight )
        turnCamera.y = -kTurnCameraIncrement;

    if ( buttons & ControlA )
        moveCamera.z = -kMoveCameraIncrement;
    if ( buttons & ControlB )
        moveCamera.z = kMoveCameraIncrement;

    if ( buttons & ControlC ) {
    	if ( buttons & ControlLeftShift )
	    moveCamera.z = -kMoveCameraIncrement;
    	if ( buttons & ControlRightShift )
	    moveCamera.z = kMoveCameraIncrement;
    } else {
    	if ( buttons & ControlLeftShift )
	    turnCamera.z = kTurnCameraIncrement;
    	if ( buttons & ControlRightShift )
	    turnCamera.z = -kTurnCameraIncrement;
    }

    /*
     * Update camera position
     */
    Matrix_GetTranslation(matrixCamera, &position);
    Vector3D_Subtract(&delta, &position, &center);
    length = Vector3D_Length( &delta );

    Matrix_Move(matrixCamera, 0.0f, 0.0f, -length );
    Matrix_TurnXLocal(matrixCamera, turnCamera.x);
    Matrix_TurnYLocal(matrixCamera, turnCamera.y);
    Matrix_TurnZLocal(matrixCamera, turnCamera.z);
    Matrix_Move(matrixCamera, 0.0f, 0.0f, length );
    Matrix_MoveByVector(matrixCamera, &moveCamera);

    Matrix_BillboardX(gBillXPod.pmatrix,&position);
    Matrix_BillboardY(gBillYPod.pmatrix,&position);
    Matrix_BillboardZ(gBillZPod.pmatrix,&position);
}


/* --------------------------------------------------------------------- */


void Program_Begin(GraphicsEnv* genv, AppData* app)
{
    bool                  done = FALSE;

    /* First setup DBlender so that the clear will also clear Z, etc. */
    GS_Reserve(genv->gs, M_DBInit_Size);
    M_DBInit(GS_Ptr(genv->gs), 0, 0, genv->d->width, genv->d->height);

    app->curRenderScreen = 0;
    app->rx = app->ry = app->rz = 0.0;
    app->tx = app->ty = app->tz = 0.0;

    do {
	Controller_CollectEvents(&app->controls);

	if (app->controls.currentButtons & ControlX)
	    done = TRUE;

        if ((app->controls.currentButtons & ControlA) &&
	    (app->controls.currentButtons & ControlB)) {
        }

	/*
	 * Update camera/pod for next frame
	 */
	Program_UserControl(app, app->matrixCamera);

	/*
	 * Set the Mercury camera
	 */
	M_SetCamera(app->close, app->matrixCamera, app->matrixSkew);


        /* Clear screen and Z-buffer */
        CLT_ClearFrameBuffer(genv->gs, 0.9, 0.9, 0.92, 1.0, TRUE, TRUE);

	/* Now reset DBlender state, since CLT_ClearFB changed its state */
	GS_Reserve(genv->gs, M_DBInit_Size);
	M_DBInit(GS_Ptr(genv->gs), 0, 0, genv->d->width, genv->d->height);
    
	/* Draw the billboards */
	M_Draw(&gBillXPod, app->close);

	/* Here's where the cmd list is sent to the Triangle Engine */
	genv->gs->gs_SendList(genv->gs);

	/* Once it's done being rendered, show the bitmap */
	GS_WaitIO(genv->gs);
	ModifyGraphicsItemVA(genv->d->view,
			     VIEWTAG_BITMAP, GS_GetDestBuffer(genv->gs),
			     TAG_END);
	app->curRenderScreen = (app->curRenderScreen + 1) % 2;
	GS_SetDestBuffer(genv->gs, genv->bitmaps[app->curRenderScreen]);
	GS_BeginFrame(genv->gs);
    } while (!done);
}

