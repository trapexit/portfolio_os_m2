/*
	File:		TlTexture.cp

	Contains:	 

	Written by:	Ravindar Reddy	 

	Copyright:	© 1994 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):


**		 <3>	 12/2/94	RRR		TlTexture tiling data
	To Do:
*/
#include "TlTexture.h"
#include <time.h>

#ifdef SPLIT_TG
#ifdef __cplusplus
extern "C" { 
#endif

#include "M2TXlib.h"
#if !defined(macintosh)
typedef M2Err OSErr;
#endif

M2Err M2TX_CloneTXTR(char *texPtr, M2TX *tex);
M2Err M2TX_CopyTXTR(char *texPtr, M2TX *tex);
M2Err M2TX_WriteTXTR(char *TXTRPtr, M2TX *tex);

#ifdef __cplusplus
} 
#endif
#endif

extern "C" double ceil( double );
extern "C" double floor( double );

TlTexture::TlTexture( const char *name )
{
#ifdef DEBUG
	Texture_numObjs++;
#endif
	t = new TextureEntry;
	PRINT_ERROR( (t==NULL), "Out of memory" );
	t->texName = new char[ strlen( name ) + 50 ];
	PRINT_ERROR( (t->texName==NULL), "Out of memory" );
	strcpy( t->texName, name );
	
	t->fileName = NULL;
	t->origFileName = NULL;
	nextTexture = NULL;
	t->xWrap = FALSE;
	t->yWrap = FALSE;
}

#ifdef SPLIT_TG
TlTexture::TlTexture( 
	const void *tptr , 
	long x_Wrap, 
	long y_Wrap,  
	Boolean txNCoords
	)
{
#ifdef DEBUG
	Texture_numObjs++;
#endif
	t = new TextureEntry;
	PRINT_ERROR( (t==NULL), "Out of memory" );
	t->txPtr = (void *)tptr;
	t->texName = new char[ 9 ];
	PRINT_ERROR( (t->texName==NULL), "Out of memory" );
	strcpy( t->texName, "Texture" );
	
	t->fileName = NULL;
	t->origFileName = NULL;
	nextTexture = NULL;
	t->xWrap = x_Wrap;
	t->yWrap = y_Wrap;
	t->txNormalized = txNCoords;
	
	xorigin = 0;
	yorigin = 0;
  	width = 0;
  	height = 0;
  	PrepareTxAttribute();
}
#endif

TlTexture::TlTexture( const TlTexture& tex )
{
#ifdef DEBUG
	Texture_numObjs++;
#endif
	tex.t->refCount++;
	t = tex.t;
	nextTexture = NULL;
}

TlTexture::~TlTexture()
{
#ifdef DEBUG
	Texture_numObjs--;
#endif
	if ( --t->refCount == 0 ) 
	{
		delete[] t->texName;
		if ( t->fileName != NULL ) delete[] t->fileName;
		if ( t->origFileName != NULL ) delete[] t->origFileName;
		delete t;
	}
	
}

void 
TlTexture::SetNextTexture( TlTexture *tex )
{
	if ( this && tex ) nextTexture = tex;
}

// Set the texture name 
void 
TlTexture::SetName( char *name )
{
	if ( t->texName == NULL ) {
		t->texName = new char[ strlen( name ) + 1 ];
		PRINT_ERROR( (t->texName==NULL), "Out of memory" );
		strcpy( t->texName, name );
	} else if ( strlen( t->texName ) < strlen( name ) ) {
		delete[] t->texName;
		t->texName = new char[ strlen( name ) + 1 ];
		PRINT_ERROR( (t->texName==NULL), "Out of memory" );
		strcpy( t->texName, name );
	} else strcpy( t->texName, name );
}	

// Data setting functions
void 
TlTexture::SetFileName( const char* name )
{	
	char buf[100];
	if ( t->fileName == NULL ) {
		t->fileName = new char[ strlen( name ) + 5 ];
		t->origFileName = new char[ strlen( name ) + 5 ];
		PRINT_ERROR( (t->fileName==NULL), "Out of memory" );
		strcpy( t->fileName, name );
	} else if ( strlen( t->fileName ) < strlen( name ) ) {
		delete[] t->fileName;
		t->fileName = new char[ strlen( name ) + 1 ];
		t->origFileName = new char[ strlen( name ) + 5 ];
		PRINT_ERROR( (t->fileName==NULL), "Out of memory" );
		strcpy( t->fileName, name );
	} else strcpy( t->fileName, name );
	strcpy(buf,ChopString(t->fileName,'.'));
	sprintf( buf, "%s.%s", LowerCase(buf), "utf" );
	strcpy(t->fileName,buf);
	strcpy(t->origFileName,name);
}

void 
TlTexture::SetxWrapMode( Boolean val )
{
	t->xWrap = val;
}

void 
TlTexture::SetyWrapMode( Boolean val )
{
	t->yWrap = val;
}

void 
TlTexture::SetTexNormalized( Boolean val )
{
	t->txNormalized = val;
}
  	  	
// Data getting fuctions
const char* 
TlTexture::GetFileName() const
{
	return t->fileName;
}

const char* 
TlTexture::GetOrigFileName() const
{
	return t->origFileName;
}

TlTexture* 
TlTexture::GetNextTexture() const
{
	return nextTexture;
}

Boolean
TlTexture::HasNullData() const
{
	if ( t->fileName == NULL ) return TRUE;
	else return FALSE;
}

TlTexture& 
TlTexture::operator=( const TlTexture& tex )
{
	tex.t->refCount++;
	if ( --t->refCount == 0 ) 
	{
		delete[] t->texName;
		if ( t->fileName != NULL ) delete[] t->fileName;
		if ( t->origFileName != NULL ) delete[] t->origFileName;
		delete t;
	}
	t = tex.t;
	return *this;
}

Boolean 
TlTexture::operator==(const TlTexture& tex) const
{
	if ( t == tex.t ) return TRUE;
	else return FALSE;
}
	
ostream& 
TlTexture::WriteSDF( ostream& os )  
{	
	if ( t->fileName != NULL ) {
		char buf[100];
		sprintf( buf, "Use Texture \"Tex_%s\"", GetName() );
		WRITE_SDF( os, buf );
		sprintf( buf, "Use TxRender \"Rnd_%s\"", GetName() );
		WRITE_SDF( os, buf );

		// write TxBlendInfo - hack for alpha 1
		// this will go away with new texture API
		sprintf( buf, "TxBlend {" );
		BEGIN_SDF( os, buf );
		sprintf( buf, "FirstColor    ColorSelectPrimColor" );
		    WRITE_SDF( os, buf );
		    sprintf( buf, "SecondColor   ColorSelectTexColor" );
			WRITE_SDF( os, buf );
		    sprintf( buf, "FirstAlpha    AlphaSelectPrimAlpha" );
			WRITE_SDF( os, buf );
		    sprintf( buf, "SecondAlpha   AlphaSelectTexAlpha" );
			WRITE_SDF( os, buf );
		    sprintf( buf, "BlendOp       BlendOpMult" );
			WRITE_SDF( os, buf );
		    sprintf( buf, "ColorOut      BlendOutSelectBlend" );
			WRITE_SDF( os, buf );
		    sprintf( buf, "AlphaOut      BlendOutSelectTex" );
			WRITE_SDF( os, buf );
		sprintf( buf, "}" );	
		END_SDF( os, buf ); 
	}
	return os;
}

ostream& 
TlTexture::WriteSDF1( ostream& os )
{	
	if ( t->fileName != NULL ) {
		char buf[100];

		// write a comment for image size
		if ( !t->xWrap || !t->yWrap )
		{
			sprintf( buf, "# Make sure the the texture map size is powers-of-two in : %s%s",
				 ( t->xWrap ? "" : "X" ), ( t->yWrap ? "" : " Y" ) );
			WRITE_SDF( os, buf );		 
		}

		// Write texture object
		sprintf( buf, "# TexControl Tex_%s", GetName() );
		WRITE_SDF( os, buf );
		BEGIN_SDF( os, "{" );
		
		// convert to lowercase and prefix .utf
		strcpy(buf,ChopString(t->fileName,'.'));
		sprintf( buf, "fileName \"%s.%s\"", LowerCase(buf), "utf" );
		WRITE_SDF( os, buf );
				
		// write default values
		WRITE_SDF( os, "txFirstColor PrimColor" );
		WRITE_SDF( os, "txSecondColor TexColor" );
		WRITE_SDF( os, "txColorOut Blend" );
		WRITE_SDF( os, "txAlphaOut Prim" );
		WRITE_SDF( os, "txBlendOp Mult" );

		// Write render info object
		if ( !t->xWrap ) WRITE_SDF( os, "xWrap Tile" );
		else WRITE_SDF( os, "xWrap Clamp" );
		if ( !t->yWrap ) WRITE_SDF( os, "yWrap Tile" );
		else WRITE_SDF( os, "yWrap Clamp" );
		
		END_SDF( os, "}" );
	}
	return os;
}

int
TlTexture::SetRefID( int id )
{
	// clear the ID
	if ( id < 0 ) t->refID = id;
	// mark with unique ID
	else if ( t->refID < 0 ) t->refID = id;
	return t->refID;
}


#ifdef SPLIT_TG
// Dummy function for texture size
unsigned int 
TlTexture::Size( float xmin, float ymin, 
               float xmax, float ymax )
{
	float area;
	unsigned int size;
	
	{
		area = GetTxRegionSize(xmin, ymin, xmax, ymax);
		size = (int)ceil(area);
		return ( size );
	}
	
	area = ( ( xmax - xmin ) * ( ymax - ymin ) );
	area = ( area < 0 ) ? -area : area;
	
	size = (unsigned int)10000 * area;
	return ( size );
}

/****************************************************************************************
 *	PrepareTxAttribute
 *
 *	Get the header of the texture map and its image information
 ****************************************************************************************/
short TlTexture::PrepareTxAttribute(void)
{
	M2TX			tex;
	M2TXHeader		*header;
	OSErr			err;
	uint16			w, h;
	uint8			cDepth, aDepth, num;
	bool			flag;
		
	if (!t->txPtr) return(-1);

	err = M2TX_CloneTXTR((char *)t->txPtr, &tex);
	if (err != noError)		// Be safe, check for allocation failure
	{
		printf("ERROR: M2TX_CloneTXTR returned with error\n");
		return(-1);		
	}
	M2TX_GetHeader(&tex,&header);
	err = M2TXHeader_GetMinXSize(header, &w);
	err = M2TXHeader_GetMinYSize(header, &h);
	width = w;
	height = h;
	err = M2TXHeader_GetNumLOD(header, &num);
	lod = num;
	num--;
	while (num)
	{
		width = width << 1;
		height = height << 1;
		num--;
	}
	if (lod > 4)
		lod = 4;
	round = 1 << (lod - 1);
	HasBilinear = 1;					// assuming bilinear filteringis turned on
	err = M2TXHeader_GetFIsLiteral(header, &flag);
	err = M2TXHeader_GetCDepth(header, &cDepth);
	if (flag)
		depth = cDepth * 3;
	else
		depth = cDepth;
	err = M2TXHeader_GetFHasAlpha(header, &flag);
	if (flag)
	{
		err = M2TXHeader_GetADepth(header, &aDepth);
		depth += aDepth;
	}
	err = M2TXHeader_GetFHasSSB(header, &flag);
	if (flag)
		depth ++;

	// M2TXHeader_FreeLODPtrs(header);
	
	return(noError);
}

/****************************************************************************************
 *	GetTxRegionSize
 *
 *	Get the header of the texture map and its image information
 ****************************************************************************************/
float 								// size of the texture tile region
TlTexture::GetTxRegionSize
	(
	float x0,						// first point of the rectangle
	float y0,
	float x1,						// second point of the rectangle
	float y1
	)
{
	float left, right, top, bottom, tmp;
	float size = 0.0;
	long rem, i;
	Boolean xMode, yMode;
	float w = 1.0, h = 1.0;

	if ( !t->txNormalized )
	{
		w = width;
		h = height;
	}
		
	xMode = GetxWrapMode();
	yMode = GetyWrapMode();
	if (xMode)			// clamp mode
	{
		if (x0 < 0) x0 = 0.0;
		if (x1 < 0) x1 = 0.0;
		if (x0 > w) x0 = w;
		if (x1 > w) x1 = w;
	}
	if (yMode)			// clamp mode
	{
		if (y0 < 0) y0 = 0.0;
		if (y1 < 0) y1 = 0.0;
		if (y0 > h) y0 = h;
		if (y1 > h) y1 = h;
	}

	if (x0 > x1)
	{
		tmp = x0;
		x0 = x1;
		x1 = tmp;
	}
	if (y0 > y1)
	{
		tmp = y0;
		y0 = y1;
		y1 = tmp;
	}
	if ( t->txNormalized )
	{
		left = x0 * (float)width;
		right = x1 * (float)width;
		top = y0 * (float)height;
		bottom = y1 * (float)height;
	}
	else
	{
		left = x0;
		right = x1;
		top = y0;
		bottom = y1;
	}

	left = (float)floor((double)left);
	rem = (long)left;
	rem = rem % round;
	if (rem) {left = left - rem;}
	
	right = (float)ceil((double)right);
	if (HasBilinear)
		right = right + 1.0;
	rem = (long)right;
	rem = rem % round;
	if (rem) {right = right + round - rem;}
	
	top = (float)floor((double)top);
	rem = (long)top;
	rem = rem % round;
	if (rem) {top = top - rem;}
	
	bottom = (float)ceil((double)bottom);
	if (HasBilinear)
		bottom = bottom + 1.0;
	rem = (long)bottom;
	rem = rem % round;
	if (rem) {bottom = bottom + round - rem;}
	
	tmp = (right - left) * (bottom - top) * (float)depth / 8.0;
	// factor in level of detail
	for (i = 0; i < lod; i++)
	{
		size = size + tmp;
		tmp = tmp / 4.0;
	}
	return(size);
}

/****************************************************************************************
 *	GetWidth
 *
 *	Get the width of the texture at finest level
 ****************************************************************************************/
short 								// width of the texture at finest level
TlTexture::GetWidth(void)
{
	return(width);
}

/****************************************************************************************
 *	GetHeight
 *
 *	Get the height of the texture at finest level
 ****************************************************************************************/
short 								// height of the texture at finest level
TlTexture::GetHeight(void)
{
	return(height);
}

/****************************************************************************************
 *	GetDepth
 *
 *	Get the depth of the texture
 ****************************************************************************************/
short 								// height of the texture
TlTexture::GetDepth(void)
{
	return(depth);
}

/****************************************************************************************
 *	GetLod
 *
 *	Get the lod of the texture
 ****************************************************************************************/
short 								// lod of the texture
TlTexture::GetLod(void)
{
	return(lod);
}

/****************************************************************************************
 *	GetLod
 *	Total texture size
 *	Get the lod of the texture
 ****************************************************************************************/
long 								// size of the texture in bits
TlTexture::GetTxSize(void)
{
	long tmp, size = 0, i;
	
	tmp = width * height * depth;
	for (i = 0; i < lod; i++)
	{
		size = size + tmp;
		tmp = tmp / 4;
	}
	return((size+7)/8);
}

static	M2TX			baseTex;
static	M2TXHeader		*baseHeader;
static	M2TX			*newTex;
static	M2TXHeader		*newHeader;
//static	M2TXPIP 		*pip;
/****************************************************************************************
 *	CreateSubTexture
 *
 *	Get the lod of the texture
 ****************************************************************************************/
void * 								// size of the texture in bits
TlTexture::CreateSubTexture
	(
	char *texChunk,
	short x0,
	short y0,
	short subw,
	short subh
	)
{
	M2Err		err;
	
	if (errflag)
		return(NULL);
	err = M2TX_Extract(&baseTex,lod,lod-1,x0,y0,subw,subh,&newTex); 
	if (err != M2_NoErr)
	{
		errflag = TRUE;
		return(NULL);
	}
	
	// M2TX_WriteFile(fname,newTex);    			// Write it to disk
	// need to have a function to return the pointer
	M2TX_GetHeader(newTex,&newHeader);
	err = M2TX_WriteTXTR(texChunk,newTex);
	M2TXHeader_FreeLODPtrs(newHeader);
	if (err != M2_NoErr)
	{
		errflag = TRUE;
		return(NULL);
	}
	return((void *)texChunk);
}

/****************************************************************************************
 *	BeginSubTexture
 *
 *	Get the lod of the texture
 ****************************************************************************************/
void 								// size of the texture in bits
TlTexture::BeginSubTexture(void)
{			
	M2Err			err;
	
	errflag = FALSE;
	M2TX_Init(&baseTex);								// Initialize a texture
	
	//err = M2TX_ReadFile((char *)GetFileName(),&baseTex);			// Read back in the uncompressed
	err = M2TX_CopyTXTR((char *)t->txPtr,&baseTex);			// Read back in the uncompressed
	if (err != M2_NoErr)								// Be safe, check for allocation failure
		errflag = TRUE;
}

/****************************************************************************************
 *	EndSubTexture
 *
 *	Get the lod of the texture
 ****************************************************************************************/
void 								// size of the texture in bits
TlTexture::EndSubTexture(void)
{
	if (errflag)
		return;
	M2TX_GetHeader(&baseTex,&baseHeader);
	M2TXHeader_FreeLODPtrs(baseHeader);
	errflag = FALSE;
}
#endif

Boolean 
TlTexture::GetxWrapMode(void)
{
	return(t->xWrap);
}

Boolean 
TlTexture::GetyWrapMode(void)
{
	return(t->yWrap);
}

Boolean 
TlTexture::GetTexNormalized(void)
{
	return(t->txNormalized);
}
