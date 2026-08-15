# M1 复现说明

## 1. 打开与构建

用 VS Code 打开 `firmware/f407_lab`，选择 CMake `Debug` 预设并执行 Build。成功时输出应包含 `Linking C executable f407_lab.elf` 和退出代码 0。

如果命令行环境已安装并配置 GNU Arm 工具链，也可以在工程目录执行：

```bash
cmake --preset Debug
cmake --build --preset Debug
```

不同 CubeMX/VS Code 扩展版本可能调用 `cube-cmake` 包装器，但生成目标相同。

## 2. 连接 fireDAP

将 fireDAP 的 SWD 接口与开发板标有 `3V3/TMS/GND/TCK/RST` 的插座对应连接。这里的 TMS 是 SWDIO，TCK 是 SWCLK；VREF/3V3 用于检测目标电平。开发板通过自身 USB 或电源接口供电，不依赖调试器给整板供电。

不要在带电时反复错位插拔。若无法识别芯片，先检查插头方向、共地、目标供电和 `3V3/VREF`。

## 3. 下载并校验

在 `firmware/f407_lab` 目录执行一条 OpenOCD 命令：

```bash
openocd -f interface/cmsis-dap.cfg \
  -c "transport select swd" \
  -f target/stm32f4x.cfg \
  -c "adapter speed 1000" \
  -c "program build/Debug/f407_lab.elf verify reset exit"
```

成功标志包括：识别 CMSIS-DAP、检测 Cortex-M4、`Programming Finished`、`Verified OK` 和 `Resetting Target`。

## 4. 观察与改速

复位后观察板载红、绿、蓝灯依次点亮。修改 `Core/Src/main.c` 中的 `LED_STEP_DELAY_MS`：数值越小切换越快，越大越慢；单位为毫秒。修改后必须重新构建和下载。

## 5. 常见问题

- 只构建未下载：开发板仍运行旧固件；
- 修改时钟而不是延时：流水节拍应改软件时间参数，不应为此破坏系统时钟基线；
- LED 逻辑相反：本板 LED 低电平有效，`RESET` 点亮、`SET` 熄灭；
- OpenOCD 能识别但立即退出：若只启动 GDB server 而没有 `program ...`，不会自动写入固件；
- CubeMX 再生成后代码消失：自定义代码没有放在 `USER CODE` 区域。
