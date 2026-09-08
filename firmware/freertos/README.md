# FreeRTOS 练习工程

本目录保存 STM32F407 的 FreeRTOS 独立工程。每个工程都应能单独用 VS Code 打开、配置、编译和烧录。

## 已有工程

- `f407_freertos_priority_inversion_uart/`：三辆汽车模拟低、中、高三个优先级任务，对比二值信号量和互斥锁的行为。

## 当前练习：电位器状态监控器

待创建工程名：`f407_freertos_adc_monitor`

计划使用三个应用任务：

1. `ADC_Task` 每 100 ms 读取 PB0/ADC1_IN8，并把采样结构体写入队列；
2. `Control_Task` 阻塞等待队列，根据电压范围控制板载 RGB 和蜂鸣器；
3. `Uart_Task` 每 2 s 输出最新测量值、等级、采样次数和队列丢包数。

这个项目用于练习任务、优先级、阻塞、周期调度、队列和串口共享。配置与代码在实际 CubeMX 工程生成后继续完成，避免用一个脱离 `.ioc` 的假工程代替上板过程。

## 工程移动规则

移动工程后不要复制旧的构建缓存：

1. 删除工程内的 `build/`；
2. 用 VS Code 的“打开文件夹”单独打开目标工程目录；
3. 运行“STM32: 修复CMake缓存”或重新 Configure；
4. 再执行 Build/烧录。

`CMakePresets.json` 中的 `${sourceDir}` 和 `.vscode/tasks.json` 中的 `${workspaceFolder}` 都是可迁移写法；`build/Debug/CMakeCache.txt`、`compile_commands.json` 和 Ninja 文件是不可迁移的生成物。
