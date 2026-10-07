# 嵌入式项目合集

本科期间完成的三个嵌入式项目，涵盖 STM32 裸机/RTOS、GUI 图形界面与 Linux 驱动开发。

## 项目列表

| 目录 | 平台 | 简介 |
|---|---|---|
| `range hood` | STM32F103ZET6 + FreeRTOS | 智能油烟机控制系统（含 Bootloader） |
| `Car_Dashboard` | STM32F103 + LVGL + CAN | 汽车 OTA 仪表盘（多节点 CAN 通信） |
| `Linux_Camera` | i.MX6ULL (Cortex-A7) + V4L2 | Linux 相机采集与显示 |

---

## 1. range hood — 智能油烟机控制系统

基于 STM32F103ZET6，采用 FreeRTOS 实时操作系统，实现多任务调度。

- **MCU**：STM32F103ZET6
- **RTOS**：FreeRTOS
- **功能**：风机调速、按键扫描、OLED 显示、温湿度采集（DHT11）、OTA 升级
- **Bootloader**：自定义 Bootloader，支持应用区固件升级与校验（CRC32）
- **工程结构**：
  - `RTOS YJ/` — FreeRTOS 应用工程（Keil MDK-ARM）
  - `bootloader YJ/` — Bootloader 工程

### 编译
使用 Keil MDK-ARM 打开对应 `.uvprojx` 工程文件，直接编译下载。

---

## 2. Car_Dashboard — 汽车 OTA 仪表盘

基于 STM32F103 + LVGL 图形库的汽车仪表盘，支持通过 CAN 总线进行 OTA 固件升级。

- **MCU**：STM32F103
- **GUI**：LVGL（先在 PC 端 CodeBlocks 调通 UI，再移植到 STM32）
- **通信**：CAN 总线（ISO-TP 协议栈）
- **OTA**：Bootloader + CAN 分包传输 + CRC 校验 + Flash 分区管理

### 工程结构
| 节点 | 说明 |
|---|---|
| `01_Car_APP` | 仪表盘主应用（LVGL 界面 + CAN 通信） |
| `02_C8T6_CAN` | CAN 发送节点（C8T6） |
| `03_Bootloader` | OTA Bootloader |
| `04_usb_tool` | USB 转 CAN 工具 |
| `00_CodeBlocks_LVGL` | PC 端 LVGL UI 调试工程 |

### 编译
Keil MDK-ARM 打开各节点 `.uvprojx` 编译。

---

## 3. Linux_Camera — Linux 相机采集显示

基于 i.MX6ULL（Cortex-A7）的 Linux 相机应用，使用 V4L2 接口采集摄像头数据并显示到 LCD。

- **平台**：i.MX6ULL (ARM Cortex-A7)
- **系统**：嵌入式 Linux
- **接口**：V4L2（Video4Linux2）
- **显示**：Framebuffer
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
├── range hood/          # 项目一：油烟机
├── Car_Dashboard/       # 项目二：汽车仪表盘
├── Linux_Camera/        # 项目三：Linux 相机
└── .gitignore           # Keil/Linux 编译产物忽略规则
```

## 已排除内容

- Keil 编译产物（`.o .crf .d .axf .hex .map` 等）
- 第三方课程资料与演示视频
- 个人面试准备文档
