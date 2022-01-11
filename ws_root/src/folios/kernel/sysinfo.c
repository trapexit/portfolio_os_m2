/* @(#) sysinfo.c 96/05/10 1.40 */

#include <kernel/types.h>
#include <kernel/sysinfo.h>
#include <kernel/kernel.h>
#include <kernel/panic.h>
#include <kernel/debug.h>
#include <dipir/dipirpub.h>
#include <dipir/rom.h>
#include <dipir/hwresource.h>
#include <hardware/bda.h>
#include <hardware/cde.h>

#define DBUG(x) /* printf x */

#define	DipirRoutines \
	((PublicDipirRoutines *)(KB_FIELD(kb_DipirRoutines)))

/* code for M2 System Information */

uint32
QueryCDSysInfo(uint32 tag, void *info, size_t size)
{
    DBUG(("enter: QueryCDSysInfo (%d, %d)\n", tag, size));
    if (size)
    switch (tag)
    {
	case SYSINFO_TAG_BDA:			/* BDA id, base addresses */
		 ((BDAInfo *)info)->bda_ID = BDA_READ(BDAPCTL_DEVID);
		 ((BDAInfo *)info)->bda_PBBase = (void *)BDAPCTL_BASE;
		 ((BDAInfo *)info)->bda_MCBase = (void *)BDAMCTL_BASE;
		 ((BDAInfo *)info)->bda_VDUBase = (void *)BDAVDU_BASE;
		 ((BDAInfo *)info)->bda_TEBase = (void *)0x40000;
		 ((BDAInfo *)info)->bda_DSPBase = (void *)0x60000;
		 ((BDAInfo *)info)->bda_CPBase = (void *)BDACP_BASE;
		 ((BDAInfo *)info)->bda_MPEGBase = (void *)0x80000;
		 break;

	case SYSINFO_TAG_CDE:			/* CDE id, base address(es) */
		 ((CDEInfo *)info)->cde_ID = CDE_READ(KB_FIELD(kb_CDEBase),CDE_DEVICE_ID);
		 ((CDEInfo *)info)->cde_Base = (void *)KB_FIELD(kb_CDEBase);
		 break;

	case SYSINFO_TAG_GRAFDISP:		/* Display mode */
		 *(DispModeInfo *)info =
		    SYSINFO_NTSC_SUPPORTED|SYSINFO_NTSC_DFLT|SYSINFO_NTSC_CURDISP;
		 break;

	case SYSINFO_TAG_CONTROLPORT:		/* Control Port Information */
		((ContPortInfo *)info)->cpi_CPFlags = 0;
		((ContPortInfo *)info)->cpi_RFU = 0;
		((ContPortInfo *)info)->cpi_CPLClkSpeedFac = 0;
		((ContPortInfo *)info)->cpi_CPSClkSpeedFac = 0;
		break;

	case SYSINFO_TAG_DSPPCLK:		/* DSPP Clock rate in Hz */
		 *(DSPPClkInfo *)info = 44100;
		 break;

	case SYSINFO_TAG_AUDIN:			/* Audio input channels */
		 *(AudInInfo *)info = 2;
		 break;

	case SYSINFO_TAG_INTLLANG:		/* Default international lang */
		 *(IntlLangInfo *)info =  SYSINFO_INTLLANG_USENGLISH;
		 break;

        case SYSINFO_TAG_HWRESOURCE:
		{
			int32 r;

			r = (DipirRoutines->pdr_GetHWResource)(
					((HWResource*)info)->hwr_InsertID,
					info, (uint32)size);
			if (r == -1)
				return SYSINFO_END;
		}
		break;

	default: return SYSINFO_BADTAG;
    }
    return SYSINFO_SUCCESS;
}

uint32
SetCDSysInfo(uint32 tag, void *info, size_t size)
{
    switch (tag)
    {
	case SYSINFO_TAG_WATCHDOG:
		if (((uint32)info == SYSINFO_WDOGENABLE) && (size == 0))
		    ;
		break;
	default:
		return SYSINFO_BADTAG;
    }
    return SYSINFO_SUCCESS;
}

/**
|||	AUTODOC -private -class kernel -name SuperQuerySysInfo
|||	Query system information
|||
|||	  Synopsis
|||
|||	    uint32 SuperQuerySysInfo(uint32 tag, void *info, size_t size)
|||
|||	  Description
|||
|||	    SuperQuerySysInfo returns the current system information
|||	    for the parameter tag.
|||
|||	  Arguments
|||
|||	    tag                         The system information to query. It
|||	                                must be one of the SuperQuerySysInfo
|||	                                tags defined in sysinfo.h.
|||
|||	    info                        The pointer to the optional buffer
|||	                                area where to return additional
|||	                                information. If no additional
|||	                                information is available for the
|||	                                tag or no additional information
|||	                                is desired, this parameter should
|||	                                be set to NULL.
|||
|||	    size                        The size of the info buffer in bytes.
|||	                                The value of this parameter is
|||	                                ignored if info is set to NULL.
|||
|||	  Return Value
|||
|||	    The procedure returns a system information for the tag.
|||	    If the possible values are predefined for the tag, the
|||	    appropriate value is returned; else SYSINFO_SUCCESS is returned
|||	    and the information for the tag is returned through info buffer.
|||	    SYSINFO_BADTAG is returned in case of an invalid tag.
|||	    SYSINFO_UNSUPPORTEDDTAG is returned in case the tag is unsupported
|||	    in the release.
|||
|||	  Implementation
|||
|||	    Supervisor-mode folio call implemented in kernel folio V21.
|||
|||	  Associated Files
|||
|||	    super.h                     ANSI C Prototype
|||	    sysinfo.h                   Tags Definition
|||
|||	  Caveats
|||
|||	    SuperQuerySysInfo relies on the system ROM to provide a
|||	    system information query procedure to which the query is to be
|||	    passed on. If the system ROM does not provide the information
|||	    query procedure, a default information appropriate for the
|||	    Opera hardware is instead returned by SuperQuerySysInfo.
|||
|||	  See Also
|||
|||	    SuperSetSysInfo()
**/

uint32
SuperQuerySysInfo(uint32 tag, void *info, size_t size)
{
	uint32 ret;

	DBUG(("SuperQuerySysInfo(%lx,%lx,%lx)\n", tag, info, size));
	DBUG(("QueryROMSysInfo=%lx\n", KB_FIELD(kb_QueryROMSysInfo)));

	ret = (*(KB_FIELD(kb_QueryROMSysInfo)))(tag, info, size);

	if (ret == SYSINFO_BADTAG)
		ret = QueryCDSysInfo(tag, info, size);

	DBUG(("SuperQuerySysInfo result = %lx\n", ret));
	return ret;
}

/**
|||	AUTODOC -private -class kernel -name SuperSetSysInfo
|||	Set system information
|||
|||	  Synopsis
|||
|||	    uint32 SuperSetSysInfo(uint32 tag, void *info, size_t size)
|||
|||	  Description
|||
|||	    SuperSetSysInfo sets the system information requested through tag.
|||
|||	  Arguments
|||
|||	    tag                         The system information to set. It
|||	                                must be one of the SuperSetSysInfo
|||	                                tags defined in sysinfo.h.
|||
|||	    info                        This parameter type is tag specific.
|||	                                If a tag requires upto two additional
|||	                                informations, the first additional
|||	                                information is passed through info.
|||	                                If a tag requires three or more
|||	                                additional informations, the info
|||	                                points to a buffer containing the
|||	                                informations. The specifics are
|||	                                detailed with every SuperSetSysInfo
|||	                                tag in sysinfo.h.
|||
|||	    size                        This parameter type is tag specific.
|||	                                If info points to a buffer, then
|||	                                size specifies the size of the
|||	                                buffer in bytes. Otherwise, it may
|||	                                be used to pass additional informations
|||	                                if required for the tag. The specifics
|||	                                are detailed with every SuperSetSysInfo
|||	                                tag in sysinfo.h.
|||
|||	  Return Value
|||
|||	    SYSINFO_SUCCESS is returned if information could be successfully
|||	    set, else SYSINFO_FAILURE is returned.
|||	    SYSINFO_BADTAG is returned in case of an invalid tag.
|||	    SYSINFO_UNSUPPORTEDDTAG is returned in case the tag is unsupported
|||	    in the release.
|||
|||	  Implementation
|||
|||	    Supervisor-mode folio call implemented in kernel folio V21.
|||
|||	  Associated Files
|||
|||	    super.h                     ANSI C Prototype
|||	    sysinfo.h                   Tags Definition
|||
|||	  Caveats
|||
|||	    SuperSetSysInfo relies on the system ROM to provide a procedure
|||	    for setting system information to which the request is to be
|||	    passed on. If the system ROM does not provide such a procedure,
|||	    the information is set as would be appropriate for the Opera
|||	    hardware.
|||
|||	  See Also
|||
|||	    SuperQuerySysInfo()
**/

uint32
SuperSetSysInfo(uint32 tag, void *info, size_t size)
{
	uint32 ret;

	DBUG(("SuperSetSysInfo(%lx,%lx,%lx)\n",tag,info,size));

	ret = (*(KB_FIELD(kb_SetROMSysInfo)))(tag, info, size);

	if (ret == SYSINFO_BADTAG)
		ret = SetCDSysInfo(tag, info, size);

	DBUG(("SuperSetSysInfo result = %lx\n", ret));
	return ret;
}
