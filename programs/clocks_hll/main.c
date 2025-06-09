#include <stdio.h>
#include <stdint.h>

void storeTime(uint8_t * dst) {
   *dst = *(uint8_t *)0xF880;
   *(dst + 1) = *(uint8_t *)0xF881;
   *(dst + 2) = *(uint8_t *)0xF882;
   *(dst + 3) = *(uint8_t *)0xF883;
   *(dst + 4) = *(uint8_t *)0xF884;
}

void printTime(char * prefix, uint8_t * tm) {
   printf("%s: %02X%02X%02X%02X%02X ticks\n", prefix, *(tm + 4), *(tm + 3), *(tm + 2), *(tm + 1), *tm);
}

int main()
{
   uint8_t startTime[5], endTime[5];

   storeTime(startTime);
   printf("Hello World !\n");
   storeTime(endTime);

   printTime("Start", startTime);
   printTime("End", endTime);
   return 0;
}
