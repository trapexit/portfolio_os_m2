! @(#) linkexec.x 96/06/13 1.6
!
MODULE 13
EXPORTS 0=intlGetCharAttrs
EXPORTS 1=intlConvertString
EXPORTS 2=intlCompareStrings
EXPORTS 3=intlTransliterateString
EXPORTS 4=intlFormatDate
EXPORTS 5=intlFormatNumber
EXPORTS 6=intlOpenLocale

IMPORT_ON_DEMAND date
IMPORT_ON_DEMAND batt
IMPORT_ON_DEMAND jstring

REIMPORT_ALLOWED date
REIMPORT_ALLOWED batt
REIMPORT_ALLOWED jstring
