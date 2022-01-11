/*
**	19-May-1994	gab		Changed numerous fields to longint to conform to
**						the version 3.4 SYM file format specification.
**
**	18-Nov-1992	pmr		Changed mte_cvte_index to a longint
**						Version string ==> 'Version 3.3'
**						Note conditional compilation based on _32bitCVTE flag...
**
**	28-Aug-1992	jkp		Reformatted with cdent and changed the #ifndef name
**						to the standard form.  Copied typedef'ed index names
**						from Paul-Marcel's SYMTypes file for use here, replacing
**						the existing raw type names with these names.
**
**	05-Feb-1990	jkp		Removed mte_def_??? as they were unused.
**						Changed mte_cmte_index to a longint
**						Changed mte_csnte_idx_? to longints
**						Changed csnte_mte_offset to longint
**						Version string ==> 'Version SADE 1.1.1'
**
**	10-Mar-1989 lmd		DTIs ==> longints
**						CVTEs acquired logical addresses
**						Version string ==> '\pVersion 3.1'
**						
**   1-Jun-1988 lmd		Added 'SYM_FILE_VERSION' constant, updated FITE stuff, added
**							file type and creator fields to the DSHB header.
**	31-May-1988 lmd		Added 'CONSTANT_POOL_ENTRY' and 'const_tinfo'
*/


/*
** DebugDataTypes
**	Mimics DebugDataTypes.p interface
*/

#ifndef __DEBUGDATATYPES__
#define __DEBUGDATATYPES__	1

#ifndef __PPCC__
	#pragma once
#endif

#ifndef __TYPES__
#include <Types.h>	/* for definitions of OSType and ResType */
/*	#include <ConditionalMacros.h>								*/
/*	#include <MixedMode.h>										*/
/*		#include <Traps.h>										*/
#endif

/* All structures must use 68k alignment */
#if defined(powerc) || defined (__powerc)
#pragma options align=mac68k
#endif

#define SYM_FILE_VERSION 			"\pVersion 3.4"
#define SYM_FILE_VERSION_v33		"\pVersion 3.3"
#define SYM_FILE_VERSION_v32		"\pVersion 3.2"
#define SYM_FILE_VERSION_v31		"\pVersion 3.1"
#define HTE_MAX_HASH				1024		
#define DEBUG_FILE_TYPE				'MPSY'		/* Debugger symbol table file type */

#define GLOBAL_DATA_RESOURCE_NAME	'gbld'
#define GLOBAL_ROOT_MODULE_NAME		"\p*EMPYREAN*"


/*
** Reserved indices
*/
#define NO_TABLE_ENTRIES			0			/* Reserved NIL table index */

#define END_OF_LIST_v32				0xFFFF
#define FILE_NAME_INDEX_v32			(END_OF_LIST_v32-1)
#define SOURCE_FILE_CHANGE_v32		(END_OF_LIST_v32-1)
#define MAXIMUM_LEGAL_INDEX_v32		(SOURCE_FILE_CHANGE_v32-1)	/* Must be one less than the above!! */
#define END_OF_LIST					0xFFFFFFFF
#define FILE_NAME_INDEX				(END_OF_LIST-1)
#define SOURCE_FILE_CHANGE			(END_OF_LIST-1)
#define MAXIMUM_LEGAL_INDEX			(SOURCE_FILE_CHANGE-1)	/* Must be one less than the above!! */

#define VARIABLE_DEFINITION			1	/* dummy */
#define STATEMENT_NUMBER_DEFINITION	2	/* dummy */
#define HASH_NAME_INDEX				3	/* dummy */
#define CHILD_MODULE_INDEX			4	/* dummy */
#define FILE_SOURCE_INCREMENT		5	/* dummy */
#define FILE_NAME_START				6	/* dummy */
#define FILE_SOURCE_START			7	/* dummy */
#define TYPE_NAME_DEFINITION		8	/* dummy */
#define LABEL_DEFINITION			9	/* dummy */
#define FITE_ENTRY					10	/* dummy */



/*	STORAGE_CLASS_xxx
** Storage class numbers taken from the OMF spec
*/
/* The actual storage class itself */
#define STORAGE_CLASS_REGISTER		0			//value is in register	
#define STORAGE_CLASS_A5			1			//relative to A5 - data
#define STORAGE_CLASS_A6			2			//relative to A6 - stack
#define STORAGE_CLASS_A7			3			//relative to A7
#define STORAGE_CLASS_ABSOLUTE		4			//relative to 0
#define STORAGE_CLASS_CONSTANT		5			//the value itself
#define STORAGE_CLASS_BIGCONSTANT	6			//CONSTANT_POOL_ENTRY index
#define STORAGE_CLASS_RESOURCE		99			//RTE index of resource

/*	STORAGE_KIND_xxx
** The storage kind for parameters: local vs. value vs. reference distinction
*/
#define STORAGE_KIND_LOCAL			0	//local (A6 relative or static)
#define STORAGE_KIND_VALUE			1	//call-by-val parm
#define STORAGE_KIND_REFERENCE		2	//call-by-ref parm
#define STORAGE_KIND_WITH			3	//"WITH" logical address

/*	MODULE_KIND_xxx
** The type of module.  Taken from the OMF document
*/
#define MODULE_KIND_NONE			0
#define MODULE_KIND_PROGRAM			1
#define MODULE_KIND_UNIT			2
#define MODULE_KIND_PROCEDURE		3
#define MODULE_KIND_FUNCTION		4
#define MODULE_KIND_DATA			5
#define MODULE_KIND_BLOCK			6				/* The module is an internal block */

/*	SYMBOL_SCOPE_xxx
** The visibility of symbols, etc.
*/
#define SYMBOL_SCOPE_LOCAL			0				/* Object is seen only inside current scope */
#define SYMBOL_SCOPE_GLOBAL			1				/* Object has global scope */

#define MIN_NONPRIMITIVE_TTE		100				/* First "real" type */

/*	kCVTE_xxx
**	CVTE addressing variants
*/
#define	kCVTE_SCA					0				/* indicate SCA variant of CVTE */
#define	kCVTE_LA_MAX_SIZE			13				/* max# of logical address bytes in a CVTE */
#define	kCVTE_BIG_LA				127				/* indicates LA redirection to constant pool */


/*	DTTE_xxx
** Values within a dtte_psize
*/
#define	DTTE_BIG_LOGICAL_SIZE		0x08000			/* Bit set when lsize is a LONGINT	*/
#define	DTTE_PSIZE_MASK				0x003FF			/* The mask for the physical size	*/



/*
** These typedefs simplify life a little bit
*/
typedef unsigned short	IndexSize_v32;
typedef unsigned long	IndexSize;

typedef unsigned short	CLTEIndex_v32;
typedef unsigned long	CLTEIndex;

typedef unsigned short	CMTEIndex_v32;
typedef unsigned long	CMTEIndex;

typedef unsigned short	CSNTEIndex_v31;
typedef unsigned long	CSNTEIndex;

typedef unsigned short	CTTEIndex_v32;
typedef unsigned long	CTTEIndex;

typedef unsigned short	CVTEIndex_v32;
typedef unsigned long	CVTEIndex;

typedef unsigned short	FITEIndex;

typedef unsigned short	FREFIndex;

typedef IndexSize_v32	FRTEIndex_v32;
typedef IndexSize		FRTEIndex;

typedef unsigned short	HTEIndex;

typedef IndexSize_v32	MTEIndex_v32;
typedef IndexSize		MTEIndex;

typedef unsigned short	MTEOffset_v32;
typedef unsigned long	MTEOffset;

typedef unsigned short	RTEIndex;

typedef IndexSize_v32	TTEIndex_v32;
typedef IndexSize		TTEIndex;

typedef unsigned long	NTEIndex;

typedef unsigned long CONSToffset;
typedef unsigned long TINFOoffset;


/*	FILE_REFERENCE
** A file reference contains an index into the file reference table (giving
** the name of the file) and an absolute offset into the file giving the
** beginning of the source referred to.  The entry is of a fixed size.
*/
struct FILE_REFERENCE_v32 {
	FRTEIndex_v32	fref_frte_index;		/* File reference table index		*/
	long			fref_offset;			/* Absolute offset into source file	*/
};

struct FILE_REFERENCE {
	FRTEIndex		fref_frte_index;		/* File reference table index		*/
	long			fref_offset;			/* Absolute offset into source file	*/
};




/*	STORAGE_CLASS_ADDRESS
** A storage class entry describes a storage class.  The storage class type
** is derived from the OMF specification.  The definition below should take
** no more than 6 bytes!  Is BYTE the proper declaration for what I intend?
*/
struct STORAGE_CLASS_ADDRESS {
	char sca_kind;								/* Distinguish local from value/var formal	*/
	char sca_class;								/* The storage class itself					*/
	long sca_offset;
};



/*	RESOURCE_TABLE_ENTRY
** All code and data is *defined* to reside in a resource.  Even A5
** relative data is defined to reside in a dummy resource of ResType
** 'gbld'.  Code always resides in a resource.  Because a code/data
** is built of many modules, when walking through a resource we must
** point back to the modules in the order they were defined.  This is
** done by requiring the entries in the Modules Entry table to be
** ordered by resource/resource-number and by the location in that
** resource.  Hence, the resource table entry points to the first
** module making up that resource.  All modules table entries following
** that first one with the same restype/resnum are contiguous and offset
** from that first entry.
*/
struct RESOURCE_TABLE_ENTRY_v32 {
	ResType			rte_ResType;		/* Resource Type							*/
	short			rte_res_number;		/* Resource Number							*/
	NTEIndex		rte_nte_index;		/* Name of the resource						*/
	MTEIndex_v32	rte_mte_first;		/* Index of first module in the resource	*/
	MTEIndex_v32	rte_mte_last;		/* Index of the last module in the resource	*/
	unsigned long	rte_res_size;		/* Size of the resource						*/
};

struct RESOURCE_TABLE_ENTRY {
	ResType			rte_ResType;		/* Resource Type							*/
	short			rte_res_number;		/* Resource Number							*/
	NTEIndex		rte_nte_index;		/* Name of the resource						*/
	MTEIndex		rte_mte_first;		/* Index of first module in the resource	*/
	MTEIndex		rte_mte_last;		/* Index of the last module in the resource	*/
	unsigned long	rte_res_size;		/* Size of the resource						*/
};


/*	MODULES_TABLE_ENTRY
** Modules table entries are ordered by their appearance in a resource.
** (Note that having a single module copied into two resources is not
** possible).  Modules map back to their resource via an index into the
** resource table and an offset into the resource.  Modules also point
** to their source files, both the definition module and implemention
** module.  Because modules can be textually nested within other
** modules, a link to the parent (containing) module is required.  This
** module can textually contain other modules.  A link to the contiguous
** list of child (contained) modules is required.  Variables, statements,
** and types defined in the module are pointed to by indexing the head of
** the contiguous lists of contained variables, contained statements,
** and contained types.
*/
struct MODULES_TABLE_ENTRY_v31 {
	RTEIndex				mte_rte_index;		/* Which resource it is in			*/
	long					mte_res_offset;		/* Offset into the resource			*/
	long					mte_size;			/* Size of module					*/
	char					mte_kind;			/* What kind of module this is		*/
	char					mte_scope;			/* How visible is it?				*/
	MTEIndex_v32			mte_parent;			/* Containing module				*/
	struct FILE_REFERENCE_v32 mte_imp_fref;		/* Implementation source			*/
	long					mte_imp_end;		/* End of implementation source		*/
	NTEIndex				mte_nte_index;		/* The name of the module			*/
	CMTEIndex_v32			mte_cmte_index;		/* Modules contained in this		*/
	CVTEIndex_v32			mte_cvte_index;		/* Variables contained in this		*/
	CLTEIndex_v32			mte_clte_index;		/* Local labels defined here		*/
	CTTEIndex_v32			mte_ctte_index;		/* Types contained in this			*/
	CSNTEIndex_v31			mte_csnte_idx_1;	/* CSNTE index of mte_snbr_first	*/
	CSNTEIndex_v31			mte_csnte_idx_2;	/* CSNTE index of mte_snbr_last		*/
};

struct MODULES_TABLE_ENTRY_v32 {
	RTEIndex				mte_rte_index;		/* Which resource it is in			*/
	long					mte_res_offset;		/* Offset into the resource			*/
	long					mte_size;			/* Size of module					*/
	char					mte_kind;			/* What kind of module this is		*/
	char					mte_scope;			/* How visible is it?				*/
	MTEIndex_v32			mte_parent;			/* Containing module				*/
	struct FILE_REFERENCE_v32 mte_imp_fref;		/* Implementation source			*/
	long					mte_imp_end;		/* End of implementation source		*/
	NTEIndex				mte_nte_index;		/* The name of the module			*/
	CMTEIndex_v32			mte_cmte_index;		/* Modules contained in this		*/
	CVTEIndex_v32			mte_cvte_index;		/* Variables contained in this		*/
	CLTEIndex_v32			mte_clte_index;		/* Local labels defined here		*/
	CTTEIndex_v32			mte_ctte_index;		/* Types contained in this			*/
	CSNTEIndex				mte_csnte_idx_1;	/* CSNTE index of mte_snbr_first	*/
	CSNTEIndex				mte_csnte_idx_2;	/* CSNTE index of mte_snbr_last		*/
};

struct MODULES_TABLE_ENTRY_v33 {
	RTEIndex				mte_rte_index;		/* Which resource it is in			*/
	long					mte_res_offset;		/* Offset into the resource			*/
	long					mte_size;			/* Size of module					*/
	char					mte_kind;			/* What kind of module this is		*/
	char					mte_scope;			/* How visible is it?				*/
	MTEIndex_v32			mte_parent;			/* Containing module				*/
	struct FILE_REFERENCE_v32 mte_imp_fref;		/* Implementation source			*/
	long					mte_imp_end;		/* End of implementation source		*/
	NTEIndex				mte_nte_index;		/* The name of the module			*/
	CMTEIndex_v32			mte_cmte_index;		/* Modules contained in this		*/
//LOOK!!! CVTEIndex is 4-bytes here!!!
//NOT CVTEIndex_v32!!
	CVTEIndex				mte_cvte_index;		/* Variables contained in this		*/
	CLTEIndex_v32			mte_clte_index;		/* Local labels defined here		*/
	CTTEIndex_v32			mte_ctte_index;		/* Types contained in this			*/
	CSNTEIndex				mte_csnte_idx_1;	/* CSNTE index of mte_snbr_first	*/
	CSNTEIndex				mte_csnte_idx_2;	/* CSNTE index of mte_snbr_last		*/
};

struct MODULES_TABLE_ENTRY {
	RTEIndex				mte_rte_index;		/* Which resource it is in			*/
	long					mte_res_offset;		/* Offset into the resource			*/
	long					mte_size;			/* Size of module					*/
	char					mte_kind;			/* What kind of module this is		*/
	char					mte_scope;			/* How visible is it?				*/
	MTEIndex				mte_parent;			/* Containing module				*/
	struct FILE_REFERENCE	mte_imp_fref;		/* Implementation source			*/
	long					mte_imp_end;		/* End of implementation source		*/
	NTEIndex				mte_nte_index;		/* The name of the module			*/
	CMTEIndex				mte_cmte_index;		/* Modules contained in this		*/
	CVTEIndex				mte_cvte_index;		/* Variables contained in this		*/
	CLTEIndex				mte_clte_index;		/* Local labels defined here		*/
	CTTEIndex				mte_ctte_index;		/* Types contained in this			*/
	CSNTEIndex				mte_csnte_idx_1;	/* CSNTE index of mte_snbr_first	*/
	CSNTEIndex				mte_csnte_idx_2;	/* CSNTE index of mte_snbr_last		*/
};




/*	CONTAINED_MODULES_TABLE_ENTRY
** Contained Modules are lists of indices into the modules table.  The
** lists are terminated by an END_OF_LIST index.  All entries are of the
** same size, hence mapping an index into a CMTE list is simple.
**
**			CMT = MTE_INDEX* END_OF_LIST.
*/
#define cmte_mte_index	cmte_.mte_index
#define cmte_nte_index	cmte_.nte_index
union CONTAINED_MODULES_TABLE_ENTRY_v32 {
	struct {
		MTEIndex_v32	mte_index;		/* Index into the Modules Table */
		NTEIndex		nte_index;		/* The name of the module		*/
	} cmte_;

	IndexSize_v32	cmte_end_of_list;	/* = END_OF_LIST				*/
};

union CONTAINED_MODULES_TABLE_ENTRY {
	struct {
		MTEIndex		mte_index;		/* Index into the Modules Table */
		NTEIndex		nte_index;		/* The name of the module		*/
	} cmte_;

	IndexSize		cmte_end_of_list;	/* = END_OF_LIST				*/
};




/*	CONTAINED_VARIABLES_TABLE_ENTRY
** Contained Variables map into the module table, file table, name table, and type
** table.  Contained Variables are a contiguous list of source file change record,
** giving the name of and offset into the source file corresponding to all variables
** following.  Variable definition records contain an index into the name table (giving
** the text of the variable as it appears in the source code), an index into the type
** table giving the type of the variable, an increment added to the source file
** offset giving the start of the implementation of the variable, and a storage
** class address, giving information on variable's runtime address.
**
**				CVT					= SOURCE_FILE_CHANGE SYMBOL_INFO* END_OF_LIST.
**				SYMBOL_INFO = SYMBOL_DEFINITION | SOURCE_FILE_CHANGE .
**
** All entries are of the same size, making the fetching of data simple.  The
** variable entries in the list are in ALPHABETICAL ORDER to simplify the display of
** available variables for several of the debugger's windows.
*/
#define cvte_file_change	cvte_file_.change
#define cvte_fref			cvte_file_.fref
#define cvte_tte_index		cvte_.tte_index
#define cvte_nte_index		cvte_.nte_index
#define cvte_file_delta 	cvte_.file_delta
#define cvte_scope			cvte_.scope
#define cvte_la_size		cvte_.la_size
#define cvte_location		cvte_.address.location
#define cvte_la				cvte_.address.lastruct.la
#define cvte_la_kind		cvte_.address.lastruct.la_kind
#define cvte_big_la			cvte_.address.biglastruct.big_la
#define cvte_big_la_kind	cvte_.address.biglastruct.big_la_kind
union CONTAINED_VARIABLES_TABLE_ENTRY_v32 {
	struct {
		IndexSize_v32				change;		/* = SOURCE_FILE_CHANGE	*/
		struct FILE_REFERENCE_v32	fref;		/* File name table		*/
	} cvte_file_;

	struct {
		TTEIndex_v32	tte_index;			/* < SOURCE_FILE_CHANGE				*/
		NTEIndex		nte_index;			/* Index into the name table		*/
		short			file_delta;			/* Increment from previous source	*/
		char			scope;
		char			la_size;			/* #bytes of LAs below */

		/*
		**	'cvte_la_size' determines the variant used below:
		**
		**		kCVTE_SCA				Traditional STORAGE_CLASS_ADDRESS;
		**
		**		1ÉkCVTE_LA_MAX_SIZE		That many logical address bytes ("in-situ");
		**
		**		kCVTE_BIG_LA			Logical address bytes in constant pool, at
		**								offset 'big_la'.
		**
		*/
		union {
			struct STORAGE_CLASS_ADDRESS location;/* traditional storage class	*/

			struct {
				char la[kCVTE_LA_MAX_SIZE];		/* logical address bytes		*/
				char la_kind;					/* eqv. cvte_location.sca_kind	*/
				char la_xxxx;					/* (reserved)					*/
			} lastruct;

			struct {
				long big_la;					/* logical address bytes in constant pool */
				char big_la_kind;				/* eqv. cvte_location.sca_kind	*/
			} biglastruct;

		} address;
	} cvte_;

	IndexSize_v32	 		cvte_end_of_list;	/* = END_OF_LIST */
};

union CONTAINED_VARIABLES_TABLE_ENTRY {
	struct {
		IndexSize				change;		/* = SOURCE_FILE_CHANGE	*/
		struct FILE_REFERENCE	fref;		/* File name table		*/
	} cvte_file_;

	struct {
		TTEIndex			tte_index;		/* < SOURCE_FILE_CHANGE				*/
		NTEIndex			nte_index;		/* Index into the name table		*/
		short				file_delta;		/* Increment from previous source	*/
		char				scope;
		char				la_size;		/* #bytes of LAs below */

		/*
		**	'cvte_la_size' determines the variant used below:
		**
		**		kCVTE_SCA				Traditional STORAGE_CLASS_ADDRESS;
		**
		**		1ÉkCVTE_LA_MAX_SIZE		That many logical address bytes ("in-situ");
		**
		**		kCVTE_BIG_LA			Logical address bytes in constant pool, at
		**								offset 'big_la'.
		**
		*/
		union {
			struct STORAGE_CLASS_ADDRESS location;	/* traditional storage class	*/

			struct {
				char	la[kCVTE_LA_MAX_SIZE];		/* logical address bytes		*/
				char	la_kind;					/* eqv. cvte_location.sca_kind	*/
				char	la_xxxx;					/* (reserved)					*/
			} lastruct;

			struct {
				long	big_la;						/* logical address bytes in constant pool */
				char	big_la_kind;				/* eqv. cvte_location.sca_kind	*/
			} biglastruct;
			
		} address;
	} cvte_;

	IndexSize				 cvte_end_of_list;		/* = END_OF_LIST */
};




/*	CONTAINED_STATEMENTS_TABLE_ENTRY
** Contained Statements table.  This table is similar to the Contained
** Variables table except that instead of VARIABLE_DEFINITION entries, this
** module contains STATEMENT_NUMBER_DEFINITION entries.  A statement number
** definition points back to the containing module (via an index into
** the module entry table) and contains the file and resource deltas
** to add to the previous values to get to this statement.
** All entries are of the same size, making the fetching of data simple.  The
** entries in the table are in order of increasing statement number within the
** source file.
**
** The Contained Statements table is indexed from two places.  An MTE contains
** an index to the first statement number within the module.  An FRTE contains
** an index to the first statement in the table (Possibly.  This is slow.)  Or
** a table of fast statement number to CSNTE entry mappings indexes into the
** table.  Choice not yet made.
*/
#define csnte_file_change		csnte_file_.change
#define csnte_fref				csnte_file_.fref
#define csnte_file_delta		csnte_.file_delta
#define csnte_mte_index			csnte_.mte_index
#define csnte_mte_offset		csnte_.mte_offset
union CONTAINED_STATEMENTS_TABLE_ENTRY_v32 {
	struct {
		IndexSize_v32				change;		/* = SOURCE_FILE_CHANGE	*/
		struct FILE_REFERENCE_v32	fref;		/* File name table		*/
	} csnte_file_;

	struct {
		MTEIndex_v32	mte_index;				/* Which module contains it		*/
		short			file_delta;				/* Where it is defined			*/
		//FIXME!! why isn't this MTEOffset_v32 ???
		MTEOffset		mte_offset;				/* Where it is in the module	*/
	} csnte_;

	IndexSize_v32		csnte_end_of_list;		/* = END_OF_LIST				*/
};

union CONTAINED_STATEMENTS_TABLE_ENTRY {
	struct {
		IndexSize				change;		/* = SOURCE_FILE_CHANGE	*/
		struct FILE_REFERENCE	fref;		/* File name table		*/
	} csnte_file_;

	struct {
		MTEIndex		mte_index;			/* Which module contains it		*/
		short			file_delta;			/* Where it is defined			*/
		MTEOffset		mte_offset;			/* Where it is in the module	*/
	} csnte_;

	IndexSize			csnte_end_of_list;	/* = END_OF_LIST				*/
};




/*	CONTAINED_LABELS_TABLE_ENTRY
** Contained Labels table names those labels local to the module.  It is similar
** to the Contained Statements table.
*/
#define clte_file_change clte_file_.change
#define clte_fref		 clte_file_.fref
#define clte_mte_index	 clte_.mte_index
#define clte_mte_offset	 clte_.mte_offset
#define clte_nte_index	 clte_.nte_index
#define clte_file_delta	 clte_.file_delta
#define clte_scope		 clte_.scope
union CONTAINED_LABELS_TABLE_ENTRY_v32 {
	struct {
		IndexSize_v32				change;		/* = SOURCE_FILE_CHANGE	*/
		struct FILE_REFERENCE_v32	fref;		/* File name table		*/
	} clte_file_;

	struct {
		MTEIndex_v32		mte_index;			/* Which module contains us		*/
		MTEOffset_v32		mte_offset;			/* Where it is in the module	*/
		//FIXME!! why isn't this NTEIndex_v32 ???
		NTEIndex			nte_index;			/* The name of the label		*/
		short				file_delta;			/* Where it is defined			*/
		short				scope;				/* How visible the label is		*/
	} clte_;

	IndexSize_v32			clte_end_of_list;	/* = END_OF_LIST				*/
};

union CONTAINED_LABELS_TABLE_ENTRY{
	struct {
		IndexSize				change;		/* = SOURCE_FILE_CHANGE	*/
		struct FILE_REFERENCE	fref;		/* File name table		*/
	} clte_file_;

	struct  {
		MTEIndex		mte_index;			/* Which module contains us		*/
		MTEOffset		mte_offset;			/* Where it is in the module	*/
		NTEIndex		nte_index;			/* The name of the label		*/
		short			file_delta;			/* Where it is defined			*/
		short			scope;				/* How visible the label is		*/
	} clte_;

	IndexSize			clte_end_of_list;	/* = END_OF_LIST				*/
};




/*	CONTAINED_TYPES_TABLE_ENTRY
** Contained Types define the named types that are in the module.  It is used to
** map name indices into type indices.  The type entries in the table are in
** alphabetical order by type name.
*/
#define ctte_file_change	ctte_file_.change
#define ctte_fref			ctte_file_.fref
#define ctte_tte_index		ctte_.tte_index
#define ctte_nte_index		ctte_.nte_index
#define ctte_file_delta		ctte_.file_delta
union CONTAINED_TYPES_TABLE_ENTRY_v32 {
	struct {
		IndexSize_v32				change;		/* = SOURCE_FILE_CHANGE			*/
		struct FILE_REFERENCE_v32	fref;		/* File definition				*/
	} ctte_file_;

	struct {
		TTEIndex_v32	tte_index;				/* < SOURCE_FILE_CHANGE			*/
		NTEIndex		nte_index;				/* Index into the name table	*/
		short			file_delta;				/* From last file definition	*/
	} ctte_;

	IndexSize_v32		ctte_end_of_list;		/* = END_OF_LIST				*/
};

union CONTAINED_TYPES_TABLE_ENTRY {
	struct {
		IndexSize				change;		/* = SOURCE_FILE_CHANGE			*/
		struct FILE_REFERENCE	fref;		/* File definition				*/
	} ctte_file_;

	struct {
		TTEIndex		tte_index;			/* < SOURCE_FILE_CHANGE			*/
		NTEIndex		nte_index;			/* Index into the name table	*/
		short			file_delta;			/* From last file definition	*/
	} ctte_;

	IndexSize			ctte_end_of_list;	/* = END_OF_LIST				*/
};




/*	FILE_REFERENCE_TABLE_ENTRY
** The FILE REFERENCES TABLE maps from source file to module & offset.
** The table is ordered by increasing file offset.  Each new offset
** references a module.
**
**				FRT	= FILE_SOURCE_START
**							FILE_SOURCE_INCREMENT*
**							END_OF_LIST.
**
**	*** THIS MECHANISM IS VERY SLOW FOR FILE+STATEMENT_NUMBER TO
**	*** MODULE/CODE ADDRESS OPERATIONS.  ANOTHER MECHANISM IS
**	***	REQUIRED!!
*/
#define frte_name_entry		frte_file_.name_entry
#define frte_nte_index		frte_file_.nte_index
#define	frte_mod_date		frte_file_.mod_date
#define frte_mte_index		frte_.mte_index
#define frte_file_offset	frte_.file_offset
union FILE_REFERENCE_TABLE_ENTRY_v32 {
	struct {
		IndexSize_v32	name_entry;				/* = FILE_NAME_INDEX	*/
		NTEIndex		nte_index;				/* Name table entry		*/
		long			mod_date;				/* When file was last modified */
	} frte_file_;

	struct {
		MTEIndex_v32	mte_index;				/* Module table entry < FILE_NAME_INDEX		*/
		long			file_offset;			/* Absolute offset into the source file		*/
	} frte_;

	IndexSize_v32		frte_end_of_list;		/* = END_OF_LIST */
};

union FILE_REFERENCE_TABLE_ENTRY {
	struct {
		IndexSize		name_entry;				/* = FILE_NAME_INDEX	*/
		NTEIndex		nte_index;				/* Name table entry		*/
		long			mod_date;				/* When file was last modified */
	} frte_file_;

	struct {
		MTEIndex		mte_index;				/* Module table entry < FILE_NAME_INDEX		*/
		long			file_offset;			/* Absolute offset into the source file		*/
	} frte_;

	IndexSize			frte_end_of_list;		/* = END_OF_LIST */
};




/*	FRTE_INDEX_TABLE_ENTRY
** The FRTE INDEX TABLE indexes into the FILE REFERENCE TABLE above.  The FRTE
** at that index is the FILE_SOURCE_START for a series of files.  The FRTEs are
** indexed from 1.  The list is terminated with an END_OF_LIST.
*/
#define fite_frte_index		fite_.frte_index
#define fite_nte_index		fite_.nte_index
union FRTE_INDEX_TABLE_ENTRY_v32 {
	struct {
		FRTEIndex_v32	frte_index;				/* Index into the FRTE table */
		NTEIndex	  	nte_index;				/* Name table index, gives filename */
	} fite_;

	IndexSize_v32		fite_end_of_list;		/* = END_OF_LIST */
};

union FRTE_INDEX_TABLE_ENTRY {
	struct {
		FRTEIndex		frte_index;				/* Index into the FRTE table */
		NTEIndex		nte_index;				/* Name table index, gives filename */
	} fite_;

	IndexSize			fite_end_of_list;		/* = END_OF_LIST */
};




/*	DISK_TYPE_TABLE_ENTRY
** The structure of the TYPE TABLE is currently unknown.  All that is
** known is that it will contain base type information, composite
** type information (indices back into itself), and name table information
** (indices into the name table for the names of record fields and
** enumerated types).
**
** The structure will be:
**		ARRAY [0 .. 32767] OF UNSIGNED
*/
#define dtte_word_size	tte_.word_.size
#define dtte_word_bytes	tte_.word_.bytes
#define dtte_long_size	tte_.long_.size
#define dtte_long_bytes	tte_.long_.bytes
struct DISK_TYPE_TABLE_ENTRY {
	NTEIndex		dtte_nte;					/* The name of the type					*/
	unsigned short	dtte_psize;					/* Hi-bit: Big.  Lower 9: physical size */
	union {
		struct {
			unsigned short	size;				/* two bytes of size	*/
			unsigned char	bytes[1];			/* type codes			*/
		} word_;

		struct {
			unsigned long	size;				/* four bytes of size	*/
			unsigned char	bytes[1];			/* type codes			*/
		} long_;
	} tte_;
};



/*	HASH_TABLE_ENTRY
** The hash table is shared between the linker and the debugger.  Each
** entry in the hash table indexes to the head of a name chain in the name
** table.  The hash chain consists of contiguous PASCAL strings (byte giving
** count of characters followed by the characters themselves, aligned on a
** word (even) boundary), terminated by a *magic* symbol (see below).
*/
typedef long HASH_TABLE_ENTRY;
typedef HASH_TABLE_ENTRY HASH_TABLE_TYPE[HTE_MAX_HASH];




/*	NAME_TABLE_ENTRY
** The NAME TABLE is a compacted list of PASCAL strings with NULL terminators
** (thereby making them C strings too), stored on word boundaries.
** The order of the strings is by hash chain: clumps of strings terminated by a
** *magic* symbol:: a string  of length 1 whose text is the zero character.  Null
** strings occur at the end of a page.  A null string means that the next string
** would not fit into the space remaining in the current page.  The string will be
** found at the beginning of the next page.  Hence, any null string is interpreted
** as "next page!" when scanned in the name table.
**
** The structure will be:
**		 Str255
**
** Entries with a length of 255 are a special semaphore and reference to a name
** whose length is greater than 254 characters.
*/
typedef short NAME_TABLE_ENTRY;

struct NTE_EXT_ENTRY {
	unsigned char	nte_ext_magic;		/* must == 0xFF (255) */
	unsigned char	nte_ext_type;		/* type of extended entry, currently 0x00 */
	unsigned short	nte_ext_length;		/* string length */
	char			nte_ext_text[1];	/* the contents of the name (unbounded array) */
};


/*	CONSTANT_POOL_ENTRY
** The CONSTANT_POOL consists of entries that start on word boundaries.  The entries
** are referenced by byte index into the constant pool, not by record number.
**
** Each entry takes the form:
**
**		<16-bit size>
**		<that many bytes of stuff>
**
** Entries do not cross page boundaries.
**
*/
typedef short CONSTANT_POOL_ENTRY;


/*
** The DISK_SYMBOL_HEADER_BLOCK is the first record in a .SYM file, defining the
** physical characteristics of the symbolic information.  The remainder of the
** .SYM file is stored in fixed block allocations.  For the purposes of paging, the
** file is considered to be an array of dshb_page_size blocks, with block 0 (and
** possibly more) devoted to the DISK_SYMBOL_HEADER_BLOCK.
**
** The dti_object_count field means that the allowed indices for that type of
** object are 0 .. dti_object_count.  An index of 0, although allowed, is
** never done.  However, an 0th entry is created in the table.  That entry is
** filled with all zeroes.  The reason for this is to avoid off-by-one programming
** errors that would otherwise occur: an index of k *MEANS* k, not k-1 when going
** to the disk table.
*/

/*	DISK_TABLE_INFO
*/
struct DISK_TABLE_INFO_v32 {
	unsigned short dti_first_page;			/* First page for this table		*/
	unsigned short dti_page_count;			/* Number of pages for the table	*/
	unsigned long  dti_object_count;		/* Number of objects in the table	*/
};

struct DISK_TABLE_INFO {
	unsigned long dti_first_page;			/* First page for this table		*/
	unsigned long dti_page_count;			/* Number of pages for the table	*/
	unsigned long dti_object_count;			/* Number of objects in the table	*/
};


/*	DISK_SYMBOL_HEADER_BLOCK
*/
struct DISK_SYMBOL_HEADER_BLOCK_v32 {
	char			dshb_id[32];		/* Version information				*/
	short			dshb_page_size;		/* Size of the pages/blocks			*/
	short			dshb_hash_page;		/* Disk page for the hash table		*/
	MTEIndex_v32	dshb_root_mte;		/* MTE index of the program root	*/
	unsigned long	dshb_mod_date;		/* modification date of executable	*/

	struct DISK_TABLE_INFO_v32	dshb_frte;	/* Per TABLE information			*/
	struct DISK_TABLE_INFO_v32	dshb_rte;
	struct DISK_TABLE_INFO_v32	dshb_mte;
	struct DISK_TABLE_INFO_v32	dshb_cmte;
	struct DISK_TABLE_INFO_v32	dshb_cvte;
	struct DISK_TABLE_INFO_v32	dshb_csnte;
	struct DISK_TABLE_INFO_v32	dshb_clte;
	struct DISK_TABLE_INFO_v32	dshb_ctte;
	struct DISK_TABLE_INFO_v32	dshb_tte;
	struct DISK_TABLE_INFO_v32	dshb_nte;
	struct DISK_TABLE_INFO_v32	dshb_tinfo;
	struct DISK_TABLE_INFO_v32	dshb_fite;			/* file information */
	struct DISK_TABLE_INFO_v32	dshb_const;			/* constant pool */
	
	OSType			dshb_file_creator;	/* executable's creator */
	ResType			dshb_file_type;		/* executable's file type */
};

struct DISK_SYMBOL_HEADER_BLOCK {
	char			dshb_id[32];		/* Version information				*/
	short			dshb_page_size;		/* Size of the pages/blocks			*/
	unsigned long	dshb_hash_page;		/* Disk page for the hash table		*/
	MTEIndex		dshb_root_mte;		/* MTE index of the program root	*/
	unsigned long	dshb_mod_date;		/* modification date of executable	*/

	struct DISK_TABLE_INFO	dshb_frte;	/* Per TABLE information			*/
	struct DISK_TABLE_INFO	dshb_rte;
	struct DISK_TABLE_INFO	dshb_mte;
	struct DISK_TABLE_INFO	dshb_cmte;
	struct DISK_TABLE_INFO	dshb_cvte;
	struct DISK_TABLE_INFO	dshb_csnte;
	struct DISK_TABLE_INFO	dshb_clte;
	struct DISK_TABLE_INFO	dshb_ctte;
	struct DISK_TABLE_INFO	dshb_tte;
	struct DISK_TABLE_INFO	dshb_nte;
	struct DISK_TABLE_INFO	dshb_tinfo;
	struct DISK_TABLE_INFO	dshb_fite;			/* file information */
	struct DISK_TABLE_INFO	dshb_const;			/* constant pool */
	
	OSType			dshb_file_creator;	/* executable's creator */
	OSType			dshb_file_type;		/* executable's file type */
};


/*
** Make the above structure definitions suitable for use by normal C
*/
#ifndef __cplusplus
typedef struct FILE_REFERENCE_v32					FILE_REFERENCE_v32;
typedef struct FILE_REFERENCE						FILE_REFERENCE;
typedef struct STORAGE_CLASS_ADDRESS				STORAGE_CLASS_ADDRESS;
typedef struct RESOURCE_TABLE_ENTRY_v32				RESOURCE_TABLE_ENTRY_v32;
typedef struct RESOURCE_TABLE_ENTRY					RESOURCE_TABLE_ENTRY;
typedef struct MODULES_TABLE_ENTRY_v31				MODULES_TABLE_ENTRY_v31;
typedef struct MODULES_TABLE_ENTRY_v32				MODULES_TABLE_ENTRY_v32;
typedef struct MODULES_TABLE_ENTRY_v33				MODULES_TABLE_ENTRY_v33;
typedef struct MODULES_TABLE_ENTRY					MODULES_TABLE_ENTRY;
typedef union  CONTAINED_MODULES_TABLE_ENTRY_v32	CONTAINED_MODULES_TABLE_ENTRY_v32;
typedef union  CONTAINED_MODULES_TABLE_ENTRY		CONTAINED_MODULES_TABLE_ENTRY;
typedef union  CONTAINED_VARIABLES_TABLE_ENTRY_v32	CONTAINED_VARIABLES_TABLE_ENTRY_v32;
typedef union  CONTAINED_VARIABLES_TABLE_ENTRY		CONTAINED_VARIABLES_TABLE_ENTRY;
typedef union  CONTAINED_STATEMENTS_TABLE_ENTRY_v32	CONTAINED_STATEMENTS_TABLE_ENTRY_v32;
typedef union  CONTAINED_STATEMENTS_TABLE_ENTRY		CONTAINED_STATEMENTS_TABLE_ENTRY;
typedef union  CONTAINED_LABELS_TABLE_ENTRY_v32		CONTAINED_LABELS_TABLE_ENTRY_v32;
typedef union  CONTAINED_LABELS_TABLE_ENTRY			CONTAINED_LABELS_TABLE_ENTRY;
typedef union  CONTAINED_TYPES_TABLE_ENTRY_v32		CONTAINED_TYPES_TABLE_ENTRY_v32;
typedef union  CONTAINED_TYPES_TABLE_ENTRY			CONTAINED_TYPES_TABLE_ENTRY;
typedef union  FILE_REFERENCE_TABLE_ENTRY_v32		FILE_REFERENCE_TABLE_ENTRY_v32;
typedef union  FILE_REFERENCE_TABLE_ENTRY			FILE_REFERENCE_TABLE_ENTRY;
typedef union  FRTE_INDEX_TABLE_ENTRY_v32			FRTE_INDEX_TABLE_ENTRY_v32;
typedef union  FRTE_INDEX_TABLE_ENTRY				FRTE_INDEX_TABLE_ENTRY;
typedef struct DISK_TYPE_TABLE_ENTRY				DISK_TYPE_TABLE_ENTRY;
typedef struct NTE_EXT_ENTRY						NTE_EXT_ENTRY;
typedef struct DISK_TABLE_INFO_v32					DISK_TABLE_INFO_v32;
typedef struct DISK_TABLE_INFO						DISK_TABLE_INFO;
typedef struct DISK_SYMBOL_HEADER_BLOCK_v32			DISK_SYMBOL_HEADER_BLOCK_v32;
typedef struct DISK_SYMBOL_HEADER_BLOCK DISK_SYMBOL_HEADER_BLOCK;
#endif


/*
** The above data types will be usually pointed to instead of directly viewed.
** Define pointer types to the above data types
*/

typedef FILE_REFERENCE_v32					*FREF_v32_P;
typedef FILE_REFERENCE						*FREF_P;
typedef RESOURCE_TABLE_ENTRY_v32			*RTE_v32_P;
typedef RESOURCE_TABLE_ENTRY				*RTE_P;
typedef MODULES_TABLE_ENTRY_v31				*MTE_v31_P;
typedef MODULES_TABLE_ENTRY_v32				*MTE_v32_P;
typedef MODULES_TABLE_ENTRY_v33				*MTE_v33_P;
typedef MODULES_TABLE_ENTRY					*MTE_P;
typedef CONTAINED_MODULES_TABLE_ENTRY_v32	*CMTE_v32_P;
typedef CONTAINED_MODULES_TABLE_ENTRY		*CMTE_P;
typedef CONTAINED_VARIABLES_TABLE_ENTRY_v32	*CVTE_v32_P;
typedef CONTAINED_VARIABLES_TABLE_ENTRY		*CVTE_P;
typedef CONTAINED_STATEMENTS_TABLE_ENTRY_v32 *CSNTE_v32_P;
typedef CONTAINED_STATEMENTS_TABLE_ENTRY	*CSNTE_P;
typedef CONTAINED_TYPES_TABLE_ENTRY_v32		*CTTE_v32_P;
typedef CONTAINED_TYPES_TABLE_ENTRY			*CTTE_P;
typedef CONTAINED_LABELS_TABLE_ENTRY_v32	*CLTE_v32_P;
typedef CONTAINED_LABELS_TABLE_ENTRY		*CLTE_P;
typedef DISK_TYPE_TABLE_ENTRY				*DTTE_P;
typedef NTE_EXT_ENTRY						*NTE_EXT_P;
typedef FILE_REFERENCE_TABLE_ENTRY_v32		*FRTE_v32_P;
typedef FILE_REFERENCE_TABLE_ENTRY			*FRTE_P;
typedef DISK_SYMBOL_HEADER_BLOCK_v32		*DSHB_v32_P;
typedef DISK_SYMBOL_HEADER_BLOCK			*DSHB_P;
typedef HASH_TABLE_TYPE						*HT_P;
typedef CONSTANT_POOL_ENTRY					*CONST_P;
typedef FRTE_INDEX_TABLE_ENTRY_v32			*FITE_v32_P;
typedef FRTE_INDEX_TABLE_ENTRY				*FITE_P;

/* Structures can now revert to default alignment */
#if defined(powerc) || defined(__powerc)
#pragma options align=reset
#endif

#endif	/* __DEBUGDATATYPES__ */
