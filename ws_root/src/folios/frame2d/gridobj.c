/*
 *	@(#) gridobj.c 96/07/08 1.13
 *
 * 2D Framework for M2 graphics
 *
 */


#include <kernel/types.h>
#include <kernel/tags.h>
#include "frame2.i"


GridObj *
Gro_Create (TagArg *t)
{
  TagArg *t1, *t2;
  GridObj *gr;
  char *name=0;

  /* Allocate memory for grid object */
  gr = AllocMem(sizeof(GridObj), MEMTYPE_NORMAL);
  if (!gr) goto error;

  /* Initialize contents of grid object to zero */
  memset (gr, 0, sizeof(GridObj));

  /* Set up node fields in grid object */
  gr->gro_Node.n_SubsysType = NODE_2D;
  gr->gro_Node.n_Type = GRIDOBJNODE;
  gr->gro_Node.n_Size = sizeof(GridObj);

  t1 = t;
  while ((t2 = NextTagArg(&t1)) != NULL) {
    switch (t2->ta_Tag) {
    case GRO_TAG_NAME:
      FreeMem(name, TRACKED_SIZE);
      name = AllocMem( strlen((char*)(t->ta_Arg)) + 1, MEMTYPE_NORMAL | MEMTYPE_TRACKSIZE );
      if (!name) goto error;
      strcpy (name, (char*)(t->ta_Arg));
      gr->gro_Node.n_Name = name;
      break;
    case GRO_TAG_PRIORITY:
      gr->gro_Node.n_Priority = (uint8)(t->ta_Arg);
      break;
    case GRO_TAG_SPRITEARRAY:
      if (Gro_SetSpriteArray(gr,(SpriteObj**)(t->ta_Arg)) < 0) goto error;
      break;
    case GRO_TAG_WIDTH:
      gr->gro_Width = (uint32)(t->ta_Arg);
      break;
    case GRO_TAG_HEIGHT:
      gr->gro_Height = (uint32)(t->ta_Arg);
      break;
    case GRO_TAG_XPOS:
      gr->gro_Position.x = *(gfloat*)(&t->ta_Arg);
      break;
    case GRO_TAG_YPOS:
      gr->gro_Position.y = *(gfloat*)(&t->ta_Arg);
      break;
    case GRO_TAG_HDELTAX:
      gr->gro_HDelta.x = (uint32)(t->ta_Arg);
      break;
    case GRO_TAG_HDELTAY:
      gr->gro_HDelta.y = (uint32)(t->ta_Arg);
      break;
    case GRO_TAG_VDELTAX:
      gr->gro_HDelta.x = (uint32)(t->ta_Arg);
      break;
    case GRO_TAG_VDELTAY:
      gr->gro_HDelta.y = (uint32)(t->ta_Arg);
      break;
    default:
      goto error;
    }
  }
  return gr;

 error:
  FreeMem(name, TRACKED_SIZE);
  FreeMem(gr, sizeof(GridObj));
  return 0;
}


Err
Gro_Delete (GridObj* gr)
{
  if (gr) {
    FreeMem(gr->gro_Node.n_Name, TRACKED_SIZE);
    FreeMem(gr, sizeof(GridObj));
  }
  return 0;
}


Err
Gro_SetSpriteArray (GridObj *gr, SpriteObj **sp)
{
  gr->gro_SpriteArray = sp;

  return 0;
}


SpriteObj **
Gro_GetSpriteArray (GridObj *gr)
{
  return gr->gro_SpriteArray;
}


Err
Gro_SetWidth (GridObj *gr, uint32 w)
{
  gr->gro_Width = w;

  return 0;
}


uint32
Gro_GetWidth (const GridObj *gr)
{
  return gr->gro_Width;
}


Err
Gro_SetHeight (GridObj *gr, uint32 h)
{
  gr->gro_Height = h;

  return 0;
}


uint32
Gro_GetHeight (const GridObj *gr)
{
  return gr->gro_Height;
}


Err
Gro_SetPosition (GridObj* gr, Point2* p)
{
  gr->gro_Position.x = p->x;
  gr->gro_Position.y = p->y;

  return 0;
}


void
Gro_GetPosition (const GridObj* gr, Point2* p)
{
  p->x = gr->gro_Position.x;
  p->y = gr->gro_Position.y;
}


Err
Gro_SetHDelta (GridObj* gr, Vector2 *h)
{
  gr->gro_HDelta.x = h->x;
  gr->gro_HDelta.y = h->y;

  return 0;
}


void
Gro_GetHDelta (const GridObj* gr, Vector2 *h)
{
  h->x = gr->gro_HDelta.x;
  h->y = gr->gro_HDelta.y;
}


Err
Gro_SetVDelta (GridObj* gr, Vector2 *h)
{
  gr->gro_VDelta.x = h->x;
  gr->gro_VDelta.y = h->y;

  return 0;
}


void
Gro_GetVDelta (const GridObj* gr, Vector2 *h)
{
  h->x = gr->gro_VDelta.x;
  h->y = gr->gro_VDelta.y;
}


