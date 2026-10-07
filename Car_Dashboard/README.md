# 汽车 OTA 仪表盘

基于 STM32F103 + LVGL 的汽车仪表盘，通过 CAN 总线接收车辆数据并驱动 LVGL 图形界面显示，支持通过 CAN 总线进行 OTA 固件升级。

## 硬件平台

- **MCU**：STM32F103
- **GUI**：LVGL（PC 端 CodeBlocks 调试 → STM32 移植）
- **显示**：LCD（SPI）
- **通信**：CAN 总线（ISO-TP 协议栈）
- **时钟**：内部 RTC
- **存储**：内部 Flash（Bootloader 区 0x08000000 + APP 区 0x08008000）

## 功能特性

### LVGL 仪表盘界面
- 转速表指针（根据转速值旋转角度）
- 安全带指示灯
- 水温指示灯
- 灯光指示灯
- 转向灯指示灯
- 日期时间显示（RTC）

### CAN 总线数据接收
主循环中 `Can_Handle()` 解析 CAN 帧，更新以下数据：
| 数据 | 来源 |
|---|---|
| 转速 RPM | CAN 帧 |
| 水温 | CAN 帧 |
| 安全带状态 | CAN 帧 |
| 灯光状态 | CAN 帧 |
| 转向灯状态 | CAN 帧 |
| 日期时间 | CAN 帧（写入 RTC） |

### 生产者-消费者模式
```
CAN 中断/轮询收帧（生产者）→ 数据缓冲区 → LVGL UI 刷新（消费者）
```
有新数据时才刷新对应 UI 控件，减少无效绘制。

### OTA 固件升级
1. CAN 收到 OTA 触发帧
2. APP 写 Flash 升级标志 `inter_flash_cfg_set_app_update_flag(1)`
3. 系统复位 `HAL_NVIC_SystemReset()`
4. Bootloader 检测到升级标志，通过 CAN 接收新固件写入 APP 区
5. CRC 校验通过后跳转到新 APP

### APP 分区与向量表重定位
```c
SCB->VTOR = 0x08008000;   // APP 从 0x08008000 运行，向量表偏移
```

### LVGL 心跳
- TIM6 中断提供 `lv_tick` 时基
- 主循环每 5ms 调用 `lv_timer_handler()`

## 工程结构

| 节点 | 说明 |
|---|---|
| `Project/01_Car_APP` | 仪表盘主应用（LVGL + CAN + OTA 触发） |
| `Project/02_C8T6_CAN` | CAN 发送节点（STM32F103C8T6，模拟车辆数据） |
| `Project/03_Bootloader` | OTA Bootloader（CAN 接收固件 + Flash 写入 + CRC 校验） |
| `Project/04_usb_tool` | USB 转 CAN 工具（PC 端固件发送） |
| `Project/00_CodeBlocks_LVGL` | PC 端 LVGL UI 调试工程 |

## OTA 升级流程

```
PC(usb_tool) ──USB──▶ 04_usb_tool ──CAN──▶ 02_C8T6_CAN ──CAN──▶ 01_Car_APP
                                                                      │
                                                              OTA 标志+复位
                                                                      ▼
                                                          03_Bootloader 接收固件
                                                                      │
                                                              写 Flash + CRC 校验
                                                                      ▼
                                                              跳转到新 APP (0x08008000)
```

## 编译

Keil MDK-ARM 打开各节点 `.uvprojx` 工程编译。
