#ifndef __INTERNATIONAL_COUNTRYDB_H
#define __INTERNATIONAL_COUNTRYDB_H


/******************************************************************************
**
**  @(#) countrydb.h 96/02/20 1.6
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __INTERNATIONAL_INTL_H
#include <international/intl.h>
#endif

#ifndef __MISC_IFF_H
#include <misc/iff.h>
#endif


/****************************************************************************/


typedef struct FormHdr
{
    uint32 ID;
    uint32 Size;
    uint32 FormType;
} FormHdr;

typedef struct ChunkHdr
{
    uint32 ID;
    uint32 Size;
} ChunkHdr;

typedef struct CountryEntry
{
    CountryCodes ce_Country;
    uint32       ce_SeekOffset;
} CountryEntry;


#define ID_PREF 0x50524546
#define ID_INTL 0x494e544c
#define ID_CTRY 0x43545259

#define IFF_ROUND(x) ((x & 1) ? (x+1) : x)


/****************************************************************************/


#endif /* __INTERNATIONAL_COUNTRYDB_H */
