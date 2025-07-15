#include <stdint.h>
#include <stdio.h>

#include "bn.h"

static uint16_t bn_ptr_mul(uint8_t * resultPtr, uint8_t * factor1Ptr, uint16_t factor1Size, uint8_t * factor2Ptr, uint16_t factor2Size, uint8_t * tmpPtr);

static uint16_t bn_ptr_size(uint8_t * srcPtr, uint16_t maxSize) {
  uint16_t i = maxSize - 1;

  if (i == 0) {
    return 0;
  }

  while (!srcPtr[i]) {
    --i;
  }

  return i + 1;
}

static void bn_clamp(bn * src) {
  src->used = bn_ptr_size(src->ptr, src->used);
}

static char hex2char[16] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

void bn_ptr_printHex(uint8_t * ptr, uint16_t used) {
  uint16_t i = used - 1;
  while (1) {
    fputc_cons(hex2char[ptr[i] >> 4]);
    fputc_cons(hex2char[ptr[i] & 0xF]);

    if (i == 0) {
      break;
    }
    --i;
  }

  fputc_cons('\n');
}

void bn_printHex(bn * src) {
  bn_ptr_printHex(src->ptr, src->used);
}

void bn_zero(bn * dst) {
  dst->used = 1;
  dst->ptr[0] = 0;
}

static void bn_ptr_zero(uint8_t * ptr, uint16_t size) {
  for (uint16_t i = 0; i < size; ++i) {
    ptr[i] = 0;
  }
}

static uint8_t bn_ptr_is_zero(uint8_t * ptr, uint16_t size) {
  for (uint16_t i = 0; i < size; ++i) {
    if (ptr[i]) {
      return 0;
    }
  }

  return 1;
}

static void bn_ptr_clone(uint8_t * dstPtr, uint8_t * srcPtr, uint16_t sz) {
  for (uint16_t i = 0; i < sz; ++i) {
    dstPtr[i] = srcPtr[i];
  }
}

void bn_fromDigits(bn * dst, uint8_t * digits, uint16_t sz) {
  dst->used = sz;
  for (uint16_t i = 0; i < sz; ++i) {
    dst->ptr[i] = digits[sz - i - 1];
  }
}

void bn_fromInt(bn * dst, uint32_t src) {
  uint8_t b0 = src & 0xFF, b1 = (src >> 8) & 0xFF, b2 = (src >> 16) & 0xFF, b3 = (src >> 24) & 0xFF;

  if (b3) {
    dst->used = 4;
    dst->ptr[0] = b0;
    dst->ptr[1] = b1;
    dst->ptr[2] = b2;
    dst->ptr[3] = b3;
    return;
  }

  if (b2) {
    dst->used = 3;
    dst->ptr[0] = b0;
    dst->ptr[1] = b1;
    dst->ptr[2] = b2;
    return;
  }

  if (b1) {
    dst->used = 2;
    dst->ptr[0] = b0;
    dst->ptr[1] = b1;
    return;
  }

  dst->used = 1;
  dst->ptr[0] = b0;
}

static uint8_t bn_ptr_add(uint8_t * resultPtr, uint8_t * smallerPtr, uint16_t smallerSize, uint8_t * largerPtr, uint16_t largerSize) {
  uint8_t carry = 0;
  uint16_t i = 0;

  // sum digits, that are present in both numbers
  for (; i < smallerSize; ++i) {
    uint16_t digit = (uint16_t)smallerPtr[i] + (uint16_t)largerPtr[i] + carry;
    carry = digit > 0xFF;
    resultPtr[i] = digit & 0xFF;
  }

  // if we have carry, then we need to propagate carry
  if (carry) {
    for (; i < largerSize; ++i) {
      if (largerPtr[i] != 0xFF) {
        resultPtr[i] = largerPtr[i] + 1;
        break;
      }

      resultPtr[i] = 0;
    }

    if (i == largerSize) {
      return 1;
    }

    ++i;
  }

  // just write left-over high digits as-is
  for (; i < largerSize; ++i) {
    resultPtr[i] = largerPtr[i];
  }

  return 0;
}

void bn_add(bn * result, bn * term) {
  uint8_t * smallerPtr, * largerPtr;
  uint16_t smallerSize, largerSize;

  uint16_t resultSize = result->used;
  uint16_t termSize = term->used;

  if (resultSize > termSize) {
    smallerPtr = term->ptr;
    largerPtr = result->ptr;
    smallerSize = termSize;
    largerSize = resultSize;
  } else {
    smallerPtr = result->ptr;
    largerPtr = term->ptr;
    smallerSize = resultSize;
    largerSize = termSize;
  }

  if (bn_ptr_add(result->ptr, smallerPtr, smallerSize, largerPtr, largerSize)) {
    result->ptr[largerSize] = 1;
    result->used = largerSize + 1;
  } else {
    result->used = largerSize;
  }
}

static uint8_t bn_ptr_sub(uint8_t * resultPtr, uint8_t * minuendPtr, uint16_t minuendSize, uint8_t * subtrahendPtr, uint16_t subtrahendSize) {
  uint8_t borrow = 0;
  uint16_t i = 0;

  // subtract subtrahend from minuend
  for (; i < subtrahendSize; ++i) {
    uint16_t minuendDigit = (uint16_t)minuendPtr[i] + 0x100;
    uint16_t subtrahendDigit = (uint16_t)subtrahendPtr[i] + borrow;
    uint16_t digit = minuendDigit - subtrahendDigit;
    resultPtr[i] = digit & 0xFF;
    borrow = digit < 0x100;
  }

  // if we have borrow, then we need to propagate borrow through 9's
  if (borrow) {
    for (; i < minuendSize; ++i) {
      if (minuendPtr[i] != 0) {
        resultPtr[i] = minuendPtr[i] - 1;
        if (resultPtr != minuendPtr) {
          for (; i < minuendSize; ++i) {
            resultPtr[i] = minuendPtr[i];
          }
        }
        return 0;
      }

      resultPtr[i] = 0xFF;
    }

    return 1;
  } else {
    if (resultPtr != minuendPtr) {
      for (; i < minuendSize; ++i) {
        resultPtr[i] = minuendPtr[i];
      }
    }
    return 0;
  }
}

void bn_sub(bn * result, bn * minuend, bn * subtrahend) {
  bn_ptr_sub(result->ptr, minuend->ptr, minuend->used, subtrahend->ptr, subtrahend->used);
  result->used = minuend->used;
  bn_clamp(result);
}

static void bn_ptr_decr(uint8_t * ptr, uint16_t size) {
  uint8_t firstDigit = ptr[0] - 1;
  ptr[0] = firstDigit;

  if (firstDigit != 0xFF) {
    return;
  }

  for (uint16_t i = 1; i < size; ++i) {
    uint8_t digit = ptr[i] - 1;
    ptr[i] = digit;
    if (digit != 0xFF) {
      return;
    }
  }
}

// size of resultPtr and tmpPtr should be at least factor1Size + factor2Size
static void bn_ptr_mul_karatsuba(uint8_t * resultPtr, uint8_t * factor1Ptr, uint16_t factor1Size, uint8_t * factor2Ptr, uint16_t factor2Size, uint8_t * tmpPtr) {
  // in worst case factor1Size == factor2Size and it's odd
  // so highSize = l + 1, if max factor size is even, then highSize <= l

  uint16_t resultSize = factor1Size + factor2Size;
  uint16_t l = factor1Size > factor2Size ? (factor1Size >> 1) : (factor2Size >> 1);
  uint16_t hX = factor1Size - l;
  uint16_t hY = factor2Size - l;
  uint8_t * xL = factor1Ptr;
  uint8_t * xH = &factor1Ptr[l];
  uint8_t * yL = factor2Ptr;
  uint8_t * yH = &factor2Ptr[l];

  uint8_t sumCarried = 0;
  uint8_t * xSum = resultPtr;
  uint16_t xSumSize;
  if (hX > l) {
    xSumSize = hX;
    if (bn_ptr_add(xSum, xL, l, xH, hX)) {
      xSum[hX] = 1;
      ++xSumSize;
      sumCarried = 1;
    }
  } else {
    xSumSize = l;
    if (bn_ptr_add(xSum, xH, hX, xL, l)) {
      xSum[l] = 1;
      ++xSumSize;
      sumCarried = 1;
    }
  }

  uint8_t * ySum = tmpPtr;
  uint16_t ySumSize;
  if (hY > l) {
    ySumSize = hY;
    if (bn_ptr_add(ySum, yL, l, yH, hY)) {
      ySum[hY] = 1;
      ++ySumSize;
      sumCarried = 1;
    }
  } else {
    ySumSize = l;
    if (bn_ptr_add(ySum, yH, hY, yL, l)) {
      ySum[l] = 1;
      ++ySumSize;
      sumCarried = 1;
    }
  }

  // c = (x1 + x0) * (y1 + y0)
  uint16_t cSize = sumCarried ? (xSumSize + ySumSize - 1) : (xSumSize + ySumSize);
  uint8_t * cPtr = &resultPtr[resultSize - cSize];
  bn_ptr_mul(cPtr, xSum, xSumSize, ySum, ySumSize, &tmpPtr[ySumSize]);

  // some digits of c would be overlapped by temp variable, save them
  uint8_t overlapped[4];
  overlapped[0] = cPtr[0];
  overlapped[1] = cPtr[1];
  overlapped[2] = cPtr[2];
  overlapped[3] = cPtr[3];

  // b = x1 * y1
  uint16_t bSize = hX + hY;
  uint8_t * bPtr = tmpPtr;
  bn_ptr_mul(bPtr, xH, hX, yH, hY, resultPtr);

  // a = x0 * y0
  uint16_t aSize = l + l;
  uint8_t * aPtr = &tmpPtr[bSize];
  bn_ptr_mul(aPtr, xL, l, yL, l, resultPtr);

  // restore c digits
  cPtr[0] = overlapped[0];
  cPtr[1] = overlapped[1];
  cPtr[2] = overlapped[2];
  cPtr[3] = overlapped[3];

  // d = c - a - b
  bn_ptr_sub(cPtr, cPtr, cSize, aPtr, aSize);
  bn_ptr_sub(cPtr, cPtr, cSize, bPtr, bSize);

  // now we can construct final result
  uint8_t * dPtr = &resultPtr[l];
  bn_ptr_add(dPtr, &aPtr[l], aSize - l, cPtr, cSize);
  for (uint16_t i = 0; i < l; i++) {
    resultPtr[i] = aPtr[i];
  }
  bn_ptr_add(&resultPtr[l + l], &dPtr[l], cSize - l, bPtr, bSize);
};

static uint16_t bn_ptr_mul(uint8_t * resultPtr, uint8_t * factor1Ptr, uint16_t factor1Size, uint8_t * factor2Ptr, uint16_t factor2Size, uint8_t * tmpPtr) {
  uint16_t resultSize = factor1Size + factor2Size;

  if (factor1Size > KARATSUBA_THRESHOLD_MUL && factor2Size > KARATSUBA_THRESHOLD_MUL) {
    bn_ptr_mul_karatsuba(resultPtr, factor1Ptr, factor1Size, factor2Ptr, factor2Size, tmpPtr);
    return resultSize;
  }

  for (uint16_t i = 0; i < resultSize; ++i) {
    resultPtr[i] = 0;
  }

  for (uint16_t i = 0; i < factor1Size; ++i) {
    uint8_t carry = 0;
    uint16_t k = i;

    for (uint16_t j = 0; j < factor2Size; ++j, ++k) {
      uint16_t w = resultPtr[k] + (factor1Ptr[i] * factor2Ptr[j]) + carry;
      resultPtr[k] = w & 0xFF;
      carry = w >> 8;
    }

    resultPtr[k] = carry;
  }

  return resultSize;
}

void bn_mul(bn * result, bn * factor1, bn * factor2, bn * tmp) {
  result->used = bn_ptr_mul(result->ptr, factor1->ptr, factor1->used, factor2->ptr, factor2->used, tmp->ptr);
  bn_clamp(result);
}

static uint8_t nlz(uint8_t val) {
  uint8_t res = 0;

  if (val == 0x0) {
    return 8;
  }

  if ((val & 0xF0) == 0x0) {
    res = res + 4;
    val = val << 4;
  }

  if ((val & 0xC0) == 0x0) {
    res = res + 2;
    val = val << 2;
  }

  if ((val & 0x80) == 0x0) {
    res = res + 1;
  }

  return res;
}

static void bn_ptr_div_school(uint8_t * quotientPtr, uint8_t * dividendPtr, uint16_t dividendSize, uint8_t * divisorPtr, uint16_t divisorSize) {
  uint16_t divisorMsdIdx = divisorSize - 1;
  uint8_t divisorMsd = divisorPtr[divisorMsdIdx];
  uint16_t quotientIdx = dividendSize - divisorSize;

  uint16_t dividendTwoHighest = dividendPtr[quotientIdx + divisorMsdIdx];

  // main loop
  while (1) {
    uint16_t quotientEst = dividendTwoHighest / divisorMsd;
    uint16_t reminderEst = dividendTwoHighest - (quotientEst * divisorMsd);

    while ((quotientEst > 0xFF) || ((uint8_t)quotientEst * (uint16_t)divisorPtr[divisorMsdIdx - 1] > (reminderEst << 8) + dividendPtr[quotientIdx + divisorMsdIdx - 1])) {
      quotientEst = quotientEst - 1;
      reminderEst = reminderEst + divisorMsd;
      if (reminderEst > 0xFF) {
        break;
      }
    }

    uint16_t dividendLastDigitIdx = quotientIdx + divisorMsdIdx + 1;
    int16_t lastDigit = 0;
    if (quotientEst) {
      uint8_t carry = 0;
      for (uint16_t i = 0; i <= divisorMsdIdx; ++i) {
        uint16_t p = (uint8_t)quotientEst * (uint16_t)divisorPtr[i];
        uint16_t t = dividendPtr[quotientIdx + i] - carry - (p & 0xFF);
        dividendPtr[quotientIdx + i] = t;
        carry = (uint8_t)(p >> 8) - (uint8_t)(t >> 8);
      }

      if (dividendLastDigitIdx != dividendSize) {
        lastDigit = dividendPtr[dividendLastDigitIdx] - carry;
        dividendPtr[dividendLastDigitIdx] = lastDigit;
      } else if (carry) {
        lastDigit = -1;
      }
    } else if (dividendLastDigitIdx != dividendSize) {
      dividendPtr[dividendLastDigitIdx] = 0x00;
    }

    if (lastDigit < 0) {
      quotientEst = quotientEst - 1;
      uint8_t carry = 0;
      for (uint16_t i = 0; i <= divisorMsdIdx; ++i) {
        uint16_t t = (uint16_t)dividendPtr[quotientIdx + i] + divisorPtr[i] + carry;
        dividendPtr[quotientIdx + i] = t;
        carry = (t >> 8);
      }
      dividendPtr[quotientIdx + divisorMsdIdx + 1] += carry;
    }

    quotientPtr[quotientIdx] = quotientEst;

    if (quotientIdx == 0x0) {
      break;
    }
    --quotientIdx;

    // need to handle first iteration separately (outside loop), when high word is 0
    dividendTwoHighest = (dividendPtr[quotientIdx + divisorMsdIdx + 1] << 8) + dividendPtr[quotientIdx + divisorMsdIdx];
  }
}

uint8_t bn_ptr_isEqual(uint8_t * firstPtr, uint8_t * secondPtr, uint16_t size) {
  for (uint16_t i = 0; i < size; ++i) {
    if (firstPtr[i] != secondPtr[i]) {
      return 0;
    }
  }

  return 1;
}

// val = (val << 3) + (val << 1)
void bn_mulBy10(bn * val, bn * tmpTerm) {
  uint16_t msd = val->used - 1;
  uint16_t msdPlus1 = msd + 1;
  uint16_t msdPlus2 = msdPlus1 + 1;

  // highest digit for terms
  uint8_t shiftedHighest = val->ptr[msd] >> 5;
  if (shiftedHighest) {
    tmpTerm->used = msdPlus2;
    tmpTerm->ptr[msdPlus1] = shiftedHighest;
    shiftedHighest = shiftedHighest >> 2;
    if (shiftedHighest) {
      val->used = msdPlus2;
      val->ptr[msdPlus1] = shiftedHighest;
    } else {
      val->used = msdPlus1;
    }
  } else {
    tmpTerm->used = msdPlus1;
    val->used = msdPlus1;
  }

  for (uint16_t i = msd; i > 0; --i) {
    uint8_t shiftedCurrentDigitBy1 = val->ptr[i] << 1;
    uint8_t shiftedPrevDigitBy3 = val->ptr[i - 1] >> 5;

    val->ptr[i] = shiftedCurrentDigitBy1 | (shiftedPrevDigitBy3 >> 2);
    tmpTerm->ptr[i] = (shiftedCurrentDigitBy1 << 2) | shiftedPrevDigitBy3;
  }

  uint8_t shiftedLowest = val->ptr[0] << 1;
  val->ptr[0] = shiftedLowest;
  tmpTerm->ptr[0] = shiftedLowest << 2;

  bn_add(val, tmpTerm);
}

void bn_powerOfDigitBase(bn * result, uint16_t power) {
  for (uint16_t i = 0; i < power; ++i) {
    result->ptr[i] = 0;
  }

  result->ptr[power] = 1;
  result->used = power + 1;
}

void bn_shiftLeftByWords(bn * res, bn * val, uint16_t wordsToShift) {
  uint16_t size = val->used;
  uint16_t dstDigitIdx = 0;

  for (; dstDigitIdx < wordsToShift; ++dstDigitIdx) {
    res->ptr[dstDigitIdx] = 0;
  }

  for (uint16_t srcDigitIdx = 0; srcDigitIdx < size; ++srcDigitIdx, ++dstDigitIdx) {
    res->ptr[dstDigitIdx] = val->ptr[srcDigitIdx];
  }

  res->used = dstDigitIdx;
}

void bn_shiftLeft1bit(bn * res, bn * val) {
  uint16_t size = val->used;
  uint8_t currDigit = 0;

  for (uint16_t i = 0; i < size; ++i) {
    uint8_t srcDigit = val->ptr[i];
    res->ptr[i] = (srcDigit << 1) | currDigit;
    currDigit = srcDigit >> 7;
  }

  if (currDigit != 0) {
    res->ptr[size - 1] = 1;
    res->used = size + 1;
  }
}

static uint16_t bn_ptr_shiftLeftByBits(uint8_t * resPtr, uint8_t * valPtr, uint16_t valSize, uint8_t bitsToShift) {
  uint8_t bitsToShiftRight = 8 - bitsToShift;
  uint16_t msd = valSize - 1;
  uint16_t dstDigitIdx = 0;
  uint8_t currDigit = 0;

  for (uint16_t srcDigitIdx = 0; srcDigitIdx <= msd; ++srcDigitIdx, ++dstDigitIdx) {
    uint8_t srcDigit = valPtr[srcDigitIdx];
    resPtr[dstDigitIdx] = (srcDigit << bitsToShift) | currDigit;
    currDigit = srcDigit >> bitsToShiftRight;
  }

  if (currDigit != 0) {
    resPtr[dstDigitIdx] = currDigit;
    return dstDigitIdx + 1;
  } else {
    resPtr[dstDigitIdx] = 0;
    return dstDigitIdx;
  }
}

static uint16_t bn_ptr_shiftRightByBits(uint8_t * resPtr, uint8_t * valPtr, uint16_t valSize, uint8_t bitsToShift) {
  uint16_t i = 0;
  uint16_t msd = valSize - 1;
  uint8_t bitsToShiftLeft = 8 - bitsToShift;

  for (; i < msd; ++i) {
    resPtr[i] = (valPtr[i] >> bitsToShift) | (valPtr[i + 1] << bitsToShiftLeft);
  }

  uint8_t highestDigit = valPtr[i] >> bitsToShift;
  if (highestDigit) {
    resPtr[i] = highestDigit;
    return valSize;
  }

  resPtr[i] = 0;
  return msd;
}

static void bn_ptr_shiftRight1bit(uint8_t * resPtr, uint8_t * valPtr, uint16_t valSize) {
  uint16_t i = 0;
  uint16_t msd = valSize - 1;

  for (; i < msd; ++i) {
    resPtr[i] = (valPtr[i] >> 1) | (valPtr[i + 1] << 7);
  }

  uint8_t highestDigit = valPtr[i] >> 1;
  if (highestDigit) {
    resPtr[i] = highestDigit;
    return 1;
  }

  return 0;
}

static void bn_ptr_div_recursive(uint8_t * quotientPtr, uint8_t * dividendPtr, uint16_t dividendSize, uint8_t * divisorPtr, uint16_t divisorSize, uint8_t * tmpPtr, uint8_t * tmpMult) {
  uint16_t m = dividendSize - divisorSize;
  uint16_t k = m >> 1;
  uint16_t k2 = k << 1;

  if (m < KARATSUBA_THRESHOLD_DIV) {
    bn_ptr_div_school(quotientPtr, dividendPtr, dividendSize, divisorPtr, divisorSize);
    return;
  }

  uint8_t * quotientL = quotientPtr;                                    // size = k
  uint8_t * quotientH = &quotientPtr[k];
  uint16_t quotientHSize = m - k + 1;
  uint8_t * divisorL = divisorPtr;                                      // size = k
  uint8_t * divisorH = &divisorPtr[k];
  uint16_t divisorHSize = divisorSize - k;
  uint8_t * dividendIIOO = &dividendPtr[k2];                            // size = dividendSize - 2 * k
  uint8_t * dividendIIIO = &dividendPtr[k];
  uint16_t dividendIIIOSize = dividendSize - k;

  // [quotientH, dividendIIOO] = dividendIIOO / divisorH
  bn_ptr_div_recursive(quotientH, dividendIIOO, dividendSize - k2, divisorH, divisorHSize, tmpPtr, tmpMult);

  bn_ptr_mul(tmpPtr, quotientH, quotientHSize, divisorL, k, tmpMult);
  if (bn_ptr_sub(dividendIIIO, dividendIIIO, dividendIIIOSize, tmpPtr, m + 1)) {
    do {
      bn_ptr_decr(quotientH, quotientHSize);
    } while (!bn_ptr_add(dividendIIIO, divisorPtr, divisorSize, dividendIIIO, dividendIIIOSize));
  }

  if (bn_ptr_is_zero(dividendIIIO, dividendIIIOSize)) {
    bn_ptr_zero(quotientL, k);
    return;
  }

  uint8_t lowestQuotientHDigit = quotientH[0];
  // [quotientL, dividendOOIO] = dividendOOIO / divisorH
  bn_ptr_div_recursive(quotientL, dividendIIIO, divisorSize, divisorH, divisorHSize, tmpPtr, tmpMult);

  uint16_t quotientLSize = quotientL[k] ? k + 1 : k;
  bn_ptr_mul(tmpPtr, quotientL, quotientLSize, divisorL, k, tmpMult);
  if (bn_ptr_sub(dividendPtr, dividendPtr, dividendSize, tmpPtr, quotientLSize + k)) {
    do {
      bn_ptr_decr(quotientL, quotientLSize);
    } while (!bn_ptr_add(dividendPtr, divisorPtr, divisorSize, dividendPtr, dividendSize));
  }

  // if quotientL intersects with quotientH
  uint16_t quotientDigit = lowestQuotientHDigit + quotientL[k];
  uint16_t quotientDigitIdx = k;
  do {
    quotientPtr[quotientDigitIdx] = quotientDigit & 0xFF;
    if (quotientDigit <= 0xFF) {
      break;
    }

    quotientDigitIdx++;
    quotientDigit = quotientPtr[quotientDigitIdx] + 1;
  } while (1);
}

uint16_t bn_ptr_div(uint8_t * quotientPtr, uint8_t * dividendPtr, uint16_t dividendSize, uint8_t * divisorPtr, uint16_t divisorSize, uint8_t * tmpDivisor, uint8_t * tmpDividend, uint8_t * tmpRecursive, uint8_t * tmpMult, uint8_t fixReminder) {
  uint8_t shift = nlz(divisorPtr[divisorSize - 1]);
  uint16_t adjDividendSize;
  uint16_t adjDivisorSize;

  if (shift) {
    adjDivisorSize = bn_ptr_shiftLeftByBits(tmpDivisor, divisorPtr, divisorSize, shift);
    adjDividendSize = bn_ptr_shiftLeftByBits(tmpDividend, dividendPtr, dividendSize, shift);
  } else {
    adjDivisorSize = divisorSize;
    adjDividendSize = dividendSize;

    if (tmpDivisor != divisorPtr) {
      bn_ptr_clone(tmpDivisor, divisorPtr, divisorSize);
    }

    if (tmpDividend != dividendPtr) {
      bn_ptr_clone(tmpDividend, dividendPtr, dividendSize);
    }
  }

  uint16_t m = adjDividendSize - adjDivisorSize;
  if (adjDivisorSize < KARATSUBA_THRESHOLD_DIV || m > adjDivisorSize) {
    bn_ptr_div_school(quotientPtr, tmpDividend, adjDividendSize, tmpDivisor, adjDivisorSize);
  } else {
    bn_ptr_div_recursive(quotientPtr, tmpDividend, adjDividendSize, tmpDivisor, adjDivisorSize, tmpRecursive, tmpMult);
  }

  if (fixReminder && shift != 0) {
    bn_ptr_shiftRightByBits(tmpDividend, tmpDividend, adjDivisorSize, shift);
  }

  return m;
}

void bn_div(bn * quotient, bn * dividend, bn * divisor, bn * tmpDivisor, bn * tmpDividend, bn * tmpRecursive, bn * tmpMult) {
  uint16_t m = bn_ptr_div(quotient->ptr, dividend->ptr, dividend->used, divisor->ptr, divisor->used, tmpDivisor->ptr, tmpDividend->ptr, tmpRecursive->ptr, tmpMult->ptr, 0);

  if (quotient->ptr[m]) {
    quotient->used = m + 1;
  } else {
    quotient->used = m;
  }
}

static uint16_t bn_sqrt_newton(uint8_t * nPtr, uint16_t nSize, uint8_t * rootPtr, uint8_t * reminderPtr, uint8_t * tmp) {
  uint16_t xSize = nSize;
  uint8_t * nextX = &tmp[0x500];

  bn_ptr_shiftRight1bit(rootPtr, nPtr, nSize);
  while (1) {
    // nextX = (x + (extendedN / x)) / 2
    uint16_t nextXSize = nSize - xSize + 1;
    bn_ptr_div(nextX, nPtr, nSize, rootPtr, xSize, &tmp[0x100], &tmp[0x200], &tmp[0x300], &tmp[0x400], 1);

    if (nextXSize > xSize) {
      if (bn_ptr_add(nextX, rootPtr, xSize, nextX, nextXSize)) {
        nextX[nextXSize] = 1;
        ++nextXSize;
      }
    } else {
      if (bn_ptr_add(nextX, nextX, nextXSize, rootPtr, xSize)) {
        nextX[xSize] = 1;
        nextXSize = xSize + 1;
      } else {
        nextXSize = xSize;
      }
    }

    if (!bn_ptr_shiftRight1bit(nextX, nextX, nextXSize)) {
      --nextXSize;
    }

    if (nextXSize == xSize && bn_ptr_isEqual(nextX, rootPtr, xSize)) {
      bn_ptr_mul(tmp, rootPtr, xSize, rootPtr, xSize, &tmp[0x100]);
      bn_ptr_sub(reminderPtr, nPtr, nSize, tmp, xSize + xSize);
      return bn_ptr_size(reminderPtr, nSize);
    }

    bn_ptr_clone(rootPtr, nextX, nextXSize);
    xSize = nextXSize;
  }
}

static uint16_t bn_sqrt_recursive(uint8_t * nPtr, uint16_t nSize, uint8_t * rootPtr, uint8_t * reminderPtr, uint8_t * tmp0, uint8_t * tmp1, uint8_t * tmp2, uint8_t * tmp3) {
  if (nSize <= 4) {
    return bn_sqrt_newton(nPtr, nSize, rootPtr, reminderPtr, tmp0);
  }

  uint16_t k = nSize >> 2;
  uint8_t * aL0 = nPtr;
  uint8_t * aL1 = &nPtr[k];
  uint8_t * aH = &aL1[k];

  // [s1, r1] = [tmp3 << k, a2] = sqrt([a3a2])
  uint8_t * s1 = &rootPtr[k];
  uint8_t * r1 = aH;
  uint16_t aHSize = nSize - k - k;
  uint16_t sqrtReminderSize = bn_sqrt_recursive(aH, aHSize, s1, r1, tmp0, tmp1, tmp2, tmp3);

  // [q, u] = [rootPtr, aL1] = [r1a1] / (2 * s1)
  uint16_t s1Size = aHSize >> 1;
  uint16_t divisorSize = bn_ptr_shiftLeftByBits(tmp2, s1, s1Size, 1);
  uint16_t dividendSize = sqrtReminderSize + k;
  uint8_t lowestS1 = s1[0];

  bn_ptr_div(rootPtr, aL1, dividendSize, tmp2, divisorSize, tmp0, aL1, tmp1, tmp3, 1);

  s1[0] = lowestS1;
  // root is computed
  uint16_t quotientSize = dividendSize - divisorSize + 1;
  if (quotientSize > k) {
    --quotientSize;
  } else {
    for (uint16_t i = quotientSize; i < k; ++i) {
      rootPtr[i] = 0;
    }
  }

  // compute reminder
  uint16_t reminderSize = divisorSize + k;
  bn_ptr_mul(tmp1, rootPtr, quotientSize, rootPtr, quotientSize, tmp2);
  if (bn_ptr_sub(reminderPtr, aL0, reminderSize, tmp1, quotientSize + quotientSize)) {
    uint16_t correctionSize = bn_ptr_shiftLeftByBits(tmp2, rootPtr, s1Size + k, 1);
    bn_ptr_add(reminderPtr, tmp2, correctionSize, reminderPtr, reminderSize);
    bn_ptr_decr(reminderPtr, reminderSize);
    bn_ptr_decr(rootPtr, s1Size + k);
  }

  return bn_ptr_size(reminderPtr, reminderSize);
};

void bn_sqrt(bn * n, bn * root, bn * tmp0, bn * tmp1, bn * tmp2, bn * tmp3) {
  uint16_t nSize = n->used;

  uint8_t shift = nlz(n->ptr[nSize - 1]);
  if (shift > 1) {
    bn_ptr_shiftLeftByBits(n->ptr, n->ptr, nSize, shift);
  }

  bn_sqrt_recursive(n->ptr, nSize, root->ptr, tmp3->ptr, tmp0->ptr, tmp1->ptr, tmp2->ptr, tmp3->ptr);
  root->used = nSize >> 1;

  if (shift > 1) {
    root->used = bn_ptr_shiftRightByBits(root->ptr, root->ptr, root->used, shift >> 1);
  }
};