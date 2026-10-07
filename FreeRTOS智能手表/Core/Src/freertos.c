/* USER CODE BEGIN Header */
#include "driver_led.h"
#include "driver_lcd.h"
#include "driver_mpu6050.h"
#include "driver_timer.h"
#include "driver_ds18b20.h"
#include "driver_dht11.h"
#include "driver_active_buzzer.h"
#include "driver_passive_buzzer.h"
#include "driver_color_led.h"
#include "driver_ir_receiver.h"
#include "driver_ir_sender.h"
#include "driver_light_sensor.h"
#include "driver_ir_obstacle.h"
#include "driver_ultrasonic_sr04.h"
#include "driver_spiflash_w25q64.h"
#include "driver_rotary_encoder.h"
#include "driver_motor.h"
#include "driver_uart.h"

/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications - 智能手表主程序
  *                      输入方式：红外遥控 + 旋转编码器（替代物理按键）
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "event_groups.h"
#include "queue.h"
#include "semphr.h"
#include "u8g2.h"
#include "driver_passive_buzzer.h"
#include "Data.h"
#include "ShowTimeTask.h"
#include "ShowMenuTask.h"
#include "ShowCalanderTask.h"
#include "ShowFlashLigntTask.h"
#include "ShowSettingTask.h"
#include "ShowClockTimeTask.h"
#include "ShowDHT11.h"
#include "ShowWoodenFish.h"
#include "InputTask.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* FreeRTOS 定时器句柄 */
TimerHandle_t g_Timer;
TimerHandle_t g_Clock_Timer;

/* 互斥量：保护定时器变量（秒表/倒计时），防止回调与显示任务竞争 */
SemaphoreHandle_t g_xTimeMutex;

/* 全局共享消息队列（由 InputTask 创建，所有任务共享） */
QueueHandle_t g_xQueueMenu;

/* 各任务句柄（用于 vTaskResume / vTaskSuspend 切换） */
TaskHandle_t xShowTimeTaskHandle = NULL;
TaskHandle_t xShowMenuTaskHandle = NULL;
TaskHandle_t xShowCalendarTaskHandle = NULL;
TaskHandle_t xShowClockTaskHandle = NULL;
TaskHandle_t xShowFlashLightTaskHandle = NULL;
TaskHandle_t xShowSettingTaskHandle = NULL;
TaskHandle_t xShowWoodenFishTaskHandle = NULL;
TaskHandle_t xShowDHT11TaskHandle = NULL;
TaskHandle_t xInputTaskHandle = NULL;

/* 当前活跃的UI任务句柄（由 InputTask 管理，决定输入消息发给谁） */
TaskHandle_t xActiveTaskHandle = NULL;

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* 创建互斥量：保护定时器变量，启用优先级继承 */
  g_xTimeMutex = xSemaphoreCreateMutex();
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* 创建软件定时器：秒表计时（1000ms 周期，自动重载） */
  g_Timer = xTimerCreate("SecTimer", 1000, pdTRUE, NULL, TimerCallBackFun);
  /* 创建软件定时器：倒计时（100ms 周期，自动重载） */
  g_Clock_Timer = xTimerCreate("ClockTimer", 100, pdTRUE, NULL, ClockTimerCallBackFun);

  /* 检查定时器是否创建成功（堆不足会返回 NULL） */
  if (g_Timer == NULL || g_Clock_Timer == NULL)
  {
    /* 定时器创建失败，可通过 LED 闪烁等方式报警 */
    for(;;) { /* 卡死便于调试 */ }
  }
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */

  /* 创建手表各功能任务（初始挂起，由任务切换机制控制） */
  /* 注意：ShowMenuTask 必须先于 ShowTimeTask 创建，因为 ShowTimeTask 启动时会挂起 MenuTask */
  xTaskCreate(ShowMenuTask,      "MenuTask",     192, NULL, 2, &xShowMenuTaskHandle);
  xTaskCreate(ShowCalanderTask,  "CalendarTask", 192, NULL, 2, &xShowCalendarTaskHandle);
  xTaskCreate(ShowClockTimeTask, "ClockTask",    192, NULL, 2, &xShowClockTaskHandle);
  xTaskCreate(ShowFlashLightTask,"FlashTask",    192, NULL, 2, &xShowFlashLightTaskHandle);
  xTaskCreate(ShowSetting_Task,  "SettingTask",  192, NULL, 2, &xShowSettingTaskHandle);
  xTaskCreate(ShowWoodenFishTask,"WoodenTask",   192, NULL, 2, &xShowWoodenFishTaskHandle);
  xTaskCreate(ShowDHT11Task,     "DHT11Task",    192, NULL, 2, &xShowDHT11TaskHandle);
  xTaskCreate(InputTask,         "InputTask",    192, NULL, 2, &xInputTaskHandle);
  xTaskCreate(ShowTimeTask,      "TimeTask",     192, NULL, 2, &xShowTimeTaskHandle);

  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */

/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* 初始化 OLED 显示 */
  u8g2_config();

  /* 启动秒表定时器 */
  if (g_Timer != NULL)
  {
    xTimerStart(g_Timer, 0);
  }

  /* 默认任务完成初始化后挂起自己，由 ShowTimeTask 接管显示 */
  vTaskSuspend(NULL);

  /* Infinite loop */
  for(;;)
  {
    vTaskDelay(1000);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
