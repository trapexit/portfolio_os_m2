/******************************************************************************
**
**  @(#) music_internal.h 95/12/12 1.8
**  $Id: music_internal.h,v 1.3 1995/01/12 22:51:00 peabody Exp $
**
**  libmusic.a common internal include file
**
**  By: Bill Barton
**
**  Copyright (c) 1994, 3DO Company.
**  This program is proprietary and confidential.
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  940921 WJB  Created.
**  950112 WJB  Removed an unnecessary space from ID string.
**  950719 WJB  Revised what strings for M2.
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**
******************************************************************************/

#ifndef __MUSIC_INTERNAL_H
#define __MUSIC_INTERNAL_H


/* -------------------- libmusic.a version string and package id strings */
/* @@@ this could be broken by a more optimal compiler or linker that eliminates unused data */

/*
    Macro to use to cause libmusic.a version to be included in key modules
*/
#define PULL_MUSICLIB_VERSION(package)          \
    extern const char libmusic_whatstring[];    \
    const char * const libmusic_##package##_versionref = libmusic_whatstring;

/*
    Define a package ID string. Also pulls in libmusic_whatstring.
*/
#define MUSICLIB_PACKAGE_ID(package)    \
    PULL_MUSICLIB_VERSION(package)      \
    const char libmusic_##package##_whatstring[] = "@(#) libmusic.a/"#package;


#endif /* __MUSIC_INTERNAL_H */
