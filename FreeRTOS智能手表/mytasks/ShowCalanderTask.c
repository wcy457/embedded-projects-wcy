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
#include "ShowCalanderTask.h"

static void Buzzer_Beep(int freq, int duty, int duration_ms)
{
    PassiveBuzzer_Set_Freq_Duty(freq, duty);
    vTaskDelay(duration_ms);
    PassiveBuzzer_Control(0);
}
/* USER CODE END Includes */
extern u8g2_t u8g2;

/* �ⲿȫ�־������ */
extern TaskHandle_t xShowMenuTaskHandle;    // ���˵�������
extern QueueHandle_t g_xQueueMenu;         // ȫ�ְ�����Ϣ����

/**
 * @brief  �ж��Ƿ�Ϊ����
 * @param  year: ���ж����
 * @retval 1:����  0:ƽ��
 * @note   ��������ܱ�400���� / �ܱ�4�����Ҳ��ܱ�100����
 */
 int judge_year(int year)
 {
	 if(year % 400 == 0 || (year % 4 == 0 && year % 100 != 0))
	 { return 1;}
	 else {return 0;}
 }

 /**
ef  ����ָ�����1��1�������ڼ�
 * @param  year: ָ�����
 * @retval ���ڱ��
 * @note   �ۼӴӹ�Ԫ1�굽��ǰ��ݵ�����������7ȡģ�õ�����ƫ��
 */
 int judge_week(int year)
{
	if(year==1){return 1;}
	
	int sum=0;
	for(int i=1;i<year;i++){
		if(judge_year(i)==1){sum=sum+366;} 
		else{sum=sum+365;}
	}
	return (sum+1)%7;
}

/**
 * @brief  ��ȡ�����꡿��Ӧ�·ݵ�����
 * @param  n: �·� 1~12
 * @retval ��ǰ�·�������
 */
int month_run(int n){
	switch(n){
		case 1:return 31;
		case 2:return 29;
		case 3:return 31;
		case 4:return 30;
		case 5:return 31;
		case 6:return 30;
		case 7:return 31;      
		case 8:return 31;
		case 9:return 30;
		case 10:return 31;
		case 11:return 30;
		case 12:return 31;
	}
}

/**
 * @brief  ��ȡ��ƽ�꡿��Ӧ�·ݵ�����
 * @param  n: �·� 1~12
 * @retval ��ǰ�·�������
 */
int month_ping(int n)
{
	switch(n)
	{
		case 1:return 31;
		case 2:return 28;
		case 3:return 31;
		case 4:return 30;
		case 5:return 31;
		case 6:return 30;
		case 7:return 31;
		case 8:return 31;
		case 9:return 30;
		case 10:return 31;
		case 11:return 30;
		case 12:return 31;
	}
}

/**
 * @brief  FreeRTOS ������������
 * @param  params: �������(δʹ��)
 * @note   ʵ�֣�2024��������ʾ�������л��·ݡ����ز˵�����
 */
void ShowCalanderTask(void *param)
{
	/* ��ʼ�������� */
	PassiveBuzzer_Init();
	
	/* 消息队列由 InputTask 统一创建，此处不再重复创建 */

u8g2_SetFont(&u8g2, u8g2_font_spleen5x8_mf);     // ����С����

	u8g2_SendBuffer(&u8g2);
	
	struct Key_data key_data;                        // ������Ϣ�ṹ��
	
	/* �����ַ��� 0~31 */
	const char ucMonthDay[32][3] = {"0","1","2","3","4","5","6","7","8","9","10","11","12","13","14","15","16","17","18","19","20","21","22","23","24","25","26","27","28","29","30","31"};	
	/* ����ͷ����д������~���� */
	const char ucWeekHeader[7][3] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};
	uint16_t usWeekX[7] = {0,  17, 34 , 51, 68, 85, 102}; // ���ڱ�ͷX����
	uint16_t usWeekY[6] = {17, 26, 35 ,44, 53, 62};       // ������Y���꣨���6��������
	uint16_t usLineY[12] = {0, 11, 22, 33, 44, 55, 66, 77, 88, 99, 110, 121}; // �Ҳ�����ָʾ��Y��
	uint8_t line_pos  = 0;        // �Ҳ൥��ָʾ��λ��
	
	uint8_t week_pos  = 0;        // ������������ƫ��
	uint32_t week_temp, week_temp_temp, month_temp, enter_temp;
	
	uint32_t month = 1;           // ��ǰ��ʾ�·ݣ�Ĭ��1��
	uint16_t wee = 0;	            // ��¼��ǰ�·ݵ�һ�������ڼ�
	
	while(1)
	{	
		/* check exit notification from InputTask */
	if (ulTaskNotifyTake(pdTRUE, 0) > 0)
	{
		vTaskSuspend(NULL);
	}
	u8g2_ClearBuffer(&u8g2);	// ���֡����
		
		// 1. ���ƶ������ڱ�ͷ Su Mo Tu We Th Fr Sa
		for(int i=0; i<=6; i++)
		{
			u8g2_DrawStr(&u8g2, usWeekX[i], 8, ucWeekHeader[i]);			
		}
		
		// 2. ��ȡ��ǰ�·ݵ����������̶�ʹ�������������2024��
		month_temp = month_run(month);
		
		// 3. ��ȡ2024��1��1�յ����ڻ���
		week_temp = judge_week(2024);
		wee = week_temp;
		
		// 4. �ۼ�ǰ�������·����������㡾��ǰ�µ�һ�졿�����ڼ�
		for(int m=1; m<month; m++){
			wee = (wee+month_run(m))%7;        
		}
		
		week_temp = wee;		
		week_temp_temp = week_temp;
		
		// 5. ѭ�����Ƶ�����������
		for(int k=1; k<=month_temp; k++){			
			enter_temp  = week_temp%7;   // ��ǰ���ڶ�Ӧ������
			week_temp++;                 // ��������ƫ��
			
			// ��һ������ƫ�ƽ���
			if(k<=(7-week_temp_temp)){
				week_pos=0;
			}else if(enter_temp == 0){
				week_pos = week_pos+1;			
			}
			// ���ƶ�Ӧ�������������
			u8g2_DrawStr(&u8g2, usWeekX[enter_temp], usWeekY[week_pos], ucMonthDay[k]);	
		}
		
		// 6. �����Ҳ����ָ���
		u8g2_DrawLine(&u8g2, 115, 0, 115, 62);
		// �Ҳ���ʾ��ǰ���ں���
		u8g2_DrawStr(&u8g2, 117, 32, ucMonthDay[line_pos+1]);	
		// �ײ�����ָʾ��
		u8g2_DrawHLine(&u8g2, usLineY[line_pos], 63, 11);
		
		// ˢ����Ļ
		u8g2_SendBuffer(&u8g2);

		// �����ȴ�����������Ϣ
		xQueueReceive(g_xQueueMenu, &key_data, 0); /* 非阻塞，InputTask 管理返回 */

		// �Ҽ����л���һ����
		if(key_data.rdata == 1)
		{
			Buzzer_Beep(2500, 100, 50);   // ��������
			month++;                  // �·�+1
			line_pos++;               // �Ҳ�ָʾ������
			
			if(line_pos>11)line_pos=0; // ָʾ��ѭ��
			if(month>12)month=1;       // 12��֮���л�1��
			key_data.rdata = 0;       // ��հ�����־
		}
		
		// ȷ�ϼ������ز˵�����
		/* 返回键由 InputTask 统一处理 */
		if(key_data.exdata == 1)
		{
			Buzzer_Beep(2500, 100, 50);
			key_data.exdata = 0;
		}
	}
	
}
