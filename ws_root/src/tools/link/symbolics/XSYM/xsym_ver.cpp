//automatic generator for XSYM struct sizes/offsets 

//call this function on any new XSYM version to generate
//an include file automatically
//include the resulting file in xsym_ver.h (see xsym_ver.h)
//REMEMBER !!!  68K struct alignment is REQUIRED!!!!!!!!


//change as needed to generate desired version
#undef __XSYM_VER__ 
#include "xsym_verdefs.h"
#define __XSYM_VER__ __XSYM_V33__


#pragma options align=mac68k

#include <stdio.h>
#include "DebugDataTypes.h"
#include "xsym.h"
#include "xsym_ver.h"
#include "stddef.h"
#include "stdlib.h"


FILE* fp;
void gen_offsets() {

	if ((fp=fopen(DEF_FILE,"w+"))==NULL) {
		printf("unable to open file %s\n",DEF_FILE);
		exit(0);
		}
		
//======================================================================
// sizeof/offsetof defines for this version
		
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// %s - contains sizeof and offsetof definitions\n",DEF_FILE);
	fprintf(fp,"// 		for XSYM V%d \n",__XSYM_VER__);
	fprintf(fp,"                                        \n");
	fprintf(fp,"// This file was gerernated automatically by file %s\n",__FILE__);
	fprintf(fp,"// 		to enable alignment & endian independant \n");
	fprintf(fp,"// 		XSYM reading for multiple versions of XSYM\n");
	fprintf(fp,"                                        \n");
	fprintf(fp,"                                        \n");
	fprintf(fp,"                                        \n");
 
	fprintf(fp,"                                        \n");
	fprintf(fp,"#ifndef __XSYM_DEFS_V%d_H__             \n",__XSYM_VER__);
	fprintf(fp,"#define __XSYM_DEFS_V%d_H__             \n",__XSYM_VER__);
	fprintf(fp,"                                        \n");
	fprintf(fp,"#include \"xsym_ver.h\"                 \n");
          

//misc sizes
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// misc sizes                           \n");
	fprintf(fp,"#define    SIZEOF_IndexSize_v%d          %d\n",__XSYM_VER__,sizeof(_IndexSize));
	fprintf(fp,"#define    SIZEOF_TTEIndex_v%d           %d\n",__XSYM_VER__,sizeof(_TTEIndex));
	fprintf(fp,"#define    SIZEOF_NTEIndex_v%d           %d\n",__XSYM_VER__,sizeof(NTEIndex));
//DSHB
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// DSHB                                 \n");
	fprintf(fp,"#define    SIZEOF_DSHB_v%d        %d\n",__XSYM_VER__,sizeof(_DSHB));
	fprintf(fp,"#define    DSHB_ID_OFF_v%d        %d\n",__XSYM_VER__,offsetof(_DSHB,dshb_id));
	fprintf(fp,"#define    DSHB_PG_SZ_OFF_v%d     %d\n",__XSYM_VER__,offsetof(_DSHB,dshb_page_size));
	fprintf(fp,"#define    DSHB_HASHPG_SZ_OFF_v%d %d\n",__XSYM_VER__,offsetof(_DSHB,dshb_hash_page));
	fprintf(fp,"#define    DSHB_ROOT_MTE_OFF_v%d  %d\n",__XSYM_VER__,offsetof(_DSHB,dshb_root_mte));
	fprintf(fp,"#define    DSHB_MOD_DATE_OFF_v%d  %d\n",__XSYM_VER__,offsetof(_DSHB,dshb_mod_date));
	fprintf(fp,"#define    DSHB_FRTE_OFF_v%d      %d\n",__XSYM_VER__,offsetof(_DSHB,dshb_frte));
	fprintf(fp,"#define    DSHB_RTE_OFF_v%d       %d\n",__XSYM_VER__,offsetof(_DSHB,dshb_rte));
	fprintf(fp,"#define    DSHB_MTE_OFF_v%d       %d\n",__XSYM_VER__,offsetof(_DSHB,dshb_mte));
	fprintf(fp,"#define    DSHB_CMTE_OFF_v%d      %d\n",__XSYM_VER__,offsetof(_DSHB,dshb_cmte));
	fprintf(fp,"#define    DSHB_CVTE_OFF_v%d      %d\n",__XSYM_VER__,offsetof(_DSHB,dshb_cvte));
	fprintf(fp,"#define    DSHB_CSNTE_OFF_v%d     %d\n",__XSYM_VER__,offsetof(_DSHB,dshb_csnte));
	fprintf(fp,"#define    DSHB_CLTE_OFF_v%d      %d\n",__XSYM_VER__,offsetof(_DSHB,dshb_clte));
	fprintf(fp,"#define    DSHB_CTTE_OFF_v%d      %d\n",__XSYM_VER__,offsetof(_DSHB,dshb_ctte));
	fprintf(fp,"#define    DSHB_TTE_OFF_v%d       %d\n",__XSYM_VER__,offsetof(_DSHB,dshb_tte));
	fprintf(fp,"#define    DSHB_NTE_OFF_v%d       %d\n",__XSYM_VER__,offsetof(_DSHB,dshb_nte));
	fprintf(fp,"#define    DSHB_TINFO_OFF_v%d     %d\n",__XSYM_VER__,offsetof(_DSHB,dshb_tinfo));
	fprintf(fp,"#define    DSHB_FITE_OFF_v%d      %d\n",__XSYM_VER__,offsetof(_DSHB,dshb_fite));
	fprintf(fp,"#define    DSHB_CONST_OFF_v%d     %d\n",__XSYM_VER__,offsetof(_DSHB,dshb_const));
	fprintf(fp,"#define    DSHB_CREATOR_OFF_v%d   %d\n",__XSYM_VER__,offsetof(_DSHB,dshb_file_creator));
	fprintf(fp,"#define    DSHB_TYPE_OFF_v%d      %d\n",__XSYM_VER__,offsetof(_DSHB,dshb_file_type));
//CLTE
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// CLTE                                 \n");
	fprintf(fp,"#define    SIZEOF_CLTE_v%d        %d\n",__XSYM_VER__,sizeof(_CLTE));
	fprintf(fp,"#define    CLTE_FILE_CH_OFF_v%d   %d\n",__XSYM_VER__,offsetof(_CLTE,clte_file_change));
	fprintf(fp,"#define    CLTE_FREF_OFF_v%d      %d\n",__XSYM_VER__,offsetof(_CLTE,clte_fref));
	fprintf(fp,"#define    CLTE_MTE_IND_OFF_v%d   %d\n",__XSYM_VER__,offsetof(_CLTE,clte_mte_index));
	fprintf(fp,"#define    CLTE_MTE_OFF_OFF_v%d   %d\n",__XSYM_VER__,offsetof(_CLTE,clte_mte_offset));
	fprintf(fp,"#define    CLTE_NTE_IND_OFF_v%d   %d\n",__XSYM_VER__,offsetof(_CLTE,clte_nte_index));
	fprintf(fp,"#define    CLTE_FDELTA_OFF_v%d    %d\n",__XSYM_VER__,offsetof(_CLTE,clte_file_delta));
	fprintf(fp,"#define    CLTE_SCOPE_OFF_v%d     %d\n",__XSYM_VER__,offsetof(_CLTE,clte_scope));
//CSNTE
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// CSNTE                                \n");
	fprintf(fp,"#define    SIZEOF_CSNTE_v%d       %d\n",__XSYM_VER__,sizeof(_CSNTE));
	fprintf(fp,"#define    CSNTE_FILE_CH_OFF_v%d  %d\n",__XSYM_VER__,offsetof(_CSNTE,csnte_file_change));
	fprintf(fp,"#define    CSNTE_FREF_OFF_v%d     %d\n",__XSYM_VER__,offsetof(_CSNTE,csnte_fref));
	fprintf(fp,"#define    CSNTE_FDELTA_OFF_v%d   %d\n",__XSYM_VER__,offsetof(_CSNTE,csnte_file_delta));
	fprintf(fp,"#define    CSNTE_MTE_IND_OFF_v%d  %d\n",__XSYM_VER__,offsetof(_CSNTE,csnte_mte_index));
	fprintf(fp,"#define    CSNTE_MTE_OFF_OFF_v%d  %d\n",__XSYM_VER__,offsetof(_CSNTE,csnte_mte_offset));
//CVTE
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// CVTE                                 \n");
	fprintf(fp,"#define    SIZEOF_CVTE_v%d        %d\n",__XSYM_VER__,sizeof(_CVTE));
	fprintf(fp,"#define    SIZEOF_CVTE_HEAD_v%d   %d\n",__XSYM_VER__,offsetof(_CVTE,cvte_location));
	fprintf(fp,"#define    CVTE_FILE_CH_OFF_v%d   %d\n",__XSYM_VER__,offsetof(_CVTE,cvte_file_change));
	fprintf(fp,"#define    CVTE_FILE_FR_OFF_v%d   %d\n",__XSYM_VER__,offsetof(_CVTE,cvte_fref));
	fprintf(fp,"#define    CVTE_TTE_OFF_v%d       %d\n",__XSYM_VER__,offsetof(_CVTE,cvte_tte_index));
	fprintf(fp,"#define    CVTE_NTE_OFF_v%d       %d\n",__XSYM_VER__,offsetof(_CVTE,cvte_nte_index));
	fprintf(fp,"#define    CVTE_FDELTA_OFF_v%d    %d\n",__XSYM_VER__,offsetof(_CVTE,cvte_file_delta));
	fprintf(fp,"#define    CVTE_SCOPE_OFF_v%d     %d\n",__XSYM_VER__,offsetof(_CVTE,cvte_scope));
	fprintf(fp,"#define    CVTE_LASIZE_OFF_v%d    %d\n",__XSYM_VER__,offsetof(_CVTE,cvte_la_size));
	fprintf(fp,"#define    CVTE_LOC_OFF_v%d       %d\n",__XSYM_VER__,offsetof(_CVTE,cvte_location));
	fprintf(fp,"#define    CVTE_LAS_LA_OFF_v%d    %d\n",__XSYM_VER__,offsetof(_CVTE,cvte_la));
	fprintf(fp,"#define    CVTE_LAS_LAK_OFF_v%d   %d\n",__XSYM_VER__,offsetof(_CVTE,cvte_la_kind));
	fprintf(fp,"#define    CVTE_BLAS_LA_OFF_v%d   %d\n",__XSYM_VER__,offsetof(_CVTE,cvte_big_la));
	fprintf(fp,"#define    CVTE_BLAS_LAK_OFF_v%d  %d\n",__XSYM_VER__,offsetof(_CVTE,cvte_big_la_kind));
//DTI
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// DTI                                  \n");
	fprintf(fp,"#define    SIZEOF_DTI_v%d         %d\n",__XSYM_VER__,sizeof(_DTI));
	fprintf(fp,"#define    DTI_FIRST_PG_OFF_v%d   %d\n",__XSYM_VER__,offsetof(_DTI,dti_first_page));
	fprintf(fp,"#define    DTI_PAGE_CNT_OFF_v%d   %d\n",__XSYM_VER__,offsetof(_DTI,dti_page_count));
	fprintf(fp,"#define    DTI_OBJ_CNT_OFF_v%d    %d\n",__XSYM_VER__,offsetof(_DTI,dti_object_count));
//CTTE
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// CTTE                                 \n");
	fprintf(fp,"#define    SIZEOF_CTTE_v%d        %d\n",__XSYM_VER__,sizeof(_CTTE));
	fprintf(fp,"#define    CTTE_FILE_CH_OFF_v%d   %d\n",__XSYM_VER__,offsetof(_CTTE,ctte_file_change));
	fprintf(fp,"#define    CTTE_FREF_OFF_v%d      %d\n",__XSYM_VER__,offsetof(_CTTE,ctte_fref));
	fprintf(fp,"#define    CTTE_TTE_IND_OFF_v%d   %d\n",__XSYM_VER__,offsetof(_CTTE,ctte_tte_index));
	fprintf(fp,"#define    CTTE_NTE_IND_OFF_v%d   %d\n",__XSYM_VER__,offsetof(_CTTE,ctte_nte_index));
	fprintf(fp,"#define    CTTE_FDELTA_OFF_v%d    %d\n",__XSYM_VER__,offsetof(_CTTE,ctte_file_delta));
//DTTE
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// DTTE                                 \n");
	fprintf(fp,"#define    SIZEOF_DTTE_v%d        %d\n",__XSYM_VER__,sizeof(_DTTE));
	fprintf(fp,"#define    DTTE_NTE_IND_OFF_v%d   %d\n",__XSYM_VER__,offsetof(_DTTE,dtte_nte));
	fprintf(fp,"#define    DTTE_PSIZE_OFF_v%d     %d\n",__XSYM_VER__,offsetof(_DTTE,dtte_psize));
	fprintf(fp,"#define    DTTE_WORD_SZ_OFF_v%d   %d\n",__XSYM_VER__,offsetof(_DTTE,dtte_word_size));
	fprintf(fp,"#define    DTTE_WORD_BZ_OFF_v%d   %d\n",__XSYM_VER__,offsetof(_DTTE,dtte_word_bytes));
	fprintf(fp,"#define    DTTE_LONG_SZ_OFF_v%d   %d\n",__XSYM_VER__,offsetof(_DTTE,dtte_long_size));
	fprintf(fp,"#define    DTTE_LONG_BZ_OFF_v%d   %d\n",__XSYM_VER__,offsetof(_DTTE,dtte_long_bytes));
//NTE & NTE_EXT (extended name, when size==255)
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// NTE & NTE_EXT                        \n");
	fprintf(fp,"#define    SIZEOF_NTE_v%d         %d\n",__XSYM_VER__,sizeof(_NTE));
	fprintf(fp,"#define    SIZEOF_NTE_EXT_v%d     %d\n",__XSYM_VER__,sizeof(_NTE_EXT));
	fprintf(fp,"#define    NTE_EXT_MAG_OFF_v%d    %d\n",__XSYM_VER__,offsetof(_NTE_EXT,nte_ext_magic));
	fprintf(fp,"#define    NTE_EXT_TYPE_OFF_v%d   %d\n",__XSYM_VER__,offsetof(_NTE_EXT,nte_ext_type));
	fprintf(fp,"#define    NTE_EXT_LEN_OFF_v%d    %d\n",__XSYM_VER__,offsetof(_NTE_EXT,nte_ext_length));
	fprintf(fp,"#define    NTE_EXT_NAME_OFF_v%d   %d\n",__XSYM_VER__,offsetof(_NTE_EXT,nte_ext_text));
//FREF
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// FREF                                 \n");
	fprintf(fp,"#define    SIZEOF_FREF_v%d        %d\n",__XSYM_VER__,sizeof(_FREF));
	fprintf(fp,"#define    FREF_FRTE_OFF_v%d      %d\n",__XSYM_VER__,offsetof(_FREF,fref_frte_index));
	fprintf(fp,"#define    FREF_FOFF_OFF_v%d      %d\n",__XSYM_VER__,offsetof(_FREF,fref_offset));
//FRTE
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// FRTE                                 \n");
	fprintf(fp,"#define    SIZEOF_FRTE_v%d        %d\n",__XSYM_VER__,sizeof(_FRTE));
	fprintf(fp,"#define    FRTE_NAME_ENT_OFF_v%d  %d\n",__XSYM_VER__,offsetof(_FRTE,frte_name_entry));
	fprintf(fp,"#define    FRTE_NTE_IND_OFF_v%d   %d\n",__XSYM_VER__,offsetof(_FRTE,frte_nte_index));
	fprintf(fp,"#define    FRTE_MOD_DATE_OFF_v%d  %d\n",__XSYM_VER__,offsetof(_FRTE,frte_mod_date));
	fprintf(fp,"#define    FRTE_MTE_IND_OFF_v%d   %d\n",__XSYM_VER__,offsetof(_FRTE,frte_mte_index));
	fprintf(fp,"#define    FRTE_FILE_OFF_OFF_v%d  %d\n",__XSYM_VER__,offsetof(_FRTE,frte_file_offset));
//FITE
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// FITE                                 \n");
	fprintf(fp,"#define    SIZEOF_FITE_v%d        %d\n",__XSYM_VER__,sizeof(_FITE));
	fprintf(fp,"#define    FITE_FRTE_IND_OFF_v%d  %d\n",__XSYM_VER__,offsetof(_FITE,fite_frte_index));
	fprintf(fp,"#define    FITE_NTE_IND_OFF_v%d   %d\n",__XSYM_VER__,offsetof(_FITE,fite_nte_index));
//RTE
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// RTE                                  \n");
	fprintf(fp,"#define    SIZEOF_RTE_v%d         %d\n",__XSYM_VER__,sizeof(_RTE));
	fprintf(fp,"#define    RTE_RES_TYPE_OFF_v%d   %d\n",__XSYM_VER__,offsetof(_RTE,rte_ResType));
	fprintf(fp,"#define    RTE_RES_NUM_OFF_v%d    %d\n",__XSYM_VER__,offsetof(_RTE,rte_res_number));
	fprintf(fp,"#define    RTE_NTE_IND_OFF_v%d    %d\n",__XSYM_VER__,offsetof(_RTE,rte_nte_index));
	fprintf(fp,"#define    RTE_MTE_FIRST_OFF_v%d  %d\n",__XSYM_VER__,offsetof(_RTE,rte_mte_first));
	fprintf(fp,"#define    RTE_MTE_LAST_OFF_v%d   %d\n",__XSYM_VER__,offsetof(_RTE,rte_mte_last));
	fprintf(fp,"#define    RTE_RES_SIZE_OFF_v%d   %d\n",__XSYM_VER__,offsetof(_RTE,rte_res_size));
//MTE
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// MTE                                  \n");
	fprintf(fp,"#define    SIZEOF_MTE_v%d         %d\n",__XSYM_VER__,sizeof(_MTE));
	fprintf(fp,"#define    MTE_RTE_IND_OFF_v%d    %d\n",__XSYM_VER__,offsetof(_MTE,mte_rte_index));
	fprintf(fp,"#define    MTE_RES_OFF_OFF_v%d    %d\n",__XSYM_VER__,offsetof(_MTE,mte_res_offset));
	fprintf(fp,"#define    MTE_SIZE_OFF_v%d       %d\n",__XSYM_VER__,offsetof(_MTE,mte_size));
	fprintf(fp,"#define    MTE_KIND_OFF_v%d       %d\n",__XSYM_VER__,offsetof(_MTE,mte_kind));
	fprintf(fp,"#define    MTE_SCOPE_OFF_v%d      %d\n",__XSYM_VER__,offsetof(_MTE,mte_scope));
	fprintf(fp,"#define    MTE_PARENT_OFF_v%d     %d\n",__XSYM_VER__,offsetof(_MTE,mte_parent));
	fprintf(fp,"#define    MTE_IMP_FREF_OFF_v%d   %d\n",__XSYM_VER__,offsetof(_MTE,mte_imp_fref));
	fprintf(fp,"#define    MTE_IMP_END_OFF_v%d    %d\n",__XSYM_VER__,offsetof(_MTE,mte_imp_end));
	fprintf(fp,"#define    MTE_NTE_IND_OFF_v%d    %d\n",__XSYM_VER__,offsetof(_MTE,mte_nte_index));
	fprintf(fp,"#define    MTE_CMTE_IND_OFF_v%d   %d\n",__XSYM_VER__,offsetof(_MTE,mte_cmte_index));
	fprintf(fp,"#define    MTE_CVTE_IND_OFF_v%d   %d\n",__XSYM_VER__,offsetof(_MTE,mte_cvte_index));
	fprintf(fp,"#define    MTE_CLTE_IND_OFF_v%d   %d\n",__XSYM_VER__,offsetof(_MTE,mte_clte_index));
	fprintf(fp,"#define    MTE_CTTE_IND_OFF_v%d   %d\n",__XSYM_VER__,offsetof(_MTE,mte_ctte_index));
	fprintf(fp,"#define    MTE_CSNTE_IND1_OFF_v%d %d\n",__XSYM_VER__,offsetof(_MTE,mte_csnte_idx_1));
	fprintf(fp,"#define    MTE_CSNTE_IND2_OFF_v%d %d\n",__XSYM_VER__,offsetof(_MTE,mte_csnte_idx_2));
//SCA
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// SCA                                  \n");
	fprintf(fp,"#define    SIZEOF_SCA_v%d         %d\n",__XSYM_VER__,sizeof(_SCA));
	fprintf(fp,"#define    SCA_KIND_OFF_v%d       %d\n",__XSYM_VER__,offsetof(_SCA,sca_kind));
	fprintf(fp,"#define    SCA_CLASS_OFF_v%d      %d\n",__XSYM_VER__,offsetof(_SCA,sca_class));
	fprintf(fp,"#define    SCA_OFFSET_OFF_v%d     %d\n",__XSYM_VER__,offsetof(_SCA,sca_offset));
//HASHTT
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// HASHTT                                  \n");
	fprintf(fp,"#define    SIZEOF_HASHTT_v%d      %d\n",__XSYM_VER__,sizeof(HASH_TABLE_TYPE));


//======================================================================
//======================================================================
// now, to make sure the generic defines work...

// first need to undef in case one of my clones got included...

//main defs
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// main defs                            \n");
	fprintf(fp,"#undef     DEF_FILE                     \n");
	fprintf(fp,"#undef     XSYM_R_FILE                  \n");
	fprintf(fp,"#undef     XSYM_D_FILE                  \n");
	fprintf(fp,"#undef     XsymReader                   \n");
	fprintf(fp,"#undef     XsymDumper                   \n");
//misc sizes
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// misc sizes                           \n");
	fprintf(fp,"#undef     _SIZEOF_IndexSize            \n");
	fprintf(fp,"#undef     _SIZEOF_TTEIndex             \n");
	fprintf(fp,"#undef     _SIZEOF_NTEIndex             \n");
//DSHB
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// DSHB                                 \n");
	fprintf(fp,"#undef     _SIZEOF_DSHB            \n");
	fprintf(fp,"#undef     _DSHB_ID_OFF            \n");
	fprintf(fp,"#undef     _DSHB_PG_SZ_OFF         \n");
	fprintf(fp,"#undef     _DSHB_HASHPG_SZ_OFF     \n");
	fprintf(fp,"#undef     _DSHB_ROOT_MTE_OFF      \n");
	fprintf(fp,"#undef     _DSHB_MOD_DATE_OFF      \n");
	fprintf(fp,"#undef     _DSHB_FRTE_OFF          \n");
	fprintf(fp,"#undef     _DSHB_RTE_OFF           \n");
	fprintf(fp,"#undef     _DSHB_MTE_OFF           \n");
	fprintf(fp,"#undef     _DSHB_CMTE_OFF          \n");
	fprintf(fp,"#undef     _DSHB_CVTE_OFF          \n");
	fprintf(fp,"#undef     _DSHB_CSNTE_OFF         \n");
	fprintf(fp,"#undef     _DSHB_CLTE_OFF          \n");
	fprintf(fp,"#undef     _DSHB_CTTE_OFF          \n");
	fprintf(fp,"#undef     _DSHB_TTE_OFF           \n");
	fprintf(fp,"#undef     _DSHB_NTE_OFF           \n");
	fprintf(fp,"#undef     _DSHB_TINFO_OFF         \n");
	fprintf(fp,"#undef     _DSHB_FITE_OFF          \n");
	fprintf(fp,"#undef     _DSHB_CONST_OFF         \n");
	fprintf(fp,"#undef     _DSHB_CREATOR_OFF       \n");
	fprintf(fp,"#undef     _DSHB_TYPE_OFF          \n");
//CLTE
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// CLTE                                 \n");
	fprintf(fp,"#undef     _SIZEOF_CLTE            \n");
	fprintf(fp,"#undef     _CLTE_FILE_CH_OFF       \n");
	fprintf(fp,"#undef     _CLTE_FREF_OFF          \n");
	fprintf(fp,"#undef     _CLTE_MTE_IND_OFF       \n");
	fprintf(fp,"#undef     _CLTE_MTE_OFF_OFF       \n");
	fprintf(fp,"#undef     _CLTE_NTE_IND_OFF       \n");
	fprintf(fp,"#undef     _CLTE_FDELTA_OFF        \n");
	fprintf(fp,"#undef     _CLTE_SCOPE_OFF         \n");
//CSNTE
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// CSNTE                                \n");
	fprintf(fp,"#undef     _SIZEOF_CSNTE           \n");
	fprintf(fp,"#undef     _CSNTE_FILE_CH_OFF      \n");
	fprintf(fp,"#undef     _CSNTE_FREF_OFF         \n");
	fprintf(fp,"#undef     _CSNTE_FDELTA_OFF       \n");
	fprintf(fp,"#undef     _CSNTE_MTE_IND_OFF      \n");
	fprintf(fp,"#undef     _CSNTE_MTE_OFF_OFF      \n");
//CVTE
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// CVTE                                 \n");
	fprintf(fp,"#undef     _SIZEOF_CVTE            \n");
	fprintf(fp,"#undef     _SIZEOF_CVTE_HEAD       \n");
	fprintf(fp,"#undef     _CVTE_FILE_CH_OFF       \n");
	fprintf(fp,"#undef     _CVTE_FILE_FR_OFF       \n");
	fprintf(fp,"#undef     _CVTE_TTE_OFF           \n");
	fprintf(fp,"#undef     _CVTE_NTE_OFF           \n");
	fprintf(fp,"#undef     _CVTE_FDELTA_OFF        \n");
	fprintf(fp,"#undef     _CVTE_SCOPE_OFF         \n");
	fprintf(fp,"#undef     _CVTE_LASIZE_OFF        \n");
	fprintf(fp,"#undef     _CVTE_LOC_OFF           \n");
	fprintf(fp,"#undef     _CVTE_LAS_LA_OFF        \n");
	fprintf(fp,"#undef     _CVTE_LAS_LAK_OFF       \n");
	fprintf(fp,"#undef     _CVTE_BLAS_LA_OFF       \n");
	fprintf(fp,"#undef     _CVTE_BLAS_LAK_OFF      \n");
//DTI
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// DTI                                  \n");
	fprintf(fp,"#undef     _SIZEOF_DTI             \n");
	fprintf(fp,"#undef     _DTI_FIRST_PG_OFF       \n");
	fprintf(fp,"#undef     _DTI_PAGE_CNT_OFF       \n");
	fprintf(fp,"#undef     _DTI_OBJ_CNT_OFF        \n");
//CTTE
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// CTTE                                 \n");
	fprintf(fp,"#undef     _SIZEOF_CTTE            \n");
	fprintf(fp,"#undef     _CTTE_FILE_CH_OFF       \n");
	fprintf(fp,"#undef     _CTTE_FREF_OFF          \n");
	fprintf(fp,"#undef     _CTTE_TTE_IND_OFF       \n");
	fprintf(fp,"#undef     _CTTE_NTE_IND_OFF       \n");
	fprintf(fp,"#undef     _CTTE_FDELTA_OFF        \n");
//DTTE
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// DTTE                                 \n");
	fprintf(fp,"#undef     _SIZEOF_DTTE            \n");
	fprintf(fp,"#undef     _DTTE_NTE_IND_OFF       \n");
	fprintf(fp,"#undef     _DTTE_PSIZE_OFF         \n");
	fprintf(fp,"#undef     _DTTE_WORD_SZ_OFF       \n");
	fprintf(fp,"#undef     _DTTE_WORD_BZ_OFF       \n");
	fprintf(fp,"#undef     _DTTE_LONG_SZ_OFF       \n");
	fprintf(fp,"#undef     _DTTE_LONG_BZ_OFF       \n");
//NTE & NTE_EXT (extended name, when size==255)
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// NTE & NTE_EXT                        \n");
	fprintf(fp,"#undef     _SIZEOF_NTE             \n");
	fprintf(fp,"#undef     _SIZEOF_NTE_EXT         \n");
	fprintf(fp,"#undef     _NTE_EXT_MAG_OFF        \n");
	fprintf(fp,"#undef     _NTE_EXT_TYPE_OFF       \n");
	fprintf(fp,"#undef     _NTE_EXT_LEN_OFF        \n");
	fprintf(fp,"#undef     _NTE_EXT_NAME_OFF       \n");
//FREF
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// FREF                                 \n");
	fprintf(fp,"#undef     _SIZEOF_FREF            \n");
	fprintf(fp,"#undef     _FREF_FRTE_OFF          \n");
	fprintf(fp,"#undef     _FREF_FOFF_OFF          \n");
//FRTE
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// FRTE                                 \n");
	fprintf(fp,"#undef     _SIZEOF_FRTE            \n");
	fprintf(fp,"#undef     _FRTE_NAME_ENT_OFF      \n");
	fprintf(fp,"#undef     _FRTE_NTE_IND_OFF       \n");
	fprintf(fp,"#undef     _FRTE_MOD_DATE_OFF      \n");
	fprintf(fp,"#undef     _FRTE_MTE_IND_OFF       \n");
	fprintf(fp,"#undef     _FRTE_FILE_OFF_OFF      \n");
//FITE
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// FITE                                 \n");
	fprintf(fp,"#undef     _SIZEOF_FITE            \n");
	fprintf(fp,"#undef     _FITE_FRTE_IND_OFF      \n");
	fprintf(fp,"#undef     _FITE_NTE_IND_OFF       \n");
//RTE
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// RTE                                  \n");
	fprintf(fp,"#undef     _SIZEOF_RTE             \n");
	fprintf(fp,"#undef     _RTE_RES_TYPE_OFF       \n");
	fprintf(fp,"#undef     _RTE_RES_NUM_OFF        \n");
	fprintf(fp,"#undef     _RTE_NTE_IND_OFF        \n");
	fprintf(fp,"#undef     _RTE_MTE_FIRST_OFF      \n");
	fprintf(fp,"#undef     _RTE_MTE_LAST_OFF       \n");
	fprintf(fp,"#undef     _RTE_RES_SIZE_OFF       \n");
//MTE
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// MTE                                  \n");
	fprintf(fp,"#undef     _SIZEOF_MTE             \n");
	fprintf(fp,"#undef     _MTE_RTE_IND_OFF        \n");
	fprintf(fp,"#undef     _MTE_RES_OFF_OFF        \n");
	fprintf(fp,"#undef     _MTE_SIZE_OFF           \n");
	fprintf(fp,"#undef     _MTE_KIND_OFF           \n");
	fprintf(fp,"#undef     _MTE_SCOPE_OFF          \n");
	fprintf(fp,"#undef     _MTE_PARENT_OFF         \n");
	fprintf(fp,"#undef     _MTE_IMP_FREF_OFF       \n");
	fprintf(fp,"#undef     _MTE_IMP_END_OFF        \n");
	fprintf(fp,"#undef     _MTE_NTE_IND_OFF        \n");
	fprintf(fp,"#undef     _MTE_CMTE_IND_OFF       \n");
	fprintf(fp,"#undef     _MTE_CVTE_IND_OFF       \n");
	fprintf(fp,"#undef     _MTE_CLTE_IND_OFF       \n");
	fprintf(fp,"#undef     _MTE_CTTE_IND_OFF       \n");
	fprintf(fp,"#undef     _MTE_CSNTE_IND1_OFF     \n");
	fprintf(fp,"#undef     _MTE_CSNTE_IND2_OFF     \n");
//SCA
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// SCA                                  \n");
	fprintf(fp,"#undef     _SIZEOF_SCA             \n");
	fprintf(fp,"#undef     _SCA_KIND_OFF           \n");
	fprintf(fp,"#undef     _SCA_CLASS_OFF          \n");
	fprintf(fp,"#undef     _SCA_OFFSET_OFF         \n");
//HASHTT
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// HASHTT                                  \n");
	fprintf(fp,"#undef     _SIZEOF_HASHTT             \n");

//======================================================================
// now, the generic defines...

	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// main defs                            \n");
	fprintf(fp,"#define DEF_FILE    \"xsym_defs_v%d.h\" \n",__XSYM_VER__);
	fprintf(fp,"#define XSYM_R_FILE \"xsym_r_v%d.h\"    \n",__XSYM_VER__);
	fprintf(fp,"#define XSYM_D_FILE \"xsym_d_v%d.h\"    \n",__XSYM_VER__);
	fprintf(fp,"#define XsymReader  XsymReader_v%d      \n",__XSYM_VER__);
	fprintf(fp,"#define XsymDumper  XsymDumper_v%d      \n",__XSYM_VER__);
//misc sizes
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// misc sizes                           \n");
	fprintf(fp,"#define    _SIZEOF_IndexSize              %d\n",sizeof(_IndexSize));
	fprintf(fp,"#define    _SIZEOF_TTEIndex               %d\n",sizeof(_TTEIndex));
	fprintf(fp,"#define    _SIZEOF_NTEIndex               %d\n",sizeof(NTEIndex));
//DSHB
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// DSHB                                 \n");
	fprintf(fp,"#define    _SIZEOF_DSHB            %d\n",sizeof(_DSHB));
	fprintf(fp,"#define    _DSHB_ID_OFF            %d\n",offsetof(_DSHB,dshb_id));
	fprintf(fp,"#define    _DSHB_PG_SZ_OFF         %d\n",offsetof(_DSHB,dshb_page_size));
	fprintf(fp,"#define    _DSHB_HASHPG_SZ_OFF     %d\n",offsetof(_DSHB,dshb_hash_page));
	fprintf(fp,"#define    _DSHB_ROOT_MTE_OFF      %d\n",offsetof(_DSHB,dshb_root_mte));
	fprintf(fp,"#define    _DSHB_MOD_DATE_OFF      %d\n",offsetof(_DSHB,dshb_mod_date));
	fprintf(fp,"#define    _DSHB_FRTE_OFF          %d\n",offsetof(_DSHB,dshb_frte));
	fprintf(fp,"#define    _DSHB_RTE_OFF           %d\n",offsetof(_DSHB,dshb_rte));
	fprintf(fp,"#define    _DSHB_MTE_OFF           %d\n",offsetof(_DSHB,dshb_mte));
	fprintf(fp,"#define    _DSHB_CMTE_OFF          %d\n",offsetof(_DSHB,dshb_cmte));
	fprintf(fp,"#define    _DSHB_CVTE_OFF          %d\n",offsetof(_DSHB,dshb_cvte));
	fprintf(fp,"#define    _DSHB_CSNTE_OFF         %d\n",offsetof(_DSHB,dshb_csnte));
	fprintf(fp,"#define    _DSHB_CLTE_OFF          %d\n",offsetof(_DSHB,dshb_clte));
	fprintf(fp,"#define    _DSHB_CTTE_OFF          %d\n",offsetof(_DSHB,dshb_ctte));
	fprintf(fp,"#define    _DSHB_TTE_OFF           %d\n",offsetof(_DSHB,dshb_tte));
	fprintf(fp,"#define    _DSHB_NTE_OFF           %d\n",offsetof(_DSHB,dshb_nte));
	fprintf(fp,"#define    _DSHB_TINFO_OFF         %d\n",offsetof(_DSHB,dshb_tinfo));
	fprintf(fp,"#define    _DSHB_FITE_OFF          %d\n",offsetof(_DSHB,dshb_fite));
	fprintf(fp,"#define    _DSHB_CONST_OFF         %d\n",offsetof(_DSHB,dshb_const));
	fprintf(fp,"#define    _DSHB_CREATOR_OFF       %d\n",offsetof(_DSHB,dshb_file_creator));
	fprintf(fp,"#define    _DSHB_TYPE_OFF          %d\n",offsetof(_DSHB,dshb_file_type));
//CLTE
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// CLTE                                 \n");
	fprintf(fp,"#define    _SIZEOF_CLTE            %d\n",sizeof(_CLTE));
	fprintf(fp,"#define    _CLTE_FILE_CH_OFF       %d\n",offsetof(_CLTE,clte_file_change));
	fprintf(fp,"#define    _CLTE_FREF_OFF          %d\n",offsetof(_CLTE,clte_fref));
	fprintf(fp,"#define    _CLTE_MTE_IND_OFF       %d\n",offsetof(_CLTE,clte_mte_index));
	fprintf(fp,"#define    _CLTE_MTE_OFF_OFF       %d\n",offsetof(_CLTE,clte_mte_offset));
	fprintf(fp,"#define    _CLTE_NTE_IND_OFF       %d\n",offsetof(_CLTE,clte_nte_index));
	fprintf(fp,"#define    _CLTE_FDELTA_OFF        %d\n",offsetof(_CLTE,clte_file_delta));
	fprintf(fp,"#define    _CLTE_SCOPE_OFF         %d\n",offsetof(_CLTE,clte_scope));
//CSNTE
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// CSNTE                                \n");
	fprintf(fp,"#define    _SIZEOF_CSNTE           %d\n",sizeof(_CSNTE));
	fprintf(fp,"#define    _CSNTE_FILE_CH_OFF      %d\n",offsetof(_CSNTE,csnte_file_change));
	fprintf(fp,"#define    _CSNTE_FREF_OFF         %d\n",offsetof(_CSNTE,csnte_fref));
	fprintf(fp,"#define    _CSNTE_FDELTA_OFF       %d\n",offsetof(_CSNTE,csnte_file_delta));
	fprintf(fp,"#define    _CSNTE_MTE_IND_OFF      %d\n",offsetof(_CSNTE,csnte_mte_index));
	fprintf(fp,"#define    _CSNTE_MTE_OFF_OFF      %d\n",offsetof(_CSNTE,csnte_mte_offset));
//CVTE
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// CVTE                                 \n");
	fprintf(fp,"#define    _SIZEOF_CVTE            %d\n",sizeof(_CVTE));
	fprintf(fp,"#define    _SIZEOF_CVTE_HEAD       %d\n",offsetof(_CVTE,cvte_location));
	fprintf(fp,"#define    _CVTE_FILE_CH_OFF       %d\n",offsetof(_CVTE,cvte_file_change));
	fprintf(fp,"#define    _CVTE_FILE_FR_OFF       %d\n",offsetof(_CVTE,cvte_fref));
	fprintf(fp,"#define    _CVTE_TTE_OFF           %d\n",offsetof(_CVTE,cvte_tte_index));
	fprintf(fp,"#define    _CVTE_NTE_OFF           %d\n",offsetof(_CVTE,cvte_nte_index));
	fprintf(fp,"#define    _CVTE_FDELTA_OFF        %d\n",offsetof(_CVTE,cvte_file_delta));
	fprintf(fp,"#define    _CVTE_SCOPE_OFF         %d\n",offsetof(_CVTE,cvte_scope));
	fprintf(fp,"#define    _CVTE_LASIZE_OFF        %d\n",offsetof(_CVTE,cvte_la_size));
	fprintf(fp,"#define    _CVTE_LOC_OFF           %d\n",offsetof(_CVTE,cvte_location));
	fprintf(fp,"#define    _CVTE_LAS_LA_OFF        %d\n",offsetof(_CVTE,cvte_la));
	fprintf(fp,"#define    _CVTE_LAS_LAK_OFF       %d\n",offsetof(_CVTE,cvte_la_kind));
	fprintf(fp,"#define    _CVTE_BLAS_LA_OFF       %d\n",offsetof(_CVTE,cvte_big_la));
	fprintf(fp,"#define    _CVTE_BLAS_LAK_OFF      %d\n",offsetof(_CVTE,cvte_big_la_kind));
//DTI
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// DTI                                  \n");
	fprintf(fp,"#define    _SIZEOF_DTI             %d\n",sizeof(_DTI));
	fprintf(fp,"#define    _DTI_FIRST_PG_OFF       %d\n",offsetof(_DTI,dti_first_page));
	fprintf(fp,"#define    _DTI_PAGE_CNT_OFF       %d\n",offsetof(_DTI,dti_page_count));
	fprintf(fp,"#define    _DTI_OBJ_CNT_OFF        %d\n",offsetof(_DTI,dti_object_count));
//CTTE
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// CTTE                                 \n");
	fprintf(fp,"#define    _SIZEOF_CTTE            %d\n",sizeof(_CTTE));
	fprintf(fp,"#define    _CTTE_FILE_CH_OFF       %d\n",offsetof(_CTTE,ctte_file_change));
	fprintf(fp,"#define    _CTTE_FREF_OFF          %d\n",offsetof(_CTTE,ctte_fref));
	fprintf(fp,"#define    _CTTE_TTE_IND_OFF       %d\n",offsetof(_CTTE,ctte_tte_index));
	fprintf(fp,"#define    _CTTE_NTE_IND_OFF       %d\n",offsetof(_CTTE,ctte_nte_index));
	fprintf(fp,"#define    _CTTE_FDELTA_OFF        %d\n",offsetof(_CTTE,ctte_file_delta));
//DTTE
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// DTTE                                 \n");
	fprintf(fp,"#define    _SIZEOF_DTTE            %d\n",sizeof(_DTTE));
	fprintf(fp,"#define    _DTTE_NTE_IND_OFF       %d\n",__XSYM_VER__,offsetof(_DTTE,dtte_nte));
	fprintf(fp,"#define    _DTTE_PSIZE_OFF         %d\n",__XSYM_VER__,offsetof(_DTTE,dtte_psize));
	fprintf(fp,"#define    _DTTE_WORD_SZ_OFF       %d\n",offsetof(_DTTE,dtte_word_size));
	fprintf(fp,"#define    _DTTE_WORD_BZ_OFF       %d\n",offsetof(_DTTE,dtte_word_bytes));
	fprintf(fp,"#define    _DTTE_LONG_SZ_OFF       %d\n",offsetof(_DTTE,dtte_long_size));
	fprintf(fp,"#define    _DTTE_LONG_BZ_OFF       %d\n",offsetof(_DTTE,dtte_long_bytes));
//NTE & NTE_EXT (extended name, when size==255)
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// NTE & NTE_EXT                        \n");
	fprintf(fp,"#define    _SIZEOF_NTE             %d\n",sizeof(_NTE));
	fprintf(fp,"#define    _SIZEOF_NTE_EXT         %d\n",sizeof(_NTE_EXT));
	fprintf(fp,"#define    _NTE_EXT_MAG_OFF        %d\n",offsetof(_NTE_EXT,nte_ext_magic));
	fprintf(fp,"#define    _NTE_EXT_TYPE_OFF       %d\n",offsetof(_NTE_EXT,nte_ext_type));
	fprintf(fp,"#define    _NTE_EXT_LEN_OFF        %d\n",offsetof(_NTE_EXT,nte_ext_length));
	fprintf(fp,"#define    _NTE_EXT_NAME_OFF       %d\n",offsetof(_NTE_EXT,nte_ext_text));
//FREF
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// FREF                                 \n");
	fprintf(fp,"#define    _SIZEOF_FREF            %d\n",sizeof(_FREF));
	fprintf(fp,"#define    _FREF_FRTE_OFF          %d\n",offsetof(_FREF,fref_frte_index));
	fprintf(fp,"#define    _FREF_FOFF_OFF          %d\n",offsetof(_FREF,fref_offset));
//FRTE
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// FRTE                                 \n");
	fprintf(fp,"#define    _SIZEOF_FRTE            %d\n",sizeof(_FRTE));
	fprintf(fp,"#define    _FRTE_NAME_ENT_OFF      %d\n",offsetof(_FRTE,frte_name_entry));
	fprintf(fp,"#define    _FRTE_NTE_IND_OFF       %d\n",offsetof(_FRTE,frte_nte_index));
	fprintf(fp,"#define    _FRTE_MOD_DATE_OFF      %d\n",offsetof(_FRTE,frte_mod_date));
	fprintf(fp,"#define    _FRTE_MTE_IND_OFF       %d\n",offsetof(_FRTE,frte_mte_index));
	fprintf(fp,"#define    _FRTE_FILE_OFF_OFF      %d\n",offsetof(_FRTE,frte_file_offset));
//FITE
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// FITE                                 \n");
	fprintf(fp,"#define    _SIZEOF_FITE            %d\n",sizeof(_FITE));
	fprintf(fp,"#define    _FITE_FRTE_IND_OFF      %d\n",offsetof(_FITE,fite_frte_index));
	fprintf(fp,"#define    _FITE_NTE_IND_OFF       %d\n",offsetof(_FITE,fite_nte_index));
//RTE
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// RTE                                  \n");
	fprintf(fp,"#define    _SIZEOF_RTE             %d\n",sizeof(_RTE));
	fprintf(fp,"#define    _RTE_RES_TYPE_OFF       %d\n",offsetof(_RTE,rte_ResType));
	fprintf(fp,"#define    _RTE_RES_NUM_OFF        %d\n",offsetof(_RTE,rte_res_number));
	fprintf(fp,"#define    _RTE_NTE_IND_OFF        %d\n",offsetof(_RTE,rte_nte_index));
	fprintf(fp,"#define    _RTE_MTE_FIRST_OFF      %d\n",offsetof(_RTE,rte_mte_first));
	fprintf(fp,"#define    _RTE_MTE_LAST_OFF       %d\n",offsetof(_RTE,rte_mte_last));
	fprintf(fp,"#define    _RTE_RES_SIZE_OFF       %d\n",offsetof(_RTE,rte_res_size));
//MTE
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// MTE                                  \n");
	fprintf(fp,"#define    _SIZEOF_MTE             %d\n",sizeof(_MTE));
	fprintf(fp,"#define    _MTE_RTE_IND_OFF        %d\n",offsetof(_MTE,mte_rte_index));
	fprintf(fp,"#define    _MTE_RES_OFF_OFF        %d\n",offsetof(_MTE,mte_res_offset));
	fprintf(fp,"#define    _MTE_SIZE_OFF           %d\n",offsetof(_MTE,mte_size));
	fprintf(fp,"#define    _MTE_KIND_OFF           %d\n",offsetof(_MTE,mte_kind));
	fprintf(fp,"#define    _MTE_SCOPE_OFF          %d\n",offsetof(_MTE,mte_scope));
	fprintf(fp,"#define    _MTE_PARENT_OFF         %d\n",offsetof(_MTE,mte_parent));
	fprintf(fp,"#define    _MTE_IMP_FREF_OFF       %d\n",offsetof(_MTE,mte_imp_fref));
	fprintf(fp,"#define    _MTE_IMP_END_OFF        %d\n",offsetof(_MTE,mte_imp_end));
	fprintf(fp,"#define    _MTE_NTE_IND_OFF        %d\n",offsetof(_MTE,mte_nte_index));
	fprintf(fp,"#define    _MTE_CMTE_IND_OFF       %d\n",offsetof(_MTE,mte_cmte_index));
	fprintf(fp,"#define    _MTE_CVTE_IND_OFF       %d\n",offsetof(_MTE,mte_cvte_index));
	fprintf(fp,"#define    _MTE_CLTE_IND_OFF       %d\n",offsetof(_MTE,mte_clte_index));
	fprintf(fp,"#define    _MTE_CTTE_IND_OFF       %d\n",offsetof(_MTE,mte_ctte_index));
	fprintf(fp,"#define    _MTE_CSNTE_IND1_OFF     %d\n",offsetof(_MTE,mte_csnte_idx_1));
	fprintf(fp,"#define    _MTE_CSNTE_IND2_OFF     %d\n",offsetof(_MTE,mte_csnte_idx_2));
//SCA
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// SCA                                  \n");
	fprintf(fp,"#define    _SIZEOF_SCA             %d\n",sizeof(_SCA));
	fprintf(fp,"#define    _SCA_KIND_OFF           %d\n",offsetof(_SCA,sca_kind));
	fprintf(fp,"#define    _SCA_CLASS_OFF          %d\n",offsetof(_SCA,sca_class));
	fprintf(fp,"#define    _SCA_OFFSET_OFF         %d\n",offsetof(_SCA,sca_offset));
//HASHTT
	fprintf(fp,"                                        \n");
	fprintf(fp,"//======================================\n");
	fprintf(fp,"// HASHTT                                  \n");
	fprintf(fp,"#define    _SIZEOF_HASHTT             %d\n",sizeof(HASH_TABLE_TYPE));

//======================================================================
//======================================================================
//DONE!!
	fprintf(fp,"                                        \n");
	fprintf(fp,"                                        \n");
	fprintf(fp,"#endif /* __XSYM_DEFS_V%d_H__ */        \n\n",__XSYM_VER__);
	
	
	fclose(fp);
	}
	
// Structures can now revert to default alignment 
#pragma options align=reset
