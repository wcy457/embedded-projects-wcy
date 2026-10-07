/* �˵����� */
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
#include "ShowMenuTask.h"

/**
 * @brief  蜂鸣器响一声后自动停止
 */
static void Buzzer_Beep(int freq, int duty, int duration_ms)
{
    PassiveBuzzer_Set_Freq_Duty(freq, duty);
    vTaskDelay(duration_ms);
    PassiveBuzzer_Control(0);
}
/* USER CODE END Includes */

extern u8g2_t u8g2;

/* �ⲿ��������������������֮����ͣ/�ָ������л����� */
extern QueueHandle_t g_xQueueMenu;                      // �˵�������Ϣ���о��

/* 任务句柄已由 InputTask 统一管理，此处不再需要 */

/* �˵���Ӧ��ʾ�������ַ������� */
const char str[5][10] = {"cleder", "torch", "hum", "clock", "more"};

/* �˵����Ӧ��ͼ��ṹ���ʼ�� */
str1 fly1 = {"fly1", NULL};
str1 dino1 = {"hum", NULL};
str1 test1 = {"torch", NULL};
str1 block1 = {"clock", NULL};
str1 setting1 = {"setting", NULL};

/* ����ͼ�ꡢ�ؼ�������� */
Image Left = {0, 0, 23, 10};                      // ���Ͻ����ͷͼ������ߴ�
Image Right = {104, 0, 23, 10};                   // ���Ͻ��Ҽ�ͷͼ������ߴ�
Image String = {53, 10, 0, 0};                    // ����������ʾ����
Image Rec_select = {49, 16, 32, 32};              // ѡ��ͼ��������λ�óߴ�

/* �ײ�ԭ�㵼������ */
uint8_t dock_pos = 2;                             // ��ǰѡ�в˵��±꣨Ĭ�ϵ�3����
uint8_t dock_status = 10;                         // �˵�������������ʱ����
uint8_t dock[5] = {45, 55, 65, 75, 85};           // ����ײ�Բ�������
uint8_t dock_y = 58, dock_r = 3;                  // �ײ�Բ�������ꡢ�뾶

int str_flag = 2;                                 // ��ǰ���������±��־
int16_t R_move_pos[5] = {-1, 9, 49, 89, 129};      // �Ҳ໬��ƫ�Ʋ���
BaseType_t select = 3;                            // �˵�ѡ��״̬���

/* ���С�����״̬��־ */
int queue_flag = 0;                              // ��������������־
uint32_t end_flag = 1;                           // �����Ƿ������־
uint32_t seclect_end = 0;                        // ���ý���ѡ�н�����־��Ԥ����

/**
 * @brief  ���Ʋ˵�����UI����
 * @note   �������Ҽ�ͷ���������֡��������ͼ�ꡢ�ײ�����Բ�㡢ѡ�п�
 */
void ShowUI(void)
{
	/* �������Ͻǡ����Ͻ������ƶ���ͷͼ�� */
	u8g2_DrawXBMP(&u8g2, Left.x, Left.y, Left.w, Left.h, LeftMove);
	u8g2_DrawXBMP(&u8g2, Right.x, Right.y, Right.w, Right.h, RightMove);
	
	/* ���ƶ�����ǰ����������ʾ */
	u8g2_DrawStr(&u8g2, String.x, String.y, str[str_flag]);

	/* ����������˵�����ͼ�꣺�������ֵ�Ͳ����ʪ�ȡ���ʱ�������� */
	u8g2_DrawXBMP(&u8g2, cleder.x, cleder.y, cleder.w, cleder.h, cleder.data);
	u8g2_DrawXBMP(&u8g2, torch.x, torch.y, torch.w, torch.h, torch.data);	
	u8g2_DrawXBMP(&u8g2, hum.x, hum.y, hum.w, hum.h, hum.data);
	u8g2_DrawXBMP(&u8g2, clock.x, clock.y, clock.w, clock.h, clock.data);
	u8g2_DrawXBMP(&u8g2, setting.x, setting.y, setting.w, setting.h, setting.data);

	/* ���Ƶײ���ǰѡ��ʵ��Բ�� */
	u8g2_DrawDisc(&u8g2, dock[dock_pos], dock_y, dock_r, U8G2_DRAW_ALL);
	/* ����ȫ���������Բ����� */
	for(int i = 0; i<5; i++)
	{
		u8g2_DrawCircle(&u8g2, dock[i], dock_y, dock_r, U8G2_DRAW_ALL);
	}	
	/* ���Ƶ�ǰѡ��ͼ�������� */
	u8g2_DrawFrame(&u8g2, Rec_select.x, Rec_select.y, Rec_select.w, Rec_select.h);
}

/**
 * @brief  FreeRTOS�˵�����������
 * @param  params�����������(δʹ��)
 * @note   ������ҳUIˢ�¡����Ұ������������������¼��ַ�����ת����Ӧ���ܽ���
 */
void ShowMenuTask(void *param)
{
	/* 初始化蜂鸣器 */
	PassiveBuzzer_Init();

	/* 消息队列由 InputTask 统一创建，此处不再重复创建 */

	/* 首次全屏刷新，初始化屏幕显示 */
	u8g2_FirstPage(&u8g2);
	do {
	u8g2_SendBuffer(&u8g2);
   	} while (u8g2_NextPage(&u8g2));
	
	/* ���尴����Ϣ���սṹ�� */
	struct Key_data	key_data;
		
	while(1)
	{
		/* ���֡���棬׼����һ֡��ͼ */
		u8g2_ClearBuffer(&u8g2);
		/* ���������˵�UI */
		ShowUI();
		/* ˢ�»��浽OLED */
		u8g2_SendBuffer(&u8g2);
		
		/* �������ȴ������������� */
		if(queue_flag == 0)
		{
			pdPASS == xQueueReceive(g_xQueueMenu, &key_data, portMAX_DELAY);
		}
		
		/******************** �Ұ������˵����󻬶����±��С�� ********************/
		if(key_data.rdata == 1)
		{	
			end_flag = 0;
			// ������߽磬ִ�л�������
			if(dock_pos != 0)
			{
				// ����ͼ����������
				ui_right(&cleder.x, 2);
				ui_right(&torch.x, 2);
				ui_right(&hum.x, 2);
				ui_right(&clock.x, 2);
				ui_right(&setting.x, 2);
				
				// ����״̬��λ
				if(dock_status==0)dock_status=1;
				dock_status--;
				
				// ���ݵ�ǰλ��ִ��ͼ������ƫ�ƶ���
				switch(dock_pos)
				{
					case 2:if(dock_status!=0){ui_up(&cleder.y, 1);ui_up(&torch.y, 1);ui_down(&hum.y, 1);ui_down(&clock.y, 1);}break;
				  case 1:if(dock_status!=0){ui_up(&cleder.y, 1);ui_up(&setting.y, 1);ui_down(&torch.y, 1);ui_down(&hum.y, 1);}break;
					case 4:if(dock_status!=0){ui_up(&clock.y, 1);ui_up(&hum.y, 1);ui_down(&setting.y, 1);ui_down(&cleder.y, 1);}break;
					case 3:if(dock_status!=0){ui_up(&hum.y, 1);ui_up(&torch.y, 1);ui_down(&setting.y, 1);ui_down(&clock.y, 1);}break;
				}
			}
       queue_flag++;	
			// ����֡�����꣬��������
			if(queue_flag == 20)
			{
				dock_status=10;
				end_flag = 1;
				if(dock_pos != 0){dock_pos--;str_flag--;}
				queue_flag = 0;
				key_data.rdata = 0;
				key_data.ldata = 0;
			}
			// ������ɷ�����ʾ
			if(end_flag == 1)Buzzer_Beep(2000, 100, 50);
		}
		/******************** �󰴼����˵����һ������±����� ********************/
		else if(key_data.ldata == 1)
		{
      end_flag = 0;
			// �����ұ߽磬ִ�л�������
			if(dock_pos < 4)
			{		
				// ����ͼ����������
				ui_left(&cleder.x, 2);
				ui_left(&torch.x, 2);
				ui_left(&hum.x, 2);
				ui_left(&clock.x, 2);
				ui_left(&setting.x, 2);
				
				if(dock_status==0)dock_status=1;
				dock_status--;
				
				// ��ͬλ��ͼ�����¸�������
				switch(dock_pos)
				{
					case 0:if(dock_status!=0){ui_up(&hum.y, 1);ui_up(&torch.y, 1);ui_down(&cleder.y, 1);ui_down(&setting.y, 1);}break;
				  case 1:if(dock_status!=0){ui_up(&hum.y, 1);ui_up(&clock.y, 1);ui_down(&torch.y, 1);ui_down(&cleder.y, 1);}break;
					case 2:if(dock_status!=0){ui_up(&clock.y, 1);ui_up(&setting.y, 1);ui_down(&hum.y, 1);ui_down(&torch.y, 1);}break;
					case 3:if(dock_status!=0){ui_up(&setting.y, 1);ui_up(&cleder.y, 1);ui_down(&clock.y, 1);ui_down(&hum.y, 1);}break;
				}				
			}
			
            queue_flag++;	
			// ��������������ѡ���±�
			if(queue_flag == 20) 
			{
				dock_status = 10;
				end_flag = 1;
				if(dock_pos < 4){dock_pos++;str_flag++;}
				queue_flag = 0;
				key_data.ldata = 0;
				key_data.rdata = 0;
			}
			if(end_flag == 1)Buzzer_Beep(2000, 100, 50);
		}
		/* 确认键由 InputTask 统一处理（切换到对应功能任务），此处不处理 */
	}
}
