/* ����ʱ���� */
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
#include "ShowClockTimeTask.h"

static void Buzzer_Beep(int freq, int duty, int duration_ms)
{
    PassiveBuzzer_Set_Freq_Duty(freq, duty);
    vTaskDelay(duration_ms);
    PassiveBuzzer_Control(0);
}
/* USER CODE END Includes */

// �ⲿ����/��ʱ��/���о��
extern TaskHandle_t xShowMenuTaskHandle;
extern TimerHandle_t g_Clock_Timer;
extern QueueHandle_t g_xQueueMenu;
extern u8g2_t u8g2;
extern SemaphoreHandle_t g_xTimeMutex;


uint16_t millisecond;                 // �����ʱ(0~9)
uint8_t len1, len2;                   // ���ֿ���ƫ�Ʊ���
uint8_t clock_flag = 0;              // ��ʱ�����б�־ 0:ֹͣ 1:����
int8_t seclect_flag = 0;             // ��ǰѡ�е���λ(0~4)

// ���ֻ������ꡢ���߲���
uint16_t g_num_x[] = {1, 9, 25, 33, 41, 17}, g_num_y[] = {22, 52};
uint16_t num_w = 6, num_h = 8;

// ѡ�п����������
uint16_t g_seclect_x[] = {1, 9, 25, 33, 41};

uint16_t g_clock_num[] = {0, 0, 0, 0};// �û��趨�ĵ���ʱʱ�� ��:��:��:��
uint16_t g_real_time[] = {0, 0, 0, 0};// ʵʱ�ݼ�����ʱʱ��

/*
 \* @brief  ���Ƶ���ʱUI����
 \* @note   �������趨ʱ�䡢ʣ��ʱ�䡢ð�š���ͷ����ȦԲ����ѡ�п�
 */
void ShowClock(void)
{
	/* ���������ʾ���� Set: Ret: */
 	len1 = u8g2_DrawStr(&u8g2, 0, 30, "Set:");
	len2 = u8g2_DrawStr(&u8g2, 0, 60, "Ret:");

	/* ѭ������4λʱ�����֣��趨ʱ�� + ʣ��ʱ�䣩 */
	for(int i = 0; i < 4; i++)
	{
		u8g2_DrawXBMP(&u8g2, len1+g_num_x[i], g_num_y[0], num_w, num_h, Num_6x8[g_clock_num[i]]);	
		u8g2_DrawXBMP(&u8g2, len2+g_num_x[i], g_num_y[1], num_w, num_h, Num_6x8[g_real_time[i]]);
	}
	/* ����ʱ��ð�� : ���Ҽ�ͷ > */
	u8g2_DrawXBMP(&u8g2, len1+g_num_x[5], g_num_y[0], num_w, num_h, Num_6x8[10]);/* : */
	u8g2_DrawXBMP(&u8g2, len1+g_num_x[4], g_num_y[0], num_w, num_h, Num_6x8[11]);/* > */
	u8g2_DrawXBMP(&u8g2, len2+g_num_x[5], g_num_y[1], num_w, num_h, Num_6x8[10]);/* : */

	/* �����Ҳ൹��ʱ��ȦԲ�� */
	u8g2_DrawCircle(&u8g2, 104, 31, 22, U8G2_DRAW_ALL);
	u8g2_DrawCircle(&u8g2, 104, 31, 23, U8G2_DRAW_ALL);
	u8g2_DrawDisc(&u8g2, 104, 31, 1, U8G2_DRAW_ALL);
	
	/* �����Ҳ�������� */
	u8g2_DrawXBMP(&u8g2, 94, 12, 20, 40, BigNum[millisecond]);			
	/* ���Ƶ�ǰѡ�е���λ�ľ���ѡ�п� */
	u8g2_DrawFrame(&u8g2, len1+g_seclect_x[seclect_flag]-1, g_num_y[0]-2, num_w+2, num_h+3);	
}

 /*
 \* @brief  ����ʱ����������
 \* @note   ��������ʱ���롢��������ʱ����ʱ�������������ز˵�
  */
void ShowClockTimeTask(void *params)
{
	/* 蜂鸣器初始化 */
	PassiveBuzzer_Init();

	/* 消息队列由 InputTask 统一创建，此处不再重复创建 */

	/* use global u8g2, only set font */
	u8g2_SetFont(&u8g2, u8g2_font_wqy16_t_chinese1);
//	u8g2_SetFont(&u8g2, u8g2_font_spleen32x64_mf);	
//	u8g2_SetFont(&u8g2, u8g2_font_fur35_tf);
	
	struct Key_data	key_data;	
	
	while(1)
	{
		/* check exit notification from InputTask */
		if (ulTaskNotifyTake(pdTRUE, 0) > 0)
		{
			clock_flag = 0;
			xTimerStop(g_Clock_Timer, 0);
			vTaskSuspend(NULL);
		}
		u8g2_ClearBuffer(&u8g2); // ����Դ�
		ShowClock();		         // ����һ��ҳUI
		//u8g2_DrawXBMP(&u8g2, 0, 0, 20, 40, BigNum[temp]);
		u8g2_SendBuffer(&u8g2);  // ˢ�µ���Ļ
		
		/* ����ʱδ����ʱ�������ȴ����� */
		if(clock_flag == 0)
		{
			xQueueReceive(g_xQueueMenu, &key_data, 0); /* 非阻塞，InputTask 管理返回 */
		}
		/* �Ҽ��������л�����λ */
		if(key_data.rdata == 1)
		{
			Buzzer_Beep(2500, 100, 50);
			seclect_flag++;
			if(seclect_flag>4)seclect_flag=0;
		}
		/* ����������л�����λ */
		if(key_data.ldata == 1)
		{
			Buzzer_Beep(2500, 100, 50);
			seclect_flag--;
			if(seclect_flag<0)seclect_flag=4;
		}
		/* ȷ�ϼ��߼� */
		if(key_data.updata == 1)
		{
			Buzzer_Beep(2500, 100, 50);
			
			if(seclect_flag == 4)//���ѡ�е�4λ = ��������ʱ
			{
				/*������ʱ��*/
				if(g_Clock_Timer != NULL)
				{
					xTimerStart(g_Clock_Timer, 0);
					clock_flag = 1;
					key_data.updata = 0;
				}
			}
			else{
				g_clock_num[seclect_flag]++;
				if(g_clock_num[seclect_flag]>9)g_clock_num[seclect_flag]=0;				
			}
		}		
		
		/* ���ؼ����˳�����ʱ���棬�ص����˵� */
		/* 返回键由 InputTask 统一处理，此处只做清理工作 */
		if(key_data.exdata == 1)
		{
			Buzzer_Beep(2500, 100, 50);
			clock_flag = 0;
			xTimerStop(g_Clock_Timer, 0);
		}	
		
    // ����ʱ�����У��ж��Ƿ��ʱ����
		if(clock_flag == 1)
		{
			/* time_stop */
			if(g_clock_num[0]==g_real_time[0]&&g_clock_num[1]==g_real_time[1]&&g_clock_num[2]==g_real_time[2]&&g_clock_num[3]==g_real_time[3])
			{
				clock_flag = 0;
				xTimerStop(g_Clock_Timer, 0);
				/* music */
				Buzzer_Beep(2500, 100, 200);
			}
		}
	}
}

/******************TimerCallBackFun*******************/
void ClockTimerCallBackFun(TimerHandle_t xTimer)
{
	if (xSemaphoreTake(g_xTimeMutex, 0) == pdTRUE)
	{
		millisecond++;
		if(millisecond>9)
		{
			millisecond = 0;
			g_real_time[3]++;
			if(g_real_time[3]>9)
			{
				g_real_time[2]++;
				g_real_time[3]=0;
				if(g_real_time[2]>5)
				{
					g_real_time[1]++;
					g_real_time[2] = 0;
					if(g_real_time[1]>9)
					{
						g_real_time[0]++;
						g_real_time[1] = 0;
					}
				}
			}
		}
		xSemaphoreGive(g_xTimeMutex);
	}
}
