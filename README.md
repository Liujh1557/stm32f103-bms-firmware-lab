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
- USART1 命令采用带帧头、长度和 CRC16-Modbus 的二进制帧，`CMD=0x01` 为 PING；
- SPI2阻塞回环已完成历史验证；当前使用重映射SPI1和DMA1 Channel2/3，避免与USART1 DMA Channel4/5冲突；
- 增加独立 `bms_protection` 模块，100 ms 周期执行电压保护状态机；
- 第一版只监控 PA0 对应的第 1 路，使用 0～3.3 V 安全模拟输入和教学阈值；
- 1000 ms 日志输出 DMA 状态、完成帧数、错误计数、第 1 路原始值/滤波值/换算电压和故障状态；
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
        -> 10 ms 主任务解析协议帧
        -> USART1 TX DMA 返回解析结果
```

半传输中断被关闭，因为 32 字节的 HT 事件并不代表一帧结束。若上一帧尚未处理，新帧会被丢弃并计入 `rx_drop`；UART/DMA 错误计入 `uart_err`。该版本命令行编译通过，`text/data/bss` 为 `15952/92/2940`。

基础 RX 链路版本曾在板端以 UTF-8/ASCII 发送 `PING\n`，成功收到 `ACK PING`；下一条周期日志中 `rx` 从 0 增长为 1，`rx_drop=0`、`uart_err=0`。同一秒内 `tx` 从 337 增长为 339，其中一次发送为应答、一次为周期日志，符合设计。

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

## UART 二进制帧与 CRC

UART 命令现采用二进制帧 `AA 55 | LEN | CMD | PAYLOAD | CRC_LO CRC_H`。`LEN` 表示 `CMD + PAYLOAD` 的字节数，CRC16-Modbus 覆盖 `LEN + CMD + PAYLOAD`。PING 命令的完整测试帧为 `AA 55 01 01 C1 E0`，需要在串口助手中使用十六进制发送且不附加换行；有效帧返回 `ACK PING CRC=OK`，错误 CRC 返回 `ERR CRC`。

为避免新增协议计数后超过 256 字节 TX 缓冲区，周期日志只保留当前有效测试通道的 `raw0`、`avg0` 和 `cell0`，不再每秒打印其余悬空 ADC 通道。日志新增 `crc_err` 和 `proto_err`；详细多通道数据后续通过查询命令按需返回。

纯 C 解析器位于 `Core/Src/uart_protocol.c`，主机端测试覆盖正确 PING、错误帧头、错误长度和错误 CRC。测试已经通过，固件交叉编译也已通过，`text/data/bss` 为 `16068/92/2948`。

板端测试结果：

- `AA 55 01 01 C1 E0` 返回 `ACK PING CRC=OK`；
- 修改 CRC 的 `AA 55 01 01 C0 E0` 返回 `ERR CRC`；
- 修改帧头的 `AB 55 01 01 C1 E0` 返回 `ERR FRAME code=1`；
- 测试日志显示 `rx=5`、`rx_drop=0`、`uart_err=0`、`crc_err=1`、`proto_err=3`。多出的协议错误来自测试期间发送过的旧文本或不完整帧，不代表 DMA 丢帧；
- `tx_drop=1` 表示协议应答占用单一 TX 缓冲区时跳过了一条低优先级周期日志，协议应答本身没有丢失。

## SPI2阻塞式回环

SPI2 第一版使用阻塞式 `HAL_SPI_TransmitReceive()`，每秒发送 `12 34 A5 5A` 并接收4字节。周期日志新增：

- `spi_ok`：最近一次发送值与接收值是否完全相同；
- `spi_n`：测试次数；
- `spi_mis`：HAL收发成功但内容不一致的次数；
- `spi_err`：HAL收发失败或超时次数；
- `spi_rx`：最近收到的4字节十六进制数据。

板端回环已经验证：回环线接好后，`spi_n` 从77增长到199，新增122次测试期间 `spi_mis` 始终保持67、没有继续增加；`spi_ok=1`、`spi_err=0`，且每次最近接收值均为 `spi_rx=1234A55A`。历史67次不匹配发生在回环线尚未正确接入的阶段，说明HAL完成一次SPI传输不等于对端数据正确，仍需比较发送与接收内容。

SPI回环运行期间，ADC `frames` 每秒仍增加约100，未观察到采样节拍被4字节阻塞传输破坏。关闭COM5只停止电脑端日志显示，不会停止MCU内部SPI、ADC和调度任务。

## SPI1 DMA回环

SPI2的RX/TX固定占用DMA1 Channel4/5，与USART1 TX/RX DMA冲突，因此DMA版本切换为重映射SPI1：

```text
PB3 / SPI1_SCK
PB4 / SPI1_MISO
PB5 / SPI1_MOSI

SPI1_RX -> DMA1 Channel2
SPI1_TX -> DMA1 Channel3
```

PB5/MOSI与PB4/MISO直连。程序启动时发起第一帧，之后每秒调用 `HAL_SPI_TransmitReceive_DMA()`。启动函数立即返回；`HAL_SPI_TxRxCpltCallback()`只清除忙标志并设置完成标志；10 ms主任务再比较收发缓冲区。日志中的 `spi_n`表示已经完成并由主任务处理的DMA帧数，`spi_busy`表示打印瞬间DMA是否仍在传输。

首次板端日志出现 `spi_busy=1`、`spi_ok=1`、`spi_n=1`，但 `spi_rx=00000000`。原因不是回环失败，而是新一帧启动时DMA活动缓冲区被清零，日志同时混用了“上一帧状态”和“下一帧活动缓冲区”。当前已增加 `g_spi_last_rx_buffer`：主任务仅在DMA完成后复制稳定快照，比较和日志均读取该快照，DMA缓冲区不再被日志直接访问。

快照修复版完成板端长时间验证：`t=758000 ms` 时 `spi_n=758`、`spi_busy=1`、`spi_ok=1`、`spi_mis=0`、`spi_err=0`、`spi_rx=1234A55A`。这表示约12分38秒内已完成758帧DMA回环，未出现数据不一致或DMA错误；打印瞬间下一帧DMA正在进行，而日志仍能读取上一帧稳定快照。

同一日志中ADC `frames=75799`、`adc_err=0`，约保持100帧/秒，说明SPI1 DMA、ADC DMA以及USART1 TX/RX DMA使用不同通道后可以并行持续运行。

SPI DMA总线状态和HAL回调已从 `main.c` 抽离到 `spi_if.c/.h`。接口层负责DMA启动、忙状态、完成/错误事件和HAL回调；应用层继续拥有发送、接收及稳定快照缓冲区，并在10 ms任务中处理结果。调用方必须保证TX/RX缓冲区在DMA完成前持续有效。

接口层重构版重新烧录后，通过COM5直接读取连续5条日志：`t=20000～24000 ms` 期间 `spi_n` 从20增长到24，`spi_busy=1`、`spi_ok=1`、`spi_mis=0`、`spi_err=0`、`spi_rx=1234A55A`；ADC `frames` 从1999增长到2399、`adc_err=0`。这确认重构没有改变SPI DMA及其他周期任务的板端行为。

## bxCAN内部回环

CAN1使用PA11/RX、PA12/TX，APB1为8 MHz，位时序为Prescaler=1、BS1=13 TQ、BS2=2 TQ、SJW=1 TQ，对应500 kbit/s和87.5%采样点。第一版使用内部Loopback，不依赖外部收发器或USB-CAN。

`can_if.c/.h`负责全接收过滤器、CAN启动、FIFO0通知、标准帧发送及RX0回调取帧；100 ms任务发送标准ID `0x321`、DLC 8、数据 `12 34 A5 5A 01 02 03 04`，10 ms任务比较接收结果。日志新增 `can_tx`、`can_rx`、`can_mis`、`can_err`、`can_drop`、`can_id`和`can_data`。

板端连续日志 `t=23000～27000 ms` 显示：`can_tx` 从230增长到270、`can_rx` 从229增长到269，保持每秒约10帧；`can_mis=0`、`can_err=0`、`can_drop=0`，且持续收到 `can_id=321`、`can_data=1234A55A01020304`。发送数比接收数多1，是因为同一调度轮次先在100 ms任务中发送新帧，随后1000 ms日志在下一次10 ms任务处理该帧之前打印。

同期ADC `frames` 每秒增加约100且 `adc_err=0`。SPI日志出现 `spi_rx=FFFFFFFF`、`spi_mis`持续增加，表示PB4/MISO处于悬空状态，通常是CAN接线时移除了 `PB5/MOSI -> PB4/MISO` 回环线；这不是CAN功能导致的SPI软件回归。

## bxCAN正常模式

Normal模式继续使用500 kbit/s，并启用CAN TX、RX FIFO0和SCE三条中断。CAN接口层现在区分：`can_q`为成功放入发送邮箱，`can_tx`为收到发送完成中断（真实总线上已获得ACK），`can_rx`为收到USB-CAN发来的帧，`can_err`和`can_le`记录错误中断及最后错误码。

STM32在启动5秒后每100 ms发送标准帧ID `0x321`、数据 `12 34 A5 5A 01 02 03 04`。USB-CAN需要配置为500 kbit/s，并周期发送标准帧ID `0x322`、DLC 8、数据 `A5 5A 12 34 04 03 02 01`。STM32接收后检查ID、DLC和全部8字节，错误计入 `can_mis`。

首次Normal模式测试未获得ACK，日志出现 `can_le=0x00000087`（Error Warning、Error Passive、Bus-Off、Bit Dominant Error），并因持续错误中断干扰ADC处理。当前接口层在首次严重错误后锁存 `can_fault=1`、停止继续排队、中止发送邮箱并关闭错误类通知，保留最后错误码，避免物理层故障拖垮其他周期任务；复位后重新测试。

真实总线诊断进一步确认：TSMaster能收到STM32发送的 `0x321`，说明PA12、收发器TX和CANH/CANL到USB-CAN的方向可用；但STM32持续 `can_rx=0`、`can_tx=0`。切换8 MHz HSE、在Silent模式持续接收USB-CAN的 `0x322` 后仍未收到报文。PA11高速采样约3614万次时低电平计数为0；将紫色RX线直接接地2秒后低电平计数增加约3788万次，证明紫线和PA11输入正常；重新接回模块RX后，STM32自身发送期间累计约2622万次采样仍无一次低电平。

旧蓝色模块及接线组合下，PA11始终未观察到接收波形；更换杜邦线后现象不变。该测试将故障范围缩小到收发器供电、模块接收端及其连接，但未单独证明旧芯片损坏。

更换为带独立 `5V` 主电源和 `VIO` 逻辑电源的新模块后，保持STM32使用ST-Link的3.3 V供电，由USB-TTL的5 V脚单独为收发器提供主电源，并共地。正式Normal/HSE固件重新烧录且校验通过。TSMaster保持500 kbit/s，周期发送标准帧 `0x322`、数据 `A5 5A 12 34 04 03 02 01`。从 `t=13000` 到 `t=46000 ms`，`can_tx`从80增至410、`can_rx`从129增至459，均约10帧/秒；`can_mis=0`、`can_err=0`、`can_fault=0`，持续收到正确ID和全部8字节。同期ADC帧数从1299增至4599，SPI回环无新增错误。

`can_drop=1` 是测试早期的一次累计接收缓冲区覆盖，此后至46秒未继续增加。当前接口层只有单帧接收槽；更高流量测试前需要改为接收队列。真实双节点CAN收发与ACK已验证，长期运行和异常恢复仍需单独测试。

### 周期报文 v1

多字节字段统一采用低字节在前。当前电压仅代表PA0的0～3.3 V教学输入，不能解释为真实电芯电压。

| ID | 周期/方向 | 数据字节0～7 |
|---|---|---|
| `0x321` 状态 | 100 ms，STM32→USB-CAN | `0..1` cell0 mV（uint16）；`2..5` 故障位图（uint32）；`6` 采样有效位；`7` 当前故障码 |
| `0x323` 心跳 | 1 s，STM32→USB-CAN | `0..3` 启动后秒数（uint32）；`4` 帧序号；`5` 保护状态；`6` 曾故障锁存位；`7` 对端在线位 |
| `0x322` 测试回发 | 100 ms，USB-CAN→STM32 | 保留测试数据 `A5 5A 12 34 04 03 02 01`，用于验证双向收发 |

收到正确`0x322`后更新对端在线状态；超过1秒未收到则心跳在线位变0，`can_to`增加。CAN总线错误仍会使接口暂时停止发送，每3秒由主任务尝试重启控制器和通知；`can_rec`记录完成的重启尝试，`can_rf`记录重启API失败次数，随后只有`can_tx`重新增加才能证明总线实际恢复。错误回调仍保留最后错误码并限制中断风暴。

本版本已编译、烧录和校验；主机端报文字节序测试通过。连续串口日志从`t=144000`到`171000 ms`显示`can_q=1531→1828`、`can_tx=1530→1827`、`can_rx=1441→1711`，`can_mis=0`、`can_err=0`、`can_fault=0`、`can_peer=1`，证实原有双节点通信在新固件下继续工作。`can_drop=20`在后段观测中未再增加，但单帧接收槽仍需后续改为队列。

TSMaster截图已核对三种帧：`0x322`数据为`A5 5A 12 34 04 03 02 01`；`0x321`数据为`F7 05 00 00 00 00 01 00`，即PA0教学输入1527 mV、无当前故障、采样有效；`0x323`数据为`07 07 00 00 02 00 00 01`，即运行1799秒、序号2、保护状态0、无故障锁存、对端在线。帧率显示为10、10、1帧/秒，符合设计。

后续USB-CAN从Windows设备列表消失，板端在`t=14707000～14710000 ms`保持运行，`can_tx=0`、`can_fault=1`、`can_rec=4742→4743`，说明控制器持续进行限频重启尝试。换USB数据线后设备重新枚举并连接TSMaster；未复位STM32，`t=15389000～15392000 ms`连续增长，`can_tx=1213→1246`、`can_fault=0`、`can_err=4927`保持不变，证明外部节点重新接入后发送与ACK自动恢复。起初`can_rx=0`是因为TSMaster发送`0x322`使用应用通道1，而接收`0x321/0x323`的总线是应用通道2。将发送行改到通道2后，`t=15687000～15691000 ms`的`can_tx=4490→4535`、`can_rx=148→188`、`can_peer=1`、`can_mis=0`、`can_fault=0`、`can_err=4927`保持不变，`can_data=A55A123404030201`；双向通信在未复位STM32的情况下恢复。`can_le=0x00001000`是保留的历史发送邮箱错误，不表示当前仍在报错。此次不是受控CANH/CANL断线测试。

下一步仅停止并恢复TSMaster的`0x322`周期发送，保持通道2与应用程序连接，验证`can_peer`和`can_to`的超时/重入；受控CANH/CANL断线恢复、CAN接收队列及长时间运行统计仍需后续验证。UART后续改进项是发送队列、流式拆包/粘包处理和二进制响应帧。
