# 嵌入式 Linux 相机（V4L2 视频采集与显示）面试笔记

> 源码位置：`Linux_Camera/src/`（40 个文件，已从 GBK 统一转 UTF-8）
> 版本：`6th_ok/video2lcd`（课程最终可用版）
> 标注约定：**【原项目】** = 代码里本来就有的，可以直接讲；**【待改造】** = 建议你亲手做完再写进简历，没做就别这么说。

---

## 0. 项目名称与简历描述

### 0.1 项目名称

**主用名**

> 嵌入式 Linux 实时视频采集与显示系统（基于 V4L2）

**备选名**（按目标岗位调整）

| 岗位倾向 | 项目名 |
|---|---|
| Linux 应用开发 | 嵌入式 Linux 实时视频采集与显示系统（基于 V4L2） |
| 驱动/内核方向 | USB UVC 摄像头视频链路：V4L2 采集到 Framebuffer 显示 |
| 偏多媒体 | 基于 V4L2 + libjpeg 的 MJPEG/YUYV 视频解码渲染引擎 |
| 保守写法 | Linux 相机（video2lcd）：V4L2 采集、软解码与 LCD 显示 |

**技术关键词行**（放项目名下方，便于 ATS 抓取）

> C / Linux / V4L2 / Video4Linux / Framebuffer / libjpeg / YUV-RGB 色彩空间转换 / MJPEG 解码 / POSIX 多线程 / 生产者-消费者队列 / 零拷贝 mmap / 可插拔模块架构 / ARM 交叉编译 / IMX6ULL

### 0.2 简历四条介绍（可直接复制，每条两行内）

> **嵌入式 Linux 实时视频采集与显示系统（基于 V4L2）**
> C | Linux | V4L2 | Framebuffer | libjpeg | POSIX 多线程 | ARM 交叉编译

- 基于 V4L2 Streaming 实现摄像头采集（REQBUFS/QUERYBUF + mmap 零拷贝 + poll 收帧），4 缓冲队列轮转，不支持 STREAMING 时降级 read 路径
- 设计采集/转换/渲染/显示四层可插拔架构（函数指针链表注册 + isSupport 格式协商），新增像素格式仅需 1 文件 1 行注册
- 实现 MJPEG/YUV422 双路软解码（libjpeg 内存源 + `setjmp/longjmp` 隔离坏帧、整数查找表替代浮点乘除），缩放列映射表预计算，除法次数 W×H→W+H
- 单线程流水线改造为三线程 + 有界帧队列（mutex/cond + 丢最旧帧），三缓冲消除撕裂；分段计时定位渲染链瓶颈，消除显存自拷贝与逐像素 `memcpy(2B)` 两处热点

**简历有余量时可加第 5 条**（工程能力）：三级 Makefile 递归构建（`obj-y` 自动收集）完成 ARM 交叉编译与 libjpeg/SDL 移植，无硬件时用内核 `vivid` 虚拟设备开发验证

### 0.2.1 写法要点

对齐你油烟机项目的既有风格：**动词开头 → 对象带规格 → 括号内用「+」堆技术名词 → 逗号收尾兜底策略或量化结果**。不加能力标签、不解释原理（原理留到面试口头讲）；每条留一个钩子引追问，而答案你恰好准备充分（如"mmap 零拷贝省了什么"→ 1.1.2，"为什么三缓冲"→ 2.2）。

### 0.3 口述版（自我介绍用，一句话）

> 我做了一个 Linux 上的实时相机程序：用 V4L2 从 USB 摄像头零拷贝采集视频帧，自己做 MJPEG 和 YUV 的软解码与色彩空间转换，再缩放合成输出到 LCD；后期把单线程流水线改成了三线程加帧队列，同时解决了直接写显存造成的画面撕裂。

### 0.4 使用注意

- 第 4 条已采用**无数字版（版本 B）**："分段计时定位渲染链瓶颈"承担原数字的表现力——方法是真的（10.4 的代码可直接用），两处热点是代码事实。若完成实测，可恢复"单帧 35ms→18ms、MJPEG 17→24fps"并注明平台与 10.4 测法。
- 三线程、三缓冲、热点移除 —— **都属于【待改造】项**（见第 5 节）。做完再写；没做完就删掉对应半句，改成描述【原项目】已有机制（见 1.2、2.2、3.1）。
- 关键词行按投递岗位增删，但 `V4L2` 和 `Framebuffer` 是这个项目的辨识度来源，建议保留。
- 简历上**不要出现"韦东山"、"100ask"、"跟着课程实现"**这类字样。

### 0.5 能力矩阵（简历每行对应什么能力、会被怎么验）

投简历前对照这张表自查：**右边两列都填不满的能力，别写进简历。**

| 简历行 | 体现的核心能力 | 面试官的验证方式 | 你要能当场答上 | 笔记位置 |
|---|---|---|---|---|
| 1 | Linux 系统调用与设备 I/O | "mmap 和 read 有什么区别？" "为什么只给 PROT_READ？" "4 个 buffer 怎么来的？" | 零拷贝原理、buffer 所有权轮转、`bytesused` vs 算出的长度 | 2.1 / 2.2 |
| 1 | 接口能力协商与降级 | "如果设备不支持 streaming 怎么办？" | `QUERYCAP` 判能力 → 替换函数指针走 `read()` 路径 | 2.3 |
| 2 | 面向接口编程 / 解耦 | "你这个可插拔具体怎么实现的？" "加一种格式要改哪些文件？" | 函数指针结构体 + 尾部追加注册 + 三种匹配方式（探测 / 自描述 / 按名）的差异 | 1.2 |
| 2 | 阅读与复用他人架构 | "为什么显示必须先于采集初始化？" | `S_FMT` 的宽高来自 `GetDispResolution`，存在硬依赖（并能指出这是设计缺陷） | 1.3 |
| 3 | 数据格式与算法基础 | "YUYV 一帧多大？" "为什么可以砍色度？" "YUV420 和 422 区别？" | 色度抽样、BT.601 公式、`U-128` 的含义、RGB565 为什么给 G 留 6 位 | 3.1 |
| 3 | 第三方库集成与定制 | "libjpeg 报错为什么会杀掉进程？" | 默认 `error_exit` 调 `exit()` → 换回调 + `setjmp/longjmp` 做非局部跳转 | 3.2 |
| 3 | 性能分析与优化 | "你怎么定位瓶颈的？" "查表为什么快？" | 无 FPU 上的浮点代价、逐像素 memcpy 的函数调用开销、除法提表 | 3.1 / 4.1 |
| 4 | 多线程与同步 | "mutex 加在哪？" "为什么不一个线程做完？" "队列满了丢哪一端？" | 生产者消费者、buffer 生命周期、丢最旧 vs 丢最新的场景取舍 | 5.1 |
| 4 | 图形显示原理 | "撕裂是怎么产生的？" "三缓冲比双缓冲好在哪？" | LCD 逐行扫描时序 vs CPU 写入的竞争、`line_length` 对齐、vsync | 4.3 / 4.4 |
| 第5行 | 构建体系与移植 | "交叉编译工具链前缀改哪？" "库依赖怎么解决？" | 三级 Makefile 的 `obj-y` 收集机制、sysroot 与 rootfs 里的 `.so` | 6 / 说明.txt |

**跨项目能力（你相对其他候选人的差异化，务必主动带出来）**

| 能力 | 来自油烟机/仪表盘项目 | 在本项目的对应物 |
|---|---|---|
| 并发原语迁移 | `xQueue` / `xSemaphore` / 优先级继承 | `pthread_mutex` / `pthread_cond` / `SCHED_FIFO` |
| 中断与业务分层 | ISR 只做 DMA 状态，业务全在任务层 | 内核线程填 buffer，用户态 `poll` 阻塞取帧 |
| 时序测量 | 50ms 窗口编码器采样、测速噪声分析 | 帧率统计用 `clock_gettime(CLOCK_MONOTONIC)`、`sequence` 判丢帧 |
| 数据完整性设计 | CRC16-CCITT(Ymodem) / CRC32(固件) / magic+checksum | `bytesused` 校验、损坏帧的 `longjmp` 跳出 |
| 资源约束意识 | heap_4 静态池、栈大小按调用深度算、bootloader 8.4KB | 无 FPU 下的查表替代浮点、离屏 buffer 的内存预算 |

这张表的作用是：当面试官问"你做的是 RTOS，Linux 上有什么区别"时，你能按维度对齐回答，而不是重新组织语言。**建议把右列打印在简历项目栏下方的小字里。**

---

## 1. 整体架构

### 1.1 数据流

```
/dev/videoX
   │  V4L2: poll + DQBUF（mmap 零拷贝，4 个 buffer 轮转）
   ▼
T_VideoBuf  ── 原始帧（YUYV / MJPEG / RGB565，指向内核 DMA buffer）
   │  Convert：查表 YUV→RGB  /  libjpeg 解码
   ▼
T_VideoBuf  ── RGB565 或 RGB32（malloc 的应用内存）
   │  PicZoom：近邻取样，按 LCD 分辨率等比缩放
   ▼
   │  PicMerge：居中拷贝到 framebuffer 画布
   ▼
FlushPixelDatasToDev → ShowPage → memcpy 到 mmap 的显存 → LCD
```

### 1.2 三层可插拔注册机制【原项目】

这是本项目最值钱的架构点，面试必讲。三个模块用的是**同一套设计模式**：静态全局单链表 + 函数指针结构体 + 尾部追加注册。

| 层 | 结构体 | 注册函数 | 查找方式 | 实现者 |
|---|---|---|---|---|
| 采集 | `T_VideoOpr` | `RegisterVideoOpr` | 遍历尝试 `InitDevice` | v4l2.c |
| 转换 | `T_VideoConvert` | `RegisterVideoConvert` | `isSupport(in,out)` 逐个询问 | yuv2rgb / mjpeg2rgb / rgb2rgb |
| 显示 | `T_DispOpr` | `RegisterDispOpr` | 按 `name` 字符串匹配 | fb.c |

**机制展开**：

- `T_VideoOpr` 把 `InitDevice / GetFrame / PutFrame / StartDevice / StopDevice / GetFormat / ExitDevice` 抽象成 7 个函数指针。`VideoDeviceInit()` 遍历链表，谁先 `InitDevice` 返回 0 就用谁 —— 这是**探测式**匹配，不是按名字。
- `T_VideoConvert` 的 `isSupport(iPixelFormatIn, iPixelFormatOut)` 是**能力自描述**：调用方不需要知道有哪些转换器，只要报出"我要从 A 变成 B"，链表里自己应答。`GetVideoConvertForFormats()` 返回第一个应答者。
- `T_DispOpr` 是**命名后端**：`SelectAndInitDefaultDispDev("fb")` 按字符串选中，然后调它的 `DeviceInit()` + `CleanScreen(0)`。

**权衡/演进**：

- 好处：加一种解码格式 = 加一个 `.c` 文件 + 一行注册，`main.c` 一行不改。这就是为什么 `VideoConvertInit()` 里只是三次 `XxxInit()` 调用。
- 代价：链表遍历是 O(n)，且 `GetVideoConvertForFormats` 每帧都会被 `main.c` 调用一次吗？—— 不会，原代码在循环外只查一次并缓存指针，这是对的。但如果格式数变多，`isSupport` 逐个询问不如二维表 `O(1)` 查表。
- 可深挖点：三个 manager 的代码是**复制粘贴**的（`RegisterXxx` 三份逻辑完全相同）。可以讲"我把它抽象成了一个通用的模块注册器"，这是真实的重构机会。

### 1.3 main.c 初始化顺序（有严格依赖，面试会问）

```c
DisplayInit();                              // 注册所有显示后端
SelectAndInitDefaultDispDev("fb");          // 选中并 mmap 显存
GetDispResolution(&w,&h,&bpp);              // 拿到 LCD 参数
GetVideoBufForDisplay(&tFrameBuf);          // 把显存本身包成 T_VideoBuf 当画布
iPixelFormatOfDisp = tFrameBuf.iPixelFormat;// ← 决定后面要转成什么格式

VideoInit();                                // 注册 v4l2 采集后端
VideoDeviceInit(argv[1], &tVideoDevice);    // open + 协商格式 + 申请 buffer
iPixelFormatOfVideo = GetFormat();          // ← 摄像头实际给的格式

VideoConvertInit();                         // 注册三个转换器
ptVideoConvert = GetVideoConvertForFormats(video格式, disp格式);  // 匹配解码器
StartDevice();                              // VIDIOC_STREAMON
while(1) { GetFrame → Convert → Zoom → Merge → Flush → PutFrame }
```

**为什么必须先初始化显示**：因为 `V4l2InitDevice()` 内部会调 `GetDispResolution()`，把 **LCD 的分辨率当作向驱动申请的视频分辨率**（`tV4l2Fmt.fmt.pix.width = iLcdWidth`）。顺序颠倒会拿到未初始化值。

**这是个可讲的设计缺陷**【待改造】：视频分辨率被 LCD 分辨率绑架了。摄像头可能支持 1280x720，但 LCD 是 800x480，于是强制只采集 800x480，缩放质量下降。正确做法是独立配置采集分辨率，采集后再缩放。改造点：把 `S_FMT` 的参数来源从 `GetDispResolution` 改成命令行/配置文件。

---

## 2. V4L2 采集层（`video/v4l2.c`，377 行）

### 2.1 完整 ioctl 序列

```c
open(dev, O_RDWR)
VIDIOC_QUERYCAP   → 检查 V4L2_CAP_VIDEO_CAPTURE，并记录 STREAMING / READWRITE 能力
VIDIOC_ENUM_FMT   → while 循环遍历，命中 g_aiSupportedFormats 就 break
VIDIOC_S_FMT      → 设置 pixelformat / width / height / field=V4L2_FIELD_ANY
VIDIOC_REQBUFS    → count=NB_BUFFER(4), type=CAPTURE, memory=MMAP
VIDIOC_QUERYBUF   → 每个 buffer 拿到 length 和 m.offset
mmap              → PROT_READ, MAP_SHARED, fd, buf.m.offset
VIDIOC_QBUF × 4   → 全部入队
VIDIOC_STREAMON
loop: poll(fd, POLLIN) → VIDIOC_DQBUF → 处理 → VIDIOC_QBUF
VIDIOC_STREAMOFF
```

#### 2.1.1 白板流程图（含所有权流转）

```
应用进程                          V4L2 驱动 / 内核                    摄像头硬件
─────────                        ────────────────                    ──────────
  │
  │  fd = open("/dev/video0", O_RDWR)
  │───────────────────────►│ 创建视频设备实例
  │  ◄─────────────────────│ fd
  │
  │  ① VIDIOC_QUERYCAP  ──────────► 查能力位
  │  ◄──── capabilities: STREAMING + VIDEO_CAPTURE ────
  │        (不支持 STREAMING? → read() 降级路径，见 2.3)
  │
  │  ② VIDIOC_ENUM_FMT 循环 ──────► 枚举硬件支持的所有格式
  │  ◄──── YUYV / MJPEG / RGB565 ... ────
  │        与 g_aiSupportedFormats[] 匹配，挑一个双方都支持的
  │
  │  ③ VIDIOC_S_FMT  ─────────────► 申请: 800×480, YUYV
  │  ◄──── 驱动回填实际值 (可能改尺寸/格式!) ────  ← 必须读回
  │
  │  ④ VIDIOC_REQBUFS ─────────────► count=4, type=MMAP
  │  ◄──── 驱动确认 4 个内核 DMA buffer ────
  │
  │  ⑤ for(i=0;i<4;i++){
  │      VIDIOC_QUERYBUF(i) ──────► 拿第 i 个 buffer 的长度/偏移
  │      mmap(length, offset) ────► 建立用户态映射
  │  ◄──── 得到 4 个用户态指针 buf[i] ────   [零拷贝: 只建页表,不拷数据]
  │
  │      VIDIOC_QBUF(i) ──────────► 4 个 buffer 全部入"输入队列"
  │                                ┌──────────────────────┐
  │                                │ 驱动队列(空,待采集)   │◄──DMA──────├ 采集
  │                                │ [0][1][2][3]         │   填数据
  │                                └──────────────────────┘
  │  ⑥ VIDIOC_STREAMON ───────────► 启动 DMA 传输 ◄════════════════════│
  │
  │╔════════════════════ 采集主循环 ════════════════════╗
  │║ ⑦ poll(fds, -1) ── 阻塞等 POLLIN(有帧可取)          ║
  │║                                                      ║   DMA 写满一个
  │║ ⑧ VIDIOC_DQBUF ◄──── 从"输出队列"取一个已填好的      ║   buffer 后,
  │║      返回: buf[i], bytesused, sequence, timestamp    ║   硬件自动填
  │║                                                      ║   下一个
  │║   此刻 buf[i] 所有权 = 应用 (驱动不再碰它)           ║
  │║                                                      ║
  │║ ⑨ 直接读 buf[i] 处理:                                ║
  │║      Convert(MJPEG/YUYV→RGB565)                     ║
  │║      PicZoom(缩放到 LCD 尺寸)                        ║
  │║      PicMerge(合成进显存) → Flush                    ║
  │║                                                      ║
  │║ ⑩ VIDIOC_QBUF(i) ──── 归还! buf[i] 所有权交回驱动     ║
  │║                       驱动才敢往里 DMA 写下一帧       ║
  │╚════════════════════════════════════════════════════╝
  │
  │  ⑪ VIDIOC_STREAMOFF ──────────► 停 DMA
  │  for(i) munmap(buf[i])     解除映射
  │  close(fd)
```

#### 2.1.2 双队列模型（理解 V4L2 的核心）

```
          QBUF 入队                    DMA 采集                  DQBUF 出队
  应用 ───────────► 【输入队列:空buffer】 ──硬件填数据──► 【输出队列:满buffer】 ───────► 应用
   ▲                    ▲                                              │
   │                    └──────────── 4 个 buffer 循环 ────────────────┘
   └──────────────────────── QBUF 归还（应用处理完）
```

**buffer 在两个队列之间循环，应用只在 DQBUF→QBUF 之间"借用"。**

#### 2.1.3 ioctl 速记表

| 阶段 | ioctl | 作用 | 易错点 |
|---|---|---|---|
| ① | `QUERYCAP` | 查设备能力 | 区分 STREAMING / READWRITE |
| ② | `ENUM_FMT` | 枚举格式 | 要循环枚举，不能假设 |
| ③ | `S_FMT` | 设格式尺寸 | **驱动可能改，必须读回** |
| ④ | `REQBUFS` | 申请内核 buffer | 4 个，MMAP 类型 |
| ⑤a | `QUERYBUF` | 拿 buffer 偏移/长度 | 每个都要查 |
| ⑤b | `mmap` | 映射到用户态 | 零拷贝的关键 |
| ⑤c | `QBUF` | 空 buffer 入队 | 4 个全要入队才能启动 |
| ⑥ | `STREAMON` | 开始采集 | |
| ⑦ | `poll` | 等帧就绪 | 带超时防设备拔出卡死（本项目写死 -1） |
| ⑧ | `DQBUF` | 取已采集帧 | 拿回 index / bytesused / sequence |
| ⑩ | `QBUF` | **归还 buffer** | 不还→buffer 耗尽→停采 |
| ⑪ | `STREAMOFF` | 停止 | 退出时清理 |

#### 2.1.4 30 秒背诵版

> open 后先 QUERYCAP 查能力，ENUM_FMT 枚举格式做双向匹配，S_FMT 设参数并读回驱动实际值；REQBUFS 申请 4 个内核缓冲，逐个 QUERYBUF 加 mmap 映射到用户态，全部 QBUF 入队，STREAMON 启动。主循环就是 poll 等帧、DQBUF 取出处理、处理完 QBUF 归还——buffer 在输入输出两个队列间循环，应用只在取出和归还之间借用，全程数据零拷贝。退出 STREAMOFF 加 munmap。

### 2.2 高频追问链

**Q：为什么用 mmap 而不是 read？**

A：`read()` 需要内核把 DMA 缓冲区的数据**拷贝一份到用户态 buffer**，一帧 800x480x2(YUYV) ≈ 768KB，30fps 就是 23MB/s 的纯内存拷贝。mmap 把内核 DMA buffer 直接映射进用户地址空间，应用读的是同一块物理页，**零拷贝**。

**Q：mmap 的权限为什么只给 `PROT_READ`？**

A：采集方向是内核写、用户读，给写权限反而危险。注意 `V4l2ExitDevice` 里对所有 buffer 调 `munmap` —— 但 READWRITE 路径下 buffer 是 `malloc` 来的，对 malloc 内存调 munmap 是**未定义行为**。这是原代码的一个真实 bug，可作为"我读代码读出的问题"来讲。【待改造：区分两种 memory 类型分别释放】

**Q：为什么是 4 个 buffer？（`NB_BUFFER = 4`）**

A：V4L2 驱动内部有两个队列：**输入队列**（driver 往里填帧）和**输出队列**（应用处理完归还）。buffer 太少 → 应用处理第 N 帧时驱动没有空闲 buffer 可写，丢帧；太多 → 延迟增大（帧在队列里排队）+ 内存浪费。4 是"处理一帧 + 驱动正在填一帧 + 缓冲 2 帧抖动"的经验值。

**Q：poll 的超时为什么是 -1？**

A：`poll(tFds, 1, -1)` 永久阻塞直到有帧。因为整个程序就是单线程专职处理视频，不需要超时。但这里只监听 **1 个 fd**，`nfds=1` 时 poll 和 select 性能无差别（都走 `poll_wait` 挂队列），选 poll 是因为不需要 `FD_SET` 那套位图操作、代码更干净。

**Q：什么时候会被要求用 select/epoll？**

A：追问方向通常是"如果同时要处理触摸输入 + 网络 + 视频呢"。答：多 fd 时用 `poll`（fd 数在几十量级，poll 的 O(n) 遍历可接受，且能同时监听多个事件类型）；fd 数上千且活跃少才用 `epoll`。这里如果加输入事件，`tFds[2]` 扩成两个 fd 即可，不用换机制。

**Q：DQBUF 之后为什么必须 QBUF 回去？**

A：buffer 的所有权在驱动和应用之间轮转。DQBUF 把 buffer 从输出队列摘走（应用独占），QBUF 归还给驱动重新可写。**不归还 = 泄漏**，归还后指针仍有效（映射没解）。注意原代码在 `处理` 中间才归还（第 161 行 `PutFrame` 在 Flush 之后），意味着**驱动在这整段时间里拿不到这个 buffer**，这是延迟和丢帧的直接原因。【待改造：拷贝出必要数据后立刻 QBUF】

**Q：`iTotalBytes = tV4l2Buf.bytesused` 为什么用驱动返回的值而不是算出来的？**

A：MJPEG 是变长压缩数据，一帧多大取决于画面内容，只能由驱动告知。而 `iLineBytes = width * bpp / 8` 对 MJPEG 算出来是 0（因为 `iBpp` 被显式设成 0）—— 这是原代码用 `(MJPEG) ? 0` 三元式硬编码的原因，也说明 MJPEG 帧在解码前**没有"行"的概念**。

### 2.3 双路径派发（很好的讲点）

```c
if (tV4l2Cap.capabilities & V4L2_CAP_STREAMING) {
    // 保持默认的 GetFrameForStreaming / PutFrameForStreaming（mmap 版）
} else if (tV4l2Cap.capabilities & V4L2_CAP_READWRITE) {
    g_tV4l2VideoOpr.GetFrame = V4l2GetFrameForReadWrite;   // 运行时替换函数指针
    g_tV4l2VideoOpr.PutFrame = V4l2PutFrameForReadWrite;
    ptVideoDevice->iVideoBufCnt = 1;
    ptVideoDevice->iVideoBufMaxLen = width * height * 4;   // 一个像素最多 4 字节
    ptVideoDevice->pucVideBuf[0] = malloc(...);
}
```

**机制**：有些老驱动/虚拟设备不支持 streaming 只支持 read/write。代码在探测能力后**改写函数指针**，把实现换成 `read()` 版本。`iVideoBufMaxLen = W*H*4` 是按最坏情况（RGB32）预留。

**权衡**：read 路径只有 1 个 buffer、必须拷贝，性能差但兼容性好。这是"能力协商 + 运行时多态"的 C 语言实现范式。

### 2.4 原代码里的两个真实瑕疵（读代码能力的证明）

1. `V4l2InitDevice` 第 78-80 行：先 `ioctl(QUERYCAP)`，**紧接着 `memset(&tV4l2Cap, 0, ...)` 把结果清掉**，然后又 ioctl 一次。第一次调用完全白做。
2. `isSupportThisFormat` 用 `sizeof(arr)/sizeof(arr[0])` 是对的（int 数组），但如果有人改成指针就错了 —— 可以顺带讲数组退化。

---

## 3. 转换层（`convert/`）

### 3.1 YUYV → RGB（`yuv2rgb.c` + `color.c`）

**YUYV 是什么**：YUV422 packed，4 字节表示 **2 个像素**：`Y0 U Y1 V`。两个像素共享一组色度 U/V —— 这叫**水平方向 2:1 色度抽样**。

**为什么可以这样**：人眼对亮度敏感、对色度不敏感（色觉细胞少）。所以砍掉一半色度分辨率，视觉几乎无差异，数据量从 RGB888 的 24bpp 降到 16bpp。

**转换公式**（BT.601，`color.h` 里的宏）：

```
R = Y + 1.402  × (V-128)
G = Y - 0.344  × (U-128) - 0.714 × (V-128)
B = Y + 1.772  × (U-128)
```

`U-128` / `V-128` 是因为 U/V 是**无符号存储的有符号量**，128 是零点。

**机制展开**：
- `initLut()` 在 `Yuv2RgbInit()` 里调用，把浮点乘法预生成整数查找表（`R_FROMYV(Y,V)` 等宏查表）。原因：一帧 800x480 = 38.4 万像素，每像素 3 次乘法 + 溢出裁剪，浮点运算在 ARM Cortex-A7 上没有 FPU 时极慢。
- 循环步长：`size = width*height/2`，每次读 4 字节产出 **2 个像素**，对应 `Y` 和 `Y1` 分别算一次。
- RGB565 打包：`r>>3 << 11 | g>>2 << 5 | b>>3`。R/B 各砍 3 位、G 砍 2 位 —— 因为人眼对绿色最敏感，所以给 G 留 6 位。
- 输出 buffer 用 `if (!aucPixelDatas) malloc(...)` **惰性分配一次**，后续帧复用。这是对的（避免每帧 malloc），但注意它挂在 `ptVideoBufOut` 上，`ConvertExit` 才 free。

**权衡/演进**：
- 近邻查表 vs 精确计算：查表快但有精度损失（量化误差）。
- 单线程逐像素：可以讲 SIMD（NEON）优化思路，或按行分块多线程。
- 更彻底的方案：**让 GPU/VPU 做转换**，或直接用支持 YUV 输入的显示通道（IMX6ULL 的 IPU、DRM 的 NV12 plane），省掉整个 convert 层。

### 3.2 MJPEG → RGB（`mjpeg2rgb.c`）

**MJPEG 本质**：每一帧都是一个**独立的完整 JPEG 文件**（无帧间依赖），所以叫 Motion JPEG。

**libjpeg 标准流程**：
```c
jpeg_create_decompress → jpeg_mem_src_tj(内存源) → jpeg_read_header
→ scale_num=scale_denom=1 → jpeg_start_decompress
→ while (output_scanline < output_height) jpeg_read_scanlines(&linebuf, 1)
→ CovertOneLine(24bpp → 16/32bpp) → jpeg_finish_decompress → jpeg_destroy_decompress
```

**三个必问点**：

1. **为什么要自定义错误处理 + `setjmp/longjmp`？**
   libjpeg 默认的 `error_exit` 会直接 `exit()` 终止进程。解码摄像头来的数据时，一帧损坏不能让整个程序死掉。所以替换 `error_exit` 为 `MyErrorExit`，里面 `longjmp(setjmp_buffer, 1)` 跳回 `setjmp` 点，在跳回分支里做资源清理（`jpeg_destroy_decompress` + 两次 `free`）后返回 -1。
   **这是 C 语言里"库提供的回调式错误处理 + 非局部跳转"的经典案例**，面试讲清楚能加分。

2. **为什么用 `jpeg_mem_src_tj` 而不是 `jpeg_stdio_src`？**
   数据在内存（mmap 的视频 buffer）里，不是文件。`jdatasrc-tj.c` 是项目自带的**内存数据源实现**（对应 libjpeg 的 `jpeg_memory_src`，老版本 libjpeg 没有这个函数，所以要自己实现 `init_source/term_source/skip_data/` 等回调）。

3. **为什么按 scanline 逐行解，还要一个 `aucLineBuffer`？**
   libjpeg 的行缓冲接口一次给 N 行。逐行处理 + 立刻转成目标 bpp 写入输出 buffer，好处是**峰值内存只多一行**，而不是先申请一整帧 RGB24 再转。坏处是 `CovertOneLine` 每行都要做一次格式转换循环。

**权衡**：MJPEG 解码是 CPU 密集（哈夫曼解码 + IDCT + 上采样），比 YUYV 转换慢一个数量级。但 MJPEG 的 USB 带宽占用远低于 YUYV —— 800x480@30fps YUYV 需要 ~23MB/s，MJPEG 只要 2~4MB/s。**这是"用 CPU 换 USB 带宽"的典型权衡**，很多 USB 摄像头在高速 USB 上根本跑不满 YUYV 30fps，只能用 MJPEG。

### 3.3 rgb2rgb.c

处理 RGB565 ↔ RGB32 互转（摄像头给 RGB565 但 LCD 是 RGB32 之类）。注意它**不处理 BGR/RGB 字节序差异** —— 如果 LCD 是 BGRA 会出现红蓝互换，这是移植时必踩的坑，值得作为"我调试过的问题"来讲。

---

## 4. 渲染与显示层

### 4.1 缩放（`render/operation/zoom.c`）

**算法：最近邻取样（Nearest Neighbor）**

```c
pdwSrcXTable[x] = x * srcWidth / dstWidth;   // 预计算列映射表
dwSrcY = y * srcHeight / dstHeight;          // 行映射现算
memcpy(dst + x*pixelBytes, src + table[x]*pixelBytes, pixelBytes);
```

**机制**：目标坐标反算源坐标（backward mapping），保证目标每个像素都有值，不会留空洞。

**性能优化点**：把内层的除法 `x*srcW/dstW` 提成**查表**，一次除法从 O(W×H) 降到 O(W)。这是原代码里已有的优化，可以讲。

**原代码的浪费**【待改造】：
- `pdwSrcXTable` 每帧 `malloc` + `free` 一次。缩放比例不变时完全可以缓存复用。
- 逐像素 `memcpy(dst, src, 2)` —— 拷 2 字节调 memcpy，函数调用开销远大于拷贝本身。改成 `*(uint16_t*)` 直接赋值。
- 最近邻有锯齿和马赛克。双线性插值（4 邻域加权）质量明显更好，代价是 4 次乘法。
- 缩放目标尺寸计算在 `main.c`：以 LCD 宽为基准保持宽高比 `k = h/w`，若高度超出则改为以高为基准 —— 等比缩放 + 居中，逻辑正确但没处理"视频比 LCD 小"的情况（此时不缩放，直接居中，OK）。

### 4.2 合成（`merge.c`）

`PicMerge(iTopLeftX, iTopLeftY, ptSrc, ptDst)`：把源图贴到目标画布的指定位置。`main.c` 里算 `(lcdW - picW)/2` 实现居中。

### 4.3 framebuffer（`display/fb.c`）

```c
open("/dev/fb0", O_RDWR)
ioctl(FBIOGET_VSCREENINFO)  → xres, yres, bits_per_pixel, red/green/blue offset
ioctl(FBIOGET_FIXSCREENINFO)→ line_length（硬件实际每行字节数，可能 > xres*bpp/8）
mmap(NULL, screen_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0)
```

**必问点**：

**Q：`var` 和 `fix` 的区别？**
A：`fb_var_screeninfo` 是**可变**参数（软件可设：分辨率、bpp、各颜色分量偏移）；`fb_fix_screeninfo` 是**固定**参数（硬件决定：显存物理地址、`line_length`、同步时序）。关键在 `line_length` —— 它可能因为硬件对齐要求大于 `xres*bpp/8`，用 `iLineWidth` 而不是 `iWidth*bpp/8` 算行偏移，否则图像会斜切。本项目 `GetVideoBufForDisplay` 里 `iLineBytes = g_ptDefaultDispOpr->iLineWidth` 正是取的这个值。

**Q：为什么直接 mmap 显存而不是 write(fd)？**
A：`write()` 每次都要拷贝整帧并经过内核，mmap 后直接写物理显存。

### 4.4 撕裂问题（最重要的性能讲点）

`GetVideoBufForDisplay(&tFrameBuf)` 返回的 `aucPixelDatas` **就是 mmap 的显存本身**。然后 `PicMerge` 直接往显存上画，画完 `FlushPixelDatasToDev` 再调一次 `ShowPage`（等于又 memcpy 一遍到显存 —— 见 `disp_manager.c:172`）。

**问题**：LCD 控制器按固定时序逐行扫描显存。你在它扫到中间时覆盖了上半部分，就会出现**上半帧是新画面、下半帧是旧画面**的撕裂。而且逐行 merge 到显存，用户可能看到"画面从上往下刷下来"。

**正确做法（三缓冲）**：
```
AllocVideoMem(3) → 在离屏 VideoMem 里画完 → 一次性 memcpy 到显存 → 等 vsync
```

**关键发现**：`disp_manager.h` 里**已经定义好了完整的 VideoMem 三缓冲机制** —— `E_VideoMemState {VMS_FREE, VMS_USED_FOR_PREPARE, VMS_USED_FOR_CUR}`、`E_PicState {PS_BLANK, PS_GENERATING, PS_GENERATED}`、`AllocVideoMem / GetVideoMem / PutVideoMem / GetDevVideoMem`。这套是从"数码相框"项目继承来的（那里用于多页面切换）。

但 **`video2lcd` 的 `main.c` 完全没用它**，走的是 `GetVideoBufForDisplay` 直接拿显存的简化路径。

**这就是你最好的"演进"素材**：不是从零发明，而是"项目里已有更完善的机制但我最初用了简化路径，导致撕裂，我把 VideoMem 接回来实现了离屏渲染 + 单次拷贝"。面试讲这个比讲"我加了个线程"有深度得多。

---

## 5. 【待改造】四个改造方向（挑 2 个真做）

### 5.1 多线程流水线（最推荐）

现状：`poll → DQBUF → 解码 → 缩放 → merge → flush → QBUF` 全串行。解码 30ms + 缩放 10ms + 显示 15ms = 55ms/帧 → **只有 18fps**，而摄像头能给 30fps。

改造：
```
线程A 采集：poll + DQBUF + 拷贝帧描述符到入队 + 立刻 QBUF
线程B 解码：取出 → convert → 入队
线程C 显示：取出 → zoom + merge → flush
```
**必须能答的问题**：
- 队列满/空怎么处理？→ `pthread_mutex` + `pthread_cond`，或 POSIX 信号量
- buffer 生命周期？→ 采集线程不能直接 QBUF 归还内核 buffer（还没解码完），必须先 memcpy 到应用 buffer 再归还；或者用引用计数
- 丢帧策略？→ 队列满时**丢最旧**（保实时性，监控场景）还是**丢最新**（保完整性，回放场景），要能说出取舍
- 为什么不一个线程做全部？→ 因为 poll 阻塞期间 CPU 空闲，串行时延相加

### 5.2 离屏渲染消除撕裂（见 4.4）

### 5.3 新增 SDL/X11 显示后端（无板子也能跑通的关键）

```c
/* display/sdl.c */
static T_DispOpr g_tSDLOpr = {
    .name = "sdl", .DeviceInit = SDLDeviceInit,
    .ShowPixel = SDLShowPixel, .CleanScreen = SDLCleanScreen, .ShowPage = SDLShowPage,
};
int SDLInit(void) { return RegisterDispOpr(&g_tSDLOpr); }
```
并把 `main.c` 第 58 行的 `"fb"` 改成命令行参数或环境变量。

**面试价值**：证明你理解"可插拔架构不是嘴上说说，是真的加一个后端零改动主流程"。而且解决了你没板子的现实问题。

### 5.4 采集分辨率与显示分辨率解耦（见 1.3）

---

## 6. 无硬件环境怎么跑通

```bash
# 1. 虚拟视频设备（内核自带，不需要摄像头）
sudo apt install linux-modules-extra-$(uname -r)
sudo modprobe vivid nbuf=4
ls /dev/video*          # 出现 video0(采集) / video1(输出)

# 2. 本机编译（去掉交叉编译前缀）
#    Makefile 里把 arm-linux- 改成空，CFLAGS 加 -I/usr/include/libdrm 之类
make

# 3. 跑
./video2lcd /dev/video0
```

**注意**：`vivid` 一定支持 `V4L2_PIX_FMT_YUYV` 和 `RGB565`，能命中 `g_aiSupportedFormats`；MJPEG 需要 4.16+ 内核。

**`/dev/fb0` 在桌面 Ubuntu 上通常不可用**（被 KMS 接管）→ 这正是 5.3 的动机。

面试诚实说法："我在 PC Linux 上用内核 vivid 虚拟设备完成开发和功能验证，后来在 IMX6ULL 上做了真机验证。" —— 如果你真买了板子。没买就别提板子型号。

---

## 7. 高频追问速答

| 问题 | 要点 |
|---|---|
| V4L2 支持哪些接口？ | `VIDEO_CAPTURE/OUTPUT/OUTPUT_VIDEO/OVERLAY` + `read/write` vs `streaming` 两种 I/O |
| `V4L2_FIELD_ANY` 什么意思？ | 不指定奇偶场，让驱动自选；驱动会通过回填 `tV4l2Fmt` 告知实际值 |
| S_FMT 后为什么要读回 width/height？ | 驱动可能无法精确满足请求，会调整并回填。原代码 `iWidth = tV4l2Fmt.fmt.pix.width` 正是这个 |
| 怎么调节亮度/白平衡？ | `VIDIOC_QUERYCTRL` 枚举 → `VIDIOC_G_CTRL`/`VIDIOC_S_CTRL`。`05_video_brightness/video_test.c` 有完整例子 |
| YUV 和 RGB 的区别？ | YUV 分离亮度/色度，兼容黑白电视 + 便于色度抽样压缩；RGB 是显示端格式 |
| YUV420 和 422 区别？ | 422 水平 2:1 抽样（YUYV）；420 水平+垂直都 2:1（NV12/I420），每 4 个像素共享 1 组 UV |
| 为什么 MJPEG 比 YUYV 省带宽？ | JPEG 有损压缩，帧内独立压缩；USB 带宽是 USB 摄像头的瓶颈 |
| 一帧 800x480 YUYV 多大？ | 800×480×2 = 768,000 B ≈ 750KB；30fps = 22.5MB/s |
| 怎么知道丢帧了？ | 检查 `tV4l2Buf.sequence`（帧序号）连续性；不连续就是丢了 |
| 摄像头拔掉怎么办？ | 驱动报 `DQBUF` 失败或 `poll` 返回 `POLLERR/POLLHUP`；应用要能 `VIDIOC_STREAMOFF` + close + 重开 |
| 为什么不用 GStreamer？ | 项目是手写理解原理；生产环境用 GStreamer/V4L2+libva。要能说出"我知道有现成框架，这个项目是为了理解底层" |

---

## 8. 和你现有项目的衔接（面试串场用）

被问"你做过 RTOS，Linux 多线程有什么区别"时：

| 维度 | 你的 FreeRTOS 项目 | 本项目 |
|---|---|---|
| 并发原语 | `xQueueCreate` / `xSemaphoreCreateBinary` | `pthread_mutex_t` / `pthread_cond_t` / POSIX 信号量 |
| 优先级反转 | 互斥量优先级继承（你的项目踩过） | Linux `pthread_mutex` 默认 `PTHREAD_PRIO_INHERIT` 需显式设置；实时调度用 `SCHED_FIFO` |
| 内存 | `heap_4` 静态池，栈大小手工算 | glibc malloc，虚拟内存，栈自动扩展 |
| 中断 | ISR 只做底层，业务在任务层（你的约定） | 内核线程 + 用户态 `poll` 阻塞等待，没有"用户态中断"概念 |
| 测速/时序 | 50ms 窗口采样编码器 | 帧率统计用 `clock_gettime(CLOCK_MONOTONIC)` |
| 你已有的 OTA | Ymodem + CRC16，IAP 16KB bootloader | 可对比 Linux 的 A/B 分区升级、`fw_update` |

**这张表是你最大的差异化优势**：绝大多数候选人只会说"我做过 Linux 项目"，你能说清两个生态的机制对应关系。

---

## 9. 建议的动手顺序

1. 装 Ubuntu（实体/VM/WSL2 注意 WSL2 默认无 vivid 和 fb）
2. `modprobe vivid` → 改 Makefile 为本机 gcc → 编译 6th_ok
3. 先写 SDL 后端让它显示出来（**这一步做完你就能演示了**）
4. 接 VideoMem 做离屏渲染（消除撕裂）
5. 改造成三线程 + 帧队列（测帧率数字，写进简历）
6. 加 `VIDIOC_G/S_CTRL` 亮度调节（对齐 `05_video_brightness` 示例）
7. （可选）买板子做真机验证

---

## 10. 性能预算与实测方法（填 X/Y 之前必读）

### 10.1 ⚠️ 单核陷阱：多线程不提升吞吐

**IMX6ULL 是单核 Cortex-A7 @ 800MHz**（STM32MP157 同样是单核 A7）。多线程**无法凭空创造 CPU 周期**，所以"三线程改造让帧率翻倍"这种说法在单核板上站不住，面试官一句"你板子几个核"就把你问穿了。

而且原代码的串行流水线**已经通过 4 个内核 buffer 实现了 I/O 与计算的重叠**：处理第 N 帧时，驱动正在往 N+1、N+2 的 buffer 里填。所以 poll 等待并不是"浪费"。

多线程在这个项目里真正的收益（照这个讲，别说提吞吐）：

| 收益 | 说明 |
|---|---|
| 职责解耦 | 显示不再被解码拖住，按键/输入响应不卡顿 |
| 丢帧策略可控 | 串行版是"被动丢"（驱动覆盖），改造后是"主动丢最旧"，可讲取舍 |
| 端到端延迟抖动 | 各阶段独立调度，延迟方差下降 |
| 多核板上才有吞吐收益 | 树莓派 / i.MX6Quad / RK 等多核平台可真正并行 |

**要真提帧率，只能降单帧 CPU 耗时** —— 这才是数字的来源。

### 10.2 单帧耗时推算（含完整推导，可当场重推一遍）

**基准假设**（写死在这里，实测后要回来校准）

| 符号 | 取值 | 来源 |
|---|---|---|
| 分辨率 W×H | 800×480 | `S_FMT` 用 LCD 分辨率，IMX6ULL 常见 4.3"/5" 屏 |
| 像素数 N | 384,000 | W×H |
| RGB565 帧字节数 | 768,000 B ≈ 750KB | N×2 |
| CPU | Cortex-A7 @800MHz，**单核**，无硬浮点加速假设 | IMX6ULL 规格 |
| 1 cycle | 1.25 ns | 1/800MHz |
| memcpy 有效带宽 | 400 MB/s | A7 + DDR，读写各计一次流量 |

**逐阶段推导（YUYV 路径，baseline）**

| 阶段 | 推导式 | 结果 |
|---|---|---|
| `poll`+`DQBUF` | 内核链表摘挂 + 唤醒，常数级 | **0.5ms** |
| YUYV→RGB565 | 每像素 3 次 LUT 访存 + 移位打包 + 裁剪 ≈ 20 cycle → 384,000×20×1.25ns | **9.6ms** |
| `PicZoom` | 每像素一次 `memcpy(dst,src,2)`：调用+长度分派 ≈ 25 cycle，加索引/表查 10 cycle → 384,000×35×1.25ns | **16.8ms** |
| `PicMerge`→显存 | 768KB 读 + 768KB 写 ÷ 400MB/s | **3.8ms** |
| `FlushPixelDatasToDev` | 同上，且源=目的=同一块 `pucDispMem`（**纯浪费**） | **3.8ms** |
| **合计** | | **34.5 ≈ 35ms** |

→ 单帧 35ms > 摄像头帧间隔 33.3ms（30fps）→ **必然周期性丢帧**，实测帧率会落在 28fps 左右。

**优化后（三处代码级改动，不依赖多线程）**

| 改动 | 推导 | 优化后耗时 |
|---|---|---|
| 移除冗余 Flush | −3.8ms | — |
| 缩放改 `*(uint16_t*)d = *(uint16_t*)s` | 25 cycle → ~5 cycle，加索引 3 cycle → 384,000×8×1.25ns | **3.8ms**（原 16.8） |
| 缩放表缓存复用 | 省每帧 1 次 malloc+free（几十 KB 级，含 TLB 抖动） | 计入上行 |
| YUYV / Merge 不变 | — | 9.6 + 4.8 |
| **合计** | | **18.2 ≈ 18ms** |

→ 18ms < 33.3ms，**算力余量 45%**，帧率被摄像头 30fps 上限卡住 → 稳定 30fps、`sequence` 连续无跳号。

**MJPEG 路径**（解码是瓶颈，优化前后差一个数量级的体验）

| | baseline | 优化后 |
|---|---|---|
| libjpeg 软解 800×480 | ~32ms（哈夫曼+IDCT+2x1 上采样+逐行 24→16bpp） | ~32ms（未动） |
| Zoom / Merge / Flush | 16.8 + 3.8 + 3.8 | 3.8 + 4.8 + 0 |
| **单帧合计** | **56.4 ≈ 58ms** | **40.6 ≈ 41ms** |
| **帧率** | **1000/58 ≈ 17fps** | **1000/41 ≈ 24fps** |

MJPEG 路径解码仍占单帧 78%，所以**这条线上"提帧率"的正解是换硬件 VPU 或降采集分辨率，不是加线程** —— 面试能说出这个判断，比报一个数字更值钱。

> ⚠️ 以上全部是**推算值**。推导链条自洽、假设透明，被问"你怎么估的"你能现场重推；但简历里最终应替换成 10.4 节的实测数。

**关于 `W×H → W+H`（第 3 行简历句，这个是代码事实不是估算）**

```c
pdwSrcXTable = malloc(sizeof(unsigned int) * ptDst->iWidth);
for (x = 0; x < ptDst->iWidth; x++)                    // W 次除法
    pdwSrcXTable[x] = x * ptSrc->iWidth / ptDst->iWidth;
for (y = 0; y < ptDst->iHeight; y++) {
    dwSrcY = y * ptSrc->iHeight / ptDst->iHeight;      // H 次除法
    for (x = ...) /* 内层已无除法，只查表 */
}
```
不建表则内层每像素一次除法 = W×H 次。建表后 = W + H 次。**这个论断可以直接断言，不需要测量。**

### 10.3 三个不需要多线程就能拿到的免费收益（优先做这些）

**① 冗余的整帧自拷贝（最明显的 bug 级浪费）**

```c
GetVideoBufForDisplay(&tFrameBuf);          // aucPixelDatas = pucDispMem（mmap 显存）
PicMerge(x, y, &tZoomedVideo, &tFrameBuf);  // 已经 memcpy 进显存了
FlushPixelDatasToDev(&tFrameBuf);           // 内部又 GetVideoBufForDisplay
                                            //   → ShowPage → memcpy(显存, 显存, 768KB)
```
`disp_manager.c:172` 的 `ShowPixelDatas` 对 `pucDispMem` 做 memcpy，而源和目的是同一块显存。**删掉这次 Flush，或改成 merge 到离屏 buffer 再一次性提交**，白省 2~3ms/帧（约 8~10%）。

**② `PicZoom` 每帧 malloc/free 列映射表**
`zoom.c` 里 `pdwSrcXTable = malloc(...)` … `free(...)` 每帧一次。缩放比例不变 → 缓存复用。省 2 次 malloc + 一次 TLB 抖动。

**③ 逐像素 `memcpy(dst, src, 2)`**
拷 2 字节却调 memcpy，函数调用 + 长度判断开销是拷贝本身的十倍。改成 `*(unsigned short*)dst = *(unsigned short*)src`。这一项可能省 4~7ms/帧，**是 YUYV/MJPEG 路径最大的单点收益**。

> 讲法建议：「我用分段计时定位到瓶颈不在解码而在缩放，因为逐像素调 memcpy 拷 2 字节」—— 这比"我加了三个线程"有说服力得多。

### 10.4 实测方法（30 分钟拿到可写进简历的数字）

**分段计时**（改 `main.c` 主循环）：

```c
#include <time.h>
static inline long long now_us(void) {
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000LL + ts.tv_nsec / 1000;
}
/* 主循环内 */
long long t0 = now_us(); GetFrame(...);  long long t1 = now_us();
                      Convert(...);      long long t2 = now_us();
                      PicZoom(...);      long long t3 = now_us();
                      PicMerge(...);     long long t4 = now_us();
                      Flush(...);        long long t5 = now_us();
                      PutFrame(...);     long long t6 = now_us();
/* 每 100 帧打印一次各段平均耗时 + 端到端平均 */
```

**帧率与丢帧**（用 V4L2 的 `sequence` 字段，这是最可信的证据）：

```c
/* DQBUF 后 */
static __u32 last_seq = 0;  static unsigned long dropped = 0, total = 0;
if (last_seq && tV4l2Buf.sequence != last_seq + 1)
    dropped += tV4l2Buf.sequence - last_seq - 1;
last_seq = tV4l2Buf.sequence;  total++;
/* 实际帧率 = 打印间隔内 total 的增量 / 秒数；丢帧率 = dropped/(total+dropped) */
```

**CPU 占用**：`top -d 1 -p $(pidof video2lcd)` 看单进程 %CPU（注意 top 默认按 1 核归一化，`H` 展开线程）。

**对照实验**：`git` 存两个 tag（`baseline` / `opt-flush` / `opt-zoom` / `opt-thread`），每个都跑同一命令取 60s 平均，做一张 4 行表。**这张表本身就是面试材料**。

```bash
./video2lcd /dev/video0 > /tmp/perf_$(git describe --tags).log 2>&1 &
sleep 60; kill $!
```

### 10.5 无数字版（已采用）

简历第 4 条现在用的就是这一版——**用"分段计时定位瓶颈"的方法论表述替代数字**：

> ……三缓冲消除撕裂；分段计时定位渲染链瓶颈，消除显存自拷贝与逐像素 `memcpy(2B)` 两处热点

每个成分都站得住：分段计时是真实可复现的方法（10.4 有现成代码）；显存自拷贝（768KB = 800×480×2，`disp_manager.c`）和逐像素 memcpy（`zoom.c`）都是代码事实，被追问当场翻源码。

**10.2 的推算表仍然保留**——它的用途变了：不再是简历数字的来源，而是面试口头展开"为什么这两处是热点、大概省多少"时的推导素材，以及实测后校准假设的基准。

若日后完成实测想恢复数字，必须注明平台（IMX6ULL 单核 A7 800MHz、800×480）和测法（clock_gettime 分段计时 + sequence 丢帧统计）。

**红线不变**：不要写"性能提升 30%"这类没有测量支撑的百分比。宁可写机制，不要写假数字。

---

## 附：文件索引

| 文件 | 行数/大小 | 作用 |
|---|---|---|
| `main.c` | 5.0KB | 主流程编排 + 单线程流水线 |
| `video/v4l2.c` | 377 行 | V4L2 采集，streaming + read/write 双路径 |
| `video/video_manager.c` | 126 行 | `T_VideoOpr` 链表注册器 |
| `convert/yuv2rgb.c` | 176 行 | YUYV→RGB565/32，查表法 |
| `convert/mjpeg2rgb.c` | 246 行 | libjpeg 内存解码 + setjmp 错误处理 |
| `convert/rgb2rgb.c` | — | RGB565↔RGB32 |
| `convert/convert_manager.c` | 129 行 | `T_VideoConvert` 注册器 + `isSupport` 匹配 |
| `convert/color.c` / `color.h` | — | YUV↔RGB 宏与 LUT，来自 GspcaGui (GPL) |
| `convert/jdatasrc-tj.c` | — | libjpeg 内存数据源实现 |
| `render/operation/zoom.c` | 72 行 | 最近邻缩放 + 列映射表优化 |
| `render/operation/merge.c` | — | 区域贴图合成 |
| `display/fb.c` | 259 行 | framebuffer 后端（var/fix + mmap） |
| `display/disp_manager.c` | 563 行 | 显示后端注册 + VideoMem 三缓冲机制 |
| `include/config.h` | — | `DBG_PRINTF`、`FB_DEVICE_NAME` |
| `include/video_manager.h` | — | `NB_BUFFER 4`、`T_VideoDevice` |
| `说明.txt` | — | 三级 Makefile 体系说明（`obj-y` 风格） |
