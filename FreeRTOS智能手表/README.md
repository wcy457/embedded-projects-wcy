# FreeRTOS 智能手表

基于 STM32F103C8T6 + FreeRTOS 的多任务智能手表，使用 u8g2 驱动 OLED 显示，支持红外遥控与旋转编码器输入，包含时间、日历、秒表、倒计时、手电筒、温湿度等多个功能界面。

## 硬件平台

- **MCU**：STM32F103C8T6
- **RTOS**：FreeRTOS（CMSIS-RTOS v2）
- **显示**：u8g2（OLED）
- **输入**：红外遥控 + 旋转编码器（替代物理按键）
- **工程生成**：STM32CubeMX（`01_freertos_template.ioc`）
- **时钟**：HSE 8MHz × PLL 9 = 72MHz

## 功能特性

### 多任务架构（9 个任务）

| 任务 | 功能 |
|---|---|
| `ShowTimeTask` | 主时间显示界面 |
| `ShowMenuTask` | 功能菜单 |
| `ShowCalanderTask` | 日历 |
| `ShowClockTimeTask` | 秒表 / 倒计时 |
| `ShowFlashLightTask` | 手电筒 |
| `ShowSettingTask` | 系统设置 |
| `ShowWoodenFishTask` | 木鱼（电子木鱼） |
| `ShowDHT11Task` | 温湿度显示 |
| `InputTask` | 输入处理（红外 + 编码器） |

### FreeRTOS 机制运用
- **软件定时器**：秒表（1000ms 周期）、倒计时（100ms 周期）
- **互斥量**：保护定时器变量，防止回调与显示任务竞争（启用优先级继承）
- **任务挂起/恢复**：通过 `vTaskSuspend` / `vTaskResume` 切换活跃 UI 任务
- **消息队列**：`g_xQueueMenu` 全局队列，InputTask 分发输入事件到当前活跃任务

### 输入方式
- **红外遥控**：红外接收器接收按键码
- **旋转编码器**：旋转切换菜单项，按下确认

### 支持的外设驱动
LED、LCD、MPU6050、DS18B20、DHT11、有源/无源蜂鸣器、彩色 LED、红外收/发、光感传感器、红外避障、超声波 SR04、SPI Flash W25Q64、旋转编码器、电机、UART

## 系统架构

```
InputTask（红外 + 编码器）
    │  消息队列 g_xQueueMenu
    ▼
当前活跃 UI 任务（时间/菜单/日历/时钟/手电筒/设置/木鱼/温湿度）
    │
    ▼
u8g2 OLED 显示
```

## 目录结构

```
FreeRTOS智能手表/
├── Core/         # 核心代码（main.c、freertos.c、外设初始化）
├── Drivers/      # HAL 驱动库
├── Middlewares/  # FreeRTOS 中间件
├── MDK-ARM/      # Keil 工程
├── mytasks/      # 自定义任务（Data、InputTask 等）
├── u8g2/         # u8g2 显示库
├── .mxproject
└── 01_freertos_template.ioc  # STM32CubeMX 工程文件
```

## 编译

1. Keil MDK-ARM 打开 `MDK-ARM/` 下工程编译，或
2. STM32CubeMX 打开 `01_freertos_template.ioc` 重新生成代码后编译
