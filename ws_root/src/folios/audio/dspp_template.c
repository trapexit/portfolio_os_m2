/* @(#) dspp_template.c 96/08/23 1.70 */
/* $Id: dspp_loader.c,v 1.105 1995/03/10 20:35:59 peabody Exp phil $ */
/****************************************************************
**
** DSPPTemplate and DSPPInstrument creation/deletion services
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
*****************************************************************
** 930617 PLB Use afi_DeleteLinkedItems in dsppFreeInstrument
** 930704 PLB Free resources if allocation fails in DSPPLoadPatch
** 930817 PLB Free memory for unrecognised chunk.
** 930824 PLB Added support for ID_DHDR
** 930824 PLB Only count external references to allow ADPCM to be freed.
** 930908 PLB Init I Mem to zero when allocated to reduce pops.
** 931215 PLB Make Knob and Port name matching case insensitive.
** 940224 PLB Move Attachment list to aitp to prepare for shared dtmp.
** 940502 PLB Add support for audio input.
** 940609 PLB Added shared library template support.
** 940811 PLB Used %.4s to print ChunkTypes instead of scratch array kludge.
** 940812 PLB Added half rate calculation:
**            Allocate Ticks from even or odd frame.
**            DSPPJumpTo() handles jump from execsplit.dsp to odd frame list.
** 940912 PLB Use Read16 to read uint16 values for NuPup
** 941102 PLB Fix bad error cleanup in DSPPLoadPatch
** 941222 WJB Added normally commented-out call DisassemblDSPP().
** 950109 WJB Removed (uint16) casts from Write16() calls. Saves an instruction.
** 950112 WJB Added test call to dsppRemapHardwareReferences().
** 950115 PLB Removed dead duck code.
** 950116 PLB Avoid allocating FIFO 4 which is messed up by Expansion port DMA.
**            See CR4151 in Clarify.
** 950116 PLB Don't deallocate ADC resources because they are not really allocated.
** 950118 PLB Remove FIFO 4 allocation order change from 950116.  Not needed.
** 950118 WJB Privatized Put16() and DSPPRelocateAll().
** 950118 WJB Groundwork changes in DSPPRelocate() to cooperate with abs address remapping.
** 950119 WJB Renamed dsppRemapHardwareReferences() to dsppRemapAbsoluteAddresses().
** 950119 WJB Made dsppRelocate() fixupFunc() callback system more general.
** 950120 WJB Privatized DSPPVerifyDataInitializer(). Added const to DSPPValidateTemplate().
** 950123 WJB Rolled DSPPValidateTemplate() and DSPPCloneTemplate() into dsppCreateSuperTemplate().
**            Moved disasm and remap hacks into dsppCreateSuperTemplate().
** 950123 WJB Added description text to dspnDisassemble().
** 950124 WJB Replaced StripInsTemplate() with dsppStripTemplate() and bullet-proofed it.
** 950124 WJB Fixed memory list corruption under low memory problem in dsppCloneTemplate(). Fixes CR 4196.
** 950125 WJB Optimized CLONEMEMBER() macro in dsppCloneTemplate().
** 950125 WJB Changed arg list for dsppRemapAbsoluteAddresses().
** 950126 PLB Moved overwrite of SLEEP to DSPPStartCodeExecution() in dspp_instr.c (Fixes CR 4215)
** 950130 WJB Integrated call to dsppRemapAbsoluteAddresses() into real code.
** 950130 WJB Replaced opcode defines with new ones from dspp_instructions.h.
** 950131 WJB Replaced dtmp_FunctionID with dtmp_Header.
** 950202 WJB Replaced magic #s in GetRsrcAttribute() w/ suitable DSPI_FIFO macros
** 950206 WJB Moved resource code to dspp_resource.c.
** 950208 WJB Made dsppVerifyDataInitializer() aware of imported resources.
** 950208 WJB Added usage of DRSC_TYPE_MASK in DSPPFindResource().
** 950214 WJB Made dins_Template==NULL trap PARANOID code in DSPPGetRsrcName().
** 950214 WJB Cleaned up DSPPGetRsrcName() source code slightly (no object code change).
** 950216 WJB Now using dsppOpen/CloseInstrumentResources().
** 950217 WJB Added an error message.
** 950217 WJB Added explicit calls to dsppExport/UnexportInstrumentResources() in preparation for
**            improved library template management.
** 950301 PLB Set direction of DMA Channels when instrument allocated.
** 950303 WJB Added usage of dsppIsValidResourceType() during template validation.
** 950307 WJB Added DSPPTemplate arg to dsppRelocate().
** 950307 WJB Added asic-dependent RBASE8 instruction writer to dsppRelocate().
** 950310 WJB Now including dspp_remap.h.
** 950412 WJB Added relocation support for DRSC_RBASE.
** 950412 WJB Added a note about DRSC_F_BIND.
** 950417 WJB Now using dsppValidateTemplateResources().
** 950417 PLB Added dsppFindResourceIndex()
** 950501 WJB Moved dsppRelocate() to dspp_relocator.c.
** 950501 WJB Converted DSPPGetRsrcName() into dsppGetTemplateRsrcName().
** 950501 WJB Moved DSPPRelocateAll() to dspp_relocator.c.
** 950601 WJB Now using correct lengths for IsRamAddr() calls in dsppValidateTemplate().
** 950612 WJB Added ASIC-specific data memory access.
** 950615 PLB Added dsppPresetInsMemory().
** 950718 WJB Added dsppCreate/DeleteUserTemplate().
** 950814 WJB Now calling dsppValidateRelocation().
** 950825 PLB Set hardware data decompression based on FIFO SubType
** 950922 WJB Now only exports/unexports for DHDR_F_PRIVILEGED templates.
** 960219 WJB Added dsppCreate/DeleteSuperTemplate(). Reorganized.
** 960326 PLB Clear SQS2 and 8BIT flags on Output DMA channels.
****************************************************************/

#include <audio/dspp_template.h>            /* self */
#include <dspptouch/dspp_instructions.h>    /* DSPP opcodes */
#include <dspptouch/dspp_touch.h>           /* dsphWriteCodeMem(), et al */
#include <dspptouch/touch_hardware.h>

#include "audio_folio_modes.h"
#include "audio_internal.h"
#include "dspp_resources.h"     /* resource allocation */
#include "ezmem_tools.h"        /* instrument allocation */

#if 0       /* @@@ remapping disabled for M2 */
#include "dspp_remap.h"         /* absolute address remapper */
#endif

#ifdef AF_ASIC_OPERA
#include "dspp_imem.h"          /* dsphWriteIMem() */
#endif

#define DEBUG_Clone                 0       /* debug dsppCloneTemplate() */
#define DEBUG_DumpSourceTemplate    0       /* dump contents of user-mode DSPPTemplate about to be cloned */
#define DEBUG_DumpFinalTemplate     0       /* dump contents of blessed supervisor mode DSPPTemplate */
#define DEBUG_DumpDownloadedCode    0

#define DBUG(x) /* PRT(x) */
#define LOG(x)  /* LogEvent(x) */

#if DEBUG_Clone
	#define DBUGCLONE(x)    PRT(x)
#else
	#define DBUGCLONE(x)
#endif


/* -------------------- user-mode DSPPTemplate creation/destruction */

/**
|||	AUTODOC -private -class audio -group DSPPTemplate -name dsppCreateUserTemplate
|||	Creates an empty user-mode DSPPTemplate.
|||
|||	  Synopsis
|||
|||	    DSPPTemplate *dsppCreateUserTemplate (void)
|||
|||	  Description
|||
|||	    Create an empty user-mode DSPPTemplate suitable for passing through
|||	    AF_TAG_TEMPLATE to create a Instrument Template Item. Caller may hang
|||	    MEMTYPE_TRACKSIZE elements off of this DSPPTemplate (e.g., dtmp_Resources),
|||	    which get automatically freed by dsppDeleteUserTemplate().
|||
|||	    Call dsppDeleteUserTemplate() to delete user-mode DSPPTemplate after using
|||	    it.
|||
|||	  Return Value
|||
|||	    Pointer to new user-mode DSPPTemplate. NULL if insufficient memory.
|||
|||	  Implementation
|||
|||	    Folio call implemented in audio folio V30.
|||
|||	  Associated Files
|||
|||	    <audio/dspp_template.h>, System.m2/Modules/audio
|||
|||	  See Also
|||
|||	    dsppDeleteUserTemplate()
**/
DSPPTemplate *dsppCreateUserTemplate (void)
{
	return (DSPPTemplate *)AllocMem (sizeof(DSPPTemplate), MEMTYPE_FILL);
}

/**
|||	AUTODOC -private -class audio -group DSPPTemplate -name dsppDeleteUserTemplate
|||	Deletes user-mode DSPPTemplate.
|||
|||	  Synopsis
|||
|||	    void dsppDeleteUserTemplate (DSPPTemplate *dtmp)
|||
|||	  Description
|||
|||	    Delete a user-mode DSPPTemplate created by dsppCreateUserTemplate() including
|||	    all MEMTYPE_TRACKSIZE members hanging off it (e.g., dspp_Resources).
|||
|||	  Arguments
|||
|||	    dtmp
|||	        Pointer to user-mode DSPPTemplate to delete. Can be NULL or partially
|||	        complete.
|||
|||	  Implementation
|||
|||	    Folio call implemented in audio folio V30.
|||
|||	  Associated Files
|||
|||	    <audio/dspp_template.h>, System.m2/Modules/audio
|||
|||	  See Also
|||
|||	    dsppCreateUserTemplate()
**/
void dsppDeleteUserTemplate (DSPPTemplate *dtmp)
{
	if (dtmp) {
		FreeMem (dtmp->dtmp_Resources,        TRACKED_SIZE);
		FreeMem (dtmp->dtmp_ResourceNames,    TRACKED_SIZE);
		FreeMem (dtmp->dtmp_Relocations,      TRACKED_SIZE);
		FreeMem (dtmp->dtmp_Codes,            TRACKED_SIZE);
		FreeMem (dtmp->dtmp_DataInitializer,  TRACKED_SIZE);
		FreeMem (dtmp->dtmp_DynamicLinkNames, TRACKED_SIZE);
		FreeMem (dtmp, sizeof *dtmp);
	}
}


/* -------------------- supervisor-mode DSPPTemplate creation/destruction */

/*
	Create an empty supervisor-mode DSPPTemplate suitable for hanging off of
	an Instrument Template Item.

	Call dsppDeleteSuperTemplate() to delete supervisor-mode DSPPTemplate after
	when done with it.

	Results
		Pointer to new supervisor-mode DSPPTemplate. NULL if insufficient
        memory.
*/
DSPPTemplate *dsppCreateSuperTemplate (void)
{
	return (DSPPTemplate *)SuperAllocMem (sizeof(DSPPTemplate), MEMTYPE_FILL);
}

/*
	Delete a supervisor-mode DSPPTemplate including any MEMTYPE_TRACKSIZE
	membe	hanging off it.

	Arguments
		dtmp
			Pointer to supervisor-mode DSPPTemplate to delete. Can be NULL or
			partially complete. Ignores dtmp_ShareCount; that is managed at a
			higher level.
*/
void dsppDeleteSuperTemplate (DSPPTemplate *dtmp)
{
	if (dtmp) {
		SuperFreeMem (dtmp->dtmp_Resources,        TRACKED_SIZE);
		SuperFreeMem (dtmp->dtmp_ResourceNames,    TRACKED_SIZE);
		SuperFreeMem (dtmp->dtmp_Relocations,      TRACKED_SIZE);
		SuperFreeMem (dtmp->dtmp_Codes,            TRACKED_SIZE);
		SuperFreeMem (dtmp->dtmp_DataInitializer,  TRACKED_SIZE);
		SuperFreeMem (dtmp->dtmp_DynamicLinkNames, TRACKED_SIZE);
		SuperFreeMem (dtmp, sizeof *dtmp);
	}
}


/* -------------------- dsppPromoteTemplate() */

static Err dsppValidateTemplate (const DSPPTemplate *);
static Err dsppValidateDynamicLinkNames (const DSPPTemplate *);
static int32 dsppVerifyDataInitializer (const DSPPTemplate *);
static DSPPTemplate *dsppCloneTemplate (const DSPPTemplate *srcTemplate);

/*
	Build supervisor mode DSPPTemplate based on one constructed from FORM DSPP
	contents.

	Arguments
		resultTemplate
			Buffer to store resulting supervisor-mode DSPPTemplate.

		srcTemplate
			User-mode DSPPTemplate to promote.

	Results
		Returns 0 on success, Err code on failure.
		Stores result in *resultTemplate if successful.
*/
Err dsppPromoteTemplate (DSPPTemplate **resultTemplate, const DSPPTemplate *srcTemplate)
{
	DSPPTemplate *dtmp = NULL;
	Err errcode;

	DBUG(("dsppPromoteTemplate(): srcTemplate=$%08lx\n", srcTemplate));

		/* clear result */
	*resultTemplate = NULL;

#if DEBUG_DumpSourceTemplate
	PRT(("dsppPromoteTemplate: SOURCE template --------\n"));
	dsppDumpTemplate( srcTemplate, NULL );
#endif

		/* validate source template */
	if ((errcode = dsppValidateTemplate (srcTemplate)) < 0) goto clean;

		/* make supervisor copy of source template */
	if ((dtmp = dsppCloneTemplate (srcTemplate)) == NULL) {
		errcode = AF_ERR_NOMEM;
		goto clean;
	}

  #if 0     /* @@@ remapping disabled for M2 */
		/* perform absolute address remapping on result template */
	if ((errcode = dsppRemapAbsoluteAddresses (dtmp)) < 0) goto clean;
  #endif

#if DEBUG_DumpFinalTemplate
	PRT(("dsppPromoteTemplate: CLONED template --------\n"));
	dsppDumpTemplate( dtmp, NULL );
#endif

		/* set result */
	DBUG(("dsppPromoteTemplate(): resultTemplate=$%08lx\n", dtmp));
	*resultTemplate = dtmp;
	return 0;

clean:
	dsppDeleteSuperTemplate (dtmp);
	return errcode;
}

/*
    Verify that contents of userTemplate is valid prior to making real DSPPTemplate
*/
static Err dsppValidateTemplate( const DSPPTemplate *dtmp )
{
	int32 Result;

	/* !!! surround a lot of this with BUILD_PARANOIA */

/* Validate Template structure address. */
	if ((Result = afi_IsRamAddr (dtmp, sizeof *dtmp)) < 0) return Result;

/* Validate header. */
	if( dtmp->dtmp_Header.dhdr_FormatVersion != DHDR_SUPPORTED_FORMAT_VERSION )
	{
		ERR(("dsppValidateTemplate: Unsupported DSPP format version!\n"));
		return AF_ERR_BADOFX;
	}

/* Validate pointers */
/* @@@ depends on IsRamAddr() returning TRUE for length == 0 */
	if (afi_IsRamAddr (dtmp->dtmp_Resources,       dtmp->dtmp_NumResources * sizeof dtmp->dtmp_Resources[0])     < 0) return AF_ERR_BADOFX;
	if (afi_IsRamAddr (dtmp->dtmp_ResourceNames,   1 /* !!! wrong length */ )                                    < 0) return AF_ERR_BADOFX;
	if (afi_IsRamAddr (dtmp->dtmp_Relocations,     dtmp->dtmp_NumRelocations * sizeof dtmp->dtmp_Relocations[0]) < 0) return AF_ERR_BADOFX;
	if (afi_IsRamAddr (dtmp->dtmp_Codes,           dtmp->dtmp_CodeSize)                                          < 0) return AF_ERR_BADOFX;
	if (afi_IsRamAddr (dtmp->dtmp_DataInitializer, dtmp->dtmp_DataInitializerSize)                               < 0) return AF_ERR_BADOFX;
	if (afi_IsRamAddr (dtmp->dtmp_DynamicLinkNames, dtmp->dtmp_DynamicLinkNamesSize)                             < 0) return AF_ERR_BADOFX;

/*
	!!! check for legal function id:
	. reject DFID_TEST when !defined(BUILD_DEBUGGER)
*/

/* Verify that resource allocations are legal - no sense in waiting until resource is actually allocated when we can check here */
	if ((Result = dsppValidateTemplateResources(dtmp)) < 0) return Result;

/* Verify that resource names are not too long */
	if (dtmp->dtmp_NumResources) {
		const char *drscname = dtmp->dtmp_ResourceNames;
		int32 numresources = dtmp->dtmp_NumResources;

		/* !!! could validate pointers here, too, I suppose */

		while (numresources--) {
			if (strlen (drscname) > AF_MAX_NAME_LENGTH) return AF_ERR_NAME_TOO_LONG;
			drscname = NextPackedString (drscname);
		}
	}

/* Verify that DynamicLinkNames are legal. */
	if ((Result = dsppValidateDynamicLinkNames(dtmp)) < 0) return Result;

/* Verify that Data Initializer (if there is one) is valid */
	Result = dsppVerifyDataInitializer( dtmp );
	if(Result < 0) return Result;

/* Validate relocations */
	{
		const DSPPRelocation *drlc = dtmp->dtmp_Relocations;
		int32 numRelocs = dtmp->dtmp_NumRelocations;

		for (; numRelocs--; drlc++)
		{
			if ((Result = dsppValidateRelocation (dtmp, drlc)) < 0) return Result;
		}
	}

	/* !!! add more traps:
		. code hunks (new loop)
			. multiple code hunks
			. code hunk types other than DCOD_DSPP_RUN
		. check offsets into name chunk?
		. minimum OS required to use instrument
		. SiliconVersion against some max and possibly taking into account compatibility mode enable
		. Knob Types in Resources
		. Default values in Resources
		. don't allow bound code resources that aren't exported.
		. don't allow user instruments to export code symbols.
		    --- these two are expectations of the patch builder so that it doesn't have to follow
		        bound code resources, which it should never actually encounter
	*/

	return Result;
}

/*
	Validate contents of dtmp_DynamicLinkNames (if present).
		. Checks max name length of each name.
		. Makes sure last name is NULL terminated.

	Arguments
		dtmp
			DSPPTemplate to check. Pointers are assumed to be valid.

	Return Value
		0 on success, Err code if invalid
*/
static Err dsppValidateDynamicLinkNames (const DSPPTemplate *dtmp)
{
	if( dtmp->dtmp_DynamicLinkNames )
	{
			/* check for NUL termination of last string. ensures that below loop won't run off the end */
		if (dtmp->dtmp_DynamicLinkNames [ dtmp->dtmp_DynamicLinkNamesSize-1 ] != '\0') return AF_ERR_BADOFX;

			/* check each name against max name length */
		{
			const char *dlnkname = dtmp->dtmp_DynamicLinkNames;
			const char * const dlnkname_end = dtmp->dtmp_DynamicLinkNames + dtmp->dtmp_DynamicLinkNamesSize;

			while (dlnkname < dlnkname_end)
			{
				const size_t len = strlen (dlnkname);

				if (len > AF_MAX_NAME_LENGTH) return AF_ERR_NAME_TOO_LONG;
				dlnkname += len + 1;        /* @@@ not using NextPackedString() to avoid a 2nd strlen() call */
			}
		}
	}

	return 0;
}

/* Verify that DINI chunk is valid. */
static int32 dsppVerifyDataInitializer( const DSPPTemplate *dtmp )
{
	const DSPPDataInitializer *dini;
	const DSPPResource *drsc;
	const int32 *DataPtr;
	int32 Result;
	int32 Many;
	uint32 PtrLimit, PtrValue;   /* For doing arithmetic comparisons on pointers. */
	Result = 0;

	dini = dtmp->dtmp_DataInitializer;
	if( dini == NULL ) return 0;

	PtrValue = (uint32) dini;
	PtrLimit = PtrValue + dtmp->dtmp_DataInitializerSize;

	    /* !!! this doesn't trap a short DSPPDataInitializer */
	while(PtrValue < PtrLimit)
	{
	    /* !!! validate that dini_RsrcIndex is in range! */
		drsc = &(dtmp->dtmp_Resources[dini->dini_RsrcIndex]);

		    /* only allow initializing an I_MEM location that is actually owned by this instrument (i.e.
		       prevent init'ing and imported resource). OK to initialize bound resources (!!! make sure this is true) */
		if( (drsc->drsc_Flags & DRSC_F_IMPORT) ||
		    !((drsc->drsc_Type == DRSC_TYPE_VARIABLE) ||
		      (drsc->drsc_Type == DRSC_TYPE_OUTPUT) ||
		      (drsc->drsc_Type == DRSC_TYPE_KNOB)) )
		{
			ERRDBUG(("dsppVerifyDataInitializer: type not VARIABLE or OUTPUT\n"));
			return AF_ERR_BADRSRCTYPE;
		}

		Many = dini->dini_Many;
		DataPtr = (const int32 *) (((const char *)dini) + sizeof(DSPPDataInitializer));
		DataPtr += Many;

		dini = (const DSPPDataInitializer *) DataPtr;  /* Next struct just past data. */
		PtrValue = (uint32) dini;
	}
	if(PtrValue > PtrLimit)
	{
		ERRDBUG(("dsppVerifyDataInitializer: dini_Many was too big.\n"));
		return AF_ERR_BADOFX;
	}

	return Result;
}

/*
	Take validated template created in user-mode and make blessed clone
	for Folio use. Supervisor mode.
*/
static DSPPTemplate *dsppCloneTemplate (const DSPPTemplate *UserTmp)
{
	DSPPTemplate *SuperTmp;

/* Allocate/Init Supervisor mode structure */
	SuperTmp = dsppCreateSuperTemplate();
	if (SuperTmp == NULL)
	{
		return NULL;
	}

/* 950124: Copy non-pointers from User template to Super (@@@ must remain in sync w/ DSPPTemplate definition) */
	SuperTmp->dtmp_NumResources         = UserTmp->dtmp_NumResources;
	SuperTmp->dtmp_NumRelocations       = UserTmp->dtmp_NumRelocations;
	SuperTmp->dtmp_CodeSize             = UserTmp->dtmp_CodeSize;
	SuperTmp->dtmp_DataInitializerSize  = UserTmp->dtmp_DataInitializerSize;
	SuperTmp->dtmp_DynamicLinkNamesSize = UserTmp->dtmp_DynamicLinkNamesSize;
	SuperTmp->dtmp_Header               = UserTmp->dtmp_Header;

TRACEB(TRACE_INT,TRACE_OFX,("dsppCloneTemplate: SuperTmp = $%x\n", SuperTmp ));

	#define CLONEMEMBER(memb) \
		if (UserTmp->memb) { \
			const int32 srcSize = GetMemTrackSize (UserTmp->memb); \
			\
			DBUGCLONE(("dsppCloneTemplate: %s @ 0x%x, %d bytes\n", #memb, UserTmp->memb, srcSize)); \
			if (!(SuperTmp->memb = SuperAllocMem (srcSize, MEMTYPE_TRACKSIZE))) goto nomem; \
			memcpy (SuperTmp->memb, UserTmp->memb, srcSize); \
		}

	CLONEMEMBER(dtmp_Resources);
	CLONEMEMBER(dtmp_ResourceNames);
	CLONEMEMBER(dtmp_Relocations);
	CLONEMEMBER(dtmp_Codes);
	CLONEMEMBER(dtmp_DataInitializer);
	CLONEMEMBER(dtmp_DynamicLinkNames);

	#undef CLONEMEMBER

  #if DEBUG_Clone   /* debug code to verify successful clone */
	{
		#define CMPMEMBER(memb) { \
			const int32 usersize = UserTmp->memb ? GetMemTrackSize(UserTmp->memb) : 0; \
			const int32 supersize = SuperTmp->memb ? GetMemTrackSize(SuperTmp->memb) : 0; \
			\
			printf (#memb ": user: $%08lx %ld super: $%08lx %ld", UserTmp->memb, usersize, SuperTmp->memb, supersize); \
			\
			if (usersize != supersize || \
				!UserTmp->memb != !SuperTmp->memb || \
				UserTmp->memb && memcmp (UserTmp->memb, SuperTmp->memb, usersize)) PRT((" *** differ")); \
			PRT(("\n")); \
		}

		PRT(("\n"));
		CMPMEMBER(dtmp_Resources);
		CMPMEMBER(dtmp_ResourceNames);
		CMPMEMBER(dtmp_Relocations);
		CMPMEMBER(dtmp_Codes);
		CMPMEMBER(dtmp_DataInitializer);
		CMPMEMBER(dtmp_DynamicLinkNames);
	}
  #endif

	return SuperTmp;

nomem:
    dsppDeleteSuperTemplate (SuperTmp);
	return NULL;
}


/* -------------------- Resource locator */

/*****************************************************************/
/**
|||	AUTODOC -private -class audio -group DSPPTemplate -name dsppFindResourceIndex
|||	Finds resource index of named resource of DSPPTemplate.
|||
|||	  Synopsis
|||
|||	    int32 dsppFindResourceIndex (const DSPPTemplate *dtmp, const char *rsrcName)
|||
|||	  Description
|||
|||	    Finds resource index of named resource of DSPPTemplate.
|||
|||	  Arguments
|||
|||	    dtmp
|||	        Pointer to DSPPTemplate to scan.
|||
|||	    drscName
|||	        Resource name.
|||
|||	  Return Value
|||
|||	    Non-negative resource index if found, Err code if not found.
|||
|||	  Implementation
|||
|||	    Folio call implemented in audio folio V30.
|||
|||	  Associated Files
|||
|||	    <audio/dspp_template.h>, System.m2/Modules/audio
|||
|||	  See Also
|||
|||	    dsppGetTemplateRsrcName(), dsppFindEnvHookResources()
**/
int32 dsppFindResourceIndex (const DSPPTemplate *dtmp, const char *Name)
{
	int32 i;
	DSPPResource *drsc;
	int32 Result = AF_ERR_NAME_NOT_FOUND;
	const char *TmpRsrcName;

	drsc = dtmp->dtmp_Resources;
TRACEE(TRACE_INT,TRACE_OFX, ("dsppFindResourceIndex ( dtmp=0x%x, %s)\n", dtmp, Name));
TRACEB(TRACE_INT,TRACE_OFX, ("dsppFindResourceIndex: NumRsrc=$%x\n", dtmp->dtmp_NumResources));

	TmpRsrcName = dtmp->dtmp_ResourceNames;

	for (i=0; i<dtmp->dtmp_NumResources; i++)
	{
TRACEB(TRACE_INT,TRACE_OFX, ("dsppFindResourceIndex: i=%d, Type=$%x, Alloc=$%x\n",
			i, drsc->drsc_Type, drsc->drsc_Allocated));
TRACEB(TRACE_INT,TRACE_OFX, ("dsppFindResourceIndex: == %s ?\n",
	TmpRsrcName ));
			/* !!! why match Name == NULL? */
		if ((Name == NULL) || (!strcasecmp(Name, TmpRsrcName))) /* 931215 */
		{
			Result = i;
			break;  /* found it */
		}
		TmpRsrcName = NextPackedString( TmpRsrcName );
		drsc++;
	}

TRACER(TRACE_INT,TRACE_OFX, ("dsppFindResourceIndex returns 0x%x\n", Result));
	return Result;
}

/**
|||	AUTODOC -private -class audio -group DSPPTemplate -name dsppGetTemplateRsrcName
|||	Scans packed DSPPTemplate resource names to find Nth name.
|||
|||	  Synopsis
|||
|||	    const char *dsppGetTemplateRsrcName (const DSPPTemplate *dtmp, int32 indx)
|||
|||	  Description
|||
|||	    Scans packed DSPPTemplate resource names to find Nth name.
|||
|||	  Arguments
|||
|||	    dtmp
|||	        Pointer to DSPPTemplate to scan.
|||
|||	    indx
|||	        Resource index (0..dtmp_NumResources-1)
|||
|||	  Return Value
|||
|||	    Pointer to resource name, or NULL if not found.
|||
|||	  Implementation
|||
|||	    Folio call implemented in audio folio V30.
|||
|||	  Associated Files
|||
|||	    <audio/dspp_template.h>, System.m2/Modules/audio
|||
|||	  See Also
|||
|||	    dsppFindResourceIndex()
**/
const char *dsppGetTemplateRsrcName (const DSPPTemplate *dtmp, int32 indx)
{
  #ifdef PARANOID
	if( dtmp == NULL )
	{
		ERRDBUG(("dsppGetTemplateRsrcName: NULL template\n"));
		return NULL;
	}
  #endif

	{
		const char *p = dtmp->dtmp_ResourceNames;
		int32 i;

		for (i=0; i<indx; i++)
		{
			p = NextPackedString(p);
		}

		return p;
	}
}


/* -------------------- Instrument allocation */
/* !!! move to a separate module */

/*****************************************************************/
/* Clear all data memory including registers. */
static void dsppPresetInsMemory( DSPPInstrument *dins, uint32 flags )
{
	int32 ri,dmi;
	DSPPResource *drsc = dins->dins_Resources;

	for (ri=0; ri<dins->dins_NumResources; ri++)
	{

		if ( drsc->drsc_Flags & flags )
		{
			for( dmi=0; dmi<drsc->drsc_Many; dmi++ )
			{
				dsphWriteDataMem( drsc->drsc_Allocated + dmi, drsc->drsc_Default );
DBUG3(("dsppPresetInsMemory: set dspi[0x%x] to 0x%x\n", drsc->drsc_Allocated, drsc->drsc_Default ));
			}
		}
		drsc++;
	}
}

/*****************************************************************/
/* Initialize memory for instrument for matching AT bits. */
int32 DSPPInitInsMemory( DSPPInstrument *dins, int32 AT_Mask )
{
	DSPPTemplate *dtmp;
	DSPPDataInitializer *dini;
	DSPPResource *drsc;
	int32 *DataPtr;
	int32 Result, i;
	int32 Many;
	int32 StartAddr;
	uint32 PtrLimit, PtrValue;   /* For doing arithmetic comparisons on pointers. */
	Result = 0;

/* Set using drsc_Default before processing DINI commands. */
	dsppPresetInsMemory( dins, AT_Mask );

	dtmp = dins->dins_Template;
	dini = dtmp->dtmp_DataInitializer;
	if( dini == NULL ) return 0;

	PtrValue = (uint32) dini;
	PtrLimit = PtrValue + dtmp->dtmp_DataInitializerSize;

	while(PtrValue < PtrLimit)
	{
		Many = dini->dini_Many;
		DataPtr = (int32 *) (((char *)dini) + sizeof(DSPPDataInitializer));
		if( dini->dini_Flags & AT_Mask )
		{
			drsc = &(dins->dins_Resources[dini->dini_RsrcIndex]);
			StartAddr = drsc->drsc_Allocated;
			for(i=0; i<Many; i++)
			{
TRACEB(TRACE_INT,TRACE_OFX, ("DSPPInitInsMemory: 0x%x <= 0x%x\n", StartAddr+i, *DataPtr));
				dsphWriteDataMem( StartAddr+i, *DataPtr++ );
			}
			dini = (DSPPDataInitializer *) DataPtr;  /* Next struct just past data. */
		}
		else
		{
			dini = (DSPPDataInitializer *) (DataPtr + Many);
		}
		PtrValue = (uint32) dini;
	}
	return Result;
}

/*****************************************************************/
/* @@@ Caveats:
        . only supports single code-hunk
        . that code hunk must be of type DCOD_RUN_DSPP
*/
/* !!! this function's name is unfortunate. It really should be something like dsppCreateInstrument() */
static int32 DSPPLoadPatch( DSPPTemplate *dtmp, DSPPInstrument **DInsPtr, int32 RateShift)
{
	DSPPInstrument *dins;
	DSPPCodeHeader *Codes = NULL;
	DSPPResource *drsc;
	FIFOControl *fico;
	int32 CodeSize, NumFIFOs;
	int32 i, Result;
	uint16 *Image;

DBUG(("DSPPLoadPatch: >>>>>>>>>>>>>>>>>>>>>>>>>\n"));
DBUG(("DSPPLoadPatch: dtmp = $%x\n", dtmp));

	*DInsPtr = NULL;

/* Allocate Instrument structure. */
	dins = (DSPPInstrument *) EZMemAlloc(sizeof(DSPPInstrument)+8,MEMTYPE_FILL);
	if (dins == NULL)
	{
		ERRDBUG(("DSPPLoadPatch: could not allocate memory.\n"));
		Result = AF_ERR_NOMEM;
		goto error;
	}
	dins->dins_Template = dtmp;
	dins->dins_RateShift = RateShift; /* 940812 */

DBUG(("DSPPLoadPatch: dins = $%x\n", dins));

/* Open all resources for this instrument */
	LOG("    dsppOpenInstrumentResources");
	if ((Result = dsppOpenInstrumentResources (dins)) < 0)
	{
		ERRDBUG(("DSPPLoadPatch: couldn't open resources, errcode=$%08lx\n", Result));
		goto error;
	}

		/* Export resources for privileged templates */
	if (dtmp->dtmp_Header.dhdr_Flags & DHDR_F_PRIVILEGED) {
		LOG("    dsppExportInstrumentResources");
		if ((Result = dsppExportInstrumentResources (dins)) < 0) goto error;
	}

/* Count number of FIFOs so we know how many controllers to allocate. */
/* !!! separate FIFO count and hardware init - move hardware init into 2nd pass thru FIFOs */
	LOG("    init FIFOs");
	NumFIFOs = 0;
	for (i=0; i<dtmp->dtmp_NumResources; i++)
	{
		drsc = &dins->dins_Resources[i];
/* Set direction of DMA based on resource type allocated. */
/* !!! What if the FIFO is imported, or exported.  Is there a security hole? */
		if (drsc->drsc_Type == DRSC_TYPE_IN_FIFO)
		{
			bool ifDecomp = FALSE;
			bool if8Bit = FALSE;

			NumFIFOs++;

/* Set hardware data decompression based on FIFO SubType 950825 */
			switch( drsc->drsc_SubType )
			{
			case DRSC_INFIFO_SUBTYPE_SQS2:
				ifDecomp = if8Bit = TRUE;
				break;
			case DRSC_INFIFO_SUBTYPE_8BIT:
				if8Bit = TRUE;
				break;
			default:
				break;
			}
			dsphSetChannelDecompression( drsc->drsc_Allocated, ifDecomp );
			dsphSetChannel8Bit( drsc->drsc_Allocated, if8Bit );

/* Clear FIFO RAM, also sets direction. CR6036 */
			dsphClearInputFIFO( drsc->drsc_Allocated );
		}
		else if (drsc->drsc_Type == DRSC_TYPE_OUT_FIFO)
		{
			NumFIFOs++;
			dsphDisableDMA( drsc->drsc_Allocated );
			dsphSetChannelDecompression( drsc->drsc_Allocated, FALSE );  /* Clear to known state. 960326 */
			dsphSetChannel8Bit( drsc->drsc_Allocated, FALSE );
			dsphSetChannelDirection( drsc->drsc_Allocated, DSPX_DIR_DSPP_TO_RAM );
		}
	}

/* Allocate FIFO Controllers */
	if (NumFIFOs)
	{
		dins->dins_FIFOControls = (FIFOControl *) EZMemAlloc( NumFIFOs * sizeof(FIFOControl), MEMTYPE_FILL);
		if (dins->dins_FIFOControls == NULL)
		{
			Result = AF_ERR_NOMEM;
			goto error;
		}
		dins->dins_NumFIFOs = NumFIFOs;
		for (i=0; i<NumFIFOs; i++)
		{
			InitList(&dins->dins_FIFOControls[i].fico_Attachments,"Attachments");
		}

/* Scan resources again to fill in FIFO control indices */
		fico = &dins->dins_FIFOControls[0];
		for (i=0; i<dtmp->dtmp_NumResources; i++)
		{
			drsc = &dins->dins_Resources[i];
			if ((drsc->drsc_Type == DRSC_TYPE_IN_FIFO) ||
		    	(drsc->drsc_Type == DRSC_TYPE_OUT_FIFO))
			{
TRACEB(TRACE_INT,TRACE_OFX,("DSPPLoadPatch: FIFO Index = %d\n", i));
				(fico++)->fico_RsrcIndex = i;
			}

		}
	}

/* Allocate Code image to perform relocations on. */
	LOG("    prepare code");
	CodeSize = dtmp->dtmp_CodeSize;
	Codes = (DSPPCodeHeader *) EZMemAlloc(CodeSize+8,0);
	if (Codes == NULL)
	{
		Result = AF_ERR_NOMEM;
		goto error;
	}
	bcopy(dtmp->dtmp_Codes, Codes, (int) CodeSize);

/* Perform relocations based on RLOC chunk. */
	Result = dsppRelocateInstrument (dins, Codes);
	if (Result != 0) goto error;

		/* @@@ This assumes only 1 code hunk */
	Image = (uint16 *) dsppGetCodeHunkImage(Codes,0);

  #if DEBUG_DumpDownloadedCode
	dspnDisassemble (Image, dins->dins_EntryPoint, dins->dins_DSPPCodeSize,
					 "DSPPInstrument $%08lx temp=$%08lx funcid=%ld", dins, dins->dins_Template, dins->dins_Template->dtmp_Header.dhdr_FunctionID);
  #endif

/* Download Code to DSPP  */
TRACEB(TRACE_INT,TRACE_OFX,("DSPPLoadPatch: Code Entry at $%x\n", dins->dins_EntryPoint));
	dsphDownloadCode(Image, dins->dins_EntryPoint,
		dins->dins_DSPPCodeSize);

	EZMemFree(Codes);
	Codes = NULL;

/* Initialize Instrument I Memory */
	LOG("    DSPPInitInsMemory");
	Result = DSPPInitInsMemory( dins, DINI_F_AT_ALLOC );
	if (Result != 0) goto error;

/* 950126 Moved overwrite of SLEEP to DSPPStartCodeExecution() in dspp_instr.c */

TRACEB(TRACE_INT,TRACE_OFX,("DSPPLoadPatch: dins = $%x\n", dins));
TRACEB(TRACE_INT,TRACE_OFX,("DSPPLoadPatch: <<<<<<<<<<<<<<<<<<<\n"));

/* We made it, set return parameter. */
	*DInsPtr = dins;
	return(Result);

error:
DBUG(("DSPPLoadPatch: error = 0x%x\n", Result));
	EZMemFree(Codes);
	if (dins)   /* Don't reference dins unless non-NULL 941102 */
	{
		if (dtmp->dtmp_Header.dhdr_Flags & DHDR_F_PRIVILEGED) {
			dsppUnexportInstrumentResources (dins);
		}
		dsppCloseInstrumentResources (dins);        /* Deallocate resources. 930704 */
		EZMemFree (dins->dins_FIFOControls);
		EZMemFree (dins);
	}
	return(Result);
}

/*****************************************************************/
int32 DSPPAllocInstrument( DSPPTemplate *dtmp, DSPPInstrument **DInsPtr, int32 RateShift )
{
	DSPPInstrument *dins;
	int32 Result;

	*DInsPtr = NULL;
	Result = DSPPLoadPatch( dtmp, &dins, RateShift );
	if (Result) return Result;

/* Attach Standard Knobs, OK if this fails. */
	dsppCreateKnobProbe(&dins->dins_AmplitudeKnob, dins, "Amplitude", DRSC_TYPE_KNOB);

	InitList( &dins->dins_EnvelopeAttachments, "EnvAtt" );

	*DInsPtr = dins;
	return(Result);
}


/*****************************************************************/
void dsppFreeInstrument( DSPPInstrument *dins )
{
	DSPPTemplate *dtmp;
	int32 i;
	List *AttList;

/* check for NULL dins */
	if (!dins) return;
	dtmp = dins->dins_Template;

TRACEE(TRACE_INT,TRACE_OFX,("dsppFreeInstrument: dins = $%x, dtmp = $%x\n", dins, dins->dins_Template));

/* Stop it in case it is still playing. */
	LOG("  DSPPStopInstrument");
	DSPPStopInstrument ( dins, NULL);

#ifdef PARANOID
/* Just to be paranoid, paint with SLEEP code. %Q */
	{
		int32 da = dins->dins_EntryPoint;
		for (i=0; i<dins->dins_DSPPCodeSize; i++ )
		{
			dsphWriteCodeMem( da++, DSPN_OPCODE_SLEEP );
		}
	}
#endif

/* Set any FIFOs to input mode for security reasons. */
	LOG("  disable fifos");
	{
		DSPPResource *drsc;
		for (i=0; i<dtmp->dtmp_NumResources; i++)
		{
			drsc = &dins->dins_Resources[i];
			if ( (drsc->drsc_Type == DRSC_TYPE_IN_FIFO) ||
			     (drsc->drsc_Type == DRSC_TYPE_OUT_FIFO) )
			{
				dsphSetChannelDirection( drsc->drsc_Allocated, DSPX_DIR_RAM_TO_DSPP );
			}
		}
	}

/* Scan list of Sample Attachments and delete them. */
	LOG("  delete attachments");
	for (i=0; i<dins->dins_NumFIFOs; i++)
	{
		AttList = &dins->dins_FIFOControls[i].fico_Attachments;
		afi_DeleteLinkedItems(AttList);  /* 930617 */
	}
	afi_DeleteLinkedItems( &dins->dins_EnvelopeAttachments );

/* Unexport resources for privileged templates */
/* Shouldn't actually be permitted to fail after clobbering parts of itself above
** and before this function is called!
*/
	if (dtmp->dtmp_Header.dhdr_Flags & DHDR_F_PRIVILEGED)
	{
DBUG(("dsppFreeInstrument: PRIVILEGED so call dsppUnexportInstrumentResources.\n"));
		dsppUnexportInstrumentResources (dins);
	}

/* Close resources */
	LOG("  close resources");
	dsppCloseInstrumentResources (dins);

/* Free structures. */
	EZMemFree (dins->dins_FIFOControls);
	EZMemFree (dins);
TRACER(TRACE_INT,TRACE_OFX,("dsppFreeInstrument done.\n"));
}

/*****************************************************************/
DSPPResource *DSPPFindResource (const DSPPInstrument *dins, int32 RsrcType, const char *Name)
{
	DSPPTemplate *dtmp;
	int32 RsrcIndex;
	DSPPResource *drsc;

	dtmp = dins->dins_Template;
	RsrcIndex = dsppFindResourceIndex (dtmp, Name );
	if (RsrcIndex < 0)
	{
		return NULL;
	}
	drsc = &dins->dins_Resources[RsrcIndex];
	if (drsc->drsc_Type != RsrcType)
	{
		return NULL;
	}

	return drsc;
}
