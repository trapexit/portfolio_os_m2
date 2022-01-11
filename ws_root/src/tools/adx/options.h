/* @(#) options.h 96/01/03 1.5 */

#ifndef __OPTIONS_H
#define __OPTIONS_H


/*****************************************************************************/


typedef enum OutputPlatforms
{
    PLATFORM_MAC,
    PLATFORM_PC,
    PLATFORM_UNIX
} OutputPlatforms;

extern OutputPlatforms outputPlatform;
extern bool            verbose;
extern bool            extractPrivate;
extern bool            extractInternal;
extern List            includeDirs;


/*****************************************************************************/


#endif /* __OPTIONS_H */
