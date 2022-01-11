#ifndef __GRAPHICS_PIPE_OBJ_H
#define __GRAPHICS_PIPE_OBJ_H


/******************************************************************************
**
**  @(#) obj.h 96/02/20 1.27
**
**  GfxObj: M2 Framework object. All Framework objects are
**  derived from the same base class. This class provides
**  support for reference counting, debug printing, and a variety
**  of general queries.
**
******************************************************************************/


/****
 *
 * Attributes:
 *	Type		type identifier of object
 *	Use			reference count value
 *	Func		pointer to virtual function table (C only)
 *
 ****/

#define OBJ_NoFree	(1 << 7)	/* object cannot be freed */

#if defined(__cplusplus) && !defined(GFX_C_Bind)
/****
 *
 * C++ Run-time type checking support
 * Each Framework class has a class structure that contains the string
 * name of the class and a pointer to the class structure of its parent.
 * These are declared static, so there is only one such structure
 * for each class. The pointer to the GfxClass structure is
 * also the identifier for the class.
 *
 * GFX_CLASS_DECLARE
 *	Declares the internal information (virtual functions and static
 *	members) necessary for run-time type checking
 *
 * GFX_CLASS_INFO(name, parent)
 *  Defines compile-time information for the class. This should
 *  be called once in the file which contains the implementation
 *  of the class.
 *
 * Framework classes also have some virtual functions to support
 * run-time type checking:
 *
 * uint32 GfxObj::GetClass()
 *	returns the class ID (pointer to the GfxClass structure)
 *
 * uint32 GfxObj::GetParentClass()
 *	returns the class ID (pointer to the GfxClass structure)
 *	of the parent class (NULL for Character)
 *
 * char* GfxObj::GetClassName()
 *	returns a pointer to the string name of the class
 *
 * bool GfxObj::IsClass(int32)
 *	returns TRUE if the object is a sub-class of the input class
 *
 * static GfxObj* GfxObj::Create(void*)
 *	instantiates (allocates memory and constructs) a new empty
 *	object of this class. Used to create an object when reading
 *	an archive. If the input pointer is NULL, a normal memory allocation
 *	is done. If non-null, only the constructor is called and the
 *	input pointer is assumed to be the memory area to be occupied by
 *	the new object.
 *
 * virtual GfxObj* GfxObj::NewSelf()
 *	instantiates (allocates memory and constructs) a new empty
 *	object of this class. Used to make clones of an existing object.
 *
 * "Do-Nothing" Constructors:
 *	Binary SDF needs constructors which will set up the pointer to
 *	the virtual function table but will not touch any other data
 *	fields (BSDF does the data copy directly from the file). We use
 *	GFX_DO_NOTHING_TYPE as the type of the argument to the constructor.
 *	We use GFX_DO_NOTHING_VALUE as a value for this argument which
 *	will be guaranteed to be of type GFX_DO_NOTHING_TYPE. Every
 *	subclass of a Framework object MUST have a "do-nothing" constructor
 *	to be readable from a binary SDF file. This constructor should
 *	not do anything except ensure that its base class and owned
 *	fields invoke their "do-nothing" constructors.
 *
 ****/
typedef struct GfxClass
  {
    GfxClass*	m_Parent;			/* -> parent class structure */
    char*       m_Name;				/* string name of class */
	GfxObj*		(*Create)(void*);	/* -> function to instantiate object */
  } GfxClass;

#define	GFX_NULL	((GfxObj*) 0)
#define	GFX_DO_NOTHING_TYPE		void*,void*
#define	GFX_DO_NOTHING_VALUE	(void*)0,(void*)0

#define GFX_CLASS_DECLARE(name)				\
	name(GFX_DO_NOTHING_TYPE);				\
	virtual GfxObj* NewSelf();				\
    virtual uint32 GetClass() const;		\
    virtual char* GetClassName() const;		\
    virtual uint32 GetParentClass() const;	\
    static GfxClass m_Class; 				\
    static GfxObj* Create(void*);			\
    static int32 ClassID();

#define GFX_CLASS_INFO(name, parent)	   			\
	GfxObj* name::NewSelf()							\
		{ return new name(); }						\
    uint32 name::GetClass() const					\
		{ return (uint32) &name::m_Class; }			\
    char* name::GetClassName() const				\
		{ return name::m_Class.m_Name; }			\
    uint32 name::GetParentClass() const				\
		{ return (uint32) name::m_Class.m_Parent; }	\
    int32 name::ClassID()							\
		{ return (uint32) &name::m_Class; }			\
    GfxObj* name::Create(void* p)					\
        { if (p) return new(p) name(); else return new name(); } \
    GfxClass name::m_Class = { &parent::m_Class, #name, name::Create };

/****
 *
 * C++ Smart reference support
 * Each Framework class can have a "reference" class associated with it
 * that acts as a smart pointer which can reference count objects of
 * that class. For the most part, you can use a reference class as a
 * pointer to an object of the class. It can automatically convert to
 * an from a pointer. When you assign into a reference class, the
 * old object (if not null) is dereferenced and freed if necessary. The
 * new object has its reference count incremented.
 *
 * GFX_CLASS_REF(refclass, objclass)
 *	Declares a reference class <refclass> which acts as a smart
 *	pointer to objects of <objclass>. This macro will automatically
 *	generate a reference class which converts to and from pointers
 *	to <objclass>.
 *
 ****/
#define GFX_CLASS_REF(refclass, objclass)				\
struct refclass : public GfxRef {						\
	refclass(GFX_DO_NOTHING_TYPE) : GfxRef(GFX_DO_NOTHING_VALUE) { }; \
    refclass() : GfxRef() { };                          \
    refclass(const refclass& src) : GfxRef(src) { };    \
    refclass(objclass* ptr) : GfxRef(ptr) { };			\
    operator objclass*() { return (objclass*) m_ObjPtr; }	\
	operator const objclass*() const { return (const objclass*) m_ObjPtr; } \
   	objclass* operator->() { return (objclass*) m_ObjPtr; }	\
	const objclass* operator->() const { return (const objclass*) m_ObjPtr; } };

/****
 *
 * C++ API
 *
 ****/
class GfxObj
   {
	friend class GfxRef;
public:
//	Constructors and Destructors
	GfxObj();
	GfxObj(GFX_DO_NOTHING_TYPE);
	GfxObj(const GfxObj&);
	GfxObj(int32 type);
	GfxObj& operator=(GfxObj&);
	virtual ~GfxObj();
	void	Delete();

//	Class Operations (for run-time type checking)
	virtual	GfxObj*	Clone();
	virtual	void	Print() const;
	virtual	void	PrintInfo() const;
			int32	GetType() const;
	virtual bool	IsClass(int32) const;
	virtual uint32	GetClass() const;
	virtual uint32	GetParentClass() const;
	virtual char*	GetClassName() const;
	static	int32	ClassID();

//	Allocation, Reference counting
	void*	operator new(size_t);
    void*	operator new(size_t, void*);
	void	operator delete(void*);

Protected:
//	Internal Functions
	void	IncUse();
	void	Init();

//	Internal Overrides for subclassing
	virtual GfxObj*	NewSelf();
	virtual	Err		Copy(GfxObj*);

//	Data members
	static	GfxClass	m_Class;
	int8				m_Type;
	int8				m_Flags;
	int16				m_Use;
  };

inline void Delete(GfxObj* obj)
	{ if (obj) obj->Delete(); }

/****
 *
 * GfxRef is a smart GfxObj pointer which automatically does reference
 * counting for you. When you assign a non-NULL GfxObj pointer to the
 * GfxRef, it increments the use count of that object. If the GfxRef
 * already references a different object, it's reference count is
 * decremented (and it is freed if necessary).
 *
 ****/
class GfxRef
   {
	friend class GfxObj;
public:
//	Constructors and Destructors
			GfxRef(const GfxRef&);
	inline	GfxRef(GFX_DO_NOTHING_TYPE);
	inline	GfxRef();
	inline	GfxRef(GfxObj*);
	inline	GfxRef(GfxObj**);
	inline	~GfxRef();

//	Operators
	inline	GfxObj*	operator*() { return m_ObjPtr; }
	inline	GfxRef& operator=(const GfxRef&);
			GfxRef& operator=(GfxObj*);
	inline	bool	operator==(const GfxObj*);
	inline	bool	operator!=(const GfxObj*);
	inline	bool	IsNull() const;

Protected:
	GfxObj*	m_ObjPtr;
   };

class ObjRef : public GfxRef
   {
public:
//	Constructors and Destructors
			ObjRef(const GfxRef&);
	inline	ObjRef(GFX_DO_NOTHING_TYPE);
	inline	ObjRef();
	inline	ObjRef(GfxObj*);
	inline	ObjRef(GfxObj**);

//	Operators
    inline operator GfxObj*();
	inline operator const GfxObj*() const;
   };

#else

#ifdef __cplusplus
extern "C" {
#endif

/****
 *
 * PUBLIC C API
 *
 ****/
#define	GFX_NULL	0

GfxObj*		Obj_Create(uint32 type);
GfxObj*		Obj_Clone(GfxObj*);
void		Obj_Delete(GfxObj*);
bool		Obj_IsNull(GfxObj*);
void		Obj_Init(GfxObj*, int);
int32		Obj_GetType(const GfxObj*);
int16		Obj_GetUse(const GfxObj*);
void		Obj_Print(const GfxObj*);
void		Obj_PrintInfo(const GfxObj*);
bool		Obj_Copy(GfxObj*, GfxObj*);
bool        Obj_IsClass(const GfxObj* c, int32 id);
uint32      Obj_GetClass(const GfxObj* c);
char*       Obj_GetClassName(const GfxObj* c);
uint32      Obj_GetParentClass(const GfxObj* c);
void		Obj_Assign(GfxObj**, GfxObj*);

#ifdef __cplusplus
}
#endif

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
 * Character class table entry. Each built-in and user-defined class
 * has an entry in the class table. The class type (CHAR_xxx) is an
 * index into this class table.
 *
 ****/
typedef struct GfxClass
  {
    uint32      m_Base;		/* base class ID */
    uint32      m_Size;		/* byte size of class members */
    char*       m_Name;		/* string name of class */
    ObjFuncs*	m_Funcs;	/* function dispatch table */
    uint32		m_FuncSize;	/* byte size of function dispatch table */
  } GfxClass;

extern GfxClass *Classes;       /* pointer to character class table */

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
   };

typedef GfxObj* GfxRef;

#endif

#include <graphics/pipe/obj.inl>

#endif /* __GRAPHICS_PIPE_OBJ_H */
