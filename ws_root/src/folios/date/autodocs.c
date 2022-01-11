/* @(#) autodocs.c 96/02/28 1.2 */

/**
|||	AUTODOC -class Date -name ConvertTimeValToGregorian
|||	Converts from a system TimeVal to a real-world date representation.
|||
|||	  Synopsis
|||
|||	    Err ConvertTimeValToGregorian(const TimeVal *tv, GregorianDate *gd);
|||
|||	  Description
|||
|||	    This function converts from a TimeVal format, which specifies a
|||	    date in number of seconds from January 1st 1993, to a gregorian
|||	    date format which is expressed in terms of year, month, and day.
|||
|||	  Arguments
|||
|||	    tv
|||	        The TimeVal to convert
|||
|||	    gd
|||	        A GregorianDate structure where the converted value is put.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Date folio V30.
|||
|||	  Associated Files
|||
|||	    <misc/date.h>, System.m2/Modules/date
|||
|||	  See Also
|||
|||	    ConvertGregorianToTimeVal()
|||
**/

/**
|||	AUTODOC -class Date -name ConvertGregorianToTimeVal
|||	Converts from a real-world date representation to a system TimeVal.
|||
|||	  Synopsis
|||
|||	    Err ConvertGregorianToTimeVal(const GregorianDate *gd, TimeVal *tv);
|||
|||	  Description
|||
|||	    This function converts from a gregorian date format, which is
|||	    expressed in terms of year, month, and day, into a system TimeVal
|||	    format, which specifies a date in number of seconds from
|||	    January 1st 1993.
|||
|||	  Arguments
|||
|||	    gd
|||	        The GregorianDate to convert.
|||
|||	    tv
|||	        A TimeVal structure where the converted date is put.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Date folio V30.
|||
|||	  Associated Files
|||
|||	    <misc/date.h>, System.m2/Modules/date
|||
|||	  See Also
|||
|||	    ConvertTimeValToGregorian()
|||
**/

/**
|||	AUTODOC -class Date -name ValidateDate
|||	Makes sure a date is valid.
|||
|||	  Synopsis
|||
|||	    Err ValidateDate(const GregorianDate *gd);
|||
|||	  Description
|||
|||	    This function makes sure that the supplied date is valid and doesn't
|||	    contain out-of-bounds values.
|||
|||	  Arguments
|||
|||	    gd
|||	        Pointer to an initialized GregorianDate structure to validate.
|||
|||	  Return Value
|||
|||	    Returns >= 0 if the date is valid, or a negative error code
|||	    if it isn't.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Date folio V30.
|||
|||	  Associated Files
|||
|||	    <misc/date.h>, System.m2/Modules/date
|||
**/

/* keep the compiler happy... */
extern int foo;
