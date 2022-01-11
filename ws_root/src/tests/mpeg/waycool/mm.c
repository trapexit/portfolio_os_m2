/*
 *	mm.c
 *  multiple-stream morphing mpeg demo
 *	Copyright 1994, The 3DO Company
 */

#define	NUM_BLOCKS	    15
#define	TX_XSIZE		320
#define	TX_YSIZE		16
#define HSEGMENTS	    32
#define	VSEGMENTS	    2
#define MPEG_DEPTH      (16)
#define	TX_DEPTH	    (16)
#define	TX_PER_WORD	    (32 / TX_DEPTH)
#define	BYTE_PER_TX	    (TX_DEPTH / 8)


#define	USEMPEG
#define kMaxStreams 5
#define kDefaultMaxStreams 3
#define kMaxObjects 10
#define kDefaultMaxObjects 3
/* #define NOSTOP */

/* #define ADJUST_FS */

#define ZPOS_START	-2.0462

#define NUMMORPHSTEPS 30
#define	XROT_START	0.0
#define YROT_START	0.0
#define	ZROT_START  180.0

#define	XSPIN_START	0.0
#define YSPIN_START	0.0
#define	ZSPIN_START	0.0
#define WOBBLE_START 1.0

#define ZPOS_MIN    -9.0
#define ZPOS_MAX    -4.0

#define WOBBLE_MIN  -2.0
#define WOBBLE_MAX  2.0

#define ROTATEINCR  0.3
#define ZPOSINCR 	0.05
#define WOBBLEINCR  0.04

#define MAX_BFXPOS  320
#define MAX_BFYPOS  240

#ifdef TIMER
#define initTime() SampleSystemTimeTV(&tv); time2 = tv.tv_sec * 1000000 + tv.tv_usec; time1 = time2;
#define getTime(t) SampleSystemTimeTV(&tv); time2 = tv.tv_sec * 1000000 + tv.tv_usec; t = time2 - time1; time1 = time2;
#define addTime(t) SampleSystemTimeTV(&tv); time2 = tv.tv_sec * 1000000 + tv.tv_usec; t += time2 - time1; time1 = time2;
#define markTime(a)  _dcbf((void *)(0x40000000 | a)); dummy = *(volatile uint8*)(0x40000000 | a);
#else
#define initTime() /* */
#define getTime(t) /* */
#define addTime(t) /* */
#define markTime(a) /* */
#endif

#include "mm.h"
#include "plaympeg.h"
#include "myputils.h"
#include "hardware/PPCasm.h"

int32 str2num(char *s);
void usage(char *pn, int32 err);

Item			timerIOReqItem;
int32			displayDepth;
uint32          gNumScreens;

main(int argc, char **argv)
{
	Color4			blue, white;
	GP				*gp;			/* -> current graphics pipeline */
	AppInitStruct   appData;
	Transform		*mv, *pers;			/* -> current model/view matrix */
    Bitmap          *currDestBuffer;
	LightProp		light;
	uint32          numObjects, numStreams;
	uint32          maxObjects, maxStreams;
	ObjDescr        *obj[kMaxObjects], *fromObj[kMaxObjects], *toObj[kMaxObjects], morphObj[kMaxObjects];
	ObjDescr		planeObj, tubeObj, qpObj, sphereObj, torusObj;
	TexDescr        *tex[kMaxStreams], opaqueTex[kMaxStreams], transparentTex[kMaxStreams];
	MPEGBuf			mpg[kMaxStreams];
	mpegStream      *stream[kMaxStreams];

	uint32          morphStep = 0;
	uint32          morphing = false, keepMorphing = false;
	uint32          fullScreen = true;
	uint32          zoomOut = false;
	uint32          pauseMPEG = false;
	int32           i;
	uint32          backFrameOn = false;

	ControlPadEventData cped;
	uint32 			Buttons;
	gfloat  		xRot = XROT_START, yRot = YROT_START, zRot = ZROT_START;
	gfloat  		xRotInc = 0.0, yRotInc = 0.0, zRotInc = 0.0;
	gfloat  		zPos = ZPOS_START, wobble = WOBBLE_START;
	gfloat			xSpin = XSPIN_START, ySpin = YSPIN_START, zSpin = ZSPIN_START;
	gfloat			xSpinInc = 0.0, ySpinInc = 0.0, zSpinInc = 0.0;
	Point3  		ztrans;

	gfloat          backFrameXPos = 0.0, backFrameYPos = 0.0;
	gfloat          backFrameXPosInc = 3.0, backFrameYPosInc = 3.0;
	uint32          debounceStart = 0, debounceA = 0, debounceStop = 0;
#ifdef ADJUST_FS
	uint32          debounceCU = 0, debounceCD = 0, debounceCR = 0, debounceCL = 0;
	gfloat          zAdjust = 0.001;
#endif

#ifdef TIMER
	TimeVal			tv;
	uint32			time1, time2;
	uint32			mpegTime, renderTime, morphTime, flushTime, teTime, viewTime, otherTime;
	volatile uint8  dummy;
	int32			frameCount;
	uint32			checkTime;
#endif

#ifdef RATELIMIT
	TimeValVBL      firstVBL, streamVBL, thisVBL;
	uint32          frameNumber, framesPerSecond = 60;
	uint32          firstFrame = 1;
	uint32			timeDisplay = 0;
#endif

	Err				err;
	char            *fn[kMaxStreams];
	uint32          fnCnt;
	uint32          transparent = 0;
	char            *cmdName;

	/* Startup */
	Gfx_PipeInit();

	/*   what the AppInitStruct looks like
	typedef struct {
		uint32		numScreens;
		uint32		screenWidth;
		uint32		screenHeight;
		uint32		screenDepth;
		uint32		numCmdListBufs;
		uint32		cmdListBufSize;			 in bytes 
		bool		autoTransparency;
		bool		enableDither;
		bool		enableVidInterp;
	} AppInitStruct;
	*/

	GfxUtil_ParseArgs(&argc, argv, &appData);

	appData.numScreens = 2;
	appData.numCmdListBufs = 2;
	appData.cmdListBufSize = 96 * 1024;

	gp = GfxUtil_SetupGraphics(&appData);

	displayDepth = appData.screenDepth;
	gNumScreens = appData.numScreens;
	
	printf("numCmdListBufs = %d, cmdListBufSize = %d\n", appData.numCmdListBufs, appData.cmdListBufSize);
	printf("currDestBuffer = %08lx\n", GP_GetDestBuffer(gp));

	/* process additional arguments */
	
	fn[0] = "mpv.date.short";
	fn[1] = "mpv.boss.short";
	fn[2] = "mpv.m2car.short";
	fnCnt = 0;
	cmdName = *argv;

	maxObjects = kDefaultMaxObjects;
	maxStreams = kDefaultMaxStreams;

	while (--argc) 
	{
		++argv;
		if ((*argv)[0] == '-')
		{
			if ((*argv)[1] == 'r')
			{
				if ((*argv)[2] != 0)
				{
#ifdef RATELIMIT
					framesPerSecond = str2num(*argv + 2);
#else
					printf("rate limiting not enabled!!!!!!!\n");
					usage(cmdName, -1);
#endif
				}
				else
				{
					if (--argc)
					{
#ifdef RATELIMIT
						++argv;
						framesPerSecond = str2num(*argv);
#else
					printf("rate limiting not enabled!!!!!!!\n");
					usage(cmdName, -1);
#endif
					}
					else
					{
						printf("insufficient arguments\n");
						usage(cmdName, -1);
					}
				}
			}
			else if ((*argv)[1] == 'o')
			{
				if ((*argv)[2] != 0)
				{
					maxObjects = str2num(*argv + 2);
				}
				else
				{
					if (--argc)
					{
						++argv;
						maxObjects = str2num(*argv);
					}
					else
					{
						printf("insufficient arguments\n");
						usage(cmdName, -1);
					}
				}
				if ((maxObjects < 1) || (maxObjects > kMaxObjects))
				{
					printf("object count out of range\n");
					usage(cmdName, -1);
				}
			}
			else if ((*argv)[1] == 'm')
			{
				if ((*argv)[2] != 0)
				{
					maxStreams = str2num(*argv + 2);
				}
				else
				{
					if (--argc)
					{
						++argv;
						maxStreams = str2num(*argv);
					}
					else
					{
						printf("insufficient arguments\n");
						usage(cmdName, -1);
					}
				}
				if ((maxStreams < 1) || (maxStreams > kMaxStreams))
				{
					printf("stream count out of range\n");
					usage(cmdName, -1);
				}
			}
			else 
			{
				printf("unknown option -%s\n", (*argv)[2]);
				usage(cmdName, -1);
			}	
		}
		else
		{
			if (fnCnt > maxStreams)
			{
				printf("whoa! I can only handle %d streams right now\n", maxStreams);
				usage(cmdName, -1);
			}
			fn[fnCnt++] = *argv;
		}
	}

	printf("taking mpeg streams from:");
	for (i=0; i<maxStreams; i++) printf(" %s", fn[i]);
	printf("\n");

#ifdef RATELIMIT
	printf("limiting rate to %d frames per second\n", framesPerSecond);
#endif	
	/* Init MPEG Buffers */
	for (i=0; i<maxStreams; i++)
	{
		printf("playing stream %d from file %s\n", i, fn[i]);

		err = getMPEGBuf(&mpg[i], TX_DEPTH);
		if (err < 0) return(err);

		stream[i] = malloc(sizeof(mpegStream));
		if (stream[i] == NULL)
		{
			printf("couldn't allocate mpeg stream structure %d\n", i);
			exit(-1);
		}

		err = initMPEG(stream[i], fn[i], &mpg[i]);
		if (err < 0) return(err);

		printf("creating opaque texture for stream %d\n", i);
		err = texCreateDescr(&opaqueTex[i], mpg[i].mpegPadded, NUM_BLOCKS, TX_XSIZE, TX_YSIZE, false, TX_DEPTH);
		if (err < 0) return err;	
		printf("creating transparent texture for stream %d\n", i);
		err = texCreateDescr(&transparentTex[i], mpg[i].mpegPadded, NUM_BLOCKS, TX_XSIZE, TX_YSIZE, true, TX_DEPTH);
		if (err < 0) return err;	
		tex[i] = &opaqueTex[i];
	}
	numStreams = 1;

	printf("initMPEG complete\n");

	/* Create objects */

	printf("creating objects\n");

	for (i=0; i<maxObjects; i++)
	{
		printf("creating morph object %d\n", i);
		err = initMorph(&morphObj[i], NUM_BLOCKS, HSEGMENTS, VSEGMENTS, TX_XSIZE, TX_YSIZE, TX_DEPTH);
		if (err < 0) return err;	
	}

	printf("creating sphere\n");
	err = createSphere(&sphereObj, NUM_BLOCKS, HSEGMENTS, VSEGMENTS, TX_XSIZE, TX_YSIZE, TX_DEPTH);
	if (err < 0) return err;
	printf("creating plane\n");
	err = createPlane(&planeObj, NUM_BLOCKS, HSEGMENTS, VSEGMENTS, TX_XSIZE, TX_YSIZE, TX_DEPTH);
	if (err < 0) return err;
	printf("creating tube\n");
	err = createTube(&tubeObj, NUM_BLOCKS, HSEGMENTS, VSEGMENTS, TX_XSIZE, TX_YSIZE, TX_DEPTH);
	if (err < 0) return err;
	printf("creating quick plane\n");
	err = createPlane(&qpObj, NUM_BLOCKS, 1, VSEGMENTS, TX_XSIZE, TX_YSIZE, TX_DEPTH);
	if (err < 0) return err;
	printf("creating torus\n");
	err = createTorus(&torusObj, NUM_BLOCKS, HSEGMENTS, VSEGMENTS, TX_XSIZE, TX_YSIZE, TX_DEPTH);
	if (err < 0) return err;

	/* link objects together to define morphing order */
	qpObj.nextObject = &tubeObj;
	planeObj.nextObject = &tubeObj;
	tubeObj.nextObject = &sphereObj;
	sphereObj.nextObject = &torusObj;
	torusObj.nextObject = &planeObj;

	/* initialize the object pointers */
	numObjects = 1;
	obj[0] = &qpObj;
	fromObj[0] = obj[0];
	toObj[0] = obj[0]->nextObject;
	for (i=1; i<maxObjects; i++)
	{
		obj[i] = obj[i-1]->nextObject;
		fromObj[i] = obj[i];
		toObj[i] = obj[i]->nextObject;
	}


	/* Initialize the EventBroker. */

	printf("initializing event broker\n");

	err = InitEventUtility(1, 0, LC_ISFOCUSED);
	if (err < 0)
	{
		PrintError(0,"InitEventUtility",0,err);
		return err;
	}
	
	/* Initialize Timer */

	timerIOReqItem = CreateTimerIOReq();
	
	/* set up graphics */

	printf("setting up graphics\n");

	NextRendBuffer();

	/* enable lighting and set up light source */
	GP_Enable(gp, GP_Lighting);
	light.Kind = LIGHT_Directional;
	Vec3_Set(&light.Position, 1.0, 1.0, 1.0);
	Vec3_Set(&light.Direction, 1.0, 1.0, 1.0);
	Vec3_Normalize(&light.Direction);
	Col_Set(&light.Color, 1.0, 1.0, 1.0, 1.0);
	light.Angle = 0.0;
	light.FallOff = 0.0;
	light.Enabled = TRUE; 
	GP_SetLight(gp, 0, &light);

	/* set background color */
	Col_Set(&blue, 0.0, 0.2, 0.2, 1.0);
	GP_SetBackColor(gp, &blue);

	/* enable texturing */
	GP_Enable(gp, GP_Texturing);

	/* enable dithering if in 16-bit mode */
	if (displayDepth < 32) GP_Enable(gp, GP_Dithering);
	else GP_Disable(gp, GP_Dithering);

	Col_Set(&white, 1.0, 1.0, 1.0, 1.0);
	GP_SetAmbient(gp, &white);
	printf("ambient set\n");

	/* get model/view matrix */
	mv = GP_GetModelView(gp);			
	pers = GP_GetProjection(gp);
	
    /* set up the perspective matrix */
	Trans_Perspective(pers, 60, 1.33333333, 1.0, 10.0); 
	printf("perspective matrix set\n");

	GP_Clear(gp, GP_ClearAll);
	
	WaitTE();
	IncCurBuffer();
	NextViewBuffer();
	GP_Flush(gp);

#ifdef TIMER
	initTime(); 
	checkTime = 0;
	frameCount = 0;
#endif

	/******************************************* main loop *****************************************************/

	printf("entering infinite loop...\n");

	while(1) {		

		markTime(0); 							/* for debugging on logic analyzer */

		NextRendBuffer();						/* set buffer for next frame */

		currDestBuffer = GP_GetDestBuffer(gp);

		/* process control pad information */

		err = GetControlPad (1, FALSE, &cped);
		if (err < 0) {
			PrintError(0,"read control pad in","diagnostic",err);
		}
		Buttons = cped.cped_ButtonBits;
		
		if (fullScreen)
		{
			if( Buttons & ControlA ) 
			{
				fullScreen = false;
				zoomOut = true;
				obj[0] = &planeObj;
			}
#ifdef ADJUST_FS
			if( Buttons & ControlDown ) 
			{
				if (!debounceCD)
				{
					debounceCD = 1;
					zPos -= zAdjust;
					printf("zPos = %g\n", zPos);
				}
			}
			else
				debounceCD = 0;
			
			if( Buttons & ControlUp ) 
			{
				if (!debounceCU)
				{
					debounceCU = 1;
					zPos += zAdjust;
					printf("zPos = %g\n", zPos);
				}
			}
			else
				debounceCU = 0;

				
			if ( Buttons & ControlLeft )
			{
				if (!debounceCL)
				{
					debounceCL = 1;
					zAdjust *= 10.0;
					printf("zAdjust = %g\n", zAdjust);
				}
			}
			else
				debounceCL = 0;

			if ( Buttons & ControlRight )
			{
				if (!debounceCR)
				{
					debounceCR = 1;
					zAdjust /= 10.0;
					printf("zAdjust = %g\n", zAdjust);
				}
			}
			else
				debounceCR = 0;
#endif
		}
		else if (zoomOut)
		{
			zPos -= ZPOSINCR;
			if (zPos < ZPOS_MAX)
			{
				zoomOut = false;
			}
		}
		else
		{
			if( Buttons & ControlRightShift ) 
			{
				if( Buttons & ControlLeft ) 			ySpinInc += ROTATEINCR;
				if( Buttons & ControlRight )			ySpinInc -= ROTATEINCR;
				if( Buttons & ControlUp ) 
				{
					if(Buttons & ControlLeftShift)  	zSpinInc -= ROTATEINCR;
					else 								xSpinInc += ROTATEINCR;
				}
				if( Buttons & ControlDown ) 
				{
					if(Buttons & ControlLeftShift)  	zSpinInc += ROTATEINCR;
					else								xSpinInc -= ROTATEINCR;
				}
				if( Buttons & ControlC ) 				wobble += WOBBLEINCR;
				if( Buttons & ControlB ) 				wobble -= WOBBLEINCR;
			} 
			else 
			{
				if( Buttons & ControlLeft ) 			yRotInc += ROTATEINCR;
				if( Buttons & ControlRight )			yRotInc -= ROTATEINCR;
				if( Buttons & ControlUp ) 
				{
					if(Buttons & ControlLeftShift)  	zRotInc -= ROTATEINCR;
					else 								xRotInc += ROTATEINCR;
				}
				if( Buttons & ControlDown ) 
				{
					if(Buttons & ControlLeftShift)  	zRotInc += ROTATEINCR;
					else								xRotInc -= ROTATEINCR;
				}
				if( Buttons & ControlC ) 				zPos += ZPOSINCR;
				if( Buttons & ControlB ) 				zPos -= ZPOSINCR;
			}
			
			if( Buttons & ControlA ) 
			{
				if (!debounceA)
				{
					debounceA = 1;
					
					if ( (Buttons & ControlLeftShift) && (Buttons & ControlRightShift)) 
					{
						switch(tex[i]->lerpMode) 
						{
						    case LERP_TEX :
								tex[i]->lerpMode = LERP_PRIM;
								setLerpMode(tex[i]);
								printf("LERP set to PRIM output\n");
								break;
							case LERP_PRIM :
								tex[i]->lerpMode = LERP_BLEND;
								setLerpMode(tex[i]);
								printf("LERP set to BLEND output\n");
								break;
							case LERP_BLEND :
								tex[i]->lerpMode = LERP_TEX;
								setLerpMode(tex[i]);
								printf("LERP set to TEX output\n");
								break;
							default:
								printf("lerpMode %d invalid\n", tex[i]->lerpMode);
								tex[i]->lerpMode = LERP_BLEND;
								setLerpMode(tex[i]);
								break;
						}
					}
					else if (Buttons & ControlRightShift)
					{
						keepMorphing = true;
						if (!morphing)
						{
							morphing = true;
							for (i=0; i<numObjects; i++)
							{
								fromObj[i] = obj[i];
								toObj[i] = obj[i]->nextObject;
							}
						}
					}
					else if (Buttons & ControlLeftShift)
					{
						if (transparent = !transparent)
						{
							for (i=0; i<maxStreams; i++)
								tex[i] = &transparentTex[i];
						}
						else
						{
							for (i=0; i<maxStreams; i++)
								tex[i] = &opaqueTex[i];
						}
						for (i=0; i<maxStreams; i++)
							setLerpMode(tex[i]);
					}
					else
					{
						keepMorphing = false;
						if (!morphing)
						{
							morphing = true;
							for (i=0; i<numObjects; i++)
							{
								fromObj[i] = obj[i];
								toObj[i] = obj[i]->nextObject;
							}
						}
					}
				}
			}
			else 
				debounceA = 0;
			
			if( Buttons & ControlStart ) 
			{
				if (!debounceStart)
				{
					debounceStart = 1;
					
					if ( (Buttons & ControlLeftShift) && (Buttons & ControlRightShift)) 
					{
						xRot = XROT_START;
						yRot = YROT_START;
						zRot = ZROT_START;
						zPos = ZPOS_START;
						xSpin = XSPIN_START;	
						ySpin = YSPIN_START;	
						zSpin = ZSPIN_START;	
						wobble = WOBBLE_START;
						fullScreen = true;
						backFrameOn = false;
						morphing = false;
						morphStep = 0;
						xSpinInc = 0.0;	
						ySpinInc = 0.0;	
						zSpinInc = 0.0;	
						
						xRotInc = 0.0;
						yRotInc = 0.0;
						zRotInc = 0.0;

						transparent = false;
						for (i=0; i<maxStreams; i++)
							tex[i] = &opaqueTex[i];
						numObjects = 1;
						numStreams = 1;
						obj[0] = &qpObj;
						fromObj[0] = obj[0];
						toObj[0] = obj[0]->nextObject;
						for (i=1; i<maxObjects; i++)
						{
							obj[i] = obj[i-1]->nextObject;
							fromObj[i] = obj[i];
							toObj[i] = obj[i]->nextObject;
						}

					}
					else if (Buttons & ControlRightShift)
					{
						if (++numObjects > maxObjects) numObjects = maxObjects;
						if (++numStreams > maxStreams) numStreams = maxStreams;
					}
					else if (Buttons & ControlLeftShift)
					{
						if (--numObjects < 1) numObjects = 1;
						if (--numStreams < 1) numStreams = 1;
					}
					else
					{
						xRotInc = 0.0;
						yRotInc = 0.0;
						zRotInc = 0.0;
						xSpinInc = 0.0;	
						ySpinInc = 0.0;	
						zSpinInc = 0.0;	
					}
				}
			}
			else
				debounceStart = 0;
		}

		if (Buttons & ControlX ) 
		{
			if (!debounceStop)
			{
				debounceStop = 1;
					
				if( Buttons & ControlRightShift ) 
				{
#ifdef TIMER
					if (!timeDisplay) timeDisplay = 1;
					else timeDisplay = 0;
					printf("Frame Display = %d\n", timeDisplay);
#endif
				}
				else if( Buttons & ControlLeftShift ) 
				{
					pauseMPEG = !pauseMPEG;
				}
#ifndef NOSTOP
				else
				{
					break;
				}
#endif
			}
		}
			else
				debounceStop = 0;
		
		xRot += xRotInc;
		yRot += yRotInc;
		zRot += zRotInc;
			
		xSpin += xSpinInc;
		ySpin += ySpinInc;
		zSpin += zSpinInc;
		
		while (xRot > 360.0) xRot -= 360.0;
		while (yRot > 360.0) yRot -= 360.0;
		while (zRot > 360.0) zRot -= 360.0;
  
		while (xSpin > 360.0) xSpin -= 360.0;
		while (ySpin > 360.0) ySpin -= 360.0;
		while (zSpin > 360.0) zSpin -= 360.0;
  
		while (xRot < 0.0) xRot += 360.0;
		while (yRot < 0.0) yRot += 360.0;
		while (zRot < 0.0) zRot += 360.0;
  
		while (xSpin < 0.0) xSpin += 360.0;
		while (ySpin < 0.0) ySpin += 360.0;
		while (zSpin < 0.0) zSpin += 360.0;
  
		if (wobble < WOBBLE_MIN) wobble = WOBBLE_MIN;
		if (wobble > WOBBLE_MAX) wobble = WOBBLE_MAX;

		if( !fullScreen && !zoomOut)
		{
			if (zPos < ZPOS_MIN) zPos = ZPOS_MIN;
			if (zPos > ZPOS_MAX) zPos = ZPOS_MAX;
		}

		getTime(otherTime);

#ifdef USEMPEG
		for (i=0; i<numStreams; i++) feedBitstream(stream[i]);
		getTime(mpegTime);
#endif

#ifdef RATELIMIT
		/* limit frame rate */
		
		if (firstFrame)
		{
			firstFrame = 0;
			frameNumber = 0;
			SampleSystemTimeVBL(&firstVBL);
		}
		else
		{
			streamVBL = (uint64) (++frameNumber * 60 / framesPerSecond);
			do
			{
				SampleSystemTimeVBL(&thisVBL);
				thisVBL -= firstVBL;
				
				if (thisVBL < streamVBL)
					WaitTimeVBL(timerIOReqItem, 1);
			} while (thisVBL < streamVBL);
		}
#endif

		GP_SetHiddenSurf(gp, GP_ZBuffer);
		GP_Clear(gp, GP_ClearAll); 

		if (backFrameOn)
		{
#ifdef BACKWINDOW
			GfxUtil_ClearScreenToBitmap(gp, mpg[0].bitmapPtr, false, backFrameXPos, backFrameYPos);
#endif
			backFrameXPos += backFrameXPosInc;
			if (backFrameXPos > MAX_BFXPOS)
			{
				backFrameXPos = MAX_BFXPOS;
				backFrameXPosInc = -backFrameXPosInc;
			}
			
			if (backFrameXPos < 0.0)
			{
				backFrameXPos = 0.0;
				backFrameXPosInc = -backFrameXPosInc;
			}
			
			backFrameYPos += backFrameYPosInc;
			if (backFrameYPos > MAX_BFYPOS)
			{
				backFrameYPos = MAX_BFYPOS;
				backFrameYPosInc = -backFrameYPosInc;
			}

			if (backFrameYPos < 0.0)
			{
				backFrameYPos = 0.0;
				backFrameYPosInc = -backFrameYPosInc;
			}
		}

		for (i=0; i<tex[0]->num_blocks; i++)
		{
			Txb_SetDblSrcBaseAddr(tex[0]->txb[i], (int32) (currDestBuffer->bm_OriginPtr));
		}

		if (numObjects > 1)
		{
			GP_SetHiddenSurf(gp, GP_ZBuffer);
		}

		Trans_Identity(mv);
		Trans_Rotate(mv, TRANS_XAxis, xRot);
		Trans_Rotate(mv, TRANS_YAxis, yRot); 
		Trans_Rotate(mv, TRANS_ZAxis, zRot);

		Pt3_Set(&ztrans, 0, 0, wobble);		
		Trans_Translate(mv, &ztrans);
	
		Trans_Rotate(mv, TRANS_XAxis, xSpin);
		Trans_Rotate(mv, TRANS_YAxis, ySpin); 
		Trans_Rotate(mv, TRANS_ZAxis, zSpin);

		Pt3_Set(&ztrans, 0, 0, zPos);
		Trans_Translate(mv, &ztrans);

		addTime(otherTime);

		for(i=0; i<numObjects; i++)
		{
			if (transparent)
			{
				GP_SetHiddenSurf(gp, GP_None);
				GP_SetCullFaces(gp, GP_None);
			}
			else
			{
				GP_SetHiddenSurf(gp, obj[i]->hiddenSurf);
				GP_SetCullFaces(gp, obj[i]->cullFaces);
			}

			if (i) Trans_Rotate(mv, TRANS_ZAxis, 360.0 / (gfloat) numObjects);
			drawObject(gp, obj[i], tex[i % maxStreams], &obj[i]->material);
		}

		getTime(renderTime);

#ifdef USEMPEG
		for (i=0; i<numStreams; i++) feedBitstream(stream[i]);
		addTime(mpegTime);
#endif

		if (morphing)
		{
			if (morphStep == NUMMORPHSTEPS)
			{
				if (keepMorphing)
				{
					for (i=0; i<numObjects; i++)
					{
						fromObj[i] = toObj[i];
						toObj[i] = fromObj[i]->nextObject;
						morphStep = 0;
					}
				}
				else
				{
					for (i=0; i<numObjects; i++)
						obj[i] = toObj[i];
					morphing = false;
					morphStep = 0;
				}
			}
			else
			{
				for (i=0; i<numObjects; i++)
				{
					morphObject(fromObj[i], toObj[i], &morphObj[i], NUMMORPHSTEPS, morphStep);
					obj[i] = &morphObj[i];
				}
				morphStep++;
			}
		}

		getTime(morphTime);

#ifdef USEMPEG
		if (!pauseMPEG) 
		{
			for (i=0; i<numStreams; i++)
				if (decodeComplete(stream[i]))
				{
					decodeFrame(stream[i], &mpg[i].bitmapItem);
					feedBitstream(stream[i]);
				}
		}
		for (i=0; i<numStreams; i++) feedBitstream(stream[i]);
		addTime(mpegTime);
#endif

		WaitTE();

		markTime(1);

		getTime(teTime);
		
		IncCurBuffer();
		NextViewBuffer();

		markTime(2);

		getTime(viewTime);
		
#ifdef USEMPEG
		for (i=0; i<numStreams; i++) feedBitstream(stream[i]);
		addTime(mpegTime);
#endif

		GP_Flush(gp);

		markTime(3);

		getTime(flushTime);
		
#ifdef USEMPEG
		for (i=0; i<numStreams; i++) feedBitstream(stream[i]);
		addTime(mpegTime);
#endif
		addTime(checkTime);

#ifdef TIMER
		frameCount++;
		if (checkTime >= 10000000) {
			if (timeDisplay) {
				printf("Frame Rate : %lf frames / sec\n",
					((gfloat) (frameCount * 1000000)) / ((gfloat) checkTime)
					);
			    printf("MPEG %d, MORPH %d, RENDER %d, TE %d, VIEW %d, FLUSH %d, OTHER %d\n",
					mpegTime, morphTime, renderTime, teTime, viewTime, flushTime, otherTime );
			}
			checkTime = 0;
			frameCount = 0;
		}
#endif
	}

#ifdef NEVER     
	/* seems to make things worse.  I dunno. */

	/* free up allocated memory */
	destroyObject(&qpObj);
	destroyObject(&planeObj);
	destroyObject(&tubeObj);
	destroyObject(&sphereObj);
	destroyObject(&torusObj);
	for (i=0; i<maxObjects; i++) destroyObject(&morphObj[i]);

	for (i=0; i<maxStreams; i++)
	{
		destroyTexture(&opaqueTex[i]);
		destroyTexture(&transparentTex[i]);
		destroyMPEG(&mpg[i]);
	}

	GP_Delete(gp);

	DeleteIOReq(timerIOReqItem);
#endif	
	printf("Goodbye ...\n");
	return(0);
}

void usage(char *pn, int32 err)
{
	printf("Usage: %s [gpipe opts] [-r <fps>] [-o <max_objects>] [-m <max_videos>] [<stream0> [<stream1> [<stream2>]]]\n", pn);
	exit(err);
}

Err
objCreateAlloc(ObjDescr *obj)
{
/*    printf("......num_blocks = %d, vsegments = %d, num_verts = %d\n", obj->num_blocks, obj->vsegments, obj->num_verts); */
	obj->vtxData = (Point3 *)
		malloc(obj->num_blocks * obj->vsegments * obj->num_verts * sizeof(Point3));
	obj->normData = (Vector3 *)
		malloc(obj->num_blocks * obj->vsegments * obj->num_verts * sizeof(Vector3));
	obj->texData = (TexCoord *)
		malloc(obj->vsegments * obj->num_verts * sizeof(TexCoord));

	if (obj->vtxData == NULL) {
		printf("objCreateAlloc() : Failed to allocate data structure for vtxData\n");
		return(APP_MEM_ERR);
	}

	if (obj->normData == NULL) {
		printf("objCreateAlloc() : Failed to allocate data structure for normData\n");
		return(APP_MEM_ERR);
	}

	if (obj->texData == NULL) {
		printf("objCreateAlloc() : Failed to allocate data structure for texData\n");
		return(APP_MEM_ERR);
	}
	return(APP_OK);
}

Err
objCreateAddSurface(ObjDescr *obj)
{
	Point3		*vptr;
	TexCoord	*tptr;
	Vector3		*nptr;
	TriStrip	ts;
	uint32		subtx, vseg, si, x;

/*	printf("......creating surface\n"); */
	obj->surf = Surf_Create();
/*	printf("......creating geo data\n"); */
	Geo_CreateData(&ts, GEO_TriStrip, GEO_Normals | GEO_TexCoords, obj->num_verts);
	vptr = obj->vtxData;
	nptr = obj->normData;
	si = 0;
/*	printf("......adding geometries"); */
	for(subtx = 0; subtx < obj->num_blocks; subtx++) {
		tptr = obj->texData;
		for(vseg = 0; vseg < obj->vsegments; vseg++) {
			for (x = 0; x < obj->num_verts; x++) {
				ts.Locations[x] = *vptr++;
				ts.Normals[x] = *nptr++;
				ts.TexCoords[x] = *tptr++;
			}
/*			printf("^"); */
			Surf_AddGeometry(obj->surf, &ts, subtx, 0);
/*			printf("."); */
/*			Geo_Print(&ts);*/
			si++;
		}
	}
/*	printf("\n......deleting geo data\n"); */

	Geo_DeleteData(&ts);

/*	printf("......done\n"); */

	return(APP_OK);
}

Err
drawObject(GP *gp, ObjDescr *obj, TexDescr *td, MatProp *mat)
{
	Surf_Display(obj->surf, gp, td->txb, mat);
	return(APP_OK);
}

void
setLerpMode(TexDescr *tex)
{
	uint32		subtx;

	for (subtx = 0; subtx < tex->num_blocks; subtx++) {
		switch(tex->lerpMode) {
			case LERP_TEX :
				Txb_SetTxColorOut(tex->txb[subtx], TX_BlendOutSelectTex);
				break;
			case LERP_PRIM :
				Txb_SetTxColorOut(tex->txb[subtx], TX_BlendOutSelectPrim);
				break;
			case LERP_BLEND :
				Txb_SetTxColorOut(tex->txb[subtx], TX_BlendOutSelectBlend);
				break;
		}
	}
}

void
destroyObject(ObjDescr *obj)
{
	Surf_Delete(obj->surf);

	free((char *) obj->vtxData);
	free((char *) obj->normData);
	free((char *) obj->texData);
}

void
destroyTexture(TexDescr *tex)
{
	uint32 i;

	for (i = 0; i < tex->num_blocks; i++) 
	{
		Tex_Delete(tex->tex[i]);
		Txb_Delete(tex->txb[i]);
	}

	free((char *) tex->txDataOff);
	free((char *) tex->txb);
	free((char *) tex->tex);
	free((char *) tex->tl);
}

Err
initMPEG(mpegStream *stream, char *fn, MPEGBuf *mpg)
{
#ifdef USEMPEG
 	if (init_mpeg(stream, fn, MPEG_DEPTH)) exit(1);
	printf("init_mpeg complete \n");

	decodeFrame(stream, &mpg->bitmapItem);

	printf("decode started\n");
	while(!decodeComplete(stream)) feedBitstream(stream);
#endif
	return(APP_OK);
}

void
destroyMPEG(MPEGBuf *mpg)
{
	free((char *)mpg->mpegBase);
	DeleteItem(mpg->bitmapItem);
}

int32 str2num(char *s)
{
	char c;
	int32 num = 0;
	do
	{
		c = *s++;
		if ((c < '0') || (c > '9'))
		{
			c = 0;
		}
		else
		{
			num = num * 10 + ((int32) (c - '0'));
		}
	} while (c != 0);
	return (num);
}

Err
texCreateDescr(TexDescr *tdp, uint32 *tmem, uint32 num_blocks, uint32 tx_xsize, uint32 tx_ysize, uint32 transparent, uint32 tx_depth)
{
	Err			err;
	uint32		subtx;
	uint32		*taddr;
	uint32      num_txwords;
/*
	uint32		dithA = 0x80A24C6EL;
	uint32		dithB = 0xB3917F5DL;
	uint32		dithA = 0x0;
	uint32		dithB = 0x0;
*/
	uint32		dithA = 0xc0d12e3fL;
	uint32		dithB = 0xe1d0302fL;

	uint32      tx_per_word;

	PipColor    pcolor = 0x80808080;

	tx_per_word = 32 / tx_depth;

/*	printf("...allocating texture data structures\n"); */

	tdp->txDataOff = (uint32 *) malloc(num_blocks * sizeof(uint32));
	tdp->txb = (TexBlend **) malloc(num_blocks * sizeof(TexBlend *));
	tdp->tex = (Texture **) malloc(num_blocks * sizeof(Texture *));
	tdp->tl = (TexLOD *) malloc(num_blocks * sizeof(TexLOD));
	
	num_txwords = (tx_xsize * (tx_ysize + 3) / tx_per_word);

	if (tdp->txDataOff == NULL) {
		printf("texCreateDescr() : Failed to allocate data structure for txDataOff\n");
		return(APP_MEM_ERR);
	}
	if (tdp->txb == NULL) {
		printf("texCreateDescr() : Failed to allocate data structure for txb\n");
		return(APP_MEM_ERR);
	}
	if (tdp->tex == NULL) {
		printf("texCreateDescr() : Failed to allocate data structure for tex\n");
		return(APP_MEM_ERR);
	}
	if (tdp->tl == NULL) {
		printf("texCreateDescr() : Failed to allocate data structure for tl\n");
		return(APP_MEM_ERR);
	}

	for (subtx = 0; subtx < num_blocks; subtx++) {
		tdp->txDataOff[subtx] = subtx * tx_xsize * tx_ysize / tx_per_word;
	}
	
/*	printf("...creating txb and tex structures\n"); */

	for (subtx = 0; subtx < num_blocks; subtx++) {
		tdp->txb[subtx] = Txb_Create();
		tdp->tex[subtx] = Tex_Create();

		taddr = &(tmem[tdp->txDataOff[subtx]]);

		tdp->tl[subtx].Data = (void *) taddr;
		tdp->tl[subtx].Size = num_txwords * sizeof(uint32);
		err = Tex_SetTexelData(tdp->tex[subtx], 1, &tdp->tl[subtx]);
		if (err < 0) {
			printf("texCreateDescr() : %d SetTexelData Failed\n", subtx);
			return(err);
		}

		Tex_SetMinWidth(tdp->tex[subtx], tx_xsize);
		if (err < 0) return(err);
		Tex_SetMinHeight(tdp->tex[subtx], tx_ysize + 3);
		Tex_SetDepth(tdp->tex[subtx], TX_DEPTH);
#if (TX_DEPTH == 16)
		Tex_SetFormat(tdp->tex[subtx], 0x1605);
#else 
		Tex_SetFormat(tdp->tex[subtx], 0x1E78);
#endif
		Txb_SetTexture(tdp->txb[subtx], tdp->tex[subtx]);
/*
		Txb_SetTxMinFilter(tdp->txb[subtx], TX_Linear);
		Txb_SetTxMagFilter(tdp->txb[subtx], TX_Linear);
		Txb_SetTxInterFilter(tdp->txb[subtx], TX_Linear);
*/
		Txb_SetTxMinFilter(tdp->txb[subtx], TX_Bilinear);
		Txb_SetTxMagFilter(tdp->txb[subtx], TX_Bilinear);
		Txb_SetTxInterFilter(tdp->txb[subtx], TX_Bilinear);

		Txb_SetTxFirstColor(tdp->txb[subtx], TX_ColorSelectTexColor);
		Txb_SetTxSecondColor(tdp->txb[subtx], TX_ColorSelectPrimColor);
		Txb_SetTxThirdColor(tdp->txb[subtx], TX_ColorSelectPrimColor);
		Txb_SetTxBlendOp(tdp->txb[subtx], TX_BlendOpLerp);
		Txb_SetTxColorOut(tdp->txb[subtx], TX_BlendOutSelectBlend);

		Txb_SetTxPipSSBSel(tdp->txb[subtx], TX_PipSelectConst);
		Txb_SetTxPipAlphaSel(tdp->txb[subtx], TX_PipSelectConst);
		Txb_SetTxPipColorSel(tdp->txb[subtx], TX_PipSelectTexture);

		Txb_SetTxPipConstSSB0(tdp->txb[subtx], 0xFFFFFFFFL);
		Txb_SetTxPipConstSSB1(tdp->txb[subtx], 0xFFFFFFFFL);

		Txb_SetDblDitherMatrixA(tdp->txb[subtx], dithA);
		Txb_SetDblDitherMatrixB(tdp->txb[subtx], dithB);

		if (transparent)
		{
			Txb_SetDblEnableAttrs(tdp->txb[subtx], DBL_BlendEnable | DBL_RGBDestOut | DBL_AlphaDestOut | DBL_SrcInputEnable);
		
			Txb_SetDblDiscard(tdp->txb[subtx], 0);
			Txb_SetDblDSBSelect(tdp->txb[subtx], DBL_DSBSelectSrcSSB);
		
			Txb_SetDblAInputSelect(tdp->txb[subtx], DBL_ASelectTexColor);
			Txb_SetDblAMultCoefSelect(tdp->txb[subtx], DBL_MASelectConst);
			Txb_SetDblAMultConstControl(tdp->txb[subtx], DBL_MAConstByTexSSB);
			Txb_SetDblAMultRtJustify(tdp->txb[subtx], 0);

			
			Txb_SetDblBInputSelect(tdp->txb[subtx], DBL_BSelectSrcColor);
			Txb_SetDblBMultCoefSelect(tdp->txb[subtx], DBL_MBSelectConst);
			Txb_SetDblBMultConstControl(tdp->txb[subtx], DBL_MAConstBySrcSSB); 
			Txb_SetDblBMultRtJustify(tdp->txb[subtx], 0);
			
			Txb_SetDblALUOperation(tdp->txb[subtx], DBL_AddClamp);
			Txb_SetDblFinalDivide(tdp->txb[subtx], 1);
		
			Txb_SetDblAMultConstSSB0(tdp->txb[subtx], &pcolor);
			Txb_SetDblAMultConstSSB1(tdp->txb[subtx], &pcolor);
			Txb_SetDblBMultConstSSB0(tdp->txb[subtx], &pcolor);
			Txb_SetDblBMultConstSSB1(tdp->txb[subtx], &pcolor);
		
			Txb_SetDblDestAlphaSelect(tdp->txb[subtx], DBL_DestAlphaSelectSrcAlpha);
			
			Txb_SetDblSrcPixels32Bit(tdp->txb[subtx], (displayDepth < 32) ? 0 : 1);
			Txb_SetDblSrcXStride(tdp->txb[subtx], 640);
			Txb_SetDblSrcXOffset(tdp->txb[subtx], 0);
			Txb_SetDblSrcYOffset(tdp->txb[subtx], 0);
		}
	}

	tdp->lerpMode = transparent ? LERP_TEX : LERP_BLEND;
	tdp->num_blocks = num_blocks;

	return(APP_OK);
}

Err
getMPEGBuf(MPEGBuf *mpg, uint32 tx_depth) 
{
    Err         err;
	
 	mpg->mpegBase = (uint32 *) malloc(320 * (240 + 2) * BYTE_PER_TX + 32);
	if (mpg->mpegBase == NULL) {
		printf("initMPEG() : Could not allocate buffer\n");
		return(APP_MEM_ERR);
	}
	mpg->mpegPadded = (uint32 *) (((char *) mpg->mpegBase) + 31);
	mpg->mpegPadded = (uint32 *) (((uint32) mpg->mpegPadded) & 0xffffffe0L);
	mpg->mpegActive = mpg->mpegPadded + (320 / (32 / tx_depth));

	printf("mpegPadded = %08x, mpegActive = %08x\n", mpg->mpegPadded, mpg->mpegActive);

    mpg->bitmapItem = CreateItemVA (MKNODEID (NST_GRAPHICS, GFX_BITMAP_NODE),
                         BMTAG_WIDTH, 320,
                         BMTAG_HEIGHT, 240,
                         BMTAG_TYPE, (TX_DEPTH == 32) ? BMTYPE_32 : BMTYPE_16,
                         BMTAG_DISPLAYABLE, TRUE,
                         BMTAG_MPEGABLE, TRUE,
                         TAG_END);

    if (mpg->bitmapItem < 0)
    {
        PrintfSysErr (mpg->bitmapItem);
        printf ("Bitmap creation failed.\n");
		exit(-1);
    }

    if ((err = ModifyGraphicsItemVA (mpg->bitmapItem, BMTAG_BUFFER, mpg->mpegActive, TAG_END)) < 0)
    {
        PrintfSysErr (err);
        printf ("Can't modify Bitmap Item.\n");
		exit(-1);
    }

	if ((mpg->bitmapPtr = (Bitmap *) LookupItem(mpg->bitmapItem)) == NULL)
	{
        printf ("Can't lookup bitmap from Bitmap Item.\n");
		exit(-1);
    }

	return(APP_OK);
}

#ifdef BACKWINDOW

static uint32 bltCList[]= {
	CLT_WriteRegistersHeader(DBSRCCNTL, 4),
	0x0,		/* Src format */
	0x0,		/* Src base addr */
	0x0,		/* Src stride */
	CLT_Bits(DBSRCOFFSET,XOFFSET,0) | CLT_Bits(DBSRCOFFSET,YOFFSET,0),

	CLT_SetRegistersHeader(DBUSERCONTROL, 1),
	CLT_Mask(DBUSERCONTROL,DESTOUTMASK) | CLT_Mask(DBUSERCONTROL,BLENDEN) |
	CLT_Mask(DBUSERCONTROL,SRCEN),

	CLT_WriteRegistersHeader(DBZCNTL, 1),
	CLA_DBZCNTL(1, 1, 1, 1, 1, 1),

	CLT_ClearRegistersHeader(DBUSERCONTROL, 1),
	CLT_Mask(DBUSERCONTROL,WINCLIPOUTEN) |
	CLT_Mask(DBUSERCONTROL,WINCLIPINEN),

	CLT_ClearRegistersHeader(DBDISCARDCONTROL, 1),
	CLT_Mask(DBDISCARDCONTROL,ZCLIPPED) |
	CLT_Mask(DBDISCARDCONTROL,SSB0) |
	CLT_Mask(DBDISCARDCONTROL,RGB0) |
	CLT_Mask(DBDISCARDCONTROL,ALPHA0),

	CLT_ClearRegistersHeader(DBBMULTCNTL,1),
	0x00000060 /* CLT_Mask(DBBMULTCNTL, BINPUTSELECT) */ |
	  CLT_Mask(DBBMULTCNTL, BMULTCOEFSELECT) |
		CLT_Mask(DBBMULTCNTL, BMULTRJUSTIFY),

	CLT_SetRegistersHeader(DBBMULTCNTL,1),
	CLT_SetConst(DBBMULTCNTL, BINPUTSELECT, SRCCOLOR) |
	  CLT_SetConst(DBBMULTCNTL, BMULTCOEFSELECT, CONST) |
		CLT_Bits(DBBMULTCNTL, BMULTRJUSTIFY, 0),

	CLT_WriteRegistersHeader(DBALUCNTL, 1),
	CLT_SetConst(DBALUCNTL, ALUOPERATION, B) |
	  CLT_Bits(DBALUCNTL, FINALDIVIDE, 0),

	/* We've got to write both MULT consts, because we don't know what SSB will
	   come from the texture mapper (it will actually just be garbage) */

	CLT_WriteRegistersHeader(DBBMULTCONSTSSB0, 1),
	CLT_Mask(DBBMULTCONSTSSB0,RED) |
	  CLT_Mask(DBBMULTCONSTSSB0,GREEN) |
	  CLT_Mask(DBBMULTCONSTSSB0,BLUE),

	CLT_WriteRegistersHeader(DBBMULTCONSTSSB1, 1),
	CLT_Mask(DBBMULTCONSTSSB1,RED) |
	  CLT_Mask(DBBMULTCONSTSSB1,GREEN) |
	  CLT_Mask(DBBMULTCONSTSSB1,BLUE),

	CLT_WriteRegistersHeader(DBDESTALPHACNTL, 2),
	CLT_SetConst(DBDESTALPHACNTL,DESTCONSTSELECT,TEXSSBCONST),
	CLT_Mask(DBDESTALPHACONST,DESTALPHACONSTSSB0) |
	  CLT_Mask(DBDESTALPHACONST,DESTALPHACONSTSSB1),

	CLA_TRIANGLE(RC_STRIP, 1, 0, 0, 4),
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
};		

static CltSnippet bltSnippet = {
	&bltCList[0],
	sizeof(bltCList)/sizeof(uint32),
};


Err GfxUtil_ClearScreenToBitmap( GP* gp, BitmapPtr bm, bool clearZ, uint32 x, uint32 y )
{
	gfloat* fp;
	char* mungedBaseAddr;
	int32 offset;

	/* Setup source frame buffer */
	if (bm->bm_Type == BMTYPE_16)
		bltCList[1] = 0;
	else
		bltCList[1] = CLT_Mask(DBSRCCNTL,SRC32BPP);
	offset = x + y * bm->bm_Width;
	mungedBaseAddr = (char*)((int32)(bm->bm_Buffer) - offset * (bm->bm_Type==BMTYPE_16 ? 2 : 4));
	bltCList[2] = (uint32)(mungedBaseAddr);
	bltCList[3] = CLT_Bits(DBSRCXSTRIDE,XSTRIDE,bm->bm_Width);
	if (clearZ)
		bltCList[8] = CLA_DBZCNTL(1, 1, 1, 1, 1, 1);
	else
		bltCList[8] = CLA_DBZCNTL(0, 1, 0, 1, 0, 1);
	fp = (gfloat*)&bltCList[bltSnippet.size - 12];
	*fp++ = x;
	*fp++ = y;
	fp++;
	*fp++ = x;
	*fp++ = y+bm->bm_Height;
	fp++;
	*fp++ = x+bm->bm_Width;
	*fp++ = y;
	fp++;
	*fp++ = x+bm->bm_Width;
	*fp = y+bm->bm_Height;

	GS_Reserve( GP_GetGState(gp), bltSnippet.size+2 );
	CLT_CopySnippetData( GS_Ptr(GP_GetGState(gp)), &bltSnippet );
	CLT_Sync( GS_Ptr(GP_GetGState(gp)) );
	GP_SetTexBlend( gp, GP_GetTxbDefault(gp) );

	return 0;
}

#endif

 
