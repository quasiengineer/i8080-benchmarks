#include <stdint.h>
#include <stdio.h>

#include "bn.h"

void bn_print(bn * src) {
  for (uint16_t i = src->msd; i > 0; i--) {
    fputc_cons('0' + src->digits[i]);
  }

  fputc_cons('0' + src->digits[0]);
  fputc_cons('\n');
}

void bn_zero(bn * dst) {
  dst->digits[0] = 0;
  dst->msd = 0;
}

void bn_clone(bn * dst, bn * src) {
  dst->msd = src->msd;
  for (uint16_t i = 0; i <= dst->msd; i++) {
    dst->digits[i] = src->digits[i];
  }
}

void bn_fromInt(bn * dst, uint32_t src) {
  uint8_t msd = 0;
  // XXX: for some reason zcc uses l_div_u instead of l_long_div_u if constant is specified (src % 10UL)
  uint8_t base = 10;

  while (src > 0) {
    dst->digits[msd++] = src % base;
    src = src / base;
  }

  dst->msd = msd - 1;
}

void bn_fromDigits(bn * dst, uint8_t * digits, uint16_t sz) {
  dst->msd = sz - 1;
  for (uint16_t i = 0; i < sz; i++) {
    dst->digits[i] = digits[sz - i - 1];
  }
}

void bn_powerOf10(bn * result, uint16_t power) {
  for (uint16_t i = 0; i < power - 1; i++) {
    result->digits[i] = 0;
  }

  result->digits[power] = 1;
  result->msd = power;
}

void bn_add(bn * result, bn * term1, bn * term2) {
  // swap numbers to make sure that src1 always has largest amount of digits
  if (term2->msd > term1->msd) {
    bn * tmp = term1;
    term1 = term2;
    term2 = tmp;
  }

  uint8_t carry = 0;
  uint16_t i = 0;

  // sum digits, that are present in both numbers
  for (; i <= term2->msd; i++) {
    uint8_t digit = term1->digits[i] + term2->digits[i] + carry;
    if (digit >= 10) {
      result->digits[i] = digit - 10;
      carry = 1;
    } else {
      result->digits[i] = digit;
      carry = 0;
    }
  }

  // if we have carry, then we need to propagate carry through 9's
  if (carry) {
    for (; i <= term1->msd; i++) {
      if (term1->digits[i] != 9) {
        result->digits[i] = term1->digits[i] + 1;
        break;
      }

      result->digits[i] = 0;
    }

    if (i > term1->msd) {
      result->digits[i] = 1;
    }

    i++;
  }

  // just write left-over high digits as-is
  for (; i <= term1->msd; i++) {
    result->digits[i] = term1->digits[i];
  }

  result->msd = i - 1;
}

uint8_t bn_sub(bn * result, bn * minuend, bn * subtrahend) {
  if (minuend->msd < subtrahend->msd) {
    return 1;
  }

  uint8_t borrow = 0;
  uint16_t i = 0;

  // subtract subtrahend from minuend
  for (; i <= subtrahend->msd; i++) {
    int8_t digit = minuend->digits[i] - subtrahend->digits[i] - borrow;
    if (digit < 0) {
      result->digits[i] = digit + 10;
      borrow = 1;
    } else {
      result->digits[i] = digit;
      borrow = 0;
    }
  }

  // if we have borrow, then we need to propagate borrow through 9's
  if (borrow) {
    for (; i <= minuend->msd; i++) {
      if (minuend->digits[i] != 0) {
        result->digits[i] = minuend->digits[i] - 1;
        break;
      }

      result->digits[i] = 9;
    }

    if (i > minuend->msd) {
      return 1;
    }

    i++;
  }

  // just write left-over high digits as-is
  for (; i <= minuend->msd; i++) {
    result->digits[i] = minuend->digits[i];
  }

  // skip leading 0s
  for (i--; result->digits[i] == 0 && i > 0; i--);
  result->msd = i;

  return 0;
}

void bn_mulByPowerOf10(bn * termAndResult, uint16_t power) {
  for (uint16_t i = termAndResult->msd; i > 0; i--) {
    termAndResult->digits[i + power] = termAndResult->digits[i];
  }

  termAndResult->digits[power] = termAndResult->digits[0];

  for (uint16_t i = 0; i < power; i++) {
    termAndResult->digits[i] = 0;
  }

  termAndResult->msd += power;
}

void bn_divByPowerOf10(bn * termAndResult, uint16_t power) {
  for (uint16_t i = power; i <= termAndResult->msd; i++) {
    termAndResult->digits[i - power] = termAndResult->digits[i];
  }

  termAndResult->msd -= power;
}

// Because we are iterating through factor1, need to try to keep factor1 lower than factor2
//
// factor2 is modified, and backed storage should fit sum of lengths of original factors
//   e.g. for 123 x 45 multiplication, factor2 should be able to fit 5 digits
//
// result also should be able to fit same amount of digits
void bn_mul(bn * result, bn * factor1, bn * factor2) {
  bn_zero(result);

  uint16_t i = 0;
  while (1) {
    for (uint8_t j = 1; j <= factor1->digits[i]; j++) {
      bn_add(result, result, factor2);
    }

    if (++i > factor1->msd) {
      break;
    }

    bn_mulByPowerOf10(factor2, 1);
  }
}

// reminder and tmp should be able to store at least same amount of digits as divisor has
//
// dividend and quotient could point to the same big number cell
void bn_divmod(bn * quotient, bn * reminder, bn * dividend, bn * divisor, bn * tmp) {
  bn * currReminder = reminder;
  bn * tmpReminder = tmp;
  bn * swap;

  bn_zero(reminder);
  bn_zero(tmp);

  uint16_t i = dividend->msd;
  while (1) {
    bn_mulByPowerOf10(currReminder, 1);
    currReminder->digits[0] = dividend->digits[i];

    uint8_t quotientDigit = 0;
    while (1) {
      uint8_t borrow = bn_sub(tmpReminder, currReminder, divisor);
      if (borrow) {
        break;
      }

      quotientDigit++;

      // swap pointers, instead of swapping values
      swap = currReminder;
      currReminder = tmpReminder;
      tmpReminder = swap;
    }
    quotient->digits[i] = quotientDigit;

    // don't want to deal with signed numbers here
    if (i == 0) {
      break;
    }

    i--;
  }

  // skip leading 0s for quotient
  for (i = dividend->msd; quotient->digits[i] == 0 && i > 0; i--);
  quotient->msd = i;

  if (reminder != currReminder) {
    bn_clone(reminder, currReminder);
  }

  // skip leading 0s for reminder
  for (i = divisor->msd; reminder->digits[i] == 0 && i > 0; i--);
  reminder->msd = i;
}

uint8_t bn_isEqual(bn * first, bn * second) {
  if (first->msd != second->msd) {
    return 0;
  }

  for (uint16_t i = 0; i < first->msd; i++) {
    if (first->digits[i] != second->digits[i]) {
      return 0;
    }
  }

  return 1;
}