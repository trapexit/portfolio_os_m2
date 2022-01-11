/******************************************************************************
**
**  @(#) cltsurface.c 96/08/25 1.6
**
******************************************************************************/
/*


This piece of sample code is intended to demonstrate several things:
-How to load textures "by hand", setting all of the registers necessary to 
load chunks of the screen into TRAM as textures
-How to triple buffer
-How to construct and send TE command lists using nothing but CLT macros
-How to get really really neat color-echo type effects on M2 (ooooooh)
-A simplistic way to use the big blocks of memory left over after using Gary
and my clipping code

Basically, this demo puts a 3D surface up on the screen, textures it, 
optionally lights it, and animates it. Control pad commands are as follows:

D-pad, Left Shift, Right Shift: rotate surface
Start: animate surface
Button A + D-pad up or down: zoom in or out
Button A + D-pad left and right: turn lighting on or off
Button C + D-pad left and right: switch between displaying a static texture
(a picture of Tom Bazzano from customer service, currently) and displaying
what's currently in the frame buffer (for the cool feedback effects)
Button C + D-pad up and down: turn auto-cycling on or off (turning
auto-cycling on is equivalent to holding down left shift and D-pad left
so the surface spins on its own)
Button B + D-pad: change the number of triangles used to draw the surface,
as follows:
  Up: 6144 triangles
  Down: 384 triangles
  Left: 1536 triangles
  Right: 24576 triangles
X: quit

A note on performance: the number of raw triangles per second that this demo 
runs at is fairly low, varying between 58,000 and 174,000, depending on
how many triangles make up the surface. There are several reasons for this:
-The 3D engine was made up off the top of my head
-Each frame, numerous short command lists are send to the triangle engine, 
which could easily be conglomerated
-Nothing fits, or even attempts to fit, inside either the D-cache or the
I-cache
So, don't look at this demo as either showing the best performance M2 could 
generate, or as being an example of how to write optimized M2 code
*/




  
#include <stdio.h>
#include <stdlib.h>
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

#include <kernel/debug.h>
#include <kernel/operror.h>

#include <assert.h>
#include <strings.h>

#include <math.h> 


#include "clip3.h"	/* this file contains the clipping code written by Gary Lake. This is
                       a special version of the code which generates 3 screen buffers,
					   for triple buffering*/

#define PI 3.141592653589793 /*mathematical constants*/
#define SQRT2 0.707106781186548*2			  



 #define DATAMINX (-1.0)
 #define DATAMAXX 1.0
 #define DATAMINY (-0.75)
 #define DATAMAXY 0.75
 /*the (x,y) coordinates in the heightfield range from (-1,-0.75) to (1,0.75)*/
 

#define NUMCOMMANDLISTS 6
/*this sets how many command lists we're going to work with.*/

#define SCREENSHIFTX 1120.0
#define SCREENSHIFTY 760.0
#define CLIPWINDOWMINX 960
#define CLIPWINDOWMAXX 1280
#define CLIPWINDOWMIDX ((CLIPWINDOWMINX+CLIPWINDOWMAXX)/2)

#define CLIPWINDOWMINY 640
#define CLIPWINDOWMAXY 880
#define CLIPWINDOWMIDY ((CLIPWINDOWMINY+CLIPWINDOWMAXY)/2)


/*with the triple buffering clipping code, our screen buffers live inside 
1312-pixel-wide bitmaps, so we need to set a clipping window from 960 to 1280,
and all of our images will be shifted over by 1120 pixels. Vertically,
we need to clip from 640 to 880, and shift 760.
(This shifting is necessary, because we internally pretend that (0,0) is the
center of the screen)*/


#define SCREENTOORIGIN 0.5
#define SCREENTOVIEWER 4.0
/*these define how far behind the screen (0,0,0) is located in our 3D model,
and how far from the screen the viewer is assumed to be*/


#define SQR(x) ((x)*(x))

#define ROTATIONSPEED 3
/*how fast the surface will turn when the control pad buttons are pressed*/

#define TEXTUREWIDTH 320
#define TEXTUREHEIGHT 240
#define TEXTURESTRIPS 12
/*how big the texture is, and how many horizontal strips we're going to carve
it into. Each strip must be be less than 16K (here, 320*20*2=12800). Also, 
since we can have a surface made up of as few as 12 strips of rectangles,
the number of strips must divide 12*/


#define DATAXRES 128
#define DATAYRES 96
#define ACTUALDATAWIDTH (DATAXRES+1)
/*this sets how many points of height data we'll actually store. Because we 
want to have 128x96 rectangles, we actually need 129x97 height values, to
set the outside edges of all the rectangles*/


#define WORDSPERVERTEX 9
/*this sets how much data we need in the command lsits for each vertex. We need
2 words for x and y, 4 words for rgba, 2 words for u,v, and 1 word for w*/


#define TXTLOADLISTSIZE 1000
/*this is how much space we'll allocate for the part of the command list which loads
the textures. This is _way_ more space than we need*/

#define COMMANDLISTSIZE (2*(ACTUALDATAWIDTH)*WORDSPERVERTEX*sizeof(uint32)+100 + TXTLOADLISTSIZE)
/*This is how much total space we need for each command list. It has to be big enough to 
send all the vertex values for an entire strip of triangles (we're dividing the surface
up into strips of triangles horizontally), so we need 129*2 vertices, with 
WORDSPERVERTEX*sizeof(uint32) bytes per vertex, plus extra space for the texture load list,
plus an extra hundred bytes for the pause command, the triangle command, etc.*/


#define TEXXCOORDFACTOR (gfloat)TEXTUREWIDTH/DATAXRES
/*this is how much the texture coordinate needs to increase between each consecutive
vertex, horizontally*/

#define TEXELSPERSTRIP (TEXTUREHEIGHT/TEXTURESTRIPS)
#define DATAPOINTSPERSTRIP (DATAYRES/TEXTURESTRIPS)
/*This sets how many texels and how many data points, vertically, are included in each
horizontal strip.*/

typedef struct vector {
  gfloat x, y, z;
  } vector;

typedef struct vector2d {
  gfloat x,y;
  } vector2d;
  
int texturesourcewidth=320;
/*this sets the width of the bitmap from which the texture is being read. When reading
data from a stored image, this is just 320. But when reading from the frame buffer,
this is 1312, due to the wacky triple buffering and clipping*/

int lighting=0;
int displayingtom=1; /*two options that can be toggled*/


gfloat screenstretchfactor=175.0; /*this is essentially the zoom factor*/
  

int actualtxtloadlistsize;/*this variable gets set to the actual size of a texture
loading list. You see, I stick a texture load list at the beginning of _every_ command
list, and then when I'm sending the command list off, I sometimes just send the portion
of the command list after the texture load list. However, I need to know how big the
texture load list is, so that I can construct the rest of the command list immediately
after it*/

int txtloadlistaddroffset; /*this variable contains the offset in the texture load
list to the address of the texture being loaded, which is the only thing that needs
to be changed on the fly*/

CmdListP commandlists[NUMCOMMANDLISTS]; /*our command list pointers*/

uint8* bigfreebuffer; /*this is a pointer to the big empty buffer that we get from the
  clipping code*/
  
int sizeofbigfreebuffer; /*this is the size of that buffer*/

ScreenContext	SC; /*this struct contains all the info for the triple buffers*/


gfloat rawsincos[257]; /*look up table for sin and cos*/


uint32 *data; /*this is a big array that we'll fill up with heightfield data.
 The way I'm managing the data is that in this array, I'm just storing the distance
 of each point from the origin, ranging from 0 to 1024 (with 1024 standing for
 2 PI). Because of the sinusoidal nature of the surface, this is all that we 
 really need to store, and this allows the animation*/


vector2d *data2dnormals; /*at each data point, we'll also store a normal pointing
  away from the origin*/
  
gfloat dataheights[1024];
gfloat normalrise[1024];
gfloat normalrun[1024];/*once we know how far from the origin a point is, we can
 then use these looup tables to find the actual surfaceheight at that point, as 
 well as rise and run for the normal vector, which, combined with the data2dnormal,
 will give us all 3 coordinates of the normal vector, which we need for 
 lighting purposes*/


vector e1, e2, e3; /*the 3 current unit vectors*/
vector lightingvector; /*the current lighting vector. Normally, we would keep
  the lighting vector still and rotate each normal vector. However, since there's
  only one surface and it rotates in a fixed fashion, it makes more sense to keep 
  the normal vectors constant and rotate the single lighting vector*/

int curtime=0; /*this stores the current time, for animation purposes*/

int curdataxres, curdatayres;
gfloat curdataxstep, curdataystep;
int curLOD, curLODspacing;

uint32 *rawimage;
uint32 *textureaddress;

int sizeofclearscreenlist;
 

void initializesincos(void) /*this function fills up the lookup table for sin and cos. */
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
 
 gfloat lusin(int32 theta) /*returns the sin of theta*PI/512*/
 {
   theta=abs(theta-256)%1024;
      if (theta<256) return rawsincos[theta];
   if (theta<512) return -rawsincos[512-theta];
   if (theta<768) return -rawsincos[theta-512];
   return rawsincos[1024-theta];
 }
 
 
  void reporterror(char *errorstring)
{
  printf("ERROR : %s\n", errorstring);
  exit(-1);
}  
 
 
 void normalizevector(vector *v) /*this function takes a vector and returns a normal vector in the 
   same direction. It will crash if it's passed the zero vector*/
   
 {
   gfloat invlength;
   
   invlength=1/sqrtf(SQR(v->x)+SQR(v->y)+SQR(v->z));
   
   v->x*=invlength;
   v->y*=invlength;
   v->z*=invlength;
  }
  
 void normalizevector2d(vector2d *v) /*same as the previous function, but for 2D vectors*/
 {
   gfloat invlength;
   
   invlength=1/sqrtf(SQR(v->x)+SQR(v->y));
   
   v->x*=invlength;
   v->y*=invlength;
  }
  
 
   
   
 void initializedata(void)/*this function initializes the data arrays*/
 
 {
   int x,y, sizeofdata, sizeofnormals;
 
 sizeofdata=  ACTUALDATAWIDTH*(DATAYRES+1)*sizeof(uint32);
 
 
 printf("size for data is %d=%d * %d * %d\n",  sizeofdata,
   ACTUALDATAWIDTH,(DATAYRES+1),(sizeof(uint32)));
 
 
 sizeofnormals=2*sizeofdata;
 
 
 
 
 if (sizeofdata+sizeofnormals>sizeofbigfreebuffer) {
   printf("ERROR. not enoug memory left in the big free buffer to create the data\n");
   exit (-1);
   }
   
 
 data=(uint32*) bigfreebuffer; /*here we're doing primitive memory management*/

 bigfreebuffer+=sizeofdata;
 
 data2dnormals=(vector2d*) bigfreebuffer;
 
 bigfreebuffer+=sizeofnormals;
 
 sizeofbigfreebuffer-=sizeofdata+sizeofnormals;
 
 
 for (y=0;y<=DATAYRES;y++) for (x=0;x<ACTUALDATAWIDTH;x++)  
   {
     data[y*ACTUALDATAWIDTH+x]=((uint32) (6.0*sqrtf(SQR(DATAMINX+(1.0*x*(DATAMAXX-DATAMINX))/DATAXRES)+
	    SQR(DATAMINY+(1.0*y*(DATAMAXY-DATAMINY))/DATAYRES))/2/PI*1024.0))%1024;
	 /*in the data vector, we store each point's distance from the origin as an integer, modulo 1024, with
	   1024 standing in for 2*PI*/
		
     data2dnormals[y*ACTUALDATAWIDTH+x].x=DATAMINX+(1.0*x*(DATAMAXX-DATAMINX))/DATAXRES;
	 data2dnormals[y*ACTUALDATAWIDTH+x].y=DATAMINY+(1.0*y*(DATAMAXY-DATAMINY))/DATAYRES;
     /*at each point, we store in data2dnormals a normalized vector pointing away from the origin*/

	 if ((x!=DATAXRES/2) || (y!=DATAYRES/2)) normalizevector2d(&(data2dnormals[y*ACTUALDATAWIDTH+x]));
      
   }
 
 for (x=0;x<1024;x++) dataheights[x]=lucos(x)/8;
 /*then in dataheigts, we store 1024 values of the actual function, which (being a pond-ripple type
   function, is the cos of the distance from the origin*/
 
 for (x=0;x<1024;x++) normalrise[x]=1/sqrtf(1+SQR(0.75*lusin(x)));
 
 for (x=0;x<1024;x++) normalrun[x]=0.75*lusin(x)/sqrtf(1+SQR(0.75*lusin(x)));
 /*in normalrise and normalrun, we store the Z value of the normal to the surface, and the
   other component, which is the length away from the origin that normal travels... this is a
   confusing and basically meaningless process which optimizes computation of normals in this case,
   since the function is circularly symmetric, but would otherwise be useless*/
 
 
 }
 
 initcommandlist(CmdListP cl)/*The parts of the command lists that never change are the
   triangle command and the pause command at the end of the list*/
   
 {
   cl[actualtxtloadlistsize]=CLA_TRIANGLE(1, RC_STRIP, 1, 1, 1, (curdataxres+1)*2);
   cl[actualtxtloadlistsize+(curdataxres+1)*2*WORDSPERVERTEX+1]=CLT_WriteRegistersHeader(DCNTL,1);
   cl[actualtxtloadlistsize+(curdataxres+1)*2*WORDSPERVERTEX+2]=CLT_Bits(DCNTL, SYNC, 1);
 }
 
 void calcunitvectors(int32 a, int32 b, int32 c) /*this function calculates the images of the 
   3 unit vectors under a transformation defined by three angles, a b & c*/
 {
     gfloat sina, sinb, sinc, cosa, cosb, cosc;
   
   sina=lusin(a);
   sinb=lusin(b);
   sinc=lusin(c);
   cosa=lucos(a);
   cosb=lucos(b);
   cosc=lucos(c);
   
   e1.x=cosa*cosc-sina*sinb*sinc;
   e2.x=-sina*cosb;
   e3.x=cosa*sinc+sina*sinb*cosc;
  
   e1.y=sina*cosc+cosa*sinb*sinc;
   e2.y=cosa*cosb;
   e3.y=sina*sinc-cosa*sinb*cosc;
  
   e1.z=-cosb*sinc;
   e2.z=sinb;
   e3.z=cosb*cosc; 
  

 }
 
void maketxtloadlists(void) /*This function initializes the texture loading portion at the
  beginning of each command list.*/
{
  uint32* listptr;
  int i;
  
  for (i=1;i<NUMCOMMANDLISTS;i++) {
    
  
  listptr=commandlists[i];
  
  CLT_TXTLDCNTL(&listptr, 0, RC_TXTLDCNTL_LOADMODE_TEXTURE, 0);
    /*specifies that we're doing an uncompressed texture load with bit offset 0*/
  
  CLT_TXTADDRCNTL(&listptr, 1, RC_TXTADDRCNTL_MINFILTER_BILINEAR,
    RC_TXTADDRCNTL_INTERFILTER_BILINEAR, RC_TXTADDRCNTL_MAGFILTER_BILINEAR, 0);  
	/*turns texturing on, sets the filtering to bilinear, and specifies only one 
	  LOD */
	
	
  CLT_TXTCOUNT(&listptr, TEXELSPERSTRIP); 
    /*when doing a texture load, this register specifies how many rows to load*/
	
  txtloadlistaddroffset=((uint32) listptr - (uint32) commandlists[i])/sizeof(uint32)+1;
    /*we save a variable that tells us how many words into the list the address of the texture
	  is found, so that we can poke into the list directly to change the address*/
 
  CLT_TXTLDSRCADDR(&listptr, 0);
    /*the actual address set here is a dummy since it will always be overwritten*/
  
  
  CLT_TXTLODBASE0(&listptr, 0);
    /*this specifies that we wish to load into the very beginning (address 0) of texture RAM*/
	
  
  CLT_TXTLDWIDTH(&listptr, TEXTUREWIDTH*16, texturesourcewidth*16);
    /*this register, during texture loads, contains the width of the texture (in bits) and the
	  width of the source bitmap from which the texture is being read, in bits*/
	  
  CLT_TXTEXPTYPE(&listptr, 5, 0, 0, 1, 1, 0, 1);
    /*sets a color depth of 5 (16 bit color has 5 bits each for R,G,B), alpha depth of 0, 
	  not transparent, has SSB, has color, has no alpha, is literal*/
	  

  CLT_TXTUVMASK(&listptr, 0x3ff, 0x3ff);
    /*turns off address masking (which we would turn on if we wanted a tiled texture)*/

  CLT_TxLoad(&listptr);
    /*the actual command to do the texture load*/
  
  CLT_TXTUVMAX(&listptr, TEXTUREWIDTH-1,TEXELSPERSTRIP-1);
    /*now that the texture load is over, we need to reset this register to the width and height of the
	  texture, minus one*/
   
   CLT_TXTPIPCNTL(&listptr, 0, 1, 1, 1); 
    /*for reasons that are not entirely clear to me, this register also needs to be set to this value,
	  even though there is no PIP since this is a literal texture, or the color coming from the 
	  texturing will always be zero*/
 
  
  actualtxtloadlistsize=((uint32) listptr - (uint32) commandlists[i])/sizeof(uint32);
    /*now we save the size that this list was*/
  
  
  }	
}
  
  
  
void makeclearscreenlist(void)/*this procedure assembles the command list that is used to clear the screen
  every frame. That list is always saved in commandlists[0].*/
  
{
  uint32* listptr;
  
  listptr=commandlists[0];
 
  
  CLT_DBUSERCONTROL(&listptr, 1,1,0,1,1,1,0,15);
    /*set the general purpose register to Z Buffering on, Z Buffer output on, 
	  clipping only outside the clip region, blending enabled, secondary source enabled,
	  dithering disabled, and RGBA all being output*/
	  
  CLT_ClearRegister(listptr, ESCNTL, FV_ESCNTL_PERSPECTIVEOFF_MASK);
    /*turns perspective correction on*/

  CLT_DBZOFFSET(&listptr, 0,0);
     /*sets the X and Y offset in the Z buffer to zero*/

  CLT_DBXWINCLIP(&listptr, CLIPWINDOWMINX, CLIPWINDOWMAXX);
  CLT_DBYWINCLIP(&listptr, CLIPWINDOWMINY, CLIPWINDOWMAXY);
   /*set the clipping region to the proper values for the triple buffered clipping buffers*/
       
  CLT_DBDISCARDCONTROL(&listptr,0,0,0,0);
   /*discard no pixels*/
 
  CLT_DBCONSTIN(&listptr, 0,0,0);
   /*set the constant color for the destination blending to black*/
  
  CLT_DBAMULTCONSTSSB0(&listptr, 0xff, 0xff, 0xff);
  CLT_DBAMULTCONSTSSB1(&listptr, 0xff, 0xff, 0xff);
  CLT_DBBMULTCONSTSSB0(&listptr, 0xff, 0xff, 0xff);
  CLT_DBBMULTCONSTSSB1(&listptr, 0xff, 0xff, 0xff);
    /*set all the destination blending multipliers to 0xff, which counts as 1*/
	

  CLT_DBALUCNTL(&listptr, RC_DBALUCNTL_ALUOPERATION_A_PLUS_B, 0);
    /*sets the ALU operation to simple addition with no shift*/
  
  CLT_DBSRCALPHACNTL(&listptr, 0);
    /*turns off Alpha clamping*/
  
  CLT_DBZCNTL(&listptr, 1, 1, 1, 1, 1, 1);
    /*sets the Z buffering to write to the Z buffer and the screen no matter what. That's so that we can
	  draw our background triangles with height 0 and clear both the frame buffer and the Z buffer in one
	  easy step*/
	  
 
  
  CLT_DBAMULTCNTL(&listptr, RC_DBAMULTCNTL_AINPUTSELECT_TEXCOLOR, 
   RC_DBAMULTCNTL_AMULTCOEFSELECT_CONST, RC_DBAMULTCNTL_AMULTCONSTCONTROL_TEXSSB, 0);
   /*sets the input to one side of the destination blending to the texture color (which in this case,
     with texturing off, is just the RGBA color from the texture blender)*/

 
  CLT_DBBMULTCNTL(&listptr,RC_DBBMULTCNTL_BINPUTSELECT_CONSTCOLOR,
   RC_DBBMULTCNTL_BMULTCOEFSELECT_CONST, RC_DBBMULTCNTL_BMULTCONSTCONTROL_TEXSSB, 0);
   /*sets the input to the other side of the destination blending to the constant color (black)*/
   
   
   CLT_TXTTABCNTL(&listptr, RC_TXTTABCNTL_FIRSTCOLOR_TEXCOLOR, RC_TXTTABCNTL_SECONDCOLOR_PRIMCOLOR,
    RC_TXTTABCNTL_THIRDCOLOR_PRIMALPHA, 0, 0, RC_TXTTABCNTL_COLOROUT_BLEND, 
	RC_TXTTABCNTL_ALPHAOUT_TEXALPHA, RC_TXTTABCNTL_BLENDOP_LERP);
   /*sets the texture application blending so that the primitive alpha lerps between the primitive color and
     the texture color. This is how our lighting model works.*/

  CLT_TXTADDRCNTL(&listptr, 0, RC_TXTADDRCNTL_MINFILTER_BILINEAR,
    RC_TXTADDRCNTL_INTERFILTER_BILINEAR, RC_TXTADDRCNTL_MAGFILTER_BILINEAR, 0);
   /*turns texturing off and sets the filtering*/
	
	  
	  
	  
	  /*this strip of two triangles covers the entire screen. Note that each vertex has alpha
	  of 1.0 and W of 0*/ 
	  
 /* CLT_TRIANGLE(&listptr, 1, RC_STRIP, 1, 0, 1, 4);
    
  CLT_VertexRgbaW(&listptr, CLIPWINDOWMINX,      CLIPWINDOWMINY, 1.0, 0, 0, 1.0, 0);
  CLT_VertexRgbaW(&listptr, CLIPWINDOWMINX,      CLIPWINDOWMAXY,0, 1.0, 0, 1.0,  0);
  CLT_VertexRgbaW(&listptr, CLIPWINDOWMAXX,      CLIPWINDOWMINY,0,0,1.0,1.0,0);
  CLT_VertexRgbaW(&listptr, CLIPWINDOWMAXX,      CLIPWINDOWMAXY,1.0,1.0,0,1.0,0);
 */
 
 CLT_TRIANGLE(&listptr, 1, RC_FAN, 1,0,1,9);
 
  CLT_VertexRgbaW(&listptr, CLIPWINDOWMIDX,      CLIPWINDOWMIDY, 1.0, 1.0, 1.0, 1.0, 0);
  CLT_VertexRgbaW(&listptr, CLIPWINDOWMINX,      CLIPWINDOWMIDY, 1.0, 0.0, 0.0, 1.0, 0);
  CLT_VertexRgbaW(&listptr, CLIPWINDOWMINX,      CLIPWINDOWMINY, 0.0, 1.0, 0.0, 1.0, 0);
  CLT_VertexRgbaW(&listptr, CLIPWINDOWMIDX,      CLIPWINDOWMINY, 1.0, 1.0, 0.0, 1.0, 0);
  CLT_VertexRgbaW(&listptr, CLIPWINDOWMAXX,      CLIPWINDOWMINY, 0.0, 0.0, 1.0, 1.0, 0);
  CLT_VertexRgbaW(&listptr, CLIPWINDOWMAXX,      CLIPWINDOWMIDY, 1.0, 0.0, 1.0, 1.0, 0);
  CLT_VertexRgbaW(&listptr, CLIPWINDOWMAXX,      CLIPWINDOWMAXY, 0.0, 1.0, 1.0, 1.0, 0);
  CLT_VertexRgbaW(&listptr, CLIPWINDOWMIDX,      CLIPWINDOWMAXY, 0.0, 0.5, 1.0, 1.0, 0);
  CLT_VertexRgbaW(&listptr, CLIPWINDOWMINX,      CLIPWINDOWMIDY, 1.0, 0.0, 0.0, 1.0, 0);
  
  CLT_TRIANGLE(&listptr, 0, RC_STRIP, 1, 0, 1, 1);
  
 CLT_VertexRgbaW(&listptr, CLIPWINDOWMINX,      CLIPWINDOWMAXY, 1.0, 0.0, 0.5, 1.0, 0);
   
 
  CLT_Sync(&listptr);
    /*always call CLT_Sync after a triangle instruction*/      
  
  CLT_DBZCNTL(&listptr, 0, 0, 0, 0, 1, 1);
     /*reset the Z buffering for normal operation*/
 
  
  sizeofclearscreenlist=listptr-commandlists[0];
  }
  

  
  
  
void initTEstuff(void) /*this procedure initializes a lot of stuff for the Triangle Engine*/

{
  int i;
  if (sizeofbigfreebuffer<NUMCOMMANDLISTS*COMMANDLISTSIZE) {
     printf("ERROR not enough memory remaining in the big free buffer for the command lists\n");
	 exit(-1);
	 }
  for (i=0;i<NUMCOMMANDLISTS;i++) 
    commandlists[i]=(CmdListP)(bigfreebuffer+(i*COMMANDLISTSIZE));
  bigfreebuffer+=NUMCOMMANDLISTS*COMMANDLISTSIZE;
  sizeofbigfreebuffer-=NUMCOMMANDLISTS*COMMANDLISTSIZE;
    /*First, we use our goofy little memory allocation code to allocate a block of memory for 
	  our command lists*/
	
  makeclearscreenlist();
    /*then we initialize the screen clearing list*/  

  
 
	 
}



void changeresolution (int newlod) /*this procedure changes the number of triangles that the
  surface is made up of... the number passed in can range from 1 (few triangles) to 4 
  (many triangles)*/
  
{ int i;
  curLOD=newlod; /*curLOD is a global variable which stores what the current LOD is*/
  
  switch (newlod) {
 	case 1: curLODspacing=8; curdataxres=16; curdatayres=12; break;
	case 2: curLODspacing=4; curdataxres=32; curdatayres=24; break;
	case 3: curLODspacing=2; curdataxres=64; curdatayres=48; break;
	case 4: curLODspacing=1; curdataxres=128; curdatayres=96; break;
	}
  /*sets up a view basic variables according to the new LOD*/	
	
curdataxstep=(DATAMAXX-DATAMINX)/curdataxres;
curdataystep=(DATAMAXY-DATAMINY)/curdatayres;
  /*these two variable represent how far apart, in mathematical data coordinates, the vertices of 
    the triangles are*/
    


	
for (i=1;i<NUMCOMMANDLISTS; i++) initcommandlist(commandlists[i]); /*when we change the resolution, the number
 of vertices per strip changes, so we have to reinitialize the command lists, to get the pause commands in the
 right place*/
 
}



doonerow(int whichrow, CmdListP lowerlist, CmdListP upperlist, 
    vector startingpoint, vector forwardvector, vector upvector)
	  /*this procedure marches down one rows (in the X direction) of vertices of the surface, 
	    calculates screen coordinates and colors, and writes those vertices into two command lists
		(because each row of vertices forms the border between two strips of triangles).
		
		So whichrow is the y coordinate of the row of data to be processed (as an offset into the
		array of data). lowerlist and upperlist are the two command lists being processed. startingpoint
		is a vector representing the position of the first point in the current row in the plane under 
		the heightfield. forwardvector is a vector which is added to one point in that row to get the
		next point. upvector is the vector which must be multiplied by the current height and added
		to those base points to get the actual point in the heightfield.
		*/
		
	
	{
	  vector curdatapoint;/*during calculations for each vector, this will store the actual 3-space location
	    of the current vector*/
	  
	  vector currowpoint;
	    /*this vector will be used to step along the row in the base plane*/
	  
	  vector curnormal;
	    /*this vector will be used to store the current normal, for lighting purposes*/
	  
	  gfloat curdatavalue, curzfactor, curcolor;
	    
	  
	  
	  float *lowerlistf, *upperlistf;
	     /*pointers which we will use to walk through the command lists*/
	  
	  register float dummy;
	    /*dummy variable for storing intermediate results, used in a slightly lame attempt to 
		  optimize the code*/
		
	  int x=0;
	    /*keeps track of the x-value of the current point, as an offset into the data*/
		
	  int curdataindex;
	  gfloat texycoord;
	  int curpointtime;
	  
	  
	  lowerlistf=(float*) lowerlist+1;
	  upperlistf=(float*) upperlist+1+WORDSPERVERTEX;
        /*initialize these two variables. Note that upperlistf starts out WORDSPERVETEX words in, because
		  the first vertex in upperlist is one that was set when upperlist was lowerlist, if you see what
		  I mean*/

	  
	  currowpoint=startingpoint;
	    /*initializes currowpoint*/
	  
	  texycoord=(whichrow % DATAPOINTSPERSTRIP)*((gfloat)TEXELSPERSTRIP/((gfloat) DATAPOINTSPERSTRIP));
	    /*calculates the y texture coordinate, which does not change as we move along in the x direction*/
	  
	  do {
	    curdataindex=whichrow*ACTUALDATAWIDTH+x;
		  /*curdataindex stores the offset into the actual memory where the data is stored*/
		
		curpointtime=(data[curdataindex]+curtime)&1023;
		  /*this tells us how far from the origin we are, adjusted for the time variable, which is
		    cycled to produce the animated "rippling" motion*/
			
		
	    curdatavalue=dataheights[curpointtime];
		  /*this is the current height*/
		
		if ((whichrow == DATAYRES/2) && (x==DATAXRES/2)) {
		  curnormal.x=0;
		  curnormal.y=0;
		  curnormal.z=1.0;} 
		/*if we're at the origin, we just set the normal vector to point straight up*/
		  
		  else {
		  
		
		curnormal.x=data2dnormals[curdataindex].x*normalrun[curpointtime];
		curnormal.y=data2dnormals[curdataindex].y*normalrun[curpointtime];
		curnormal.z=normalrise[curpointtime];
		  /*the normal vector is computed by combining the 2D vector with the rise and run
		    determined by the current time*/
		}
		
	
		curdatapoint.x=currowpoint.x+upvector.x*curdatavalue;
		curdatapoint.y=currowpoint.y+upvector.y*curdatavalue;
		curdatapoint.z=currowpoint.z+upvector.z*curdatavalue;
		/*the current data point is a simple linear combination*/
		
		curzfactor=1.0/(SCREENTOVIEWER+SCREENTOORIGIN-curdatapoint.z);
        /*this Z factor is used several places*/
		
        dummy=(screenstretchfactor*SCREENTOVIEWER*curdatapoint.x*curzfactor+SCREENSHIFTX);
         /*this is the on screen X coordinate*/
		 
		*lowerlistf++=(float) dummy;
		*upperlistf++=(float) dummy;
		
		dummy=(screenstretchfactor*SCREENTOVIEWER*curdatapoint.y*curzfactor+SCREENSHIFTY);
        /*this is the on screen Y coordinate*/
		
	    *lowerlistf++=(float) dummy;
		*upperlistf++=(float) dummy;

	  
	   if (lighting) {
	  
	    curcolor=lightingvector.x*curnormal.x+
		         lightingvector.y*curnormal.y+
				 lightingvector.z*curnormal.z;
	      /*this is going to be a value between -2 and 2. (It's the dot product of a unit normal
		    vector (the current normal) and a vector of length 2 (the lighting vector)*/
		
		
		if (curcolor<0) curcolor=0; 
		
		if (curcolor<1) {
		  /*if curcolor<1, then we know that we're going to want to darken the texture, so we set the
		    primite color to black, and use the alpha channel to LERP towards black*/
		
	    *lowerlistf++=(float)(0.0);
		*lowerlistf++=(float)(0.0);
		*lowerlistf++=(float)(0.0);
	
		*upperlistf++=(float)(0.0);
		*upperlistf++=(float)(0.0);
		*upperlistf++=(float)(0.0);
		
	
		
		} else {
		  /*otherwise we know we're going to want to brighten the color, so we set the 
		    primitive color to white and use the alpha channel to LERP towards white*/
		
		
		*lowerlistf++=(float)(1.0);
		*lowerlistf++=(float)(1.0);
		*lowerlistf++=(float)(1.0);
	
		*upperlistf++=(float)(1.0);
		*upperlistf++=(float)(1.0);
		*upperlistf++=(float)(1.0);
		
		}
		
		curcolor-=1;
		if (curcolor<0) curcolor=-curcolor;
		  /*this calculates the proper value for the alpha channel. If the alpha channel is 
		    zero, then the texture color will be displayed, unchanged. If the alpha channel
			is 1, then the primitive color will be displayed, unchanged.*/
		
		
		curcolor*=0.85;
		  /*this keeps things from being too dark or too bright*/
		
		*lowerlistf++=(float)(curcolor);
		*upperlistf++=(float)(curcolor);
        
		} else {
		 *lowerlistf++=(float)(0.0);
		*lowerlistf++=(float)(0.0);
		*lowerlistf++=(float)(0.0);
    	*lowerlistf++=(float)(0.0);
		*upperlistf++=(float)(0.0);
		*upperlistf++=(float)(0.0);
		*upperlistf++=(float)(0.0); 
		*upperlistf++=(float)(0.0); 
		  /*if lighting is turned off, we just stick zero into everything, and we're happy*/
		  
		  
		}  
     	
		
		
        *lowerlistf++=(float) curzfactor;
		*upperlistf++=(float) curzfactor;
          /*we then use curzfactor by itself as W*/
		
  
        dummy=TEXXCOORDFACTOR*curzfactor*x;
		  /*this is the u value. Note that it is multiplied by curzfactor*/
 
		*lowerlistf++=(float) dummy;
		*upperlistf++=(float) dummy;
		
		if (texycoord==0.0) {
		  /*if we're right on the border of a texture, then the texture coordinates are going to
		    differ between lowerlist and upperlist*/
		  *lowerlistf++= (float) TEXELSPERSTRIP*curzfactor;
		  *upperlistf++= (float) 0.0;
		  }
		else {
            dummy=texycoord*curzfactor;
		  	*lowerlistf++=(float) dummy;
		    *upperlistf++=(float) dummy;
		}
		  
		

		
		if (x==DATAXRES) break;
		  /*this is the only place where the completion condition for the loop is checked*/
		
		
		upperlistf+=WORDSPERVERTEX;
		lowerlistf+=WORDSPERVERTEX;
		  /*skip over a vertex in each list. In upperlist, these are vertices that will be filled in
		  when upperlist is lowerlist. In lowerlist, these are vertices that were filled in when lowerlist 
		  was upperlist.*/
		
		
		currowpoint.x+=forwardvector.x;
		currowpoint.y+=forwardvector.y;
		currowpoint.z+=forwardvector.z;
	    /*increment currowpoint along the base plane*/
	 	
		x+=curLODspacing;
		/*increment x*/
		
		} while (1);
        

}		

/*the following handy function was written by Gary Lake... it loads in raw
RGB images as exported from Debabelizer*/

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


main(void)
{
  ControlPadEventData cped; 
  
  TimerTicks time1, time2;
  TimeVal timeval1, timeval2;
  int curnumframes;
  
  int curbuffer=0;
    /*indexes which frame buffer we're currently writing into*/  

 int lowerlist=1, upperlist=2;
    /*indexes which two command lists are currently being assembled*/
	
  vector curpoint, xvector, yvector;
    /*curpoint reperesents the current starting point for rows in the base plane.
	  xvector and yvector are the vectors between adjacent points in the base 
	  plane underneath data points in the heightfield*/
  
  int32 a1=0, a2=0, a3=0;
    /*the three angles representing the current rotational state*/
	
  int *dummy;
    /*a dummy integer for silly optimization purposes*/
	
  int currow=0;
    /*an index into the data specifying which row we're currently on*/
  
  int done=0;
	
  int autocycle=0;
  
  int32 imagesize;
    /*the size of the raw image*/
  
  GState *gs;
  
  initializesincos();

  CreateClip3Display (&SC);
    /*this call sets up 3 frame buffers and a z buffer with nice big clipping
	  regions*/

     gs=GS_Create();
  GS_AllocLists(gs, 2, 100000);
  GS_SetView (gs, SC.sc_ViewItem);



  bigfreebuffer=SC.sc_FreeZonePtr;
  sizeofbigfreebuffer=SC.sc_FreeZoneSize;
    /*intialize our simplistic memory management*/


  if ((rawimage=(uint32*)LoadRawImage("tom.raw", &imagesize))<0) {
    printf("ERROR unable to load raw image\n");
	exit(-1);
	};
    /*loads in the raw image file and sets rawimage to point to it*/
   
  InitEventUtility(1, 0, 0);
    /*initializes the event utility... necessary for control pad input*/
  
  initializedata();
    /*allocates memory for all the data, and fills it all up*/
  
  initTEstuff();
    /*allocates a bunch of necessary triangle engine stuff*/
  
  maketxtloadlists();
    /*sets up the preamble to each command list which contains the texture loading instructions*/
	
  changeresolution(2);
    /*start us off on medium resolution*/
	
    
  SampleSystemTimeTT(&time1);
  curnumframes=0;
  
  do {
    /*this is the main loop, that runs once per frame*/

    curnumframes++;
	
	
    calcunitvectors(a1, a2, a3);
      /*fills in e1, e2 and e3 as rotated unit vectors, given angles a1, a2, a3*/
  
	curpoint.x=(e1.x*DATAMINX)+(e2.x*DATAMINY);
	curpoint.y=(e1.y*DATAMINX)+(e2.y*DATAMINY);
	curpoint.z=(e1.z*DATAMINX)+(e2.z*DATAMINY);
	  /*calculates curpoint, which starts out as one corner of the base plane under
	    the heightfield data*/
	
	
	xvector.x=e1.x*curdataxstep;
	xvector.y=e1.y*curdataxstep;
	xvector.z=e1.z*curdataxstep;
	  /*calculates xvector, which is parallel to e1*/
	
	yvector.x=e2.x*curdataystep;
	yvector.y=e2.y*curdataystep;
	yvector.z=e2.z*curdataystep;
	  /*calculates yvector, which is parallel to e2*/
	
	lightingvector.x=-SQRT2*e1.y+SQRT2*e1.z;
	lightingvector.y=-SQRT2*e2.y+SQRT2*e2.z;
	lightingvector.z=-SQRT2*e3.y+SQRT2*e3.z;
	  /*calculates the lightingvector, which is a constant vector multiplied
	    by the _inverse_ of the current rotation matrix*/
	
	
	if (displayingtom) textureaddress=rawimage; 
	  else textureaddress=(uint32*) (((uint8*) SC.sc_Bitmaps[curbuffer]->bm_Buffer)+(640*1312+960)*2);
	  /*sets the beginning address of the current texture, which is either the raw image or the actual
	    physical start of the old frame buffer*/
	
	curbuffer=(curbuffer+1)%3;
	  /*increment curbuffer. Because we're triple buffering, we never bother waiting for the vertical
	    blank, which will result in flickering only if we're running significantly faster than 60 fps*/
	
	GS_SetDestBuffer(gs, SC.sc_BitmapItems[curbuffer]);
	
	memcpy(gs->gs_ListPtr, commandlists[0], sizeofclearscreenlist*4);
    gs->gs_ListPtr+=sizeofclearscreenlist;
	/*this has the effect of sending commandlists[0], which is the screen clearing/initialization list, */
	  

	currow=0;
    do {
      /*this loop runs once per command list*/  	
	  
	  doonerow(currow, commandlists[lowerlist]+actualtxtloadlistsize, 
	    commandlists[upperlist]+actualtxtloadlistsize, curpoint, 
	    xvector,e3);
		/*we then process a row of vertices. Note that we pass pointers that are part of the
		  way into each command list, since we don't want to overwrite the texture loading
		  commands*/
		  
		
	  if (currow==0) {}
	    /*in this case, we don't have a complete list to send yet*/
	  else 
        if ((currow-curLODspacing)%DATAPOINTSPERSTRIP == 0) {
		  /*in this case, we have to do a texture load*/
		
          commandlists[lowerlist][txtloadlistaddroffset]=
		   (uint32)( ((ubyte*) textureaddress)+(texturesourcewidth*2*TEXELSPERSTRIP)*
		   ((currow-curLODspacing)/DATAPOINTSPERSTRIP));
		   /*if we're doing a texture load, we need to poke the address of the texture into the command list*/
		  
		  GS_Reserve(gs, actualtxtloadlistsize+3+(curdataxres+1)*2*WORDSPERVERTEX);
		  memcpy(gs->gs_ListPtr, commandlists[lowerlist], (actualtxtloadlistsize+3+(curdataxres+1)*2*WORDSPERVERTEX)*4);
          gs->gs_ListPtr+=actualtxtloadlistsize+3+(curdataxres+1)*2*WORDSPERVERTEX;
		  
		   /*we then send the list off*/ 
		  
		  }
		else 
		{
		  GS_Reserve(gs, 3+(curdataxres+1)*2*WORDSPERVERTEX);
		  memcpy(gs->gs_ListPtr, commandlists[lowerlist]+actualtxtloadlistsize, (3+(curdataxres+1)*2*WORDSPERVERTEX)*4);
          gs->gs_ListPtr+=3+(curdataxres+1)*2*WORDSPERVERTEX;
            /*if there's no texture load, we just send the list immediately*/

	    }
	  lowerlist=upperlist;
	  upperlist=(upperlist %(NUMCOMMANDLISTS-1))+1;
	    /*we then increment lowerlist and uppperlist*/
	  
	  if (currow==DATAYRES) break;
	    /*here's where we check the completion condition for the loop*/
		
	  currow+=curLODspacing;
	  curpoint.x+=yvector.x;
	  curpoint.y+=yvector.y;
	  curpoint.z+=yvector.z;
	    /*we then increment currow and curpoint*/
	  
	 } while (1);
	 
	 GS_SendList(gs);
	 GS_EndFrame(gs);
	 
	 GetControlPad(1, FALSE, &cped);

if (cped.cped_ButtonBits & ControlA) {
if (cped.cped_ButtonBits & ControlLeft) lighting=1;
if (cped.cped_ButtonBits & ControlRight) lighting=0;
if (cped.cped_ButtonBits & ControlUp) screenstretchfactor*=1.005;
if (cped.cped_ButtonBits & ControlDown) screenstretchfactor*=0.995;
} else
if (cped.cped_ButtonBits & ControlB) {
if (cped.cped_ButtonBits & ControlLeft) {if (curLOD!=2) changeresolution(2);}else
if (cped.cped_ButtonBits & ControlRight) {if (curLOD!=3) changeresolution(3);}else
if (cped.cped_ButtonBits & ControlUp) {if (curLOD!=4) changeresolution(4);}else
if (cped.cped_ButtonBits & ControlDown) {if (curLOD!=1) changeresolution(1);};
  } else 
if (cped.cped_ButtonBits & ControlC) {
  if (cped.cped_ButtonBits & ControlLeft) {displayingtom=0; texturesourcewidth=1312; maketxtloadlists();};
  if (cped.cped_ButtonBits & ControlRight) {displayingtom=1; texturesourcewidth=320; maketxtloadlists();};
  if (cped.cped_ButtonBits & ControlUp) autocycle=1;
if (cped.cped_ButtonBits & ControlDown) autocycle=0;

} else
if ((cped.cped_ButtonBits & ControlLeftShift) && (cped.cped_ButtonBits & ControlRightShift))
 {
 SampleSystemTimeTT(&time1);
 curnumframes=0;
 
 
 }
else 
{
if (cped.cped_ButtonBits & ControlLeft) a2+=ROTATIONSPEED;
if (cped.cped_ButtonBits & ControlRight) a2-=ROTATIONSPEED;
if (cped.cped_ButtonBits & ControlUp) a3+=ROTATIONSPEED;
if (cped.cped_ButtonBits & ControlDown) a3-=ROTATIONSPEED;
if (cped.cped_ButtonBits & ControlLeftShift) a1+=ROTATIONSPEED;
if (cped.cped_ButtonBits & ControlRightShift) a1-=ROTATIONSPEED;
if (cped.cped_ButtonBits & ControlStart) curtime+=ROTATIONSPEED*3-1;
if (cped.cped_ButtonBits & ControlX) done=TRUE;
  
  
  
  
}


if (autocycle) {curtime+=ROTATIONSPEED*3-1; a1+=ROTATIONSPEED;};
}
while (!done);
	   
SampleSystemTimeTT(&time2);

ConvertTimerTicksToTimeVal(&time1, &timeval1);
ConvertTimerTicksToTimeVal(&time2, &timeval2);
printf("%d frames displayed\n", curnumframes);
printf("starting time %d seconds %d microseconds\n", timeval1.tv_sec, timeval1.tv_usec);
printf("ending time %d seconds %d microseconds\n", timeval2.tv_sec, timeval2.tv_usec);

printf("frame rate is %d frames per second\n", 
     curnumframes*1000/(timeval2.tv_sec*1000+timeval2.tv_usec/1000-timeval1.tv_sec*1000-timeval1.tv_usec/1000));

printf("tris per second is %d\n",
   curnumframes*curdataxres*curdatayres*2/(timeval2.tv_sec*1000+timeval2.tv_usec/1000-timeval1.tv_sec*1000-timeval1.tv_usec/1000)*1000);
	   
	   
DestroyClipDisplay (&SC);

ShutDownClip (QUIT, QUIT);

}
