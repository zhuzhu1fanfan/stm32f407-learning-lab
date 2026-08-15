# Day 01：板载 RGB 流水灯

日期：2026-08-15

状态：已完成并通过实物验证

## 目标

从空白 CubeMX 工程开始，独立完成时钟、GPIO、CMake、下载器和 OpenOCD 的完整链路，使板载 RGB LED 按固定顺序循环。

## 硬件结论

- 开发板：野火 STM32F407 霸天虎 V2
- MCU：STM32F407ZGT6
- 下载器：fireDAP（CMSIS-DAP），SWD 五线连接
- 板载 LED：PF6、PF7、PF8，低电平有效
- 本实验使用板载 LED，不需要外接面包板或限流电阻

## CubeMX 配置

### GPIO

| 信号 | 引脚 | 模式 | 上下拉 | 速度 | 初始电平 |
|---|---|---|---|---|---|
| LED1 | PF6 | Output Push-Pull | No pull | Low | High（熄灭） |
| LED2 | PF7 | Output Push-Pull | No pull | Low | High（熄灭） |
| LED3 | PF8 | Output Push-Pull | No pull | Low | High（熄灭） |

选择推挽输出，是因为 LED 需要 GPIO 主动输出高、低电平；板上已有完整驱动和限流连接，不需要开漏输出。选择无上下拉，是因为输出状态由推挽级明确驱动。LED 翻转频率很低，因此低速足够，还能减小边沿噪声和不必要的瞬态电流。初始高电平可避免低有效 LED 在初始化时误亮。

### 时钟

```text
HSE 25 MHz
  / PLLM 25 = 1 MHz
  × PLLN 336 = 336 MHz (VCO)
  / PLLP 2 = 168 MHz SYSCLK

AHB  /1 = 168 MHz
APB1 /4 = 42 MHz
APB2 /2 = 84 MHz
```

APB1 与 APB2 必须分频，因为 STM32F407 对它们的最高频率分别是 42 MHz 和 84 MHz。未启用 USB/SDIO/RNG 时，CubeMX 中 PLLQ 可能是灰色且显示 `/4`；它不影响本次 GPIO 实验。

## 关键代码

循环中依次把三个低有效 LED 拉低，等待后再拉高。使用 CubeMX 生成的 `LEDx_Pin` 和 `LEDx_GPIO_Port` 名称，避免在业务代码里重复写死物理端口和引脚。

```c
HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
HAL_Delay(LED_STEP_DELAY_MS);
HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
```

代码全部写在 `USER CODE BEGIN/END` 区域，重新生成 CubeMX 代码时不会被覆盖。

## 验证结果

- CMake/GCC 编译退出代码为 0；
- Flash 使用 5972 B，RAM 使用 1584 B；
- OpenOCD 识别 CMSIS-DAP、SWD DPIDR 和 Cortex-M4；
- 烧录提示 `Programming Finished`、`Verified OK`、`Resetting Target`；
- 开发板实物观察到红、绿、蓝依次循环；
- 将延时改大或改小后，切换速度按预期变化。

原始结果摘要见 [../evidence/day01/RESULTS.md](../evidence/day01/RESULTS.md)。

## 本次踩坑与修正

- 主频单位是 168 **MHz**，不是 168 Hz；
- PLLQ 变灰是因为当前没有启用需要 48 MHz 域的外设，不是配置损坏；
- 板载 LED 已接在 PCB 上，无需再连接面包板；
- 多行 OpenOCD 命令通过反斜杠续行，本质上仍是一条终端命令；
- `HAL_Delay` 只改变流水灯节拍，不需要重新配置系统时钟树；
- 用户逻辑应放在 `USER CODE` 区域内。

## 能力边界

本里程碑证明了 GPIO 输出、CubeMX 生成、CMake 编译和 SWD 下载链路可用。它还没有证明中断、消抖、非阻塞调度、串口、传感器或长期运行可靠性，这些放在后续里程碑验证。
