                                        
//======================================
// xsym_defs_v34.h - contains sizeof and offsetof definitions
// 		for XSYM V34 
                                        
// This file was gerernated automatically by file xsym_ver.cpp
// 		to enable alignment & endian independant 
// 		XSYM reading for multiple versions of XSYM
                                        
                                        
                                        
                                        
#ifndef __XSYM_DEFS_V34_H__             
#define __XSYM_DEFS_V34_H__             
#include "xsym_ver.h"            
                                       
                                        
//======================================
// misc sizes                           
#define    SIZEOF_IndexSize_v34          4
#define    SIZEOF_TTEIndex_v34           4
#define    SIZEOF_NTEIndex_v34           4
                                        
//======================================
// DSHB                                 
#define    SIZEOF_DSHB_v34        210
#define    DSHB_ID_OFF_v34        0
#define    DSHB_PG_SZ_OFF_v34     32
#define    DSHB_HASHPG_SZ_OFF_v34 34
#define    DSHB_ROOT_MTE_OFF_v34  38
#define    DSHB_MOD_DATE_OFF_v34  42
#define    DSHB_FRTE_OFF_v34      46
#define    DSHB_RTE_OFF_v34       58
#define    DSHB_MTE_OFF_v34       70
#define    DSHB_CMTE_OFF_v34      82
#define    DSHB_CVTE_OFF_v34      94
#define    DSHB_CSNTE_OFF_v34     106
#define    DSHB_CLTE_OFF_v34      118
#define    DSHB_CTTE_OFF_v34      130
#define    DSHB_TTE_OFF_v34       142
#define    DSHB_NTE_OFF_v34       154
#define    DSHB_TINFO_OFF_v34     166
#define    DSHB_FITE_OFF_v34      178
#define    DSHB_CONST_OFF_v34     190
#define    DSHB_CREATOR_OFF_v34   202
#define    DSHB_TYPE_OFF_v34      206
                                        
//======================================
// CLTE                                 
#define    SIZEOF_CLTE_v34        16
#define    CLTE_FILE_CH_OFF_v34   0
#define    CLTE_FREF_OFF_v34      4
#define    CLTE_MTE_IND_OFF_v34   0
#define    CLTE_MTE_OFF_OFF_v34   4
#define    CLTE_NTE_IND_OFF_v34   8
#define    CLTE_FDELTA_OFF_v34    12
#define    CLTE_SCOPE_OFF_v34     14
                                        
//======================================
// CSNTE                                
#define    SIZEOF_CSNTE_v34       12
#define    CSNTE_FILE_CH_OFF_v34  0
#define    CSNTE_FREF_OFF_v34     4
#define    CSNTE_FDELTA_OFF_v34   4
#define    CSNTE_MTE_IND_OFF_v34  0
#define    CSNTE_MTE_OFF_OFF_v34  6
                                        
//======================================
// CVTE                                 
#define    SIZEOF_CVTE_v34        28
#define    SIZEOF_CVTE_HEAD_v34   12
#define    CVTE_FILE_CH_OFF_v34   0
#define    CVTE_FILE_FR_OFF_v34   4
#define    CVTE_TTE_OFF_v34       0
#define    CVTE_NTE_OFF_v34       4
#define    CVTE_FDELTA_OFF_v34    8
#define    CVTE_SCOPE_OFF_v34     10
#define    CVTE_LASIZE_OFF_v34    11
#define    CVTE_LOC_OFF_v34       12
#define    CVTE_LAS_LA_OFF_v34    12
#define    CVTE_LAS_LAK_OFF_v34   25
#define    CVTE_BLAS_LA_OFF_v34   12
#define    CVTE_BLAS_LAK_OFF_v34  16
                                        
//======================================
// DTI                                  
#define    SIZEOF_DTI_v34         12
#define    DTI_FIRST_PG_OFF_v34   0
#define    DTI_PAGE_CNT_OFF_v34   4
#define    DTI_OBJ_CNT_OFF_v34    8
                                        
//======================================
// CTTE                                 
#define    SIZEOF_CTTE_v34        12
#define    CTTE_FILE_CH_OFF_v34   0
#define    CTTE_FREF_OFF_v34      4
#define    CTTE_TTE_IND_OFF_v34   0
#define    CTTE_NTE_IND_OFF_v34   4
#define    CTTE_FDELTA_OFF_v34    8
                                        
//======================================
// DTTE                                 
#define    SIZEOF_DTTE_v34        12
#define    DTTE_NTE_IND_OFF_v34   0
#define    DTTE_PSIZE_OFF_v34     4
#define    DTTE_WORD_SZ_OFF_v34   6
#define    DTTE_WORD_BZ_OFF_v34   8
#define    DTTE_LONG_SZ_OFF_v34   6
#define    DTTE_LONG_BZ_OFF_v34   10
                                        
//======================================
// NTE & NTE_EXT                        
#define    SIZEOF_NTE_v34         2
#define    SIZEOF_NTE_EXT_v34     6
#define    NTE_EXT_MAG_OFF_v34    0
#define    NTE_EXT_TYPE_OFF_v34   1
#define    NTE_EXT_LEN_OFF_v34    2
#define    NTE_EXT_NAME_OFF_v34   4
                                        
//======================================
// FREF                                 
#define    SIZEOF_FREF_v34        8
#define    FREF_FRTE_OFF_v34      0
#define    FREF_FOFF_OFF_v34      4
                                        
//======================================
// FRTE                                 
#define    SIZEOF_FRTE_v34        12
#define    FRTE_NAME_ENT_OFF_v34  0
#define    FRTE_NTE_IND_OFF_v34   4
#define    FRTE_MOD_DATE_OFF_v34  8
#define    FRTE_MTE_IND_OFF_v34   0
#define    FRTE_FILE_OFF_OFF_v34  4
                                        
//======================================
// FITE                                 
#define    SIZEOF_FITE_v34        8
#define    FITE_FRTE_IND_OFF_v34  0
#define    FITE_NTE_IND_OFF_v34   4
                                        
//======================================
// RTE                                  
#define    SIZEOF_RTE_v34         22
#define    RTE_RES_TYPE_OFF_v34   0
#define    RTE_RES_NUM_OFF_v34    4
#define    RTE_NTE_IND_OFF_v34    6
#define    RTE_MTE_FIRST_OFF_v34  10
#define    RTE_MTE_LAST_OFF_v34   14
#define    RTE_RES_SIZE_OFF_v34   18
                                        
//======================================
// MTE                                  
#define    SIZEOF_MTE_v34         56
#define    MTE_RTE_IND_OFF_v34    0
#define    MTE_RES_OFF_OFF_v34    2
#define    MTE_SIZE_OFF_v34       6
#define    MTE_KIND_OFF_v34       10
#define    MTE_SCOPE_OFF_v34      11
#define    MTE_PARENT_OFF_v34     12
#define    MTE_IMP_FREF_OFF_v34   16
#define    MTE_IMP_END_OFF_v34    24
#define    MTE_NTE_IND_OFF_v34    28
#define    MTE_CMTE_IND_OFF_v34   32
#define    MTE_CVTE_IND_OFF_v34   36
#define    MTE_CLTE_IND_OFF_v34   40
#define    MTE_CTTE_IND_OFF_v34   44
#define    MTE_CSNTE_IND1_OFF_v34 48
#define    MTE_CSNTE_IND2_OFF_v34 52
                                        
//======================================
// SCA                                  
#define    SIZEOF_SCA_v34         6
#define    SCA_KIND_OFF_v34       0
#define    SCA_CLASS_OFF_v34      1
#define    SCA_OFFSET_OFF_v34     2
                                        
//======================================
// HASHTT                                  
#define    SIZEOF_HASHTT_v34      4096
                                        
//======================================
// misc sizes                           
#undef     _SIZEOF_IndexSize              
#undef     _SIZEOF_TTEIndex               
#undef     _SIZEOF_NTEIndex               


//======================================
// main defs                            
#undef DEF_FILE 
#undef XSYM_R_FILE
#undef XSYM_D_FILE 
#undef XsymReader 
#undef XsymDumper 
                                    
//======================================
// DSHB                                 
#undef     _SIZEOF_DSHB            
#undef     _DSHB_ID_OFF            
#undef     _DSHB_PG_SZ_OFF         
#undef     _DSHB_HASHPG_SZ_OFF     
#undef     _DSHB_ROOT_MTE_OFF      
#undef     _DSHB_MOD_DATE_OFF      
#undef     _DSHB_FRTE_OFF          
#undef     _DSHB_RTE_OFF           
#undef     _DSHB_MTE_OFF           
#undef     _DSHB_CMTE_OFF          
#undef     _DSHB_CVTE_OFF          
#undef     _DSHB_CSNTE_OFF          
#undef     _DSHB_CLTE_OFF          
#undef     _DSHB_CTTE_OFF          
#undef     _DSHB_TTE_OFF           
#undef     _DSHB_NTE_OFF           
#undef     _DSHB_TINFO_OFF         
#undef     _DSHB_FITE_OFF          
#undef     _DSHB_CONST_OFF         
#undef     _DSHB_CREATOR_OFF       
#undef     _DSHB_TYPE_OFF          
                                        
//======================================
// CLTE                                 
#undef     _SIZEOF_CLTE            
#undef     _CLTE_FILE_CH_OFF       
#undef     _CLTE_FREF_OFF          
#undef     _CLTE_MTE_IND_OFF       
#undef     _CLTE_MTE_OFF_OFF       
#undef     _CLTE_NTE_IND_OFF       
#undef     _CLTE_FDELTA_OFF        
#undef     _CLTE_SCOPE_OFF         
                                        
//======================================
// CSNTE                                
#undef     _SIZEOF_CSNTE           
#undef     _CSNTE_FILE_CH_OFF      
#undef     _CSNTE_FREF_OFF         
#undef     _CSNTE_FDELTA_OFF       
#undef     _CSNTE_MTE_IND_OFF      
#undef     _CSNTE_MTE_OFF_OFF      
                                        
//======================================
// CVTE                                 
#undef     _SIZEOF_CVTE            
#undef     _SIZEOF_CVTE_HEAD       
#undef     _CVTE_FILE_CH_OFF       
#undef     _CVTE_FILE_FR_OFF       
#undef     _CVTE_TTE_OFF           
#undef     _CVTE_NTE_OFF           
#undef     _CVTE_FDELTA_OFF        
#undef     _CVTE_SCOPE_OFF         
#undef     _CVTE_LASIZE_OFF        
#undef     _CVTE_LOC_OFF           
#undef     _CVTE_LAS_LA_OFF        
#undef     _CVTE_LAS_LAK_OFF       
#undef     _CVTE_BLAS_LA_OFF       
#undef     _CVTE_BLAS_LAK_OFF      
                                        
//======================================
// DTI                                  
#undef     _SIZEOF_DTI             
#undef     _DTI_FIRST_PG_OFF       
#undef     _DTI_PAGE_CNT_OFF       
#undef     _DTI_OBJ_CNT_OFF        
                                        
//======================================
// CTTE                                 
#undef     _SIZEOF_CTTE            
#undef     _CTTE_FILE_CH_OFF       
#undef     _CTTE_FREF_OFF          
#undef     _CTTE_TTE_IND_OFF       
#undef     _CTTE_NTE_IND_OFF       
#undef     _CTTE_FDELTA_OFF        
                                        
//======================================
// DTTE                                 
#undef     _SIZEOF_DTTE            
#undef     _DTTE_NTE_IND_OFF       
#undef     _DTTE_PSIZE_OFF       
#undef     _DTTE_WORD_SZ_OFF       
#undef     _DTTE_WORD_BZ_OFF       
#undef     _DTTE_LONG_SZ_OFF       
#undef     _DTTE_LONG_BZ_OFF       
                                        
//======================================
// NTE & NTE_EXT                        
#undef     _SIZEOF_NTE             
#undef     _SIZEOF_NTE_EXT         
#undef     _NTE_EXT_MAG_OFF        
#undef     _NTE_EXT_TYPE_OFF       
#undef     _NTE_EXT_LEN_OFF        
#undef     _NTE_EXT_NAME_OFF       
                                        
//======================================
// FREF                                 
#undef     _SIZEOF_FREF            
#undef     _FREF_FRTE_OFF          
#undef     _FREF_FOFF_OFF          
                                        
//======================================
// FRTE                                 
#undef     _SIZEOF_FRTE            
#undef     _FRTE_NAME_ENT_OFF      
#undef     _FRTE_NTE_IND_OFF       
#undef     _FRTE_MOD_DATE_OFF      
#undef     _FRTE_MTE_IND_OFF       
#undef     _FRTE_FILE_OFF_OFF      
                                        
//======================================
// FITE                                 
#undef     _SIZEOF_FITE            
#undef     _FITE_FRTE_IND_OFF      
#undef     _FITE_NTE_IND_OFF       
                                        
//======================================
// RTE                                  
#undef     _SIZEOF_RTE             
#undef     _RTE_RES_TYPE_OFF       
#undef     _RTE_RES_NUM_OFF        
#undef     _RTE_NTE_IND_OFF        
#undef     _RTE_MTE_FIRST_OFF      
#undef     _RTE_MTE_LAST_OFF       
#undef     _RTE_RES_SIZE_OFF       
                                        
//======================================
// MTE                                  
#undef     _SIZEOF_MTE             
#undef     _MTE_RTE_IND_OFF        
#undef     _MTE_RES_OFF_OFF        
#undef     _MTE_SIZE_OFF           
#undef     _MTE_KIND_OFF           
#undef     _MTE_SCOPE_OFF          
#undef     _MTE_PARENT_OFF         
#undef     _MTE_IMP_FREF_OFF       
#undef     _MTE_IMP_END_OFF        
#undef     _MTE_NTE_IND_OFF        
#undef     _MTE_CMTE_IND_OFF       
#undef     _MTE_CVTE_IND_OFF       
#undef     _MTE_CLTE_IND_OFF       
#undef     _MTE_CTTE_IND_OFF       
#undef     _MTE_CSNTE_IND1_OFF     
#undef     _MTE_CSNTE_IND2_OFF     
                                        
//======================================
// SCA                                  
#undef     _SIZEOF_SCA             
#undef     _SCA_KIND_OFF           
#undef     _SCA_CLASS_OFF          
#undef     _SCA_OFFSET_OFF         
                                        
//======================================
// HASHTT                                  
#undef     _SIZEOF_HASHTT             
                                        
                                        
//======================================
// main defs                            
#define DEF_FILE    "xsym_defs_v34.h" 
#define XSYM_R_FILE "xsym_r_v34.h"    
#define XSYM_D_FILE "xsym_d_v34.h"    
#define XsymReader  XsymReader_v34     
#define XsymDumper  XsymDumper_v34     
	
//======================================
// misc sizes                           
#define    _SIZEOF_IndexSize              4
#define    _SIZEOF_TTEIndex               4
#define    _SIZEOF_NTEIndex               4
                                        
//======================================
// DSHB                                 
#define    _SIZEOF_DSHB            210
#define    _DSHB_ID_OFF            0
#define    _DSHB_PG_SZ_OFF         32
#define    _DSHB_HASHPG_SZ_OFF     34
#define    _DSHB_ROOT_MTE_OFF      38
#define    _DSHB_MOD_DATE_OFF      42
#define    _DSHB_FRTE_OFF          46
#define    _DSHB_RTE_OFF           58
#define    _DSHB_MTE_OFF           70
#define    _DSHB_CMTE_OFF          82
#define    _DSHB_CVTE_OFF          94
#define    _DSHB_CSNTE_OFF         106
#define    _DSHB_CLTE_OFF          118
#define    _DSHB_CTTE_OFF          130
#define    _DSHB_TTE_OFF           142
#define    _DSHB_NTE_OFF           154
#define    _DSHB_TINFO_OFF         166
#define    _DSHB_FITE_OFF          178
#define    _DSHB_CONST_OFF         190
#define    _DSHB_CREATOR_OFF       202
#define    _DSHB_TYPE_OFF          206
                                        
//======================================
// CLTE                                 
#define    _SIZEOF_CLTE            16
#define    _CLTE_FILE_CH_OFF       0
#define    _CLTE_FREF_OFF          4
#define    _CLTE_MTE_IND_OFF       0
#define    _CLTE_MTE_OFF_OFF       4
#define    _CLTE_NTE_IND_OFF       8
#define    _CLTE_FDELTA_OFF        12
#define    _CLTE_SCOPE_OFF         14
                                        
//======================================
// CSNTE                                
#define    _SIZEOF_CSNTE           12
#define    _CSNTE_FILE_CH_OFF      0
#define    _CSNTE_FREF_OFF         4
#define    _CSNTE_FDELTA_OFF       4
#define    _CSNTE_MTE_IND_OFF      0
#define    _CSNTE_MTE_OFF_OFF      6
                                        
//======================================
// CVTE                                 
#define    _SIZEOF_CVTE            28
#define    _SIZEOF_CVTE_HEAD       12
#define    _CVTE_FILE_CH_OFF       0
#define    _CVTE_FILE_FR_OFF       4
#define    _CVTE_TTE_OFF           0
#define    _CVTE_NTE_OFF           4
#define    _CVTE_FDELTA_OFF        8
#define    _CVTE_SCOPE_OFF         10
#define    _CVTE_LASIZE_OFF        11
#define    _CVTE_LOC_OFF           12
#define    _CVTE_LAS_LA_OFF        12
#define    _CVTE_LAS_LAK_OFF       25
#define    _CVTE_BLAS_LA_OFF       12
#define    _CVTE_BLAS_LAK_OFF      16
                                        
//======================================
// DTI                                  
#define    _SIZEOF_DTI             12
#define    _DTI_FIRST_PG_OFF       0
#define    _DTI_PAGE_CNT_OFF       4
#define    _DTI_OBJ_CNT_OFF        8
                                        
//======================================
// CTTE                                 
#define    _SIZEOF_CTTE            12
#define    _CTTE_FILE_CH_OFF       0
#define    _CTTE_FREF_OFF          4
#define    _CTTE_TTE_IND_OFF       0
#define    _CTTE_NTE_IND_OFF       4
#define    _CTTE_FDELTA_OFF        8
                                        
//======================================
// DTTE                                 
#define    _SIZEOF_DTTE            12
#define    _DTTE_NTE_IND_OFF       0   
#define    _DTTE_PSIZE_OFF         4
#define    _DTTE_WORD_SZ_OFF       6
#define    _DTTE_WORD_BZ_OFF       8
#define    _DTTE_LONG_SZ_OFF       6
#define    _DTTE_LONG_BZ_OFF       10
                                        
//======================================
// NTE & NTE_EXT                        
#define    _SIZEOF_NTE             2
#define    _SIZEOF_NTE_EXT         6
#define    _NTE_EXT_MAG_OFF        0
#define    _NTE_EXT_TYPE_OFF       1
#define    _NTE_EXT_LEN_OFF        2
#define    _NTE_EXT_NAME_OFF       4
                                        
//======================================
// FREF                                 
#define    _SIZEOF_FREF            8
#define    _FREF_FRTE_OFF          0
#define    _FREF_FOFF_OFF          4
                                        
//======================================
// FRTE                                 
#define    _SIZEOF_FRTE            12
#define    _FRTE_NAME_ENT_OFF      0
#define    _FRTE_NTE_IND_OFF       4
#define    _FRTE_MOD_DATE_OFF      8
#define    _FRTE_MTE_IND_OFF       0
#define    _FRTE_FILE_OFF_OFF      4
                                        
//======================================
// FITE                                 
#define    _SIZEOF_FITE            8
#define    _FITE_FRTE_IND_OFF      0
#define    _FITE_NTE_IND_OFF       4
                                        
//======================================
// RTE                                  
#define    _SIZEOF_RTE             22
#define    _RTE_RES_TYPE_OFF       0
#define    _RTE_RES_NUM_OFF        4
#define    _RTE_NTE_IND_OFF        6
#define    _RTE_MTE_FIRST_OFF      10
#define    _RTE_MTE_LAST_OFF       14
#define    _RTE_RES_SIZE_OFF       18
                                        
//======================================
// MTE                                  
#define    _SIZEOF_MTE             56
#define    _MTE_RTE_IND_OFF        0
#define    _MTE_RES_OFF_OFF        2
#define    _MTE_SIZE_OFF           6
#define    _MTE_KIND_OFF           10
#define    _MTE_SCOPE_OFF          11
#define    _MTE_PARENT_OFF         12
#define    _MTE_IMP_FREF_OFF       16
#define    _MTE_IMP_END_OFF        24
#define    _MTE_NTE_IND_OFF        28
#define    _MTE_CMTE_IND_OFF       32
#define    _MTE_CVTE_IND_OFF       36
#define    _MTE_CLTE_IND_OFF       40
#define    _MTE_CTTE_IND_OFF       44
#define    _MTE_CSNTE_IND1_OFF     48
#define    _MTE_CSNTE_IND2_OFF     52
                                        
//======================================
// SCA                                  
#define    _SIZEOF_SCA             6
#define    _SCA_KIND_OFF           0
#define    _SCA_CLASS_OFF          1
#define    _SCA_OFFSET_OFF         2
                                        
//======================================
// HASHTT                                  
#define    _SIZEOF_HASHTT             4096
                                        
                                        
#endif /* __XSYM_DEFS_V34_H__ */        

