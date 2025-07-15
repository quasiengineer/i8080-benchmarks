#ifndef __BN_H__
#define __BN_H__

#include <stdint.h>

typedef struct {
  // most significant digit index
  uint16_t msd;
  uint8_t digits[];
} bn;

// output
void bn_print(bn * src);

// initializators
void bn_fromInt(bn * dst, uint32_t src);
void bn_fromDigits(bn * dst, uint8_t * digits, uint16_t sz);
void bn_zero(bn * dst);
void bn_clone(bn * dst, bn * src);
void bn_powerOf10(bn * result, uint16_t power);

// operations
void bn_mul(bn * result, bn * factor1, bn * factor2);
void bn_mulByPowerOf10(bn * termAndResult, uint16_t power);
void bn_divByPowerOf10(bn * termAndResult, uint16_t power);
void bn_add(bn * result, bn * term1, bn * term2);
uint8_t bn_sub(bn * result, bn * minuend, bn * subtrahend); // returns 1 if minuend is smaller than subtrahend
void bn_divmod(bn * quotient, bn * reminder, bn * dividend, bn * divisor, bn * tmp);
uint8_t bn_isEqual(bn * first, bn * second); // return 1 if first is equals to second

#endif