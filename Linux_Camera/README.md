# Linux 相机采集显示

基于 i.MX6ULL（ARM Cortex-A7）的嵌入式 Linux 相机应用，使用 V4L2 采集摄像头数据，经格式转换后显示到 LCD Framebuffer。

## 硬件平台

- **MPU**：i.MX6ULL (ARM Cortex-A7)
- **系统**：嵌入式 Linux
- **采集接口**：V4L2（Video4Linux2），`/dev/video0`
- **显示接口**：Framebuffer，`/dev/fb0`

## 功能特性

### 模块化架构
采用管理器模式，支持扩展多种采集设备、显示设备和格式转换：

| 模块 | 说明 |
|---|---|
| `video_manager` | 视频设备管理器，注册/选择采集设备 |
| `v4l2` | V4L2 摄像头采集实现 |
| `disp_manager` | 显示设备管理器，注册/选择显示设备 |
| `fb` | Framebuffer 显示实现 |
| `convert_manager` | 格式转换管理器，自动匹配转换链 |
| `yuv2rgb` | YUV → RGB 转换 |
| `mjpeg2rgb` | MJPEG → RGB 解码（libjpeg） |
| `rgb2rgb` | RGB 缩放/裁剪 |
| `render` | 渲染（缩放 zoom、合并 merge） |

### V4L2 采集流程
```
open(/dev/videoX)
  → VIDIOC_S_FMT 设置采集格式
  → VIDIOC_REQBUFS 请求缓冲区
  → mmap 映射缓冲区
  → VIDIOC_STREAMON 启动采集
  → VIDIOC_DQBUF 取帧（处理）
  → VIDIOC_QBUF 归还缓冲区
  → VIDIOC_STREAMOFF 停止
```

### 显示流程
```
GetFrame（V4L2 取帧）
  → 格式转换（YUV/MJPEG → RGB）
  → 缩放/裁剪（适配 LCD 分辨率）
  → 写入 Framebuffer
```

### 自动格式匹配
- 获取摄像头输出像素格式和显示设备像素格式
- 自动查找对应的格式转换模块
- 支持 YUV、MJPEG、RGB 等多种格式

## 命令行用法

```bash
./video2lcd </dev/video0,1,...>
```

## 目录结构

```
Linux_Camera/
└── src/
    └── 6th_ok/video2lcd/       # 主工程
        ├── main.c               # 主程序
        ├── video/               # 视频采集（v4l2.c、video_manager.c）
        ├── display/             # 显示（fb.c、disp_manager.c）
        ├── convert/             # 格式转换（yuv2rgb、mjpeg2rgb、rgb2rgb）
        └── render/              # 渲染（zoom 缩放、merge 合并）
```

## 编译

```bash
cd src/6th_ok/video2lcd
make
```

在 i.MX6ULL 开发板上运行编译生成的可执行文件。
