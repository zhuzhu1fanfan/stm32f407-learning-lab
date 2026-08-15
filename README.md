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
│   ├── f407_lab/                     # M1：三色基础流水灯
│   └── f407_rgb_7color_buzzer/       # M2：七色组合与蜂鸣提示
├── docs/
│   ├── DAY01.md          # 第一次上板的过程与结论
│   ├── DAY02.md          # 七色组合与蜂鸣器实验
│   ├── LEARNING_LOG.md   # 持续学习记录
│   ├── REPRODUCE.md      # 构建、接线与下载复现步骤
│   └── ROADMAP.md        # 后续项目路线及验收标准
└── evidence/             # 每个里程碑的构建与实物结果
```

## 快速复现

1. 用 VS Code 打开 `firmware/f407_lab`。
2. 选择 CMake 的 `Debug` 预设并执行 Build。
3. fireDAP 的 SWD 插头接到开发板 SWD 座，开发板另行供电。
4. 在 `firmware/f407_lab` 目录执行：

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

下一里程碑是 M3：按键输入与软件消抖，让按键切换颜色或启停蜂鸣器。路线见 [docs/ROADMAP.md](docs/ROADMAP.md)。
