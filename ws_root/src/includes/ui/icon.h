#ifndef __UI_ICON_H
#define __UI_ICON_H


/******************************************************************************
**
**  @(#) icon.h 96/02/28 1.9
**
**  Definitions for the icon folio.
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_ITEM_H
#include <kernel/item.h>
#endif

#ifndef __KERNEL_OPERROR_H
#include <kernel/operror.h>
#endif

#ifndef __GRAPHICS_FRAME2D_SPRITEOBJ_H
#include <graphics/frame2d/spriteobj.h>
#endif

#ifndef __KERNEL_TIME_H
#include <kernel/time.h>
#endif


/****************************************************************************/


/* kernel interface definitions */
#define ICON_FOLIONAME   "icon"


/****************************************************************************/


/* Error codes */

#define MakeIconErr(svr,class,err) MakeErr(ER_FOLI,ER_ICON,svr,ER_E_SSTM,class,err)

/* Bad pointer passed in */
#define ICON_ERR_BADPTR          MakeIconErr(ER_SEVERE,ER_C_STND,ER_BadPtr)

/* Unknown tag supplied */
#define ICON_ERR_BADTAG          MakeIconErr(ER_SEVERE,ER_C_STND,ER_BadTagArg)

/* Require tag missing */
#define ICON_ERR_BADTAGVAL       MakeIconErr(ER_SEVERE,ER_C_STND,ER_BadTagArgVal)

/* No memory */
#define ICON_ERR_NOMEM           MakeIconErr(ER_SEVERE,ER_C_STND,ER_NoMem)

/* Object not found */
#define ICON_ERR_NOTFOUND        MakeIconErr(ER_SEVERE,ER_C_STND,ER_NotFound)

/* Bad argument name supplied */
#define ICON_ERR_BADNAME         MakeIconErr(ER_SEVERE,ER_C_STND,ER_BadName)

#define ICON_ERR_BADITEM		 MakeIconErr(ER_SEVERE, ER_C_STND, ER_BadItem)

/* Incomplete Icon */
#define ICON_ERR_INCOMPLETE      MakeIconErr(ER_SEVERE,ER_C_NSTND,1)

/* Mutually-exclusive options */
#define ICON_ERR_MUTUALLYEXCLUSIVE \
						         MakeIconErr(ER_SEVERE,ER_C_NSTND,2)
/* Missing arguments */
#define ICON_ERR_ARGUMENTS       MakeIconErr(ER_SEVERE,ER_C_NSTND,3)

/* Unknown icon format */
#define ICON_ERR_UNKNOWNFORMAT   MakeIconErr(ER_SEVERE,ER_C_NSTND,4)

/* Oversized Icon */
#define ICON_ERR_OVERSIZEDICON	 MakeIconErr(ER_SEVERE, ER_C_NSTND, 5)

/*****************************************************************************/


/* definition for a standard IFF ICON chunk */
#define ID_ICON MAKE_ID('I', 'C', 'O', 'N')
#define ID_IDTA MAKE_ID('I', 'D', 'T', 'A')

/*****************************************************************************/

/* The definition of an IDTA chunk */
typedef struct IconData
{
	TimeVal			TimeBetweenFrames;	 /* Animation delay between frames */
	char			ApplicationName[32]; /* Name of App that created icon  */
} IconData;


/* This is the in-memory representation of an icon */
typedef struct Icon
{
	Node	 Node;					/* For users to use as they wish	  */
	List	 SpriteObjs;			/* A list of SpriteObjs with imagery  */
	TimeVal	 TimeBetweenFrames;		/* Opt. time between frames 	      */
	char	 ApplicationName[32];	/* Name of App that created this icon */
} Icon;


/*****************************************************************************/


/* tags used when loading an icon */
typedef enum LoadIconTags
{
    LOADICON_TAG_FILENAME=10,   /* name of file to get icon of       	 */
	LOADICON_TAG_IFFPARSER,		/* IFFParser * of IFF file			 	 */
	LOADICON_TAG_IFFPARSETYPE,	/* Type of chunk currently in, for above */
    LOADICON_TAG_FILESYSTEM,    /* name of file system to get icon of	 */
    LOADICON_TAG_DRIVER,        /* name of device to get icon of     	 */
	LOADICON_TAG_HARDWARE		/* HWID of HW to request an icon for     */
} LoadIconTags;

#define LOADICON_TYPE_AUTOPARSE		0
#define LOADICON_TYPE_PARSED		1

/*****************************************************************************/

/* tags used when saving an icon */
typedef enum SaveIconTags
{
    SAVEICON_TAG_FILENAME=10, 		/* name of file to write icon to       	 */
	SAVEICON_TAG_IFFPARSER,			/* IFFParser * of IFF file			 	 */
	SAVEICON_TAG_TIMEBETWEENFRAMES	/* TimeVal between frames of textures 	 */
} SaveIconTags;


/*****************************************************************************/

#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */


/* folio management */
Err OpenIconFolio(void);
Err CloseIconFolio(void);

Err LoadIcon(Icon **icon, const TagArg *tags);
Err LoadIconVA(Icon **icon, uint32 tag, ...);
Err SaveIcon(char *utffile, char *appname, TagArg *tags);
Err SaveIconVA(char *utffile, char *appname, uint32 tag, ...);
Err UnloadIcon(Icon *icon);


#ifdef  __cplusplus
}
#endif  /* __cplusplus */


/*****************************************************************************/


#endif /* __UI_ICON_H */

