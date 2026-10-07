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
#include "ShowSettingTask.h"

static void Buzzer_Beep(int freq, int duty, int duration_ms)
{
    PassiveBuzzer_Set_Freq_Duty(freq, duty);
    vTaskDelay(duration_ms);
    PassiveBuzzer_Control(0);
}
/* USER CODE END Includes */

extern TaskHandle_t xShowMenuTaskHandle;  // ���˵������������ڽ�����ת�ָ�
extern QueueHandle_t g_xQueueMenu;        // ȫ�ְ�����Ϣ���У����հ����ж�����
extern u8g2_t u8g2;                       // ȫ��OLED��Ļ���
extern BaseType_t end_flag;               // ���涯��������־��ȫ�֣�
extern BaseType_t seclect_end;            // �˵�ѡ�ж������ȱ�־

/* some data */
// ���ý���5���˵�ѡ������
const char strs[5][10] = {"<<<", "record", "Sound", "Power", "About"}; 

BaseType_t str_x_pos = 1;                          // �˵�����ͳһX����ʼ����
BaseType_t str_y_pos[] = {11, 23, 36, 49, 62};    // 5���˵�����Y�����꣬��ֱ�Ų�

BaseType_t about_x_pos = 55;                       // ����ҳ������X����
BaseType_t about_y_pos[] = {13, 27, 41, 55};       // ����ҳ��4������Y����

int32_t seclect = 0;                               // ��ǰѡ�еĲ˵��±꣨0~4��
int32_t seclect_h = 13;                            // �˵�ѡ�п�̶��߶�
int32_t seclect_y[6] = {0, 13, 25, 38, 51, 0};     // ÿ���˵�ѡ�п�Y�����꣨���䶯��������
int32_t seclect_w[6] = {24, 44, 37, 37, 37, 24};   // ÿ���˵�ѡ�п����

int width[5] = {0};                                // �洢ÿ���˵����ֵ����ؿ��ȣ�Ԥ��������У�

/* control system sound */
int power_button = 0;                              // ����״̬��־ 0:���� 1:�ر�

/*
 * @brief  �������ý�����UI
 * @note   �������ȫ���˵����֡���̬ѡ�б߿��Ҳ�����װ�ηָ���
 * @param  ��
 * @retval ��
 * @UI���֣��������5�в˵������䶯̬����Ӧѡ�п��Ҳ�̶��ָ�����
 */
void ShowSetiing(void)
{
	// ѭ������5���˵�ѡ������
	for(int i = 0; i<5; i++)
	{
		u8g2_DrawStr(&u8g2, str_x_pos, str_y_pos[i], strs[i]);
		// ��ȡÿ���˵����ֵ����ؿ��ȣ�Ԥ����̬����UI���ܣ�
		width[i] = u8g2_GetStrWidth(&u8g2, &strs[i][10]);		
	}
	// ���Ƶ�ǰ�˵�ѡ�п򣨶�̬���ߡ�λ�ã�����ѡ���±껬����
	u8g2_DrawFrame(&u8g2, 0, seclect_y[0], seclect_w[0], seclect_h);		
	// �����Ҳ�����װ������
	u8g2_DrawBox(&u8g2, 48, seclect_y[0], 5, 12);
	u8g2_DrawFrame(&u8g2, 48, 0, 5, 63);	
}

/**
 * @brief  ���ƹ���ҳ��UI
 * @note   չʾ��Ŀ��л�İ���������Ϣ����Դ��ע
 * @param  ��
 * @retval ��
 */
void ShowAbout(void)
{
	u8g2_DrawStr(&u8g2, about_x_pos, about_y_pos[0],  "thank you");   // ��л����
	u8g2_DrawStr(&u8g2, about_x_pos, about_y_pos[1],  "following");   // ��Ŀ����
	u8g2_DrawStr(&u8g2, about_x_pos, about_y_pos[2],  "my project");  // ��Ŀ��ʶ
	u8g2_DrawStr(&u8g2, about_x_pos, about_y_pos[3],  "@moyiji");     // ��������
}

/**
 * @brief  ���Ʒ°�׿�������ؿؼ�
 * @note   �Զ���OLED����UI��֧�ֿ���/�ر�����״̬�л�
 * @param  switch_status: ����״̬ 0-����  1-�ر�
 * @retval ��
 * @UI�߼����������鿿����ʾON���رջ��鿿�ҡ���ʾOFF
 */
void ShowSwitch(int switch_status)
{
	// ���ƿ�����������������ߣ���ɿ��ص�������
	u8g2_DrawHLine(&u8g2, 70, 22, 40);
	u8g2_DrawHLine(&u8g2, 70, 40, 40);
	
	// ����״̬������ ON
	if(switch_status == 0)
	{
		u8g2_DrawDisc(&u8g2, 70, 31, 9, U8G2_DRAW_ALL);        // ���ʵ�Ļ�������
		u8g2_DrawStr(&u8g2, 100, 34,  "on");                   // ����״̬������ʾ
		// �����Ҳ���İ�Բ�����뿪��Բ������
		u8g2_DrawCircle(&u8g2, 110, 31, 9, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_LOWER_RIGHT);		
	}
	// ����״̬���ر� OFF
	if(switch_status == 1)
	{
		u8g2_DrawDisc(&u8g2, 110, 31, 9, U8G2_DRAW_ALL);       // �Ҳ�ʵ�Ļ�������
		u8g2_DrawStr(&u8g2, 69, 36,  "off");                  // �ر�״̬������ʾ
		// ���������İ�Բ�����뿪��Բ������
		u8g2_DrawCircle(&u8g2, 70, 31, 9, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_LOWER_LEFT);
	}
}

/**
 * @brief  ���ý���FreeRTOS������
 * @note   ���Ӳ����ʼ���������ʼ����������ѯ���˵�������ҳ���л�����������
 * @param  params: �����βΣ�FreeRTOS��׼������������δʹ�ã�
 * @retval ��
 * @�������̣�Ӳ����ʼ�� -> ���д��� -> OLED��ʼ�� -> ����ѭ��ˢ�� -> �����¼����� -> �������� & ҳ����ת
 */
void ShowSetting_Task(void)
{
	PassiveBuzzer_Init();

	/* 消息队列由 InputTask 统一创建，此处不再重复创建 */

	/* OLED��Ļ�ײ��ʼ������������ */
	u8g2_SetFont(&u8g2, u8g2_font_7x13_mf);	// �������ý���ר������
	
	// �״�ȫ��ˢ�£���ʼ����Ļ���棬���������Ӱ
    u8g2_FirstPage(&u8g2);
	do {
	ShowSetiing();
	u8g2_SendBuffer(&u8g2);
   	} while (u8g2_NextPage(&u8g2));

	// Ԥ�������в˵����ֵ����ؿ��ȣ�Ϊ��������ӦUI������׼��
	for(int i;i < 5; i++)
	{
		width[i] = u8g2_GetStrWidth(&u8g2, &strs[i][10]);
	}
	
	struct Key_data	key_data;  // ���尴�����ݽ��սṹ�壬�洢������������

	/* FreeRTOS������ѭ��������ˢ�½��桢���������¼������¶���״̬ */
	while(1)
	{
		/* check exit notification from InputTask */
	if (ulTaskNotifyTake(pdTRUE, 0) > 0)
	{
		vTaskSuspend(NULL);
	}
	u8g2_ClearBuffer(&u8g2);  // ����Դ滺�棬��ֹ������Ӳ�Ӱ
		
		// ���ݵ�ǰѡ�еĲ˵��±꣬��̬��Ⱦ�Ҳ��Ӧ����ҳ��
		switch(seclect)
		{
			case 0: u8g2_DrawStr(&u8g2, 64, 25,"@moyiji");u8g2_DrawStr(&u8g2, 61, 50,"2021/05/27");break; // ��ҳ��Ȩ��Ϣ
			case 1: ShowSwitch(power_button);break; // ¼�����ܿ���ҳ��
			case 2: ShowSwitch(power_button);break; // ϵͳ��Ч����ҳ��
			case 3: ShowSwitch(power_button);break; // ��Դ���ܿ���ҳ��
			case 4: ShowAbout();break;              // �����豸��Ϣҳ��
		}
		ShowSetiing();  // ������ೣפ���˵�UI
		u8g2_SendBuffer(&u8g2);  // ���Դ�����ˢ�µ�OLED��Ļ

		// ����������ȫ����ʱ�������ȴ�������Ϣ�����⶯�����١�������ͻ
		if(seclect_end == 0)
		{
			xQueueReceive(g_xQueueMenu, &key_data, 0); /* 非阻塞，InputTask 管理返回 */
        }

		/******************** �Ҽ��������˵������л� + ƽ������ ********************/
		if(key_data.rdata == 1)
		{
			seclect_end++;  // ����֡������������ƽ������Ч��
			// �����һ���˵���ִ�в˵��»������߼�
			if(seclect!=4)
			{
				// ���ݵ�ǰѡ���±꣬��̬����ѡ�п�����ߡ����꣬ʵ��֡��������
				switch(seclect)
				{
					case 0: ui_run(&seclect_w[0], &seclect_w[1], 1);ui_run(&seclect_y[0], &seclect_y[1], 1);break;
					case 1: ui_run(&seclect_w[0], &seclect_w[2], 1);ui_run(&seclect_y[0], &seclect_y[2], 1);break;
					case 2: ui_run(&seclect_w[0], &seclect_w[3], 1);ui_run(&seclect_y[0], &seclect_y[3], 1);break;
					case 3: ui_run(&seclect_w[0], &seclect_w[4], 1);ui_run(&seclect_y[0], &seclect_y[4], 1);break;
					case 4: ui_run(&seclect_w[0], &seclect_w[5], 1);ui_run(&seclect_y[0], &seclect_y[5], 1);break;				
				}
			}
			// 20֡����ִ����ɣ���ʽ�л��˵�ѡ���±�
			if(seclect_end == 20)
			{
				if(seclect!=4)seclect++;
				seclect_end = 0;    // ���ö���������־
				key_data.rdata = 0; // ��հ���������־����ֹ�ظ�����
			}
			// ������������������������ʾ������Ӧ�ɹ�
			if(seclect_end == 0)
			{
				Buzzer_Beep(2500, 100, 50);
			}	                     
		}

		/******************** ����������˵������л� + ƽ������ ********************/
		else if(key_data.ldata == 1)
		{
			seclect_end++;  // ����֡������
			// �ǵ�һ���˵���ִ�в˵��ϻ������߼�
			if(seclect!=0)
			{
				// ���ݵ�ǰѡ���±꣬����ѡ�п�λ�ã�ʵ���ϻ����ɶ���
				switch(seclect)
				{
					case 0: break;
					case 1: ui_run(&seclect_w[0], &seclect_w[5], 1);ui_run(&seclect_y[0], &seclect_y[5], 1);break;
					case 2: ui_run(&seclect_w[0], &seclect_w[1], 1);ui_run(&seclect_y[0], &seclect_y[1], 1);break;
					case 3: ui_run(&seclect_w[0], &seclect_w[2], 1);ui_run(&seclect_y[0], &seclect_y[2], 1);break;
					case 4: ui_run(&seclect_w[0], &seclect_w[3], 1);ui_run(&seclect_y[0], &seclect_y[3], 1);break;				
				}				
			}
			// 20֡����ִ����ɣ��л���һ���˵�
			if(seclect_end == 20)
			{
				if(seclect!=0)seclect--;
				seclect_end = 0;    // ���ö�����־
				key_data.ldata = 0; // ��հ�����־
			}
			// ������Ӧ������ʾ
			if(seclect_end == 0)
			{
				Buzzer_Beep(2500, 100, 50);
			}			                     			
		}

		/******************** ȷ�ϼ�����������״̬��ת�л� ********************/
		if(key_data.updata == 1)
		{
			Buzzer_Beep(2500, 100, 50);	// ������������
			power_button++;             // ״̬��������
			power_button = power_button%2; // ģ2���㣬ʵ�� 0/1 ѭ����ת��ģ�⿪���л�
		}		

		/******************** ���ؼ��������˳����ý��棬�������˵� ********************/
		/* 返回键由 InputTask 统一处理 */
		if(key_data.exdata == 1)
		{
			Buzzer_Beep(2500, 100, 50);
		}
	}
}
