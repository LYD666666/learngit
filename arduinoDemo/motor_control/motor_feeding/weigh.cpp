#include "weight.h"

//读取AD值
long ReadCount()
{
  digitalWrite(HX_SCk, LOW);
  while (digitalRead(HX_DT));
  unsigned long count = 0;
  for (int i = 0; i < 24; i++)
  {
    digitalWrite(HX_SCk, HIGH);
    count <<= 1;
    digitalWrite(HX_SCk, LOW);
    if (digitalRead(HX_DT))
      count |= 1;
  }
  digitalWrite(HX_SCk, HIGH);
  if (count & 0x00800000)
    count |= 0xFF000000;
  digitalWrite(HX_SCk, LOW);
  return (long)count;
}
