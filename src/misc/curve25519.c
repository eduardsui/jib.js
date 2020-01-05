/* Copyright 2008, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * curve25519-donna: Curve25519 elliptic curve, public key function
 *
 * http://code.google.com/p/curve25519-donna/
 *
 * Adam Langley <agl@imperialviolet.org>
 *
 * Derived from public domain C code by Daniel J. Bernstein <djb@cr.yp.to>
 *
 * More information about curve25519 can be found here
 *   http://cr.yp.to/ecdh.html
 *
 * djb's sample implementation of curve25519 is written in a special assembly
 * language called qhasm and uses the floating point registers.
 *
 * This is, almost, a clean room reimplementation from the curve25519 paper. It
 * uses many of the tricks described therein. Only the crecip function is taken
 * from the sample implementation.
 */

#include <stdint.h>

/* Field element representation:
 *
 * Field elements are written as an array of signed, 64-bit limbs, least
 * significant first. The value of the field element is:
 *   x[0] + 2^26·x[1] + x^51·x[2] + 2^102·x[3] + ...
 *
 * i.e. the limbs are 26, 25, 26, 25, ... bits wide.
 */

/* Sum two numbers: output += in */
static void fsum(int64_t *output, const int64_t *in) {
  unsigned i;
  for (i = 0; i < 10; i += 2) {
    output[0+i] = (output[0+i] + in[0+i]);
    output[1+i] = (output[1+i] + in[1+i]);
  }
}

/* Find the difference of two numbers: output = in - output
 * (note the order of the arguments!)
 */
static void fdifference(int64_t *output, const int64_t *in) {
  unsigned i;
  for (i = 0; i < 10; ++i) {
    output[i] = (in[i] - output[i]);
  }
}

/* Multiply a number by a scalar: output = in * scalar */
static void fscalar_product(int64_t *output, const int64_t *in, const int64_t scalar) {
  unsigned i;
  for (i = 0; i < 10; ++i) {
    output[i] = in[i] * scalar;
  }
}

/* Multiply two numbers: output = in2 * in
 *
 * output must be distinct to both inputs. The inputs are reduced coefficient
 * form, the output is not.
 */
static void fproduct(int64_t *output, const int64_t *in2, const int64_t *in) {
  output[0] =       ((int64_t) ((int32_t) in2[0])) * ((int32_t) in[0]);
  output[1] =       ((int64_t) ((int32_t) in2[0])) * ((int32_t) in[1]) +
                    ((int64_t) ((int32_t) in2[1])) * ((int32_t) in[0]);
  output[2] =  2 *  ((int64_t) ((int32_t) in2[1])) * ((int32_t) in[1]) +
                    ((int64_t) ((int32_t) in2[0])) * ((int32_t) in[2]) +
                    ((int64_t) ((int32_t) in2[2])) * ((int32_t) in[0]);
  output[3] =       ((int64_t) ((int32_t) in2[1])) * ((int32_t) in[2]) +
                    ((int64_t) ((int32_t) in2[2])) * ((int32_t) in[1]) +
                    ((int64_t) ((int32_t) in2[0])) * ((int32_t) in[3]) +
                    ((int64_t) ((int32_t) in2[3])) * ((int32_t) in[0]);
  output[4] =       ((int64_t) ((int32_t) in2[2])) * ((int32_t) in[2]) +
               2 * (((int64_t) ((int32_t) in2[1])) * ((int32_t) in[3]) +
                    ((int64_t) ((int32_t) in2[3])) * ((int32_t) in[1])) +
                    ((int64_t) ((int32_t) in2[0])) * ((int32_t) in[4]) +
                    ((int64_t) ((int32_t) in2[4])) * ((int32_t) in[0]);
  output[5] =       ((int64_t) ((int32_t) in2[2])) * ((int32_t) in[3]) +
                    ((int64_t) ((int32_t) in2[3])) * ((int32_t) in[2]) +
                    ((int64_t) ((int32_t) in2[1])) * ((int32_t) in[4]) +
                    ((int64_t) ((int32_t) in2[4])) * ((int32_t) in[1]) +
                    ((int64_t) ((int32_t) in2[0])) * ((int32_t) in[5]) +
                    ((int64_t) ((int32_t) in2[5])) * ((int32_t) in[0]);
  output[6] =  2 * (((int64_t) ((int32_t) in2[3])) * ((int32_t) in[3]) +
                    ((int64_t) ((int32_t) in2[1])) * ((int32_t) in[5]) +
                    ((int64_t) ((int32_t) in2[5])) * ((int32_t) in[1])) +
                    ((int64_t) ((int32_t) in2[2])) * ((int32_t) in[4]) +
                    ((int64_t) ((int32_t) in2[4])) * ((int32_t) in[2]) +
                    ((int64_t) ((int32_t) in2[0])) * ((int32_t) in[6]) +
                    ((int64_t) ((int32_t) in2[6])) * ((int32_t) in[0]);
  output[7] =       ((int64_t) ((int32_t) in2[3])) * ((int32_t) in[4]) +
                    ((int64_t) ((int32_t) in2[4])) * ((int32_t) in[3]) +
                    ((int64_t) ((int32_t) in2[2])) * ((int32_t) in[5]) +
                    ((int64_t) ((int32_t) in2[5])) * ((int32_t) in[2]) +
                    ((int64_t) ((int32_t) in2[1])) * ((int32_t) in[6]) +
                    ((int64_t) ((int32_t) in2[6])) * ((int32_t) in[1]) +
                    ((int64_t) ((int32_t) in2[0])) * ((int32_t) in[7]) +
                    ((int64_t) ((int32_t) in2[7])) * ((int32_t) in[0]);
  output[8] =       ((int64_t) ((int32_t) in2[4])) * ((int32_t) in[4]) +
               2 * (((int64_t) ((int32_t) in2[3])) * ((int32_t) in[5]) +
                    ((int64_t) ((int32_t) in2[5])) * ((int32_t) in[3]) +
                    ((int64_t) ((int32_t) in2[1])) * ((int32_t) in[7]) +
                    ((int64_t) ((int32_t) in2[7])) * ((int32_t) in[1])) +
                    ((int64_t) ((int32_t) in2[2])) * ((int32_t) in[6]) +
                    ((int64_t) ((int32_t) in2[6])) * ((int32_t) in[2]) +
                    ((int64_t) ((int32_t) in2[0])) * ((int32_t) in[8]) +
                    ((int64_t) ((int32_t) in2[8])) * ((int32_t) in[0]);
  output[9] =       ((int64_t) ((int32_t) in2[4])) * ((int32_t) in[5]) +
                    ((int64_t) ((int32_t) in2[5])) * ((int32_t) in[4]) +
                    ((int64_t) ((int32_t) in2[3])) * ((int32_t) in[6]) +
                    ((int64_t) ((int32_t) in2[6])) * ((int32_t) in[3]) +
                    ((int64_t) ((int32_t) in2[2])) * ((int32_t) in[7]) +
                    ((int64_t) ((int32_t) in2[7])) * ((int32_t) in[2]) +
                    ((int64_t) ((int32_t) in2[1])) * ((int32_t) in[8]) +
                    ((int64_t) ((int32_t) in2[8])) * ((int32_t) in[1]) +
                    ((int64_t) ((int32_t) in2[0])) * ((int32_t) in[9]) +
                    ((int64_t) ((int32_t) in2[9])) * ((int32_t) in[0]);
  output[10] = 2 * (((int64_t) ((int32_t) in2[5])) * ((int32_t) in[5]) +
                    ((int64_t) ((int32_t) in2[3])) * ((int32_t) in[7]) +
                    ((int64_t) ((int32_t) in2[7])) * ((int32_t) in[3]) +
                    ((int64_t) ((int32_t) in2[1])) * ((int32_t) in[9]) +
                    ((int64_t) ((int32_t) in2[9])) * ((int32_t) in[1])) +
                    ((int64_t) ((int32_t) in2[4])) * ((int32_t) in[6]) +
                    ((int64_t) ((int32_t) in2[6])) * ((int32_t) in[4]) +
                    ((int64_t) ((int32_t) in2[2])) * ((int32_t) in[8]) +
                    ((int64_t) ((int32_t) in2[8])) * ((int32_t) in[2]);
  output[11] =      ((int64_t) ((int32_t) in2[5])) * ((int32_t) in[6]) +
                    ((int64_t) ((int32_t) in2[6])) * ((int32_t) in[5]) +
                    ((int64_t) ((int32_t) in2[4])) * ((int32_t) in[7]) +
                    ((int64_t) ((int32_t) in2[7])) * ((int32_t) in[4]) +
                    ((int64_t) ((int32_t) in2[3])) * ((int32_t) in[8]) +
                    ((int64_t) ((int32_t) in2[8])) * ((int32_t) in[3]) +
                    ((int64_t) ((int32_t) in2[2])) * ((int32_t) in[9]) +
                    ((int64_t) ((int32_t) in2[9])) * ((int32_t) in[2]);
  output[12] =      ((int64_t) ((int32_t) in2[6])) * ((int32_t) in[6]) +
               2 * (((int64_t) ((int32_t) in2[5])) * ((int32_t) in[7]) +
                    ((int64_t) ((int32_t) in2[7])) * ((int32_t) in[5]) +
                    ((int64_t) ((int32_t) in2[3])) * ((int32_t) in[9]) +
                    ((int64_t) ((int32_t) in2[9])) * ((int32_t) in[3])) +
                    ((int64_t) ((int32_t) in2[4])) * ((int32_t) in[8]) +
                    ((int64_t) ((int32_t) in2[8])) * ((int32_t) in[4]);
  output[13] =      ((int64_t) ((int32_t) in2[6])) * ((int32_t) in[7]) +
                    ((int64_t) ((int32_t) in2[7])) * ((int32_t) in[6]) +
                    ((int64_t) ((int32_t) in2[5])) * ((int32_t) in[8]) +
                    ((int64_t) ((int32_t) in2[8])) * ((int32_t) in[5]) +
                    ((int64_t) ((int32_t) in2[4])) * ((int32_t) in[9]) +
                    ((int64_t) ((int32_t) in2[9])) * ((int32_t) in[4]);
  output[14] = 2 * (((int64_t) ((int32_t) in2[7])) * ((int32_t) in[7]) +
                    ((int64_t) ((int32_t) in2[5])) * ((int32_t) in[9]) +
                    ((int64_t) ((int32_t) in2[9])) * ((int32_t) in[5])) +
                    ((int64_t) ((int32_t) in2[6])) * ((int32_t) in[8]) +
                    ((int64_t) ((int32_t) in2[8])) * ((int32_t) in[6]);
  output[15] =      ((int64_t) ((int32_t) in2[7])) * ((int32_t) in[8]) +
                    ((int64_t) ((int32_t) in2[8])) * ((int32_t) in[7]) +
                    ((int64_t) ((int32_t) in2[6])) * ((int32_t) in[9]) +
                    ((int64_t) ((int32_t) in2[9])) * ((int32_t) in[6]);
  output[16] =      ((int64_t) ((int32_t) in2[8])) * ((int32_t) in[8]) +
               2 * (((int64_t) ((int32_t) in2[7])) * ((int32_t) in[9]) +
                    ((int64_t) ((int32_t) in2[9])) * ((int32_t) in[7]));
  output[17] =      ((int64_t) ((int32_t) in2[8])) * ((int32_t) in[9]) +
                    ((int64_t) ((int32_t) in2[9])) * ((int32_t) in[8]);
  output[18] = 2 *  ((int64_t) ((int32_t) in2[9])) * ((int32_t) in[9]);
}

/* Reduce a long form to a short form by taking the input mod 2^255 - 19. */
static void freduce_degree(int64_t *output) {
  /* Each of these shifts and adds ends up multiplying the value by 19. */
  output[8] += output[18] << 4;
  output[8] += output[18] << 1;
  output[8] += output[18];
  output[7] += output[17] << 4;
  output[7] += output[17] << 1;
  output[7] += output[17];
  output[6] += output[16] << 4;
  output[6] += output[16] << 1;
  output[6] += output[16];
  output[5] += output[15] << 4;
  output[5] += output[15] << 1;
  output[5] += output[15];
  output[4] += output[14] << 4;
  output[4] += output[14] << 1;
  output[4] += output[14];
  output[3] += output[13] << 4;
  output[3] += output[13] << 1;
  output[3] += output[13];
  output[2] += output[12] << 4;
  output[2] += output[12] << 1;
  output[2] += output[12];
  output[1] += output[11] << 4;
  output[1] += output[11] << 1;
  output[1] += output[11];
  output[0] += output[10] << 4;
  output[0] += output[10] << 1;
  output[0] += output[10];
}


/* return v / 2^26, using only shifts and adds. */
static inline int64_t div_by_2_26(const int64_t v) {
  /* High word of v; no shift needed*/
  const uint32_t highword = (uint32_t)(((uint64_t) v) >> 32);
  /* Set to all 1s if v was negative; else set to 0s. */
  const int32_t sign = ((int32_t) highword) >> 31;
  /* Set to 0x3ffffff if v was negative; else set to 0. */
  const int32_t roundoff = ((uint32_t) sign) >> 6;
  /* Should return v / (1<<26) */
  return (v + roundoff) >> 26;
}

/* return v / (2^25), using only shifts and adds. */
static inline int64_t div_by_2_25(const int64_t v) {
  /* High word of v; no shift needed*/
  const uint32_t highword = (uint32_t)(((uint64_t) v) >> 32);
  /* Set to all 1s if v was negative; else set to 0s. */
  const int32_t sign = ((int32_t) highword) >> 31;
  /* Set to 0x1ffffff if v was negative; else set to 0. */
  const int32_t roundoff = ((uint32_t) sign) >> 7;
  /* Should return v / (1<<25) */
  return (v + roundoff) >> 25;
}

static inline int32_t div_s32_by_2_25(const int32_t v) {
   const int32_t roundoff = ((uint32_t)(v >> 31)) >> 7;
   return (v + roundoff) >> 25;
}

/* Reduce all coefficients of the short form input so that |x| < 2^26.
 *
 * On entry: |output[i]| < 2^62
 */
static void freduce_coefficients(int64_t *output) {
  unsigned i;

  output[10] = 0;

  for (i = 0; i < 10; i += 2) {
    int64_t over = div_by_2_26(output[i]);
    output[i] -= over << 26;
    output[i+1] += over;

    over = div_by_2_25(output[i+1]);
    output[i+1] -= over << 25;
    output[i+2] += over;
  }
  /* Now |output[10]| < 2 ^ 38 and all other coefficients are reduced. */
  output[0] += output[10] << 4;
  output[0] += output[10] << 1;
  output[0] += output[10];

  output[10] = 0;

  /* Now output[1..9] are reduced, and |output[0]| < 2^26 + 19 * 2^38
   * So |over| will be no more than 77825  */
  {
    int64_t over = div_by_2_26(output[0]);
    output[0] -= over << 26;
    output[1] += over;
  }

  /* Now output[0,2..9] are reduced, and |output[1]| < 2^25 + 77825
   * So |over| will be no more than 1. */
  {
    /* output[1] fits in 32 bits, so we can use div_s32_by_2_25 here. */
    int32_t over32 = div_s32_by_2_25((int32_t)output[1]);
    output[1] -= over32 << 25;
    output[2] += over32;
  }

  /* Finally, output[0,1,3..9] are reduced, and output[2] is "nearly reduced":
   * we have |output[2]| <= 2^26.  This is good enough for all of our math,
   * but it will require an extra freduce_coefficients before fcontract. */
}

/* A helpful wrapper around fproduct: output = in * in2.
 *
 * output must be distinct to both inputs. The output is reduced degree and
 * reduced coefficient.
 */
static void fmul(int64_t *output, const int64_t *in, const int64_t *in2) {
  int64_t t[19];
  fproduct(t, in, in2);
  freduce_degree(t);
  freduce_coefficients(t);
  memcpy(output, t, sizeof(int64_t) * 10);
}

static void fsquare_inner(int64_t *output, const int64_t *in) {
  output[0] =       ((int64_t) ((int32_t) in[0])) * ((int32_t) in[0]);
  output[1] =  2 *  ((int64_t) ((int32_t) in[0])) * ((int32_t) in[1]);
  output[2] =  2 * (((int64_t) ((int32_t) in[1])) * ((int32_t) in[1]) +
                    ((int64_t) ((int32_t) in[0])) * ((int32_t) in[2]));
  output[3] =  2 * (((int64_t) ((int32_t) in[1])) * ((int32_t) in[2]) +
                    ((int64_t) ((int32_t) in[0])) * ((int32_t) in[3]));
  output[4] =       ((int64_t) ((int32_t) in[2])) * ((int32_t) in[2]) +
               4 *  ((int64_t) ((int32_t) in[1])) * ((int32_t) in[3]) +
               2 *  ((int64_t) ((int32_t) in[0])) * ((int32_t) in[4]);
  output[5] =  2 * (((int64_t) ((int32_t) in[2])) * ((int32_t) in[3]) +
                    ((int64_t) ((int32_t) in[1])) * ((int32_t) in[4]) +
                    ((int64_t) ((int32_t) in[0])) * ((int32_t) in[5]));
  output[6] =  2 * (((int64_t) ((int32_t) in[3])) * ((int32_t) in[3]) +
                    ((int64_t) ((int32_t) in[2])) * ((int32_t) in[4]) +
                    ((int64_t) ((int32_t) in[0])) * ((int32_t) in[6]) +
               2 *  ((int64_t) ((int32_t) in[1])) * ((int32_t) in[5]));
  output[7] =  2 * (((int64_t) ((int32_t) in[3])) * ((int32_t) in[4]) +
                    ((int64_t) ((int32_t) in[2])) * ((int32_t) in[5]) +
                    ((int64_t) ((int32_t) in[1])) * ((int32_t) in[6]) +
                    ((int64_t) ((int32_t) in[0])) * ((int32_t) in[7]));
  output[8] =       ((int64_t) ((int32_t) in[4])) * ((int32_t) in[4]) +
               2 * (((int64_t) ((int32_t) in[2])) * ((int32_t) in[6]) +
                    ((int64_t) ((int32_t) in[0])) * ((int32_t) in[8]) +
               2 * (((int64_t) ((int32_t) in[1])) * ((int32_t) in[7]) +
                    ((int64_t) ((int32_t) in[3])) * ((int32_t) in[5])));
  output[9] =  2 * (((int64_t) ((int32_t) in[4])) * ((int32_t) in[5]) +
                    ((int64_t) ((int32_t) in[3])) * ((int32_t) in[6]) +
                    ((int64_t) ((int32_t) in[2])) * ((int32_t) in[7]) +
                    ((int64_t) ((int32_t) in[1])) * ((int32_t) in[8]) +
                    ((int64_t) ((int32_t) in[0])) * ((int32_t) in[9]));
  output[10] = 2 * (((int64_t) ((int32_t) in[5])) * ((int32_t) in[5]) +
                    ((int64_t) ((int32_t) in[4])) * ((int32_t) in[6]) +
                    ((int64_t) ((int32_t) in[2])) * ((int32_t) in[8]) +
               2 * (((int64_t) ((int32_t) in[3])) * ((int32_t) in[7]) +
                    ((int64_t) ((int32_t) in[1])) * ((int32_t) in[9])));
  output[11] = 2 * (((int64_t) ((int32_t) in[5])) * ((int32_t) in[6]) +
                    ((int64_t) ((int32_t) in[4])) * ((int32_t) in[7]) +
                    ((int64_t) ((int32_t) in[3])) * ((int32_t) in[8]) +
                    ((int64_t) ((int32_t) in[2])) * ((int32_t) in[9]));
  output[12] =      ((int64_t) ((int32_t) in[6])) * ((int32_t) in[6]) +
               2 * (((int64_t) ((int32_t) in[4])) * ((int32_t) in[8]) +
               2 * (((int64_t) ((int32_t) in[5])) * ((int32_t) in[7]) +
                    ((int64_t) ((int32_t) in[3])) * ((int32_t) in[9])));
  output[13] = 2 * (((int64_t) ((int32_t) in[6])) * ((int32_t) in[7]) +
                    ((int64_t) ((int32_t) in[5])) * ((int32_t) in[8]) +
                    ((int64_t) ((int32_t) in[4])) * ((int32_t) in[9]));
  output[14] = 2 * (((int64_t) ((int32_t) in[7])) * ((int32_t) in[7]) +
                    ((int64_t) ((int32_t) in[6])) * ((int32_t) in[8]) +
               2 *  ((int64_t) ((int32_t) in[5])) * ((int32_t) in[9]));
  output[15] = 2 * (((int64_t) ((int32_t) in[7])) * ((int32_t) in[8]) +
                    ((int64_t) ((int32_t) in[6])) * ((int32_t) in[9]));
  output[16] =      ((int64_t) ((int32_t) in[8])) * ((int32_t) in[8]) +
               4 *  ((int64_t) ((int32_t) in[7])) * ((int32_t) in[9]);
  output[17] = 2 *  ((int64_t) ((int32_t) in[8])) * ((int32_t) in[9]);
  output[18] = 2 *  ((int64_t) ((int32_t) in[9])) * ((int32_t) in[9]);
}

static void fsquare(int64_t *output, const int64_t *in) {
  int64_t t[19];
  fsquare_inner(t, in);
  freduce_degree(t);
  freduce_coefficients(t);
  memcpy(output, t, sizeof(int64_t) * 10);
}

/* Take a little-endian, 32-byte number and expand it into polynomial form */
static void fexpand(int64_t *output, const uint8_t *input) {
  output[0] = ((((int64_t) input[0 + 0]) | ((int64_t) input[0 + 1]) << 8 | ((int64_t) input[0 + 2]) << 16 | ((int64_t) input[0 + 3]) << 24) >> 0) & 0x3ffffff;;
  output[1] = ((((int64_t) input[3 + 0]) | ((int64_t) input[3 + 1]) << 8 | ((int64_t) input[3 + 2]) << 16 | ((int64_t) input[3 + 3]) << 24) >> 2) & 0x1ffffff;;
  output[2] = ((((int64_t) input[6 + 0]) | ((int64_t) input[6 + 1]) << 8 | ((int64_t) input[6 + 2]) << 16 | ((int64_t) input[6 + 3]) << 24) >> 3) & 0x3ffffff;;
  output[3] = ((((int64_t) input[9 + 0]) | ((int64_t) input[9 + 1]) << 8 | ((int64_t) input[9 + 2]) << 16 | ((int64_t) input[9 + 3]) << 24) >> 5) & 0x1ffffff;;
  output[4] = ((((int64_t) input[12 + 0]) | ((int64_t) input[12 + 1]) << 8 | ((int64_t) input[12 + 2]) << 16 | ((int64_t) input[12 + 3]) << 24) >> 6) & 0x3ffffff;;
  output[5] = ((((int64_t) input[16 + 0]) | ((int64_t) input[16 + 1]) << 8 | ((int64_t) input[16 + 2]) << 16 | ((int64_t) input[16 + 3]) << 24) >> 0) & 0x1ffffff;;
  output[6] = ((((int64_t) input[19 + 0]) | ((int64_t) input[19 + 1]) << 8 | ((int64_t) input[19 + 2]) << 16 | ((int64_t) input[19 + 3]) << 24) >> 1) & 0x3ffffff;;
  output[7] = ((((int64_t) input[22 + 0]) | ((int64_t) input[22 + 1]) << 8 | ((int64_t) input[22 + 2]) << 16 | ((int64_t) input[22 + 3]) << 24) >> 3) & 0x1ffffff;;
  output[8] = ((((int64_t) input[25 + 0]) | ((int64_t) input[25 + 1]) << 8 | ((int64_t) input[25 + 2]) << 16 | ((int64_t) input[25 + 3]) << 24) >> 4) & 0x3ffffff;;
  output[9] = ((((int64_t) input[28 + 0]) | ((int64_t) input[28 + 1]) << 8 | ((int64_t) input[28 + 2]) << 16 | ((int64_t) input[28 + 3]) << 24) >> 6) & 0x1ffffff;;
}

/* Take a fully reduced polynomial form number and contract it into a
 * little-endian, 32-byte array
 */
static void fcontract(uint8_t *output, int64_t *input) {
  int i;
  int j;

  for (j = 0; j < 2; ++j) {
    for (i = 0; i < 9; ++i) {
      if ((i & 1) == 1) {
        /* This calculation is a time-invariant way to make input[i] positive
           by borrowing from the next-larger int64_t.
        */
        const int32_t mask = (int32_t)(input[i]) >> 31;
        const int32_t carry = -(((int32_t)(input[i]) & mask) >> 25);
        input[i] = (int32_t)(input[i]) + (carry << 25);
        input[i+1] = (int32_t)(input[i+1]) - carry;
      } else {
        const int32_t mask = (int32_t)(input[i]) >> 31;
        const int32_t carry = -(((int32_t)(input[i]) & mask) >> 26);
        input[i] = (int32_t)(input[i]) + (carry << 26);
        input[i+1] = (int32_t)(input[i+1]) - carry;
      }
    }
    const int32_t mask = (int32_t)(input[9]) >> 31;
    const int32_t carry = -(((int32_t)(input[9]) & mask) >> 25);
    input[9] = (int32_t)(input[9]) + (carry << 25);
    input[0] = (int32_t)(input[0]) - (carry * 19);
  }

  /* The first borrow-propagation pass above ended with every int64_t
     except (possibly) input[0] non-negative.

     Since each input int64_t except input[0] is decreased by at most 1
     by a borrow-propagation pass, the second borrow-propagation pass
     could only have wrapped around to decrease input[0] again if the
     first pass left input[0] negative *and* input[1] through input[9]
     were all zero.  In that case, input[1] is now 2^25 - 1, and this
     last borrow-propagation step will leave input[1] non-negative.
  */
  const int32_t mask = (int32_t)(input[0]) >> 31;
  const int32_t carry = -(((int32_t)(input[0]) & mask) >> 26);
  input[0] = (int32_t)(input[0]) + (carry << 26);
  input[1] = (int32_t)(input[1]) - carry;

  /* Both passes through the above loop, plus the last 0-to-1 step, are
     necessary: if input[9] is -1 and input[0] through input[8] are 0,
     negative values will remain in the array until the end.
   */

  input[1] <<= 2;
  input[2] <<= 3;
  input[3] <<= 5;
  input[4] <<= 6;
  input[6] <<= 1;
  input[7] <<= 3;
  input[8] <<= 4;
  input[9] <<= 6;
  output[0] = 0;
  output[16] = 0;
  output[0+0] |= input[0] & 0xff; output[0+1] = (input[0] >> 8) & 0xff; output[0+2] = (input[0] >> 16) & 0xff; output[0+3] = (input[0] >> 24) & 0xff;;
  output[3+0] |= input[1] & 0xff; output[3+1] = (input[1] >> 8) & 0xff; output[3+2] = (input[1] >> 16) & 0xff; output[3+3] = (input[1] >> 24) & 0xff;;
  output[6+0] |= input[2] & 0xff; output[6+1] = (input[2] >> 8) & 0xff; output[6+2] = (input[2] >> 16) & 0xff; output[6+3] = (input[2] >> 24) & 0xff;;
  output[9+0] |= input[3] & 0xff; output[9+1] = (input[3] >> 8) & 0xff; output[9+2] = (input[3] >> 16) & 0xff; output[9+3] = (input[3] >> 24) & 0xff;;
  output[12+0] |= input[4] & 0xff; output[12+1] = (input[4] >> 8) & 0xff; output[12+2] = (input[4] >> 16) & 0xff; output[12+3] = (input[4] >> 24) & 0xff;;
  output[16+0] |= input[5] & 0xff; output[16+1] = (input[5] >> 8) & 0xff; output[16+2] = (input[5] >> 16) & 0xff; output[16+3] = (input[5] >> 24) & 0xff;;
  output[19+0] |= input[6] & 0xff; output[19+1] = (input[6] >> 8) & 0xff; output[19+2] = (input[6] >> 16) & 0xff; output[19+3] = (input[6] >> 24) & 0xff;;
  output[22+0] |= input[7] & 0xff; output[22+1] = (input[7] >> 8) & 0xff; output[22+2] = (input[7] >> 16) & 0xff; output[22+3] = (input[7] >> 24) & 0xff;;
  output[25+0] |= input[8] & 0xff; output[25+1] = (input[8] >> 8) & 0xff; output[25+2] = (input[8] >> 16) & 0xff; output[25+3] = (input[8] >> 24) & 0xff;;
  output[28+0] |= input[9] & 0xff; output[28+1] = (input[9] >> 8) & 0xff; output[28+2] = (input[9] >> 16) & 0xff; output[28+3] = (input[9] >> 24) & 0xff;;
}

/* Input: Q, Q', Q-Q'
 * Output: 2Q, Q+Q'
 *
 *   x2 z3: long form
 *   x3 z3: long form
 *   x z: short form, destroyed
 *   xprime zprime: short form, destroyed
 *   qmqp: short form, preserved
 */
static void fmonty(int64_t *x2, int64_t *z2,  /* output 2Q */
                   int64_t *x3, int64_t *z3,  /* output Q + Q' */
                   int64_t *x, int64_t *z,    /* input Q */
                   int64_t *xprime, int64_t *zprime,  /* input Q' */
                   const int64_t *qmqp /* input Q - Q' */) {
  int64_t origx[10], origxprime[10], zzz[19], xx[19], zz[19], xxprime[19],
        zzprime[19], zzzprime[19], xxxprime[19];

  memcpy(origx, x, 10 * sizeof(int64_t));
  fsum(x, z);
  fdifference(z, origx);  // does x - z

  memcpy(origxprime, xprime, sizeof(int64_t) * 10);
  fsum(xprime, zprime);
  fdifference(zprime, origxprime);
  fproduct(xxprime, xprime, z);
  fproduct(zzprime, x, zprime);
  freduce_degree(xxprime);
  freduce_coefficients(xxprime);
  freduce_degree(zzprime);
  freduce_coefficients(zzprime);
  memcpy(origxprime, xxprime, sizeof(int64_t) * 10);
  fsum(xxprime, zzprime);
  fdifference(zzprime, origxprime);
  fsquare(xxxprime, xxprime);
  fsquare(zzzprime, zzprime);
  fproduct(zzprime, zzzprime, qmqp);
  freduce_degree(zzprime);
  freduce_coefficients(zzprime);
  memcpy(x3, xxxprime, sizeof(int64_t) * 10);
  memcpy(z3, zzprime, sizeof(int64_t) * 10);

  fsquare(xx, x);
  fsquare(zz, z);
  fproduct(x2, xx, zz);
  freduce_degree(x2);
  freduce_coefficients(x2);
  fdifference(zz, xx);  // does zz = xx - zz
  memset(zzz + 10, 0, sizeof(int64_t) * 9);
  fscalar_product(zzz, zz, 121665);
  /* No need to call freduce_degree here:
     fscalar_product doesn't increase the degree of its input. */
  freduce_coefficients(zzz);
  fsum(zzz, xx);
  fproduct(z2, zz, zzz);
  freduce_degree(z2);
  freduce_coefficients(z2);
}

/* Conditionally swap two reduced-form int64_t arrays if 'iswap' is 1, but leave
 * them unchanged if 'iswap' is 0.  Runs in data-invariant time to avoid
 * side-channel attacks.
 *
 * NOTE that this function requires that 'iswap' be 1 or 0; other values give
 * wrong results.  Also, the two int64_t arrays must be in reduced-coefficient,
 * reduced-degree form: the values in a[10..19] or b[10..19] aren't swapped,
 * and all all values in a[0..9],b[0..9] must have magnitude less than
 * INT32_MAX.
 */
static void swap_conditional(int64_t a[19], int64_t b[19], int64_t iswap) {
  unsigned i;
  const int32_t swap = (int32_t)-iswap;

  for (i = 0; i < 10; ++i) {
    const int32_t x = swap & ( ((int32_t)a[i]) ^ ((int32_t)b[i]) );
    a[i] = ((int32_t)a[i]) ^ x;
    b[i] = ((int32_t)b[i]) ^ x;
  }
}

/* Calculates nQ where Q is the x-coordinate of a point on the curve
 *
 *   resultx/resultz: the x coordinate of the resulting curve point (short form)
 *   n: a little endian, 32-byte number
 *   q: a point of the curve (short form)
 */
static void cmult(int64_t *resultx, int64_t *resultz, const uint8_t *n, const int64_t *q) {
  int64_t a[19] = {0}, b[19] = {1}, c[19] = {1}, d[19] = {0};
  int64_t *nqpqx = a, *nqpqz = b, *nqx = c, *nqz = d, *t;
  int64_t e[19] = {0}, f[19] = {1}, g[19] = {0}, h[19] = {1};
  int64_t *nqpqx2 = e, *nqpqz2 = f, *nqx2 = g, *nqz2 = h;

  unsigned i, j;

  memcpy(nqpqx, q, sizeof(int64_t) * 10);

  for (i = 0; i < 32; ++i) {
    uint8_t byte = n[31 - i];
    for (j = 0; j < 8; ++j) {
      const int64_t bit = byte >> 7;

      swap_conditional(nqx, nqpqx, bit);
      swap_conditional(nqz, nqpqz, bit);
      fmonty(nqx2, nqz2,
             nqpqx2, nqpqz2,
             nqx, nqz,
             nqpqx, nqpqz,
             q);
      swap_conditional(nqx2, nqpqx2, bit);
      swap_conditional(nqz2, nqpqz2, bit);

      t = nqx;
      nqx = nqx2;
      nqx2 = t;
      t = nqz;
      nqz = nqz2;
      nqz2 = t;
      t = nqpqx;
      nqpqx = nqpqx2;
      nqpqx2 = t;
      t = nqpqz;
      nqpqz = nqpqz2;
      nqpqz2 = t;

      byte <<= 1;
    }
  }

  memcpy(resultx, nqx, sizeof(int64_t) * 10);
  memcpy(resultz, nqz, sizeof(int64_t) * 10);
}

// -----------------------------------------------------------------------------
// Shamelessly copied from djb's code
// -----------------------------------------------------------------------------
static void crecip(int64_t *out, const int64_t *z) {
  int64_t z2[10];
  int64_t z9[10];
  int64_t z11[10];
  int64_t z2_5_0[10];
  int64_t z2_10_0[10];
  int64_t z2_20_0[10];
  int64_t z2_50_0[10];
  int64_t z2_100_0[10];
  int64_t t0[10];
  int64_t t1[10];
  int i;

  /* 2 */ fsquare(z2,z);
  /* 4 */ fsquare(t1,z2);
  /* 8 */ fsquare(t0,t1);
  /* 9 */ fmul(z9,t0,z);
  /* 11 */ fmul(z11,z9,z2);
  /* 22 */ fsquare(t0,z11);
  /* 2^5 - 2^0 = 31 */ fmul(z2_5_0,t0,z9);

  /* 2^6 - 2^1 */ fsquare(t0,z2_5_0);
  /* 2^7 - 2^2 */ fsquare(t1,t0);
  /* 2^8 - 2^3 */ fsquare(t0,t1);
  /* 2^9 - 2^4 */ fsquare(t1,t0);
  /* 2^10 - 2^5 */ fsquare(t0,t1);
  /* 2^10 - 2^0 */ fmul(z2_10_0,t0,z2_5_0);

  /* 2^11 - 2^1 */ fsquare(t0,z2_10_0);
  /* 2^12 - 2^2 */ fsquare(t1,t0);
  /* 2^20 - 2^10 */ for (i = 2;i < 10;i += 2) { fsquare(t0,t1); fsquare(t1,t0); }
  /* 2^20 - 2^0 */ fmul(z2_20_0,t1,z2_10_0);

  /* 2^21 - 2^1 */ fsquare(t0,z2_20_0);
  /* 2^22 - 2^2 */ fsquare(t1,t0);
  /* 2^40 - 2^20 */ for (i = 2;i < 20;i += 2) { fsquare(t0,t1); fsquare(t1,t0); }
  /* 2^40 - 2^0 */ fmul(t0,t1,z2_20_0);

  /* 2^41 - 2^1 */ fsquare(t1,t0);
  /* 2^42 - 2^2 */ fsquare(t0,t1);
  /* 2^50 - 2^10 */ for (i = 2;i < 10;i += 2) { fsquare(t1,t0); fsquare(t0,t1); }
  /* 2^50 - 2^0 */ fmul(z2_50_0,t0,z2_10_0);

  /* 2^51 - 2^1 */ fsquare(t0,z2_50_0);
  /* 2^52 - 2^2 */ fsquare(t1,t0);
  /* 2^100 - 2^50 */ for (i = 2;i < 50;i += 2) { fsquare(t0,t1); fsquare(t1,t0); }
  /* 2^100 - 2^0 */ fmul(z2_100_0,t1,z2_50_0);

  /* 2^101 - 2^1 */ fsquare(t1,z2_100_0);
  /* 2^102 - 2^2 */ fsquare(t0,t1);
  /* 2^200 - 2^100 */ for (i = 2;i < 100;i += 2) { fsquare(t1,t0); fsquare(t0,t1); }
  /* 2^200 - 2^0 */ fmul(t1,t0,z2_100_0);

  /* 2^201 - 2^1 */ fsquare(t0,t1);
  /* 2^202 - 2^2 */ fsquare(t1,t0);
  /* 2^250 - 2^50 */ for (i = 2;i < 50;i += 2) { fsquare(t0,t1); fsquare(t1,t0); }
  /* 2^250 - 2^0 */ fmul(t0,t1,z2_50_0);

  /* 2^251 - 2^1 */ fsquare(t1,t0);
  /* 2^252 - 2^2 */ fsquare(t0,t1);
  /* 2^253 - 2^3 */ fsquare(t1,t0);
  /* 2^254 - 2^4 */ fsquare(t0,t1);
  /* 2^255 - 2^5 */ fsquare(t1,t0);
  /* 2^255 - 21 */ fmul(out,t1,z11);
}

static const unsigned char kCurve25519BasePoint[ 32 ] = { 9 };

void curve25519(uint8_t *mypublic, const uint8_t *secret, const uint8_t *basepoint) {
  int64_t bp[10], x[10], z[11], zmone[10];
  uint8_t e[32];
  int i;

  if (basepoint == 0) basepoint = kCurve25519BasePoint;

  for (i = 0; i < 32; ++i) e[i] = secret[i];
  e[0] &= 248;
  e[31] &= 127;
  e[31] |= 64;

  fexpand(bp, basepoint);
  cmult(x, z, e, bp);
  crecip(zmone, z);
  fmul(z, x, zmone);
  freduce_coefficients(z);
  fcontract(mypublic, z);
}
