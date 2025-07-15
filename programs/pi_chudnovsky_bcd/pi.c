#include <stdint.h>
#include <stdio.h>

#include "bn.h"

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

#define N           1000
#define PRECISION   10

// 0x0 .. 0x2FFF memory for ROM and data, 0x3000 bytes = 12Kb

// 10240 bytes each (5 slots)
static bn * denominator = (bn *)0x3000;
static bn * a = (bn *)0x3000;
static bn * sqrtNextX = (bn *)0x3000;

static bn * pi = (bn *)0x5800;
static bn * numerator = (bn *)0x5800;
static bn * aK = (bn *)0x5800;
static bn * t2 = (bn *)0x5800;

static bn * b = (bn *)0x8000;
static bn * t1 = (bn *)0x8000;

static bn * t0 = (bn *)0xA800;

static bn * sqrtX = (bn *)0xD000;
static bn * t3 = (bn *)0xD000;

// 32 bytes each
static bn * coef = (bn *)0xF800;
static bn * small0 = (bn *)0xF820;
static bn * small1 = (bn *)0xF840;
static bn * small2 = (bn *)0xF860;
// 0xF880 reserved for tick counter
static bn * small3 = (bn *)0xF8A0;

// literal constants
static uint8_t c1[6] = { 6, 4, 0, 3, 2, 0 };
static uint8_t c2[2] = { 2, 4 };
static uint8_t c3[8] = { 1, 3, 5, 9, 1, 4, 0, 9 };
static uint8_t c4[9] = { 5, 4, 5, 1, 4, 0, 1, 3, 4 };
static uint8_t c5[6] = { 4, 2, 6, 8, 8, 0 };
static uint8_t c6[5] = { 1, 0, 0, 0, 5 };
static uint8_t c7[1] = { 2 };

void computeCoef() {
  bn_fromDigits(small0, c1, sizeof(c1));
  bn_clone(small1, small0);
  bn_mul(small2, small0, small1);
  bn_mul(coef, small0, small2);
  bn_fromDigits(small2, c2, sizeof(c2));
  bn_divmod(coef, small0, coef, small2, small3);
}

void computeAk(uint16_t k) {
  bn_fromInt(small0, k);
  uint32_t firstFactor = 6L * k - 5L;
  uint32_t secondFactor = 2L * k - 1L;
  bn_fromInt(small0, firstFactor * secondFactor);
  bn_fromInt(small1, 6L * k - 1L);
  bn_mul(small2, small1, small0);

  // need to force compiler to produce u32 instead of u16
  uint32_t kSq = k * k;
  bn_fromInt(small0, kSq * k);
  bn_mul(small1, coef, small0);
  bn_mul(t0, small2, aK);
  bn_divmod(aK, small0, t0, small1, small2);
}

void computeDenominatorParts() {
  uint16_t k = 1;
  uint8_t isPositiveStep = 0;

  bn_powerOf10(a, N + PRECISION);
  bn_powerOf10(aK, N + PRECISION);
  bn_zero(b);

  while (1) {
    computeAk(k);
    if (aK->msd == 0) {
      break;
    }

    bn_fromInt(small3, k);
    bn_mul(t0, small3, aK);

    // restore aK
    bn_divByPowerOf10(aK, small3->msd);
    if (isPositiveStep) {
      bn_add(a, a, aK);
      bn_sub(b, b, t0);
      isPositiveStep = 0;
    } else {
      bn_sub(a, a, aK);
      bn_add(b, b, t0);
      isPositiveStep = 1;
    }

    k++;
  }
}

void computeDenominator() {
  computeCoef();
  computeDenominatorParts();
  bn_fromDigits(small0, c3, sizeof(c3));
  bn_fromDigits(small1, c4, sizeof(c4));
  bn_mul(t0, small1, b);
  bn_mul(t1, small0, a);
  bn_sub(denominator, t1, t0);
}

void computeSquareRootedConstant() {
  bn_fromDigits(sqrtX, c6, sizeof(c6));
  bn_mulByPowerOf10(sqrtX, N + PRECISION);

  while (1) {
    // have no free slots to keep it as variable, need to prepare each time
    bn_fromDigits(sqrtNextX, c6, sizeof(c6));
    bn_mulByPowerOf10(sqrtNextX, 2 * (N + PRECISION));

    // nextX = (10005 * 10**(2N)) / x
    bn_divmod(sqrtNextX, t0, sqrtNextX, sqrtX, t1);

    // nextX = x + ((10005 * 10**(2N)) / x)
    bn_add(sqrtNextX, sqrtNextX, sqrtX);

    // nextX = (x + ((10005 * 10**(2N)) / x)) / 2
    bn_fromDigits(small0, c7, sizeof(c7));
    bn_divmod(sqrtNextX, t0, sqrtNextX, small0, t1);

    if (bn_isEqual(sqrtNextX, sqrtX)) {
      return;
    }

    bn_clone(sqrtX, sqrtNextX);
  }
}

void computePi() {
  bn_fromDigits(small0, c5, sizeof(c5));
  bn_mul(numerator, small0, sqrtX);
  bn_mulByPowerOf10(numerator, N + PRECISION);
  bn_divmod(numerator, t3, numerator, denominator, t0);
}

int main()
{
  uint8_t startTime[5], endTime[5];

  fputc_cons(0x05);
  storeTime(startTime);

  computeSquareRootedConstant();
  computeDenominator();
  computePi();

  fputc_cons('\n');
  for (uint16_t i = pi->msd; i > PRECISION - 1; i--) {
    fputc_cons('0' + pi->digits[i]);
  }

  fputc_cons(0x05);
  storeTime(endTime);

  printTime("\nStart", startTime);
  printTime("End", endTime);

  return 0;
}
