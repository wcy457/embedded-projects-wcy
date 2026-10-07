# FreeRTOS 智能手表

基于 STM32F103C8T6 + FreeRTOS 的多任务智能手表，使用 u8g2 驱动 OLED 显示。

## 硬件平台

- **MCU**：STM32F103C8T6
- **RTOS**：FreeRTOS
- **显示**：u8g2（OLED）
- **工程生成**：STM32CubeMX（`01_freertos_template.ioc`）

## 功能

- FreeRTOS 多任务调度
- OLED 图形显示（u8g2 库）
- 输入处理任务

## 目录结构

```
FreeRTOS智能手表/
├── Core/         # 核心代码（main.c 等）
├── Drivers/      # HAL 驱动
├── Middlewares/  # FreeRTOS 中间件
├── MDK-ARM/      # Keil 工程
├── mytasks/      # 自定义任务（Data、InputTask 等）
├── u8g2/         # u8g2 显示库
├── .mxproject
└── 01_freertos_template.ioc  # CubeMX 工程
```

## 编译

1. Keil MDK-ARM 打开 `MDK-ARM/` 下工程编译，或
2. STM32CubeMX 打开 `.ioc` 重新生成代码后编译
