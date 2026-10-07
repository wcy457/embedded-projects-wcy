# 智能冰箱

基于 STM32F103 的智能冰箱控制系统，采集温湿度并通过 OLED 显示，支持蜂鸣器报警与 PWM 调速。

## 硬件平台

- **MCU**：STM32F103
- **传感器**：DHT11（温湿度）
- **显示**：OLED
- **输入**：旋转编码器
- **输出**：蜂鸣器、PWM（压缩机/风机调速）
- **通信**：串口

## 功能

- 温湿度实时采集与 OLED 显示
- 旋转编码器调节参数
- 温湿度超限蜂鸣器报警
- PWM 调速控制

## 目录结构

```
智能冰箱/
├── Hardware/   # 外设驱动（BEEP、DHT11、Encoder、OLED、PWM、Serial）
├── User/       # 应用代码
├── library/    # 标准外设库
├── start/      # 启动文件
├── system/     # 系统文件
├── RTE/        # 运行时环境
└── project.uvprojx  # Keil 工程
```

## 编译

Keil MDK-ARM 打开 `project.uvprojx`，编译后下载到开发板。
