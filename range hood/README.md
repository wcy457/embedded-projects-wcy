# 智能油烟机控制系统

基于 STM32F103ZET6 + FreeRTOS 的油烟机控制系统，含自定义 Bootloader 支持 OTA 固件升级。

## 硬件平台

- **MCU**：STM32F103ZET6
- **RTOS**：FreeRTOS
- **显示**：OLED
- **传感器**：DHT11（温湿度）

## 功能

- FreeRTOS 多任务调度
- 风机调速
- 按键扫描
- OLED 状态显示
- DHT11 温湿度采集
- OTA 固件升级（自定义 Bootloader + CRC32 校验）

## 目录结构

```
range hood/
├── RTOS YJ/        # FreeRTOS 应用工程（Keil MDK-ARM）
└── bootloader YJ/  # Bootloader 工程
```

## 编译

Keil MDK-ARM 打开对应 `.uvprojx` 工程，编译后下载到开发板。

> Bootloader 与应用工程需分别编译，先烧录 Bootloader 再烧录应用。
