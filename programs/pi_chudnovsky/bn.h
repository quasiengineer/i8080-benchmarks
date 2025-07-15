#ifndef __BN_H__
#define __BN_H__

#include <stdint.h>

#ifndef KARATSUBA_THRESHOLD_DIV
#define KARATSUBA_THRESHOLD_DIV           15
#endif

#ifndef KARATSUBA_THRESHOLD_MUL
#define KARATSUBA_THRESHOLD_MUL           20
#endif

typedef struct {
  uint8_t * ptr;
  uint16_t used;
} bn;

// output
void bn_printHex(bn * src);
void bn_ptr_printHex(uint8_t * ptr, uint16_t used);

// initializators
void bn_fromInt(bn * dst, uint32_t src);
void bn_fromDigits(bn * dst, uint8_t * digits, uint16_t sz);
void bn_zero(bn * dst);
void bn_powerOfDigitBase(bn * result, uint16_t power);

// operations
void bn_mul(bn * result, bn * factor1, bn * factor2, bn * tmp);
void bn_mulBy10(bn * val, bn * tmpTerm);
void bn_add(bn * result, bn * term);
void bn_sub(bn * result, bn * minuend, bn * subtrahend);
void bn_div(bn * quotient, bn * dividend, bn * divisor, bn * tmpDivisor, bn * tmpDividend, bn * tmpRecursive, bn * tmpMult);
void bn_sqrt(bn * n, bn * root, bn * tmp0, bn * tmp1, bn * tmp2, bn * tmp3);

// shift operations
void bn_shiftLeftByWords(bn * res, bn * val, uint16_t wordsToShift);
void bn_shiftLeft1bit(bn * res, bn * val);

#endif