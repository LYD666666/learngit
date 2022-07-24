#ifndef __WEIGHT_H_
#define __WEIGHT_H_
#include <Arduino.h>
#include <Ticker.h>

//阀门与推杆电机驱动 （电机驱动器）
#define valve 14
#define push1 4
#define push2 13
#define pwmct 25        

#define PWMA 25  //控制电机1 PWM控制引脚
#define freq 1000      //PWM波形频率5KHZ
#define pwm_Channel_1  0 //使用PWM的通道0
#define resolution  10    //使用PWM占空比的分辨率，占空比最大可写2^10-1=1023
#define interrupt_time_control 15//定时器15ms中断控制时间

void motor_init(void);
void Set_Pwm(int moto1);
void posetive_motor(int arg);
void motor_restart(void);

#endif 
