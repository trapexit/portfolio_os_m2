/*
	File:		TlTexture.h

	Contains:	 

	Written by:	Ranindar Reddy	 

	Copyright:	© 1994 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):


**		 <2>	 12/2/94	RRR		TlTexture tiling data
	To Do:
*/

#include "TlBasicTypes.h"

#ifndef TEXTURE
#define TEXTURE

#ifdef SPLIT_TG
// Dummy variable
#ifndef TRAM_SIZE
#define TRAM_SIZE 16384
#endif
#endif

// Texure map information
// This will be used later
typedef struct TexMapInfo
{
	Int16	maptype;       // 0: planar, 1: cyl, 2: sphere
	float	tilex, tiley; // texture map tiling values
	Point3D	position;     // 3D location of texture map
	float	scale;        // scale for plane, radius of
	                      // cyl. and spherical map
	float	matrix[3][4]; // transformation matrix
	float	planeWidth;
	float	planeHeight;
	float	cylHeight;
} TexMapInfo;

// TlTexture map definition	
class TlTexture
{
  public:
  	TlTexture( const char *name );
  	
#ifdef SPLIT_TG
  	TlTexture( const void *tptr, long x_Wrap, long y_Wrap, Boolean txNormalized );
#endif

  	TlTexture( const TlTexture& tex );
  	~TlTexture();
  	
  	// Data setting functions
  	void SetName( char * );
  	void SetFileName( const char* );
  	void SetxWrapMode( Boolean );
  	void SetyWrapMode( Boolean );
  	void SetTexNormalized( Boolean );
  	
  	// Data getting fuctions
  	inline const char* GetName() const
  	{ return t->texName; };
  	const char* GetFileName() const;
  	const char* GetOrigFileName() const;
  	TlTexture* GetNextTexture() const;
  	Boolean HasNullData() const;	
  	void SetNextTexture( TlTexture *tex );
  	
  	TlTexture& operator=( const TlTexture& tex );
  	Boolean operator==(const TlTexture& tex) const;
  	ostream& WriteSDF( ostream& );
	ostream& WriteSDF1( ostream& os );  
	int SetRefID( int id );
	inline int GetRefID()
		{ return ( t->refID ); }
		
#ifdef SPLIT_TG
  	unsigned int Size( float xmin, float ymin, 
  	                   float xmax, float ymax );
  	long GetTxSize(void);

  	// TlTexture files functions
  	short PrepareTxAttribute(void);
  	float GetTxRegionSize(float x0, float y0, float x1, float y1);
  	short GetWidth(void);
  	short GetHeight(void);
  	short GetDepth(void);
  	short GetLod(void);
	void *CreateSubTexture(char *dst_txb, short x0, short y0, short subw, short subh);
	void BeginSubTexture(void);
	void EndSubTexture(void);
#endif

	Boolean GetxWrapMode(void);
	Boolean GetyWrapMode(void);
	Boolean GetTexNormalized(void);

  public:
  	struct TextureEntry {
  		int refCount;
		int refID;      // Shared texture ID  
				
#ifdef SPLIT_TG
  		void *txPtr;
#endif

  		char *texName;
  		char *fileName;
  		char *origFileName;
		Boolean xWrap;
		Boolean yWrap;
		Boolean txNormalized;
  		TextureEntry() { refCount = 1; }
  	};
  	TextureEntry *t;
  		
  	TlTexture *nextTexture;

// <Reddy 4-3-95> clean this mess and move this stuff into 
// TextureEntry later 	
#ifdef SPLIT_TG
	char	errflag;
	short	xorigin;
	short	yorigin;
  	short	width;
  	short	height;
  	short	depth;
  	short	lod;
  	short	round;
  	short	HasBilinear;
#endif
};

# endif // TEXTURE
