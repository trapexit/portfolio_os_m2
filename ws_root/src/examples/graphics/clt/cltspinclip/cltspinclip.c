/******************************************************************************
**
**  @(#) cltspinclip.c 96/09/06 1.8
**
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <kernel/types.h>
#include <graphics/clt/clt.h>
#include <graphics/clt/gstate.h>
#include <graphics/graphics.h>
#include <graphics/view.h>
#include <kernel/io.h>
#include <kernel/mem.h>
#include <file/fileio.h>
#include <kernel/time.h>
#include <kernel/random.h>
#include <misc/event.h>



#include <assert.h>
#include <strings.h>
#include "texload.h" /*this file includes headers for all of the UTF loading code*/

#include "math.h" /*this file contains single precision sin, cos and sqrt functions*/
		
#include "clip.h"	/* Headers for the clipping window routines. */

#define SQR(x) x*x

#define DBGX(x) /* printf x */

#define ICOS_P2 0.85065080835204 /*these are a bunch of geometric constants for*/
#define ICOS_P1 0.688190960235587/*getting the vertices of an icosahedron in the*/
#define ICOS_C 0.525731112119134 /*right place*/
#define ICOS_PY 0.262865556059567
#define ICOS_PX 0.809016994374947
#define ICOS_TRIPLENORMAL 2.267283942228513
#define ICOS_TRIPLENORMALSQUARED 5.140576474687267

#define PI 3.141592653589793
						  



#define SCREENTOORIGIN 0.5 /*this represents how many units behind the screen the origin*/
 						   /*of the coordinate system is located*/
#define SCREENTOVIEWER 4.0


#define MINSHADING 0.2 /*this is the minimum darkness value for unlit faces*/


/*The following constants are needed for the clipping. CLIPWINDOWMINX,
CLIPWINDOWMAXX, CLIPWINDOWMINY and CLIPWINDOWMAXY specify the 320x240 region
inside our bitmaps that we're interested in. FINALOFFSETX and FINALOFFSETY are added
to all screen coordinates before display.*/

#define CLIPWINDOWMINX 640
#define CLIPWINDOWMAXX 960
#define CLIPWINDOWMINY 640
#define CLIPWINDOWMAXY 880
#define FINALOFFSETX 640.0
#define FINALOFFSETY 640.0



typedef gfloat vector[3];
typedef vector matrix [3];




vector icospoints[12]={
 {0,0,ICOS_P2/2+ICOS_C},
 {0,-ICOS_P2, ICOS_P2/2},
 {-ICOS_PX, -ICOS_PY, ICOS_P2/2},
 {-0.5, ICOS_P1, ICOS_P2/2},
 {0.5, ICOS_P1, ICOS_P2/2},
 {ICOS_PX, -ICOS_PY, ICOS_P2/2},
 {-0.5, -ICOS_P1, -ICOS_P2/2},
 {-ICOS_PX, ICOS_PY, -ICOS_P2/2},
 {0,ICOS_P2, -ICOS_P2/2},
 {ICOS_PX, ICOS_PY, -ICOS_P2/2},
 {0.5, -ICOS_P1, -ICOS_P2/2},
 {0,0,-ICOS_P2/2-ICOS_C}}; /*these are coordintes for the 12 faces of an icosahedron*/
                           /*with edge length 1. cool, huh?*/
 
 uint32 icosfaceindices[20][3]=
 {{0,3,4},
 {0,2,3},
 {0,1,2},
 {0,1,5},
 {0,5,4},
 {8,3,4},
 {8,4,9},
 {4,9,5},
 {9,5,10},
 {5,10,1},
 {10,1,6},
 {1,6,2},
 {6,2,7},
 {2,7,3},
 {7,3,8},
 {6,10,11},
 {10,9,11},
 {9,8,11},
 {8,7,11},
 {7,6,11}};/*and this array divides the 12 points up into 20 faces*/
 
 uint32 icostextureindices[20]={
   4,3,2,1,0,2,1,3,2,4,3,0,4,1,0,1,0,4,3,2};
 
 vector curicospoints[12]; /*these are the translated coordinates*/
 vector screenicospoints[12]; /*these are the translated coordinates converted*/
   						      /*to two screen coordinates and one 1/w perspective*/
							  /*value*/
 
 
 int32 curfaceorder[20]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19};
int32 dummyfaceorder[20];
 gfloat curdistances[20];/*these three arrays are for face sorting (which I have to*/
  						 /*do because I'm not Z-buffering*/
						 
 vector curnormals[20];/*after rotating the icosahedron, normals to each face are */
 					   /*computed for lighting purposes*/
					   
 gfloat curshading[20]; /*and then the brightness of each face is computed*/
 

 
 gfloat rawsincos[257]; /*look up table for sin and cos*/
 
bool transparency=0; /*transparency can be toggled on and off*/
bool bilinearfiltering=1; /*bilinear filtering can be toggled on and off*/

gfloat stretchfactor=230.0; /*one unit in 3d coordinates equals approximately this many*/
						  /*pixels , when he object in question is lined up with the*/
						  /*screen*/
 


 

 void reporterror(char *errorstring)
{
  printf("ERROR : %s\n", errorstring);
  exit(-1);
}  
 
void initializesincos() /*this function fills up the lookup table for sin and cos. */
                        /*note the use of cosf instead of cos (since cosf is*/
						/*single precision, which is what M2 works with*/
 {
   int32 i;
   
   for (i=0;i<257;i++) rawsincos[i]=cosf(PI/512*i);
   rawsincos[0]=1.0;
   rawsincos[256]=0.0;
 }
 
 gfloat lucos(int32 theta) /*returns the cos of theta*PI/512*/
 {
   theta=abs(theta) % 1024;
   
   if (theta<256) return rawsincos[theta];
   if (theta<512) return -rawsincos[512-theta];
   if (theta<768) return -rawsincos[theta-512];
   return rawsincos[1024-theta];
 }
 
 gfloat lusin(int32 theta)
 {
   theta=abs(theta-256)%1024;
      if (theta<256) return rawsincos[theta];
   if (theta<512) return -rawsincos[512-theta];
   if (theta<768) return -rawsincos[theta-512];
   return rawsincos[1024-theta];
 }
 
 void multvectmatrix(matrix a, vector b, vector c) 
 /*this function multiplies a and b an puts the result in c*/
 
 
 {
   c[0]=a[0][0]*b[0]+a[0][1]*b[1]+a[0][2]*b[2];
   c[1]=a[1][0]*b[0]+a[1][1]*b[1]+a[1][2]*b[2];
   c[2]=a[2][0]*b[0]+a[2][1]*b[1]+a[2][2]*b[2];
 }
 
 void multmatrixmatrix(matrix a, matrix b, matrix c)
 /*this function multiples a and b and puts the result in c*/
 
 {
   int i,j,k;
   
   for (i=0;i<3;i++) 
     for (j=0;j<3;j++)
       {c[i][j]=0;
	    for (k=0;k<3;k++) c[i][j]+=a[i][k]*b[k][j];
		}
  }		
 void addvectorvectorvector (vector a, vector b, vector c,vector d)
 /*this function adds a, b and c and puts the result in d*/
 
 {
   d[0]=a[0]+b[0]+c[0];
   d[1]=a[1]+b[1]+c[1];
   d[2]=a[2]+b[2]+c[2];
  }
  
 void maketransformmatrix(matrix t, int32 a, int32 b, int32 c)
 /*this function takes three angles and makes a transformation matrix which */
 /*rotates c degrees around the z axis, b degrees around the y axis and then*/
 /*a degrees around the x axis...*/
 
 {
   gfloat sina, sinb, sinc, cosa, cosb, cosc;
   
   sina=lusin(a);
   sinb=lusin(b);
   sinc=lusin(c);
   cosa=lucos(a);
   cosb=lucos(b);
   cosc=lucos(c);
   
   t[0][0]=cosa*cosc-sina*sinb*sinc;
   t[0][1]=-sina*cosb;
   t[0][2]=cosa*sinc+sina*sinb*cosc;
   t[1][0]=sina*cosc+cosa*sinb*sinc;
   t[1][1]=cosa*cosb;
   t[1][2]=sina*sinc-cosa*sinb*cosc;
   t[2][0]=-cosb*sinc;
   t[2][1]=sinb;
   t[2][2]=cosb*cosc;
 }
 
/*note: the next four functions collectively are hacks to do the face sorting*/
/*since I was too lazy to right a real quicksort. please do not even read them*/
 
sort2(int32 list1[20], int32 list2[20], int32 i)
{
  if (curdistances[list1[i]]<=curdistances[list1[i+1]]) {
    list2[i]=list1[i];
	list2[i+1]=list1[i+1];
	} else {
	list2[i]=list1[i+1];
	list2[i+1]=list1[i];
	}
}

sort3(int32 list1[20], int32 list2[20], int32 i)
{
  if (curdistances[list1[i]]<=curdistances[list1[i+1]]) {
    if (curdistances[list1[i+1]]<=curdistances[list1[i+2]]) {
	  list2[i]=list1[i];
	  list2[i+1]=list1[i+1];
	  list2[i+2]=list1[i+2];
	  }
	else {
	  if (curdistances[list1[i]]<=curdistances[list1[i+2]]) {
	    list2[i]=list1[i];
	    list2[i+1]=list1[i+2];
	    list2[i+2]=list1[i+1];
	    }
	  else {
	    list2[i]=list1[i+2];
		list2[i+1]=list1[i];
		list2[i+2]=list1[i+1];
		}
    }		
  }		
  else {
    if (curdistances[list1[i+2]]<=curdistances[list1[i+1]]) {
	  list2[i]=list1[i+2];
	  list2[i+1]=list1[i+1];
	  list2[i+2]=list1[i];
	  }
	else {
	  if (curdistances[list1[i]]<=curdistances[list1[i+2]]) {
	    list2[i]=list1[i+1];
		list2[i+1]=list1[i];
		list2[i+2]=list1[i+2];
	  }
	  else {
	    list2[i]=list1[i+1];
		list2[i+1]=list1[i+2];
		list2[i+2]=list1[i];
		}
	  }
    }	
  }	   

void merge(int32 list1[20], int32 list2[20], int32 l1start, int32 l1end, int32 l2start, int32 l2end)
{
  int32 dest=l1start;

  do {
    if (curdistances[list1[l1start]]<=curdistances[list1[l2start]]) 
	 {
	 list2[dest++]=list1[l1start++];
	 if (l1start>l1end) while (l2start<=l2end) list2[dest++]=list1[l2start++];
	 }
	  else {
	 list2[dest++]=list1[l2start++];
	 if (l2start>l2end) while (l1start<=l1end) list2[dest++]=list1[l1start++];
     }
	} while (dest<=l2end);
}


checksorting(int32 list[20], int32 from, int32 to)
{
  int i;
  for (i=from;i<to;i++) 
    if (curdistances[list[i]]>curdistances[list[i+1]]) printf("ERROR between entries %d and %d\n", i, i+1);
	}

void sortlist()
{
  sort2(curfaceorder, dummyfaceorder, 0);
 sort2(curfaceorder, dummyfaceorder, 5);
  sort2(curfaceorder, dummyfaceorder, 10);
  sort2(curfaceorder, dummyfaceorder, 15);
  sort3(curfaceorder, dummyfaceorder, 2);
  sort3(curfaceorder, dummyfaceorder, 7);
  sort3(curfaceorder, dummyfaceorder, 12);
  sort3(curfaceorder, dummyfaceorder, 17);
  

  
  
  merge(dummyfaceorder, curfaceorder, 0,1,2,4);
  merge(dummyfaceorder, curfaceorder, 5,6,7,9);
  merge(dummyfaceorder, curfaceorder, 10,11,12,14);
  merge(dummyfaceorder, curfaceorder, 15, 16, 17, 19);
  

  
  merge(curfaceorder, dummyfaceorder, 0, 4, 5, 9);
  merge(curfaceorder, dummyfaceorder, 10, 14, 15, 19);
  

  
  merge(dummyfaceorder, curfaceorder, 0, 9, 10, 19);
  
  
  
}




 
 
 void transformicos(int32 a, int32 b, int32 c) 
 /*this function takes three angles, creates a rotation matrix, multiplies*/
/* all the vertices of the icosahedron by that matrix, sorts the faces and*/
/* finds screen coordinates for the translated vertices*/
 {
   matrix t;

   int32 i;
   gfloat curzfactor;
   
  maketransformmatrix(t, a, b, c);
   
   for (i=0;i<12;i++) {
     multvectmatrix(t, icospoints[i], curicospoints[i]);

	}

	 
  for (i=0;i<12;i++) {
    curzfactor=1/(SCREENTOVIEWER+SCREENTOORIGIN-curicospoints[i][2]);
	/*this z factor, which is effectively the reciprocal of the distance from the
	viewer to the vertex is used several places. 
	--it's passed to the triangle engine for perspective correction and z-buffering
	(of course, there's no z-buffering in this example, but because I'm already passing
	z-values for perspective correction, it would be a cinch to turn z-buffering on)
	--it's multiplied by the x and y screen coordinates so that things get smaller in 
	the distance
	--it's multiplied by the u and v texture coordinates, because that's what the
	triangle engine expects when perspective correction is on
	*/
	
	screenicospoints[i][0]=stretchfactor*SCREENTOVIEWER*curzfactor*curicospoints[i][0]+160.0+FINALOFFSETX;
	screenicospoints[i][1]=stretchfactor*SCREENTOVIEWER*curzfactor*curicospoints[i][1]+120.0+FINALOFFSETY;
    screenicospoints[i][2]=curzfactor;
  	} 
  
     
   for (i=0;i<20;i++)
     {
	   addvectorvectorvector(curicospoints[icosfaceindices[i][0]],
	     curicospoints[icosfaceindices[i][1]],
		 curicospoints[icosfaceindices[i][2]], 
		 curnormals[i]); /*with a regular icosahedron, normals to each face can be*/
		                 /*easily computed by just adding up the three vertices*/
	   curdistances[i]=curnormals[i][2];
	 }  
   
  
   

   sortlist();
}   
	
void calculateshading()
{
  int i;

  
  for (i=0;i<20;i++) {
    curshading[i]=(-curnormals[i][1]+curnormals[i][2])*1.60321185042515/ICOS_TRIPLENORMALSQUARED;
	if (curshading[i]<MINSHADING) curshading[i]=MINSHADING;
  /*in this simple lighting model, the light falling on each face is just the dot*/
  /*product of the normal to that face with some arbitrary vector (in this case,*/
  /*(0,-1.60321185042515, 1.60321185042515)*/

	}
}	
	
/*the following function was written by Gary Lake, and is quite handy for*/
/*loading in images... if you want to create raw images of this sort, they can*/
/*be exported from debabelizer*/
	
/* LoadRawImage() - Given the path name and file name of a raw RGB image	*/
/* and a pointer to a image size variable, loads the image using the raw	*/
/* file device and returns a pointer to the buffer and updates the size.	*/
uint8 *LoadRawImage (char *FileName, int32 *ImageSizePtr)
{
	RawFile		*ImageFileHandle;
	FileInfo	ImageFileInfo;
	uint8		*ImageBuffer;
	int32		ReturnVal;

	/* Open the specified image file. */
	if ((ReturnVal = OpenRawFile (&ImageFileHandle,
			FileName, FILEOPEN_READ)) < 0)
		reporterror("couldn't open raw file\n");
		
	/* Get the size of the image file. */
	if ((ReturnVal = GetRawFileInfo (ImageFileHandle,
			&ImageFileInfo, sizeof (FileInfo))) < 0)
		reporterror("couldn't get raw file info\n");
		
	/* Allocate memory for the image using file block size. */
	if ((ImageBuffer = (uint8 *) AllocMem ((ImageFileInfo.fi_BlockCount *
			ImageFileInfo.fi_BlockSize), MEMTYPE_NORMAL)) == NULL)
	reporterror("couldn't allocate memory for the image\n");

	/* Read the image file into memory. */
	if ((*ImageSizePtr = ReadRawFile (ImageFileHandle,
			ImageBuffer, ImageFileInfo.fi_ByteCount)) < 0)
	reporterror("couldn't load the image into memory\n");
	
	

	/* Close the image file. */
	if ((ReturnVal = CloseRawFile (ImageFileHandle)) < 0)
		reporterror("couldn't close the raw file \n");

	/* Resize the memory from file block size down to true image size. */
	if (*ImageSizePtr < 
			(ImageFileInfo.fi_BlockCount * ImageFileInfo.fi_BlockSize))
		if ((ImageBuffer = (uint8 *)
				ReallocMem (ImageBuffer,
				(ImageFileInfo.fi_BlockCount * ImageFileInfo.fi_BlockSize),
				*ImageSizePtr, MEMTYPE_NORMAL)) == NULL)
			reporterror("couldn't reallocmem\n");

	return (ImageBuffer);
}
	

int32 main()
{
uint8 *rawimage; /*pointer to the background image*/
int32 imagesize; /*the size of the background image*/
				 
int32 a1=0, a2=0, a3=0; /*three rotation angles*/
int32 i; /*this is a super duper top secret variable, and I can't tell you what I*/
         /*use it for without violating lots of confidentiality :) */
		 /*(just checking to see if anyone's reading all these comments....)*/
		 
int32 curpoint;
gfloat curcolor; /*a couple of short term storage variables*/

	GState	*gs; /*in order to use the CLT, you gotta have a GState...*/
	
TextureSnippets texturesnips[5]; 
/* Will hold cmd lists to load the texture */


ScreenContext	SC;	/* Screen Context structure useful for Bitmap/View managment.
                       This structure is pecific to the clipping code.*/



gfloat	U, V; /*these two variables store the size of the textures (64x64)*/

	bool	done = 0;

ControlPadEventData cped; 



initializesincos();

	CreateClipDisplay (&SC); /* Open Graphics Folio and create all Bitmaps. */
    
	InitEventUtility(1, 0, 0); /*gets us ready to read control port data*/
							   /*(note that this is just a convenience routine*/
							   /*and if you want cool high performance control port*/
							   /*code, you shouldn't use it)*/
	
    rawimage=LoadRawImage("background.raw", &imagesize);

	gs=GS_Create(); /*create the graphics state*/
	
	GS_AllocLists(gs, 2, 4096); /*get lists for the graphics state*/
	GS_SetView (gs, SC.sc_ViewItem);
		
	/*GS_SetVidSignal(gs, SC.sc_RenderSignal);*/  
		 /*If we wanted the CLT to handle our double buffering for us, we'd*/
		 /*uncomment this line and then call GS_BeginFrame() at the beginning of*/
		 /*each frame... */
   
   
   GS_SetZBuffer(gs, SC.sc_ZBufferItem);/*sets the Z-buffer for our GState to
                                         the one we got from CreateClipDisplay*/
   
   for (i=0;i<5;i++) {

	CLT_InitSnippet( &texturesnips[i].dab );
	CLT_InitSnippet( &texturesnips[i].tab );
	texturesnips[i].lcb = NULL;
	texturesnips[i].pip = NULL;
	texturesnips[i].txdata = NULL;
	
	
}

	LoadTexture( &texturesnips[0], "alexface.utf" );
	LoadTexture( &texturesnips[1], "randyface.utf" );
	LoadTexture( &texturesnips[2], "garyface.utf" );
	LoadTexture( &texturesnips[3], "paulface.utf" );
	LoadTexture( &texturesnips[4], "daveface.utf" );
	
	
	UseTxLoadCmdLists( gs, &texturesnips[0] );
	
	
	U = (float)(texturesnips[0].txdata->minX << (texturesnips[0].txdata->maxLOD - 1)) ;
	V = (float)(texturesnips[0].txdata->minY << (texturesnips[0].txdata->maxLOD - 1)) ;
    /*set up U and V to point to the lower right corner of the textures (if all*/
	/*five textures weren't the same size, we'd have to do this seperately for each*/
	/*one*/
 
CLT_DBUSERCONTROL(GS_Ptr(gs), 1,1,0,1,1,1,0,15);
/*basic initialization. In this case, Z-buffering is on, 
window clipping is on, destination blending is on, source
blending is on, and dithering is off*/

CLT_DBZOFFSET(GS_Ptr(gs), 0,0);
/*Sets the Z offset to (0,0), because we want to read from the same place in
the Z-buffer that we're reading from in the frame buffer*/



CLT_DBXWINCLIP(GS_Ptr(gs), CLIPWINDOWMINX, CLIPWINDOWMAXX);
CLT_DBYWINCLIP(GS_Ptr(gs), CLIPWINDOWMINY, CLIPWINDOWMAXY);
/*here's where the clipping window is set*/

CLT_DBDISCARDCONTROL(GS_Ptr(gs),0,0,0,0);
/*all pixel discards are turned off. these would be turned on, for instance, to
make all black pixels transparent, a la opera*/

CLT_DBSRCCNTL(GS_Ptr(gs),0,0);
/*turns off replication of
the low 3 bits, and says that the source frame buffer is 16 bits*/



CLT_DBSRCOFFSET(GS_Ptr(gs), 0,0);
/*we could change these if we wanted to read from some other position in the source 
frame buffer, without changing the source frame buffer pointer*/

CLT_DBCONSTIN(GS_Ptr(gs), 0,0,0);
/*set the constant color used in destination blending to black
(note: this color is the one that is referred to when one of the sources
is chosen to be a constant color, and is _not_ the same as DBAMULTCONSTSSB0 etc.
which are the constants by which the sources are multiplied when the multipliers
are set to be constants*/

CLT_DBAMULTCONSTSSB0(GS_Ptr(gs), 0xff, 0xff, 0xff);
CLT_DBAMULTCONSTSSB1(GS_Ptr(gs), 0xff, 0xff, 0xff);
CLT_DBBMULTCONSTSSB0(GS_Ptr(gs), 0xff, 0xff, 0xff);
CLT_DBBMULTCONSTSSB1(GS_Ptr(gs), 0xff, 0xff, 0xff);
/*set all of our multiplying constants to 1, so we can just pass
colors from the texture and source data through unchanged*/



CLT_DBALUCNTL(GS_Ptr(gs), RC_DBALUCNTL_ALUOPERATION_A_PLUS_B, 0);
/*This set the final ALU stage of the destination blending to just add the
source and texture colors together*/

CLT_DBSRCALPHACNTL(GS_Ptr(gs), 0);
/*turns off Alpha clamping. note that the docs specify an incorrect
number of arguments for this macro*/

CLT_ClearRegister(gs->gs_ListPtr,TXTADDRCNTL,
							FV_TXTADDRCNTL_TEXTUREENABLE_MASK);

GS_SendList(gs);
/*send all that wacky setup information off to the triangle engine... we're doing
this so that our CLT buffers don't overflow*/



	transformicos(a1, a2, a3);
calculateshading();
 /*do all of our 3D calculations for the first pass through*/
 

 
	while(!done) {





GS_SetDestBuffer(gs, SC.sc_BitmapItems[SC.sc_CurrentScreen]);
/*sets the frame buffer that the TE will render into*/

CLT_DBZCNTL(GS_Ptr(gs), 1, 1, 1, 1, 1, 1);
/*for now we want to clear the Z-buffer, so we keep Z-buffering on, but just
set the ZCNTL register so that every value we write will go into the Z-buffer,
and then while we're drawing the background image, we fill the z-buffer up
with 0*/






	
	/*now we're going to set up to clear the screen. We'll do that by drawing
	two large triangles, whose data comes entirely from reading the source
	image out of the raw image we loaded from disk*/
	
	CLT_DBSRCBASEADDR(GS_Ptr(gs), rawimage-(2*(CLIPWINDOWMINX))-(2*320*CLIPWINDOWMINY));
	/*set the address of the raw image. Because we're going to want to read from
	coordinates (CLIPWINDOWMINX, CLIPWINDOWMINY) in the raw image, we have to
	decrement its starting location (note that the coordinates we want to read from 
	are actually way out of range for the raw image, but that doesn't matter*/
	
	CLT_DBSRCXSTRIDE(GS_Ptr(gs), 320);
   /*the width of the source raw image is 320 pixels*/

	
	

	/*turn off texturing*/						
							
CLT_DBAMULTCNTL(GS_Ptr(gs), RC_DBAMULTCNTL_AINPUTSELECT_CONSTCOLOR, 
   RC_DBAMULTCNTL_AMULTCOEFSELECT_CONST, RC_DBAMULTCNTL_AMULTCONSTCONTROL_TEXSSB, 0);
   /*set the texture input to just be a constant color (which in this case we*/
   /*earlier set to black*/
   
CLT_DBBMULTCNTL(GS_Ptr(gs),RC_DBBMULTCNTL_BINPUTSELECT_SRCCOLOR,
   RC_DBBMULTCNTL_BMULTCOEFSELECT_CONST, RC_DBBMULTCNTL_BMULTCONSTCONTROL_TEXSSB, 0);
  /*set the source input to be the frame buffer data, which in this case is just*/
  /*raw image data*/
 

CLT_TRIANGLE(GS_Ptr(gs), 1, RC_STRIP, 1, 0, 0, 4);
/*we need one strip, with perspective, but no color or textures*/

CLT_VertexW(GS_Ptr(gs), CLIPWINDOWMINX,      CLIPWINDOWMINY,0);
CLT_VertexW(GS_Ptr(gs), CLIPWINDOWMINX,      CLIPWINDOWMAXY,0);
CLT_VertexW(GS_Ptr(gs), CLIPWINDOWMAXX,      CLIPWINDOWMINY,0);
CLT_VertexW(GS_Ptr(gs), CLIPWINDOWMAXX,      CLIPWINDOWMAXY,0);
/*we need those 4 vertices to cover the screen. The fourth argument, 0, is
the perspective value, which is what gets written into the z buffer*/

/* Wait for the vertical blank. */
WaitSignal(SC.sc_RenderSignal);

GS_SendList(gs);
/*the most efficient time to wait for a vertical blank is just before doing the first*/
/*drawing to the screen, and that SendList clears the screen*/


CLT_DBZCNTL(GS_Ptr(gs), 0,0,0,0,1, 1);
/*now we want to do normal Z-buffering. This is the standard ZCNTL setup*/

if (transparency) {
CLT_DBAMULTCNTL(GS_Ptr(gs), RC_DBAMULTCNTL_AINPUTSELECT_TEXCOLOR, 
   RC_DBAMULTCNTL_AMULTCOEFSELECT_TEXALPHACOMPLEMENT, 
   RC_DBAMULTCNTL_AMULTCONSTCONTROL_TEXSSB, 0);
  /*if transparency is turned on, then the amount of transparency depends on the*/
  /*alpha values from the texture, with 1.0 being totally transparent. Therefore,*/
  /*the value from the texture that we need is (1-alpha)*texture */


CLT_DBBMULTCNTL(GS_Ptr(gs),RC_DBBMULTCNTL_BINPUTSELECT_SRCCOLOR,
   RC_DBBMULTCNTL_BMULTCOEFSELECT_TEXALPHA, RC_DBBMULTCNTL_BMULTCONSTCONTROL_TEXSSB, 0);
   /*and the value from the background we need is alpha*source */
   
CLT_DBSRCBASEADDR(GS_Ptr(gs), SC.sc_Bitmaps[SC.sc_CurrentScreen]->bm_Buffer);
CLT_DBSRCXSTRIDE(GS_Ptr(gs), 960);
	/*says that the source frame buffer is 320 pixels wide*/


/*and of course we have to reset the source address to point to the current*/
/*screen buffer*/



} else
{

CLT_DBAMULTCNTL(GS_Ptr(gs), RC_DBAMULTCNTL_AINPUTSELECT_TEXCOLOR, 
   RC_DBAMULTCNTL_AMULTCOEFSELECT_CONST, RC_DBAMULTCNTL_AMULTCONSTCONTROL_TEXSSB, 0);
/*if transparency is off, then all we want is the texture color*/

CLT_DBBMULTCNTL(GS_Ptr(gs),RC_DBBMULTCNTL_BINPUTSELECT_CONSTCOLOR,
   RC_DBBMULTCNTL_BMULTCOEFSELECT_CONST, RC_DBBMULTCNTL_BMULTCONSTCONTROL_TEXSSB, 0);

}
CLT_TXTTABCNTL(GS_Ptr(gs), RC_TXTTABCNTL_FIRSTCOLOR_PRIMCOLOR,
                           RC_TXTTABCNTL_SECONDCOLOR_TEXCOLOR,
						   0, 0, 0, RC_TXTTABCNTL_COLOROUT_BLEND,
						   RC_TXTTABCNTL_ALPHAOUT_TEXALPHA,
                           RC_TXTTABCNTL_BLENDOP_MULT);
/*this sets up the texture application blending which is the blending which determines
whether what the destination blending gets is the texture data, the RGB color data,
or both. This setup up has the output from the TAB being primarycolor*texturecolor,
so that the RGB data can basically be used to dim out the texture color, treating
each of the three color channels separately (although in this program they're always
dimmed equally*/




for (i=0;i<20;i++) {
  /*loop through the 20 faces*/
  
  
  UseTxLoadCmdLists( gs, &texturesnips[icostextureindices[curfaceorder[i]]] );
  /*for each face, load the appropriate texture into TRAM*/

  if (bilinearfiltering) {
  CLT_TXTADDRCNTL(GS_Ptr(gs), 1, RC_TXTADDRCNTL_MINFILTER_BILINEAR,
    RC_TXTADDRCNTL_INTERFILTER_BILINEAR, RC_TXTADDRCNTL_MAGFILTER_BILINEAR, 0);
  /*turn texturing back on, and select bilinear filtering*/
  } else {

  CLT_TXTADDRCNTL(GS_Ptr(gs), 1, RC_TXTADDRCNTL_MINFILTER_POINT,
    RC_TXTADDRCNTL_INTERFILTER_POINT, RC_TXTADDRCNTL_MAGFILTER_POINT, 0);
  }

 


  CLT_TRIANGLE(GS_Ptr(gs), 1, RC_STRIP, 1, 1, 1, 3);
  /*for each face, we need to draw only one triangle*/
  
  curpoint=icosfaceindices[curfaceorder[i]][0];
  curcolor=curshading[curfaceorder[i]];
  
  
  
  CLT_VertexRgbaUvW(GS_Ptr(gs), screenicospoints[curpoint][0],
         screenicospoints[curpoint][1],
		 curcolor, curcolor, curcolor, 0,
		 0.5*U*screenicospoints[curpoint][2],
		 0.0*4, 
		 screenicospoints[curpoint][2]);
/*note that for each vertex we pass in color information and texture information,
and note that the texture coordinates are multiplied by the W value. this is 
necessary for perspective correction*/


  curpoint=icosfaceindices[curfaceorder[i]][1];
  CLT_VertexRgbaUvW(GS_Ptr(gs), screenicospoints[curpoint][0],
         screenicospoints[curpoint][1],
		 curcolor, curcolor, curcolor, 0,
		 0.0, 
		 V*0.7*screenicospoints[curpoint][2], 
		 screenicospoints[curpoint][2]);
  
  curpoint=icosfaceindices[curfaceorder[i]][2];
  CLT_VertexRgbaUvW(GS_Ptr(gs), screenicospoints[curpoint][0],
         screenicospoints[curpoint][1],
		 curcolor, curcolor, curcolor, 0,
		 U* screenicospoints[curpoint][2], 
		 V*0.7*screenicospoints[curpoint][2], 
		 screenicospoints[curpoint][2]);		 

  CLT_Sync(	GS_Ptr(gs));
  /*after calling CLT_TRIANGLE it's always necessary to call CLT_Sync (or SendList)*/
  
  
 }

GS_SendList(gs);
/*this call to sendlist actually draws the icosahedron*/

GS_EndFrame(gs);
/*this call tells the triangle engine to wait until it's done with the previously sent command
 *list, and then swap the buffers so that the current screen is displayed (although that swap
 *won't actually happen until the next vertical blank
 */

SC.sc_CurrentScreen = 1 - SC.sc_CurrentScreen;
	


GetControlPad(1, FALSE, &cped);

if (cped.cped_ButtonBits & ControlA) {
  if (cped.cped_ButtonBits & ControlLeft) bilinearfiltering=0;  
  if (cped.cped_ButtonBits & ControlRight) bilinearfiltering=1;
  if (cped.cped_ButtonBits & ControlUp) transparency=1;
  if (cped.cped_ButtonBits & ControlDown) transparency=0;
} else {
if (cped.cped_ButtonBits & ControlLeft) a2+=4; 
if (cped.cped_ButtonBits & ControlRight) a2-=4;
if (cped.cped_ButtonBits & ControlUp) a3+=4; 
if (cped.cped_ButtonBits & ControlDown) a3-=4; 
}
if (cped.cped_ButtonBits & ControlLeftShift) a1+=4; 
if (cped.cped_ButtonBits & ControlRightShift) a1-=4;
if (cped.cped_ButtonBits & ControlX) done=TRUE;
if (cped.cped_ButtonBits & ControlB) stretchfactor*=1.02;
if (cped.cped_ButtonBits & ControlC) stretchfactor*=0.98;
/*here we do basic control pad input*/


    transformicos(a1, a2, a3);
	calculateshading();
 /*rotate and transform the Icosahedron for the next frame*/


}		

DestroyClipDisplay (&SC);

ShutDownClip (QUIT, QUIT);

}
/*have a nice day*/
