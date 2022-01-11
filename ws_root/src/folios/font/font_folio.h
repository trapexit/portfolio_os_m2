/* @(#) font_folio.h 95/12/05 1.3 */

#ifndef __FONT_FOLIO_H
#define __FONT_FOLIO_H


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

#ifndef __KERNEL_SEMAPHORE_H
#include <kernel/semaphore.h>
#endif

#ifndef __GRAPHICS_FONT_H
#include <graphics/font.h>
#endif

/****************************************************************************/


#ifdef TRACING
#include <stdio.h>
#define TRACE(x)      printf x
#else
#define TRACE(x)
#endif

#define PROF_NAME "FONT_PROFILER"
#ifdef PROFILE
#define PROF_BLIT 0
#define PROF_F2_DRAW 1
#define PROF_CPY_SETUP 2
#define PROF_CPY_1 3
#define PROF_CPY_2 4
#define PROF_CPY_3 5
#define PROF_CNT 6
typedef struct Profile
{
    TimerTicks in;
    TimerTicks out;
    TimerTicks diff;
    TimerTicks total;
    TimeVal tv;
    uint32 cnt;
} Profile;
Profile *StartProfile(int32 entry);
void StopProfile(Profile *p);
void PrintProfile(void);
#define PROF_IN(p, x) (p) = StartProfile(x);
#define PROF_OUT(p) StopProfile(p);
#define PROF_PRINT PrintProfile();
#else
typedef uint32 Profile;
#define PROF_IN(p, x)
#define PROF_OUT(p)
#define PROF_PRINT
#endif

/****************************************************************************/

#define CACHE_SIZE GetPageSize(MEMTYPE_NORMAL)

typedef struct FontFolio
{
    Folio      ff;

    List       ff_Fonts;
    Semaphore *ff_FontLock;
    void      *ff_Cache;
    Item       ff_CacheLock; /* Semaphore */
} FontFolio;


/****************************************************************************/


extern FontFolio *FontBase;


/****************************************************************************/


#endif /* __FONT_FOLIO_H */
