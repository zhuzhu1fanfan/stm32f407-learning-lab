# Day 01 实测结果

日期：2026-08-15

## 构建

```text
Linking C executable f407_lab.elf
RAM:     1584 B / 128 KB (1.21%)
CCMRAM:     0 B / 64 KB  (0.00%)
FLASH:   5972 B / 1 MB   (0.57%)
生成已完成，退出代码为 0
```

## 下载器与目标识别

```text
CMSIS-DAP FW Version = 2.0.0
CMSIS-DAP Interface Initialised (SWD)
CMSIS-DAP Interface ready
SWD DPIDR 0x2ba01477
Cortex-M4 r0p1 processor detected
target has 6 breakpoints, 4 watchpoints
```

## 烧录校验

```text
Programming Started
device id = 0x1011f413
flash size = 1024 KiB
Programming Finished
Verify Started
Verified OK
Resetting Target
```

## 实物结果

- 板载红、绿、蓝 LED 按顺序循环；
- 延时参数改为 3000 ms 后，每一步明显变慢；
- fireDAP 与开发板连接稳定，固件可重复构建并下载。

本次没有使用示波器或逻辑分析仪，因此没有声称测得 GPIO 边沿或精确周期；当前结论来自构建日志、OpenOCD 校验和肉眼观察。
