/**
 * @file   InputTask.c
 * @brief  红外遥控 + 旋转编码器输入任务
 * @note   轮询输入设备，发送到共享队列，管理任务切换
 *
 *  输入映射：
 *    旋转编码器旋转 → rdata/ldata (菜单导航 / 功能内部操作)
 *    旋转编码器按键 → updata (菜单确认 / 功能内部操作)
 *    红外遥控 左右/确认 → 菜单导航 / 功能内部操作
 *    红外遥控 返回 → exdata (退出功能，返回菜单)
 */
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "queue.h"

#include "driver_ir_receiver.h"
#include "driver_rotary_encoder.h"
#include "driver_passive_buzzer.h"
#include "Data.h"

/**
 * @brief  蜂鸣器响一声后自动停止
 * @param  freq: 频率  duty: 占空比  duration_ms: 持续时间(ms)
 */
static void Buzzer_Beep(int freq, int duty, int duration_ms)
{
    PassiveBuzzer_Set_Freq_Duty(freq, duty);
    vTaskDelay(duration_ms);
    PassiveBuzzer_Control(0);
}

/* 共享消息队列 */
extern QueueHandle_t g_xQueueMenu;

/* 各任务句柄 */
extern TaskHandle_t xShowTimeTaskHandle;
extern TaskHandle_t xShowMenuTaskHandle;
extern TaskHandle_t xShowCalendarTaskHandle;
extern TaskHandle_t xShowClockTaskHandle;
extern TaskHandle_t xShowFlashLightTaskHandle;
extern TaskHandle_t xShowSettingTaskHandle;
extern TaskHandle_t xShowWoodenFishTaskHandle;
extern TaskHandle_t xShowDHT11TaskHandle;

/* 当前活跃的任务句柄 */
extern TaskHandle_t xActiveTaskHandle;

/* 菜单当前选中的项目（由 ShowMenuTask 维护） */
extern uint8_t dock_pos;

/**
 * @brief  清空队列中的所有待处理消息
 */
static void FlushQueue(QueueHandle_t xQueue)
{
    Key_data dummy;
    while (xQueueReceive(xQueue, &dummy, 0) == pdPASS)
    {
        /* 丢弃 */
    }
}

/**
 * @brief  切换到指定任务
 * @note   更新活跃任务句柄，清空队列，挂起当前任务，恢复目标任务
 *         切换回菜单时发送一条空消息唤醒菜单任务，避免阻塞
 */
static void SwitchToTask(TaskHandle_t xTarget)
{
    TaskHandle_t xOld = xActiveTaskHandle;
    xActiveTaskHandle = xTarget;
    vTaskResume(xTarget);

    if (xOld == xShowMenuTaskHandle && xTarget != xShowMenuTaskHandle)
    {
        /* entering feature: drain stale notification, flush queue, suspend menu */
        ulTaskNotifyTake(pdTRUE, 0);
        FlushQueue(g_xQueueMenu);
        vTaskSuspend(xOld);
    }
    else if (xTarget == xShowMenuTaskHandle && xOld != xShowMenuTaskHandle)
    {
        /* exiting feature: notify feature task (so it can cleanup),
           resume menu, suspend feature */
        xTaskNotify(xOld, 1, eSetValueWithOverwrite);
        vTaskSuspend(xOld);
    }
}

void InputTask(void *params)
{
    /* 创建共享队列 */
    g_xQueueMenu = xQueueCreate(8, sizeof(Key_data));
    if (g_xQueueMenu != NULL)
    {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
    }

    /* 挂起所有 UI 任务，由 InputTask 统一管理切换 */
    vTaskSuspend(xShowCalendarTaskHandle);
    vTaskSuspend(xShowFlashLightTaskHandle);
    vTaskSuspend(xShowDHT11TaskHandle);
    vTaskSuspend(xShowClockTaskHandle);
    vTaskSuspend(xShowSettingTaskHandle);
    vTaskSuspend(xShowTimeTaskHandle);
    vTaskSuspend(xShowWoodenFishTaskHandle);

    xActiveTaskHandle = xShowMenuTaskHandle;

    IRReceiver_Init();
    RotaryEncoder_Init();
    PassiveBuzzer_Init();

    uint8_t dev, data;
    int32_t cnt, speed, key;
    Key_data key_data;

    /* 调试：编码器中断回调中已加 LED 翻转，此处恢复正常逻辑 */

    while (1)
    {
        key_data.rdata  = 0;
        key_data.ldata  = 0;
        key_data.updata = 0;
        key_data.exdata = 0;

        /* 红外遥控器 */
        if (IRReceiver_Read(&dev, &data) == 0)
        {
            if (dev == 0 && data == 0)
                goto check_encoder;

            switch (data)
            {
                case 0xE2: case 0xC2: key_data.ldata = 1;  break;
                case 0x22: case 0x90: key_data.rdata = 1;  break;
                case 0x02: case 0x62: key_data.updata = 1; break;
                case 0xA2: case 0x98: key_data.exdata = 1; break;
                default: break;
            }

            if (key_data.rdata || key_data.ldata || key_data.updata || key_data.exdata)
                Buzzer_Beep(2000, 100, 50);
        }

check_encoder:
        /* 旋转编码器 */
        RotaryEncoder_Read(&cnt, &speed, &key);

        if (speed > 0)      { key_data.rdata = 1; Buzzer_Beep(1500, 80, 30); }
        else if (speed < 0) { key_data.ldata = 1; Buzzer_Beep(1500, 80, 30); }

        if (key == 1)
        {
            key_data.updata = 1;
            Buzzer_Beep(2500, 100, 50);
            while (key == 1) { vTaskDelay(10); RotaryEncoder_Read(&cnt, &speed, &key); }
        }

        /* ---------- 输入处理 ---------- */
        if (key_data.rdata || key_data.ldata || key_data.updata || key_data.exdata)
        {
            if (xActiveTaskHandle == xShowMenuTaskHandle)
            {
                /*
                 * 菜单活跃：
                 *   左右 → 发送到队列，菜单导航
                 *   确认 → 根据 dock_pos 切换到对应功能任务
                 *   返回 → 忽略（已在菜单）
                 */
                if (key_data.updata == 1)
                {
                    /* 确认键：切换到对应功能任务 */
                    TaskHandle_t target = NULL;
                    switch (dock_pos)
                    {
                        case 0: target = xShowCalendarTaskHandle;   break;
                        case 1: target = xShowFlashLightTaskHandle; break;
                        case 2: target = xShowDHT11TaskHandle;     break;
                        case 3: target = xShowClockTaskHandle;  break;
                        case 4: target = xShowSettingTaskHandle;    break;
                    }
                    if (target != NULL)
                    {
                        SwitchToTask(target);
                    }
                }
                else if (key_data.rdata || key_data.ldata)
                {
                    /* 左右键：发送给菜单导航 */
                    xQueueSend(g_xQueueMenu, &key_data, 0);
                }
            }
            else
            {
                /*
                 * 功能任务活跃：
                 *   所有输入先转发给功能任务（编码器旋转/按键、红外左右/确认）
                 *   红外返回键(exdata) 额外触发切回菜单
                 */
                if (key_data.exdata == 1)
                {
                    /* exit: notify feature task via task notification, not queue */
                    SwitchToTask(xShowMenuTaskHandle);
                }
                else
                {
                    /* forward non-exit keys to feature task */
                    xQueueSend(g_xQueueMenu, &key_data, 0);
                }
            }
        }

        vTaskDelay(20);
    }
}
