
/******************************************************************************
**
**  @(#) hf.h 96/02/22 1.10
**
**  Data structures used by the height field object
**
******************************************************************************/

#ifndef _HFIELD_H
#define _HFIELD_H


typedef struct HFMaterial {
	TexBlend	*txb;
	CltSnippet	useSnip;
	CltSnippet	tabSnip;
    float	uscale, vscale; /* 1 grid unit in texture space */
    uint16 	usize, vsize;   /* Size of repeating cel
			      >= uscale, vscale
			      Must be Power of 2 for
			      wrap to work */
} HFMaterial;

typedef struct HFPacket {
    gfloat h;
    uint8 r,g,b,a;
    uint16 mat1, mat2;
} HFPacket;

#define CHUNK_HF      CHAR4LITERAL('H','F','L','D')

typedef GfxObj	HField;

typedef struct HFData {
    SurfaceData parent;
    Box3		bound;	/* Min and Max for rendered set */
    int16		xstart, ystart; /* Starting coord */
	gfloat		deltax, deltay;	/* Size of the grid in model coords */
    uint16		rx, ry;	/* Rendered width height */
    uint16		nx, ny;	/* Real width height */
    HFMaterial	*mattable;/* Array of materials used */
	uint32		nmats;
    HFPacket	*fdata;
	uint32		*clist;	/* Command list to send to TE */
	uint32		nwordsclist; /* Number of words in clist */
} HFData;

typedef struct HFHeader {
	int 	nrows, ncols;
	float 	deltax, deltay;
	int		ntex;
} HFHeader;

extern void optDrawHF (GPData *gp, HFData *hf);
extern void optDrawHFClip (GPData *gp, HFData *hf);
extern void InitHFieldClass(void);
extern void HField_ComputeBound(HFData *field);

extern int32 GFX_HField;

#endif /*_HFIELD_H*/
