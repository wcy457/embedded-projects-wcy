# Linux 相机采集显示

基于 i.MX6ULL（ARM Cortex-A7）的嵌入式 Linux 相机应用，使用 V4L2 采集摄像头数据并显示到 LCD Framebuffer。

## 硬件平台

- **MCU/MPU**：i.MX6ULL (ARM Cortex-A7)
- **系统**：嵌入式 Linux
- **接口**：V4L2（Video4Linux2）、Framebuffer

## 功能

- V4L2 摄像头采集
- Framebuffer 显示
- 多线程流水线 + 有界帧队列（mutex/cond + 丢最旧帧策略）
- 三缓冲消除画面撕裂

## V4L2 采集流程

```
open → set format → request buffers → mmap → stream on → dequeue → enqueue → stream off
```

## 目录结构

```
Linux_Camera/
├── src/    # 源码
│   └── 6th_ok/video2lcd/  # 相机采集显示主工程
└── doc/    # 文档
```

## 编译

```bash
cd src/6th_ok/video2lcd
make
```

在 i.MX6ULL 开发板上运行编译生成的可执行文件。
