# Day 02 实测结果

日期：2026-08-15

## 构建结果

教学注释版工程使用 CMake/GNU Arm 工具链重新构建成功：

```text
Building C object .../Core/Src/main.c.obj
Linking C executable f407_rgb_7color_buzzer.elf
RAM:     1584 B / 128 KB (1.21%)
CCMRAM:     0 B / 64 KB  (0.00%)
FLASH:   6196 B / 1 MB   (0.59%)
exit = 0
```

## 实物结果

- 红、黄、绿、青、蓝、紫、白依次显示；
- 七种颜色结束后 RGB 熄灭；
- 板载蜂鸣器随后短响一次；
- 等待轮次间隔后重新开始；
- 用户已确认实际现象符合预期。

本次验证证明 GPIO 组合逻辑和蜂鸣器开关控制可用，但尚未测量精确延时误差，也未验证非阻塞调度能力。
