/* @(#) dspp_mixer.c 96/06/18 1.21 */
/****************************************************************
**
** Make a mixer template from scratch based on a MixerSpec.
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
*****************************************************************
** 950714 PLB Created.
** 950804 WJB Added debug call to dsppDumpTemplate().
** 950814 PLB Added Amplitude and LineOut options.
** 960130 PLB Fixed problem with numInputs==1  CR 5551
** 960618 PLB Imported gMixer must be variable.  CR6215
****************************************************************/

#include <dspptouch/dspp_instructions.h>    /* DSPP opcodes */

#include "audio_folio_modes.h"
#include "audio_internal.h"


/* -------------------- Debug */

#define DBUG(x)                 /* PRT(x) */
#define DEBUG_DumpMixerTemplate 0           /* dsppDumpTemplate() completed Mixer Template */


/* -------------------- Internal Structures */

typedef struct MixerMakerPartReferences
{
	int16     mmpr_FirstRef;
	int16     mmpr_LastRef;
} MixerMakerPartReferences;

typedef struct MixerMakerResource
{
	int32      mmres_NumParts;
	MixerMakerPartReferences     *mmres_PartRefs;
	               /* Array of first and last refs for each part.
                   ** References are linked in code memory.
                   ** Set to -1 if unreferenced or >= 0 */
} MixerMakerResource;

typedef struct MixerMaker
{
	uint8      mxmk_NumInputs;
	uint8      mxmk_NumOutputs;
	uint8      mxmk_MixerFlags;
	uint8      mxmk_Reserved;
	int32      mxmk_CodeChunkSize;
	int32      mxmk_CodeMany;
	uint16    *mxmk_CodeChunk;
	uint16    *mxmk_Code;
/* Declare the handfull of address resources in this template. */
	MixerMakerResource  mxmk_RsrcAmplitude;
	MixerMakerResource  mxmk_RsrcGain;
	MixerMakerResource  mxmk_RsrcInput;
	MixerMakerResource  mxmk_RsrcOutput;
} MixerMaker;

/* Build resources in this order! ResourceOrder */
typedef struct MixerResources
{
	DSPPResource  mrsc_RsrcTicks;
	DSPPResource  mrsc_RsrcEntry;
	DSPPResource  mrsc_RsrcGain;
	DSPPResource  mrsc_RsrcInput;
	DSPPResource  mrsc_RsrcOutput;     /* May be Output resource or imported gMixer resource */
	DSPPResource  mrsc_RsrcAmplitude;  /* This may not be used so put at end. */
} MixerResources;

/* Names for resource. ResourceOrder
** Yes, it seems silly to have duplicate strings but the code needed to build these strings
** based on the flag values seemed bigger than simply having multiple copies of the data.
*/
/* !!! might consider doing away with the Ticks and Entry resource names */
static const char MixerRsrcNames_la[] = "Ticks\0Entry\0Gain\0Input\0Output";
static const char MixerRsrcNames_lA[] = "Ticks\0Entry\0Gain\0Input\0Output\0Amplitude";
static const char MixerRsrcNames_La[] = "Ticks\0Entry\0Gain\0Input\0gMixer";
static const char MixerRsrcNames_LA[] = "Ticks\0Entry\0Gain\0Input\0gMixer\0Amplitude";

static const DSPPHeader StandardMixerHeader = { DFID_MIXER, DSPP_SILICON_BULLDOG,
		DHDR_SUPPORTED_FORMAT_VERSION, 0 };

#define OPERAND_DIRECT            (DSPN_OPERAND_ADDR)
#define OPERAND_WRITEBACK         (DSPN_OPERAND_ADDR | DSPN_ADDR_F_WRITE_BACK)
#define OPCODE_TIMES              (0x5C80)  /* @@@ these could be built out of DSPN_ARITH_ defines */
#define OPCODE_TIMES_PLUS_ACCUME  (0x5C27)
#define OPCODE_TIMES_ACCUME_PLUS  (0x4D27)
#define OPCODE_PLUS_ACCUME        (0x2127)
#define OPCODE_EQUALS_TIMES       (0x7C80)
#define OPCODE_EQUALS_TIMES_ACCUME (0x4C80)
#define OPCODE_EQUALS_TIMES_PLUS_A  (0x7C27)

#define mxSetCode(ci,n) { const int32 cj=(ci); mxmk->mxmk_Code[cj] = (n); DBUG(("Code[0x%x]=0x%x\n",cj,(n))); }
#define mxSetCodeLink(ci,n) { const int32 cj=(ci); mxmk->mxmk_Code[cj] = (mxmk->mxmk_Code[cj] & ~DSPN_ADDR_MASK) | (n); }

/***********************************************************************
** Compile and link a reference to an address operand.
*/
static void mxReferenceResource( MixerMaker *mxmk, MixerMakerResource *mmres, int32 partNum, int32 pc, uint32 operand )
{
	int32   firstRef;

/* Get first reference to this part of resource. */
	firstRef = mmres->mmres_PartRefs[partNum].mmpr_FirstRef;

/* Is this the first time referenced? */
	if( firstRef < 0 )
	{
DBUG(("mxReferenceResource: first reference.\n"));
		mmres->mmres_PartRefs[partNum].mmpr_FirstRef = pc;  /* Keep PC of first reference. */
	}
	else
	{
		int32   lastRef = mmres->mmres_PartRefs[partNum].mmpr_LastRef;
DBUG(("mxReferenceResource: earlier reference at 0x%x.\n", lastRef ));
/* Link last reference to current reference to make it tail of list. */
/* Use PC relative addressing in list. */
		mxSetCodeLink(lastRef, pc - lastRef );
	}
	mmres->mmres_PartRefs[partNum].mmpr_LastRef = pc;

/* List is terminated with a zero offset in the code. */
	mxSetCode(pc, operand );
}
#undef MakeLinkedOperand

/**********************************************************************/
static Err mxAllocMixerParts( MixerMakerResource *mmres, int32 numParts )
{
	int32 i;
	mmres->mmres_NumParts = numParts;
	mmres->mmres_PartRefs = (MixerMakerPartReferences *) SuperAllocMem (
	           sizeof(MixerMakerPartReferences) * numParts, MEMTYPE_TRACKSIZE | MEMTYPE_FILL);
	if( mmres->mmres_PartRefs == NULL ) return AF_ERR_NOMEM;
/* Mark as having no references yet. */
	for(i=0; i<numParts; i++)
	{
		mmres->mmres_PartRefs[i].mmpr_FirstRef = -1;
	}
	return 0;
}

/**********************************************************************/
static Err mxBuildCode( MixerMaker *mxmk )
{
	int32 inputIndex, outputIndex, gainIndex;
	int32 codeSize, pc, numMACs;

/* Init Resources to starting state. */
	if( mxmk->mxmk_MixerFlags & AF_F_MIXER_WITH_AMPLITUDE )
	{
		if( mxAllocMixerParts( &mxmk->mxmk_RsrcAmplitude, 1 ) < 0 ) goto cleanup;
	}
	if( mxAllocMixerParts( &mxmk->mxmk_RsrcGain,
	           mxmk->mxmk_NumOutputs * mxmk->mxmk_NumInputs ) < 0 ) goto cleanup;
	if( mxAllocMixerParts( &mxmk->mxmk_RsrcInput, mxmk->mxmk_NumInputs ) < 0 ) goto cleanup;
	if( mxAllocMixerParts( &mxmk->mxmk_RsrcOutput, mxmk->mxmk_NumOutputs ) < 0 ) goto cleanup;

/* Calculate size of Code chunk needed. */
/* 2 bytes per word, 3 words per MAC plus 2 for SLEEP and _NOP at end?  */
	mxmk->mxmk_CodeMany = (3 * mxmk->mxmk_NumOutputs * (mxmk->mxmk_NumInputs)) + 2;
	{
/*  3 words for final Scale by Amplitude or to add to gMixer, otherwise just 1 extra word. */
		int32 numPerOutput;
		if( mxmk->mxmk_MixerFlags & (AF_F_MIXER_WITH_AMPLITUDE | AF_F_MIXER_WITH_LINE_OUT))
		{
			numPerOutput = 3;
			numMACs = mxmk->mxmk_NumInputs;
		}
		else
		{
			numPerOutput = 1;
			numMACs = mxmk->mxmk_NumInputs - 1;
		}
		mxmk->mxmk_CodeMany += numPerOutput * mxmk->mxmk_NumOutputs;
	}
	codeSize = 2 * mxmk->mxmk_CodeMany;
	mxmk->mxmk_CodeChunkSize = sizeof(DSPPCodeHeader) + codeSize;

/* Allocate Code Chunk */
	mxmk->mxmk_CodeChunk = (uint16 *)SuperAllocMem (mxmk->mxmk_CodeChunkSize, MEMTYPE_TRACKSIZE | MEMTYPE_FILL);
	if (mxmk->mxmk_CodeChunk == NULL) goto cleanup;
	DBUG(("mxBuildCode: allocated %d bytes of code at 0x%x\n", codeSize, mxmk->mxmk_CodeChunk));

/* Point to actual DSPP code in chunk. */
	mxmk->mxmk_Code = (uint16 *) (((char *)mxmk->mxmk_CodeChunk) + sizeof(DSPPCodeHeader));

/* Fill in code and update resource relocations. */
	pc = 0;
	gainIndex = 0;
	for( outputIndex = 0; outputIndex < mxmk->mxmk_NumOutputs; outputIndex++ )
	{
/* Lay down MAC array. */
		for( inputIndex = 0; inputIndex < numMACs; inputIndex++ )
		{
			if( inputIndex == 0 )
			{
				mxSetCode(pc++,OPCODE_TIMES);
			}
			else
			{
				mxSetCode(pc++, OPCODE_TIMES_PLUS_ACCUME);
			}
			mxReferenceResource( mxmk, &mxmk->mxmk_RsrcInput, inputIndex, pc++, OPERAND_DIRECT );
			mxReferenceResource( mxmk, &mxmk->mxmk_RsrcGain, gainIndex++, pc++, OPERAND_DIRECT );
		}

/* Implement output options. */
		if( mxmk->mxmk_MixerFlags & AF_F_MIXER_WITH_AMPLITUDE )
		{
/* Scale result by amplitude and add to global mixer. */
			if( mxmk->mxmk_MixerFlags & AF_F_MIXER_WITH_LINE_OUT )
			{
				mxSetCode(pc++, OPCODE_TIMES_ACCUME_PLUS);
				mxReferenceResource( mxmk, &mxmk->mxmk_RsrcAmplitude, 0, pc++, OPERAND_DIRECT );
				mxReferenceResource( mxmk, &mxmk->mxmk_RsrcOutput, outputIndex, pc++, OPERAND_WRITEBACK );
			}
			else
			{
/* Scale result by amplitude and write to output. */
				mxSetCode(pc++, OPCODE_EQUALS_TIMES_ACCUME);
				mxReferenceResource( mxmk, &mxmk->mxmk_RsrcAmplitude, 0, pc++, OPERAND_DIRECT );
				mxReferenceResource( mxmk, &mxmk->mxmk_RsrcOutput, outputIndex, pc++, OPERAND_DIRECT );
			}
		}
		else
		{

/* Add result to global mixer. */
			if( mxmk->mxmk_MixerFlags & AF_F_MIXER_WITH_LINE_OUT )
			{
				mxSetCode(pc++, DSPN_OPCODE_NOP);  /* Allow time for Accumulator to settle. */
				mxSetCode(pc++, OPCODE_PLUS_ACCUME);
				mxReferenceResource( mxmk, &mxmk->mxmk_RsrcOutput, outputIndex, pc++, OPERAND_WRITEBACK );
			}
			else
			{
/* Add result to output. */
				if( numMACs == 0 )
				{
/* Nothing in ACCUME so just use variables. */
					mxSetCode(pc++, OPCODE_EQUALS_TIMES);  /* 960130 CR 5551 */
					mxReferenceResource( mxmk, &mxmk->mxmk_RsrcInput, inputIndex, pc++, OPERAND_DIRECT );
					mxReferenceResource( mxmk, &mxmk->mxmk_RsrcGain, gainIndex++, pc++, OPERAND_DIRECT );
					mxReferenceResource( mxmk, &mxmk->mxmk_RsrcOutput, outputIndex, pc++, OPERAND_DIRECT );
				}
				else
				{
					mxSetCode(pc++, OPCODE_EQUALS_TIMES_PLUS_A);
					mxReferenceResource( mxmk, &mxmk->mxmk_RsrcInput, inputIndex, pc++, OPERAND_DIRECT );
					mxReferenceResource( mxmk, &mxmk->mxmk_RsrcGain, gainIndex++, pc++, OPERAND_DIRECT );
					mxReferenceResource( mxmk, &mxmk->mxmk_RsrcOutput, outputIndex, pc++, OPERAND_DIRECT );
				}
			}
		}
	}
	return 0;

cleanup:
	return AF_ERR_NOMEM;
}

/**********************************************************************/
static void mxDeleteMixerMaker( MixerMaker *mxmk )
{
	if (mxmk) {
		SuperFreeMem (mxmk->mxmk_CodeChunk,                    TRACKED_SIZE);
		SuperFreeMem (mxmk->mxmk_RsrcAmplitude.mmres_PartRefs, TRACKED_SIZE);
		SuperFreeMem (mxmk->mxmk_RsrcGain.mmres_PartRefs,      TRACKED_SIZE);
		SuperFreeMem (mxmk->mxmk_RsrcInput.mmres_PartRefs,     TRACKED_SIZE);
		SuperFreeMem (mxmk->mxmk_RsrcOutput.mmres_PartRefs,    TRACKED_SIZE);
		SuperFreeMem (mxmk, sizeof(MixerMaker));
	}
}

/**********************************************************************/
static MixerMaker *mxCreateMixerMaker( int32 numIn, int32 numOut, int32 flags )
{
	MixerMaker *mxmk;
DBUG(("mxCreateMixerMaker: numIn=%d, numOut=%d, flags=0x%x\n", numIn, numOut, flags ));
	mxmk = (MixerMaker *)SuperAllocMem(sizeof(MixerMaker), MEMTYPE_FILL);
	if (mxmk == NULL)
	{
		return mxmk;
	}
	mxmk->mxmk_NumInputs = numIn;
	mxmk->mxmk_NumOutputs = numOut;
	mxmk->mxmk_MixerFlags = flags;
DBUG(("mxCreateMixerMaker: mxmk = 0x%x\n", mxmk ));
	return mxmk;
}

/***********************************************************************
** Scan parts of resource and add relocations as needed.
*/
static DSPPRelocation *mxRelocateResource( MixerMakerResource *mmres,
                              DSPPRelocation *drlc, int32 rsrcIndex )
{
	int32 partIndex;
	for( partIndex=0; partIndex < mmres->mmres_NumParts; partIndex++ )
	{
		drlc->drlc_RsrcIndex  = rsrcIndex;
		drlc->drlc_Part       = partIndex;
		drlc->drlc_CodeHunk   = 0;
		drlc->drlc_CodeOffset = mmres->mmres_PartRefs[partIndex].mmpr_FirstRef;
		drlc++;
	}
	return drlc;
}

/***********************************************************************
** Fill in mixer template based on MixerMaker
*/
static Err mxBuildTemplate( MixerMaker *mxmk, DSPPTemplate *dtmp )
{
	DSPPResource *drsc;
	DSPPRelocation *drlc;
	int32 numRelocs, rsrcIndex;
	MixerMakerResource *mmres;
	int32 size;
	const char *names;

/* Allocate resources and fill them in. */
	dtmp->dtmp_NumResources = sizeof(MixerResources) / sizeof(DSPPResource);
	if( (mxmk->mxmk_MixerFlags & AF_F_MIXER_WITH_AMPLITUDE) == 0) dtmp->dtmp_NumResources -= 1;
	drsc = (DSPPResource *)SuperAllocMem (dtmp->dtmp_NumResources * sizeof(DSPPResource), MEMTYPE_TRACKSIZE | MEMTYPE_FILL);
	if (drsc == NULL) goto err_nomem;
	dtmp->dtmp_Resources = drsc;

/* Make copy of MixerRsrcNames cuz they will get deleted with template. */
	if( mxmk->mxmk_MixerFlags & AF_F_MIXER_WITH_AMPLITUDE )
	{
		if( mxmk->mxmk_MixerFlags & AF_F_MIXER_WITH_LINE_OUT )
		{
			size = sizeof( MixerRsrcNames_LA );
			names = MixerRsrcNames_LA;
		}
		else
		{
			size = sizeof( MixerRsrcNames_lA );
			names = MixerRsrcNames_lA;
		}
	}
	else
	{
		if( mxmk->mxmk_MixerFlags & AF_F_MIXER_WITH_LINE_OUT )
		{
			size = sizeof( MixerRsrcNames_La );
			names = MixerRsrcNames_La;
		}
		else
		{
			size = sizeof( MixerRsrcNames_la );
			names = MixerRsrcNames_la;
		}
	}
	dtmp->dtmp_ResourceNames = (char *)SuperAllocMem (size, MEMTYPE_TRACKSIZE | MEMTYPE_FILL);
	if( dtmp->dtmp_ResourceNames == NULL )  goto err_nomem;
	memcpy( dtmp->dtmp_ResourceNames, names, size );

/* Calculate how many relocations are needed by adding up NumParts of address resources. */
	numRelocs = mxmk->mxmk_RsrcGain.mmres_NumParts;
	numRelocs += mxmk->mxmk_RsrcInput.mmres_NumParts;
	numRelocs += mxmk->mxmk_RsrcOutput.mmres_NumParts;
	if( mxmk->mxmk_MixerFlags & AF_F_MIXER_WITH_AMPLITUDE )
	{
		numRelocs += mxmk->mxmk_RsrcAmplitude.mmres_NumParts;
	}
	dtmp->dtmp_NumRelocations = numRelocs;

	dtmp->dtmp_Header = StandardMixerHeader;

/* Allocate Relocations */
	drlc = (DSPPRelocation *)SuperAllocMem (sizeof(DSPPRelocation) * numRelocs, MEMTYPE_TRACKSIZE | MEMTYPE_FILL);
	if (drlc == NULL) goto err_nomem;
	dtmp->dtmp_Relocations = drlc;

/* Fill in other info. */
	{
		DSPPCodeHeader *dcod;
		dtmp->dtmp_CodeSize = mxmk->mxmk_CodeChunkSize;
		dcod = (DSPPCodeHeader *) mxmk->mxmk_CodeChunk;
		mxmk->mxmk_CodeChunk = NULL;
		dcod->dcod_Offset = sizeof(DSPPCodeHeader);
		dcod->dcod_Size = mxmk->mxmk_CodeMany;
DBUG(("dcod->dcod_Size = 0x%x\n", dcod->dcod_Size ));
		dtmp->dtmp_Codes = dcod;
DBUG(("dtmp->dtmp_Codes = 0x%x\n", dtmp->dtmp_Codes));
		dtmp->dtmp_DataInitializerSize = 0;
		dtmp->dtmp_DataInitializer = NULL;
	}

/* Point to first relocation. */
	drlc = dtmp->dtmp_Relocations;
	rsrcIndex = 0;

/* Now set specific values in resources. ResourceOrder */
/* Ticks */
	drsc = &(((MixerResources *) (dtmp->dtmp_Resources))->mrsc_RsrcTicks);
	drsc->drsc_Type = DRSC_TYPE_TICKS;
	drsc->drsc_Many = mxmk->mxmk_CodeMany; /* !!! Is this true? */
	rsrcIndex++;

/* Entry */
	drsc = &(((MixerResources *) (dtmp->dtmp_Resources))->mrsc_RsrcEntry);
	drsc->drsc_Type = DRSC_TYPE_CODE;
	drsc->drsc_Many = mxmk->mxmk_CodeMany;
	rsrcIndex++;

/* Gain */
	drsc = &(((MixerResources *) (dtmp->dtmp_Resources))->mrsc_RsrcGain);
	mmres = &mxmk->mxmk_RsrcGain;
	drlc = mxRelocateResource( mmres, drlc, rsrcIndex++ );
	drsc->drsc_Many = mmres->mmres_NumParts;
	drsc->drsc_SubType = AF_SIGNAL_TYPE_GENERIC_SIGNED;
	drsc->drsc_Flags = DRSC_F_INIT_AT_ALLOC;
	drsc->drsc_Type = DRSC_TYPE_KNOB;

/* Input */
	drsc = &(((MixerResources *) (dtmp->dtmp_Resources))->mrsc_RsrcInput);
	mmres = &mxmk->mxmk_RsrcInput;
	drlc = mxRelocateResource( mmres, drlc, rsrcIndex++ );
	drsc->drsc_Many = mmres->mmres_NumParts;
	drsc->drsc_Type = DRSC_TYPE_INPUT;

/* Output */
	drsc = &(((MixerResources *) (dtmp->dtmp_Resources))->mrsc_RsrcOutput);
	mmres = &mxmk->mxmk_RsrcOutput;
	drlc = mxRelocateResource( mmres, drlc, rsrcIndex++ );
	drsc->drsc_Many = mmres->mmres_NumParts;
	if( mxmk->mxmk_MixerFlags & AF_F_MIXER_WITH_LINE_OUT )
	{
		drsc->drsc_Type = DRSC_TYPE_VARIABLE;  /* 960618 Import must be variable. */
		drsc->drsc_Flags = DRSC_F_IMPORT;
	}
	else
	{
		drsc->drsc_Type = DRSC_TYPE_OUTPUT;
		drsc->drsc_Flags = DRSC_F_INIT_AT_ALLOC;
	}

/* Amplitude */
	if( mxmk->mxmk_MixerFlags & AF_F_MIXER_WITH_AMPLITUDE )
	{
		drsc = &(((MixerResources *) (dtmp->dtmp_Resources))->mrsc_RsrcAmplitude);
		mmres = &mxmk->mxmk_RsrcAmplitude;
		mxRelocateResource( mmres, drlc, rsrcIndex );   /* @@@ last one, so don't set drlc or increment rsrcIndex */
		drsc->drsc_Many = mmres->mmres_NumParts;
		drsc->drsc_SubType = AF_SIGNAL_TYPE_GENERIC_SIGNED;
		drsc->drsc_Type = DRSC_TYPE_KNOB;
		drsc->drsc_Default = DSPP_MAX_SIGNED;
		drsc->drsc_Flags = DRSC_F_INIT_AT_ALLOC;
	}

	return 0;

err_nomem:
	dsppDeleteSuperTemplate( dtmp );
	return AF_ERR_NOMEM;
}

/**********************************************************************/
Err dsppCreateMixerTemplate( DSPPTemplate **dtmpPtr, MixerSpec mixerSpec )
{
	int32 numIn, numOut, flags;
	int32 Result;
	MixerMaker *mxmk;
	DSPPTemplate *dtmp;

/* Extract info from spec. */
	numIn = MixerSpecToNumIn(mixerSpec);
	numOut = MixerSpecToNumOut(mixerSpec);
	flags = MixerSpecToFlags(mixerSpec);

/* Validate Spec */
	if( (numIn < AF_MIXER_MIN_INPUTS) || (numIn > AF_MIXER_MAX_INPUTS) ||
	    (numOut < AF_MIXER_MIN_OUTPUTS ) || (numOut > AF_MIXER_MAX_OUTPUTS ) ||
	    (flags & ~AF_MIXER_LEGAL_FLAGS ) ||
	    ((flags & AF_F_MIXER_WITH_LINE_OUT) && (numOut != 2)) ) return AF_ERR_BAD_MIXER;

/* Create a data structure for tracking construction of Mixer Build. */
	mxmk = mxCreateMixerMaker( numIn, numOut, flags );
	if( mxmk == NULL ) return AF_ERR_NOMEM;

/* Create an empty DSPPTemplate */
	dtmp = dsppCreateSuperTemplate();
	if( dtmp == NULL )
	{
		Result = AF_ERR_NOMEM;
		goto err_free_mxmk;
	}

/* Create Code and Resources */
	Result = mxBuildCode( mxmk );
	if( Result < 0 ) goto err_free_dtmp;

/* Scan MixerMaker and fill out DSPPTemplate. */
	Result = mxBuildTemplate( mxmk, dtmp );
	if( Result < 0 ) goto err_free_dtmp;
	*dtmpPtr = dtmp;

/* Dispose of MixerMaker and related structures. */
	mxDeleteMixerMaker( mxmk );

#if DEBUG_DumpMixerTemplate
	{
		char b[64];

		sprintf (b, "Mixer %d x %d, flags=0x%02x", MixerSpecToNumIn(mixerSpec), MixerSpecToNumOut(mixerSpec), MixerSpecToFlags(mixerSpec));
		dsppDumpTemplate (dtmp, b);
	}
#endif

	return Result;

/* ERROR cleanup. */
/* Dispose of MixerMaker and related structures. */
err_free_dtmp:
	dsppDeleteSuperTemplate( dtmp );
err_free_mxmk:
	mxDeleteMixerMaker( mxmk );
	return Result;
}
