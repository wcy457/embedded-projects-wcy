/* �������������棩 */
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
#include "ShowWoodenFish.h"

static void Buzzer_Beep(int freq, int duty, int duration_ms)
{
    PassiveBuzzer_Set_Freq_Duty(freq, duty);
    vTaskDelay(duration_ms);
    PassiveBuzzer_Control(0);
}

extern SemaphoreHandle_t g_xSemKey;       // ȫ�ְ����ź�����Ԥ���������⣬��δ���ã�
extern TaskHandle_t xShowMenuTaskHandle;   // ���˵������������ڷ��ؽ���
extern QueueHandle_t g_xQueueMenu;         // ȫ�ְ������У����հ����ж�����
extern u8g2_t u8g2;                        // ȫ�� OLED ��Ļ���

uint16_t WfisTaskRuning;


/**
 * @brief  ShowWoodenFishTask
 * @note   ����ľ�㹦�¼�����������
 * @function
 *     1. OLED �����ʼ�����������á�����
 *     2. ������Ϣ���г�ʼ��
 *     3. ����ľ�㡢���ӡ��������֡���λ��������
 *     4. �����Ҽ��û��¼�����Ч��������������λ��+1����
 *     5. �������ؼ����˳����桢�ָ��˵�����������
 *     6. ˫����ˢ�½��棬ʵ�ֶ��������ص�
 * @param  params : FreeRTOS �����βΣ�δʹ�ã�
 * @retval ��
 */
void ShowWoodenFishTask(void *params)
{
	//xSemaphoreTake(g_xSemKey, portMAX_DELAY);
	
	/* system sound */
	PassiveBuzzer_Init();
	/* 消息队列由 InputTask 统一创建，此处不再重复创建 */

	/* use global u8g2 */
	u8g2_SetFont(&u8g2, u8g2_font_wqy16_t_chinese1);
//	u8g2_SetFont(&u8g2, u8g2_font_spleen32x64_mf);	
//	u8g2_SetFont(&u8g2, u8g2_font_fur35_tf);
	
	struct Key_data	key_data;
	uint8_t wooden_fish_flag = 0, seclect_flag = 0, add_flag = 0;
	uint8_t num1 = 0, num2 = 0, num3 = 0; 
	while(1)
	{
		/* check exit notification from InputTask */
	if (ulTaskNotifyTake(pdTRUE, 0) > 0)
	{
		vTaskSuspend(NULL);
	}
	WfisTaskRuning = 1;
		u8g2_ClearBuffer(&u8g2);

		u8g2_DrawXBMP(&u8g2, 80, 0, 6, 8, Num_6x8[num1]);		
		u8g2_DrawXBMP(&u8g2, 88, 0, 6, 8, Num_6x8[num2]);		
		u8g2_DrawXBMP(&u8g2, 96, 0, 6, 8, Num_6x8[num3]);		

		switch(seclect_flag)
		{
			case 0: u8g2_DrawXBMP(&u8g2, 8, 15, 63, 48, wooden_flsh[0]);u8g2_DrawXBMP(&u8g2, 5, 0, 30, 8, hammer);break;
      case 1: u8g2_DrawXBMP(&u8g2, 8, 15, 49, 38, wooden_flsh[1]);u8g2_DrawXBMP(&u8g2, 10, 8, 30, 8, hammer);seclect_flag = 0;break;
		}
		u8g2_DrawXBMP(&u8g2, 75, 37, 32, 16, gongde);
		u8g2_DrawStr(&u8g2, 110, 47, ":");		
		
		u8g2_SendBuffer(&u8g2);
		
		/* �������ж϶��� */
		if(wooden_fish_flag == 0 && seclect_flag == 0)
		{
			xQueueReceive(g_xQueueMenu, &key_data, 0); /* 非阻塞，InputTask 管理返回 */
		}
		/* processing data */
		if(key_data.rdata == 1)
		{
			Buzzer_Beep(3000, 200, 50);
			num3++;
			if(num3>9){num3=0;num2++;}
			if(num2>9){num2=0;num1++;}
			seclect_flag = 1;
			add_flag = 100;
			key_data.rdata = 0;
			key_data.exdata = 0;
		}		
		/* 返回键由 InputTask 统一处理 */
		if(key_data.exdata == 1)
		{
			Buzzer_Beep(2500, 100, 50);
		}
		
		u8g2_ClearBuffer(&u8g2);
		if(add_flag!=0){add_flag--;u8g2_DrawStr(&u8g2, 105, 40, "+1");}
		switch(seclect_flag)
		{
			case 0: u8g2_DrawXBMP(&u8g2, 8, 15, 63, 48, wooden_flsh[0]);u8g2_DrawXBMP(&u8g2, 5, 0, 30, 8, hammer);break;
      case 1: u8g2_DrawXBMP(&u8g2, 8, 15, 49, 38, wooden_flsh[1]);u8g2_DrawXBMP(&u8g2, 10, 8, 30, 8, hammer);seclect_flag = 0;break;
		}			
		u8g2_SendBuffer(&u8g2);
	}
}
