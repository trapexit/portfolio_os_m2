/*
 *	@(#) display.c 96/04/10 1.39
 *	Copyright 1995,1996 The 3DO Company
 *
 * Code to put pictures up on the display.
 */

#include "kernel/types.h"
#include "dipir/hw.storagecard.h"
#include "dipir.h"
#include "insysrom.h"

/*****************************************************************************
  Defines
*/

#define	RectWidth(rect)		(rect.UR.x - rect.LL.x)
#define	RectHeight(rect)	(rect.UR.y - rect.LL.y)
#define	BitsPerByte		8

/* Banner rectangle is horizontally centered, below the vertical center. */
#define	BANNER_XPOS	((theBootGlobals->bg_LinePixels - BANNER_WIDTH) / 2)
#define	BANNER_YPOS	(theBootGlobals->bg_ScrnLines / 2)
/* Icon rectangle is above vertical center, right of horizontal center. */
#define	ICON_XPOS	(theBootGlobals->bg_LinePixels / 2)
#define	ICON_YPOS	((theBootGlobals->bg_ScrnLines / 2) - ICON_RECT_HEIGHT)

/*****************************************************************************
  Types
*/


/*****************************************************************************
  Macros
*/

/*
 * Extract the i-th bitfield from buf.  
 * Buf is treated as an array of elements, each "sz" bits long.
 * sz may be 1 to 32.
 */
#define	GET_FIELD8(buf,i,sz) \
	(((uint8)(((uint8*)buf)[(uint32)(i)*(uint32)(sz)/8L]) >> \
		((8L-(sz))-(((uint32)(i)*(uint32)(sz))%8))) & ((1L<<(sz))-1))

#define	GET_FIELD16(buf,i,sz) \
	(((uint16)(((uint16*)buf)[(uint32)(i)*(uint32)(sz)/16L]) >> \
		((16L-(sz))-(((uint32)(i)*(uint32)(sz))%16))) & ((1L<<(sz))-1))

#define	GET_FIELD(buf,i,sz) \
	(((sz) <= 8) ? GET_FIELD8(buf,i,sz) : GET_FIELD16(buf,i,sz))


/*****************************************************************************
  External variables
*/


/*****************************************************************************
  External functions
*/


/*****************************************************************************
  Local functions - forward declarations
*/


/*****************************************************************************
  Local Data
*/
/*
 * Standard color lookup tables.
 */
static const uint16 StdClut1[] = { 0x0000, 0x7FFF };


/*****************************************************************************
  Code follows...
*****************************************************************************/


/*****************************************************************************
 CanDisplay
 Is it ok to use the video display right now?
*/
	Boolean
CanDisplay(void)
{
	return (theBootGlobals->bg_DipirFlags & DF_VIDEO_INUSE) == 0;
}

/*****************************************************************************
*/
	static uint32
GetPixelClut(uint8 *bitmap, VideoPos pos, 
		uint32 width, uint32 depth, const uint16 *clut)
{
	uint32 color;

	bitmap += (width * depth * pos.y) / BitsPerByte;
	color = GET_FIELD(bitmap, pos.x, depth);
	if (clut != NULL)
		color = clut[color];
	return color;
}

/*****************************************************************************
 GetPixel
 Get the value (color) of a specific pixel in a VideoImage.
*/
	static uint32
GetPixel(VideoImage *image, VideoPos pos)
{
	uint8 *bitmap;
	const uint16 *clut;

	switch (image->vi_Type)
	{
	case VI_DIRECT:
		bitmap = ((uint8*)image) + sizeof(struct VideoImage);
		return GetPixelClut(bitmap, pos, 
				image->vi_Width, image->vi_Depth, NULL);
	case VI_CLUT:
		bitmap = ((uint8*)image) + sizeof(struct VideoImage);
		clut = (const uint16 *) bitmap;
		bitmap += *clut++;
		return GetPixelClut(bitmap, pos, 
				image->vi_Width, image->vi_Depth, clut);
	case VI_STDCLUT:
		bitmap = ((uint8*)image) + sizeof(struct VideoImage);
		switch (image->vi_Depth)
		{
		case 1: clut = StdClut1; break;
		default: 
			EPRINTF(("STDCLUT invalid with depth %d\n", 
				image->vi_Depth));
			clut = NULL;
		}
		return GetPixelClut(bitmap, pos, 
				image->vi_Width, image->vi_Depth, clut);
	}
	return 0; /* BLACK */
}

/*****************************************************************************
 SetPixel
 Set the value (color) of a specific pixel in the video display.
*/
#define	FastSetPixel(pos,color) \
	*(((uint16*)(theBootGlobals->bg_DisplayPointer)) + \
	 (pos.y * theBootGlobals->bg_LinePixels) + pos.x) = (color)

	void
SetPixel(VideoPos pos, uint32 color)
{
	FastSetPixel(pos, color);
}

/*****************************************************************************
 FillRect
 Display a solid rectangle of a specified color.
*/
	int32
FillRect(VideoRect rect, uint32 color)
{
	VideoPos pos;

	if (!CanDisplay())
		return 0;
	for (pos.y = rect.LL.y;  pos.y < rect.UR.y;  pos.y++)
	for (pos.x = rect.LL.x;  pos.x < rect.UR.x;  pos.x++)
	{
		FastSetPixel(pos, color);
	}
	FlushDCacheAll();
	return 0;
}

/*****************************************************************************
 DisplayImage
 Display a VideoImage on the display, at a specified screen position.
*/
	int32
DisplayImage(VideoImage *image, VideoPos pos, uint32 expand, uint32 attr, uint32 attr2)
{
	VideoPos ipos;
	VideoPos vpos;
	uint32 i, k, mk;
	int32 ix, iy;
	int32 vx, vy;
	uint32 iwidth;
	uint32 iheight;

	TOUCH(attr);
	TOUCH(attr2);
	PRINTF(("DisplayImage: type %x, %dx%d depth %d\n",
		image->vi_Type, image->vi_Width, image->vi_Height, 
		image->vi_Depth));
	if (!CanDisplay())
		return 0;

	if (expand == 0)
		expand = 1;
	iwidth = image->vi_Width * expand;
	iheight = image->vi_Height * expand;
	/* Spiral out from center, for better visual effect */
	mk = max(iwidth, iheight) + 3;
	ix = iwidth / 2;
	iy = iheight/ 2;

#define	COPYPIXEL(yinc,xinc) { \
		vx = ix + pos.x;  vy = iy + pos.y; \
		if (ix >= 0 && ix < iwidth && \
		    iy >= 0 && iy < iheight && \
		    vx >= 0 && vx < theBootGlobals->bg_LinePixels && \
		    vy >= 0 && vy < theBootGlobals->bg_ScrnLines) \
		{ \
			ipos.x = ix / expand;  ipos.y = iy / expand; \
			vpos.x = vx;  vpos.y = vy; \
			FastSetPixel(vpos, GetPixel(image, ipos)); \
		} \
		ix += (xinc);  iy += (yinc); \
	}
/* end COPYPIXEL */

	for (k = 1;  k < mk;  k += 2)
	{
		for (i = 0;  i < k;  i++)
			COPYPIXEL(0,+1)
		ix--;
		iy--;
		for (i = 0;  i < k;  i++)
			COPYPIXEL(-1,0);
		ix--;
		iy++;
		for (i = 0;  i < k;  i++)
			COPYPIXEL(0,-1);
		ix++;
		iy++;
		for (i = 0;  i < k;  i++)
			COPYPIXEL(+1,0);
	}
	FlushDCacheAll();
	return 0;
}

/*****************************************************************************
 DisplayBanner
 Display an application banner image.
*/
	int32
DisplayBanner(VideoImage *image, uint32 attr, uint32 attr2)
{
	VideoPos pos;

	if (memcmp(image->vi_Pattern, VI_APPBANNER, sizeof(image->vi_Pattern)) != 0)
	{
		/* Hey, this isn't a banner! */
		EPRINTF(("Bad banner pattern\n"));
		return -1;
	}
	if (image->vi_Width > BANNER_WIDTH ||
	    image->vi_Height > BANNER_HEIGHT)
	{
		EPRINTF(("Banner too big\n"));
		return -1;
	}

	pos.x = BANNER_XPOS + (BANNER_WIDTH - image->vi_Width) / 2;
	pos.y = BANNER_YPOS + (BANNER_HEIGHT - image->vi_Height) / 2;
	return DisplayImage(image, pos, 1, attr, attr2);
}

/*****************************************************************************
 InitIcons
 Initialize the icon display (erase any icons currently displayed).
*/
	void
InitIcons(void)
{
	theBootGlobals->bg_NumIcons = 0;
}

/*****************************************************************************
 DisplayIcon
 Display an icon.
*/
	int32
DisplayIcon(VideoImage *image, uint32 attr, uint32 attr2)
{
	uint32 row;
	uint32 col;
	VideoPos pos;
	VideoRect rect;
	uint32 expand;
	uint32 xborder;
	uint32 yborder;

	if (memcmp(image->vi_Pattern, VI_ICON, sizeof(image->vi_Pattern)) != 0)
	{
		/* Hey, this isn't an icon! */
		EPRINTF(("Invalid icon pattern\n"));
		return -1;
	}

	if (image->vi_Height > ICON_HEIGHT ||
	    image->vi_Width > ICON_WIDTH)
	{
		EPRINTF(("Icon too big (%xx%x) > (%xx%x)\n",
			image->vi_Height, image->vi_Width, 
			ICON_HEIGHT, ICON_WIDTH));
		return -1;
	}

	/*
	 * Display the icon in the Icon Rectangle, 
	 * at the first available position.
	 */
	if (theBootGlobals->bg_NumIcons >= NUM_X_ICONS * NUM_Y_ICONS)
		theBootGlobals->bg_NumIcons = 0;
	row = theBootGlobals->bg_NumIcons / NUM_X_ICONS;
	col = NUM_X_ICONS - 1 - theBootGlobals->bg_NumIcons % NUM_X_ICONS;
	theBootGlobals->bg_NumIcons++;

	pos.x = ICON_XPOS + (col * (ICON_WIDTH + ICON_X_SPACING));
	pos.y = ICON_YPOS + (row * (ICON_HEIGHT + ICON_Y_SPACING));
	/* Magnify the icon if it is much smaller than the max size. */
	expand = min(ICON_WIDTH / image->vi_Width,
			ICON_HEIGHT / image->vi_Height);

	/* Draw the border rectangle */
	xborder = (ICON_WIDTH - (image->vi_Width * expand)) / 2;
	yborder = (ICON_HEIGHT - (image->vi_Height * expand)) / 2;
#ifdef SIMPLE_ICON_BORDER
	/* Just fill the whole icon rectangle. */
	rect.LL = pos;
	rect.UR.x = pos.x + ICON_WIDTH;
	rect.UR.y = pos.y + ICON_HEIGHT;
	FillRect(rect, ICON_BORDER_COLOR);
#else /* SIMPLE_ICON_BORDER */
	/* Fill just the border area around the actual icon. */
#define	DRAW_RECT(LLx,LLy, URx,URy) \
	rect.LL.x = pos.x + (LLx);  rect.LL.y = pos.y + (LLy); \
	rect.UR.x = pos.x + (URx);  rect.UR.y = pos.y + (URy); \
	FillRect(rect, ICON_BORDER_COLOR);

	DRAW_RECT(0,0, xborder,ICON_HEIGHT)
	DRAW_RECT(xborder,0, ICON_WIDTH-xborder,yborder)
	DRAW_RECT(ICON_WIDTH-xborder,0, ICON_WIDTH,ICON_HEIGHT)
	DRAW_RECT(xborder,ICON_HEIGHT-yborder, ICON_WIDTH-xborder,ICON_HEIGHT)
#endif /* SIMPLE_ICON_BORDER */
	/* Draw the icon, centered in the border rectangle. */
	pos.x += xborder;
	pos.y += yborder;
	return DisplayImage(image, pos, expand, attr, attr2);
}

/*****************************************************************************
*/
	int32
InternalIcon(uint32 iconID, VideoImage *newIcon, uint32 newSize)
{
	VideoImage **p;
	int32 iconSize;

	extern VideoImage *internalIcons[];

	for (p = internalIcons;  *p != NULL;  p++)
		if ((*p)->vi_ImageID == iconID)
		{
			iconSize = sizeof(VideoImage) + (*p)->vi_Size;
			if (iconSize > newSize)
				return -iconSize;
			memcpy(newIcon, *p, iconSize);
			return iconSize;
		}
	PRINTF(("InternalIcon(%x): icon not found\n", iconID));
	return 0;
}

/*****************************************************************************
*/
      int32
ReplaceStorageCardIcon(DipirHWResource *dev, VideoImage *newIcon, uint32 newSize)
{
#ifdef MANY_STORCARD_ICONS
	HWResource_StorCard *sc;

	sc = (HWResource_StorCard *)(dev->dev.hwr_DeviceSpecific);
	PRINTF(("ReplaceStorCardIcon (memsize %x)\n", sc->sc_MemSize));
	switch (sc->sc_MemSize)
	{
	case 64*1024:
		return InternalIcon(VI_STORCARD_64K_ICON, newIcon, newSize);
	case 128*1024:
		return InternalIcon(VI_STORCARD_128K_ICON, newIcon, newSize);
	case 256*1024:
		return InternalIcon(VI_STORCARD_256K_ICON, newIcon, newSize);
	case 512*1024:
		return InternalIcon(VI_STORCARD_512K_ICON, newIcon, newSize);
	case 1024*1024:
		return InternalIcon(VI_STORCARD_1M_ICON, newIcon, newSize);
	case 2*1024*1024:
		return InternalIcon(VI_STORCARD_2M_ICON, newIcon, newSize);
	}
	return 0;
#else
	TOUCH(dev);
	return InternalIcon(VI_STORCARD_GEN_ICON, newIcon, newSize);
#endif
}

/*****************************************************************************
*/
	int32
ReplaceVisaIcon(DipirHWResource *dev, VideoImage *newIcon, uint32 newSize)
{
	PRINTF(("ReplaceVisaIcon\n"));
#ifdef MANY_VISA_ICONS
	if (strncmp(dev->dev.hwr_Name+4, "00\\", 3) == 0)
		return InternalIcon(VI_VISA_0_ICON, newIcon, newSize);
	if (strncmp(dev->dev.hwr_Name+4, "01\\", 3) == 0)
		return InternalIcon(VI_VISA_1_ICON, newIcon, newSize);
	if (strncmp(dev->dev.hwr_Name+4, "02\\", 3) == 0)
		return InternalIcon(VI_VISA_2_ICON, newIcon, newSize);
	if (strncmp(dev->dev.hwr_Name+4, "03\\", 3) == 0)
		return InternalIcon(VI_VISA_3_ICON, newIcon, newSize);
	return 0;
#else
	TOUCH(dev);
	return InternalIcon(VI_VISA_GEN_ICON, newIcon, newSize);
#endif
}

/*****************************************************************************
*/
	int32
ReplaceIcon(VideoImage *oldIcon, DipirHWResource *dev, VideoImage *newIcon, uint32 newSize)
{
	switch (oldIcon->vi_ImageID)
	{
	case VI_VENTURI_ICON:
		if (strncmp(dev->dev.hwr_Name, "Microcard00\\", 12) == 0)
			return ReplaceStorageCardIcon(dev, newIcon, newSize);
		break;
	case VI_VISA_ICON:
		if (strncmp(dev->dev.hwr_Name, "VISA", 4) == 0)
			return ReplaceVisaIcon(dev, newIcon, newSize);
		break;
	}
	return 0;
}

/*****************************************************************************
 Convert a VideoImage to external form.
 Only difference is that the CLUT, if any, becomes 32 bit instead of 16 bit.
*/
	int32
MakeExternalImage(VideoImage *image, uint32 bufLen)
{
	uint32 imageSize;
	uint32 clutSize;
	uint16 *clut;
	uint32 *clut32;
	uint32 i;
	uint32 color;
	uint32 red, green, blue;

	imageSize = image->vi_Size + sizeof(VideoImage);
	if (image->vi_Type != VI_CLUT)
	{
		/* No CLUT in the image; nothing to do. */
		return imageSize;
	}

	/* Expand the CLUT values to 32 bits. */
	clut = (uint16 *) (image + 1);
	clut32 = (uint32 *) clut;
	clutSize = *clut;
	/*
	 * New CLUT is double the size of the old one,
	 * so increase the image size by the CLUT size.
	 */
	imageSize += clutSize;
	if (bufLen < imageSize)
	{
		/* Buffer is not big enough to hold the expanded CLUT. */
		return -imageSize;
	}
	/* Make room in the buffer for the new 32 bit CLUT. */
	memcpy((uint8*)clut + clutSize, clut, clutSize + image->vi_Size);
	/* Point to the new location of the 16 bit CLUT. */
	clut += clutSize / sizeof(uint16);
	/* Store new CLUT size. */
	*clut32++ = clutSize * (sizeof(uint32)-sizeof(uint16));
	/* Expand the color range from 5 bits to 8 bits. */
#define	EXPAND_COLOR(c)	((8*(c)) + (((c)/4) & 7))
	for (i = 1;  i < clutSize / sizeof(uint16);  i++)
	{
		color = *++clut;
		red   = (color >> 10) & 0x1F;
		green = (color >> 5) & 0x1F;
		blue  = color & 0x1F;
		*clut32++ =
			(EXPAND_COLOR(red) << 16) +
			(EXPAND_COLOR(green) << 8) + 
			 EXPAND_COLOR(blue);
	}
	return imageSize;
}

