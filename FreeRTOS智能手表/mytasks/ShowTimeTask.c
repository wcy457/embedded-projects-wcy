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
#include "ShowTimeTask.h"
extern SemaphoreHandle_t g_xTimeMutex;
extern u8g2_t u8g2;


static void Buzzer_Beep(int freq, int duty, int duration_ms)
{
    PassiveBuzzer_Set_Freq_Duty(freq, duty);
    vTaskDelay(duration_ms);
    PassiveBuzzer_Control(0);
}
/* USER CODE END Includes */

/* �ⲿ��������������������֮����ͣ/�ָ������л����� */
extern QueueHandle_t g_xQueueMenu;                      // �˵�������Ϣ���о��

extern TaskHandle_t xShowMenuTaskHandle;                // �˵�����������
extern TaskHandle_t xShowTimeTaskHandle;                // ʱ�ӽ���������
extern TaskHandle_t xShowWoodenFishTaskHandle;          // ľ�����������
extern TaskHandle_t xShowFlashLightTaskHandle;          // �ֵ�Ͳ����������
extern TaskHandle_t xShowSettingTaskHandle;             // ���ý���������
extern TaskHandle_t xShowCalendarTaskHandle;            // ��������������
extern TaskHandle_t xShowClockTaskHandle;               // ����ʱʱ�ӽ�����
extern TaskHandle_t xShowDHT11TaskHandle;               // ��ʪ��DHT11������

/* �궨�壺Բ�Ǿ���Բ�ǰ뾶 */
#define BOX_R 1
uint8_t time_flag = 0;                                  // ʱ������־λ��Ԥ����չ��

/* ʱ���������λ��ʮλ������� */
uint8_t sec_unit, sec_decade;
uint8_t min_unit, min_decade;
uint8_t hour_unit, hour_decade;

/* ʱ��������ʾ�ṹ�� */
typedef struct Timer_param{
  int x[4];
	int y;
	int w;
	int h;
	int x_arg;
}T;

/* ��ʼ��ʱ�����������ʾλ�ò��� */
T time = { {8, 35, 71, 98}, 15, 20, 40, 98};

Image Box1 = {62, 22, 4, 4,};                           // ʱ���м�ָ�����Բ�����
Image Box2 = {62, 39, 4, 4,};                           // ʱ���м�ָ�����Բ�����

/*
 * @brief  ��FreeRTOS����ʱ������ʾ��������
 * @param  params�����������(δʹ��)
 * @note   ����ʱ��ҳ��ʱ���У���ͣ�������н������񣻽��հ�����Ϣ�л��ز˵�ҳ��
 */
void ShowTimeTask(void *param)
{
	PassiveBuzzer_Init();

	/* use global u8g2, only set font for this task */
	u8g2_SetFont(&u8g2, u8g2_font_fur30_tf);

	while(1)
	{
		/* check exit notification from InputTask */
		if (ulTaskNotifyTake(pdTRUE, 0) > 0)
		{
			vTaskSuspend(NULL);
		}

		/* mutex protects timer variables from callback preemption */
		if (xSemaphoreTake(g_xTimeMutex, portMAX_DELAY) == pdTRUE)
		{
			u8g2_ClearBuffer(&u8g2);

			u8g2_DrawXBMP(&u8g2, 0, 0, 23, 10, ShowPower);
			u8g2_DrawXBMP(&u8g2, 105, 0, 23, 10, ShowGame);
			u8g2_DrawXBMP(&u8g2, time.x[3], time.y, time.w, time.h, BigNum[sec_unit]);
			u8g2_DrawXBMP(&u8g2, time.x[2], time.y, time.w, time.h, BigNum[sec_decade]);
			u8g2_DrawRBox(&u8g2, Box1.x, Box1.y, Box1.w, Box1.h, BOX_R);
			u8g2_DrawRBox(&u8g2, Box2.x, Box2.y, Box2.w, Box2.h, BOX_R);
			u8g2_DrawXBMP(&u8g2, time.x[1], time.y, time.w, time.h, BigNum[min_unit]);
			u8g2_DrawXBMP(&u8g2, time.x[0], time.y, time.w, time.h, BigNum[min_decade]);
			u8g2_DrawXBMP(&u8g2, 56, 2, 6, 8, Num_6x8[hour_decade]);
			u8g2_DrawXBMP(&u8g2, 66, 2, 6, 8, Num_6x8[hour_unit]);

			u8g2_SendBuffer(&u8g2);
			xSemaphoreGive(g_xTimeMutex);
		}

		vTaskDelay(250);
  }
}

/**********************************************************************
 * �������ƣ� FreeRTOS������ʱ���ص�����
 * @param  xTimer����ʱ�����
 * @note   ��ʱ������ʵ���롢�֡�ʱ�Զ���λ��ʱ
 ***********************************************************************/
void TimerCallBackFun(TimerHandle_t xTimer)
{
	/* try to take mutex; skip this tick if display is rendering */
	if (xSemaphoreTake(g_xTimeMutex, 0) == pdTRUE)
	{
		sec_unit++;
		if(sec_unit>9){sec_unit = 0; sec_decade++;}
		if(sec_decade>5){sec_decade = 0; min_unit++;}
		if(min_unit>9){min_unit = 0; min_decade++;}
		if(min_decade>5){min_decade = 0; hour_unit++;}
		if(hour_unit>9){hour_unit = 0; hour_decade++;}
		/* 24h reset */
		if(hour_decade==2 && hour_unit==4)
		{
			sec_unit=0; sec_decade=0;
			min_unit=0; min_decade=0;
			hour_unit=0; hour_decade=0;
		}
		xSemaphoreGive(g_xTimeMutex);
	}
}
