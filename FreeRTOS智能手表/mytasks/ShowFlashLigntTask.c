/* �ֵ�Ͳ���� */
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
#include "ShowFlashLigntTask.h"

static void Buzzer_Beep(int freq, int duty, int duration_ms)
{
    PassiveBuzzer_Set_Freq_Duty(freq, duty);
    vTaskDelay(duration_ms);
    PassiveBuzzer_Control(0);
}
/* USER CODE END Includes */
extern u8g2_t u8g2;

/* ȫ�ְ������У����ղ˵������¼� */
extern QueueHandle_t g_xQueueMenu;
/* ���˵������������ڷ��ز˵� */
extern TaskHandle_t xShowMenuTaskHandle;

/*
 \* @brief  �ֵ�Ͳ��������
 \* @���ܣ� ����ȷ�ϼ��л���ȫ������ / �ֵ�Ͳͼ�꡿
 \*         ���·��ؼ��˻����˵�
 */
void ShowFlashLightTask(void *params)
{
	PassiveBuzzer_Init();

	/* 消息队列由 InputTask 统一创建，此处不再重复创建 */

	// ����ֲ�OLED�����������Ļ��ʼ����

	// ���ó�������
	u8g2_SetFont(&u8g2, u8g2_font_fur30_tf);
	
	// ��ʼ���棺�����ֵ�Ͳͼ��
	u8g2_DrawXBMP(&u8g2, 48, 16, 30, 30, light);
	u8g2_SendBuffer(&u8g2);

	uint8_t light_flag = 0;         // �ֵ�Ͳ״̬��־ 0:��ͨͼ�� 1:ȫ������
	struct Key_data	key_data;      // ���հ������ݽṹ��
	
	while(1)
	{
		/* check exit notification from InputTask */
	if (ulTaskNotifyTake(pdTRUE, 0) > 0)
	{
		vTaskSuspend(NULL);
	}
	u8g2_ClearBuffer(&u8g2);	// ��յ�ǰ֡����
		
		// �����ȴ�������Ϣ��һֱ�ȣ���ռ��CPU��
		xQueueReceive(g_xQueueMenu, &key_data, 0); /* 非阻塞，InputTask 管理返回 */
		
		// ȷ�ϼ����л��ֵ�Ͳģʽ
		if(key_data.updata == 1)
		{
			Buzzer_Beep(2500, 100, 50); // ����������ʾ
			switch(light_flag)
			{
				case 0: 
					u8g2_DrawBox(&u8g2, 0, 0, 128, 64); // ȫ��Ϳ�ڣ�ȫ������Ч����
					light_flag++;  // �л�Ϊ����ģʽ
					break;
				case 1: 
					u8g2_DrawXBMP(&u8g2, 48, 16, 30, 30, light); // �ָ��ֵ�Ͳͼ��
					light_flag--;  // �л���ͨģʽ
					break;
			}
		}		

		// ���ؼ����˳��ֵ�Ͳ���棬�ص����˵�
		/* 返回键由 InputTask 统一处理 */
		if(key_data.exdata == 1)
		{
			Buzzer_Beep(2500, 100, 50);
		}		
		
		u8g2_SendBuffer(&u8g2); // ˢ����Ļ
	}
}
