# 智能油烟机控制系统

基于 STM32F103ZET6 + FreeRTOS 的油烟机控制系统，采用多任务架构，支持手动/自动/油烟回流四种工作模式，集成 PID 转速闭环、传感器融合自动调速，并含自定义 Bootloader 支持 OTA 升级。

## 硬件平台

- **MCU**：STM32F103ZET6
- **RTOS**：FreeRTOS
- **传感器**：DHT11（温湿度）、MQ135（空气质量/油烟）、编码器（转速采集）
- **显示**：OLED
- **执行**：PWM 电机（风机）、LED
- **通信**：USART + DMA
- **存储**：内部 Flash（Bootloader + APP 分区）

## 功能特性

### 四种工作模式
| 模式 | 说明 |
|---|---|
| `MODE_OFF` | 关机，PWM 占空比清零 |
| `MODE_MANUAL` | 手动模式，按键切换高低档 |
| `MODE_AUTO` | 自动模式，传感器融合 + 状态机自动调速 |
| `MODE_BACKFLOW` | 油烟回流模式 |

### 多任务架构（7 个任务）

| 任务 | 职责 |
|---|---|
| `h_key` | 4 按键扫描（模式切换、高档、低档、电源） |
| `h_speed` | 编码器转速采集 + PID 闭环 |
| `h_motor` | 电机 PWM 输出 |
| `h_fusion` | 传感器融合 + 自动模式状态机 |
| `h_backflow` | 油烟回流检测 |
| `h_sensor` | DHT11 / MQ135 数据采集 |
| `h_ui` | OLED 显示 + 串口日志 |

### PID 转速闭环
- 编码器采集实际转速
- PID 控制器调节 PWM 占空比，实现手动模式下的转速闭环
- 基础占空比 `s_pid_base = 400`，PID 输出叠加

### 自动模式状态机
- 进入自动模式时启动状态机 `auto_fsm_start()`
- 根据传感器数据（温度、油烟浓度）自动调节风量
- 退出自动模式时重置状态机 `auto_fsm_reset()`

### 并发安全
- `state_lock()` / `state_unlock()` 全局状态锁
- 保护 `g_state`（模式、档位、PWM 占空比）的多任务并发访问

### OTA 升级
- 自定义 Bootloader（`bootloader YJ/`）
- 支持应用区固件升级与 CRC32 校验
- Flash 分区：Bootloader 区 + APP 区

## 目录结构

```
range hood/
├── RTOS YJ/        # FreeRTOS 应用工程
│   ├── Core/       # main.c、外设初始化
│   ├── APP/        # tasks.c（7个任务）、状态机、PID、传感器融合
│   ├── Drivers/    # HAL 驱动
│   └── Middlewares/# FreeRTOS
└── bootloader YJ/  # Bootloader 工程
```

## 编译

Keil MDK-ARM 打开对应 `.uvprojx` 工程编译下载。

> 烧录顺序：先烧录 Bootloader，再烧录 APP（APP 运行在 Flash 高地址区）
