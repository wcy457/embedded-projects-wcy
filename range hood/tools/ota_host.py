#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ota_host.py - YJ 油烟机 bootloader 上位机

协议帧: [4B 小端 payload 长度][payload 固件数据][4B 小端 CRC32(zlib)]
握手:   发送长度帧后等待 bootloader 回 'R'(全片擦除完成),
        收到 'R' 后分块发送数据 + CRC 尾,最后等待 'K'(成功)/'N'(失败)。

用法:
    python ota_host.py gen <输出.bin> <大小字节>
        生成测试固件(非法向量表,用于验证传输/校验/ACK 链路,
        bootloader 校验通过回 'K' 后会因 APP 非法而留在升级模式)

    python ota_host.py flash -p COM5 -f 固件.bin [-b 115200] [--chunk 128]
                             [--inter-delay 6] [--retries 3]
        执行升级。真正升级 APP 请用 Keil fromelf 生成的 bin
        (fromelf --bin -o app.bin "RTOS  YJ.axf",bin 基址 0x08004000)
"""

import argparse
import struct
import sys
import time
import zlib

try:
    import serial  # pip install pyserial
except ImportError:
    print("缺少 pyserial, 请先执行: pip install pyserial")
    sys.exit(1)

ACK = b"K"      # 校验通过, bootloader 将跳转 APP
NAK = b"N"      # 失败, 请求重发整帧
READY = b"R"    # 擦除完成, 可以开始发数据


def cmd_gen(args):
    size = args.size
    if size < 16 or size > 0x7C000:
        print("大小需在 16 ~ 507904 (0x7C000) 字节之间")
        sys.exit(1)
    # 伪随机递增图案, 便于肉眼核对
    data = bytes((i * 7 + 0x5A) & 0xFF for i in range(size))
    with open(args.out, "wb") as f:
        f.write(data)
    print("已生成 %s (%d 字节, CRC32=0x%08X)" % (args.out, size, zlib.crc32(data) & 0xFFFFFFFF))


def wait_for(ser, token, timeout, what):
    """等待指定单字节, 返回 True/False"""
    deadline = time.time() + timeout
    while time.time() < deadline:
        b = ser.read(1)
        if b == token:
            return True
        if b:
            # 收到其它字节 (可能是上一次尝试的残留), 忽略并继续等
            continue
    print("等待 %s 超时" % what)
    return False


def send_frame(ser, data, chunk, inter_delay_ms, tail_delay_ms):
    ser.write(struct.pack("<I", len(data)))
    # 等待 bootloader 全片擦除完成 (F1 248 页, 数秒级)
    if not wait_for(ser, READY, 20.0, "Ready('R', 擦除完成)"):
        return False
    print("擦除完成, 开始发送固件 (%d 字节, %d B/块)..." % (len(data), chunk))
    t0 = time.time()
    for off in range(0, len(data), chunk):
        ser.write(data[off:off + chunk])
        done = min(off + chunk, len(data))
        pct = done * 100 // len(data)
        print("\r进度: %d/%d 字节 (%d%%)" % (done, len(data), pct), end="")
        if inter_delay_ms:
            time.sleep(inter_delay_ms / 1000.0)
    # bootloader 写最后一块 + flush 需数毫秒, 先延时再发 CRC 尾防止丢失
    if tail_delay_ms:
        time.sleep(tail_delay_ms / 1000.0)
    ser.write(struct.pack("<I", zlib.crc32(data) & 0xFFFFFFFF))
    print("\n发送完成, 耗时 %.1fs, 等待校验结果..." % (time.time() - t0))
    return True


def cmd_flash(args):
    with open(args.file, "rb") as f:
        data = f.read()
    if len(data) == 0 or len(data) > 0x7C000 - 4:
        print("固件大小 (%d 字节) 超出 APP 区范围" % len(data))
        sys.exit(1)
    crc = zlib.crc32(data) & 0xFFFFFFFF
    print("固件: %s, %d 字节, CRC32=0x%08X" % (args.file, len(data), crc))

    ser = serial.Serial(args.port, args.baud, timeout=0.1)
    print("串口 %s @ %d 已打开" % (args.port, args.baud))
    print("请按住 BOOT 键(PB0)并复位设备进入升级模式... 3 秒后开始")
    time.sleep(3)
    ser.reset_input_buffer()

    ok = False
    for attempt in range(1, args.retries + 1):
        print("=== 第 %d/%d 次尝试 ===" % (attempt, args.retries))
        ser.reset_input_buffer()
        if not send_frame(ser, data, args.chunk, args.inter_delay, args.tail_delay):
            continue  # 等擦除完成超时, 重来
        # 等 ACK/NAK
        deadline = time.time() + 10.0
        resp = None
        while time.time() < deadline:
            b = ser.read(1)
            if b in (ACK, NAK):
                resp = b
                break
            if b == READY:
                continue  # 残留, 忽略
        if resp == ACK:
            print("\n结果: 校验通过(ACK), 设备正在跳转 APP")
            ok = True
            break
        elif resp == NAK:
            print("\n结果: 校验失败(NAK), 准备重发...")
        else:
            print("\n结果: 等待校验结果超时, 准备重发...")

    ser.close()
    if ok:
        print("升级成功!")
        sys.exit(0)
    print("升级失败, 已达最大重试次数")
    sys.exit(2)


def main():
    ap = argparse.ArgumentParser(description="YJ 油烟机串口 IAP 上位机")
    sub = ap.add_subparsers(dest="cmd", required=True)

    g = sub.add_parser("gen", help="生成测试固件")
    g.add_argument("out", help="输出 bin 路径")
    g.add_argument("size", type=int, help="大小(字节)")
    g.set_defaults(func=cmd_gen)

    fl = sub.add_parser("flash", help="升级固件")
    fl.add_argument("-p", "--port", required=True, help="串口号, 如 COM5")
    fl.add_argument("-f", "--file", required=True, help="固件 bin 路径")
    fl.add_argument("-b", "--baud", type=int, default=115200)
    fl.add_argument("--chunk", type=int, default=128, help="分块大小, 默认128")
    fl.add_argument("--inter-delay", type=int, default=10, help="块间隔(ms), 默认10, 防止写Flash丢字节")
    fl.add_argument("--tail-delay", type=int, default=20, help="CRC尾前延时(ms), 默认20")
    fl.add_argument("--retries", type=int, default=3, help="整帧重试次数, 默认3")
    fl.set_defaults(func=cmd_flash)

    args = ap.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
