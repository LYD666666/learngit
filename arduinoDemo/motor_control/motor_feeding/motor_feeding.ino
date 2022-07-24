#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "weight.h"
#include "motor.h"
#include <stdio.h>

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
void sendADC(void); //Task1 称重并发送云平台任务
void pushrod(void *pvParameter); //Task2 推杆状态发送云任务
void weight_request(void *pvPrameter); 
void callback(char *topic, byte *payload, unsigned int length); //mqtt回调

long count;
long count0;
long count1 = 400;
char string1[10];
char msgJson[75];                       //要发送的json格式的数据
char msg_buf[200];                      //发送信息缓冲区
char dataWeight[] = "{\"castweight\":%ld}"; //信息模板
char dataWeight1[] = "{\"castweight1\":%ld}"; //反馈的信息
char datastausR [] = "{\"caststaus\":ready}";
char datastausB [] = "{\"caststaus\":busy}";
char datastausF [] = "{\"caststaus\":finish}";
char castbegin1 [] = "{\"castbegin\":%s}";  

unsigned short json_len = 4;            // json长度
long weight = 2500;
long weight_weizhi;   //待投量
long weight2 = 50;
long weight_zhenshi;  //真实投喂量
const char* castbegin;     //投喂开关
int castbegin_num;

// WiFi
const char *ssid = "STOP607_2.4";         // Enter your WiFi name      STOP607_2.4  iQOO3
const char *password = "sigmaWYU3601";    // Enter WiFi password       sigmaWYU3601 asdfghjkl

// MQTT Broker
const char *mqtt_broker = "106.14.145.57";
const char *topic = "/casting_publish";
const char *topic1 = "/casting_subscribe";
const char *mqtt_username = "cwl";
const char *mqtt_password = "19260817";
const int mqtt_port = 1883; 

//实例对象
WiFiClient espClient;
PubSubClient client(espClient);

//硬件初始化
void HarewardInit(void)
{
  // Set software serial baud to 115200;
  Serial.begin(115200); //串口

  // Set ADC GPIO
  pinMode(HX_DT, INPUT);
  pinMode(HX_SCk, OUTPUT);

  //motor _push
  motor_init();

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

  //    1   推杆、阀门复位，都是关闭状态,发送ready，准备称重
  digitalWrite(valve,LOW);
  Set_Pwm(500);
  delay(7000);
  Serial.print("now is reset\r\n");
  Set_Pwm(0);
  delay(500);

    // ADC转换(获取count0的值用于调平衡)
    ReadCount();
    count0 = 0;
    for (int i = 0; i < 8; i++)
    count0 += ReadCount();
    count0 /= 8;

    client.subscribe(topic);                 //连接MQTT服务器后订阅主题
    client.publish(topic1,datastausR);       //发送一个ready
    
}

//  2    阀门打开，开始称重并发送busy
void feeding1(void)
{
   digitalWrite(valve,HIGH);
   delay(5000);
   client.publish(topic1,datastausB);
}


// 5 获取投喂真实量
void tasktomotor(int countt,int weight1)
{
  if(countt > weight1) 
  {
      //打开
      //断开NC,关阀门
      digitalWrite(valve,LOW); 

      delay(5000);
      long count = ReadCount() - count0;
      count = ((count - count1)/3000);
      weight_zhenshi = count;
      weight_weizhi = weight - weight_zhenshi;
      Serial.print("publish weight\r\n");
      Serial.print(weight_zhenshi);
      
      if(weight_weizhi < weight2)
      {
        Set_Pwm(-500); //下滑推杆开始投料
        delay(6000);  //等待漏料
        Set_Pwm(0);
        snprintf(msgJson, 40, dataWeight1, weight_zhenshi); //将称重数据套入dataWeight模板中, 生成的字符串传给msgJson
        json_len = strlen(msgJson);               // msgJson的长度
        client.publish(topic1, msgJson);
        }
      else{
        Set_Pwm(-500); //下滑推杆开始投料
        delay(6000);  //等待漏料
        Set_Pwm(0);
        snprintf(msgJson, 40, dataWeight1, weight_zhenshi); //将称重数据套入dataWeight模板中, 生成的字符串传给msgJson
        json_len = strlen(msgJson);               // msgJson的长度
        client.publish(topic1, msgJson);
        
        }
      delay(50);
      client.publish(topic1,datastausF);
      //关闭
      posetive_motor(1);
      delay(6000);
      Set_Pwm(0);
      Serial.print("now is close");
      delay(1000);
       count0 = 0;
       for (int i = 0; i < 8; i++)
       count0 += ReadCount();
       count0 /= 8;
      client.publish(topic1,datastausR);
        
    }
  }
  
// 4 Task1  json格式传输  反馈一次重量
void sendADC(void)
{
    if (client.connected())
    {
      String id;
      long count = ReadCount() - count0;
      count = ((count - count1)/3000);
      Serial.println(count);
      snprintf(msgJson, 40, dataWeight1, count); //将称重数据套入dataWeight模板中, 生成的字符串传给msgJson
      json_len = strlen(msgJson);               // msgJson的长度
      //client.publish(topic1, msgJson);
         
      tasktomotor(count,weight2);
      vTaskDelay(1000/portTICK_PERIOD_MS); //需要更改，最后用RTOS   一延时就过去执行别的任务
    }
}

//  3  回调函数 读取总重量
void callback(char *topic, byte *payload, unsigned int length)
{
  char json[200];
  char *parameter = (char *)malloc(length); 
  /* 解析JSON */
  StaticJsonDocument<200> jsonBuffer2; //声明一个JsonDocument对象，长度200
  
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message:");
  for (int i = 0; i < length; i++)
  {
    //Serial.print((char)payload[i]);
    parameter[i] = (char)payload[i];
    Serial.print(parameter[i]);
  } 
  
  // 反序列化JSON
  DeserializationError error = deserializeJson(jsonBuffer2, parameter,length);
     if (error) 
    {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
    }
    JsonObject dataWeight = jsonBuffer2.as<JsonObject>();
    
  weight = jsonBuffer2["castweight"];              // 读取整形数据,总重量
  //castbegin = jsonBuffer2["castbegin"];
 // tasktomotor(weight);
  
  Serial.println("now:\r\n");
  Serial.print(weight);
 // Serial.print(castbegin);
//  if(strcmp(castbegin,"on")== 0)
//  {
//    castbegin_num = 1;
//    }
  Serial.println();
  free(parameter);
  Serial.println("--------------------------");
}


//初始化
void setup()
{
  /*
      注意：1、靠后生成的同级任务优先执行
      ·····2、
  */
  //联网和硬件设备初始化
  HarewardInit();

  //开启阀门发busy
  feeding1();
    /* 创建5个任务: 订阅/点灯/称重/发布重量/阀口开关
     */
    Serial.print("program in this");

    /* 启动调度器 */
  //  vTaskStartScheduler();
  // }
}

/*-----------------------------------------------------------*/


//循环
void loop()
{
  client.loop();
//  if(castbegin_num==1)
//  {
     sendADC();
    delay(500);
 //   }
 // else
  //;
  digitalWrite(valve,HIGH); 
  client.publish(topic1,datastausB);


}
