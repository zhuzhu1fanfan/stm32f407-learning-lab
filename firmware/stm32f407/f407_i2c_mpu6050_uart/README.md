# STM32F407 I2C1 + 板载 MPU6050 + 串口姿态显示

## 本工程完成什么

- 使用 I2C1 读取野火霸天虎 V2 底板上的 MPU6050。
- 自动尝试 7 位地址 `0x68` 和 `0x69`，并读取 `WHO_AM_I` 自检。
- 串口输出三轴加速度、三轴角速度、温度。
- 使用互补滤波计算 Roll、Pitch；Yaw 是陀螺积分得到的相对角度，会随时间漂移。
- 上电后自动进行约 1 秒陀螺零偏校准。

## CubeMX 配置

- MCU：STM32F407ZGT6
- HSE：25 MHz
- SYSCLK：168 MHz
- I2C1：PB8 = SCL，PB9 = SDA，100 kHz，7-bit address
- USART1：PA9 = TX，PA10 = RX，115200，8-N-1
- Debug：Serial Wire

## 使用

1. 在 VS Code 中打开本工程文件夹。
2. 保持 CoolTerm 暂时断开。
3. 执行任务 `STM32: 编译、烧录并运行`。
4. CoolTerm 连接板载 CH340 对应串口，设置为 `115200 8-N-1`。
5. 复位后保持开发板静止约 1 秒，看到 `Calibration complete` 后再移动板子。

正常启动应看到：

```text
MPU6050 found: address=0x68, WHO_AM_I=0x68.
Keep the board still: calibrating gyro...
Calibration complete. Move or tilt the board.
```

姿态滤波在后台每 10 ms 更新，随后每 2 秒向串口输出一组数据：

```text
ACC[mg] X=... Y=... Z=...
GYRO[dps] X=... Y=... Z=...
ANGLE[deg] Roll=... Pitch=... Yaw=...(relative/drifts)
TEMP=... C
```

如果一直显示 `MPU6050 not found`，说明当前连接的可能只是核心板、底板上的 MPU6050/I2C 跳线未接通，或 PB8/PB9 总线被其他外设占用。

## 角度说明

- Roll/Pitch：加速度计与陀螺仪互补滤波，倾斜板子时会变化。
- Yaw：MPU6050 没有磁力计，只能输出相对转角，静止时也可能缓慢漂移。
- 若要更稳定的完整姿态，可在后续工程中加入 MPU6050 DMP，或使用带磁力计的九轴 IMU。
