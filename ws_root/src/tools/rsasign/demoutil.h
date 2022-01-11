/* Copyright (C) RSA Data Security, Inc. created 1992.

   This file is used to demonstrate how to interface to an
   RSA Data Security, Inc. licensed development product.

   You have a royalty-free right to use, modify, reproduce and
   distribute this demonstration file (including any modified
   version), provided that you agree that RSA Data Security,
   Inc. has no warranty, implied or otherwise, or liability
   for this demonstration file or any modified version.
 */

int DemoSignData PROTO_LIST
  ((unsigned char *, unsigned int *, unsigned int, unsigned char *,
    unsigned int, B_KEY_OBJ));
int DemoSealEnvelope PROTO_LIST
  ((unsigned char *, unsigned int *, unsigned int, unsigned char *,
    unsigned int *, unsigned int, unsigned char *, unsigned int *,
    unsigned int, unsigned char *, unsigned char *, unsigned int, B_KEY_OBJ,
    B_KEY_OBJ, B_ALGORITHM_OBJ));
int DemoVerifyData PROTO_LIST
  ((unsigned char *, unsigned int, unsigned char *, unsigned int,
    B_KEY_OBJ));
int DemoOpenEnvelope PROTO_LIST
  ((unsigned char *, unsigned int *, unsigned int, unsigned char *,
    unsigned int, unsigned char *, unsigned int, unsigned char *,
    unsigned int, unsigned char *, B_KEY_OBJ, B_KEY_OBJ));
int DemoGenerateRSAKeypair PROTO_LIST
  ((B_KEY_OBJ, B_KEY_OBJ, unsigned int, B_ALGORITHM_OBJ));
