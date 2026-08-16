# 固件工程

本目录保存每个里程碑对应的独立 STM32CubeMX CMake/GCC 工程：

- `f407_lab/`：M1，红绿蓝基础流水灯；
- `f407_rgb_7color_buzzer/`：M2，七色组合流水灯与一轮结束蜂鸣提示。

- `*.ioc`：芯片、时钟和引脚的可再生成配置；
- `Core/`：应用入口、初始化代码和中断处理；
- `Drivers/`：CMSIS 与 STM32CubeF4 HAL；
- `CMakeLists.txt`、`CMakePresets.json`：VS Code/CMake 构建入口；
- `STM32F407XX_FLASH.ld`、`startup_stm32f407xx.s`：链接布局与启动代码。

编译生成的 `build/` 不进入 Git。重新用 CubeMX 生成代码时，自定义代码必须保留在 `USER CODE BEGIN/END` 区域内。
