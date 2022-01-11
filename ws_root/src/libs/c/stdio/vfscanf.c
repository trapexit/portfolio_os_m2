/* @(#) vfscanf.c 96/05/08 1.10 */

#include <kernel/types.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <math.h>
#include "stdioerrs.h"

/*****************************************************************************/


typedef enum ResultSize
{
    SIZE_DEFAULT,
    SIZE_SHORT,
    SIZE_LONG,
    SIZE_LONG_DOUBLE
} ResultSize;

typedef enum FPStates
{
    FP_SIGN,
    FP_LEFT_NUMBER,
    FP_POINT,
    FP_RIGHT_NUMBER,
    FP_E,
    FP_EXPONENT_SIGN,
    FP_EXPONENT_NUMBER,
    FP_DONE
} FPStates;


/*****************************************************************************/


#define GetCh {ch = (*get)(userData); numGotten++;}

int __vfscanf(const char *fmt, va_list va,
              GetCFunc get, UnGetCFunc unget, void *userData)
{
ResultSize    size;
int           ch;
int           fieldWidth;
int           numGotten;
void         *ptr;

int           args;
bool          neg;
long          num;
long          new;
long          base;

    args      = 0;
    numGotten = 0;

    GetCh;
    while ((ch != EOF) && (*fmt))
    {
        if (isspace(*fmt))
        {
            while (isspace(ch))
            {
                GetCh;
            }

            do
            {
                fmt++;
            }
            while (isspace(*fmt));
        }
        else if (*fmt != '%')
        {
            if (ch != *fmt)
            {
                break;
            }
            GetCh;
            fmt++;
        }
        else
        {
            fmt++;
            if (*fmt == '%')
            {
                if (ch != '%')
                {
                    break;
                }
                GetCh;
                fmt++;
            }
            else if (*fmt)
            {
                if (*fmt == '*')
                {
                    ptr = NULL;
                    fmt++;
                }
                else
                {
                    ptr = va_arg(va, void *);
                }

                fieldWidth = -1;
		if (isdigit(*fmt))
                {
                    fieldWidth = 0;
                    do
                    {
                        fieldWidth = fieldWidth*10 + (*fmt - '0');
                        fmt++;
                    }
		    while (isdigit(*fmt));
                }

                size = SIZE_DEFAULT;
                if (*fmt == 'h')
                {
                    size = SIZE_SHORT;
                    fmt++;
                }
                else if (*fmt == 'l')
                {
                    size = SIZE_LONG;
                    fmt++;
                }
                else if (*fmt == 'L')
                {
                    size = SIZE_LONG_DOUBLE;
                    fmt++;
                }

                if ((*fmt != 'c') && (*fmt != 'n') && (*fmt != '[') && (*fmt))
                {
                    while (isspace(ch) && (ch != EOF))
                        GetCh;

                    if (ch == EOF)
                        break;
                }

                switch (*fmt)
                {
                    case 'c': if (fieldWidth < 0)
                                  fieldWidth = 1;

                              do
                              {
                                  if (ptr)
                                      *(char *)ptr = ch;

                                  fieldWidth--;
                                  GetCh;
                              }
                              while (fieldWidth && (ch != EOF));

                              if (ptr)
                                  args++;

                              break;

                    case 'e':
                    case 'E':
                    case 'f':
                    case 'g':
                    case 'G': if (fieldWidth < 0)
                                  fieldWidth = INT_MAX;

                              {
                              char     buf[32];
                              int      n;
                              FPStates state;

                                  n     = 0;
                                  state = FP_SIGN;

                                  while (fieldWidth && (n < sizeof(buf) - 1))
                                  {
                                      switch (state)
                                      {
                                          case FP_SIGN:
                                          case FP_EXPONENT_SIGN:
                                                  state++;
                                                  if ((ch == '-') || (ch == '+'))
                                                      break;
                                                  continue;

                                          case FP_LEFT_NUMBER:
                                          case FP_RIGHT_NUMBER:
                                          case FP_EXPONENT_NUMBER:
						  if (isdigit(ch))
                                                      break;
                                                  state++;
                                                  continue;

                                          case FP_POINT:
                                                  state++;
                                                  if (ch == '.')
                                                      break;
                                                  continue;

                                          case FP_E:
                                                  if ((ch == 'e') || (ch == 'E'))
                                                  {
                                                      state++;
                                                      break;
                                                  }
                                                  state = FP_DONE;
                                                  break;
                                      }

                                      if (state == FP_DONE)
                                          break;

                                      buf[n++] = ch;
                                      GetCh;
                                      fieldWidth--;
                                  }

                                  buf[n] = 0;
                                  if (n == 0)
                                      return (EOF);

                                  if (ptr)
                                  {
                                      if (size == SIZE_LONG)
                                      {
#if 0
                                          *(double *)ptr = strtod(buf, NULL);
#else
					  /* the 602 can't do doubles */
					  return -1;
#endif
                                      }
                                      else if (size == SIZE_LONG_DOUBLE)
                                      {
#if 0
                                          *(long double *)ptr = strtod(buf,NULL);
#else
					  /* the 602 can't do doubles */
					  return -1;
#endif
                                      }
                                      else
                                      {
                                          /* this should be strtod(), but since
                                           * the 602 doesn't do doubles...
                                           */
                                          *(float *)ptr = strtof(buf, NULL);
                                      }
                                      args++;
                                  }
                              }
                              break;

                    case 'n': if (ptr)
                              {
                                  if (size == SIZE_SHORT)
                                      *(short *)ptr = numGotten - 1;
                                  else if (size == SIZE_LONG)
                                      *(long *)ptr = numGotten - 1;
                                  else
                                      *(int *)ptr = numGotten - 1;
                              }
                              break;

                    case 'd':
                    case 'o':
                    case 'i':
                    case 'p':
                    case 'u':
                    case 'x':
                    case 'X': if (fieldWidth < 0)
                                  fieldWidth = INT_MAX;

                              num = 0;
                              neg = FALSE;
                              if (fieldWidth && (ch == '-'))
                              {
                                  neg = TRUE;
                                  GetCh;
                                  fieldWidth--;
                              }

                              if (fieldWidth)
                              {
                                  base = 10;
                                  if (*fmt == 'o')
                                  {
                                      base = 8;
                                  }
                                  else if ((*fmt == 'x') || (*fmt == 'X') || (*fmt == 'p'))
                                  {
                                      base = 16;
                                      if (ch == '0')
                                      {
                                          GetCh;
                                          fieldWidth--;

                                          if (fieldWidth && ((ch == 'x') || (ch == 'X')))
                                          {
                                              GetCh;
                                              fieldWidth--;
                                          }
                                      }
                                  }
                                  else if (*fmt == 'i')
                                  {
                                      if (ch == '0')
                                      {
                                          GetCh;
                                          fieldWidth--;
                                          if (fieldWidth && ((ch == 'x') || (ch == 'X')))
                                          {
                                              base = 16;
                                              GetCh;
                                              fieldWidth--;
                                          }
                                          else
                                          {
                                              base = 8;
                                          }
                                      }
                                  }

                                  while (fieldWidth)
                                  {
                                      if (ch == EOF)
                                      {
                                          break;
                                      }
				      else if (isdigit(ch))
                                      {
                                          new = ch - '0';
                                      }
                                      else if ((ch >= 'a') && (ch <= 'f'))
                                      {
                                          new = ch - 'a' + 10;
                                      }
                                      else if ((ch >= 'A') && (ch <= 'F'))
                                      {
                                          new = ch - 'A' + 10;
                                      }
                                      else
                                      {
                                          break;
                                      }

                                      if (new >= base)
                                          break;

                                      num = num * base + new;

                                      GetCh;
                                      fieldWidth--;
                                  }

                                  if (neg)
                                      num = -num;
                              }

                              if (ptr)
                              {
                                  if (size == SIZE_SHORT)
                                      *(short *)ptr = num;
                                  else if (size == SIZE_LONG)
                                      *(long *)ptr = num;
                                  else
                                      *(int *)ptr = num;
                                  args++;
                              }
                              break;

                    case 's': if (fieldWidth < 0)
                                  fieldWidth = INT_MAX;

                              while (fieldWidth && (!isspace(ch)) && (ch != EOF))
                              {
                                  if (ptr)
                                  {
                                      *(char *)ptr = ch;
                                      ptr = (void *)((char *)ptr + 1);
                                  }
                                  GetCh;
                                  fieldWidth--;
                              }

                              if (ptr)
                              {
                                  *(char *)ptr = 0;
                                  args++;
                              }
                              break;

                    case '[': {
                              unsigned char mask[32];
                              unsigned char i;
                              unsigned char start;
                              unsigned int  index;
                              unsigned int  bit;
                              bool          flip;

                                  fmt++;
                                  if (*fmt == '^')
                                  {
                                      fmt++;
                                      memset(mask,1,sizeof(mask));
                                      flip = TRUE;
                                  }
                                  else
                                  {
                                      memset(mask,0,sizeof(mask));
                                      flip = FALSE;
                                  }

                                  while (*fmt)
                                  {
                                      index = (*fmt) / 8;
                                      bit   = (*fmt) % 8;

                                      if (flip)
                                          mask[index] &= ~(1 << bit);
                                      else
                                          mask[index] |= (1 << bit);

                                      start = *fmt++;

                                      if (*fmt == '-')
                                      {
                                          if ((fmt[1] != ']') && fmt[1])
                                          {
                                              fmt++;
                                              for (i = start; i <= *fmt; i++)
                                              {
                                                  index = i / 8;
                                                  bit   = i % 8;

                                                  if (flip)
                                                      mask[index] &= ~(1 << bit);
                                                  else
                                                      mask[index] |= (1 << bit);
                                              }
                                              fmt++;
                                          }
                                      }

                                      if (*fmt == ']')
                                          break;
                                  }

                                  if (fieldWidth < 0)
                                      fieldWidth = INT_MAX;

                                  while (fieldWidth && (ch != EOF))
                                  {
                                      index = ch / 8;
                                      bit   = ch % 8;
                                      if ((mask[index] & (1 << bit)) == 0)
                                          break;

                                      if (ptr)
                                      {
                                          *(char *)ptr = ch;
                                          ptr = (void *)((char *)ptr + 1);
                                      }

                                      GetCh;
                                      fieldWidth--;
                                  }

                                  if (ptr)
                                      args++;
                              }
                              break;

                    case '\0'  : break; /* unexpected end of format string */

                    default : if (ch != EOF)
                                  (*unget)(ch,userData);

                              ch = EOF;
                              break;
                }

                if (*fmt)
                    fmt++;
            }
        }
    }

    if (ch != EOF)
        (*unget)(ch,userData);

    if (args == 0)
        return EOF;

    return (args);
}

