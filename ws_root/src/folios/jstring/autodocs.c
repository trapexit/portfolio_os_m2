/* @(#) autodocs.c 96/01/26 1.6 */

/**
|||	AUTODOC -class Jstring -name ConvertASCII2ShiftJIS
|||	Converts an ASCII string to Shift-JIS.
|||
|||	  Synopsis
|||
|||	    int32 ConvertASCII2ShiftJIS(const char *string,
|||	                                char *result, uint32 resultSize,
|||	                                uint8 unknownFiller);
|||
|||	  Description
|||
|||	    Converts a string from ASCII to Shift-JIS. The conversion is done
|||	    as well as possible. If certain characters from the source string
|||	    cannot be represented in ASCII, the unknownFiller byte will be
|||	    inserted in the result buffer in their place.
|||
|||	  Arguments
|||
|||	    string
|||	        The string to convert.
|||
|||	    result
|||	        A memory buffer where the result of the conversion can be put.
|||
|||	    resultSize
|||	        The number of bytes available in the result buffer. This limits
|||	        the number of bytes that are put into the buffer.
|||
|||	    unknownFiller
|||	        If a character sequence cannot be adequately converted, then
|||	        this byte will be put into the result buffer in place of the
|||	        character sequence.
|||
|||	  Return Value
|||
|||	    If positive, returns the number of characters in the result buffer. If
|||	    negative, returns an error code. Possible error codes currently
|||	    include:
|||
|||	    JSTR_ERR_BUFFERTOOSMALL
|||	        There was not enough room in the result buffer.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Jstring folio V24.
|||
|||	  Associated Files
|||
|||	    <international/jstring.h>, System.m2/Modules/jstring
|||
**/

/**
|||	AUTODOC -class Jstring -name ConvertShiftJIS2ASCII
|||	Converts a Shift-JIS string to ASCII.
|||
|||	  Synopsis
|||
|||	    int32 ConvertShiftJIS2ASCII(const char *string,
|||	                                char *result, uint32 resultSize,
|||	                                uint8 unknownFiller);
|||
|||	  Description
|||
|||	    Converts a string from Shift-JIS to ASCII. The conversion is done
|||	    as well as possible. If certain characters from the source string
|||	    cannot be represented in ASCII, the unknownFiller byte will be
|||	    inserted in the result buffer in their place.
|||
|||	  Arguments
|||
|||	    string
|||	        The string to convert.
|||
|||	    result
|||	        A memory buffer where the result of the conversion can be put.
|||
|||	    resultSize
|||	        The number of bytes available in the result buffer. This limits
|||	        the number of bytes that are put into the buffer.
|||
|||	    unknownFiller
|||	        If a character sequence cannot be adequately converted, then
|||	        this byte will be put into the result buffer in place of the
|||	        character sequence.
|||
|||	  Return Value
|||
|||	    If positive, then the number of characters in the result buffer. If
|||	    negative, then an error code. Possible error codes currently
|||	    include:
|||
|||	    JSTR_ERR_BUFFERTOOSMALL
|||	        There was not enough room in the result buffer.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Jstring folio V24.
|||
|||	  Associated Files
|||
|||	    <international/jstring.h>, System.m2/Modules/jstring
**/

/**
|||	AUTODOC -class Jstring -name ConvertShiftJIS2UniCode
|||	Converts a Shift-JIS string to UniCode.
|||
|||	  Synopsis
|||
|||	    int32 ConvertShiftJIS2UniCode(const char *string,
|||	                                  unichar *result, uint32 resultSize,
|||	                                  uint8 unknownFiller);
|||
|||	  Description
|||
|||	    Converts a string from Shift-JIS to UniCode. The conversion is done
|||	    as well as possible. If certain characters from the source string
|||	    cannot be represented in ASCII, the unknownFiller byte will be
|||	    inserted in the result buffer in their place.
|||
|||	  Arguments
|||
|||	    string
|||	        The string to convert.
|||
|||	    result
|||	        A memory buffer where the result of the conversion can be put.
|||
|||	    resultSize
|||	        The number of bytes available in the result buffer. This limits
|||	        the number of bytes that are put into the buffer.
|||
|||	    unknownFiller
|||	        If a character sequence cannot be adequately converted, then
|||	        this byte will be put into the result buffer in place of the
|||	        character sequence.
|||
|||	  Return Value
|||
|||	    If positive, then the number of characters in the result buffer. If
|||	    negative, then an error code. Possible error codes currently
|||	    include:
|||
|||	    JSTR_ERR_BUFFERTOOSMALL
|||	        There was not enough room in the result buffer.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Jstring folio V24.
|||
|||	  Associated Files
|||
|||	    <international/jstring.h>, System.m2/Modules/jstring
**/

/**
|||	AUTODOC -class Jstring -name ConvertUniCode2ShiftJIS
|||	Converts a UniCode string to Shift-JIS.
|||
|||	  Synopsis
|||
|||	    int32 ConvertUniCode2ShiftJIS(const unichar *string,
|||	                                  char *result, uint32 resultSize,
|||	                                  uint8 unknownFiller);
|||
|||	  Description
|||
|||	    Converts a string from UniCode to Shift-JIS. The conversion is done
|||	    as well as possible. If certain characters from the source string
|||	    cannot be represented in ASCII, the unknownFiller byte will be
|||	    inserted in the result buffer in their place.
|||
|||	  Arguments
|||
|||	    string
|||	        The string to convert.
|||
|||	    result
|||	        A memory buffer where the result of the conversion can be put.
|||
|||	    resultSize
|||	        The number of bytes available in the result buffer. This limits
|||	        the number of bytes that are put into the buffer.
|||
|||	    unknownFiller
|||	        If a character sequence cannot be adequately converted, then
|||	        this byte will be put into the result buffer in place of the
|||	        character sequence.
|||
|||	  Return Value
|||
|||	    If positive, then the number of characters in the result buffer. If
|||	    negative, then an error code. Possible error codes currently
|||	    include:
|||
|||	    JSTR_ERR_BUFFERTOOSMALL
|||	        There was not enough room in the result buffer.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Jstring folio V24.
|||
|||	  Associated Files
|||
|||	    <international/jstring.h>, System.m2/Modules/jstring
|||
**/

/**
|||	AUTODOC -class Jstring -name ConvertFullKana2HalfKana
|||	Convert a full-width kana string to a half-width kana string.
|||
|||	  Synopsis
|||
|||	    int32 ConvertFullKana2HalfKana(const char *string,
|||	                                   char *result, uint32 resultSize,
|||	                                   uint8 unknownFiller);
|||
|||	  Description
|||
|||	    Converts a string from full-width kana to half-width kana. The
|||	    conversion is done as well as possible. If certain characters from
|||	    the source string cannot be represented in the target character set,
|||	    the unknownFiller byte will be inserted in the result buffer in
|||	    their place.
|||
|||	  Arguments
|||
|||	    string
|||	        The string to convert.
|||
|||	    result
|||	        A memory buffer where the result of the conversion can be put.
|||
|||	    resultSize
|||	        The number of bytes available in the result buffer. This limits
|||	        the number of bytes that are put into the buffer.
|||
|||	    unknownFiller
|||	        If a character sequence cannot be adequately converted, then
|||	        this byte will be put into the result buffer in place of the
|||	        character sequence.
|||
|||	  Return Value
|||
|||	    If positive, then the number of characters in the result buffer. If
|||	    negative, then an error code. Possible error codes currently
|||	    include:
|||
|||	    JSTR_ERR_BUFFERTOOSMALL
|||	        There was not enough room in the result buffer.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Jstring folio V24.
|||
|||	  Associated Files
|||
|||	    <international/jstring.h>, System.m2/Modules/jstring
**/

/**
|||	AUTODOC -class Jstring -name ConvertFullKana2Romaji
|||	Convert a full-width kana string to a romaji string.
|||
|||	  Synopsis
|||
|||	    int32 ConvertFullKana2Romaji(const char *string,
|||	                                 char *result, uint32 resultSize,
|||	                                 uint8 unknownFiller);
|||
|||	  Description
|||
|||	    Converts a string from full-width kana to romaji. The conversion is
|||	    done as well as possible. If certain characters from the source
|||	    string cannot be represented in the target character set, the
|||	    unknownFiller byte will be inserted in the result buffer in their
|||	    place.
|||
|||	  Arguments
|||
|||	    string
|||	        The string to convert.
|||
|||	    result
|||	        A memory buffer where the result of the conversion can be put.
|||
|||	    resultSize
|||	        The number of bytes available in the result buffer. This limits
|||	        the number of bytes that are put into the buffer.
|||
|||	    unknownFiller
|||	        If a character sequence cannot be adequately converted, then
|||	        this byte will be put into the result buffer in place of the
|||	        character sequence.
|||
|||	  Return Value
|||
|||	    If positive, then the number of characters in the result buffer. If
|||	    negative, then an error code. Possible error codes currently
|||	    include:
|||
|||	    JSTR_ERR_BUFFERTOOSMALL
|||	        There was not enough room in the result buffer.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Jstring folio V24.
|||
|||	  Associated Files
|||
|||	    <international/jstring.h>, System.m2/Modules/jstring
**/

/**
|||	AUTODOC -class Jstring -name ConvertFullKana2Hiragana
|||	Convert a full-width kana string to a hiragana string.
|||
|||	  Synopsis
|||
|||	    int32 ConvertFullKana2Hiragana(const char *string,
|||	                                   char *result, uint32 resultSize,
|||	                                   uint8 unknownFiller);
|||
|||	  Description
|||
|||	    Converts a string from full-width kana to hiragana. The conversion
|||	    is done as well as possible. If certain characters from the source
|||	    string cannot be represented in the target character set, the
|||	    unknownFiller byte will be inserted in the result buffer in their
|||	    place.
|||
|||	  Arguments
|||
|||	    string
|||	        The string to convert.
|||
|||	    result
|||	        A memory buffer where the result of the conversion can be put.
|||
|||	    resultSize
|||	        The number of bytes available in the result buffer. This limits
|||	        the number of bytes that are put into the buffer.
|||
|||	    unknownFiller
|||	        If a character sequence cannot be adequately converted, then
|||	        this byte will be put into the result buffer in place of the
|||	        character sequence.
|||
|||	  Return Value
|||
|||	    If positive, then the number of characters in the result buffer. If
|||	    negative, then an error code. Possible error codes currently
|||	    include:
|||
|||	    JSTR_ERR_BUFFERTOOSMALL
|||	        There was not enough room in the result buffer.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Jstring folio V24.
|||
|||	  Associated Files
|||
|||	    <international/jstring.h>, System.m2/Modules/jstring
**/

/**
|||	AUTODOC -class Jstring -name ConvertHalfKana2FullKana
|||	Convert a half-width kana string to a full-width kana string.
|||
|||	  Synopsis
|||
|||	    int32 ConvertHalfKana2FullKana(const char *string,
|||	                                   char *result, uint32 resultSize,
|||	                                   uint8 unknownFiller);
|||
|||	  Description
|||
|||	    Converts a string from half-width kana to full-width kana. The
|||	    conversion is done as well as possible. If certain characters from
|||	    the source string cannot be represented in the target character set,
|||	    the unknownFiller byte will be inserted in the result buffer in
|||	    their place.
|||
|||	  Arguments
|||
|||	    string
|||	        The string to convert.
|||
|||	    result
|||	        A memory buffer where the result of the conversion can be put.
|||
|||	    resultSize
|||	        The number of bytes available in the result buffer. This limits
|||	        the number of bytes that are put into the buffer.
|||
|||	    unknownFiller
|||	        If a character sequence cannot be adequately converted, then
|||	        this byte will be put into the result buffer in place of the
|||	        character sequence.
|||
|||	  Return Value
|||
|||	    If positive, then the number of characters in the result buffer. If
|||	    negative, then an error code. Possible error codes currently
|||	    include:
|||
|||	    JSTR_ERR_BUFFERTOOSMALL
|||	        There was not enough room in the result buffer.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Jstring folio V24.
|||
|||	  Associated Files
|||
|||	    <international/jstring.h>, System.m2/Modules/jstring
**/

/**
|||	AUTODOC -class Jstring -name ConvertHalfKana2Romaji
|||	Convert a half-width kana string to a romaji string.
|||
|||	  Synopsis
|||
|||	    int32 ConvertHalfKana2Romaji(const char *string,
|||	                                 char *result, uint32 resultSize,
|||	                                 uint8 unknownFiller);
|||
|||	  Description
|||
|||	    Converts a string from half-width kana to romaji. The conversion is
|||	    done as well as possible. If certain characters from the source
|||	    string cannot be represented in the target character set, the
|||	    unknownFiller byte will be inserted in the result buffer in their
|||	    place.
|||
|||	  Arguments
|||
|||	    string
|||	        The string to convert.
|||
|||	    result
|||	        A memory buffer where the result of the conversion can be put.
|||
|||	    resultSize
|||	        The number of bytes available in the result buffer. This limits
|||	        the number of bytes that are put into the buffer.
|||
|||	    unknownFiller
|||	        If a character sequence cannot be adequately converted, then
|||	        this byte will be put into the result buffer in place of the
|||	        character sequence.
|||
|||	  Return Value
|||
|||	    If positive, then the number of characters in the result buffer. If
|||	    negative, then an error code. Possible error codes currently
|||	    include:
|||
|||	    JSTR_ERR_BUFFERTOOSMALL
|||	        There was not enough room in the result buffer.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Jstring folio V24.
|||
|||	  Associated Files
|||
|||	    <international/jstring.h>, System.m2/Modules/jstring
**/

/**
|||	AUTODOC -class Jstring -name ConvertHalfKana2Hiragana
|||	Convert a half-width kana string to a hiragana string.
|||
|||	  Synopsis
|||
|||	    int32 ConvertHalfKana2Hiragana(const char *string,
|||	                                   char *result, uint32 resultSize,
|||	                                   uint8 unknownFiller);
|||
|||	  Description
|||
|||	    Converts a string from half-width kana to hiragana. The conversion
|||	    is done as well as possible. If certain characters from the source
|||	    string cannot be represented in the target character set, the
|||	    unknownFiller byte will be inserted in the result buffer in their
|||	    place.
|||
|||	  Arguments
|||
|||	    string
|||	        The string to convert.
|||
|||	    result
|||	        A memory buffer where the result of the conversion can be put.
|||
|||	    resultSize
|||	        The number of bytes available in the result buffer. This limits
|||	        the number of bytes that are put into the buffer.
|||
|||	    unknownFiller
|||	        If a character sequence cannot be adequately converted, then
|||	        this byte will be put into the result buffer in place of the
|||	        character sequence.
|||
|||	  Return Value
|||
|||	    If positive, then the number of characters in the result buffer. If
|||	    negative, then an error code. Possible error codes currently
|||	    include:
|||
|||	    JSTR_ERR_BUFFERTOOSMALL
|||	        There was not enough room in the result buffer.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Jstring folio V24.
|||
|||	  Associated Files
|||
|||	    <international/jstring.h>, System.m2/Modules/jstring
**/

/**
|||	AUTODOC -class Jstring -name ConvertRomaji2HalfKana
|||	Convert a romaji string to a half-width kana string.
|||
|||	  Synopsis
|||
|||	    int32 ConvertRomaji2HalfKana(const char *string,
|||	                                 char *result, uint32 resultSize,
|||	                                 uint8 unknownFiller);
|||
|||	  Description
|||
|||	    Converts a string from romaji to half-width kana. The conversion is
|||	    done as well as possible. If certain characters from the source
|||	    string cannot be represented in the target character set, the
|||	    unknownFiller byte will be inserted in the result buffer in their
|||	    place.
|||
|||	  Arguments
|||
|||	    string
|||	        The string to convert.
|||
|||	    result
|||	        A memory buffer where the result of the conversion can be put.
|||
|||	    resultSize
|||	        The number of bytes available in the result buffer. This limits
|||	        the number of bytes that are put into the buffer.
|||
|||	    unknownFiller
|||	        If a character sequence cannot be adequately converted, then
|||	        this byte will be put into the result buffer in place of the
|||	        character sequence.
|||
|||	  Return Value
|||
|||	    If positive, then the number of characters in the result buffer. If
|||	    negative, then an error code. Possible error codes currently
|||	    include:
|||
|||	    JSTR_ERR_BUFFERTOOSMALL
|||	        There was not enough room in the result buffer.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Jstring folio V24.
|||
|||	  Associated Files
|||
|||	    <international/jstring.h>, System.m2/Modules/jstring
**/

/**
|||	AUTODOC -class Jstring -name ConvertRomaji2FullKana
|||	Convert a romaji string to a full-width kana string.
|||
|||	  Synopsis
|||
|||	    int32 ConvertRomaji2FullKana(const char *string,
|||	                                 char *result, uint32 resultSize,
|||	                                 uint8 unknownFiller);
|||
|||	  Description
|||
|||	    Converts a string from romaji to full-width kana. The conversion is
|||	    done as well as possible. If certain characters from the source
|||	    string cannot be represented in the target character set, the
|||	    unknownFiller byte will be inserted in the result buffer in their
|||	    place.
|||
|||	  Arguments
|||
|||	    string
|||	        The string to convert.
|||
|||	    result
|||	        A memory buffer where the result of the conversion can be put.
|||
|||	    resultSize
|||	        The number of bytes available in the result buffer. This limits
|||	        the number of bytes that are put into the buffer.
|||
|||	    unknownFiller
|||	        If a character sequence cannot be adequately converted, then
|||	        this byte will be put into the result buffer in place of the
|||	        character sequence.
|||
|||	  Return Value
|||
|||	    If positive, then the number of characters in the result buffer. If
|||	    negative, then an error code. Possible error codes currently
|||	    include:
|||
|||	    JSTR_ERR_BUFFERTOOSMALL
|||	        There was not enough room in the result buffer.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Jstring folio V24.
|||
|||	  Associated Files
|||
|||	    <international/jstring.h>, System.m2/Modules/jstring
**/

/**
|||	AUTODOC -class Jstring -name ConvertRomaji2Hiragana
|||	Convert a romaji string to a hiragana string.
|||
|||	  Synopsis
|||
|||	    int32 ConvertRomaji2Hiragana(const char *string,
|||	                                 char *result, uint32 resultSize,
|||	                                 uint8 unknownFiller);
|||
|||	  Description
|||
|||	    Converts a string from romaji to hiragana. The conversion is done
|||	    as well as possible. If certain characters from the source string
|||	    cannot be represented in the target character set, the unknownFiller
|||	    byte will be inserted in the result buffer in their place.
|||
|||	  Arguments
|||
|||	    string
|||	        The string to convert.
|||
|||	    result
|||	        A memory buffer where the result of the conversion can be put.
|||
|||	    resultSize
|||	        The number of bytes available in the result buffer. This limits
|||	        the number of bytes that are put into the buffer.
|||
|||	    unknownFiller
|||	        If a character sequence cannot be adequately converted, then
|||	        this byte will be put into the result buffer in place of the
|||	        character sequence.
|||
|||	  Return Value
|||
|||	    If positive, then the number of characters in the result buffer. If
|||	    negative, then an error code. Possible error codes currently
|||	    include:
|||
|||	    JSTR_ERR_BUFFERTOOSMALL
|||	        There was not enough room in the result buffer.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Jstring folio V24.
|||
|||	  Associated Files
|||
|||	    <international/jstring.h>, System.m2/Modules/jstring
**/

/**
|||	AUTODOC -class Jstring -name ConvertHiragana2HalfKana
|||	Convert a hiragana string to a half-width kana string.
|||
|||	  Synopsis
|||
|||	    int32 ConvertHiragana2HalfKana(const char *string,
|||	                                   char *result, uint32 resultSize,
|||	                                   uint8 unknownFiller);
|||
|||	  Description
|||
|||	    Converts a string from hiragana to half-width kana. The
|||	    conversion is done as well as possible. If certain characters from
|||	    the source string cannot be represented in the target character set,
|||	    the unknownFiller byte will be inserted in the result buffer in
|||	    their place.
|||
|||	  Arguments
|||
|||	    string
|||	        The string to convert.
|||
|||	    result
|||	        A memory buffer where the result of the conversion can be put.
|||
|||	    resultSize
|||	        The number of bytes available in the result buffer. This limits
|||	        the number of bytes that are put into the buffer.
|||
|||	    unknownFiller
|||	        If a character sequence cannot be adequately converted, then
|||	        this byte will be put into the result buffer in place of the
|||	        character sequence.
|||
|||	  Return Value
|||
|||	    If positive, then the number of characters in the result buffer. If
|||	    negative, then an error code. Possible error codes currently
|||	    include:
|||
|||	    JSTR_ERR_BUFFERTOOSMALL
|||	        There was not enough room in the result buffer.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Jstring folio V24.
|||
|||	  Associated Files
|||
|||	    <international/jstring.h>, System.m2/Modules/jstring
**/

/**
|||	AUTODOC -class Jstring -name ConvertHiragana2FullKana
|||	Convert a hiragana string to a full-width kana string.
|||
|||	  Synopsis
|||
|||	    int32 ConvertHiragana2FullKana(const char *string,
|||	                                   char *result, uint32 resultSize,
|||	                                   uint8 unknownFiller);
|||
|||	  Description
|||
|||	    Converts a string from hiragana to full-width kana. The conversion
|||	    is done as well as possible. If certain characters from the source
|||	    string cannot be represented in the target character set, the
|||	    unknownFiller byte will be inserted in the result buffer in their
|||	    place.
|||
|||	  Arguments
|||
|||	    string
|||	        The string to convert.
|||
|||	    result
|||	        A memory buffer where the result of the conversion can be put.
|||
|||	    resultSize
|||	        The number of bytes available in the result buffer. This limits
|||	        the number of bytes that are put into the buffer.
|||
|||	    unknownFiller
|||	        If a character sequence cannot be adequately converted, then
|||	        this byte will be put into the result buffer in place of the
|||	        character sequence.
|||
|||	  Return Value
|||
|||	    If positive, then the number of characters in the result buffer. If
|||	    negative, then an error code. Possible error codes currently
|||	    include:
|||
|||	    JSTR_ERR_BUFFERTOOSMALL
|||	        There was not enough room in the result buffer.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Jstring folio V24.
|||
|||	  Associated Files
|||
|||	    <international/jstring.h>, System.m2/Modules/jstring
**/

/**
|||	AUTODOC -class Jstring -name ConvertHiragana2Romaji
|||	Convert a hiragana string to a romaji string.
|||
|||	  Synopsis
|||
|||	    int32 ConvertHiragana2Romaji(const char *string,
|||	                                 char *result, uint32 resultSize,
|||	                                 uint8 unknownFiller);
|||
|||	  Description
|||
|||	    Converts a string from hiragana to romaji. The conversion is done
|||	    as well as possible. If certain characters from the source string
|||	    cannot be represented in the target character set, the unknownFiller
|||	    byte will be inserted in the result buffer in their place.
|||
|||	  Arguments
|||
|||	    string
|||	        The string to convert.
|||
|||	    result
|||	        A memory buffer where the result of the conversion can be put.
|||
|||	    resultSize
|||	        The number of bytes available in the result buffer. This limits
|||	        the number of bytes that are put into the buffer.
|||
|||	    unknownFiller
|||	        If a character sequence cannot be adequately converted, then
|||	        this byte will be put into the result buffer in place of the
|||	        character sequence.
|||
|||	  Return Value
|||
|||	    If positive, then the number of characters in the result buffer. If
|||	    negative, then an error code. Possible error codes currently
|||	    include:
|||
|||	    JSTR_ERR_BUFFERTOOSMALL
|||	        There was not enough room in the result buffer.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Jstring folio V24.
|||
|||	  Associated Files
|||
|||	    <international/jstring.h>, System.m2/Modules/jstring
**/

/* keep the compiler happy... */
extern int foo;
