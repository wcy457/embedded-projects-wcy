# Bootloader 入门讲解（一步一步来）

> 目标读者：刚刚接触这个项目、看到 bootloader 代码和面试文档里满屏专业名词（VTOR / MSP / NVIC / ICER / NDTR / ORE / Thumb / ZI …）有点懵的同学。
> 阅读策略：**先建立直观图景 → 再逐个拆名词 → 再逐行讲代码 → 最后讲为什么这么设计**。每一节都自包含，可以跳读。
> 配套源码：
> - Bootloader 工程：`bootloader YJ/BSP_BOOT/bootloader.c`、`bootloader.h`、`crc32.c`
> - 链接脚本：`bootloader YJ/MDK-ARM/bootloader YJ/bootloader YJ.sct`
> - APP 工程（被升级的那一个）：`RTOS  YJ/Core/Src/system_stm32f1xx.c`、`RTOS  YJ/MDK-ARM/RTOS  YJ/RTOS  YJ.sct`
> - 上位机：`tools/ota_host.py`
> - 面试口径：`doc/项目面试深度讲解.md` 第 1 章

---

## 0. 先用大白话把整个事情说清楚

### 0.1 什么是 Bootloader，为什么要有它

把 STM32 芯片想象成一台 PC：
- PC 开机先跑 BIOS/UEFI，再由它把 Windows/Linux 加载进内存运行。
- STM32 上电后也是先跑一段"小程序"，这段小程序叫 **Bootloader**（引导加载器），它的任务只有一个：**决定接下来跑哪段程序，必要时还能把新程序写进芯片的 Flash 里**。

为什么需要它？因为产品出厂后不可能每次都拆开壳子用烧录器刷固件：
- 产线上批量烧录后想升级？走串口。
- 售后远程给用户升级固件？走串口/WiFi。
- 芯片焊在板子上、外壳都封好了？只能走串口。

所以 Bootloader 的价值就是：**让设备在使用现场（不拆机）就能更新固件**。专业术语叫 **IAP**（In-Application Programming，应用中编程），意思是"芯片自己跑着程序的时候，自己把自己的 Flash 擦掉重写"。

> 和 IAP 对应的是 **ISP**（In-System Programming），那是用烧录器通过 SWD/BOOT0 等硬件方式刷，需要拆机或接线。本项目是 IAP 方案。

### 0.2 这个项目的整体结构（一张图记住）

```
              [云端OTA / 产线烧录器 / PC上位机]
                          │ 串口(将来可换WiFi模组)
                          ▼
┌────────────────────────────────────────────────────┐
│  STM32F103ZET6 (512KB Flash / 64KB SRAM)          │
│                                                    │
│  0x08000000 ┌─────────────────────┐ 16KB          │
│             │  Bootloader          │ USART1+DMA    │
│             │  负责接收/校验/跳转   │               │
│             └──────────┬──────────┘               │
│  0x08004000 ┌──────────▼──────────┐ 496KB         │
│             │  APP(业务固件)       │               │
│             │  FreeRTOS+7任务+PID  │               │
│             └─────────────────────┘               │
└────────────────────────────────────────────────────┘
```

一句话总结：**Flash 前面 16KB 永远是 Bootloader（出厂烧一次就不再动它），后面 496KB 是 APP（可以被 Bootloader 反复擦写升级）**。

为什么是 16KB？因为 STM32F103 中容量芯片 Flash 每页 2KB，16KB 正好是 8 整页，便于擦写对齐；同时 16KB 对一个只做"收数据+校验+跳转"的小程序完全够用（本项目 bootloader 编出来只有约 8.4KB）。

### 0.3 上电后到底发生了什么（一句话版）

> **上电 → 跑 Bootloader → 检查按键 → 没按就跳到 APP；按了就留在升级模式等串口数据。**

升级时：上位机把新固件 bin 文件按自定义协议发过来 → Bootloader 收一块写一块 → 全部收完后算 CRC32 → 跟上位机发的 CRC 对一遍 → 对了就跳到新 APP，错了就等重发。

---

## 1. 必须先搞懂的前置知识（专业名词逐个拆）

面试文档里出现的每个名词，下面都有解释。**看不懂代码，十有八九是名词没搞懂。**这一节读完了，再回头看代码就通透了。

### 1.1 Flash 和 SRAM

- **Flash**：芯片里的"硬盘"，断电不丢。程序代码、常量都存在这里。STM32F103ZET6 有 512KB Flash，地址从 `0x08000000` 开始。
- **SRAM**：芯片里的"内存"，断电即丢。变量、栈、堆都在这里。F103ZET6 有 64KB SRAM，地址从 `0x20000000` 开始。
- 类比：Flash 是硬盘，SRAM 是内存条。程序上电后从 Flash 取指令执行，变量放 SRAM。

### 1.2 地址、字节、字、小端序

- **地址**：每个字节都有一个编号，比如 `0x08004000`。CPU 就是靠地址找数据。
- **字节(byte)**：8 个 bit，一个 `uint8_t`。
- **半字(halfword)**：2 字节，一个 `uint16_t`。
- **字(word)**：4 字节，一个 `uint32_t`。STM32F1 的 Flash **只能按字（4 字节）写**，不能只写 1 个字节——这是硬件限制，代码里到处都在处理这个约束。
- **小端序(Little Endian, LE)**：一个 4 字节数 `0x12345678` 在内存里存的顺序是 `78 56 34 12`（低字节在低地址）。STM32、PC 都是小端。所以协议里"4 字节长度"是按小端发的。

### 1.3 向量表、MSP、Reset_Handler、Thumb 位（最高频考点）

Cortex-M 芯片上电后，硬件自动做两件事，**不需要任何软件参与**：
1. 从地址 `0x00000000`（对 F103 从 Flash 启动时映射自 `0x08000000`）读一个 32 位数，作为 **MSP 初值**（主栈指针）。
2. 从地址 `0x00000004` 读一个 32 位数，作为 **复位向量**（Reset_Handler 的入口地址），然后跳过去执行。

这块连续的内存叫 **向量表(Vector Table)**，它的前几个字长这样：

| 偏移 | 内容 | 作用 |
|---|---|---|
| 0x00 | 初始 MSP | 上电后栈顶位置 |
| 0x04 | Reset_Handler | 复位后跑的第一个函数 |
| 0x08 | NMI_Handler | 不可屏蔽中断入口 |
| 0x0C | HardFault_Handler | 硬件错误入口 |
| 0x10+ | 其它中断 | 每个中断一个字 |

几个名词解释：
- **MSP(Main Stack Pointer)**：主栈指针。栈是函数调用、中断压栈用的临时内存区。`__set_MSP(xxx)` 这条代码就是把栈顶切到新地址——跳转到 APP 前必须切，因为 APP 的栈位置和 Bootloader 不一样。
- **Reset_Handler**：C 启动文件(`startup_stm32f103xe.s`)里的汇编入口，它干三件事：把 `.data` 段从 Flash 拷到 SRAM、把 `.bss` 段清零、最后跳进 C 库的 `__main`，最终进入 `main()`。
- **Thumb 位**：Cortex-M 只能跑 **Thumb 指令集**，所以函数地址的 **最低位(bit0)必须是 1**。你看 `entry=0x08004279`，末尾是 9（奇数），bit0=1。如果拿到一个偶数地址当复位向量跳过去，CPU 会报错。代码里 `boot_is_app_valid` 检查 `(entry & 1U) == 0U` 就是拒掉这种非法镜像。

### 1.4 VTOR：向量表重定位寄存器

`SCB->VTOR` 是一个硬件寄存器，**它告诉 CPU"中断向量表现在在哪个地址"**。

- Bootloader 在 `0x08000000`，上电后 VTOR 默认指向这里，中断来了就去这里找 handler。
- 跳转到 APP 后，APP 的向量表在 `0x08004000`，如果不改 VTOR，下次来中断 CPU 还是去 `0x08000000` 找——找到的是 Bootloader 的旧 handler，**加上 APP 的新 MSP = 灾难**。

所以跳转前 Bootloader 设一次 `SCB->VTOR = APP_BASE_ADDR`，APP 的 `SystemInit` 里也会再设一次（`VECT_TAB_OFFSET=0x4000`），双保险。代码在 [system_stm32f1xx.c](file:///d:/Projects/JL%20project/range%20hood/RTOS%20%20YJ/Core/Src/system_stm32f1xx.c#L185) 第 185 行。

### 1.5 NVIC、ICER、ICPR（跳转前清中断那一坨）

**NVIC(Nested Vectored Interrupt Controller)** 是 Cortex-M 的中断控制器。它有几个关键寄存器：

- **ISER/ICER**：使能/失能中断。ICER 写 1 把对应中断关掉。
- **ISPR/ICPR**：挂起/清除挂起。一个中断来了但 CPU 正在处理更高优先级，它会"挂起(pending)"等会儿处理；ICPR 写 1 清掉挂起标志。

跳转前 Bootloader 干了这段：
```c
for(idx = 0U; idx < 8U; idx++) {
    NVIC->ICER[idx] = 0xFFFFFFFFU;   /* 关掉所有中断 */
    NVIC->ICCP[idx] = 0xFFFFFFFFU;   /* 清掉所有挂起 */
}
```
- `8U` 是因为 F103 有 240 个外设中断，每个 ICER 是 32 位，需要 8 个 32 位寄存器才能覆盖完。
- **不清会怎样**：Bootloader 用过的 USART1/DMA 中断如果在跳转瞬间还挂着，APP 的启动代码可能还没装好自己的 handler，CPU 跑到 C 启动文件的弱符号 `Default_Handler` 死循环里 → 跑飞。

### 1.6 DMA、Normal 模式、NDTR

**DMA(Direct Memory Access)** 是一个独立于 CPU 的小引擎，专门干"把数据从外设搬到内存"这种累活，CPU 不用每来一个字节就中断一次。
- **UART + DMA**：串口每收到一个字节，DMA 自动把它搬到内存缓冲区，收满指定个数才中断一次 CPU。
- **Normal 模式**：DMA 收满 N 个字节就 **停**，需要软件再次启动才能再收。和它对应的 **Circular 模式** 是收满后自动从头循环收。本项目 Bootloader 用 Normal（收满一块处理一块再重启），APP 用 Circular（连续数据流）。
- **NDTR(Normal Data Transfer Register)**：DMA 的"剩余计数器"。你让它收 128 字节，每收一个 NDTR 减 1，收到 0 表示收满。代码里 `__HAL_DMA_GET_COUNTER` 就是读 NDTR。**如果 HAL 状态显示 READY 但 NDTR 不为 0，说明中途出错了（ORE/FE），字节没收够**——这是健壮性检查的关键。

### 1.7 ORE、FE（串口错误标志）

- **ORE(Overrun Error)**：上一个字节 CPU/DMA 还没读走，下一个字节就来了，覆盖了 → 数据丢失。
- **FE(Framing Error)**：停止位没对上，时序乱了。

这两个错误会让 HAL 内部提前终止 DMA 接收（状态变 READY），但字节没真收够——所以光看 READY 不够，还要看 NDTR。

### 1.8 SysTick 和 HAL_GetTick

- **SysTick**：Cortex-M 内核自带的一个 24 位倒计时定时器，CMSIS 标准配置。HAL 库把它配成 **1ms 中断一次**，每次中断把 `uwTick++`，所以 `HAL_GetTick()` 返回一个毫秒计数。
- 代码里 `(HAL_GetTick() - tickstart) > timeout_ms` 就是经典的"无符号减法超时判断"——`HAL_GetTick()` 是 `uint32_t`，约 49.7 天回绕一次，**无符号减法天然处理回绕**（数学上 `(a-b) mod 2^32` 永远是对的），这是面试高频题。
- 跳转到 APP 前必须 `SysTick->CTRL = 0` 停掉它，否则 Bootloader 挂起的 SysTick 中断会在 APP 刚配置一半的向量表上炸掉。

### 1.9 CRC32、小端、zlib 兼容

- **CRC32**：对一段数据算出一个 32 位"指纹"，数据改一个 bit 指纹就大变。用来校验整包固件有没有传错。
- **多项式 0xEDB88320**：这是标准 CRC-32 多项式 `0x04C11DB7` 的 **位反转形式**（反射算法从低位算起）。
- **初值 0xFFFFFFFF、结果再异或 0xFFFFFFFF**：这是为了和 **Python 标准库 `zlib.crc32`** 结果完全一致。这样上位机一行 `zlib.crc32(data)` 就能算出和 bootloader 一样的 CRC，两边能互相验证。见 [crc32.c](file:///d:/Projects/JL%20project/range%20hood/bootloader%20YJ/BSP_BOOT/crc32.c)。
- **流式计算**：`crc32_update` 一块一块喂，`crc32_finalize` 末尾异或一次得到最终结果。这样不需要把整包数据缓存到 RAM 再算，**收一块算一块**，省内存。

### 1.10 Scatter File（.sct 链接脚本）、IROM、LR_IROM1

Keil 用 `.sct` 文件告诉链接器："代码段放哪个地址、多大、变量放哪个地址"。本项目两份：

Bootloader 的 [bootloader YJ.sct](file:///d:/Projects/JL%20project/range%20hood/bootloader%20YJ/MDK-ARM/bootloader%20YJ/bootloader%20YJ.sct):
```
LR_IROM1 0x08000000 0x00004000  {  ; 起址 0x08000000, 大小 16KB
  ER_IROM1 0x08000000 0x00004000  { ... }  ; 代码段
  RW_IRAM1 0x20000000 0x00020000  { ... }  ; 变量段(SRAM 128KB)
}
```
APP 的 [RTOS  YJ.sct](file:///d:/Projects/JL%20project/range%20hood/RTOS%20%20YJ/MDK-ARM/RTOS%20%20YJ/RTOS%20%20YJ.sct):
```
LR_IROM1 0x08004000 0x0007C000  {  ; 起址 0x08004000, 大小 496KB
  ER_IROM1 0x08004000 0x0007C000  { ... }
  RW_IRAM1 0x20000000 0x00010000  { ... }  ; SRAM 64KB
}
```

几个名词：
- **LR_IROM1 / ER_IROM1**：Load Region / Exec Region 的名字，"加载区"和"执行区"。简单理解就是"代码在 Flash 里存哪儿"。
- **IROM**：指令 ROM，代码段。
- **.ANY (+RO / +RW / +ZI)**：链接器把所有 `.text`(RO,代码常量)、`.data`(RW,已初始化变量)、`.bss`(ZI,零初始化变量) 都放进这个区。
- **关键约束**：APP 的 IROM 起址必须是 `0x08004000`，否则编译出来会从 `0x08000000` 开始——直接盖到 Bootloader 头上，烧完 APP 后 Bootloader 没了。这是项目里踩过的大坑（见面试文档 §5）。

### 1.11 hex 文件 vs bin 文件

- **hex 文件**：Intel HEX **文本格式**，每行带地址信息，像这样：
  ```
  :020000040800 F2   ← 扩展线性地址(高16位=0x0800)
  :1040000030...      ← 数据记录(低16位=0x4000)
  ```
  真实地址 = 高 16 位 + 低 16 位 = `0x08004000`。**只看 ELA 记录就喊"基址错了"是常见误判**，要算组合地址。
- **bin 文件**：纯二进制，**不带任何地址信息**。上位机升级发的是 bin（`fromelf --bin` 产物），基址由分区协议约定。
- 编译出来：Keil 默认产 hex，`fromelf --bin -o app.bin xxx.axf` 把 axf 转 bin。

### 1.12 RO/RW/ZI（map 文件那几个数）

Keil 编译完会在 map 文件里报告：
- **Code(RO 的一部分)**：代码本身。
- **RO-data**：常量、字符串字面量。
- **RW-data**：已初始化变量。Flash 里存初值，上电后搬到 SRAM。
- **ZI-data**：零初始化变量。只占 SRAM，不占 Flash。FreeRTOS 的堆（heap_4 的 `ucHeap[]`）就在这里。

本项目 APP 大约 `Code=22KB RO=460 RW=180 ZI=19.8KB`——SRAM 里大头是 FreeRTOS 堆。这直接回答"你 20KB RAM 都花哪了"。

### 1.13 BOOT 引脚（芯片怎么知道从哪儿启动）

STM32F103 有 BOOT0/BOOT1 引脚，上电时硬件采样决定启动方式：
- **BOOT0=0**：从主 Flash 启动，硬件把 `0x08000000` 映射到取指地址 `0x00000000`。**这是本项目用的方式**。
- BOOT0=1, BOOT1=0：从系统存储器（ST 出厂烧的 ISP bootloader）启动。
- BOOT0=1, BOOT1=1：从 SRAM 启动。

所以 `0x08000000` 和 `0x00000000` 在 BOOT0=0 时是同一块内存的两个别名。这就是为什么硬件上电"从地址 0 取 MSP、从地址 4 取 Reset"等价于"从 `0x08000000` 取"。

---

## 2. Bootloader 的分区设计（为什么这样切）

在 [bootloader.h](file:///d:/Projects/JL%20project/range%20hood/bootloader%20YJ/BSP_BOOT/bootloader.h) 里定义：

```c
#define APP_BASE_ADDR         0x08004000UL     /* APP起始地址 */
#define APP_MAX_SIZE          0x7C000UL        /* APP最大 496KB */
#define F1_FLASH_PAGE_SIZE    0x800UL          /* 每页 2KB */
```

为什么这样切？三个约束共同决定：

1. **Bootloader 占前 16KB**：要给"接收/校验/跳转"留够空间，16KB 足够（实际只用了 8.4KB），同时是 2KB 页的整数倍。
2. **APP 从 0x08004000 起**：刚好在 Bootloader 后面，且 `0x4000` 满足 VTOR 对齐约束（VTOR 要求地址是 0x200 的倍数，因为向量表最小 512B 向上取 2 的幂）。所以 0x4000 是双约束的最小正解。
3. **APP 最大 496KB(0x7C000)**：512KB Flash - 16KB Bootloader = 496KB，全给 APP。
4. **Bootloader 区永不擦除**：它是"救援通道"，擦了就只能拆机上 SWD。这就是"不变砖"的根本保证。

---

## 3. 上电启动流程（main 函数逐行讲）

源码在 [bootloader.c](file:///d:/Projects/JL%20project/range%20hood/bootloader%20YJ/BSP_BOOT/bootloader.c#L350) 第 350 行：

```c
int main(void)
{
    HAL_Init();                     /* 1. HAL 库初始化,启动 SysTick 1ms tick */
    SystemClock_Config();           /* 2. 配置系统时钟到 72MHz */
    MX_GPIO_Init();                 /* 3. GPIO 初始化,PB0 是升级按键 */
    MX_DMA_Init();                  /* 4. DMA 控制器初始化 */
    MX_USART1_UART_Init();          /* 5. 串口1初始化,接收走 DMA Normal */

    if(!boot_is_key_pressed())      /* 6. 没按 PB0:尝试跳 APP */
    {
        (void)boot_jump_to_app();   /*    返回 0 = APP 无效,顺延进升级 */
    }

    boot_run_update();              /* 7. 升级主循环,永不返回 */

    while(1) { ; }                  /* 8. 跑到这里说明出问题了,死循环 */
}
```

**逐行讲为什么：**

| 行 | 为什么这么写 |
|---|---|
| HAL_Init | 不初始化 HAL 库，后面的 `HAL_GPIO_ReadPin`/`HAL_UART_Receive_DMA` 全都不能用 |
| SystemClock_Config | 配 PLL 把外部 8MHz 倍频到 72MHz，否则串口波特率全算错。代码里 `RCC_PLL_MUL9` 就是 8×9=72 |
| MX_GPIO_Init | PB0 配成"上拉输入"——按键按下时引脚被拉到地(GND)，读出来是 `GPIO_PIN_RESET` |
| MX_DMA_Init | DMA 控制器得先初始化，后面 UART 才能用 DMA 收发 |
| MX_USART1_UART_Init | 115200 8N1，**接收走 DMA Normal 模式**（区别于 APP 的 Circular） |
| `boot_is_key_pressed()` | 没按键时直接跳 APP——**正常使用每次上电都跳 APP，Bootloader 只是兜底** |
| `boot_run_update` | 按了按键或 APP 无效，进升级循环 |

**关键认知**：**Bootloader 不只是"按键才进升级"**。`boot_jump_to_app()` 内部会校验 APP 向量表是否合法，不合法返回 0，照样落到升级模式。所以 **空芯片/升级失败的芯片上电自动处于"待接收固件"状态**，这是"不变砖"体验的根本。

### 3.1 按键检测的细节

```c
uint8_t boot_is_key_pressed(void)
{
    if(HAL_GPIO_ReadPin(BOOT_KEY_GPIO_PORT, BOOT_KEY_GPIO_PIN) == GPIO_PIN_RESET)
        return 1U;
    return 0U;
}
```

PB0 在 CubeMX 里配成 **GPIO_MODE_INPUT + GPIO_PULLUP**（上拉输入）：
- 按键没按：引脚被上拉电阻拉到 3.3V，读出来是 `GPIO_PIN_SET`（1）。
- 按键按下：引脚接 GND，读出来是 `GPIO_PIN_RESET`（0）。

所以"按下"=读到 `RESET`。这是为啥判断写反直觉——**检测的是低电平**。

---

## 4. 跳转到 APP：五个步骤逐条讲为什么

这是整个 Bootloader 最关键、也是面试最爱挖的代码。源码在 [bootloader.c:202-242](file:///d:/Projects/JL%20project/range%20hood/bootloader%20YJ/BSP_BOOT/bootloader.c#L202)。

```c
uint8_t boot_jump_to_app(void)
{
    uint32_t app_msp_stack   = *(volatile uint32_t *)APP_BASE_ADDR;        /* APP 向量表[0] */
    uint32_t app_reset_entry = *(volatile uint32_t *)(APP_BASE_ADDR + 4U); /* APP 向量表[1] */

    if(boot_is_app_valid(app_msp_stack, app_reset_entry) == 0U)
        return 0U;                       /* APP 非法,留在升级模式 */

    /* 1. 停掉 Bootloader 的 SysTick */
    SysTick->CTRL = 0U;
    SysTick->LOAD = 0U;
    SysTick->VAL  = 0U;

    /* 2. 关闭并清除全部 NVIC 中断 */
    for(idx = 0U; idx < 8U; idx++) {
        NVIC->ICER[idx] = 0xFFFFFFFFU;
        NVIC->ICCP[idx] = 0xFFFFFFFFU;
    }

    /* 3. 反初始化 Bootloader 用过的外设 */
    HAL_UART_DeInit(&huart1);
    HAL_DMA_DeInit(&hdma_usart1_rx);
    HAL_DMA_DeInit(&hdma_usart1_tx);

    /* 4. 向量表重定位 */
    SCB->VTOR = APP_BASE_ADDR;

    /* 5. 关中断 → 切栈 → 跳复位向量 */
    __disable_irq();
    __set_MSP(app_msp_stack);
    ((void (*)(void))app_reset_entry)();

    return 1U;  /* 实际上跳过去就不返回了 */
}
```

### 4.1 取向量：`*(volatile uint32_t *)APP_BASE_ADDR`

- `APP_BASE_ADDR` = `0x08004000`，这是 APP 向量表第 0 个字（MSP 初值）。
- `+4U` 是第 1 个字（Reset_Handler 入口）。
- **为什么 `volatile`**：编译器看到"读一个地址"会以为是常量，可能只读一次把值缓存进寄存器，后续都从寄存器取——但这里我们要读的是 **Flash 真实内容**（升级后内容会变），必须每次从内存取。`volatile` 强制编译器每次都生成真正的 load 指令。这是嵌入式高频考点。

### 4.2 镜像合法性校验：`boot_is_app_valid`

```c
static uint8_t boot_is_app_valid(uint32_t msp, uint32_t entry)
{
    if((msp < RAM_BASE_ADDR) || (msp >= RAM_END_ADDR))            return 0U;  /* MSP 必须在 SRAM 区 */
    if((entry < APP_BASE_ADDR) || (entry >= APP_FLASH_END_ADDR)) return 0U;  /* 入口必须在 APP 区 */
    if((entry & 1U) == 0U)                                        return 0U;  /* Thumb 位 */
    return 1U;
}
```

**三项校验各自防什么：**

| 检查 | 不检查会怎样 | 防止的场景 |
|---|---|---|
| MSP 落在 SRAM(0x20000000~0x20010000) | MSP 是 `0xFFFFFFFF`（空 Flash），切过去栈顶在非法地址，第一次压栈就 HardFault | 空芯片/刚擦完没写的 APP 区 |
| 入口在 APP 区(0x08004000~0x0807FFFF) | 入口是 `0xFFFFFFFF`，跳过去执行 `0xFFFFFFFE`（bit0 清 0 后）的指令 → 非法 | 同上 |
| bit0=1(Thumb 位) | 跳到偶数地址，Cortex-M 不允许 → 异常 | 损坏的镜像头 |

实测 APP 编出来的向量：`MSP=0x20004E30`（在 SRAM 内）、`entry=0x08004279`（bit0=1，落在 APP 区）→ 校验通过。

### 4.3 五个跳转步骤逐条讲为什么

| # | 步骤 | 不做会怎样 |
|---|---|---|
| 1 | 停 SysTick (`CTRL/LOAD/VAL=0`) | Bootloader 用 SysTick 中断驱动 `HAL_GetTick`，如果在跳转瞬间 SysTick 还开着，挂起的中断会在 APP 刚启动、向量表还没装好时触发，跑到 `Default_Handler` 死循环 → 跑飞 |
| 2 | 清 ICER/ICPR(8 个寄存器全写 1) | Bootloader 开了 USART1/DMA 中断。如果跳转时还挂着这些中断，APP 启动文件里没写对应 handler，会落到 C 启动文件的弱符号 `Default_Handler`（死循环）；挂起标志不清还会"补触发"——稍一开中断立刻炸 |
| 3 | DeInit UART/DMA | Bootloader 的 UART/DMA 可能停在中间状态（半块 DMA 没完成）。APP 的 `MX_USART1_UART_Init` 假设寄存器是复位态，配置失败或行为异常。DeInit 把外设恢复到复位默认 |
| 4 | 设 VTOR = APP_BASE_ADDR | 不设的话下次来中断，CPU 还去 `0x08000000`（Bootloader 向量表）找 handler——**Bootloader 的旧 handler 地址 + APP 的新 MSP = 灾难**。APP 的 `SystemInit` 里 `VECT_TAB_OFFSET=0x4000` 会再设一次，双保险 |
| 5a | `__disable_irq()` 先关中断 | 紧接着要 `__set_MSP` 切栈指针。如果在切栈的瞬间来中断，CPU 会压栈到新栈顶——但新栈内容还是未知的，写坏栈。关中断窗口只有几条指令，APP 启动后自己会开 |
| 5b | `__set_MSP(app_msp_stack)` | APP 的栈位置只有 APP 自己知道（链接器安排的），Bootloader 硬编码一个值是非法的。直接读 APP 向量表第 0 字就是它的栈顶 |
| 5c | `((void(*)(void))app_reset_entry)()` | 把 32 位数强转成函数指针再调用，等价于"跳到这个地址执行"。Cortex-M 的函数调用会自动把 bit0 当 Thumb 位用，所以这里直接跳 |

**一句话总结这五步：把 Bootloader 的"痕迹"全部抹干净，给 APP 一个像刚上电一样的干净现场。**

### 4.4 `return 1U` 其实执行不到

最后一行 `return 1U;` 不会真的执行到——前一行函数指针调用跳到 APP 的 Reset_Handler 后，APP 重新走一遍 C 启动流程，栈都换了，根本不会回到 Bootloader。写 `return` 只是为了让编译器别报"non-void function should return a value"。

---

## 5. 升级模式：协议帧结构

### 5.1 帧格式

源码状态机对应 [boot_run_update](file:///d:/Projects/JL%20project/range%20hood/bootloader%20YJ/BSP_BOOT/bootloader.c#L248)：

```
┌────────────┬────────────────────────┬────────────┐
│ 4 字节长度 │      payload 固件      │ 4 字节 CRC │
│ (小端 LE)  │ (按 128B/块分批发送)   │ (小端 LE)  │
└────────────┴────────────────────────┴────────────┘
```

### 5.2 为什么这样设计

| 设计 | 为什么 |
|---|---|
| 长度放帧头 | 状态机要先知道"要收多少字节"，才能做合法性检查(拒 0/超界)、定擦写范围、定每块 DMA 装载量。放尾部就成了"收完才知道",无法提前拒帧 |
| CRC 放尾部 | 整包 CRC 只能在数据发完后算完。Bootloader 用 `crc32_update` **流式边收边算**，CRC 到达时只差 finalize 一步，零额外缓存 |
| 整包一次 CRC | 状态机最简:整帧重传逻辑;每块 CRC+块号是 XMODEM 思路,代价是逐块停等,慢 |
| 长度字段 4 字节 | 固件最大 496KB,2 字节(65535)不够,4 字节足够且对齐 |
| 小端序 | STM32 和 PC 都是小端,直接 `memcpy` 当 uint32_t 用,不用转换 |

### 5.3 握手时序（升级的真实流程）

```
上位机(PC)                         Bootloader
   │  [4B 长度]                     │ 收长度→合法性检查→Unlock→全片擦除(~5s)
   │◄──────'R'(擦除完成)─────────────│ ← 关键握手!擦除期间发数据必丢
   │  [固件, 128B/块, 块间10ms]      │ 每块: DMA收满→按字写Flash→更新CRC
   │  [4B CRC32] (发前延时20ms)     │ flush最后一字→Lock→比对CRC
   │◄───────'K' 或 'N'──────────────│ 'K'=通过→跳转; 'N'=失败→等整帧重发
```

三个握手字符的含义：
- `'R'` (Ready)：擦除完成，可以发数据了。
- `'K'` (ACK)：CRC 校验通过，马上跳 APP。
- `'N'` (NAK)：失败，请重发整帧。

### 5.4 三个延时为什么是这些值

| 延时 | 值 | 原因 |
|---|---|---|
| 等 'R' 超时 | 20s | 全片擦除 248 页约 5s，留 4 倍裕量 |
| 块间延时 | 10ms | 覆盖"写 32 字 + 重启 DMA"的空窗（约 5ms），否则下一块前几字节丢 |
| CRC 尾前延时 | 20ms | Bootloader 写最后一块 + flush 需要几 ms，这几 ms 空窗里发的 CRC 尾会丢进 ORE |

这些值是 [ota_host.py](file:///d:/Projects/JL%20project/range%20hood/tools/ota_host.py#L147) 的 `--inter-delay 10 --tail-delay 20` 默认值，是踩过坑调出来的。

---

## 6. 升级主循环逐行讲

源码 [boot_run_update](file:///d:/Projects/JL%20project/range%20hood/bootloader%20YJ/BSP_BOOT/bootloader.c#L248):

```c
void boot_run_update(void)
{
    /* 一堆局部变量声明 */
    for(;;)  /* 永不退出,失败也重来 */
    {
        /* === 第一步:收 4 字节帧头(长度,小端) === */
        if(uart_dma_block_recv((uint8_t *)&payload_total_len, 4U, BOOT_HEAD_TIMEOUT_MS) == 0U)
            continue;  /* 超时,重新等下一帧 */

        /* 合法性检查:拒 0 长度和超界长度 */
        if((payload_total_len == 0U) || (payload_total_len > (APP_MAX_SIZE - 4U)))
            continue;

        /* === 第二步:Unlock Flash,全片擦除 === */
        HAL_FLASH_Unlock();
        if(flash_erase_all_app_page() == 0U) {
            HAL_UART_Transmit(&huart1, (uint8_t)'N', 1U, 100U);  /* 擦除失败 */
            HAL_FLASH_Lock();
            continue;
        }
        flash_write_state_reset();  /* 复位写状态机 */
        crc_temp = 0xFFFFFFFFU;    /* CRC 初值 */
        HAL_UART_Transmit(&huart1, (uint8_t)'R', 1U, 100U);  /* 告诉上位机可以发数据 */

        /* === 第三步:循环收数据块,边写 Flash 边算 CRC === */
        remain_byte = payload_total_len;
        recv_ok = 1U;
        while(remain_byte > 0U) {
            recv_chunk_len = (remain_byte > BOOT_CHUNK_SIZE) ? BOOT_CHUNK_SIZE : (uint16_t)remain_byte;
            if(uart_dma_block_recv(recv_buf, recv_chunk_len, BOOT_DATA_TIMEOUT_MS) == 0U) {
                recv_ok = 0U;
                break;
            }
            for(i = 0U; i < recv_chunk_len; i++)
                flash_write_byte(recv_buf[i]);           /* 攒 4 字节写一次 */
            crc_temp = crc32_update(crc_temp, recv_buf, recv_chunk_len);  /* 流式更新 */
            remain_byte -= recv_chunk_len;
        }

        if(recv_ok == 0U) {
            HAL_UART_Transmit(&huart1, (uint8_t)'N', 1U, 100U);
            HAL_FLASH_Lock();
            continue;
        }

        flash_write_flush();  /* 凑不满 4 字节的尾部用 0xFF 填满写一次 */
        HAL_FLASH_Lock();     /* 重新锁 Flash,防止误写 */

        crc_calculate_result = crc32_finalize(crc_temp);  /* 末尾异或得最终 CRC */

        /* === 第四步:收上位机发来的 4 字节 CRC === */
        if(uart_dma_block_recv((uint8_t *)&crc_from_uart, 4U, BOOT_TAIL_TIMEOUT_MS) == 0U)
            continue;

        /* === 第五步:比对,成功跳 APP,失败等重发 === */
        if(crc_from_uart == crc_calculate_result) {
            HAL_UART_Transmit(&huart1, (uint8_t)'K', 1U, 100U);
            (void)boot_jump_to_app();  /* 校验跳转,跳不过去留在循环 */
        } else {
            HAL_UART_Transmit(&huart1, (uint8_t)'N', 1U, 100U);
        }
    }
}
```

**几个关键点：**

1. **`for(;;)` 永不退出**：失败也 `continue` 重来，保证设备永远在"等下一帧"的状态——升级到一半拔线、断电都不会死。
2. **`HAL_FLASH_Unlock/Lock` 成对**：Flash 默认是写保护的，写前必须解锁，写完必须重新锁上，防止程序跑飞时误写。
3. **长度合法性检查**：`len == 0` 或 `len > APP_MAX_SIZE - 4` 直接弃帧。`-4` 是要给 CRC 尾留位置。这防住了"上位机发个 500KB 假固件"这种攻击/失误。
4. **'R' 在擦除完成后才发**：擦除 5s 期间发数据必丢，所以必须握手。
5. **写 Flash 失败直接 `Error_Handler`**：编程失败（Flash 寿命/电压异常）是硬件级故障，不能静默——但仔细看，**只有 `flash_write_byte` 里失败调 `Error_Handler`，`HAL_FLASHEx_Erase` 失败只是返 0 让上层重试**。差别在严重性：擦除失败可能只是一次抖动，编程失败说明 Flash 出问题了。
6. **发 'K' 用 `HAL_UART_Transmit` 阻塞发送**：必须等字节移位寄存器真正发完才跳转，否则 'K' 还在缓冲里没出去 CPU 就跳走了，上位机收不到 'K'。`HAL_UART_Transmit` 是阻塞的，返回时数据已发完。

---

## 7. DMA 分块接收（带超时 + 错误检测）

源码 [uart_dma_block_recv](file:///d:/Projects/JL%20project/range%20hood/bootloader%20YJ/BSP_BOOT/bootloader.c#L134):

```c
static uint8_t uart_dma_block_recv(uint8_t *dst, uint16_t n, uint32_t timeout_ms)
{
    uint32_t tickstart;

    if(HAL_UART_Receive_DMA(&huart1, dst, n) != HAL_OK)
        return 0U;  /* DMA 启动失败 */

    tickstart = HAL_GetTick();
    while(huart1.RxState != HAL_UART_STATE_READY)  /* DMA 收满才置 READY */
    {
        if((HAL_GetTick() - tickstart) > timeout_ms) {
            (void)HAL_UART_AbortReceive(&huart1);  /* 必须释放句柄,否则下次 BUSY */
            return 0U;
        }
    }

    /* ORE/FE 让 HAL 提前回 READY,但字节没收够 */
    if((huart1.hdmarx != NULL) && (__HAL_DMA_GET_COUNTER(huart1.hdmarx) != 0U))
        return 0U;
    return 1U;
}
```

### 7.1 四个必懂的细节

**Q1: 为什么轮询 `RxState` 不用信号量？**
Bootloader 是 **裸机**(没有 RTOS)，DMA 传输完成中断把 `RxState` 置回 `READY`，轮询是最简单可靠的同步方式。如果上 RTOS 可以用信号量让出 CPU，但 Bootloader 跑升级时本来就没别的事干，轮询无所谓。

**Q2: 超时判断 `(HAL_GetTick() - tickstart) > timeout_ms` 为什么健壮？**
`HAL_GetTick()` 是 `uint32_t`，约 49.7 天回绕一次。假设 `tickstart = 0xFFFFFFF0`，10ms 后 `HAL_GetTick()` 回绕到 `0x00000000`，看起来"变小了"，但无符号减法 `0x00000000 - 0xFFFFFFF0 = 0x00000010 = 16`，数学上 `(a-b) mod 2^32` 永远是对的。这是嵌入式面试高频题。

**Q3: `__HAL_DMA_GET_COUNTER` 的妙用？**
它读 DMA 的 NDTR 寄存器（剩余计数）。你让它收 128 字节，正常收完 NDTR=0。但串口线路出 **ORE(溢出) / FE(帧错误)** 时，HAL 内部会提前终止 DMA，`RxState` 也变 READY，**但字节没收够**——NDTR 不为 0。用"READY 但 NDTR≠0"判定异常，比单纯等 READY 健壮得多。

**Q4: 超时后为什么必须 `AbortReceive`？**
HAL 的 UART 句柄停在 BUSY 状态，下一次 `HAL_UART_Receive_DMA` 会返回 `HAL_BUSY`。`AbortReceive` 强制把状态机复位到 READY，下次才能重新启动。

### 7.2 三级超时

```c
#define BOOT_HEAD_TIMEOUT_MS   2000U   /* 帧头:等用户操作,可以长 */
#define BOOT_DATA_TIMEOUT_MS   1000U   /* 数据块:128B@115200 只要 11ms,1s 很宽裕 */
#define BOOT_TAIL_TIMEOUT_MS   1000U   /* CRC 尾 */
```

不同阶段用不同超时是因为容忍度不同：帧头要等用户按按钮+点上位机，可以慢；数据块必须连续，1s 没动静就是出问题了。

---

## 8. Flash 编程细节（F1 的物理约束）

### 8.1 按字写：攒 4 字节

源码 [flash_write_byte](file:///d:/Projects/JL%20project/range%20hood/bootloader%20YJ/BSP_BOOT/bootloader.c#L61):

```c
static void flash_write_byte(uint8_t b)
{
    s_word_buf |= ((uint32_t)b) << (s_byte_cnt * 8U);  /* 小端序攒字节 */
    s_byte_cnt++;

    if(s_byte_cnt == 4U) {  /* 攒够 4 字节才写一次 */
        if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, s_flash_write_addr, s_word_buf) != HAL_OK)
            Error_Handler();
        s_flash_write_addr += 4U;
        s_word_buf = 0U;
        s_byte_cnt = 0U;
    }
}
```

**为什么这么写？**
- STM32F1 的 Flash **硬件只支持按字(32bit)编程**，不支持只写 1 字节。串口来的却是字节流。
- 解决办法：用 `s_word_buf` 攒 4 字节再调一次 `HAL_FLASH_Program`。
- **小端序攒字节**：第 0 字节放最低 8 位（`<<0`），第 1 字节放次低 8 位（`<<8`）……这样写进 Flash 的 4 字节顺序和字节流顺序一致。

### 8.2 凑不满 4 字节的尾部：用 0xFF 填充

```c
static void flash_write_flush(void)
{
    while(s_byte_cnt > 0U && s_byte_cnt < 4U) {
        s_word_buf |= 0xFFU << (s_byte_cnt * 8U);  /* 0xFF 填充 */
        s_byte_cnt++;
    }
    if(s_byte_cnt == 4U) {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, s_flash_write_addr, s_word_buf);
        /* ... */
    }
}
```

**为什么填 0xFF 而不是 0x00？**
- Flash 擦除后是 **全 1（0xFF）**，编程只能把 **1 改成 0**。
- 尾部凑不满 4 字节时，用 0xFF 填充等于"没改"，**不会破坏数据**。
- 如果填 0x00，会把那几个字节写成 0，覆盖了原本的 0xFF，下次不擦除再写就出问题了。

### 8.3 全片擦除：循环按页擦

源码 [flash_erase_all_app_page](file:///d:/Projects/JL%20project/range%20hood/bootloader%20YJ/BSP_BOOT/bootloader.c#L106):

```c
static uint8_t flash_erase_all_app_page(void)
{
    FLASH_EraseInitTypeDef erase_cfg;
    uint32_t page_error = 0U;
    uint32_t cur_addr = APP_BASE_ADDR;

    while(cur_addr < (APP_BASE_ADDR + APP_MAX_SIZE))
    {
        erase_cfg.TypeErase    = FLASH_TYPEERASE_PAGES;  /* 模式:按页擦 */
        erase_cfg.PageAddress  = cur_addr;
        erase_cfg.NbPages      = 1U;                      /* 每次只擦 1 页 */
        if(HAL_FLASHEx_Erase(&erase_cfg, &page_error) != HAL_OK)
            return 0U;
        cur_addr += F1_FLASH_PAGE_SIZE;  /* 0x800 = 2KB,推进到下一页 */
    }
    return 1U;
}
```

**为什么这样写？**
- F1 中容量芯片 Flash 每页 **2KB**，擦除必须按整页（不能只擦几个字节）。
- APP 区 496KB ÷ 2KB = **248 页**，循环 248 次。
- 为什么每次只擦 1 页而不是 248 页一次性擦？因为 HAL 的 `NbPages` 一次擦多页行为依赖芯片型号，逐页擦最稳。代价是慢（约 5s），但升级本来不是高频操作。
- 擦除期间不能写 Flash，也不能读 Flash（会让总线 stall），所以这 5s 内上位机发数据必丢——这就是必须等 'R' 握手的原因。

---

## 9. CRC32：为什么和 zlib 兼容

源码 [crc32.c](file:///d:/Projects/JL%20project/range%20hood/bootloader%20YJ/BSP_BOOT/crc32.c):

```c
uint32_t crc32_update(uint32_t crc, const uint8_t *data, uint32_t len)
{
    for (i = 0; i < len; i++) {
        crc ^= data[i];
        for (j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320u & (uint32_t)(0u - (crc & 1u)));
        }
    }
    return crc;
}

uint32_t crc32_finalize(uint32_t crc)
{
    return crc ^ 0xFFFFFFFFu;
}
```

### 9.1 几个关键参数

| 参数 | 值 | 含义 |
|---|---|---|
| 多项式 | `0xEDB88320` | 标准 CRC-32 多项式 `0x04C11DB7` 的位反转形式 |
| 初值 | `0xFFFFFFFF` | 进入 `crc32_update` 前 `crc_temp = 0xFFFFFFFF` |
| 结果异或 | `0xFFFFFFFF` | `crc32_finalize` 里 `crc ^ 0xFFFFFFFF` |

### 9.2 为什么要选这一套参数

**因为它和 Python 标准库 `zlib.crc32` 完全一致**。上位机一行代码：
```python
crc = zlib.crc32(data) & 0xFFFFFFFF
```
就能算出和 Bootloader 一样的 CRC。如果用别的多项式/初值，上位机得自己实现一遍 CRC32 算法——能用，但麻烦且容易写错。**让上位机能用标准库，是这个设计选择的回报。**

### 9.3 为什么用流式接口

`crc32_update` 一块一块喂，`crc32_finalize` 末尾异或一次。好处：
- **不需要把整包数据缓存到 RAM** 再算（496KB 远超 64KB SRAM）。
- 收一块算一块，CRC 到达时只差 finalize 一步，零额外缓存。

### 9.4 那行位运算的小技巧

```c
(0xEDB88320u & (uint32_t)(0u - (crc & 1u)))
```

`0u - (crc & 1u)` 的妙处：
- 如果 `crc & 1 == 0`：`0u - 0u = 0`，和多项式 `&` 后是 0，等价于"不变"。
- 如果 `crc & 1 == 1`：`0u - 1u = 0xFFFFFFFF`（无符号回绕），和多项式 `&` 后还是多项式，等价于"异或多项式"。

这是 `crc & 1 ? 0xEDB88320 : 0` 的无分支写法，避免 if 跳转，流水线友好。

---

## 10. APP 端的配合（双工程怎么不互相打架）

APP 工程在 `RTOS  YJ/`，它要做两件配合工作：

### 10.1 修改链接地址：`.sct` 文件

[RTOS  YJ.sct](file:///d:/Projects/JL%20project/range%20hood/RTOS%20%20YJ/MDK-ARM/RTOS%20%20YJ/RTOS%20%20YJ.sct):
```
LR_IROM1 0x08004000 0x0007C000  {  ; 起址 0x08004000, 大小 496KB
  ER_IROM1 0x08004000 0x0007C000  { ... }
  ...
}
```
起址 `0x08004000` 而不是默认的 `0x08000000`。**只改向量表偏移不改链接地址是踩过的大坑**——编译出来代码会从 0x08000000 开始，烧 APP 时把 Bootloader 直接覆盖，芯片变砖只能 SWD 救。

### 10.2 修改向量表偏移：`system_stm32f1xx.c`

[system_stm32f1xx.c:110](file:///d:/Projects/JL%20project/range%20hood/RTOS%20%20YJ/Core/Src/system_stm32f1xx.c#L110):
```c
#define VECT_TAB_OFFSET         0x00004000U  /* APP linked at 0x08004000 (16KB bootloader) */
```
和:
```c
void SystemInit(void)
{
    SCB->VTOR = VECT_TAB_BASE_ADDRESS | VECT_TAB_OFFSET;  /* 设向量表位置 */
}
```

APP 上电后 `SystemInit` 会被启动汇编调用，它再设一次 VTOR。和 Bootloader 跳转前的设置等价，**双保险**。

### 10.3 两个工程独立的保证

- Bootloader 工程只编 `BSP_BOOT` 下的文件（不含 APP 的 FreeRTOS、不含 `Core/main.c`）。
- APP 工程从 `0x08004000` 起链接。
- 最终用 **hex 首地址 + map 文件 `LR_IROM1`** 双重确认两个工程的物理边界没重叠。

---

## 11. 上位机脚本流程

[ota_host.py](file:///d:/Projects/JL%20project/range%20hood/tools/ota_host.py):

```python
def cmd_flash(args):
    data = open(args.file,'rb').read()           # 1. 读固件 bin
    if len(data)==0 or len(data)>0x7C000-4: exit # 2. 越界检查
    ser = serial.Serial(args.port, args.baud)    # 3. 打开串口
    print("按住 BOOT 键(PB0)并复位设备进入升级模式...")
    time.sleep(3)                                 # 4. 等用户操作

    for attempt in range(1, args.retries+1):
        ser.reset_input_buffer()                  # 5. 清空接收缓冲
        if not send_frame(...): continue          # 6. 发帧+等'R'

        # 7. 等 'K' 或 'N'
        deadline = time.time() + 10.0
        while time.time() < deadline:
            b = ser.read(1)
            if b == ACK:  ok=True; break          # 通过,设备已跳 APP
            elif b == NAK: break                  # 失败,重发
        if ok: break
```

```python
def send_frame(ser, data, chunk, inter_delay_ms, tail_delay_ms):
    ser.write(struct.pack("<I", len(data)))       # 1. 发 4 字节长度(小端)
    if not wait_for(ser, READY, 20.0, ...):       # 2. 等 'R'(擦除完成)
        return False
    for off in range(0, len(data), chunk):        # 3. 分块发固件
        ser.write(data[off:off+chunk])
        time.sleep(inter_delay_ms/1000.0)         #    块间延时 10ms
    time.sleep(tail_delay_ms/1000.0)              # 4. CRC 尾前延时 20ms
    ser.write(struct.pack("<I", zlib.crc32(data) & 0xFFFFFFFF))  # 5. 发 4 字节 CRC
    return True
```

**亮点**：
- `zlib.crc32(data)` 一行算 CRC，和 bootloader 算出来必然相等（同一个算法）。
- `struct.pack("<I", ...)` 把 32 位数按小端打包成 4 字节，和 bootloader `*(uint32_t*)` 解析方式一致。
- 三次重试 + 超时，是"防呆"设计。

---

## 12. 完整升级时序（一次成功的升级）

```
时间 │ 上位机 PC                          │ Bootloader STM32
─────┼─────────────────────────────────────┼──────────────────────────────────
 t0  │ 提示"按住 BOOT+复位"               │ 用户按下 PB0 后复位
 t1  │                                    │ 上电: HAL_Init→Clock→GPIO→DMA→UART
 t2  │                                    │ boot_is_key_pressed()=1 → boot_run_update()
 t3  │ 打开串口,清空接收缓冲              │ 在 uart_dma_block_recv 等帧头(2s 超时)
 t4  │ 发 [4B 长度=固件字节数]            │ 收到长度 → 合法性检查 → HAL_FLASH_Unlock
 t5  │ 等 'R'(20s 超时)                  │ flash_erase_all_app_page() 循环 248 页(约 5s)
 t6  │                                    │ 擦除完成 → flash_write_state_reset → 发 'R'
 t7  │ 收到 'R',开始分块发固件           │
     │   发 128B → 等 10ms → 发 128B ... │ 每块: DMA 收满 → 攒 4B 写 Flash → crc32_update
 ... │ (重复 ~800 块)                    │
 t8  │ 发完最后一块 → 等 20ms            │ flush 最后一字 → HAL_FLASH_Lock → finalize CRC
 t9  │ 发 [4B CRC32]                     │ uart_dma_block_recv 收 4B
 t10 │                                    │ crc_from_uart == crc_calculate_result?
 t11 │                                    │ 相等: 发 'K' → boot_jump_to_app()
 t12 │ 收到 'K'                            │ 五步清理现场 → 跳到 APP Reset_Handler
 t13 │ "升级成功!"                         │ APP 开始跑,FreeRTOS 启动
```

---

## 13. 常见问题与排查速查表

| 现象 | 最可能原因 | 怎么定位 |
|---|---|---|
| 跳转后 APP 跑飞/HardFault | NVIC 没清、VTOR 没设、MSP 错、入口 Thumb 位丢 | 逐项对照第 4 节五步;Keil 调试看 MSP/PC 落点 |
| CRC 永远校验失败 | 块间隔不够丢字节;擦除期间发了数据;ORE 错误 | 串口抓包比对;增大 inter-delay;确认等了 'R' |
| 收到 'K' 但设备没进 APP | test 镜像向量非法(预期行为);或 APP 本身跑飞 | 用真实 APP bin 验证;查 boot_is_app_valid 拒因 |
| 编译过但 APP 覆盖了 bootloader | Keil Target 里 IROM 起址还是 0x08000000 | 看 map 文件 LR_IROM1 基址 |
| DMA 收不到任何数据 | Normal 模式收满后没重启;句柄 BUSY 没处理 | 调试看 NDTR 计数、RxState |
| 跳转后马上 HardFault | 启动代码 .data/.bss 没初始化好 / VTOR 没指对 | 调试单步看是否进 main |

---

## 14. 已知局限和演进方向（主动讲加分）

1. **无双区备份(A/B)**：升级中掉电 → APP 区被擦 → 只能重传。工业方案是双区 + 状态标志 + 回滚。
2. **无固件签名/加密**：任何人可刷任意固件。演进:Flash 里存公钥,验签后再写。
3. **无版本防回滚、无逐包断点续传**。
4. **串口无流控依赖延时规避**：协议层解耦后可整体换成 CAN/UDS 或 WiFi 模组 OTA，bootloader 侧协议不变。

---

## 15. 面试文档名词速查表（看完这个回头看 §1 就懂了）

把 [项目面试深度讲解.md](file:///d:/Projects/JL%20project/range%20hood/doc/项目面试深度讲解.md) 第 1 章里高频出现的名词集中解释：

| 名词 | 一句话解释 |
|---|---|
| **IAP** | In-Application Programming，芯片自己跑着程序时刷自己的 Flash |
| **ISP** | In-System Programming，用烧录器硬件方式刷 |
| **MSP** | Main Stack Pointer，主栈指针，函数调用/中断压栈的临时内存顶 |
| **Reset_Handler** | C 启动汇编入口，做 .data 拷贝/.bss 清零后跳 __main |
| **Thumb 位** | 函数地址 bit0=1，Cortex-M 只认 Thumb 指令集 |
| **VTOR** | 向量表偏移寄存器，告诉 CPU 当前向量表在哪个地址 |
| **向量表** | 一段连续内存，存 MSP/Reset/各中断 handler 入口 |
| **NVIC** | Cortex-M 中断控制器 |
| **ICER/ICPR** | NVIC 的中断使能清除/挂起清除寄存器 |
| **ISER/ISPR** | 对应的使能/挂起寄存器 |
| **DMA** | 直接内存访问引擎，外设↔内存搬运不占 CPU |
| **Normal 模式** | DMA 收满 N 字节就停，需重启再收 |
| **Circular 模式** | DMA 收满后从头循环收 |
| **NDTR** | DMA 剩余计数器，收到一个减 1 |
| **ORE** | 串口 Overrun Error，字节没读走被覆盖 |
| **FE** | 串口 Framing Error，停止位没对上 |
| **SysTick** | 内核 24 位定时器，HAL 用它做 1ms tick |
| **HAL_GetTick** | 返回毫秒计数(uint32_t，49.7 天回绕) |
| **小端序(LE)** | 低位字节放低地址，STM32/PC 都是小端 |
| **字(word)** | 4 字节，F1 Flash 只能按字编程 |
| **页(Page)** | F103 中容量 2KB/页，擦除按整页 |
| **CRC32** | 32 位循环冗余校验指纹，0xEDB88320 多项式 |
| **zlib.crc32** | Python 标准库 CRC32，和本项目算法一致 |
| **Scatter File(.sct)** | Keil 链接脚本，定义代码/变量放哪个地址 |
| **LR_IROM1/ER_IROM1** | 链接器的 Load Region / Exec Region 名字 |
| **IROM** | 指令 ROM 段（代码） |
| **.data (RW)** | 已初始化变量，Flash 存初值+SRAM 运行 |
| **.bss (ZI)** | 零初始化变量，只占 SRAM |
| **Code/RO/RW/ZI** | map 文件四项，分别=代码/常量/已初始化/零初始化 |
| **hex** | Intel HEX 文本格式，带地址，给烧录器用 |
| **bin** | 纯二进制，不带地址，给 IAP 升级用 |
| **VECT_TAB_OFFSET** | CubeMX 生成的向量表偏移宏，APP 端设 0x4000 |
| **BOOT0/BOOT1** | STM32 启动选择引脚，BOOT0=0 从 Flash 启动 |
| **0x08000000** | Flash 起始地址(BOOT0=0 时也映射到 0x00000000) |
| **0x08004000** | APP 起始地址(Bootloader 占 16KB 之后) |
| **0x20000000** | SRAM 起始地址 |
| **PRIMASK** | 中断屏蔽位，`__disable_irq()` 就是设它，只屏蔽不清除 |
| **Default_Handler** | C 启动文件里的弱符号空函数，未注册的中断会落到这里死循环 |
| **fromelf** | Keil 工具，把 .axf 转 .bin: `fromelf --bin -o app.bin xxx.axf` |
| **SWD** | Serial Wire Debug，烧录器接口，救砖用 |
| **IWDG** | Independent Watchdog，独立看门狗，本项目未启用 |
| **双区 A/B** | 两个 APP 区交替升级的方案，可回滚 |
| **XMODEM** | 古老串口传输协议，逐块 128B + CRC16 + 停等 ACK |
| **UDS** | Unified Diagnostic Services，车规诊断协议栈 |

---

## 16. 面试文档 §1 的 15 题追问链（每题配更通俗解释）

面试文档列了 15 个追问，这里把每题用大白话再讲一遍：

1. **Bootloader 自己用什么栈?** 上电后硬件从 `0x08000000` 取 MSP 给 Bootloader 用；跳转时 `__set_MSP` 把栈顶切到 APP 向量表[0]，APP 用自己的栈。双方各用各的，不串。
2. **Bootloader 里用中断吗?** 用。DMA 传输完成中断把 `RxState` 置 READY——所以 `stm32f1xx_it.c` 的 USART1/DMA1_Ch4/Ch5 handler 必须留着。
3. **收 4 字节和收 128 字节区别?** 同一个 `uart_dma_block_recv`，参数 n 不同，DMA NDTR 装载值不同，逻辑零成本复用。
4. **对长度字段做什么检查?** `len==0` 或 `len > APP_MAX_SIZE-4` 直接弃帧；防止 0 长度和越界擦写。
5. **CRC 为什么整包而不是每块?** 整帧重传逻辑最简；每块 CRC+块号是 XMODEM 思路，代价是逐块停等，496KB 会慢十几秒。
6. **发 'K' 后立刻跳转,上位机来得及收吗?** `HAL_UART_Transmit` 是阻塞发送，字节移位寄存器发完才返回，所以 'K' 一定完整出去。
7. **`__disable_irq()` 后,APP 里谁负责开中断?** PRIMASK 只是掩码位；APP 的启动代码/CubeMX 流程会恢复。**关中断 ≠ 清 ICER**，两层是独立机制，跳转前两层都要处理。
8. **为什么不失败就回滚旧固件?** 单区方案没有"旧固件"可回滚(先擦后写)。双区 A/B + 状态标志才能回滚。
9. **Bootloader 怎么压到 8.4KB?** 只编入必需 HAL 模块(GPIO/DMA/UART/FLASH/FLASHEx)，不引 printf/动态内存；且不含 APP 的 FreeRTOS。
10. **上位机发 500KB 固件怎么办?** 长度检查直接拒帧，不会越界擦写——边界检查的价值。
11. **VTOR 每次中断都重新读吗?** 设置一次即可，硬件在每次异常取址时自动引用；APP 的 SystemInit 会再设一次，双保险。
12. **Bootloader 需要喂狗吗?** 本项目未启用 IWDG；若启用，全片擦除(数秒)和升级循环必须插喂狗，否则升级到一半被狗咬复位。
13. **写 Flash 时丢了几个字节怎么办?** NDTR≠0 检出异常，或整包 CRC 必错 → 回 'N' → 上位机整帧重传。安全性由"先擦后写+整包校验"兜底。
14. **量产时怎么烧 bootloader?** SWD/治具只烧一次 bootloader，之后全生命周期走串口 IAP——这是 IAP 方案的商业价值。
15. **上位机协议跟 WiFi 模组怎么衔接?** 传输层解耦：模组从云端拉固件后按同一帧格式透传字节流，bootloader 不感知对端是 PC 还是模组。

---

## 17. 学习路径建议（怎么从这份文档回到代码）

1. **第一遍：只看 main() 流程**——第 3 节。理解"上电 → 按键 → 跳 APP 或升级"这条主线。
2. **第二遍：跳转五步**——第 4 节。对照 [bootloader.c:202-242](file:///d:/Projects/JL%20project/range%20hood/bootloader%20YJ/BSP_BOOT/bootloader.c#L202) 逐行读，每步问"不做会怎样"。
3. **第三遍：升级协议**——第 5、6 节。对照 [boot_run_update](file:///d:/Projects/JL%20project/range%20hood/bootloader%20YJ/BSP_BOOT/bootloader.c#L248) 把状态机五步在脑子里走一遍。
4. **第四遍：Flash 约束**——第 8 节。理解"按字写 + 0xFF 填充 + 全片擦除"三个 F1 物理约束。
5. **第五遍：CRC 和上位机**——第 9、11 节。理解"为什么和 zlib 兼容"和流式计算的好处。
6. **最后：名词速查表**——第 15 节。回过头看面试文档第 1 章，每个名词都查得到。

---

## 18. 一句话总结

> **Bootloader 是芯片上电后跑的第一段程序，负责"决定跑哪段 APP"和"必要时把新 APP 写进 Flash"。它把 Flash 切成 16KB(自己用，永不擦) + 496KB(APP 用，可反复擦写)。升级时按 `[长度][数据][CRC]` 自定义协议收数据，DMA 分块搬，攒 4 字节写一次 Flash，最后整包 CRC32 校验，通过就清现场跳 APP，失败就等重发。跳转前必须停 SysTick + 清 NVIC + DeInit 外设 + 设 VTOR + 切 MSP——把 Bootloader 的痕迹全抹干净，给 APP 一个像刚上电一样的干净现场。**

这就是整个 Bootloader 的全部逻辑。剩下的全是工程细节和边界处理，逐节对照这份文档过一遍代码就懂了。
