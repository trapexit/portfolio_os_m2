/* @(#) autodocs.c 96/02/28 1.8 */

/**
|||	AUTODOC -class International -name intlCloseLocale
|||	Terminates use of a given Locale item.
|||
|||	  Synopsis
|||
|||	    Err intlCloseLocale(Item locItem);
|||
|||	  Description
|||
|||	    This function concludes a client's use of the given Locale item.
|||	    After this call is made, the Locale item may no longer be used.
|||
|||	  Arguments
|||
|||	    locItem
|||	        The Locale item, as obtained from intlOpenLocale().
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||	    Possible error codes currently include:
|||
|||	    INTL_ERR_BADITEM
|||	        locItem was not an existing Locale item.
|||
|||	    INTL_ERR_CANTCLOSEITEM
|||	        An attempt was made to close this Locale item more often than
|||	        it was opened.
|||
|||	  Implementation
|||
|||	    Macro implemented in <international/intl.h> V24.
|||
|||	  Associated Files
|||
|||	    <international/intl.h>, System.m2/Modules/international
|||
|||	  See Also
|||
|||	    intlOpenLocale(), intlLookupLocale()
|||
**/

/**
|||	AUTODOC -class International -name intlCompareStrings
|||	Compares two strings for collation purposes.
|||
|||	  Synopsis
|||
|||	    int32 intlCompareStrings(Item locItem, const unichar *string1,
|||	                             const unichar *string2);
|||
|||	  Description
|||
|||	    Compares two strings according to the collation rules of the Locale
|||	    item's language.
|||
|||	  Arguments
|||
|||	    locItem
|||	        A Locale item, as obtained from intlOpenLocale().
|||
|||	    string1
|||	        The first string to compare.
|||
|||	    string2
|||	        The second string to compare.
|||
|||	  Return Value
|||
|||	    -1
|||	        (string1 < string2)
|||
|||	    0
|||	        (string1 == string2)
|||
|||	    1
|||	        (string1>string2)
|||
|||	    INTL_ERR_BADITEM
|||	        locItem was not an existing Locale item.
|||
|||	  Implementation
|||
|||	    Folio call implemented in International folio V24.
|||
|||	  Associated Files
|||
|||	    <international/intl.h>, System.m2/Modules/international
|||
|||	  See Also
|||
|||	    intlOpenLocale(), intlConvertString()
|||
**/

/**
|||	AUTODOC -class International -name intlConvertString
|||	Changes certain attributes of a string.
|||
|||	  Synopsis
|||
|||	    int32 intlConvertString(Item locItem, const unichar *string,
|||	                            unichar *result, uint32 resultSize,
|||	                            uint32 flags);
|||
|||	  Description
|||
|||	    Converts character attributes within a string. The flags argument
|||	    specifies the type of conversion to perform.
|||
|||	  Arguments
|||
|||	    locItem
|||	        A Locale item, as obtained from intlOpenLocale().
|||
|||	    string
|||	        The string to convert.
|||
|||	    result
|||	        Where the result of the conversion is put. This area must be at
|||	        least as large as the number of bytes in the string.
|||
|||	    resultSize
|||	        The number of bytes available in the result buffer. This limits
|||	        the number of bytes that are put into the buffer.
|||
|||	    flags
|||	        Description of the conversion process to apply:
|||
|||	        INTL_CONVF_UPPERCASE will convert all characters to uppercase
|||	        if possible.
|||
|||	        INTL_CONVF_LOWERCASE will convert all characters to lowercase
|||	        if possible.
|||
|||	        INTL_CONVF_STRIPDIACRITICALS will remove diacritical marks from
|||	        all characters.
|||
|||	        INTL_CONVF_FULLWIDTH will convert all HalfKana characters to FullKana.
|||
|||	        INTL_CONVF_HALFWIDTH will convert all FullKana characters to HalfKana.
|||
|||	    You can also specify (INTL_CONVF_UPPERCASE|INTL_CONVF_STRIPDIACRITICALS) or
|||	    (INTL_CONVF_LOWERCASE|INTL_CONVF_STRIPDIACRITICALS) in order to
|||	    perform two conversions in one call.
|||
|||	    If flags is 0, then a straight copy occurs.
|||
|||	  Return Value
|||
|||	    If positive, returns the number of characters in the result buffer.
|||	    If negative, returns an error code. The string copied into the
|||	    result buffer is guaranteed to be NULL-terminated. Possible error
|||	    codes currently include:
|||
|||	    INTL_ERR_BADSOURCEBUFFER
|||	        The string pointer supplied was bad.
|||
|||	    INTL_ERR_BADRESULTBUFFER
|||	        The result buffer pointer was NULL or wasn't valid writable
|||	        memory.
|||
|||	    INTL_ERR_BUFFERTOOSMALL
|||	        There was not enough room in the result buffer.
|||
|||	    INTL_ERR_BADITEM
|||	        locItem was not an existing Locale item.
|||
|||	  Implementation
|||
|||	    Folio call implemented in International folio V24.
|||
|||	  Associated Files
|||
|||	    <international/intl.h>, System.m2/Modules/international
|||
|||	  Caveats
|||
|||	    This function varies in intelligence depending on the language
|||	    bound to the Locale argument. Specifically, most of the time,
|||	    all characters above 0x0ff are not affected by the routine. The
|||	    exception is with the Japanese language, where the Japanese
|||	    characters are also affected by this routine.
|||
|||	  See Also
|||
|||	    intlOpenLocale(), intlCompareStrings()
|||
**/

/**
|||	AUTODOC -class International -name intlFormatDate
|||	Formats a date in a localized manner.
|||
|||	  Synopsis
|||
|||	    int32 intlFormatDate(Item locItem, DateSpec spec,
|||	                         const GregorianDate *date,
|||	                         unichar *result, uint32 resultSize);
|||
|||	  Description
|||
|||	    Formats a date according to a template and to the rules specified by
|||	    the Locale item provided. The formatting string works similarly to
|||	    printf() formatting strings, but uses specialized format commands
|||	    tailored to date generation. The following format commands are
|||	    supported:
|||
|||	    %D - day of month
|||
|||	    %H - hour using 24-hour style
|||
|||	    %h - hour using 12-hour style
|||
|||	    %M - minutes
|||
|||	    %N - month name
|||
|||	    %n - abbreviated month name
|||
|||	    %O - month number
|||
|||	    %P - AM or PM strings
|||
|||	    %S - seconds
|||
|||	    %W - weekday name
|||
|||	    %w - abbreviated weekday name
|||
|||	    %Y - year
|||
|||	    In addition, the formatting string can also specify a field width,
|||	    a field limit, and a field pad character, in a manner identical to
|||	    the way printf() formatting strings specify these values:
|||
|||	    %[flags][width][.limit]command
|||
|||	    Flags can be "-" or "0", width is a positive numeric value, limit
|||	    is a positive numeric value, and command is one of the format
|||	    commands mentioned above. Refer to documentation on the standard C
|||	    printf() function for more information on how these numbers and
|||	    flags interact.
|||
|||	    A difference with standard printf() processing is that the limit
|||	    value is applied starting from the rightmost digits, instead of the
|||	    leftmost. For example:
|||
|||	      %.2Y
|||
|||	    Prints the rightmost two digits of the current year.
|||
|||	  Arguments
|||
|||	    locItem
|||	        A Locale item, as obtained from intlOpenLocale().
|||
|||	    spec
|||	        The formatting template describing the date layout. This value
|||	        is typically taken from the Locale structure (loc_Date,
|||	        loc_ShortDate, loc_Time, loc_ShortTime), but it can also be
|||	        built up by hand for custom formatting.
|||
|||	    date
|||	        The date to convert into a string.
|||
|||	    result
|||	        Where the result of the formatting is put.
|||
|||	    resultSize
|||	        The number of bytes available in the result buffer. This limits
|||	        the number of bytes that are put into the buffer.
|||
|||	  Return Value
|||
|||	    If positive, then the number of characters in the result buffer. If
|||	    negative, then an error code. The string copied into the result
|||	    buffer guaranteed to be NULL-terminated. Possible error codes
|||	    currently include:
|||
|||	    INTL_ERR_BADRESULTBUFFER
|||	        The result buffer pointer was NULL or wasn't valid writable
|||	        memory.
|||
|||	    INTL_ERR_BUFFERTOOSMALL
|||	        There was not enough room in the result buffer.
|||
|||	    INTL_ERR_BADDATESPEC
|||	        The pointer to the DateSpec array was bad.
|||
|||	    INTL_ERR_BADITEM
|||	        locItem was not an existing Locale item.
|||
|||	    INTL_ERR_BADDATE
|||	        The date specified in the GregorianDate structure is not a
|||	        valid date. For example, the gd_Month is greater than 12.
|||
|||	  Implementation
|||
|||	    Folio call implemented in International folio V24.
|||
|||	  Associated Files
|||
|||	    <international/intl.h>, System.m2/Modules/international
|||
|||	  See Also
|||
|||	    intlOpenLocale(), intlFormatNumber()
|||
**/

/**
|||	AUTODOC -class International -name intlFormatNumber
|||	Format a number in a localized manner.
|||
|||	  Synopsis
|||
|||	    int32 intlFormatNumber(Item locItem, const NumericSpec *spec,
|||	                           uint32 whole, uint32 frac,
|||	                           bool negative, bool doFrac,
|||	                           unichar *result, uint32 resultSize);
|||
|||	  Description
|||
|||	    This function formats a number according to the rules contained in
|||	    the NumericSpec structure. The NumericSpec structure is normally
|||	    taken from a Locale structure. The Locale structure contains three
|||	    initialized NumericSpec structures (loc_Numbers, loc_Currency, and
|||	    loc_SmallCurrency) which let you format numbers in an appropriate
|||	    manner for the system's current country selection.
|||
|||	    You can create your own NumericSpec structure, which lets you use
|||	    intlFormatNumber() to handle custom formatting needs. The fields in
|||	    the structure have the following meaning:
|||
|||	    ns_PosGroups
|||	        A GroupingDesc value defining how digits are grouped to the
|||	        left of the decimal character. A GroupingDesc is simply a 32-bit
|||	        bitfield. Every ON bit in the bitfield indicates that the
|||	        separator sequence should be inserted after the associated
|||	        digit. So if the third bit (bit #2) is ON, it means that the
|||	        grouping separator should be inserted after the third digit of
|||	        the formatted number.
|||
|||	    ns_PosGroupSep
|||	        A string used to separate groups of digits to the left of
|||	        the decimal character.
|||
|||	    ns_PosRadix
|||	        A string used as a decimal character.
|||
|||	    ns_PosFractionalGroups
|||	        A GroupingDesc value defining how digits are grouped to the
|||	        right of the decimal character.
|||
|||	    ns_PosFractionalGroupSep
|||	        A string used to separate groups of digits to the right of the
|||	        decimal character.
|||
|||	    ns_PosFormat
|||	        This field is used to do post-processing on a formatted
|||	        number. This is typically used to add currency notation around
|||	        a numeric value. The string in this field is used as a format
|||	        string in a sprintf() function call, and the formatted numeric
|||	        value is supplied as a parameter to the same sprintf() call.
|||	        For example, if the ns_PosFormat field is defined as "$%s", and
|||	        the formatted numeric value is "0.02", then the result of the
|||	        post-processing will be "$0.02". When this field is NULL, no
|||	        post-processing occurs.
|||
|||	    ns_PosMinFractionalDigits
|||	        Specifies the minimum number of digits to display to the right
|||	        of the decimal character. If there are not enough digits, then
|||	        the string will be padded on the right with 0s.
|||
|||	    ns_PosMaxFractionalDigits
|||	        Specifies the maximum number of digits to display to the right
|||	        of the decimal character. Any excess digits are just removed.
|||
|||	    ns_NegGroups, ns_NegGroupSep, ns_NegRadix, ns_NegFractionalGroups,
|||	    ns_NegFractionalGroupSep, ns_NegFormat, ns_NegMinFractionalDigits,
|||	    ns_NegMaxFractionalDigits
|||	        These fields have the same function as the eight fields
|||	        described above, except that they are used to process negative
|||	        amounts, while the previous fields are used for positive
|||	        amounts.
|||
|||	    ns_Zero
|||	        If the number being processed is 0, then this string pointer is
|||	        used as-is'and is copied directly into the result buffer. If
|||	        this field is NULL, the number is formatted as if it were a
|||	        positive number.
|||
|||	    ns_Flags
|||	        This is reserved for future use and must always be 0.
|||
|||	  Arguments
|||
|||	    locItem
|||	        A Locale Item, as obtained from intlOpenLocale().
|||
|||	    spec
|||	        The formatting template describing the number layout. This
|||	        structure is typically taken from a Locale structure
|||	        (loc_Numbers, loc_Currency, loc_SmallCurrency), but it can
|||	        also be built up by hand for custom formatting.
|||
|||	    whole
|||	        The whole component of the number to format.
|||	        (The part of the number to the left of the radix character.)
|||
|||	    frac
|||	        The decimal component of the number to format. (The part of
|||	        the number to the right of the radix character.). This is
|||	        specified in number of billionth. For example, to
|||	        represent .5, you would use 500000000. To represent .0004, you
|||	        would use 400000.
|||
|||	    negative
|||	        TRUE if the number is negative, and FALSE if the number is
|||	        positive.
|||
|||	    doFrac
|||	        TRUE if a complete number with a decimal mark and decimal
|||	        digits is desired. FALSE if only the whole part of the number
|||	        should be output.
|||
|||	    result
|||	        Where the result of the formatting is put.
|||
|||	    resultSize
|||	        The number of bytes available in the result buffer. This limits
|||	        the number of bytes which are put into the buffer.
|||
|||	  Return Value
|||
|||	    If positive, then the number of characters in the result buffer. If
|||	    negative, then an error code. The string copied into the result
|||	    buffer is guaranteed to be NULL-terminated. Possible error codes
|||	    currently include:
|||
|||	    INTL_ERR_BADNUMERICSPEC
|||	        The pointer to the NumericSpec structure was bad.
|||
|||	    INTL_ERR_BADRESULTBUFFER
|||	        The result buffer pointer was NULL or wasn't valid writable
|||	        memory.
|||
|||	    INTL_ERR_BUFFERTOOSMALL
|||	        There was not enough room in the result buffer.
|||
|||	    INTL_ERR_BADITEM
|||	        locItem was not an existing Locale Item.
|||
|||	  Implementation
|||
|||	    Folio call implemented in International folio V24.
|||
|||	  Associated Files
|||
|||	    <international/intl.h>, System.m2/Modules/international
|||
|||	  See Also
|||
|||	    intlOpenLocale(), intlFormatDate()
|||
**/

/**
|||	AUTODOC -class International -name intlGetCharAttrs
|||	Returns attributes describing a given character.
|||
|||	  Synopsis
|||
|||	    int32 intlGetCharAttrs(Item locItem, unichar character);
|||
|||	  Description
|||
|||	    This function examines the provided UniCode character and returns
|||	    general information about it.
|||
|||	  Arguments
|||
|||	    locItem
|||	        A Locale item, as obtained from intlOpenLocale().
|||
|||	    character
|||	        The character to get the attribute of.
|||
|||	  Return Value
|||
|||	    Returns a bit mask, with bit sets to indicate various
|||	    characteristics as defined by the UniCode standard. The possible
|||	    bits are:
|||
|||	    INTL_ATTRF_UPPERCASE
|||	        This character is uppercase.
|||
|||	    INTL_ATTRF_LOWERCASE
|||	        This character is lowercase.
|||
|||	    INTL_ATTRF_PUNCTUATION
|||	        This character is a punctuation mark.
|||
|||	    INTL_ATTRF_DECIMAL_DIGIT
|||	        This character is a numeric digit.
|||
|||	    INTL_ATTRF_NUMBER
|||	        This character represent a numerical value not representable as
|||	        a single decimal digit. For example, the character 0x00bc
|||	        represents the constant 1/2.
|||
|||	    INTL_ATTRF_NONSPACING
|||	        This character is a nonspacing mark.
|||
|||	    INTL_ATTRF_SPACE
|||	        This character is a space character.
|||
|||	    INTL_ATTRF_HALF_WIDTH
|||	        This character is HalfKana.
|||
|||	    INTL_ATTRF_FULL_WIDTH
|||	        This character is FullKana.
|||
|||	    INTL_ATTRF_KANA
|||	        This character is Kana (Katakana).
|||
|||	    INTL_ATTRF_HIRAGANA
|||	        This character is Hiragana.
|||
|||	    INTL_ATTRF_KANJI
|||	        This character is Kanji.
|||
|||	    If the value returned by this function is negative, then it is not
|||	    a bit mask, and is instead an error code.
|||
|||	    INTL_ERR_BADITEM
|||	        locItem was not an existing Locale item.
|||
|||	  Implementation
|||
|||	    Folio call implemented in International folio V24.
|||
|||	  Associated Files
|||
|||	    <international/intl.h>, System.m2/Modules/international
|||
|||	  Caveats
|||
|||	    This function currently does not report any attributes for many
|||	    upper UniCode characters. Only the ECMA Latin-1 character page
|||	    (0x0000 to 0x00ff) is handled correctly at this time. If the
|||	    language bound to the Locale structure is Japanese, then this
|||	    function will also work correctly for Japanese characters.
|||
|||	  See Also
|||
|||	    intlOpenLocale(), intlConvertString()
|||
**/

/**
|||	AUTODOC -class International -name intlLookupLocale
|||	Returns a pointer to a Locale structure.
|||
|||	  Synopsis
|||
|||	    Locale *intlLookupLocale(Item locItem);
|||
|||	  Description
|||
|||	    This macro returns a pointer to a Locale structure. The structure
|||	    can then be examined and its contents used to localize titles.
|||
|||	  Arguments
|||
|||	    locItem
|||	        A Locale item, as obtained from intlOpenLocale().
|||
|||	  Return Value
|||
|||	    The macro returns a pointer to a locale structure, which contains
|||	    localization information, or NULL if the supplied Item was not a
|||	    valid Locale item.
|||
|||	  Implementation
|||
|||	    Macro implemented in <international/intl.h> V24.
|||
|||	  Associated Files
|||
|||	    <international/intl.h>, System.m2/Modules/international
|||
|||	  Example
|||
|||	    {
|||	    Item    |t;
|||	    Locale *loc;
|||
|||	        it = intlOpenLocale(NULL);
|||	        {
|||	            loc = intlLookupLocale(it);
|||
|||	            // you can now read fields in the Locale structure.
|||	            printf("Language is %ldn",loc->loc_Language);
|||
|||	            intlCloseLocale(it);
|||	        }
|||	    }
|||
|||	  See Also
|||
|||	    intlOpenLocale(), intlCloseLocale()
|||
**/

/**
|||	AUTODOC -class International -name intlOpenLocale
|||	Gains access to a Locale item.
|||
|||	  Synopsis
|||
|||	    Item intlOpenLocale(const TagArg *tags);
|||
|||	  Description
|||
|||	    This function returns a Locale item. You can then use
|||	    intlLookupLocale() to gain access to the localization information
|||	    within the Locale item. This information enables software to adapt
|||	    to different languages and customs automatically at run-time, thus
|||	    creating truly international software.
|||
|||	  Arguments
|||
|||	    tags
|||	        A pointer to an array of tag arguments containing extra data
|||	        for this function. This must currently always be NULL.
|||
|||	  Return Value
|||
|||	    The function returns the item number of a Locale. You can use
|||	    intlLookupLocale() to get a pointer to the locale structure, which
|||	    contains localization information.
|||
|||	  Implementation
|||
|||	    Macro implemented in <international/intl.h> V24.
|||
|||	  Associated Files
|||
|||	    <international/intl.h>, System.m2/Modules/international
|||
|||	  Notes
|||
|||	    Once you are finished with the Locale item, you should call
|||	    intlCloseLocale().
|||
|||	  See Also
|||
|||	    intlCloseLocale(), intlLookupLocale()
|||
**/

/**
|||	AUTODOC -class International -name intlTransliterateString
|||	Converts a string between character sets.
|||
|||	  Synopsis
|||
|||	    int32 intlTransliterateString(const void *string, CharacterSets stringSet,
|||	                                  void *result, CharacterSets resultSet,
|||	                                  uint32 resultSize, uint8 unknownFiller);
|||
|||	  Description
|||
|||	    Converts a string between two character sets. This is typically used
|||	    to convert a string from or to UniCode. The conversion is done as well
|||	    as possible. If certain characters from the source string cannot be
|||	    represented in the destination character set, the unknownFiller byte
|||	    will be inserted in the result buffer in their place.
|||
|||	  Arguments
|||
|||	    string
|||	        The string to transliterate.
|||
|||	    stringSet
|||	        The character set of the string to transliterate. This describes
|||	        the interpretation of the bytes in the source string.
|||
|||	    result
|||	        A memory buffer where the result of the transliteration can be
|||	        put.
|||
|||	    resultSet
|||	        The character set to use for the resulting string.
|||
|||	    resultSize
|||	        The number of bytes available in the result buffer. This limits
|||	        the number of bytes that are put into the buffer.
|||
|||	    unknownFiller
|||	        If a character sequence cannot be adequately converted from the
|||	        source character set, then this byte will be put into the result
|||	        buffer in place of the character sequence. When converting to a
|||	        16-bit character set, then this byte will be extended to 16-bits
|||	        and inserted.
|||
|||	  Return Value
|||
|||	    If positive, returns the number of characters in the result buffer. If
|||	    negative, returns an error code. Possible error codes currently
|||	    include:
|||
|||	    INTL_ERR_BADSOURCEBUFFER
|||	        The string pointer supplied was bad.
|||
|||	    INTL_ERR_BADCHARACTERSET
|||	        The "stringSet" or "resultSet" arguments did
|||	        not specify valid character sets.
|||
|||	    INTL_ERR_BADRESULTBUFFER
|||	        The result buffer pointer was NULL or wasn't valid writable
|||	        memory.
|||
|||	    INTL_ERR_BUFFERTOOSMALL
|||	        There was not enough room in the result buffer.
|||
|||	    INTL_ERR_BADITEM
|||	        locItem was not an existing Locale item.
|||
|||	  Implementation
|||
|||	    Folio call implemented in International folio V24.
|||
|||	  Caveats
|||
|||	    This function is not as smart as it could be. When converting from
|||	    UniCode to ASCII, characters not in the first UniCode page (codes
|||	    greater than 0x00ff) are never converted and always replaced with
|||	    the unknownFiller byte. The upper pages of the UniCode set contain
|||	    many characters which could be converted to equivalent ASCII
|||	    characters, but these are not supported at this time.
|||
|||	  Associated Files
|||
|||	    <international/intl.h>, System.m2/Modules/international
|||
**/

/**
|||	AUTODOC -class Items -name Locale
|||	A database of international information.
|||
|||	  Description
|||
|||	    A Locale structure contains a number of definitions to enable
|||	    software to operate transparently across a wide range of cultural
|||	    and language environments.
|||
|||	  Folio
|||
|||	    International
|||
|||	  Item Type
|||
|||	    LOCALENODE
|||
|||	  Use
|||
|||	    intlOpenLocale(), intlCloseLocale(), intlLookupLocale()
|||
**/

/* keep the compiler happy... */
extern int foo;
