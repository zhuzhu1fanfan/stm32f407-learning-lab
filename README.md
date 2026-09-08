# STM32F407 Learning Lab

野火霸天虎 V2（STM32F407ZGT6）的上板学习与工程记录。仓库不仅保存能运行的代码，也保存配置依据、复现步骤和实测证据，便于复盘和面试演示。

## 当前里程碑

### M1：板载 RGB 流水灯（已完成）

- PF6、PF7、PF8 分别控制板载三个 LED，低电平点亮；
- 红、绿、蓝按顺序循环，每个颜色默认保持 3000 ms；
- CMake/GCC 构建成功：Flash 5972 B，RAM 1584 B；
- fireDAP（CMSIS-DAP）通过 SWD 识别 Cortex-M4；
- OpenOCD 下载、校验和复位成功，实物观察到三个颜色交替。

实测记录见 [evidence/day01/RESULTS.md](evidence/day01/RESULTS.md)，当天学习记录见 [docs/DAY01.md](docs/DAY01.md)。

### M2：七色 RGB 流水灯与蜂鸣提示（已完成）

- 使用 3 位颜色掩码组合红、绿、蓝，依次显示红、黄、绿、青、蓝、紫、白；
- 使用颜色数组和统一控制函数，避免在主循环中重复编写 GPIO 操作；
- PG7 控制板载蜂鸣器，每轮七色播放完成后蜂鸣 200 ms；
- RGB 初始高电平保持熄灭，蜂鸣器初始低电平保持静音；
- CMake/GCC 注释版重新构建成功：Flash 6196 B，RAM 1584 B；
- 实物已确认七色循环和一轮结束蜂鸣提示正常。

代码中的宏、颜色表、循环、位运算、三目运算符和辅助函数均附有教学注释。过程见 [docs/DAY02.md](docs/DAY02.md)，结果见 [evidence/day02/RESULTS.md](evidence/day02/RESULTS.md)。

### M3-M8：按键、PWM、中断、串口与定时器（已完成）

- M3 `f407_key_toggle_rgb`：轮询 KEY1，按一次切换一次红灯状态；
- M4 `f407_dual_key_pwm_breathing`：KEY1/KEY2分别启停红、绿两路PWM呼吸灯；
- M5 `f407_key_exti_buzzer`：PA0外部中断产生按键事件，主循环非阻塞驱动蜂鸣器；
- M6 `f407_dual_key_exti_uart_counter`：KEY1计数、KEY2清零，通过USART1把事件实时发到电脑；
- M7 `f407_tim6_irq_uart_led_counter`：TIM6每2秒产生更新中断，计数、翻转红灯并输出日志；
- M8 `f407_tim2_etr_decimal_counter`：KEY1作为TIM2_ETR外部脉冲，CNT按十进制进位并短鸣提示。

这些工程均在野火霸天虎V2上完成实测；过程与故障修复见 [docs/DAY03.md](docs/DAY03.md)、[docs/DAY04.md](docs/DAY04.md)，验收记录见 [evidence/day03/RESULTS.md](evidence/day03/RESULTS.md) 和 [evidence/day04/RESULTS.md](evidence/day04/RESULTS.md)。

### M9-M16：ADC、DMA、通信、传感器与FreeRTOS（已完成基础实验）

- M9 光照阈值蜂鸣告警；
- M10 PWM输入捕获，串口显示频率、周期和占空比；
- M11 板载电位器单通道ADC与电压换算；
- M12 ADC扫描模式和DMA循环采集；
- M13 USART1中断收发；
- M14 串口包头/包尾接收状态机；
- M15 I2C读取板载MPU6050并输出加速度、角速度和姿态；
- M16 FreeRTOS三任务优先级反转与互斥锁优先级继承对照。

上述工程已重新执行独立 CMake Configure 和 Debug Build。实物观察结论仍以各次上板记录为准，仓库中的构建验证不替代硬件复测。

## 硬件与工具链

- 开发板：野火 STM32F407 霸天虎 V2
- MCU：STM32F407ZGT6，Cortex-M4，1 MiB Flash
- 下载器：野火 fireDAP，CMSIS-DAP/SWD
- 主频：168 MHz（外部 25 MHz HSE 经 PLL 倍频）
- 开发环境：macOS、STM32CubeMX、VS Code、CMake、GNU Arm Embedded Toolchain、OpenOCD
- 固件库：STM32CubeF4 HAL 1.28.3

## 仓库结构

```text
stm32f407-learning-lab/
├── firmware/
│   ├── stm32f407/                       # HAL/裸机外设练习
│   │   ├── f407_lab/                    # M1：三色基础流水灯
│   │   ├── f407_rgb_7color_buzzer/      # M2：七色组合与蜂鸣提示
│   │   ├── f407_key_toggle_rgb/         # M3：按键轮询翻转红灯
│   │   ├── f407_dual_key_pwm_breathing/ # M4：双按键双路PWM呼吸灯
│   │   ├── f407_key_exti_buzzer/        # M5：外部中断与非阻塞蜂鸣
│   │   ├── f407_dual_key_exti_uart_counter/ # M6：双按键中断与串口计数
│   │   ├── f407_tim6_irq_uart_led_counter/  # M7：内部定时中断
│   │   ├── f407_tim2_etr_decimal_counter/   # M8：外部脉冲计数与进位
│   │   ├── f407_light_buzzer_alarm/         # M9：光照阈值告警
│   │   ├── f407_pwm_input_capture_uart/     # M10：PWM输入捕获
│   │   ├── f407_adc_pot_uart/               # M11：单通道ADC
│   │   ├── f407_adc_multichannel_dma_uart/  # M12：多通道ADC与DMA
│   │   ├── f407_uart_tx_rx_it/              # M13：串口中断收发
│   │   ├── f407_uart_packet_protocol/       # M14：串口数据包协议
│   │   └── f407_i2c_mpu6050_uart/           # M15：I2C与MPU6050
│   └── freertos/                            # M16起：FreeRTOS练习
│       └── f407_freertos_priority_inversion_uart/
├── docs/
│   ├── DAY01.md          # 第一次上板的过程与结论
│   ├── DAY02.md          # 七色组合与蜂鸣器实验
│   ├── LEARNING_LOG.md   # 持续学习记录
│   ├── REPRODUCE.md      # 构建、接线与下载复现步骤
│   └── ROADMAP.md        # 后续项目路线及验收标准
└── evidence/             # 每个里程碑的构建与实物结果
```

## 快速复现

1. 用 VS Code 打开 `firmware/stm32f407/f407_lab`。
2. 选择 CMake 的 `Debug` 预设并执行 Build。
3. fireDAP 的 SWD 插头接到开发板 SWD 座，开发板另行供电。
4. 在 `firmware/stm32f407/f407_lab` 目录执行：

```bash
openocd -f interface/cmsis-dap.cfg \
  -c "transport select swd" \
  -f target/stm32f4x.cfg \
  -c "adapter speed 1000" \
  -c "program build/Debug/f407_lab.elf verify reset exit"
```

完整说明与排错方法见 [docs/REPRODUCE.md](docs/REPRODUCE.md)。

## 项目原则

- `.ioc`、源代码、链接脚本、CMake 配置和小体积证据进入版本控制；
- `build/`、ELF、BIN、HEX 等生成物不提交；
- 每个里程碑必须有可复现操作和真实验证结果；
- HAL 用于快速形成可靠基线，同时结合参考手册理解时钟、GPIO 和寄存器；
- 详细个人复习笔记与面试题保存在独立私有仓库，不放入本公开仓库。

下一阶段进入 FreeRTOS 任务、队列、互斥锁和周期调度练习，规划见 [firmware/freertos/README.md](firmware/freertos/README.md)。
