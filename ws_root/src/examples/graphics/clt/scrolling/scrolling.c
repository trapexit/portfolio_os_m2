/******************************************************************************
**
**  @(#) scrolling.c 96/08/25 1.10
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

#include <kernel/debug.h>
#include <kernel/operror.h>

#include <assert.h>
#include <strings.h>

#include "math.h" /*this file contains single precision sin, cos and sqrt functions*/
				
#include "clip3.h"	/*this file contains the clipping code written by Gary Lake. This is
                     *a special version of the code which generates 3 screen buffers,
					 *for triple buffering
					 */
#include "AsyncReadFile.h"

#define NUMCOMMANDLISTS 2 /*one for the initialization list and one for the scrolling layers list*/

#define BORDERWIDTH 0 
  /*this constant defines a border around the screen which won't be drawn. If
   *the performance starts falling below 60fps for some reason, changing this to 5 or 10 should help.
   *This value must be less than the width of one of the strips that the screen is divided into (20)
   */
 


#define CLIPWINDOWMINX 960
#define CLIPWINDOWMAXX 1280

#define CLIPWINDOWMINY 640
#define CLIPWINDOWMAXY 880

#define NUMLAYERS 11 /*the number of scrolling layers*/

#define COMMANDSPERLAYER 5000 
  /*an approximation to the number of words in the command list
   *for each layer. This is probably way bigger than necessary
   */
   
#define COMMANDLISTSIZE NUMLAYERS*COMMANDSPERLAYER



int texaddressoffsets[NUMLAYERS][12];
  /*this is a key array. It stores offsets into the command lists that contain the locations of
   *the various texture addresses. Knowing these offsets allows us to poke new texture addresses
   *directly into our lists without reassembling the entire lists
   */
   
int layerpositions[NUMLAYERS][2];
int layerdirections[NUMLAYERS][2];


CmdListP commandlists[NUMCOMMANDLISTS]; 
    /*our command list pointers*/

uint8* bigfreebuffer; 
   /*this is a pointer to the big empty buffer that we get from the
    *clipping code
	*/
  
int sizeofbigfreebuffer; 

ScreenContext	SC; 
    /*this struct contains all the info for the triple buffers*/

uint16 *rawimage;

uint32 pip[256];
    /*this array contains our PIP*/
 
int sizeofinitlist; /*we need to store the size of the initialization list*/ 
int sizeoflayerlist; /*we also need to store the size of the big real list*/
 
  void reporterror(char *errorstring)
{
  printf("ERROR : %s\n", errorstring);
  exit(-1);
}  
 
void initializetextureandpip(void)
  /*this is a very kludgy function. The only art I have is uncoded, so this reads the uncoded 
   *image while in memory, creates a pip, turns it into coded art, and sticks in the nice diagonal
   *stripes for the pallette cycling
   */
   
{
  int x,y,i, r, g, b;
  bool found;
  int curnumofpipentries=1;
  
  for (i=0;i<256;i++) pip[i]=0;
  
  for (y=0;y<480;y++) for (x=0;x<640;x++) {
    if (rawimage[y*640+x]==0) {} else
	if (rawimage[y*640+x]==31) {
	  rawimage[y*640+x]=((x+y)%128)+128;} else
	{
	  found=FALSE;
	  for (i=0;i<256;i++)
	    if (pip[i]==rawimage[y*640+x]) {
		  found=TRUE;
		  rawimage[y*640+x]=i;
		};
	  if (!found) {
	    pip[curnumofpipentries]=rawimage[y*640+x];
		rawimage[y*640+x]=curnumofpipentries++;
		}
     }		
	}	
   for (i=0;i<256;i++) {
     b=pip[i]%32;
	 g=(pip[i]>>5)%32;
	 r=(pip[i]>>10)%32;
	 pip[i]=(b<<3)+(g<<11)+(r<<19);
	 }
}	 
 


 
void makecommandlists(void) 
  /*This function fills up the command lists with all the proper commands to draw the
   *screens. It also sets the offsets into the lists to the correct values
   */
  
  {
  uint32* listptr;
  int i;
  int curlayer, curstrip;
 
  for (i=1;i<NUMCOMMANDLISTS;i++) {
    
  
    listptr=commandlists[i];

  
  
    for (curlayer=0;curlayer<NUMLAYERS;curlayer++) {
   
      if (curlayer==1) {CLT_DBDISCARDCONTROL(&listptr,0,0,1,0); }
        /*for the bottom layer, we have black pixels set to opaque, which results
		 *in the screen being cleared. After that, however, we want black pixels to
		 *be transparent*/
		
      CLT_TXTCONST2(&listptr, (255*(curlayer+1))/(NUMLAYERS), 
        (255*(curlayer+1))/(NUMLAYERS), 
	    (255*(curlayer+1))/(NUMLAYERS),0, 0);
	
      CLT_TXTCONST3(&listptr, (255*(curlayer+1))/(NUMLAYERS), 
       (255*(curlayer+1))/(NUMLAYERS), 
       (255*(curlayer+1))/(NUMLAYERS),0, 0);
	    /*these two constants are multiplied by the texture color in the texture
		 *application blending. We set them here to get the back layers to fade out
		 */
	
	 	
	  for (curstrip=0;curstrip<12;curstrip++) {
	    
		texaddressoffsets[curlayer][curstrip]=((uint32) listptr - (uint32) commandlists[i])/sizeof(uint32)+1;
          /*we save a variable that tells us how many words into 
		   *the list the address of the texture is found, so that 
		   *we can poke into the list directly to change the address
		   */
 
        CLT_TXTLDSRCADDR(&listptr, 0);
          /*the actual address set here is a dummy since it will 
		   *always be overwritten
		   */
  
        CLT_TXTLDWIDTH(&listptr, (320-2*BORDERWIDTH)*16, 640*16);
          /*this register, during texture loads, contains the 
		   *width of the texture (in bits) and the
	       *width of the source bitmap from which the
		   *texture is being read, in bits
		   */
	  
         if ((curstrip==11) || (curstrip==0)) 
		   {CLT_TXTCOUNT(&listptr, 20-BORDERWIDTH);}
           else {CLT_TXTCOUNT(&listptr, 20);}
           /*the TXTCOUNT register, during texture loads, specifies how 
		    *many rows of texels to load in
			*/
			
	     CLT_TxLoad(&listptr);
           /*the actual command to do the texture load*/
  
         if ((curstrip==11) || (curstrip==0)) {
	       CLT_TXTUVMAX(&listptr, 319-2*BORDERWIDTH, 19-BORDERWIDTH);}
	       else {CLT_TXTUVMAX(&listptr, 319-2*BORDERWIDTH,19);}
           /*now that the texture load is over, this register specifies the
		    *width and height of the texture, minus one
		    */
   
         if (curstrip==0) {
           CLT_TRIANGLE(&listptr, 1, RC_STRIP, 0, 1, 0, 4);
           CLT_VertexUv(&listptr, CLIPWINDOWMINX+BORDERWIDTH, CLIPWINDOWMINY+BORDERWIDTH, 0,0);
           CLT_VertexUv(&listptr, CLIPWINDOWMINX+BORDERWIDTH, CLIPWINDOWMINY+20.0, 0, 20-BORDERWIDTH);
           CLT_VertexUv(&listptr, CLIPWINDOWMAXX-BORDERWIDTH, CLIPWINDOWMINY+BORDERWIDTH,320-2*BORDERWIDTH, 0);
           CLT_VertexUv(&listptr, CLIPWINDOWMAXX-BORDERWIDTH, CLIPWINDOWMINY+20.0, 320.0-2*BORDERWIDTH, 20-BORDERWIDTH);
		   } else
         if (curstrip==11) {
           CLT_TRIANGLE(&listptr, 1, RC_STRIP, 0, 1, 0, 4);
		   CLT_VertexUv(&listptr, CLIPWINDOWMINX+BORDERWIDTH, CLIPWINDOWMINY+220.0, 0,0);
           CLT_VertexUv(&listptr, CLIPWINDOWMINX+BORDERWIDTH, CLIPWINDOWMINY+240.0-BORDERWIDTH, 0,20-BORDERWIDTH);
           CLT_VertexUv(&listptr, CLIPWINDOWMAXX-BORDERWIDTH, CLIPWINDOWMINY+220.0,320.320-2*BORDERWIDTH, 0);
           CLT_VertexUv(&listptr, CLIPWINDOWMAXX-BORDERWIDTH, CLIPWINDOWMINY+240.0-BORDERWIDTH, 320.0-2*BORDERWIDTH, 20-BORDERWIDTH);
           } else {
           CLT_TRIANGLE(&listptr, 1, RC_STRIP, 0, 1, 0, 4);
           CLT_VertexUv(&listptr, CLIPWINDOWMINX+BORDERWIDTH, CLIPWINDOWMINY+curstrip*20.0,     0.0,0.0);
           CLT_VertexUv(&listptr, CLIPWINDOWMINX+BORDERWIDTH, CLIPWINDOWMINY+(curstrip+1)*20.0, 0.0,20.0);
           CLT_VertexUv(&listptr, CLIPWINDOWMAXX-BORDERWIDTH, CLIPWINDOWMINY+curstrip*20.0,     320.0-2*BORDERWIDTH, 0.0);
           CLT_VertexUv(&listptr, CLIPWINDOWMAXX-BORDERWIDTH, CLIPWINDOWMINY+(curstrip+1)*20.0, 320.0-2*BORDERWIDTH, 20.0);
		   }
             /*note that we are using CLT_VertexUv, because we are passing in texture coordinates
			  *but no height value and no color. Note also that with perspective correction turned
			  *off, we simply input the actual texture coordinates with no extra futzing
			  */
 
         CLT_Sync(&listptr);
         }
       }
  
	 
	 sizeoflayerlist=listptr-commandlists[i];
	 printf("layer command list is %d words long\n", sizeoflayerlist);
     }	
   }
  
  
void updatepip(int t)
  /*this function cycles the colors for pip indices 128-255, which are used for
   *the color cycling
   */
 { int i,t2, t3;
   if ((t%512)>255) t2=511-(t%512); 
   else t2=t%256;
   if (((t>>2)%512)>255) t3=511-((t>>2)%512); 
   else t3=(t>>2)%256; 
   for (i=128;i<256;i++) pip[i]=((i+2*t)%128)*2+1+(t2<<8)+(t3<<16);
 }  
  
void makeinitlist(void)
  /*this procedure assembles the command list that sets up all the
   *necessary TE registers and loads in the PIP. It is called once per
   *frame, but really, the only part of it that needs to be called 
   *repeatedly is the PIP load
   */
  
{
  uint32* listptr;
  
  listptr=commandlists[0];
 
  CLT_DBUSERCONTROL(&listptr, 0,0,0,1,0,0,0,15);
    /*set the general purpose register to Z Buffering off, Z Buffer output off, 
	 *clipping only outside the clip region, blending disabled, secondary source disabled,
	 *dithering disabled, and RGBA all being output
	 */
	  
  CLT_DBXWINCLIP(&listptr, CLIPWINDOWMINX, CLIPWINDOWMAXX);
  CLT_DBYWINCLIP(&listptr, CLIPWINDOWMINY, CLIPWINDOWMAXY);
   /*set the clipping region to the proper values for the triple 
    *buffered clipping buffers. (Note that no clipping is necessary in this program.
	*I just set these registers as a matter of habit.)
	*/
       
  CLT_DBDISCARDCONTROL(&listptr,0,0,0,0);
   /*discard no pixels (only true for the bottom layer of each frame)*/
 
  CLT_DBCONSTIN(&listptr, 0,0,0);
   /*set the constant color for the destination blending to black*/
  
  CLT_DBAMULTCONSTSSB0(&listptr, 0xff, 0xff, 0xff);
  CLT_DBAMULTCONSTSSB1(&listptr, 0xff, 0xff, 0xff);
  CLT_DBBMULTCONSTSSB0(&listptr, 0xff, 0xff, 0xff);
  CLT_DBBMULTCONSTSSB1(&listptr, 0xff, 0xff, 0xff);
    /*set all the destination blending multipliers to 0xff, which counts as 1*/
	

  CLT_DBALUCNTL(&listptr, RC_DBALUCNTL_ALUOPERATION_A_PLUS_B, 0);
    /*sets the ALU operation to simple addition with no shift*/
  
  CLT_DBAMULTCNTL(&listptr, RC_DBAMULTCNTL_AINPUTSELECT_TEXCOLOR, 
   RC_DBAMULTCNTL_AMULTCOEFSELECT_CONST, RC_DBAMULTCNTL_AMULTCONSTCONTROL_TEXSSB, 0);
   /*sets the input to one side of the destination blending to the texture color*/

  CLT_DBBMULTCNTL(&listptr,RC_DBBMULTCNTL_BINPUTSELECT_CONSTCOLOR,
   RC_DBBMULTCNTL_BMULTCOEFSELECT_CONST, RC_DBBMULTCNTL_BMULTCONSTCONTROL_TEXSSB, 0);
   /*sets the input to the other side of the destination blending to the constant color (black)*/
   
  CLT_TXTTABCNTL(&listptr, RC_TXTTABCNTL_FIRSTCOLOR_TEXCOLOR, RC_TXTTABCNTL_SECONDCOLOR_CONSTCOLOR,
    RC_TXTTABCNTL_THIRDCOLOR_PRIMALPHA, 0, 0, RC_TXTTABCNTL_COLOROUT_BLEND,  /*RC_TXTTABCNTL_COLOROUT_TEXCOLOR,*/
	RC_TXTTABCNTL_ALPHAOUT_PRIMALPHA, RC_TXTTABCNTL_BLENDOP_MULT);
    /*sets up the texture application blending so that the texture
	 *color is multiplied by a constant color. This allows the back layers
	 *to be faded out simply
	 */
  
  CLT_TXTCONST0(&listptr, 0, 0, 0, 0, 0);
  CLT_TXTCONST1(&listptr, 0, 0, 0, 0, 0);
    /*during texture loads, these two registers may screw up the PIP offsets*/
   
  CLT_TXTLODBASE0(&listptr, 0x46000);
    /*during a PIP load, this register must be set to this value*/
	
  CLT_TXTLDCNTL(&listptr, 0, RC_TXTLDCNTL_LOADMODE_PIP, 0);
  
  CLT_TXTLDSRCADDR(&listptr, &(pip[0]));
    /*we then set the address of the PIP to load*/
  CLT_TXTCOUNT(&listptr, 1024);
    /*during a PIP load, this register specifies how many bytes to load*/
	
  CLT_TxLoad(&listptr);
    /*do the PIP load*/
	
  
  
  CLT_TXTLDCNTL(&listptr, 0, RC_TXTLDCNTL_LOADMODE_TEXTURE, 0);
    /*specifies that we're doing an uncompressed texture load with bit offset 0*/
  
  CLT_TXTADDRCNTL(&listptr, 1, RC_TXTADDRCNTL_MINFILTER_POINT,
    RC_TXTADDRCNTL_INTERFILTER_BILINEAR, RC_TXTADDRCNTL_MAGFILTER_POINT, 0);  
	/*turns texturing on, sets the filtering to point, and specifies only one 
	  LOD */

  CLT_TXTLODBASE0(&listptr, 0);
    /*this specifies that we wish to load into the very beginning (address 0) of texture RAM*/
	
  CLT_TXTEXPTYPE(&listptr, 8, 7, 0, 1, 1, 1, 0);
    /*this sets the texture type to indexed 1-7-8*/
	  
  CLT_TXTUVMASK(&listptr, 0x3ff, 0x3ff);
    /*turns off address masking (which we would turn on if we wanted a tiled texture)*/
 
  CLT_TXTPIPCNTL(&listptr, 2, 2, 2, 0); 
    /*sets the color, alpha and SSB to come from the PIP*/
 
  CLT_SetRegister(listptr, ESCNTL, FV_ESCNTL_PERSPECTIVEOFF_MASK);
    /*this undocumented register bit turns perspective correction off, which
	  allows correct texturing in 2D*/
	
  sizeofinitlist=listptr-commandlists[0];
  
  printf("initialization list is %d words long\n", sizeofinitlist);

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
	 *our command lists
	 */
	
  makeinitlist();
    /*then we initialize the screen clearing list*/  

   }


/*the following handy function was written by Gary Lake... it loads in raw
 *RGB images as exported from Debabelizer
 */

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
	if ((*ImageSizePtr =ReadRawFile (ImageFileHandle,
			ImageBuffer, ImageFileInfo.fi_ByteCount)) < 0)
	reporterror("couldn't load the image into memory\n");
	


	if ((ReturnVal = CloseRawFile (ImageFileHandle)) < 0)
		reporterror("couldn't close the raw file \n");

	
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
  
  int curbuffer=0, oldbuffer=0;
    /*indexes which frame buffer we're currently writing into*/  

  TimerTicks time1, time2;
  TimeVal timeval1, timeval2;

  int done=0;
  int i;
  int curlayer, curstrip;	
	
  uint32 *dummy;

  int imagesize;
    /*the size of the raw image*/
  
  int paused=0;
 
  int separatemovements=0;
 
 int curnumframes=0;
 
  uint32 filesignal;
  
  GState *gs; /*to do CLT graphics, you gotta have a gstate*/
  
  
  filesignal=AllocSignal(0);



 
  CreateClip3Display (&SC);
    /*this call sets up 3 frame buffers and a z buffer with nice big clipping
	  regions*/

 bigfreebuffer=SC.sc_FreeZonePtr;
  sizeofbigfreebuffer=SC.sc_FreeZoneSize;
    /*intialize our simplistic memory management*/
    
  rawimage=NULL;
  
  printf("about to call asyncrawfile\n");
  
  AsyncReadFile("scrolling.rgb", 0,(void**) &rawimage, filesignal, &imagesize);
 
 /* if ((rawimage=(uint16*)LoadRawImage("hangin.raw", &imagesize))<0) {
    printf("ERROR unable to load raw image\n");
	exit(-1);
	};*/
    /*loads in the raw image file and sets rawimage to point to it*/
  
  printf("returned from call to load\n");
  
  gs=GS_Create();
  GS_AllocLists(gs, 2, COMMANDLISTSIZE+1000);
  GS_SetView (gs, SC.sc_ViewItem);
  
  InitEventUtility(1, 0, 0);
    /*initializes the event utility... necessary for control pad input*/
 
  initTEstuff();
    /*allocates a bunch of necessary triangle engine stuff*/
   
  makecommandlists();
    /*fills up the command lists*/
	
  updatepip(0);
	
	
  for (i=0;i<NUMLAYERS;i++) {
    layerpositions[i][0]=rand() % 320;
	layerpositions[i][1]=rand() % 240;
	layerdirections[i][0]=rand() % 10 - 5;
	layerdirections[i][1]=rand() % 10 - 5;
  }
	
	
  printf("about to wait for file signal\n");
  WaitSignal(filesignal);
  printf("got file signal\n");
  
  initializetextureandpip();
  
  printf("initialized texture and pip\n");
  
  SampleSystemTimeTT(&time1);
  
  do {
    /*this is the main loop that runs once per frame*/
    curnumframes++;
	
	if (!paused) { 

#define OTHERDIRECTION(x) if (x>=0) x=-(x%10+1); else x=(-x % 10)+1;
#define NEXTDIRECTION(x) x=(x%10)+1;
	
	if (separatemovements) {
	  for (i=0;i<NUMLAYERS;i++) {
	    layerpositions[i][0]+=layerdirections[i][0];
	   layerpositions[i][1]+=layerdirections[i][1];

       if (layerpositions[i][0]<0) {
  	     layerpositions[i][0]=0;
 	     OTHERDIRECTION(layerdirections[i][0]);
	     NEXTDIRECTION(layerdirections[i][1]);
	     }
	   if (layerpositions[i][0]>320) {
	     layerpositions[i][0]=320;
	     OTHERDIRECTION(layerdirections[i][0]);
	     NEXTDIRECTION(layerdirections[i][1]);	  } 
	   if (layerpositions[i][1]<0) {
	     layerpositions[i][1]=0;
	     OTHERDIRECTION(layerdirections[i][1]);
	     NEXTDIRECTION(layerdirections[i][0]);}
	   if (layerpositions[i][1]>240) {
	     layerpositions[i][1]=240;
	     OTHERDIRECTION(layerdirections[i][1]);
         NEXTDIRECTION(layerdirections[i][0]);
       }
	  
	  
	  
	  
	  }
	
	
	} else
	{
	
	 layerpositions[NUMLAYERS-1][0]+=layerdirections[NUMLAYERS-1][0];
	 layerpositions[NUMLAYERS-1][1]+=layerdirections[NUMLAYERS-1][1];

     if (layerpositions[NUMLAYERS-1][0]<0) {
	   layerpositions[NUMLAYERS-1][0]=0;
	   OTHERDIRECTION(layerdirections[NUMLAYERS-1][0]);
	   NEXTDIRECTION(layerdirections[NUMLAYERS-1][1]);
	   }
	 if (layerpositions[NUMLAYERS-1][0]>320) {
	   layerpositions[NUMLAYERS-1][0]=320;
	   OTHERDIRECTION(layerdirections[NUMLAYERS-1][0]);
	   NEXTDIRECTION(layerdirections[NUMLAYERS-1][1]);	  } 
	 if (layerpositions[NUMLAYERS-1][1]<0) {
	   layerpositions[NUMLAYERS-1][1]=0;
	   OTHERDIRECTION(layerdirections[NUMLAYERS-1][1]);
	   NEXTDIRECTION(layerdirections[NUMLAYERS-1][0]);}
	 if (layerpositions[NUMLAYERS-1][1]>240) {
	   layerpositions[NUMLAYERS-1][1]=240;
	   OTHERDIRECTION(layerdirections[NUMLAYERS-1][1]);
       NEXTDIRECTION(layerdirections[NUMLAYERS-1][0]);
     }

     for (curlayer=0;curlayer<NUMLAYERS-1;curlayer++) {
       layerpositions[curlayer][0]=((curlayer+1)*(layerpositions[NUMLAYERS-1][0]-160))/NUMLAYERS+160;
       layerpositions[curlayer][1]=((curlayer+1)*(layerpositions[NUMLAYERS-1][1]-120))/NUMLAYERS+120;
     }

    }

   }							  
							   
   updatepip(curnumframes);
   
   curbuffer=(curbuffer+1)%3;
	
   GS_SetDestBuffer(gs, SC.sc_BitmapItems[curbuffer]);	
	
   memcpy(gs->gs_ListPtr, commandlists[0], sizeofinitlist*4);
   gs->gs_ListPtr+=sizeofinitlist;
   


     for (curlayer=0;curlayer<NUMLAYERS;curlayer++) {
	   commandlists[1][texaddressoffsets[curlayer][0]]=(uint32)
		    (rawimage+layerpositions[curlayer][0]+layerpositions[curlayer][1]*640+BORDERWIDTH*640+BORDERWIDTH);

     for (curstrip=1;curstrip<12;curstrip++) 
		  commandlists[1][texaddressoffsets[curlayer][curstrip]]=(uint32)
		    (rawimage+layerpositions[curlayer][0]+layerpositions[curlayer][1]*640+curstrip*20*640+BORDERWIDTH);
	  	}


	 WaitSignal(SC.sc_RenderSignal);
       /*we then wait for the VBL so we know that our swapping is truly complete*/

     memcpy(gs->gs_ListPtr, commandlists[1], sizeoflayerlist*4);
     gs->gs_ListPtr+=sizeoflayerlist;
   	 
     GS_SendList(gs);
      
	 GS_EndFrame(gs);
	
	    
	 GetControlPad(1, FALSE, &cped);
	 if (cped.cped_ButtonBits & ControlB) separatemovements=1; else separatemovements=0;
     if (cped.cped_ButtonBits & ControlA) paused=FALSE;
     if (cped.cped_ButtonBits & ControlStart) paused=TRUE;
     if (cped.cped_ButtonBits & ControlX) done=TRUE;
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

   DestroyClipDisplay (&SC);
   ShutDownClip (QUIT, QUIT);
}
