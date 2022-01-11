/* Table of stab names for the BFD library.
   Copyright (C) 1990, 91, 92, 93, 94, 1995 Free Software Foundation, Inc.
   Written by Cygnus Support.

This file is part of BFD, the Binary File Descriptor library.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "bfd.h"

#define ARCH_SIZE 32		/* Value doesn't matter. */
#include "libaout.h"
#include "aout/aout64.h"

/* Create a table of debugging stab-codes and corresponding names.  */

#define __define_stab(NAME, CODE, STRING) __define_name(CODE, STRING)

/* These are not really stab symbols, but it is
   convenient to have them here for the sake of nm.
   For completeness, we could also add N_TEXT etc, but those
   are never needed, since nm treats those specially. */
#define EXTRA_SYMBOLS \
  __define_name (N_SETA, "SETA")/* Absolute set element symbol */ \
  __define_name (N_SETT, "SETT")/* Text set element symbol */ \
  __define_name (N_SETD, "SETD")/* Data set element symbol */ \
  __define_name (N_SETB, "SETB")/* Bss set element symbol */ \
  __define_name (N_SETV, "SETV")/* Pointer to set vector in data area. */ \
  __define_name (N_INDR, "INDR") \
  __define_name (N_WARNING, "WARNING")

CONST char *
aout_stab_name (code)
     int code;
{
#if 0 /* This lookup table is slower than lots of explicit tests, at
	 least on the i386.  One advantage is that the compiler can
	 eliminate duplicates from the code, whereas they can't easily
	 be eliminated from the lookup table.  */

#define __define_name(CODE, STRING) {(int)CODE, STRING},
  static const struct {
    int code;
    char string[7];
  } aout_stab_names[] = {
#include "aout/stab.def"
    EXTRA_SYMBOLS
  };
  register int i = sizeof (aout_stab_names) / sizeof (aout_stab_names[0]);
  while (--i >= 0)
    if (aout_stab_names[i].code == code)
      return aout_stab_names[i].string;

#else

#define __define_name(val, str) if (val == code) return str ;
#include "aout/stab.def"
  EXTRA_SYMBOLS

#endif

  return 0;
}
