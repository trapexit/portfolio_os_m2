
//generic includes for the various versions of XSYM
//included by xsym_defs_v?r?.h

	#undef _SYM_FILE_VERSION 
	#undef _DISK_SYMBOL_HEADER_BLOCK 
	#undef _DISK_TABLE_INFO 
	#undef _MODULES_TABLE_ENTRY 
	#undef _FRTE_INDEX_TABLE_ENTRY 
	#undef _FILE_REFERENCE_TABLE_ENTRY 
	#undef _CONTAINED_TYPES_TABLE_ENTRY 
	#undef _CONTAINED_LABELS_TABLE_ENTRY 
	#undef _CONTAINED_STATEMENTS_TABLE_ENTRY 
	#undef _CONTAINED_VARIABLES_TABLE_ENTRY 
	#undef _CONTAINED_MODULES_TABLE_ENTRY 
	#undef _RESOURCE_TABLE_ENTRY 
	#undef _FILE_REFERENCE 
	#undef _IndexSize 
	#undef _CSNTEIndex 
	#undef _CLTEIndex 
	#undef _CMTEIndex 
	#undef _CTTEIndex
	#undef _CVTEIndex 
	#undef _FRTEIndex 
	#undef _MTEIndex
	#undef _MTEOffset 
	#undef _TTEIndex 
	#undef _END_OF_LIST			
	#undef _FILE_NAME_INDEX		
	#undef _SOURCE_FILE_CHANGE
	#undef _MAXIMUM_LEGAL_INDEX
	#undef _CSNTE_P 
	#undef _CVTE_P 
	#undef _CTTE_P 
	#undef _CMTE_P 
	#undef _CLTE_P 
	#undef _DSHB_P 
	#undef _FRTE_P 
	#undef _FITE_P 
	#undef _FREF_P 
	#undef _RTE_P 
	#undef _MTE_P 
	#undef _SIZEOF_DSHB 	
	#undef _SIZEOF_HASHTT 
	#undef _SIZEOF_NTE 		
	#undef _SIZEOF_RTE 		
	#undef _SIZEOF_MTE 	
	#undef _SIZEOF_CVTE 
	#undef _SIZEOF_FRTE
	#undef _SIZEOF_CSTE
	#undef _SIZEOF_CTTE 
	#undef _SIZEOF_FITE 
	#undef _TYPE_CSNTEIndex 	
	#undef _TYPE_IndexSize 
	#undef _TYPE_CLTEIndex 
	#undef _TYPE_CMTEIndex 
	#undef _TYPE_CTTEIndex 
	#undef _TYPE_CVTEIndex 
	#undef _TYPE_CVTEMTEIndex 
	#undef _TYPE_FRTEIndex 
	#undef _TYPE_MTEIndex  
	#undef _TYPE_MTEOffset 
	#undef _TYPE_TTEIndex  
	#undef _TYPE_FITEIndex 
	#undef _TYPE_FREFIndex 
	#undef _TYPE_HTEIndex  
	#undef _TYPE_RTEIndex  
	#undef _TYPE_NTEIndex  
	#undef _TYPE_ResType
	#undef _TYPE_short  
	#undef _TYPE_ushort  
	#undef _TYPE_long
	#undef _TYPE_ulong
	#undef _TYPE_char  
	#undef _TYPE_uchar  

#if __XSYM_VER__ == __XSYM_V33__ && defined(__XSYM_R_CP__)
	//used for type checking to get the right value for each version
	short 			t_short=0;
	long 			t_long=0;
	char 			t_char=0;
	unsigned short 	t_ushort=0;
	unsigned long 	t_ulong=0;
	unsigned char 	t_uchar=0;
	ResType		 	t_ResType=0;
#else
	extern short 			t_short;
	extern long 			t_long;
	extern char 			t_char;
	extern unsigned short 	t_ushort;
	extern unsigned long 	t_ulong;
	extern unsigned char 	t_uchar;
	extern ResType		 	t_ResType;
#endif
	#define _TYPE_short  	t_short
	#define _TYPE_ushort  	t_ushort
	#define _TYPE_long		t_long
	#define _TYPE_ulong		t_ulong
	#define _TYPE_char  	t_char
	#define _TYPE_uchar  	t_uchar
	#define _TYPE_ResType   t_ResType
	//had to treat _TYPE_CVTEMTEIndex as a special case...

#if __XSYM_VER__ <= __XSYM_V33__
//V31
#if __XSYM_VER__ == __XSYM_V31__
	#define _TYPE_CSNTEIndex _TYPE_ushort	
	#define _SYM_FILE_VERSION SYM_FILE_VERSION_v31
	#define _MODULES_TABLE_ENTRY MODULES_TABLE_ENTRY_v31
	#define _CSNTEIndex CSNTEIndex_v31
	#define _TYPE_CVTEMTEIndex _TYPE_ushort
	#define _TYPE_MTEOffset _TYPE_ushort
	#define _MTE_P MTE_v31_P
	//use v32's defs for rest...
//V32
#elif __XSYM_VER__ == __XSYM_V32__
	#define _TYPE_CSNTEIndex _TYPE_ulong
	#define _SYM_FILE_VERSION SYM_FILE_VERSION_v32
	#define _MODULES_TABLE_ENTRY MODULES_TABLE_ENTRY_v32
	#define _CSNTEIndex CSNTEIndex
	#define _TYPE_CVTEMTEIndex _TYPE_ushort
	#define _TYPE_MTEOffset _TYPE_ulong	//INCONSISTANT!!
	#define _MTE_P MTE_v32_P
//V33
#elif __XSYM_VER__ == __XSYM_V33__
	#define _TYPE_CSNTEIndex _TYPE_ulong
	#define _SYM_FILE_VERSION SYM_FILE_VERSION_v33
	#define _MODULES_TABLE_ENTRY MODULES_TABLE_ENTRY_v33
	#define _CSNTEIndex CSNTEIndex
	#define _TYPE_CVTEMTEIndex _TYPE_ulong	//INCONSISTANT!!
	#define _TYPE_MTEOffset _TYPE_ulong		//INCONSISTANT!!
	#define _MTE_P MTE_v33_P
#endif
	#define _TYPE_IndexSize _TYPE_ushort
	#define _TYPE_CLTEIndex _TYPE_ushort
	#define _TYPE_CMTEIndex _TYPE_ushort
	#define _TYPE_CTTEIndex _TYPE_ushort
	#define _TYPE_CVTEIndex _TYPE_ushort
	#define _TYPE_FRTEIndex _TYPE_ushort
	#define _TYPE_MTEIndex  _TYPE_ushort
	#define _TYPE_TTEIndex  _TYPE_ushort
	#define _DISK_SYMBOL_HEADER_BLOCK DISK_SYMBOL_HEADER_BLOCK_v32
	#define _DISK_TABLE_INFO DISK_TABLE_INFO_v32
	#define _FRTE_INDEX_TABLE_ENTRY FRTE_INDEX_TABLE_ENTRY_v32
	#define _FILE_REFERENCE_TABLE_ENTRY FILE_REFERENCE_TABLE_ENTRY_v32
	#define _CONTAINED_TYPES_TABLE_ENTRY CONTAINED_TYPES_TABLE_ENTRY_v32
	#define _CONTAINED_LABELS_TABLE_ENTRY CONTAINED_LABELS_TABLE_ENTRY_v32
	#define _CONTAINED_STATEMENTS_TABLE_ENTRY CONTAINED_STATEMENTS_TABLE_ENTRY_v32
	#define _CONTAINED_VARIABLES_TABLE_ENTRY CONTAINED_VARIABLES_TABLE_ENTRY_v32
	#define _CONTAINED_MODULES_TABLE_ENTRY CONTAINED_MODULES_TABLE_ENTRY_v32
	#define _RESOURCE_TABLE_ENTRY RESOURCE_TABLE_ENTRY_v32
	#define _FILE_REFERENCE FILE_REFERENCE_v32
	#define _IndexSize IndexSize_v32
	#define _CLTEIndex CLTEIndex_v32
	#define _CMTEIndex CMTEIndex_v32
	#define _CTTEIndex CTTEIndex_v32
	#define _CVTEIndex CVTEIndex_v32
	#define _FRTEIndex FRTEIndex_v32
	#define _MTEIndex MTEIndex_v32
	#define _MTEOffset MTEOffset_v32
	#define _TTEIndex TTEIndex_v32
	#define _END_OF_LIST			END_OF_LIST_v32
	#define _FILE_NAME_INDEX		FILE_NAME_INDEX_v32
	#define _SOURCE_FILE_CHANGE		SOURCE_FILE_CHANGE_v32
	#define _MAXIMUM_LEGAL_INDEX	MAXIMUM_LEGAL_INDEX_v32	/* Must be one less than the above!! */
	#define _CVTE_P CVTE_v32_P
	#define _CMTE_P CMTE_v32_P
	#define _CTTE_P CTTE_v32_P
	#define _CLTE_P CLTE_v32_P
	#define _CSNTE_P CSNTE_v32_P
	#define _DSHB_P DSHB_v32_P
	#define _FITE_P FITE_v32_P
	#define _FREF_P FREF_v32_P
	#define _FRTE_P FRTE_v32_P
	#define _RTE_P RTE_v32_P
//V34
#else	//default...
	#define _TYPE_CVTEMTEIndex _TYPE_ulong
	#define _TYPE_CSNTEIndex _TYPE_ulong
	#define _TYPE_IndexSize _TYPE_ulong
	#define _TYPE_CLTEIndex _TYPE_ulong
	#define _TYPE_CMTEIndex _TYPE_ulong
	#define _TYPE_CTTEIndex _TYPE_ulong
	#define _TYPE_CVTEIndex _TYPE_ulong
	#define _TYPE_FRTEIndex _TYPE_ulong
	#define _TYPE_MTEIndex  _TYPE_ulong
	#define _TYPE_MTEOffset _TYPE_ulong
	#define _TYPE_TTEIndex  _TYPE_ulong
	#define _SYM_FILE_VERSION SYM_FILE_VERSION
	#define _MODULES_TABLE_ENTRY MODULES_TABLE_ENTRY
	#define _CSNTEIndex CSNTEIndex
	#define _MTE_P MTE_P
	#define _DISK_SYMBOL_HEADER_BLOCK DISK_SYMBOL_HEADER_BLOCK
	#define _DISK_TABLE_INFO DISK_TABLE_INFO
	#define _FRTE_INDEX_TABLE_ENTRY FRTE_INDEX_TABLE_ENTRY
	#define _FILE_REFERENCE_TABLE_ENTRY FILE_REFERENCE_TABLE_ENTRY
	#define _CONTAINED_TYPES_TABLE_ENTRY CONTAINED_TYPES_TABLE_ENTRY
	#define _CONTAINED_LABELS_TABLE_ENTRY CONTAINED_LABELS_TABLE_ENTRY
	#define _CONTAINED_STATEMENTS_TABLE_ENTRY CONTAINED_STATEMENTS_TABLE_ENTRY
	#define _CONTAINED_VARIABLES_TABLE_ENTRY CONTAINED_VARIABLES_TABLE_ENTRY
	#define _CONTAINED_MODULES_TABLE_ENTRY CONTAINED_MODULES_TABLE_ENTRY
	#define _RESOURCE_TABLE_ENTRY RESOURCE_TABLE_ENTRY
	#define _FILE_REFERENCE FILE_REFERENCE
	#define _IndexSize IndexSize
	#define _CLTEIndex CLTEIndex
	#define _CMTEIndex CMTEIndex
	#define _CTTEIndex CTTEIndex
	#define _CVTEIndex CVTEIndex
	#define _FRTEIndex FRTEIndex
	#define _MTEIndex MTEIndex
	#define _MTEOffset MTEOffset
	#define _TTEIndex TTEIndex
	#define _END_OF_LIST			END_OF_LIST
	#define _FILE_NAME_INDEX		FILE_NAME_INDEX
	#define _SOURCE_FILE_CHANGE		SOURCE_FILE_CHANGE
	#define _MAXIMUM_LEGAL_INDEX	MAXIMUM_LEGAL_INDEX	/* Must be one less than the above!! */
	#define _CMTE_P CMTE_P
	#define _CVTE_P CVTE_P
	#define _CSNTE_P CSNTE_P
	#define _CTTE_P CTTE_P
	#define _CLTE_P CLTE_P
	#define _DSHB_P DSHB_P
	#define _FRTE_P FRTE_P
	#define _FREF_P FREF_P
	#define _FITE_P FITE_P
	#define _RTE_P RTE_P
#endif
//these types are the same for all the above versions
	#define _TYPE_FITEIndex _TYPE_ushort
	#define _TYPE_FREFIndex _TYPE_ushort
	#define _TYPE_HTEIndex  _TYPE_ushort
	#define _TYPE_RTEIndex  _TYPE_ushort
	#define _TYPE_NTEIndex  _TYPE_ulong

//abbreviations
	#define _MTE _MODULES_TABLE_ENTRY
	#define _DSHB _DISK_SYMBOL_HEADER_BLOCK
	#define _DTI _DISK_TABLE_INFO
	#define _FITE _FRTE_INDEX_TABLE_ENTRY
	#define _FRTE _FILE_REFERENCE_TABLE_ENTRY
	#define _CTTE _CONTAINED_TYPES_TABLE_ENTRY
	#define _DTTE DISK_TYPE_TABLE_ENTRY
	#define _CLTE _CONTAINED_LABELS_TABLE_ENTRY
	#define _CSNTE _CONTAINED_STATEMENTS_TABLE_ENTRY
	#define _CVTE _CONTAINED_VARIABLES_TABLE_ENTRY
	#define _CMTE _CONTAINED_MODULES_TABLE_ENTRY
	#define _RTE _RESOURCE_TABLE_ENTRY
	#define _FREF _FILE_REFERENCE
	#define _SCA STORAGE_CLASS_ADDRESS
	#define _NTE_EXT NTE_EXT_ENTRY
	#define _NTE NAME_TABLE_ENTRY


	
	
