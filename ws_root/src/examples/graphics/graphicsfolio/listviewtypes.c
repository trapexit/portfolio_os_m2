/******************************************************************************
**
**  @(#) listviewtypes.c 96/06/14 1.1
**
******************************************************************************/

/**
|||	AUTODOC -public -class Examples -group Graphics -name listviewtypes
|||	Illustrates how to create a basic View.
|||
|||	  Synopsis
|||
|||	    listviewtypes [<projector>]
|||
|||	  Description
|||
|||	    This program illustrates the use of the graphics folio function
|||	    NextViewTypeInfo(), using it to print out a list of all the
|||	    available View types for a given Projector.
|||
|||	    The program iterates through each available View type, printing
|||	    out a brief decription of each.
|||
|||	    If an argument is given on the command line, it is taken to be
|||	    the name of the Projector to be listed.  If there is no
|||	    argument, the default Projector is listed.
|||
|||	  Arguments
|||
|||	    [<projector>]
|||	        The name of a Projector whose View types are to be listed.
|||	        If this argument is absent, the default Projector is used.
|||
|||	  Associated Files
|||
|||	    listviewtypes.c
|||
|||	  Location
|||
|||	    examples/graphics/graphicsfolio
|||
|||	  See Also
|||
|||	    NextViewTypeInfo(), listprojectors(@), View(@)
|||
**/

#include <kernel/types.h>
#include <kernel/item.h>
#include <graphics/graphics.h>
#include <graphics/view.h>
#include <stdio.h>
#include <stdlib.h>


/***************************************************************************
 * Prototypes.
 */
int main(int ac, char **av);
void die(Err err, char *str);


/***************************************************************************
 * Code.
 */
int
main (ac, av)
int	ac;
char	**av;
{
	ViewTypeInfo	*vti;
	Item		pi;
	Err		err;

	if ((err = OpenGraphicsFolio ()) < 0)
		die (err, "Can't open graphics folio.\n");

	if (ac == 1) {
		pi = 0;
	} else {
		if ((pi = FindNamedItem
			   (MKNODEID (NST_GRAPHICS, GFX_PROJECTOR_NODE),
			    av[1])) < 0)
			die (pi, "Couldn't locate specified Projector.\n");
	}

	printf ("List of ViewTypeInfos for Projector #0x%08lx:\n", pi);
	vti = NULL;
	while (vti = NextViewTypeInfo (pi, vti)) {
		printf ("--ViewTypeInfo @ 0x%08lx: \n", vti);
		printf ("                       Type:  0x%08lx\n",
			vti->vti_Type);
		printf ("                      Flags:  0x%08lx\n",
			vti->vti_Flags);
		printf ("  MaxPixWidth, MaxPixHeight:  %d, %d\n",
			vti->vti_MaxPixWidth, vti->vti_MaxPixHeight);
		printf ("           XAspect, YAspect:  %d:%d\n\n",
			vti->vti_XAspect, vti->vti_YAspect);
	}

	CloseGraphicsFolio ();
	printf ("Normal exit.\n");

	return (0);
}

void
die (err, str)
Err	err;
char	*str;
{
	if (err < 0)
		PrintfSysErr (err);
	printf ("%s", str);
	exit (20);
}
