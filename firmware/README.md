# 固件工程

`f407_lab/` 是 STM32CubeMX 生成的 STM32F407ZGT6 CMake/GCC 工程。

- `f407_lab.ioc`：芯片、时钟和引脚的可再生成配置；
- `Core/`：应用入口、初始化代码和中断处理；
- `Drivers/`：CMSIS 与 STM32CubeF4 HAL；
- `CMakeLists.txt`、`CMakePresets.json`：VS Code/CMake 构建入口；
- `STM32F407XX_FLASH.ld`、`startup_stm32f407xx.s`：链接布局与启动代码。

编译生成的 `build/` 不进入 Git。重新用 CubeMX 生成代码时，自定义代码必须保留在 `USER CODE BEGIN/END` 区域内。
