# 固件工程

本目录保存每个里程碑对应的独立 STM32CubeMX CMake/GCC 工程：

- `f407_lab/`：M1，红绿蓝基础流水灯；
- `f407_rgb_7color_buzzer/`：M2，七色组合流水灯与一轮结束蜂鸣提示。
- `f407_key_toggle_rgb/`：M3，按键轮询翻转红灯；
- `f407_dual_key_pwm_breathing/`：M4，双按键控制红绿PWM呼吸；
- `f407_key_exti_buzzer/`：M5，按键外部中断与非阻塞蜂鸣器；
- `f407_dual_key_exti_uart_counter/`：M6，双按键中断、计数与USART1日志；
- `f407_tim6_irq_uart_led_counter/`：M7，TIM6内部定时中断计数；
- `f407_tim2_etr_decimal_counter/`：M8，TIM2 ETR外部计数与十进制进位。

- `*.ioc`：芯片、时钟和引脚的可再生成配置；
- `Core/`：应用入口、初始化代码和中断处理；
- `Drivers/`：CMSIS 与 STM32CubeF4 HAL；
- `CMakeLists.txt`、`CMakePresets.json`：VS Code/CMake 构建入口；
- `STM32F407XX_FLASH.ld`、`startup_stm32f407xx.s`：链接布局与启动代码。

编译生成的 `build/` 不进入 Git。重新用 CubeMX 生成代码时，自定义代码必须保留在 `USER CODE BEGIN/END` 区域内。
