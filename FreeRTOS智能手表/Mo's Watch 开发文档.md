# Mo's Watch 开发文档

## 一、项目概述

Mo's Watch 是一个基于 **STM32F103** 的智能手表项目，使用 **FreeRTOS** 实时操作系统进行任务调度，**u8g2** 库驱动 **SSD1306 OLED** 显示屏（128×64，I2C），配合 4 个按键、无源蜂鸣器和 DHT11 温湿度传感器。

### 硬件清单
| 硬件 | 说明 |
|------|------|
| STM32F103C8T6 | 主控芯片 |
| SSD1306 OLED | 128×64 像素，I2C 接口 |
| 4 个按键 | 分别接 PA0、PA1、PA10、PA11 |
| 无源蜂鸣器 | 用于按键音和提示音 |
| DHT11 | 温湿度传感器 |
| LED (PC13) | 状态指示灯 |

### 按键定义
| GPIO 引脚 | 功能 | 变量名 |
|-----------|------|--------|
| PA11 (PIN_11) | 右移 / 下一项 | `key_data.rdata` |
| PA10 (PIN_10) | 左移 / 上一项 | `key_data.ldata` |
| PA1 (PIN_1) | 确认 / 切换 | `key_data.updata` |
| PA0 (PIN_0) | 返回 | `key_data.exdata` |

---

## 二、整体架构

### 2.1 任务总览

```
┌─────────────────────────────────────────────────────┐
│                   FreeRTOS 调度器                      │
├──────────┬──────────┬──────────┬──────────┬──────────┤
│ShowTimeTask│ShowMenuTask│ShowCalendar│ShowClock │ShowFlash │
│  (表盘)    │  (菜单)     │  (日历)     │ (倒计时)  │ (手电筒)  │
├──────────┼──────────┼──────────┼──────────┼──────────┤
│ShowDHT11 │ShowSetting│WoodenFish │          │          │
│ (温湿度)   │ (设置)     │ (木鱼)      │          │          │
└──────────┴──────────┴──────────┴──────────┴──────────┘
```

### 2.2 任务间通信机制

```
                    ┌──────────────┐
   按键中断 ───────►│  消息队列      │──────► 当前活跃任务
  (HAL_GPIO_       │ g_xQueueMenu │        (xQueueReceive)
   EXTI_Callback)  └──────────────┘
                         │
                         ▼
                  ┌──────────────┐
                  │  Key_data 结构体│
                  │  rdata  - 右   │
                  │  ldata  - 左   │
                  │  updata - 确认  │
                  │  exdata - 返回  │
                  └──────────────┘
```

**关键设计**：所有任务共享同一个消息队列 `g_xQueueMenu`，哪个任务当前在运行，就由哪个任务接收按键数据。

### 2.3 任务切换机制（核心！）

本项目使用 **vTaskSuspend / vTaskResume** 实现任务切换，这是最简单直观的方式：

```
当前任务 A 运行中
    │
    ├── 收到"切换"按键
    │
    ├── vTaskResume(目标任务B的Handle)  ← 唤醒目标任务
    │
    └── vTaskSuspend(NULL)             ← 挂起自己（NULL 表示挂起当前任务）
    
目标任务 B 被唤醒，继续运行
```

---

## 三、数据结构详解 (Data.h / Data.c)

### 3.1 核心结构体

```c
/* UI 元素结构体 —— 用于存储图标信息 */
typedef struct UI {
    const char name[20];    // 图标名称（如 "cleder", "torch"）
    uint8_t num;            // 图标编号
    const uint8_t data[128]; // 图标位图数据（XBM 格式）
    int32_t x;              // 当前 x 坐标
    int32_t y;              // 当前 y 坐标
    int32_t w;              // 宽度（像素）
    int32_t h;              // 高度（像素）
} ui;

/* 图片区域结构体 —— 用于绘制区域 */
typedef struct Images {
    int x;   // x 坐标
    int y;   // y 坐标
    int w;   // 宽度
    int h;   // 高度
} Image;

/* 按键数据结构体 */
typedef struct Key_data {
    uint8_t rdata;    // 右移标志
    uint8_t ldata;    // 左移标志
    uint8_t updata;   // 确认标志
    uint8_t exdata;   // 返回标志
} Key_data;
```

### 3.2 位图数据说明

所有图标和数字都以 `const uint8_t` 数组形式存储在 Flash 中（节省 RAM）：

```c
// 大数字 0-9，每个 120 字节，用于时间显示
extern const uint8_t BigNum[10][120];

// 小数字 0-9 + 冒号 + 箭头，每个 8 字节
extern const uint8_t Num_6x8[12][8];

// 木鱼两帧动画，每帧 384 字节
extern const uint8_t wooden_flsh[2][384];

// 功德文字图标 64 字节
extern const uint8_t gongde[64];

// 锤子图标 40 字节
extern const uint8_t hammer[40];
```

### 3.3 菜单图标定义

5 个菜单图标用 `ui` 结构体定义，包含位置信息用于动画：

```c
ui cleder   = {"cleder",   1, {...}, -30, 37, 30, 30};  // 日历
ui torch    = {"torch",    2, {...},   9, 27, 30, 30};  // 手电筒
ui hum      = {"hum",      3, {...},  49, 17, 30, 30};  // 温湿度
ui clock    = {"clock",    4, {...},  89, 27, 30, 30};  // 倒计时
ui setting  = {"setting",  5, {...}, 129, 37, 30, 30};  // 设置
```

### 3.4 动画辅助函数

```c
/* 平滑移动函数：当前值 a 向目标值 a_trg 移动，步长为 b */
void ui_run(int* a, int* a_trg, int b) {
    if (*a < *a_trg) { *a += b; if (*a > *a_trg) *a = *a_trg; }
    if (*a > *a_trg) { *a -= b; if (*a < *a_trg) *a = *a_trg; }
}

/* 方向移动函数 */
void ui_left(int32_t* a, int b);   // 向左移动
void ui_right(int32_t* a, int b);  // 向右移动
void ui_up(int32_t* a, int b);     // 向上移动
void ui_down(int32_t* a, int b);   // 向下移动
```

---

## 四、各任务编写逻辑详解

### 4.1 ShowTimeTask（表盘任务）—— 主界面

**功能**：显示当前时间，大字体显示分秒，小字体显示小时。

**编写逻辑**：

```
1. 初始化
   ├── 初始化蜂鸣器 buzzer_init()
   ├── 挂起其他所有任务（防止冲突）
   ├── 创建消息队列 g_xQueueMenu
   └── 配置 u8g2 显示屏，设置大字体 u8g2_font_fur35_tf

2. 主循环 while(1)
   ├── 清屏 u8g2_ClearBuffer()
   ├── 绘制顶部图标（电源、游戏图标）
   ├── 绘制大数字（秒个位、秒十位、分个位、分十位）
   ├── 绘制冒号分隔符（两个圆角方块）
   ├── 绘制小数字（小时）
   ├── 发送到屏幕 u8g2_SendBuffer()
   ├── 延时 vTaskDelay(250)  ← 250ms 刷新一次
   ├── 接收队列消息（非阻塞）
   └── 如果收到 updata（确认键）
       ├── 蜂鸣器响
       ├── vTaskResume(xShowMenuTaskHandle)  ← 唤醒菜单任务
       └── vTaskSuspend(NULL)                ← 挂起自己
```

**时间更新**：由 FreeRTOS 软件定时器 `g_Timer`（1秒周期）调用 `TimerCallBackFun()` 自动递增：

```c
void TimerCallBackFun(TimerHandle_t xTimer) {
    sec_unit++;                          // 秒个位 +1
    if (sec_unit > 9) { sec_unit = 0; sec_decade++; }  // 进位
    if (sec_decade > 5) { sec_decade = 0; min_unit++; }
    if (min_unit > 9) { min_unit = 0; min_decade++; }
    if (min_decade > 5) { min_decade = 0; hour_unit++; }
    if (hour_unit > 5) { hour_unit = 0; hour_decade++; }
}
```

**关键点**：
- 时间数据用 6 个全局变量存储：`sec_unit`, `sec_decade`, `min_unit`, `min_decade`, `hour_unit`, `hour_decade`
- 每个变量范围 0-9（个位）或 0-5（十位），通过进位逻辑实现计时
- 屏幕刷新 4Hz（250ms），时间更新 1Hz（定时器）

---

### 4.2 ShowMenuTask（菜单任务）—— iOS 风格滑动菜单

**功能**：显示 5 个应用图标，支持左右滑动选择，确认进入应用。

**编写逻辑**：

```
1. 初始化
   ├── 初始化蜂鸣器
   ├── 创建消息队列
   ├── 配置 u8g2，设置字体
   └── 初始化 UI 元素位置

2. 主循环 while(1)
   ├── 清屏
   ├── 调用 ShowUI() 绘制：
   │   ├── 左右箭头指示器
   │   ├── 当前选中项名称文字
   │   ├── 5 个图标（cleder, torch, hum, clock, setting）
   │   ├── 底部圆点导航指示器
   │   └── 选中框（方框）
   ├── 发送到屏幕
   ├── 接收队列消息
   │
   ├── 处理右键 rdata（向右滑动）
   │   ├── 所有图标 x 坐标 += 2（ui_right）
   │   ├── 选中项上下移动动画（状态机控制）
   │   ├── 累计 20 次后完成一次滑动
   │   │   ├── dock_pos--（选中项索引）
   │   │   └── str_flag--（名称索引）
   │   └── 蜂鸣器响
   │
   ├── 处理左键 ldata（向左滑动）
   │   ├── 所有图标 x 坐标 -= 2（ui_left）
   │   ├── 类似的动画逻辑
   │   └── 累计 20 次后 dock_pos++，str_flag++
   │
   ├── 处理确认键 exdata（进入应用）
   │   └── switch(dock_pos)
   │       ├── case 0: 唤醒 ShowCalendarTask，挂起自己
   │       ├── case 1: 唤醒 ShowFlashLightTask，挂起自己
   │       ├── case 2: 唤醒 ShowDHT11Task，挂起自己
   │       ├── case 3: 唤醒 ShowClockTask，挂起自己
   │       └── case 4: 唤醒 ShowSettingTask，挂起自己
   │
   └── 处理返回键 updata（回到表盘）
       ├── 唤醒 ShowTimeTask
       └── 挂起自己
```

**动画原理**：
- 每收到一次按键，图标移动 2 像素
- 需要累计 20 次（共移动 40 像素）才算完成一次完整的滑动
- 中间过程用 `queue_flag` 计数，`dock_status` 控制上下弹跳动画
- `end_flag` 用于防止动画未完成时接收新按键

**关键变量**：
```c
uint8_t dock_pos = 2;       // 当前选中项索引（0-4）
uint8_t dock_status = 10;   // 动画状态机
int str_flag = 2;           // 当前显示的名称索引
int queue_flag = 0;         // 动画帧计数
uint32_t end_flag = 1;      // 动画完成标志（1=可接收按键）
```

---

### 4.3 ShowCalendarTask（日历任务）

**功能**：显示 2024 年某月的日历，支持按月切换。

**编写逻辑**：

```
1. 初始化
   ├── 蜂鸣器、消息队列、u8g2 配置
   └── 设置小字体 u8g2_font_spleen5x8_mf

2. 主循环 while(1)
   ├── 清屏
   ├── 绘制星期头（Su Mo Tu We Th Fr Sa）
   ├── 计算当前月第一天是星期几
   │   ├── judge_week(2024) 计算 2024.1.1 是星期几
   │   └── 逐月累加天数取模得到当前月第一天
   ├── 绘制日期数字（1-28/29/30/31）
   ├── 绘制右侧月份指示条
   ├── 发送到屏幕
   ├── 阻塞等待按键
   │
   ├── 右键 rdata：切换到下一个月
   │   ├── month++（超过 12 回到 1）
   │   └── line_pos++（指示条位置）
   │
   └── 返回键 exdata：返回菜单
       ├── 唤醒 ShowMenuTask
       └── 挂起自己
```

**日期计算核心算法**：
```c
// 判断闰年
int judge_year(int year) {
    return (year % 400 == 0 || (year % 4 == 0 && year % 100 != 0));
}

// 计算某年 1 月 1 日是星期几
int judge_week(int year) {
    int sum = 0;
    for (int i = 1; i < year; i++) {
        sum += judge_year(i) ? 366 : 365;
    }
    return (sum + 1) % 7;  // 0=周日, 1=周一, ...
}
```

---

### 4.4 ShowClockTimeTask（倒计时任务）

**功能**：设置倒计时时间，到时蜂鸣器报警。

**编写逻辑**：

```
1. 初始化
   ├── 蜂鸣器、消息队列、u8g2 配置
   └── 设置中文字体 u8g2_font_wqy16_t_chinese1

2. 主循环 while(1)
   ├── 清屏
   ├── 调用 ShowClock() 绘制：
   │   ├── "Set:" + 4 位设置时间 + ">" 确认按钮
   │   ├── "Ret:" + 4 位实际计时
   │   ├── 右侧圆形表盘 + 中心数字
   │   └── 选中框（高亮当前编辑位）
   ├── 发送到屏幕
   ├── 接收队列消息
   │
   ├── 右键/左键：移动选中位（seclect_flag 0-4）
   │   └── 0-3 对应 4 个数字位，4 对应 ">" 确认按钮
   │
   ├── 确认键 updata：
   │   ├── 如果选中 ">"（seclect_flag == 4）
   │   │   ├── 启动倒计时定时器 g_Clock_Timer（100ms 周期）
   │   │   └── clock_flag = 1（进入计时模式）
   │   └── 否则：当前选中位数字 +1（0-9 循环）
   │
   ├── 返回键 exdata：
   │   ├── 停止定时器
   │   └── 返回菜单
   │
   └── 倒计时运行中（clock_flag == 1）
       └── 如果设置时间 == 实际时间
           ├── 停止定时器
           └── 蜂鸣器响 1 秒报警
```

**定时器回调**（100ms 周期）：
```c
void ClockTimerCallBackFun(TimerHandle_t xTimer) {
    millisecond++;  // 0.1 秒
    if (millisecond > 9) {
        millisecond = 0;
        g_real_time[3]++;           // 秒个位
        if (g_real_time[3] > 9) {   // 进位逻辑...
            g_real_time[2]++;       // 秒十位
            // ... 依次进位到分十位
        }
    }
}
```

---

### 4.5 ShowFlashLightTask（手电筒任务）

**功能**：按确认键切换全屏点亮/熄灭，最简单的任务。

**编写逻辑**：

```
1. 初始化
   ├── 蜂鸣器、消息队列、u8g2 配置
   └── 绘制手电筒图标并显示

2. 主循环 while(1)
   ├── 清屏
   ├── 阻塞等待按键
   │
   ├── 确认键 updata：
   │   └── switch(light_flag)
   │       ├── case 0: u8g2_DrawBox 全屏填充（亮）→ light_flag = 1
   │       └── case 1: 绘制手电筒图标（灭）→ light_flag = 0
   │
   ├── 返回键 exdata：
   │   ├── 唤醒菜单任务
   │   └── 挂起自己
   │
   └── 发送到屏幕
```

**关键函数**：
```c
u8g2_DrawBox(&u8g2, 0, 0, 128, 64);  // 填充矩形（全屏白）
u8g2_DrawXBMP(...);                    // 绘制位图图标
```

---

### 4.6 ShowSettingTask（设置任务）

**功能**：设置菜单，包含多个选项，支持上下选择和开关切换。

**编写逻辑**：

```
1. 初始化
   ├── 蜂鸣器、消息队列、u8g2 配置
   └── 设置字体 u8g2_font_7x13_mf

2. 主循环 while(1)
   ├── 清屏
   │
   ├── 根据当前选项 seclect 绘制右侧内容：
   │   ├── case 0: 显示作者信息 "@moyiji" 和日期
   │   ├── case 1-3: 显示开关控件 ShowSwitch()
   │   └── case 4: 显示关于页面 ShowAbout()
   │
   ├── 调用 ShowSetiing() 绘制左侧菜单：
   │   ├── 5 个选项文字（<<<, record, Sound, Power, About）
   │   ├── 选中高亮框（带动画）
   │   └── 右侧滚动条
   │
   ├── 发送到屏幕
   ├── 接收队列消息
   │
   ├── 右键：向下选择
   │   ├── ui_run() 动画移动选中框
   │   └── 累计 20 次后 seclect++
   │
   ├── 左键：向上选择
   │   └── 类似逻辑，seclect--
   │
   ├── 确认键：切换开关状态
   │   └── power_button = (power_button + 1) % 2
   │
   └── 返回键：返回菜单
```

---

### 4.7 ShowWoodenFishTask（木鱼任务）

**功能**：点击敲木鱼，累计功德数，带动画效果。

**编写逻辑**：

```
1. 初始化
   ├── 蜂鸣器、消息队列、u8g2 配置
   └── 设置中文字体

2. 主循环 while(1)
   ├── 清屏
   │
   ├── 绘制功德计数（3 位数字 Num_6x8）
   │   ├── num1（百位）、num2（十位）、num3（个位）
   │   └── 位置：x=80, 88, 96  y=0
   │
   ├── 绘制木鱼动画（switch seclect_flag）
   │   ├── case 0: 木鱼静止图 + 锤子在上方
   │   └── case 1: 木鱼被敲图 + 锤子在下方 → 重置为 0
   │
   ├── 绘制功德文字图标和冒号
   ├── 发送到屏幕
   ├── 阻塞等待按键
   │
   ├── 右键 rdata（敲木鱼）：
   │   ├── 蜂鸣器响（3000Hz, 200ms）
   │   ├── 功德数 +1（num3→num2→num1 进位）
   │   ├── seclect_flag = 1（播放敲击动画）
   │   └── add_flag = 100（显示 "+1" 提示）
   │
   ├── 返回键 exdata：返回菜单
   │
   ├── 第二次清屏，绘制动画帧：
   │   ├── 如果 add_flag > 0，显示 "+1"
   │   └── 绘制木鱼和锤子
   └── 发送到屏幕
```

**关键点**：每次循环绘制两次——第一次绘制完整界面，第二次绘制动画帧，实现敲击效果。

---

## 五、从零开始写一个新任务的步骤

假设你要添加一个新应用 **"ShowBatteryTask"（电池电量显示）**：

### 步骤 1：创建文件

创建 `ShowBattery.h` 和 `ShowBattery.c`：

```c
// ============ ShowBattery.h ============
#ifndef __SHOWBATTERY_H__
#define __SHOWBATTERY_H__

void ShowBatteryTask(void *params);

#endif
```

### 步骤 2：编写 .c 文件框架

```c
// ============ ShowBattery.c ============
/* 1. 头文件包含（照抄其他任务） */
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "event_groups.h"
#include "queue.h"

#include "beep.h"      // 蜂鸣器
#include "u8g2.h"       // OLED 显示
#include "Data.h"       // 公共数据结构

/* 2. 外部变量声明 */
extern TaskHandle_t xShowMenuTaskHandle;    // 菜单任务句柄
extern QueueHandle_t g_xQueueMenu;          // 消息队列
extern u8g2_t u8g2;                         // u8g2 实例

/* 3. 任务函数 */
void ShowBatteryTask(void *params)
{
    /* ===== 初始化部分 ===== */
    buzzer_init();                           // 初始化蜂鸣器
    
    // 创建消息队列
    g_xQueueMenu = xQueueCreate(1, 4);
    if (NULL != g_xQueueMenu)
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
    
    // 配置 u8g2
    u8g2_t u8g2;
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(
        &u8g2, U8G2_R0, u8x8_byte_hw_i2c, u8g2_stm32_delay);
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
    u8g2_ClearDisplay(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_fur35_tf);  // 选择字体
    
    /* ===== 变量声明 ===== */
    struct Key_data key_data;
    
    /* ===== 主循环 ===== */
    while (1)
    {
        // (1) 清屏
        u8g2_ClearBuffer(&u8g2);
        
        // (2) 绘制你的内容
        // TODO: 在这里画电池图标和电量
        // u8g2_DrawXBMP(&u8g2, x, y, w, h, bitmap_data);
        // u8g2_DrawStr(&u8g2, x, y, "text");
        
        // (3) 发送到屏幕
        u8g2_SendBuffer(&u8g2);
        
        // (4) 接收按键（阻塞等待）
        xQueueReceive(g_xQueueMenu, &key_data, portMAX_DELAY);
        
        // (5) 处理按键
        if (key_data.exdata == 1)  // 返回键
        {
            buzzer_buzz(2500, 100);
            vTaskResume(xShowMenuTaskHandle);  // 唤醒菜单
            vTaskSuspend(NULL);                 // 挂起自己
        }
        
        // (6) 清除按键标志
        key_data.rdata = 0;
        key_data.ldata = 0;
        key_data.updata = 0;
        key_data.exdata = 0;
    }
}
```

### 步骤 3：注册任务

在 `Core/Src/freertos.c` 中：

```c
// 1. 添加头文件包含
#include "ShowBattery.h"

// 2. 添加任务句柄
TaskHandle_t xShowBatteryTaskHandle = NULL;

// 3. 在 MX_FREERTOS_Init() 中创建任务
xTaskCreate(ShowBatteryTask, "ShowBatteryTask", 128, 
            NULL, osPriorityNormal, &xShowBatteryTaskHandle);
```

### 步骤 4：添加到菜单

在 `ShowMenu.c` 中：

```c
// 1. 添加外部句柄声明
extern TaskHandle_t xShowBatteryTaskHandle;

// 2. 在 exdata 处理的 switch 中添加 case
// 假设你想把它放在 dock_pos == 5 的位置
case 5: 
    vTaskResume(xShowBatteryTaskHandle); 
    vTaskSuspend(NULL); 
    break;
```

### 步骤 5：在任务内部添加返回菜单的逻辑

在你的任务中，当用户按返回键时：
```c
if (key_data.exdata == 1)
{
    buzzer_buzz(2500, 100);
    vTaskResume(xShowMenuTaskHandle);
    vTaskSuspend(NULL);
}
```

---

## 六、u8g2 常用绘图函数速查

### 6.1 基础绘图

```c
// 清空缓冲区（每次绘制前调用）
u8g2_ClearBuffer(&u8g2);

// 将缓冲区内容发送到屏幕（每次绘制后调用）
u8g2_SendBuffer(&u8g2);

// 设置字体
u8g2_SetFont(&u8g2, u8g2_font_fur35_tf);       // 大字体
u8g2_SetFont(&u8g2, u8g2_font_7x13_mf);         // 中等字体
u8g2_SetFont(&u8g2, u8g2_font_spleen5x8_mf);    // 小字体
u8g2_SetFont(&u8g2, u8g2_font_wqy16_t_chinese1); // 中文字体
```

### 6.2 文字绘制

```c
// 绘制字符串，返回字符串宽度（像素）
int width = u8g2_DrawStr(&u8g2, x, y, "Hello");

// 获取字符串宽度（不绘制）
int width = u8g2_GetStrWidth(&u8g2, "Hello");
```

### 6.3 图形绘制

```c
// 画点
u8g2_DrawPixel(&u8g2, x, y);

// 画线
u8g2_DrawLine(&u8g2, x1, y1, x2, y2);
u8g2_DrawHLine(&u8g2, x, y, len);     // 水平线
u8g2_DrawVLine(&u8g2, x, y, len);     // 垂直线

// 画矩形
u8g2_DrawFrame(&u8g2, x, y, w, h);    // 空心矩形
u8g2_DrawBox(&u8g2, x, y, w, h);      // 实心矩形
u8g2_DrawRFrame(&u8g2, x, y, w, h, r); // 圆角空心矩形
u8g2_DrawRBox(&u8g2, x, y, w, h, r);   // 圆角实心矩形

// 画圆
u8g2_DrawCircle(&u8g2, x, y, r, U8G2_DRAW_ALL);  // 空心圆
u8g2_DrawDisc(&u8g2, x, y, r, U8G2_DRAW_ALL);     // 实心圆

// 画位图（XBM 格式）
u8g2_DrawXBMP(&u8g2, x, y, w, h, bitmap_data);
```

### 6.4 分页模式（大缓冲区用）

```c
u8g2_FirstPage(&u8g2);
do {
    // 在这里绘图
    ShowUI();
} while (u8g2_NextPage(&u8g2));
```

---

## 七、FreeRTOS API 速查

### 7.1 任务管理

```c
// 创建任务
xTaskCreate(任务函数, "名称", 栈大小, 参数, 优先级, &句柄);

// 挂起任务
vTaskSuspend(句柄);      // 挂起指定任务
vTaskSuspend(NULL);      // 挂起当前任务

// 恢复任务
vTaskResume(句柄);       // 恢复指定任务

// 延时
vTaskDelay(ticks);       // 延时（单位：系统节拍）
vTaskDelay(250);         // 延时 250ms（如果 tick=1ms）
```

### 7.2 消息队列

```c
// 创建队列
QueueHandle_t queue = xQueueCreate(队列长度, 每项大小);

// 发送（任务中）
xQueueSendToBack(queue, &data, ticks_to_wait);

// 发送（中断中）
xQueueSendToBackFromISR(queue, &data, NULL);

// 接收
xQueueReceive(queue, &data, ticks_to_wait);
// ticks_to_wait = portMAX_DELAY 表示永久阻塞
// ticks_to_wait = 0 表示非阻塞
```

### 7.3 软件定时器

```c
// 创建定时器
TimerHandle_t timer = xTimerCreate(
    "名称",           // 名称
    周期ticks,        // 周期
    pdTRUE,           // 自动重载
    NULL,             // ID
    回调函数           // TimerCallbackFunction_t
);

// 启动/停止
xTimerStart(timer, 0);
xTimerStop(timer, 0);
```

---

## 八、常见问题与调试技巧

### 8.1 屏幕不显示
- 检查 I2C 地址是否正确（SSD1306 通常是 0x3C 或 0x78）
- 确认 `u8g2_SetPowerSave(&u8g2, 0)` 已调用
- 确认 `u8g2_SendBuffer()` 在绘图之后调用

### 8.2 按键无响应
- 检查 GPIO 中断是否配置（CubeMX 中 EXTI）
- 确认队列已创建且未满
- 检查 `end_flag` 和 `seclect_end` 状态

### 8.3 任务卡死
- 可能是 `xQueueReceive` 用了 `portMAX_DELAY` 但队列为空
- 检查是否有任务忘记 `vTaskResume` 其他任务
- 栈溢出：增大栈大小（`xTaskCreate` 的第三个参数）

### 8.4 内存不足
- 位图数据用 `const` 修饰，存储在 Flash 而非 RAM
- 减小任务栈大小（最小 128 words = 512 bytes）
- 减少同时运行的任务数量

---

## 九、项目文件结构

```
Mo's Watch/
├── Core/
│   ├── Inc/            ← 头文件
│   └── Src/
│       ├── main.c      ← 主函数
│       ├── freertos.c  ← FreeRTOS 初始化、任务创建、按键中断
│       ├── gpio.c      ← GPIO 配置
│       ├── i2c.c       ← I2C 配置
│       └── tim.c       ← 定时器配置
├── Drivers/            ← HAL 库驱动
├── Middlewares/         ← FreeRTOS 中间件
├── driver/             ← 自定义外设驱动
│   ├── beep.c/h        ← 蜂鸣器驱动
│   ├── driver_dht11.c/h ← DHT11 驱动
│   ├── driver_passive_buzzer.c/h ← 无源蜂鸣器驱动
│   └── driver_timer.c/h ← 定时器驱动
├── u8g2/               ← u8g2 显示库
└── mytasks/            ← 应用任务（你要写的代码在这里）
    ├── Data.c/h        ← 公共数据、图标、动画函数
    ├── ShowTimeTask.c/h ← 表盘任务
    ├── ShowMenu.c/h     ← 菜单任务
    ├── ShowCalendar.c/h ← 日历任务
    ├── ShowClock.c/h    ← 倒计时任务
    ├── ShowFlashLight.c/h ← 手电筒任务
    ├── ShowSetting.c/h  ← 设置任务
    ├── ShowWoodenFish.c/h ← 木鱼任务
    └── ShowDHT11.c/h   ← 温湿度任务
```

---

## 十、编写新任务的检查清单

- [ ] 创建 `.h` 和 `.c` 文件
- [ ] 包含必要的头文件（FreeRTOS, u8g2, Data, beep）
- [ ] 声明外部变量（TaskHandle, QueueHandle, u8g2_t）
- [ ] 初始化蜂鸣器 `buzzer_init()`
- [ ] 创建消息队列 `xQueueCreate()`
- [ ] 配置 u8g2 显示屏
- [ ] 主循环中：清屏 → 绘图 → 发送 → 接收按键 → 处理
- [ ] 返回键处理：唤醒菜单任务，挂起自己
- [ ] 在 `freertos.c` 中创建任务 `xTaskCreate()`
- [ ] 在 `ShowMenu.c` 中添加入口
- [ ] 测试：按键响应、显示正常、返回功能
