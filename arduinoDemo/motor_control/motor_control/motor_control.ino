#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Ticker.h>
#include <stdio.h>

//驱动模块控制信号
#define IN1 14  //控制电机1的方向A，01为正转，10为反转 
#define IN2 4  //控制电机1的方向B，01为正转，10为反转

#define PWMA 25  //控制电机1 PWM控制引脚
#define freq 1000      //PWM波形频率5KHZ
#define pwm_Channel_1  0 //使用PWM的通道0
#define resolution  10    //使用PWM占空比的分辨率，占空比最大可写2^10-1=1023
#define interrupt_time_control 15//定时器15ms中断控制时间

// WiFi
const char *ssid = "STOP607_2.4";         // Enter your WiFi name      STOP607_2.4  iQOO3
const char *password = "sigmaWYU3601";    // Enter WiFi password       sigmaWYU3601 asdfghjkl

// MQTT Broker
const char *mqtt_broker = "106.14.145.57";
const char *topic = "/feeding";
const char *topic1 = "feeding1";
const char *mqtt_username = "cwl";
const char *mqtt_password = "19260817";
const int mqtt_port = 1883;

//json格式转化所需数据
char string1[10];
char msgJson[75];                       //要发送的json格式的数据
char msg_buf[200];                      //发送信息缓冲区
char dataMotor[] = "{\"motorfeed\":%ld}"; //信息模板
unsigned short json_len = 4;            // json长度

//全局变量电机
long motor;

WiFiClient espClient;           //创建一个WIFI连接客户端
PubSubClient client(espClient); // 创建一个PubSub客户端, 传入创建的WIFI客户端

void Set_Pwm(int moto1);
void callback(char *topic, byte *payload, unsigned int length);

//wifi\MQTT连接
void MQTT_init(void){
  // connecting to a WiFi network
  WiFi.begin(ssid, password);
  while (!WiFi.isConnected())
  {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");

  // connecting to a mqtt broker
  client.setServer(mqtt_broker, mqtt_port);   //MQTT服务器连接函数（服务器IP，端口号）
  client.setCallback(callback);               //设定回调方式，当ESP32收到订阅消息时会调用此方法
  while (!client.connected())
  {
    String client_id = "90d6856e-515d-49a7-b049-ce017cc3ff79";
    client_id += String(WiFi.macAddress());
    Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password))
    {
      Serial.println("Public emqx mqtt broker connected");
    }
    else
    {
      Serial.print("failed with state ");
      Serial.print(client.state());
      vTaskDelay(100/portTICK_PERIOD_MS); //需要更改，最后用RTOS
    }
  }

  client.subscribe(topic);                 //连接MQTT服务器后订阅主题
}

//硬件初始化
void HarewardInit(void){
  // Set software serial baud to 115200;
  Serial.begin(115200); //串口
  
  pinMode(IN1, OUTPUT);          //TB6612控制引脚，控制电机1的方向，01为正转，10为反转
  pinMode(IN2, OUTPUT);          //TB6612控制引脚，
  pinMode(PWMA, OUTPUT);         //TB6612控制引脚，电机PWM
  digitalWrite(IN1, LOW);          //TB6612控制引脚拉低
  digitalWrite(IN2, LOW);          //TB6612控制引脚拉低

  ledcSetup(pwm_Channel_1, freq, resolution);  //PWM通道一开启设置  //通道0， 5KHz，10位解析度
  ledcAttachPin(PWMA, pwm_Channel_1);     //PWM通道一和引脚PWMA关联 //pin25定义为通道0的输出引脚
  ledcWrite(pwm_Channel_1, 0);        //PWM通道一占空比设置为零
  //timer_control.attach_ms(interrupt_time_control, control); //定时器中断开启
  
  MQTT_init();
}


void setup() {
  // put your setup code here, to run once:
  HarewardInit();
}

void loop() {
  // put your main code here, to run repeatedly:
  client.loop(); //客户端循环检测

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
   digitalWrite(IN1, HIGH);     
   digitalWrite(IN2, LOW);
  }//TB6612的电平控制
  else if(moto1 == 0)
  {
    digitalWrite(IN1, LOW);       
    digitalWrite(IN2, LOW);
  }//TB6612的电平控制
  else 
  {
    digitalWrite(IN1, LOW);       
    digitalWrite(IN2, HIGH);
    }
  
  //功能：限制PWM赋值 
  if (moto1 < -Amplitude)  moto1 = -Amplitude;
  if (moto1 >  Amplitude)  moto1 =  Amplitude;
  
  //赋值给PWM寄存器 
  ledcWrite(pwm_Channel_1,abs(moto1));
}

//收到主题下发的回调, 注意这个回调要实现三个形参 1:topic 主题, 2: payload: 传递过来的信息 3: length: 长度
void callback(char *topic, byte *payload, unsigned int length)
{
  char json[200];
  char *parameter = (char *)malloc(length); 
  /* 解析JSON */
  StaticJsonDocument<200> jsonBuffer2; //声明一个JsonDocument对象，长度200
  
  Serial.println("message rev:");
  Serial.println(topic);
  for (size_t i = 0; i < length; i++)
  {
    //Serial.print((char)payload[i]);
    parameter[i] = (char)payload[i];
    Serial.print(parameter[i]);
  }
  Serial.println();
  
  // 反序列化JSON
  DeserializationError error = deserializeJson(jsonBuffer2, parameter,length);
     if (error) 
    {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
    }
    JsonObject motordata = jsonBuffer2.as<JsonObject>();
    
  long motor = jsonBuffer2["motorfeed"];              // 读取整形数据
  Set_Pwm(motor);
  Serial.println("now_PWM:");
  Serial.print(motor);
  Serial.println();
  free(parameter);
}
