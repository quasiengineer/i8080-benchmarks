#include <stdint.h>
#include <stdio.h>
#include "bn.h"

#ifndef N
#define N           100
#endif

#define PRECISION   10

void storeTime(uint8_t * dst) {
  *dst = *(uint8_t *)0xF880;
  *(dst + 1) = *(uint8_t *)0xF881;
  *(dst + 2) = *(uint8_t *)0xF882;
  *(dst + 3) = *(uint8_t *)0xF883;
  *(dst + 4) = *(uint8_t *)0xF884;
}

static char hex2char[16] = {
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

static printHex(uint8_t val) {
  fputc_cons(hex2char[val >> 4]);
  fputc_cons(hex2char[val & 0xF]);
}

void printTime(char * prefix, uint8_t * tm) {
  fputs(prefix, stdout);
  fputs(": ", stdout);
  printHex(*(tm + 4));
  printHex(*(tm + 3));
  printHex(*(tm + 2));
  printHex(*(tm + 1));
  printHex(*tm);
  fputs(" ticks\n", stdout);
}

// (log2(10) * decDigitsToKeep) / 8 ~ ((10/3) * decDigitsToKeep) / 8
static uint16_t wordsForIntegerForm = ((((10 * (N + PRECISION)) / 3) / 8) + 1);

// 0x0 .. 0x2FFF memory for ROM and data, 0x3000 bytes = 12Kb

// numbers for N = 10_000 occupies a bit more than 4Kb, so we can reserve 5Kb (0x1400) for each variable
// we can have 10 5Kb slots

/*
 For square root constant computation

 * 00  sqrtN | sqrtX
 * 01  sqrtN
 * 02  sqrtRoot
 * 03  tmp0
 * 04  tmp1
 * 05  tmp1
 * 06  tmp2
 * 07  tmp2
 * 08  tmp3
 * 09  tmp3
 */
static bn sqrtX = { .ptr = 0x3000 };

/*
 For denominator computation

 * 00  sqrtX
 * 01  aK
 * 02  a
 * 03  b
 * 04  aKMult
 * 05
 * 06
 * 07
 * 08
 * 09
 */
static bn aK = { .ptr = 0x4400 };
static bn a = { .ptr = 0x5800 };
static bn b = { .ptr = 0x6C00 };
static bn aKMult = { .ptr = 0x8000 };

/*
 For pi computation

 * 00  sqrtX       | tmpRecursive
 * 01  denominator | tmpRecursive
 * 02  numerator   | tmpMult
 * 03  numerator   | tmpMult
 * 04  pi
 * 05  pi
 * 06  tmpDivisor
 * 07  tmpDividend
 * 08  tmpDividend
 * 09
 */
static bn denominator = { .ptr = 0x4400 };
static bn numerator = { .ptr = 0x5800 };
static bn pi = { .ptr = 0x8000 };

// temp variables
static bn slot0 = { .ptr = 0x3000 };
static bn slot1 = { .ptr = 0x4400 };
static bn slot2 = { .ptr = 0x5800 };
static bn slot3 = { .ptr = 0x6C00 };
static bn slot4 = { .ptr = 0x8000 };
static bn slot5 = { .ptr = 0x9400 };
static bn slot6 = { .ptr = 0xA800 };
static bn slot7 = { .ptr = 0xBC00 };
static bn slot8 = { .ptr = 0xD000 };
static bn slot9 = { .ptr = 0xE400 };

// 32 bytes each (up to 78 decimal digits)
static bn coef = { .ptr = 0xF800 };
static bn small0 = { .ptr = 0xF820 };
static bn small1 = { .ptr = 0xF840 };
static bn small2 = { .ptr = 0xF860 };

void computeCoef() {
  bn_fromInt(&small0, 640320);
  bn_mul(&small1, &small0, &small0, &slot1);
  bn_mul(&small2, &small0, &small1, &slot1);
  bn_fromInt(&small0, 24);
  bn_div(&coef, &small2, &small0, &small1, &slot1, &slot2, &slot3);
}

uint8_t computeAk(uint16_t k) {
  uint32_t firstFactor = 6L * k - 5L;
  uint32_t secondFactor = 2L * k - 1L;
  bn_fromInt(&small0, firstFactor * secondFactor);
  bn_fromInt(&small1, 6L * k - 1L);
  bn_mul(&small2, &small1, &small0, &slot5);
  bn_mul(&aKMult, &small2, &aK, &slot5);

  // need to force compiler to produce u32 instead of u16
  uint32_t k32 = (uint32_t)k;
  bn_fromInt(&small0, k32 * k32 * k32);
  bn_mul(&small1, &coef, &small0, &slot5);
  if (aKMult.used > small1.used) {
    bn_div(&aK, &aKMult, &small1, &small2, &slot5, &slot6, &slot7);
    return 0;
  }

  if (small1.ptr[small1.used - 1] > aKMult.ptr[aKMult.used - 1]) {
    return 1;
  }

  if (small1.used > aKMult.used) {
    return 1;
  }

  bn_div(&aK, &aKMult, &small1, &small2, &slot5, &slot6, &slot7);
  return 0;
}

void computeDenominatorParts() {
  uint16_t k = 1;
  uint8_t isPositiveStep = 0;

  bn_powerOfDigitBase(&a, wordsForIntegerForm);
  bn_powerOfDigitBase(&aK, wordsForIntegerForm);
  bn_zero(&b);

  while (1) {
    if (computeAk(k) == 1) {
      break;
    }

    bn_fromInt(&small0, k);
    bn_mul(&aKMult, &small0, &aK, &slot5);

    if (isPositiveStep) {
      bn_add(&a, &aK);
      bn_sub(&b, &b, &aKMult);
      isPositiveStep = 0;
    } else {
      bn_sub(&a, &a, &aK);
      bn_add(&b, &aKMult);
      isPositiveStep = 1;
    }

    ++k;
  }

}

void computeDenominator() {
  computeDenominatorParts();
  bn_fromInt(&small0, 13591409);
  bn_fromInt(&small1, 545140134);
  bn_mul(&aKMult, &small1, &b, &slot5);
  bn_mul(&denominator, &small0, &a, &slot5);
  bn_sub(&denominator, &denominator, &aKMult);
}

void computeSquareRootedConstant() {
  bn_fromInt(&small0, 10005);
  bn_shiftLeftByWords(&sqrtX, &small0, wordsForIntegerForm + wordsForIntegerForm);
  bn_sqrt(&sqrtX, &slot2, &slot3, &slot4, &slot6, &slot8);
  bn_fromInt(&small0, 426880);
  bn_mul(&sqrtX, &small0, &slot2, &slot3);
}

void computePi() {
  bn_shiftLeftByWords(&numerator, &sqrtX, wordsForIntegerForm);
  bn_div(&pi, &numerator, &denominator, &slot6, &slot7, &slot0, &slot2);
}

void printPi() {
  uint16_t currentDigitIdx = pi.used - 1;
  bn_fromInt(&small0, 10);

  for (uint16_t i = (N + 1); i > 0; --i) {
    fputc_cons(pi.ptr[currentDigitIdx] + '0');
    pi.ptr[currentDigitIdx] = 0x00;
    bn_mulBy10(&pi, &slot0);
  }

  fputc_cons('\n');
}

int main() {
  uint8_t startTime[5], endTime[5];

  fputc_cons(0x05);
  storeTime(startTime);

  computeSquareRootedConstant();
  computeCoef();
  computeDenominator();
  computePi();
  printPi();

  fputc_cons(0x05);
  storeTime(endTime);

  printTime("\nStart", startTime);
  printTime("End", endTime);

  return 0;
}
