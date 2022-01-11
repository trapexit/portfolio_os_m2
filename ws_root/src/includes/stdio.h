#ifndef __STDIO_H
#define __STDIO_H


/******************************************************************************
**
**  @(#) stdio.h 96/02/20 1.5
**
**  Standard C file IO definitions
**
******************************************************************************/


#ifndef __FILE_FILESYSTEM_H
#include <file/filesystem.h>
#endif

#ifndef __FILE_FILEIO_H
#include <file/fileio.h>
#endif

#ifndef __STDDEF_H
#include <stddef.h>
#endif

#ifndef __STDARG_H
#include <stdarg.h>
#endif


/*****************************************************************************/


#define SEEK_SET     FILESEEK_START
#define SEEK_CUR     FILESEEK_CURRENT
#define SEEK_END     FILESEEK_END

#define BUFSIZ       2048
#define EOF          -1
#define FOPEN_MAX    20
#define FILENAME_MAX FILESYSTEM_MAX_PATH_LEN
#define TMP_MAX      4294967295
#define L_tmpnam     32

#define _IOFBF	     0
#define _IOLBF	     1
#define _IONBF	     2

typedef struct FILE FILE;
typedef unsigned int fpos_t;


/*****************************************************************************/


/* default streams */
extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;


/*****************************************************************************/


#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */


extern FILE  *fopen(const char *filename, const char *mode);
extern FILE  *freopen(const char *filename, const char *mode, FILE *file);
extern int    fclose(FILE *file);
extern size_t fread(void *ptr, size_t size, size_t nitems, FILE *file);
extern size_t fwrite(const void *ptr, size_t size, size_t nitems, FILE *file);
extern int    fflush(FILE *file);

extern int putc(int c, FILE *file);
extern int putchar(int c);
extern int puts(const char *s);
extern int fputc(int c, FILE *file);
extern int fputs(const char *s, FILE *file);

extern int   getc(FILE *file);
extern int   getchar(void);
extern char *gets(char *s);
extern int   fgetc(FILE *file);
extern char *fgets(char *s, int n, FILE *file);
extern int   ungetc(int c, FILE *file);

extern int      fseek(FILE *file, long int offset, int whence);
extern long int ftell(FILE *file);
extern void     rewind(FILE *file);
extern int      fgetpos(FILE *file, fpos_t *pos);
extern int      fsetpos(FILE *file, const fpos_t *pos);

extern void setbuf(FILE *file, char *buf);
extern int  setvbuf(FILE *file, char *buf, int mode, size_t size);

extern void clearerr(FILE *file);
extern int  feof(FILE *file);
extern int  ferror(FILE *file);
extern void perror(const char *s);

extern int printf(const char *format, ...);
extern int fprintf(FILE *file, const char *format, ...);
extern int sprintf(char *s, const char *format, ...);

extern int vprintf(const char *format, va_list a);
extern int vfprintf(FILE *file, const char *format, va_list a);
extern int vsprintf(char *s, const char *format, va_list a);

extern int scanf(const char *format, ...);
extern int fscanf(FILE *file, const char *format, ...);
extern int sscanf(const char *s, const char *format, ...);

extern FILE *tmpfile(void);
extern char *tmpnam(char *s);
extern int   remove(const char *filename);
extern int   rename(const char *oldName, const char *newName);

/* non-standard extensions, string formatting with callback */
typedef int (* OutputFunc)(int ch, void *userData);
int vcprintf(const char *fmt, OutputFunc of, void *userData, va_list va);
int cprintf(const char *fmt, OutputFunc of, void *userData, ... );


#ifdef __cplusplus
}
#endif /* __cplusplus */


/*****************************************************************************/


#endif /* __STDIO_H */
