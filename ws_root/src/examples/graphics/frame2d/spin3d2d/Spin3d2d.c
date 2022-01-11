
/******************************************************************************
**
**  @(#) Spin3d2d.c 96/07/02 1.7
**
******************************************************************************/

/**
|||	AUTODOC -class Examples -group Frame2D -name spin3d2d
|||	Rotate 2d and 3d objects together
|||
|||	  Synopsis
|||
|||	    spin3d2d [InitGP params] file.bsf file.utf
|||
|||	  Description
|||
|||	    Given a binary SDF file and a UTF file, load the SDF object
|||	    and spin it in 3 space, while moving and spinning the UTF texture
|||	    as a sprite.  Also, move the sprite's Z coordinate in and out
|||	    so that it moves through the SDF object.  Control pad actions
|||	    will exit the program.
|||
|||	  Arguments
|||
|||	    [InitGP params]
|||	        -?:    Display help.
|||	        -b:    Enable detination blending.
|||	        -640:  Wide display.
|||	        -480:  Tall display.
|||	        -32:   True color (32 bits per pixel).
|||	        -h:    Hi-res (640x480x32bpp) shortcut.
|||	        -s#:    Use # frame buffers (default = -s2).
|||
|||	    file.bsf
|||	        Name of binary SDF file to load.  The last object defined
|||	        within the world class structure will be used.
|||
|||	    file.utf
|||	        Name of the texture to load into the 2D sprite.
|||
|||	  Associated Files
|||
|||	    spin3d2d.c
|||
|||	  Location
|||
|||	    Examples/Graphics/Frame2d
|||
**/


#include <kernel/task.h>
#include <stdio.h>
#include <graphics/fw.h>
#include <misc/event.h>
#include <graphics/gfxutils/putils.h>
#include <graphics/frame2d/f2d.h>


/* Constants */

#define		kXRotateIncrement	10
#define		kYRotateIncrement	(-5)


/* Globals */

Character*	theScene;
Err			theErr;
GP*			gp;
SDF*		theSDF = NULL;
Camera		*cam;
SpriteObj*	theSprite;
gfloat		zv[] = {.999998, .999998, .999998, .999998};


/* Prototypes */

void InitM2Graphics (int* argc, char* argv[]);
bool SetupScene (Character** theScenePtrPtr,
				char* sdfFilename, char* objectName);
void RotateScene (Character* theScene, SpriteObj *theSprite);
Err SetupSprite (SpriteObj **theSprite, char *filename);


/* Code */

void main (int argc, char* argv[])
{
    InitM2Graphics (&argc, argv);
    if (theSDF) SDF_Close (theSDF);
}


void InitM2Graphics (int* argc, char* argv[])
{
	theErr = 0;

	if (*argc < 3)
	{
	  printf ("Usage: %s [InitGP params] file.bsf file.utf\n", argv[0]);
	  return;
	}

	Gfx_Init();
	gp = InitGP (argc, argv);

	if (SetupScene (&theScene, argv[1], NULL)) {
	  if (SetupSprite (&theSprite, argv[2])>=0) {
	    RotateScene (theScene, theSprite);
	  } else {
	    printf ("Error creating sprite\n");
	  }
	} else {
	  printf ("Error creating scene!\n");
	}
}


bool SetupScene (Character** theScenePtrPtr, char* sdfFilename,
				char* objectName)
{
	Character*	theScene;
	Character*	theObj = NULL;
	Character*	theSDFObj;

	/* Create a scene, into which the objects will be placed. */
	theScene = Scene_Create();
	if (theScene == NULL) goto SetupFailed;
	cam = Scene_GetCamera (theScene);

	/* Load the object(s) from the SDF file. */
	theSDF = SDF_Open (sdfFilename);
	if (theSDF == NULL) goto SetupFailed;

	/* Get the named object from the SDF file. */
	theSDFObj = SDF_FindObj (theSDF, "character", objectName);
	if (theSDFObj == NULL) goto SetupFailed;

	Obj_Assign (&theObj, theSDFObj);
	printf ("SDF finished loading.\n");

	/* Last, add the scene to the group */
	Scene_SetDynamic (theScene, theObj);

	/* Prepare the scene for rendering */
	Scene_ShowAll (theScene);
	Scene_SetVisibility (theScene, TRUE);
	Scene_SetAutoClip (theScene, TRUE);
	Scene_SetAutoAdjust (theScene, TRUE);

	/* Set up the final bits of the pipeline for the framework,	*/
	/* then display the scene.									*/
	GP_SetHiddenSurf (gp, GP_ZBuffer);
	GP_SetCullFaces (gp, GP_Back);
	*theScenePtrPtr = theScene;
	return (1);

SetupFailed:
	return (0);
}


Err SetupSprite (SpriteObj **theSprite, char *filename)
{
  Err err;

  /* Create the sprite */
  *theSprite = Spr_CreateExtended(NULL);

  /* Load a texture file into the sprite */
  err = Spr_LoadUTF(*theSprite, filename);
  if (err<0) return err;

  /* Set the sprite's anchor to the center of the sprite */
  Spr_ResetCorners (*theSprite, SPR_CENTER);

  /* enable Z buffering, disable destination blending when drawing sprite */
  Spr_SetDBlendAttr (*theSprite, DBLA_ZBuffEnable, 1);
  Spr_SetDBlendAttr (*theSprite, DBLA_ZBuffOutEnable, 1);
  Spr_SetDBlendAttr (*theSprite, DBLA_BlendEnable, 0);

  /* Disable discard conditions */
  Spr_SetDBlendAttr (*theSprite, DBLA_Discard, 0);

  /* Enable texture */
  Spr_SetTextureAttr (*theSprite, TXA_TextureEnable, 1);

  /* Set texture to only output texture - no texture blending */
  Spr_SetTextureAttr (*theSprite, TXA_ColorOut, TX_BlendOutSelectTex);

  return 0;
}


void RotateScene (Character* theScene, SpriteObj *theSprite)
{
    Character*			theDynamic;
    ControlPadEventData	cped;
    Point2 p;
    gfloat dx=1., dy=1.;
    GState *gs=GP_GetGState (gp);
    gfloat zvdel = .98;


    p.x = 0.;
    p.y = 0.;

    if (InitEventUtility (1, 0, TRUE) < 0)
	{
		printf("Error in InitEventUtility\n");
		return;
    }

    theDynamic = Scene_GetDynamic (theScene);

    while (1)
	{
		GetControlPad (1, FALSE, &cped);

		/* if any button event, then break */
		if (cped.cped_ButtonBits) break;

		/* Move the model around the screen and spin it */
		Char_Rotate (theDynamic, TRANS_XAxis, kXRotateIncrement);
		Char_Rotate (theDynamic, TRANS_YAxis, kYRotateIncrement);
		Scene_Display (theScene, gp );


		/* Move the sprite around the screen, and spin it as well */
		p.x += dx;
		if ((p.x>=320) || (p.x<0)) {
		  dx = -dx;
		  p.x += dx+dx;
		}
		p.y += dy;
		if ((p.y>=240) || (p.y<0)) {
		  dy = -dy;
		  p.y += dy+dy;
		}
		Spr_SetPosition (theSprite, &p);
		Spr_Rotate (theSprite, 2.);

		zv[0] *= zvdel;
		if ((zv[0]<.01) || (zv[0]>=1.0)) {
		  zvdel = 1./zvdel;
		  zv[0] = zv[1]*zvdel;
		}
		zv[1] = zv[2] = zv[3] = zv[0];
		Spr_SetZValues (theSprite, zv);

		GS_Reserve (gs, 4);	/* Disable perspective */
		CLT_Sync (GS_Ptr(gs));
		CLT_ESCNTL (GS_Ptr(gs), 1, 0, 0);
		F2_Draw (gs, theSprite);
		GS_Reserve (gs, 4);
		CLT_Sync (GS_Ptr(gs));	/* Reenable perspective */
		CLT_ESCNTL (GS_Ptr(gs), 0, 0, 0);

		SwapBuffers();

    }

    KillEventUtility();
    printf ("Done.\n");
}


