#ifndef __DATACHUNK_INTERNAL_H
#define __DATACHUNK_INTERNAL_H


/****************************************************************************
**
**  @(#) datachunk_internal.h 96/02/09 1.3
**
**  Data chunk handler internal include file.
**
****************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif


/* -------------------- Debug */

#define DEBUG_DataChunk 0


/* -------------------- Internal data structures */

    /* ci_Data for MIFF_CI_DATACHUNK ContextInfo */
typedef struct DataChunk {
    void   *dc_Data;
} DataChunk;


/*****************************************************************************/


#endif /* __DATACHUNK_INTERNAL_H */
