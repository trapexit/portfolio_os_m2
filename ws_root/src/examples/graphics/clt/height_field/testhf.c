
/******************************************************************************
**
**  @(#) testhf.c 96/05/01 1.20
**
**  Test program creates a height field object and renders it
**
******************************************************************************/

#include <graphics/fw.h>
#include "hf.h"
#include "sample.h"
#include <graphics/gfxutils/putils.h>
#include <stdio.h>

extern CltSnippet*	Txb_GetLoadCommands(TexBlend*);

#define BUFLEN 1024

main(int argc, char **argv)
{
    HField*		surf;			/* -> surface for cube */
    HFData*		field;			/* -> internal private version */
    Model*		model;			/* -> model of a cube */
    Character*	group;			/* -> group with cube */
    GP*			gp;
	TexBlend	*bTex;
	HFMaterial	matTable[10];	/* This example has 2 textures */
	int32		lStart,pStart;
	PipTable    *pip_table;
	uint32		i;
	float		uscale, vscale;
	int			cliponly = 0;
	int			noclip = 0;
	int			notexture = 0;

    Gfx_Init();		/* startup initialization */
	if (argc > 1) {
		if (!strcmp(argv[1],"-c")) {
			cliponly = 1;
			argv++;
			argc--;
		} else if (!strcmp(argv[1],"-nc")) {
			noclip = 1;
			argv++;
			argc--;
		} else if (!strcmp(argv[1],"-nt")) {
			notexture = 1;
			argv++;
			argc--;
		}
	}
	gp = InitGP(&argc, argv);

	if (argc > 2 ) {
		uscale = atoi(argv[1]);
		vscale = atoi(argv[2]);
	} else {
		uscale = 10.;
		vscale = 10.;
	}
	printf("uscale = %f vscale = %f\n", uscale, vscale);

    InitHFieldClass();

    model = Mod_Create();		/* create empty model */

    surf = (HField*) Obj_Create(GFX_HField);

    field = (HFData*)surf;
	printf("nrows, ncols = %d %d \n", header.nrows, header.ncols);
	printf("deltax, deltay = %f %f\n", header.deltax, header.deltay);
	printf("ntex = %d\n",header.ntex);
	lStart = 0;
	pStart = 0;
	for (i=0; i<header.ntex; i++) {
		printf("%s\n",texnames[i]);
		/*
		 * Create and load textures.
		 */
		bTex = Txb_Create();
		matTable[i].txb = bTex;

		if (Txb_Load(bTex, texnames[i]) != GFX_OK ) {
			printf("Can not load texture %s\n",texnames[i]);
			exit(-1);
		};
		Txb_SetTxFirstColor(bTex, TX_ColorSelectTexColor);
		Txb_SetTxSecondColor(bTex, TX_ColorSelectPrimColor);
		Txb_SetTxColorOut(bTex, TX_BlendOutSelectBlend);
		Txb_SetTxBlendOp(bTex, TX_BlendOpMult);
		Txb_SetWrap(bTex, TX_WrapModeTile);
		matTable[i].usize = Txb_GetXSize(bTex);
		matTable[i].vsize = Txb_GetYSize(bTex);
		/* Make texture repeat every 4 cells */
		matTable[i].uscale = matTable[i].usize/uscale;
		matTable[i].vscale = matTable[i].vsize/vscale;
		printf("usize = %d vsize = %d\n",matTable[i].usize, matTable[i].vsize);
		(void)Txb_GetLoadCommands(bTex); /* Force computation of tram size */
		Txb_SetTramOffset(bTex, lStart);
		lStart += Txb_GetTramSize(bTex);
		if (notexture) {
			matTable[i].tabSnip = CltNoTextureSnippet;
		} else {
			matTable[i].tabSnip = *Txb_GetTxCommands(bTex);
		}
		matTable[i].useSnip = *Txb_GetUseCommands(bTex);
		if (( pip_table = Txb_GetPip(bTex)) != NULL ) {
			Pip_SetIndex(pip_table,pStart);
			Txb_SetTxPipIndexOffset(bTex,pStart);
			pStart += Pip_GetSize(pip_table);
		}
	}

    field->xstart = 0;
    field->ystart = 0;
    field->rx = header.nrows;
    field->ry = header.ncols;
    field->nx = header.nrows;
    field->ny = header.ncols;
    field->mattable = &matTable[0];
	field->nmats = header.ntex;
    field->fdata = data;
	field->deltax = header.deltax;
	field->deltay = header.deltay;

    group = Char_Create(0);		/* create empty group */
	if (cliponly) {
		GP_Enable(gp, GP_Clipping);
		Char_SetCulling(group, FALSE);
	} else if (noclip) {
		GP_Disable(gp, GP_Clipping);
		Char_SetCulling(group, FALSE);
	}
    Char_Append(group, model);	/* put model into it */

	HField_ComputeBound(field);
    Mod_SetSurface(model, surf); /* save the surface in our model */
	ViewGroup(group,gp);
}


