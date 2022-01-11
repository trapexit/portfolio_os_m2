/* @(#) international_folio.h 95/12/01 1.6 */
/* $Id: international_folio.h,v 1.4 1994/12/22 20:13:42 vertex Exp $ */

#ifndef __INTERNATIONAL_FOLIO_H
#define __INTERNATIONAL_FOLIO_H


/****************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_NODES_H
#include <kernel/nodes.h>
#endif

#ifndef __KERNEL_FOLIO_H
#include <kernel/folio.h>
#endif

#ifndef __INTERNATIONAL_INTL_H
#include <international/intl.h>
#endif


/****************************************************************************/


#ifdef TRACING
#include <stdio.h>
#include <kernel/super.h>
#define TRACE(x)      printf x
#else
#define TRACE(x)
#endif


/****************************************************************************/


typedef struct
{
    Folio iff;
    Item  if_LocaleLock;
    Item  if_DefaultLocale;
} InternationalFolio;



/****************************************************************************/


extern InternationalFolio *InternationalBase;


/****************************************************************************/


#endif /* __INTERNATIONAL_FOLIO_H */
