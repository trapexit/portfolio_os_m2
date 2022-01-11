/******************************************************************************
**
**  @(#) listprojectors.c 96/06/14 1.1
**
******************************************************************************/

/**
|||	AUTODOC -public -class Examples -group Graphics -name listprojectors
|||	Illustrates how to get a list of the system's available Projectors.
|||
|||	  Synopsis
|||
|||	    listprojectors
|||
|||	  Description
|||
|||	    This simple program illustrates the use of the graphics folio
|||	    function NextProjector(), using it to print out a list of all
|||	    available Projectors in the system.
|||
|||	    The program iterates through all the Projectors, and prints out
|||	    a brief description of each one.
|||
|||	  Associated Files
|||
|||	    listprojectors.c
|||
|||	  Location
|||
|||	    examples/graphics/graphicsfolio
|||
|||	  See Also
|||
|||	    NextProjector(), listviewtypes(@), Projector(@)
|||
**/

#include <kernel/types.h>
#include <kernel/item.h>
#include <graphics/graphics.h>
#include <graphics/projector.h>
#include <stdio.h>
#include <stdlib.h>


/***************************************************************************
 * Prototypes.
 */
int main(int ac, char **av);


/***************************************************************************
 * Code.
 */
int
main (ac, av)
int	ac;
char	**av;
{
	Projector	*p;
	Item		pi;
	Err		err;

	TOUCH (ac);
	TOUCH (av);

	if ((err = OpenGraphicsFolio ()) < 0) {
		PrintfSysErr (err);
		printf ("Can't open graphics folio.\n");
		return (20);
	}

	pi = 0;
	while (pi = NextProjector (pi)) {
		p = LookupItem (pi);

		printf ("Projector Item #0x%08lx @ 0x%08lx: \"%s\"\n",
			pi, p, p->p.n_Name);
		printf ("         Width, Height:  %d, %d\n",
			p->p_Width, p->p_Height);
		printf ("   PixWidth, PixHeight:  %d, %d\n",
			p->p_PixWidth, p->p_PixHeight);
		printf ("       FieldsPerSecond:  %f\n", p->p_FieldsPerSecond);
		printf ("        FieldsPerFrame:  %d\n", p->p_FieldsPerFrame);
		printf ("      XAspect, YAspect:  %d:%d\n",
			p->p_XAspect, p->p_YAspect);
		printf ("                  Type:  %d\n", p->p_Type);
		printf ("                 Flags:  0x%08lx\n\n", p->p_Flags);
	}

	CloseGraphicsFolio ();
	printf ("Normal exit.\n");

	return (0);
}
