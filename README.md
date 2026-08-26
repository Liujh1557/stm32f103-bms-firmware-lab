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
- 主循环不使用 `HAL_Delay()` 进行任务调度。

## TIM2 参数

当前系统及 TIM2 时钟为 8 MHz：

```text
Prescaler = 799
Period    = 9

8,000,000 / ((799 + 1) * (9 + 1)) = 1,000 Hz
```

因此 TIM2 更新中断周期为 1 ms。

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

定义 `BmsData`、`BmsConfig` 和 `BmsFault`，为 ADC 采集与保护状态机准备数据模型。
