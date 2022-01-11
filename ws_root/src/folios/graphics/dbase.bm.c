/*  :ts=8 bk=0
 *
 * dbase.bm.c:	Bitmap type database.
 *
 * @(#) dbase.bm.c 96/03/23 1.1
 *
 * Leo L. Schwab					9512.12
 */
#include <kernel/types.h>

#include <graphics/gfx_pvt.h>
#include <graphics/projector.h>
#include <graphics/bitmap.h>
#include <graphics/view.h>

#include "protos.h"


/**************************************************************************
 * BMTypeInfo array.
 * This describes Bitmap capabilities and default ViewType assignments.
 * This has to track the BMTYPE_ enum declarations.
 */
BMTypeInfo	bmtypeinfos[] = {
 {
	/*  BMTYPE_INVALID  */
	NULL,
	NULL,
	0,
	0,
	VIEWTYPE_INVALID,
 }, {
	/*  BMTYPE_16	*/
	bmsize,
	simple_addr,
	BMF_DISPLAYABLE | BMF_RENDERABLE | BMF_MPEGABLE,
	sizeof (uint16),
	PROJTYPE_NTSC | VIEWTYPE_16
 }, {
	/*  BMTYPE_32	*/
	bmsize,
	simple_addr,
	BMF_DISPLAYABLE | BMF_RENDERABLE | BMF_MPEGABLE,
	sizeof (uint32),
	PROJTYPE_NTSC | VIEWTYPE_32
 }, {
	/*  BMTYPE_16_ZBUFFER	*/
	bmsize,
	simple_addr,
	BMF_RENDERABLE,
	sizeof (uint16),
	VIEWTYPE_INVALID
 }
};
