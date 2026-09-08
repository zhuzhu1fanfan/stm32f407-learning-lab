# STM32F407 FreeRTOS 三辆汽车与优先级反转

## 实验目标

用三辆汽车表示三个抢占式任务：

| 汽车 | FreeRTOS 优先级 | 行为 |
|---|---:|---|
| Car1 | 1（低） | 先占用单车道桥梁，即共享资源 |
| Car2 | 2（中） | 执行持续占用 CPU 的任务 |
| Car3 | 3（高） | 稍后到达，需要等待 Car1 占用的桥梁 |

程序自动循环运行两种对照：

1. **DEMO A：二值信号量**  
   不提供优先级继承。Car3 等待 Car1；Car2 又抢占 Car1，导致高优先级 Car3 被间接阻塞，这就是优先级反转。
2. **DEMO B：互斥锁**  
   Car3 等待互斥锁后，Car1 临时继承 Car3 的优先级 3，先完成并释放桥梁，Car2 不能在此时抢占 Car1。

## 使用方法

1. 在 VS Code 打开本工程文件夹。
2. 执行任务 `STM32: 编译、烧录并运行`。
3. CoolTerm 连接板载 CH340，设置 `115200 8-N-1`。
4. 复位开发板，程序会自动交替运行 DEMO A 和 DEMO B。

典型结果：

```text
DEMO A: binary semaphore - NO priority inheritance
[   0 ms] Car1 LOW entered the one-lane bridge, priority=1.
[ 500 ms] Car3 HIGH arrives and waits for the bridge.
[ 800 ms] Car2 MEDIUM starts CPU-heavy driving.
[3800 ms] Car2 MEDIUM finishes.
[3800 ms] Car1 finishing; effective priority=1.
[3800 ms] Car3 HIGH enters; waited about 3300 ms.
RESULT A: priority inversion.

DEMO B: mutex - priority inheritance ENABLED
[   0 ms] Car1 LOW entered the one-lane bridge, priority=1.
[ 500 ms] Car3 HIGH arrives and waits for the bridge.
[2000 ms] Car1 finishing; effective priority=3.
[2000 ms] Car3 HIGH enters; waited about 1500 ms.
RESULT B: priority inheritance protects the high-priority task.
```

具体毫秒数会受到串口输出和任务切换影响，重点比较：

- DEMO A 中 Car1 的有效优先级仍为 1，Car3 等待更久。
- DEMO B 中 Car1 的有效优先级升为 3，Car3 等待明显缩短。

## 本工程涉及的 FreeRTOS 知识

- `xTaskCreate()`：创建任务。
- 抢占式优先级调度。
- `vTaskDelay()`：阻塞任务。
- 任务通知：同步三辆汽车的启动和完成。
- 队列：所有任务把日志发给独立 Logger 任务，避免串口输出互相穿插。
- 二值信号量：没有优先级继承。
- 互斥锁：具有优先级继承。
- 堆栈溢出和内存分配失败钩子。

FreeRTOS 源码由 STM32CubeF4 V1.28.3 本地软件包提供，并使用 Cortex-M4F GCC 移植层。
