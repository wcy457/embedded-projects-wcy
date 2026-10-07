# 嵌入式项目合集

本科期间完成的六个嵌入式项目，涵盖 STM32 裸机开发、FreeRTOS 实时系统、LVGL 图形界面与 Linux 驱动开发。

## 项目列表

| # | 目录 | 平台 | 简介 |
|---|---|---|---|
| 1 | `智能冰箱` | STM32F103 | 基于温湿度采集的智能冰箱控制系统 |
| 2 | `智能锁项目` | STM32F103 | 指纹 + RFID + 蓝牙的智能门锁（分课时递进） |
| 3 | `FreeRTOS智能手表` | STM32F103C8T6 + FreeRTOS | 多任务智能手表（OLED 显示） |
| 4 | `range hood` | STM32F103ZET6 + FreeRTOS | 智能油烟机控制系统（含 Bootloader） |
| 5 | `Car_Dashboard` | STM32F103 + LVGL + CAN | 支持 CAN OTA 的汽车仪表盘 |
| 6 | `Linux_Camera` | i.MX6ULL + V4L2 | Linux 相机采集与显示 |

---

## 1. 智能冰箱

基于 STM32F103 的智能冰箱控制系统，采集温湿度并通过 OLED 显示，支持蜂鸣器报警与 PWM 调速。

- **MCU**：STM32F103
- **外设**：DHT11 温湿度、OLED 显示、旋转编码器、蜂鸣器、PWM、串口
- **功能**：温湿度实时采集显示、按键/编码器调节、超限报警
- **工程结构**：`Hardware/`（驱动）、`User/`（应用）、`library/`（标准外设库）

### 编译
Keil MDK-ARM 打开 `project.uvprojx` 编译下载。

---

## 2. 智能锁项目

基于 STM32F103 的智能门锁，按功能模块分课时递进实现，最终集成语音播报。

- **MCU**：STM32F103（标准外设库）
- **功能模块**（对应目录 `1、点灯` ~ `12、语音播报`）：
  - OLED 显示（SPI）
  - 串口通信 / 蓝牙配置
  - AT24C04 EEPROM 存储
  - RTC 实时时钟
  - BS8116A 触摸按键
  - MG200 指纹模块
  - RFID 门禁卡
  - 门锁电机驱动
  - 语音播报
- **工程结构**：每个课时为独立 Keil 工程，含 `HARDWARE/`、`SYSTEM/`、`USER/`、`STM32F10x_StdPeriph_Driver/`

### 编译
进入对应课时目录，Keil MDK-ARM 打开工程编译。

---

## 3. FreeRTOS智能手表

基于 STM32F103C8T6 + FreeRTOS 的多任务智能手表，使用 u8g2 驱动 OLED 显示。

- **MCU**：STM32F103C8T6
- **RTOS**：FreeRTOS
- **显示**：u8g2（OLED）
- **任务**：`mytasks/` 下含数据处理、输入处理等任务
- **工程**：STM32CubeMX 生成（`01_freertos_template.ioc`）

### 编译
Keil MDK-ARM 打开 `MDK-ARM/` 下工程编译，或用 STM32CubeMX 重新生成。

---

## 4. range hood — 智能油烟机控制系统

基于 STM32F103ZET6 + FreeRTOS 的油烟机控制，含自定义 Bootloader 支持 OTA。

- **MCU**：STM32F103ZET6
- **RTOS**：FreeRTOS
- **功能**：风机调速、按键扫描、OLED 显示、DHT11 温湿度、OTA 升级
- **Bootloader**：自定义 Bootloader，固件 CRC32 校验
- **工程结构**：
  - `RTOS YJ/` — FreeRTOS 应用
  - `bootloader YJ/` — Bootloader

### 编译
Keil MDK-ARM 打开 `.uvprojx` 编译下载。

---

## 5. Car_Dashboard — 汽车 OTA 仪表盘

基于 STM32F103 + LVGL 的汽车仪表盘，通过 CAN 总线实现 OTA 固件升级。

- **MCU**：STM32F103
- **GUI**：LVGL（PC 端 CodeBlocks 调试 → STM32 移植）
- **通信**：CAN 总线（ISO-TP）
- **OTA**：Bootloader + CAN 分包传输 + CRC 校验 + Flash 分区

### 工程结构
| 节点 | 说明 |
|---|---|
| `01_Car_APP` | 仪表盘主应用（LVGL + CAN） |
| `02_C8T6_CAN` | CAN 发送节点 |
| `03_Bootloader` | OTA Bootloader |
| `04_usb_tool` | USB 转 CAN 工具 |
| `00_CodeBlocks_LVGL` | PC 端 LVGL UI 调试 |

### 编译
Keil MDK-ARM 打开各节点 `.uvprojx` 编译。

---

## 6. Linux_Camera — Linux 相机采集显示

基于 i.MX6ULL（Cortex-A7）的 Linux 相机应用，V4L2 采集 + Framebuffer 显示。

- **平台**：i.MX6ULL (ARM Cortex-A7)
- **系统**：嵌入式 Linux
- **接口**：V4L2、Framebuffer
- **技术点**：
  - V4L2 采集流程（open → set format → request buffers → mmap → stream on → dequeue → stream off）
  - 多线程流水线 + 有界帧队列（mutex/cond + 丢最旧帧）
  - 三缓冲消除画面撕裂

### 编译
```bash
cd src/6th_ok/video2lcd
make
```

---

## 目录说明

```
.
├── 智能冰箱/              # 项目一
├── 智能锁项目/            # 项目二
├── FreeRTOS智能手表/      # 项目三
├── range hood/            # 项目四：油烟机
├── Car_Dashboard/         # 项目五：仪表盘
├── Linux_Camera/          # 项目六：相机
└── .gitignore             # 编译产物 & 非公开文档忽略规则
```

## 已排除内容

- Keil / GCC 编译产物（`.o .crf .d .axf .hex .map`、`Objects/` `OBJ/` `Listings/` 等）
- IDE 用户配置（`.uvguix.*` `.uvoptx`）
- 面试准备、复刻指南、课程讲解、学习笔记等个人文档
- 第三方课程资料与演示视频
