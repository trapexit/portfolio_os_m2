/******************************************************************************
**
**  @(#) satemplates.c 96/06/05 1.2
**
******************************************************************************/

#include <kernel/types.h>
#include <kernel/debug.h>
#include <audio/audio.h>
#include <streaming/dserror.h>
#include <video_cd_streaming/dsstreamdefs.h>
#include <video_cd_streaming/saudiosubscriber.h>

#include "sachannel.h"
#include "sasupport.h"

#include <streaming/subscribertraceutils.h>

#if SAUDIO_TRACE_TEMPLATES
	
	#define		ADD_TRACE_L1(bufPtr, event, chan, value, ptr)	\
					AddTrace(bufPtr, event, chan, value, ptr)

#else	/* Trace is off */
	#define		ADD_TRACE_L1(bufPtr, event, chan, value, ptr)
#endif

#define FIXED_RATE		44100
#define VARIABLE_RATE	22050

/* The following defines are for the purpose of making the Opera instrument tags
 * compatible with M2.  The new tags are the new DSP instruments that are
 * optimized for M2.
 */

/* The following table describes all the AudioFolio instrument templates
 * that we know about.  These will be dynamically or pre-use loaded. The
 * user is able to cause them to be preloaded by sending us a control
 * message that tells us which of these is desired. Each is specified by
 * its tag value. The Item associated with the entity is cached so that
 * if we are asked for the same thing again later, we will already have
 * loaded it from disk. A copy of this table is made for each
 * instantiation of an audio subscriber. This is done because each 
 * subscriber thread must own its own items (which are cached in the table).
 * The following table is COPIED to allocated memory when a subscriber is
 * instantiated. The contents of the following should never be modified. */
const TemplateRec gInitialTemplates[] = {

	{ SA_22K_8B_M,			0,	"sampler_8_v1.dsp"		}, /* was: halfmono8.dsp */
	{ SA_22K_8B_S,			0,	"sampler_8_v2.dsp"		}, /* was: halfstereo8.dsp */
	{ SA_44K_8B_S,			0,	"sampler_8_f2.dsp"		}, /* was: fixedstereo8.dsp */
	{ SA_44K_8B_M,			0,	"sampler_8_f1.dsp"		}, /* was: fixedmono8.dsp */
	{ SA_44K_16B_M,			0,	"sampler_16_f1.dsp"		}, /* was: fixedmonosample.dsp */
	{ SA_44K_16B_S,			0,	"sampler_16_f2.dsp"		}, /* was: fixedstereosample.dsp */
	{ SA_22K_16B_M,			0,	"sampler_16_v1.dsp"		}, /* was: halfmonosample.dsp */
	{ SA_22K_16B_S,			0,	"sampler_16_v2.dsp"		}, /* was: halfstereosample.dsp */
	{ SA_44K_16B_M_SQD2,	0,	"sampler_sqd2_v1.dsp"	}, /* was: dcsqxdmono.dsp */
	{ SA_44K_16B_S_SQD2,	0,	"sampler_sqd2_v2.dsp" 	}, /* was: dcsqxdstereo.dsp */
	{ SA_22K_16B_M_SQD2,	0,	"sampler_sqd2_v1.dsp"	}, /* was: dcsqxdhalfmono.dsp */
	{ SA_22K_16B_S_SQD2,	0,	"sampler_sqd2_v2.dsp"	}, /* was: dcsqxdhalfstereo.dsp */
	{ SA_44K_16B_M_SQS2,	0,	"sampler_sqs2_f1.dsp"	}, /* was: dcsqxdmono.dsp */

/* Currently not supported */
	/* { SA_44K_16B_S_SQS2,	0,	"sampler_sqs2_f2.dsp" 	},*/ /* was: dcsqxdstereo.dsp */

	{ SA_22K_16B_M_SQS2,	0,	"sampler_sqs2_v1.dsp"	}, /* was: dcsqxdhalfmono.dsp */

/* Currently not supported */
	/* { SA_22K_16B_S_SQS2,	0,	"sampler_sqs2_v2.dsp"	}, */ /* was: dcsqxdhalfstereo.dsp */

	{ SA_44K_16B_S_CBD2,	0,	"sampler_cbd2_v2.dsp"	}, /* this is also used for SA_44K_16B_S_CBD2 samples */
	{ SA_44K_16B_M_CBD2,	0,	"sampler_cbd2_f1.dsp"	}, /* M2 only: Cube-root delta extact */
	{ SA_22K_16B_M_CBD2,	0,	"sampler_cbd2_v1.dsp"	},
	{ SA_22K_16B_S_CBD2,	0,	"sampler_cbd2_v2.dsp"	},
	{ SA_44K_16B_M_ADP4,	0,	"sampler_adp4_v1.dsp"	}, /* was: ADPmono.dsp */
	{ SA_22K_16B_M_ADP4,	0,	"sampler_adp4_v1.dsp"	}  /* was: ADPhalfmono.dsp */
	};

const uint32	kMaxTemplateCount =
	(sizeof(gInitialTemplates) / sizeof(TemplateRec));


/*******************************************************************************************
 * Routine to determine template cache tag based on the sample descriptor structure.
 * If we don't have a template for the data type return an error.
 *******************************************************************************************/
int32	GetTemplateTag( SAudioSampleDescriptorPtr descPtr )    
	{
	if ( descPtr->numChannels == 1 )
		{
		if ( descPtr->compressionType == ID_SQD2 )
			{
			if ( descPtr->sampleRate == VARIABLE_RATE )
				return SA_22K_16B_M_SQD2;
			else
				return SA_44K_16B_M_SQD2;			
			}

		if ( descPtr->compressionType == ID_SQS2 )
			{
			if ( descPtr->sampleRate == VARIABLE_RATE )
				return SA_22K_16B_M_SQS2;
			else
				return SA_44K_16B_M_SQS2;			
			}

		if ( descPtr->compressionType == ID_CBD2 )
			{
			if ( descPtr->sampleRate == VARIABLE_RATE )
				return SA_22K_16B_M_CBD2;
			else
				return SA_44K_16B_M_CBD2;			
			}

		if ( descPtr->compressionType == ID_ADP4 )
			{
			if ( descPtr->sampleRate == VARIABLE_RATE )
				return SA_22K_16B_M_ADP4;
			else
				return SA_44K_16B_M_ADP4;			
			}

		if (descPtr->sampleSize == 8)
			{
			if (descPtr->sampleRate == VARIABLE_RATE)
				return SA_22K_8B_M;
			else
				return SA_44K_8B_M;
			}

		if (descPtr->sampleRate == VARIABLE_RATE)
			return SA_22K_16B_M;

		if (descPtr->sampleRate == FIXED_RATE)
			return SA_44K_16B_M;
		}	

	if (descPtr->numChannels == 2)
		{
		if ( descPtr->compressionType == ID_SQD2)
			{
			if (descPtr->sampleRate == VARIABLE_RATE)
				return SA_22K_16B_S_SQD2;			
			else
				return SA_44K_16B_S_SQD2;			
			}

		if ( descPtr->compressionType == ID_SQS2 )
			{
			if (descPtr->sampleRate == VARIABLE_RATE)
				return SA_22K_16B_S_SQD2;			
			else
				return SA_44K_16B_S_SQD2;			
			}

		if ( descPtr->compressionType == ID_CBD2 )
			{
			if (descPtr->sampleRate == VARIABLE_RATE) 
				return SA_22K_16B_S_CBD2;
			else
				/* Since there is no instrument for 44K, 16bit, stereo samples,
				 * use the variable rate instrument. */
				return SA_44K_16B_S_CBD2;
			}

		if (descPtr->sampleSize == 8)
			{
			if (descPtr->sampleRate == VARIABLE_RATE)
				return SA_22K_8B_S;
			else
				return SA_44K_8B_S;
			}

		if (descPtr->sampleRate == VARIABLE_RATE)
			return SA_22K_16B_S;

		if (descPtr->sampleRate == FIXED_RATE)
			return SA_44K_16B_S;
		}

	/* Couldn't find a template match audio data type */
	return -1;
	}

/*******************************************************************************************
 * Routine to walk through the array of template records looking for the template specified
 * by 'templateTag'. If the template for the matching record has not been loaded, then load 
 * it immediately and cache the template item for later use.
 *******************************************************************************************/
Item GetTemplateItem( SAudioContextPtr ctx, int32 templateTag,
		uint32 templateCount )
	{
	uint32				k;
	TemplateRecPtr 		tp;
	
	/* Point to the array of instrument templates */
	tp = ctx->datatype.ThdoAudio.templateArray;
	
	/* Walk through the array of template records (cache) looking for the
	 * template specified by 'templateTag'. If the template for the matching
	 * record has not been loaded, then load it immediately and cache the
	 * template item for later use. */
	for ( k = 0; k < templateCount; k++, tp++ )
		{
		/* Check for an exact match of bits and that the template
		 * has not already been loaded. */
		if ( tp->templateTag == templateTag ) 
			{
			/* Check to see if the entity needs to be loaded and
			 * load it if so. */
			if ( tp->templateItem == 0 )
				tp->templateItem = LoadInsTemplate( tp->instrumentName, 0 );

			/* If we're going to use an ADPCM instrument, the support
			 * code must be loaded */
			if ( ( (templateTag == SA_44K_16B_M_ADP4) ||
				 (templateTag == SA_22K_16B_M_ADP4) ) 
				&& (ctx->datatype.ThdoAudio.decodeADPCMIns == 0) )
				{
				 ctx->datatype.ThdoAudio.decodeADPCMIns =
					LoadInstrument( "decodeadpcm.dsp", 0, 100 ); 
				}

			return tp->templateItem;
			}
		}

	return kDSTemplateNotFoundErr;
	}


/******************************************************************************
 * Routine search for and load a number of instrument templates given a pointer to a NULL
 * terminated list of tags that describe the instruments. The templates are cached (if
 * one searched for is already loaded, it is ignored).
 ******************************************************************************/
Err LoadTemplates( SAudioContextPtr ctx, int32* tagPtr, uint32 templateCount )
	{
	Err				status;

	/* Walk through the array of template tags and load any
	 * that are requested that have not already been loaded. */
	while ( *tagPtr != 0 )
		{
		status = GetTemplateItem( ctx, *tagPtr, templateCount );
		if ( status < 0 )
			return status;

		/* Advance to the next tag specified */
		tagPtr++;
		}

	ADD_TRACE_L1( SATraceBufPtr, kLoadedTemplates, -1, 0, 0 );
	
	return kDSNoErr;
	}

