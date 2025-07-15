#include <stdio.h>
#include <stdint.h>

#define MEM_START 0x4000
#define N 2048

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

int main()
{
  uint8_t startTime[5], endTime[5];
  uint16_t len = ((10 * N) / 3) + 1;
  uint16_t * A = (uint16_t *)MEM_START;
  uint16_t printed = 0;
  uint8_t nineCount = 0;
  uint8_t previousDigit = 2;

  fputc_cons(0x05);
  storeTime(startTime);
  for(uint16_t i = 0; i < len; i++) {
    A[i] = 2;
  }

  while (printed < N) {
    uint32_t carry = 0;

    uint16_t denominator = len - 1, numerator = (2 * len - 1), idx = 0;
    while (denominator > 0) {
      uint32_t x = ((uint32_t)A[idx]) * 10L + carry;
      carry = denominator * (x / numerator);
      A[idx] = x % numerator;
      denominator--;
      numerator -= 2;
      idx++;
    }

    uint8_t digitFromCarry = carry < 10 ? 0 : 1;
    uint8_t nextDigit = carry < 10 ? carry : ((uint8_t)carry - 10);
    if (nextDigit == 9) {
      nineCount++;
      continue;
    }

    uint8_t currentDigit = previousDigit + digitFromCarry;
    fputc_cons(currentDigit + '0');
    previousDigit = nextDigit;
    printed++;

    for (uint8_t i = 0; i < nineCount; i++) {
      fputc_cons(digitFromCarry == 0 ? '9' : '0');
      printed++;
    }
    nineCount = 0;
  }

  fputc_cons(0x05);
  storeTime(endTime);

  printTime("\nStart", startTime);
  printTime("End", endTime);

  return 0;
}
