/****
 *
 * @(#) obj.h 95/09/21 1.26
 *
 * GfxObj: M2 Framework object. All Framework objects are
 * derived from the same base class. This class provides
 * support for reference counting, debug printing, and a variety
 * of general queries.
 *
 * Attributes:
 *	Type		type identifier of object
 *	Use			reference count value
 *	Func		pointer to virtual function table (C only)
 *
 ****/

#ifndef _FWOBJ
#define _FWOBJ


/****
 *
 * PUBLIC C API
 *
 ****/
#include "kerneltypes.h"    /* TMA- how this worked without this? */


/****
 *
 * C Function Dispatch Table for GfxClass allocation/free
 * These functions are used internally to allocate and free
 * instances of base objects. Each sub-class of the base
 * object must supply their own versions of these functions
 *	Construct	Constructor: initializes attributes
 *	DeleteAttrs	Destructor: frees attributes and resources
 *	PrintInfo	prints attributes on standard output
 *	Copy		copies one object into another
 *
 ****/
typedef struct ObjFuncs
   {
    Err		(*Construct)(GfxObj* dst);
    void	(*DeleteAttrs)(GfxObj*);
    void	(*PrintInfo)(const GfxObj*);
    Err		(*Copy)(GfxObj* dst, GfxObj* src);
   } ObjFuncs;

/****
 *
 * Base object for C Framework implementation
 * Contains type, reference count, flags and dispatch vector
 * GfxObj is a structure that represents the INTERNAL FORMAT of
 * class GfxObj in the C binding. DO NOT RELY ON THE FIELDS IN THIS
 * STRUCTURE - THEY ARE SUBJECT TO CHANGE WITHOUT NOTICE! GfxObj
 * attributes should only be accessed by the Obj_XXX functions.
 *
 ****/
struct GfxObj
   {
	ObjFuncs*	m_Funcs;		/* -> virtual function table */
	int8		m_Type;			/* type GFX_xxx */
	int8		m_Flags;		/* memory allocation flags */
	int16		m_Use;			/* reference count */
	int32		m_Objid;		/* obj id */
	void		*m_Ref;			/* reference pointer */
   };

typedef GfxObj* GfxRef;


#endif /* FWOBJ */
