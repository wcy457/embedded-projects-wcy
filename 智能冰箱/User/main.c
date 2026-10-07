#include "stm32f10x.h"        // Device header
#include <string.h>
#include "Delay.h"
#include "OLED.h"
#include "DHT11.h"
#include "LED.h"
#include "Key.h"
#include "BEEP.h"
#include "Encoder.h"
#include "Timer.h"
#include "MyRTC.h"
#include "PWM.h"
#include "Motor.h"
#include "Serial.h"

float SetTemp = 25.0f;//用户设定温度
uint8_t Door_State = 1;//门状态：1关门  0开门
uint32_t Open_Count = 0;//开门次数统计
uint32_t Open_Start_Time = 0;//记录开门开始时间，用于超时报警
uint8_t KeyNum;

uint8_t LastOpen_H = 0;
uint8_t LastOpen_M = 0;

int main(void)
{  
	OLED_Init();
	DHT11_Init();
	LED_Init();
	Key_Init();
	BEEP_Init();
	Encoder_Init();
	Timer_Init();
	MyRTC_Init();
	PWM_Init();
	Motor_Init();
	Serial_Init();
	
	OLED_ShowString(1, 1, "T:");//实际温度
	OLED_ShowString(1, 8, "Set:");//设定温度
	OLED_ShowString(2, 1, "H:");//湿度
	OLED_ShowString(3, 1, "Door:");//门状态
	OLED_ShowString(4, 1, "Num:");//开门次数
	OLED_ShowString(4, 9, "Time:");//实时时钟
	
	
  while(1)
	 {
		 DHT11_ReadData(&DHT11_Data);//读取温湿度
		 float Temp = DHT11_Data.temp_int + DHT11_Data.temp_dec * 0.1f;
		 float Humi = DHT11_Data.humi_int + DHT11_Data.humi_dec * 0.1f;
		 
		 //显示温度
		 OLED_ShowNum(1, 3, DHT11_Data.temp_int, 2);
		 OLED_ShowString(1, 5, ".");
		 OLED_ShowNum(1, 6, DHT11_Data.temp_dec, 1);
		 //显示湿度
		 OLED_ShowNum(2, 3, DHT11_Data.humi_int, 2);
		 OLED_ShowString(2, 5, ".");
		 OLED_ShowNum(2, 6, DHT11_Data.humi_dec, 1);
		 
		 //功能2：旋钮编码器调节设定温度
		 SetTemp += 0.5f * Encoder_Get();
		 if (SetTemp < 20){SetTemp = 20;}
		 if (SetTemp > 30){SetTemp = 30;}
		 
		 OLED_ShowNum(1, 12, (int)SetTemp, 2);
		 OLED_ShowString(1, 14, ".");
		 OLED_ShowNum(1, 15, (int)((SetTemp - (int)SetTemp)*10), 1);
		 
		 //功能3：温差控制电机（模拟压缩机）
		 float diff = Temp - SetTemp;
		 if (diff <= 0.5f)
		 {
			 Motor_SetSpeed(0);
		 }
		 else
		 {
			 int speed = diff * 20;
			 if (speed > 100){speed = 100;}
			 Motor_SetSpeed(speed);
		 }
		 
		 //功能4：按键模拟开关门 + 开关门次数统计 +最后一次显示开门时间
		 static uint8_t key_lock = 0;//按键状态锁，防止按键一直触发
		 KeyNum = Key_GetNum();
		 if (KeyNum == 1 && key_lock == 0)
		 {
			key_lock = 1;
			Door_State = !Door_State;
        if(Door_State == 0)
				{
					Open_Count ++;
					Open_Start_Time = System_tick_ms;
					MyRTC_ReadTime();
					LastOpen_H = MyRTC_Time[3];
					LastOpen_M = MyRTC_Time[4];
				}
				else
				{
					Open_Start_Time = 0;
				}
		 }
		 key_lock = KeyNum;//更新上次按键状态
		 
		 //根据门状态亮灯 + 显示
		 if (Door_State == 0)
		 {
			 LED1_ON();
			 OLED_ShowString(3, 6, "OPEN");
		 }
		 else
		 {
			 LED1_OFF();
			 OLED_ShowString(3, 6, "OFF");
			 OLED_ShowString(3, 9, " ");
		 }
		 
		 OLED_ShowNum(4, 6, Open_Count, 3);
		 //显示开门时间
		 OLED_ShowNum(4, 13, LastOpen_H, 2);
		 OLED_ShowNum(4, 15, LastOpen_M, 2);
		 
		 //功能5：开门超时10秒蜂鸣报警 + 蓝牙上报
		 if (Door_State == 0 && System_tick_ms - Open_Start_Time > 10000)
		 {
			 BEEP_ON();
			 Delay_ms(500);
			 BEEP_OFF();
			 Delay_ms(500);
			 
			 static uint32_t cnt;
			 if (System_tick_ms - cnt > 2000)
			 {
				 cnt = System_tick_ms;
				 Serial_Printf("[WARN] Door Open Too Long!\r\n");
			 }
		 }
		 else
		 {
			 BEEP_OFF();
		 }
		 
		 //功能6：APP管理冰箱 + 蓝牙定时上报
			if(Serial_RxFlag == 1)//串口接收完成标志
			{
					//远程设置温度 SET20~SET30
					if(strstr(Serial_RxPacket, "SET") != NULL)//如果接收到SET，就执行设置温度
					{
						uint8_t num = (Serial_RxPacket[3] - '0') * 10 + (Serial_RxPacket[4] - '0');//取出指令的4，5个字符
						if(num >= 20 && num <= 30)//（SET25 → 第 3 位是 '2'，第 4 位是 '5'）转成数字：2*10 +5 =25
						{
							SetTemp = num;
						}
					}

					if(strstr(Serial_RxPacket, "GET") != NULL)
					{
						Serial_Printf("Temp:%.1f  Humi:%.1f  Set:%.1f Door:%d\r\n",
						Temp, Humi, SetTemp, Door_State);
					}
			//清空数据缓冲区，准备下一次接收
				Serial_RxFlag = 0;//把标志位清0
				for(uint8_t i = 0; i < 20; i++)
				{
						Serial_RxPacket[i] = 0;//把接收数组全部清零，防止旧数据干扰下一次指令
				}
			}
		 
			//蓝牙定时上报
			static uint32_t last_send;//变量：记住上一次发送时间
			if (System_tick_ms - last_send > 1000)
			{
				last_send = System_tick_ms;
				Serial_Printf("[LOG] T=%.1f Set=%.1f Door%s\r\n",
				             Temp, SetTemp, Door_State ? "CLOSED":"OPEN");
			}
			
    }
}
