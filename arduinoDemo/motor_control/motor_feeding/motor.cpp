#include "motor.h"

void motor_init(void)
{
  pinMode(valve, OUTPUT);           //继电器
  
  pinMode(push1, OUTPUT);          //TB6612控制引脚，控制电机1的方向，01为正转，10为反转
  pinMode(push2, OUTPUT);          //TB6612控制引脚，
  pinMode(pwmct, OUTPUT);         //TB6612控制引脚，电机PWM
  digitalWrite(push1, LOW);     
  digitalWrite(push2, LOW);

  ledcSetup(pwm_Channel_1, freq, resolution);  //PWM通道一开启设置  //通道0， 5KHz，10位解析度
  ledcAttachPin(PWMA, pwm_Channel_1);     //PWM通道一和引脚PWMA关联 //pin25定义为通道0的输出引脚
  ledcWrite(pwm_Channel_1, 0);        //PWM通道一占空比设置为零
  }

/**************************************************************************
函数功能：赋值给PWM寄存器 
入口参数：左轮PWM、右轮PWM
返回  值：无
**************************************************************************/
void Set_Pwm(int moto1)
{
  int Amplitude = 950;  //===PWM满幅是1024 限制在950
  
  if (moto1 > 0)
  {
   digitalWrite(push1, HIGH);     
   digitalWrite(push2, LOW);
  }//TB6612的电平控制
  else
  {
    digitalWrite(push1, LOW),       
    digitalWrite(push2, HIGH);
  }//TB6612的电平控制
  
  //功能：限制PWM赋值 
  if (moto1 < -Amplitude)  moto1 = -Amplitude;
  if (moto1 >  Amplitude)  moto1 =  Amplitude;
  
  //赋值给PWM寄存器 
  ledcWrite(pwm_Channel_1,abs(moto1));
}


//推杆电机，阀门控制
void posetive_motor(int arg)
{
  if(arg == 1)  //阀门开启，电机上推
  {
        //闭合NC,开阀门
    digitalWrite(valve,HIGH);

    //驱动器控制电机正转
    Set_Pwm(500);
  }
  else if (arg == 0) //阀门关闭，电机下移
  {
     //断开NC,关阀门
    digitalWrite(valve,LOW); 

    //驱动器控制电机反转
    Set_Pwm(-500);
  } 
  else
    delay(10);
}

//电机复位
void motor_restart(void)
{
  //闭合NC,开阀门
    digitalWrite(valve,HIGH);

    //驱动器控制电机正转
    Set_Pwm(500);
  }
