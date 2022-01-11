#ifndef _PARSERTYPES_H_
#define _PARSERTYPES_H_

#ifndef __KERNEL_TYPES_H
typedef long int32;
typedef unsigned long uint32;
typedef int32 Err;
typedef unsigned char bool;
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef NULL
#define NULL 0
#endif

#define BUILTIN_FILENAME "builtinclasses.sdf"
#define EOF_MARKER 0

#define PRINTDEBUG printDebug

#define PRTDEPTH {if (PRINTDEBUG) printf("Recurse [%ld %s:%d]\n", \
                                         recursionDepth, GetCurrentFileName(), lineNumber);}
#define PRTDEF(x) {if (printDefinitions) printf x;}
#define DPRT(x) {if (PRINTDEBUG) printf x;}
#define DTOKEN(x) {if (printTokenising) printf x;}

/* #define TOKEN_BUFFER_SIZE 65536 */
#define TOKEN_BUFFER_SIZE 65536 * 32
typedef struct TokenBuffer
{
    struct TokenBuffer *next;
    struct TokenBuffer *prev;
    char *position;
    char buffer[TOKEN_BUFFER_SIZE];
} TokenBuffer;

#define INC_BUFFER_POS(cast) \
      tb->position += sizeof(cast); \
      err = CheckTokenBufferOverflow(); \
      RET_ON_ERR(err); \

#define WRITE_TOKEN(cast, value) \
      DTOKEN(("Writing token 0x%lx [%s : %d]\n", value, GetCurrentFileName(), lineNumber)); \
      *(cast *)tb->position = value; \
      INC_BUFFER_POS(cast); \

void *mymalloc(uint32 size);

#endif

