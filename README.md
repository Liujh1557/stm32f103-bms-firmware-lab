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
- 采样处理增加 4 点滑动平均，`BmsData` 同时保留最新原始值和滤波值；
- USART1 使用 DMA1 Channel4 非阻塞发送日志，发送缓冲区为静态数组，并统计发送成功和丢弃次数；
- 为完成 Normal 模式 DMA 的最后一个字节发送，启用 USART1 TC 中断并在 `USART1_IRQHandler()` 中调用 `HAL_UART_IRQHandler()`；
- USART1 RX 使用 DMA1 Channel5 和 `HAL_UARTEx_ReceiveToIdle_DMA()` 接收不定长数据；
- RX 空闲事件回调只复制一帧、记录长度并重启 DMA，命令解析放在 10 ms 主任务中；
- 第一条测试命令为 `PING`，识别成功后通过 TX DMA 返回 `ACK PING`；
- 增加独立 `bms_protection` 模块，100 ms 周期执行电压保护状态机；
- 第一版只监控 PA0 对应的第 1 路，使用 0～3.3 V 安全模拟输入和教学阈值；
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
- DMA 修复版本已完成硬件测试：PA1 通过限流电阻接板载 3.3 V 时，`raw[1]` 连续保持在 4092～4095；
- 15 秒测试期间 `frames` 从 99 增长到 1499，保持每秒约 100 帧，`adc_err=0`，说明 10 ms 周期采样和 DMA 完成中断持续运行；
- PA0 接 GND 后，`raw[0]` 稳定为 0、`cell0=0 mV`，且 56～58 秒测试中 `frames` 从 5599 增长到 5799，`adc_err=0`；
- 其余未接线的 ADC 通道出现漂移，属于高阻悬空输入，不能作为有效测量数据。
- UART TX DMA 中断修复版已经完成硬件复测：日志持续输出，`tx` 从 5、6 增长到 13，`tx_drop=0`；关闭再打开 COM5 时 MCU 未复位，采样帧数继续增长。

## UART TX DMA 中断修复

第一次使用 USART1 TX DMA 时，硬件只收到第一条日志。原因是 DMA1 Channel4 只报告“最后一个字节已搬到 USART1”，HAL 随后还要等待 USART1 的 TC（Transmission Complete）中断来释放 UART 状态并调用 `HAL_UART_TxCpltCallback()`；当时工程没有 `USART1_IRQHandler()`，也没有在 `.ioc` 中启用 `USART1_IRQn`，所以 `g_uart_tx_busy` 一直为 1，后续日志被丢弃。

当前已补齐：

```text
led-test.ioc          -> NVIC.USART1_IRQn=true
usart.c               -> 启用 USART1_IRQn
stm32f1xx_it.c        -> USART1_IRQHandler()
                         HAL_UART_IRQHandler(&huart1)
```

修复版本已经命令行编译并烧录验证。串口日志持续输出，`tx` 正常递增且 `tx_drop=0`，说明 DMA 完成中断、USART TC 中断和发送完成回调链路均已闭环。

## UART RX DMA

USART1 RX DMA 使用 64 字节静态缓冲区和 Normal 模式。接收链路为：

```text
PA10 / USART1_RX
        -> DMA1 Channel5
        -> 64 字节 DMA 缓冲区
        -> USART1 IDLE 或 DMA TC 事件
        -> HAL_UARTEx_RxEventCallback()
        -> 复制到稳定帧缓冲区并立即重启 RX DMA
        -> 10 ms 主任务解析 PING
        -> USART1 TX DMA 返回 ACK PING
```

半传输中断被关闭，因为 32 字节的 HT 事件并不代表一帧结束。若上一帧尚未处理，新帧会被丢弃并计入 `rx_drop`；UART/DMA 错误计入 `uart_err`。该版本命令行编译通过，`text/data/bss` 为 `15952/92/2940`。

板端实测以 UTF-8/ASCII 发送 `PING\n` 后收到 `ACK PING`，下一条周期日志中 `rx` 从 0 增长为 1，`rx_drop=0`、`uart_err=0`。同一秒内 `tx` 从 337 增长为 339，其中一次发送为应答、一次为周期日志，符合设计。

此次测试期间 `frames` 从 33799 增长为 33899，10 ms ADC DMA 采样未受串口双向通信影响。PA0 保持接 GND，日志同时出现 `fault=0x00000002`、`code=2`、`latch=1`、`pstate=2`，因此单路欠压确认路径也已完成硬件验证；过压和恢复路径仍未完成板端验证。

## 电压保护状态机（待硬件验证）

第一版保护逻辑在 100 ms 任务中运行，只监控 PA0 对应的 `cell_voltage_mv[0]`：

```text
NORMAL -> CONFIRMING -> FAULT_ACTIVE -> RECOVERING -> NORMAL
```

- 模拟过压阈值：3000 mV；
- 模拟欠压阈值：300 mV；
- 回差：100 mV；
- 故障确认时间：300 ms；
- 恢复确认时间：500 ms。

这些是适配开发板 0～3.3 V 输入的教学参数，不是真实锂电池保护阈值。故障恢复后 `flags` 和 `active_code` 清除，`latched` 保留为 1，表示本次上电期间曾发生过故障。
当采样数据 `valid=0` 时，保护状态保持不变，不把无效数据误判为故障恢复。`Tests/test_bms_protection.c` 提供主机端状态机测试，覆盖毛刺抑制、欠压/过压确认、回差、恢复、锁存和无效数据保持。

## 下一步

下一步为 UART 命令增加明确的帧格式和 CRC 校验；完成后进入 SPI2 回环实验。过压与恢复路径保留为后续保护模块的硬件测试项。
