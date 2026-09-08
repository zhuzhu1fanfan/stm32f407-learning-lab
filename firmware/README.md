# 固件工程

本目录按开发方式分为两个类别。每个子目录都是一个可独立打开、配置、编译和烧录的 STM32CubeMX CMake/GCC 工程。

## `stm32f407/`：HAL 与裸机外设练习

- `f407_lab/`：M1，红绿蓝基础流水灯；
- `f407_rgb_7color_buzzer/`：M2，七色组合流水灯与一轮结束蜂鸣提示；
- `f407_key_toggle_rgb/`：M3，按键轮询翻转红灯；
- `f407_dual_key_pwm_breathing/`：M4，双按键控制红绿 PWM 呼吸；
- `f407_key_exti_buzzer/`：M5，按键外部中断与非阻塞蜂鸣器；
- `f407_dual_key_exti_uart_counter/`：M6，双按键中断、计数与 USART1 日志；
- `f407_tim6_irq_uart_led_counter/`：M7，TIM6 内部定时中断计数；
- `f407_tim2_etr_decimal_counter/`：M8，TIM2 ETR 外部计数与十进制进位；
- `f407_light_buzzer_alarm/`：M9，ADC 光照阈值检测与蜂鸣告警；
- `f407_pwm_input_capture_uart/`：M10，PWM 输入捕获并通过串口显示频率和占空比；
- `f407_adc_pot_uart/`：M11，板载电位器单通道 ADC 采样与电压换算；
- `f407_adc_multichannel_dma_uart/`：M12，ADC 扫描模式、DMA 循环采集与串口显示；
- `f407_uart_tx_rx_it/`：M13，USART1 中断接收和收发模式练习；
- `f407_uart_packet_protocol/`：M14，带包头和包尾的串口数据包接收状态机；
- `f407_i2c_mpu6050_uart/`：M15，I2C 读取板载 MPU6050 并计算姿态。

## `freertos/`：FreeRTOS 练习

- `f407_freertos_priority_inversion_uart/`：M16，三任务优先级反转与互斥锁优先级继承对照实验；
- 后续 FreeRTOS 独立练习也统一放入此目录。

## 工程内容与移动规则

- `*.ioc`：芯片、时钟和引脚的可再生成配置；
- `Core/`：应用入口、初始化代码和中断处理；
- `Drivers/`：CMSIS 与 STM32CubeF4 HAL；
- `CMakeLists.txt`、`CMakePresets.json`：VS Code/CMake 构建入口；
- `STM32F407XX_FLASH.ld`、`startup_stm32f407xx.s`：链接布局与启动代码。

编译生成的 `build/` 不进入 Git。重新用 CubeMX 生成代码时，自定义代码必须保留在 `USER CODE BEGIN/END` 区域内。

移动任一工程后，旧 `build/` 中的 CMake 缓存仍记录原来的绝对路径，不能继续复用。应删除该工程的 `build/`，用 VS Code 单独打开具体工程目录，再执行“STM32: 修复CMake缓存”或重新 Configure，然后 Build。工程内的任务使用 `${workspaceFolder}` 作为当前目录，不能打开更高一级目录后直接套用。
