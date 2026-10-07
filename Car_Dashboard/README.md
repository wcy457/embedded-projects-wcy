# 汽车 OTA 仪表盘

基于 STM32F103 + LVGL 的汽车仪表盘，通过 CAN 总线实现 OTA 固件升级。

## 硬件平台

- **MCU**：STM32F103
- **GUI**：LVGL（先在 PC 端 CodeBlocks 调通 UI，再移植到 STM32）
- **通信**：CAN 总线（ISO-TP 协议栈）

## 功能

- LVGL 图形界面仪表盘
- CAN 总线多节点通信
- OTA 固件升级（Bootloader + CAN 分包传输 + CRC 校验 + Flash 分区管理）

## 工程结构

| 节点 | 说明 |
|---|---|
| `Project/01_Car_APP` | 仪表盘主应用（LVGL 界面 + CAN 通信） |
| `Project/02_C8T6_CAN` | CAN 发送节点（STM32F103C8T6） |
| `Project/03_Bootloader` | OTA Bootloader |
| `Project/04_usb_tool` | USB 转 CAN 工具 |
| `Project/00_CodeBlocks_LVGL` | PC 端 LVGL UI 调试工程 |

## 编译

Keil MDK-ARM 打开各节点 `.uvprojx` 工程编译。

> OTA 流程：PC 端通过 USB-CAN 工具发送固件 → C8T6 节点转发到 CAN 总线 → 主节点 Bootloader 接收并写入 Flash。
