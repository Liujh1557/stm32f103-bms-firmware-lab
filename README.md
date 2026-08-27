# STM32F103 定时调度基础工程

这是 BMS 学习路线的第一个固件节点，用于验证 STM32F103C8T6 的 GPIO、USART1 和 TIM2 中断调度链路。

## 当前功能

- PC13 配置为 LED 推挽输出，默认高电平；
- USART1 使用 PA9/PA10，配置为 115200、8-N-1；
- TIM2 使用内部时钟，每 1 ms 产生一次更新中断；
- 中断回调只递增 `g_system_ms`；
- 主循环派生 10 ms、100 ms 和 1000 ms 三档非阻塞周期任务；
- 10 ms、100 ms 任务分别统计执行次数；
- 1000 ms 任务翻转 LED，并发送系统时间与任务计数日志；
- 新增 `Core/Inc/bms_types.h`，定义 `BmsData`、`BmsConfig`、`BmsFault` 和故障码；
- BMS 数据采用固定点整数单位：电压 mV、电流 mA、温度 0.01 °C；
- ADC1 扫描 PA0～PA6 共 7 个 Rank，DMA1 Channel1 自动按 Rank 顺序写入 7 项数组；
- 前 4 路按 12 位 ADC、3.3 V 参考电压换算为 mV；电流和温度暂不猜测传感器模型，`calibrated=0`；
- 10 ms 任务启动一帧 DMA，DMA 完成中断只设置标志，下一次任务再处理完整采样数组；
- 1000 ms 日志输出 DMA 状态、完成帧数、错误次数、第 1 路换算电压、7 路原始值和故障标志；
- 主循环不使用 `HAL_Delay()` 进行任务调度。

## TIM2 参数

当前系统及 TIM2 时钟为 8 MHz：

```text
Prescaler = 799
Period    = 9

8,000,000 / ((799 + 1) * (9 + 1)) = 1,000 Hz
```

因此 TIM2 更新中断周期为 1 ms。

## ADC 通道映射

```text
Rank 1 / PA0 / ADC1_IN0 -> cell_voltage_mv[0]
Rank 2 / PA1 / ADC1_IN1 -> cell_voltage_mv[1]
Rank 3 / PA2 / ADC1_IN2 -> cell_voltage_mv[2]
Rank 4 / PA3 / ADC1_IN3 -> cell_voltage_mv[3]
Rank 5 / PA4 / ADC1_IN4 -> pack_current_ma（待传感器换算）
Rank 6 / PA5 / ADC1_IN5 -> temperature_cdeg[0]（待传感器换算）
Rank 7 / PA6 / ADC1_IN6 -> temperature_cdeg[1]（待传感器换算）
```

当前 ADC 输入仅作为 0～3.3 V 安全模拟信号演示。`valid=1` 表示 DMA 已完成一帧 7 路转换，`sensor_cal=0` 表示电流和温度的实际传感器传递函数尚未配置。

## ADC 轮询问题与修复

STM32F1 在多 Rank 扫描模式下，EOC 在扫描序列结束时才有效，不能用循环调用 `HAL_ADC_PollForConversion()` 的方式逐项取得 Rank 数据。该写法会重复读取同一个数据寄存器值，实测表现为 7 路原始值完全相同。当前版本已改用 DMA1 Channel1，使每个 Rank 自动写入独立数组元素。

## 构建产物

工程使用 STM32CubeMX 生成的 Makefile。编译产物位于 `build/`，该目录不提交到 Git。

## VS Code 编译与烧录

工程已添加 `.vscode/tasks.json` 和 `.vscode/launch.json`：

- `Ctrl+Shift+B`：调用 `mingw32-make` 编译；
- `Ctrl+Shift+P` → `Tasks: Run Task` → `Flash STM32 (HEX)`：先编译，再通过 ST-Link/SWD 烧录并复位；
- `F5`：使用 Cortex-Debug 和 ST-LINK GDB Server 编译、下载并进入 `main` 断点。

烧录前确认 ST-Link 已连接、目标板已供电，并且 ST-Link 与目标板共地。

## 已验证现象

- 工程可以成功编译并生成 ELF、HEX 和 BIN；
- 固件可以通过 ST-Link 下载并运行；
- LED 每秒翻转一次；
- USART1 每秒输出一次运行日志。

## 下一步

下一步烧录并验证 PA1 接 3.3 V 时，第二个原始值接近 4095，且其他通道不再被重复填入同一个值。
