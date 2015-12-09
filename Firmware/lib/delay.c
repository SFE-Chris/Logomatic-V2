#include "delay.h"


//BHW TODO: This is bad and will vary depending on interrupts etc
// Short delay
void delay_ms(int count)
{
  int i;
  count *= 10000;
  for (i = 0; i < count; i++)
  {
    asm volatile ("nop");
  }
}
