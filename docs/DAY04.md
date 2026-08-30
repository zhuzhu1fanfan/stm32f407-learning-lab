# DAY04：TIM6内部定时与TIM2外部计数

## TIM6内部定时

APB1为42MHz，APB1分频不为1，因此TIM6定时器时钟为84MHz：

```text
T = (PSC+1) * (ARR+1) / 84MHz
  = 8400 * 20000 / 84MHz
  = 2s
```

每2秒产生一次更新中断。回调设置事件，主循环完成计数、LED翻转和USART1输出。

## TIM2外部计数

PA0连接板载KEY1，同时配置为TIM2_ETR。外部时钟模式2让有效边沿直接驱动CNT：

```text
PSC=0, ARR=9
第1至9次：CNT=1...9
第10次：CNT=0，更新中断使NUM加1
TOTAL=NUM*10+CNT
```

主循环检测CNT/NUM变化，每次有效计数短鸣并输出串口日志。

## 已解决问题

- CubeMX中的`No Division`对应HAL的`DIV1`；
- `Clock Prescaler = Prescaler not used`对应ETR预分频DIV1；
- PA0显示被TIM2_ETR占用是正确结果，不能再重复配置为GPIO或EXTI；
- 仅调用`HAL_TIM_Base_Start_IT()`不够，还必须在NVIC启用TIM2全局中断；
- 主循环漏贴检测代码时，硬件可能计数，但不会蜂鸣或输出更新日志。
