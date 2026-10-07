/* �������� */
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "event_groups.h"
#include "queue.h"
#include "semphr.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "u8g2.h"
#include "driver_passive_buzzer.h"
#include "Data.h"
#include "ShowDHT11.h"

static void Buzzer_Beep(int freq, int duty, int duration_ms)
{
    PassiveBuzzer_Set_Freq_Duty(freq, duty);
    vTaskDelay(duration_ms);
    PassiveBuzzer_Control(0);
}
/* USER CODE END Includes */
extern u8g2_t u8g2;

extern TaskHandle_t xShowMenuTaskHandle;
extern QueueHandle_t g_xQueueMenu;

void ShowDHT11Task(void *params)
{
	DHT11_Init();
	PassiveBuzzer_Init();
	/* 消息队列由 InputTask 统一创建，此处不再重复创建 */

u8g2_SetFont(&u8g2, u8g2_font_wqy16_t_chinese1);

	u8g2_SendBuffer(&u8g2);
	
	struct Key_data	key_data;
	int hum, temp;
    int hum1, hum2, hum3, temp1, temp2 ,temp3;

    int max;
	int g_max[] = {20, 30, 40, 50, 60, 70};
	
	while(1)
	{	
		/* check exit notification from InputTask */
	if (ulTaskNotifyTake(pdTRUE, 0) > 0)
	{
		vTaskSuspend(NULL);
	}
	u8g2_ClearBuffer(&u8g2);			
		if (DHT11_Read(&hum, &temp) !=0 ){
			//printf("\n\rdht11 read err!\n\r");
			DHT11_Init();
		}
		else{
			temp1 = temp%20;	//low bit
			temp3 = temp/10;
			temp2 = temp3%20;   //high bit
						
			for(int i=0; i<5; i++)
			{
				max = hum>g_max[i]?g_max[i]:max;
			}			
			hum1 = hum%max;		//low bit	
			hum3 = hum/10;
			hum2 = hum3%max;    //high bit
			
			u8g2_DrawXBMP(&u8g2, 10, 20, 20, 40, BigNum[temp2]);
			u8g2_DrawXBMP(&u8g2, 35, 20, 20, 40, BigNum[temp1]);
			
			u8g2_DrawXBMP(&u8g2, 75, 20, 20, 40, BigNum[hum2]);
			u8g2_DrawXBMP(&u8g2, 100, 20, 20, 40, BigNum[hum1]);
		}
		u8g2_DrawStr(&u8g2, 15, 15, "temp");
		u8g2_DrawStr(&u8g2, 85, 15, "Hum");
		
		u8g2_SendBuffer(&u8g2);
	
		/* �������ж϶��� */
		xQueueReceive(g_xQueueMenu, &key_data, 0);
		
		/* 返回键由 InputTask 统一处理 */
		if(key_data.exdata == 1)
		{
			Buzzer_Beep(2500, 100, 50);
			key_data.exdata = 0;
		}
	}
}

