/* Copyright (C) RSA Data Security, Inc. created 1992.

   This file is used to demonstrate how to interface to an
   RSA Data Security, Inc. licensed development product.

   You have a royalty-free right to use, modify, reproduce and
   distribute this demonstration file (including any modified
   version), provided that you agree that RSA Data Security,
   Inc. has no warranty, implied or otherwise, or liability
   for this demonstration file or any modified version.
 */

#include "global.h"
#include "bsafe2.h"
#include "demochos.h"

/* This chooser selects the standard C implementations of the algorithm
     methods.
 */
B_ALGORITHM_METHOD *DEMO_ALGORITHM_CHOOSER[] = {
  &AM_RSA_ENCRYPT, &AM_RSA_DECRYPT,
  &AM_RSA_CRT_ENCRYPT, &AM_RSA_CRT_DECRYPT,
  &AM_MD2, &AM_MD5, &AM_DES_CBC_ENCRYPT, &AM_DES_CBC_DECRYPT,
  &AM_RC2_CBC_ENCRYPT, &AM_RC2_CBC_DECRYPT, &AM_RC4_ENCRYPT, &AM_RC4_DECRYPT,
  &AM_MD2_RANDOM, &AM_MD5_RANDOM, &AM_MD, &AM_DESX_CBC_ENCRYPT,
  &AM_DESX_CBC_DECRYPT, &AM_RSA_KEY_GEN, &AM_DH_PARAM_GEN,
  (B_ALGORITHM_METHOD *)NULL_PTR
};
