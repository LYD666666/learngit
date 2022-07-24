#ifndef __WEIGHT_H_
#define __WEIGHT_H_
#include <Arduino.h>

// ADC（注意使用了wifi的话，ADC2的所有通道都不能用）故使用ADC1 GPIO34
#define HX_DT 34
#define HX_SCk 3

long ReadCount(void);

#endif 
