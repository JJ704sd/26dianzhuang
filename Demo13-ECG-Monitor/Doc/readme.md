# Demo13 ECG Monitor 使用与验收说明

本目录以 `Oscilloscope-V2.0.zip` 为源码基准，保留其 20 kSa/s ADC-DMA、示波器/ECG 双页面和滚动波形框架，并增加可控信号输出、自由滚动/上升沿/下降沿显示以及按键功能。

## 接线

- 信号输入：`PA3 / ADC_CHANNEL_3`，必须与信号源共地。
- PWM 输出：`PA2 / TIMER14_CH0`，默认关闭，防止上电误输出。
- 自测方波：连接 `PA2 -> PA3`，并连接 GND。可直接观察 500 Hz、1 kHz、2 kHz 方波。
- 正弦和 ECG 输出采用 20 kHz PWM 包络。板上没有 DAC，`PA2 -> PA3` 直接相连时采到的是 PWM 载波，不是原始模拟正弦/ECG。验收模拟波形时，应在 PA2 与 PA3 之间加低通滤波器，或使用外部信号发生器。
- ADC 允许范围为 0~3.3 V，严禁输入负电压或超过 3.3 V；双极性信号必须先衰减并抬升到约 1.65 V 中点。

## 操作

| 操作 | 功能 |
| --- | --- |
| KEY1 短按 | PWM 输出开/关 |
| KEY1 长按 2 s | SCOPE 页面切换小信号/大信号量程 |
| KEY2 短按 | 输出波形依次切换 SQU、SIN、ECG |
| KEY2 长按 2 s | 切换 SCOPE / ECG 页面 |
| KEY3 短按 | 调节频率或 ECG 心率 |
| KEY3 长按 2 s | SCOPE 页面切换 FREE、RISE、FALL |
| 编码器旋转 | 调节当前页面时基 |
| 编码器按下 | RUN / HOLD |

FREE 显示连续滚动的原始采样窗口，用于观察波形是否持续移动；RISE/FALL 分别按中点上升沿/下降沿对齐，用于稳定观察边沿。触发显示稳定不移动属于预期行为。

## 建议验收顺序

1. 不接 PA2，使用信号发生器向 PA3 输入 1 kHz、0.5~2 Vpp、1.65 V 偏置方波，确认 FREE 下连续滚动，RISE/FALL 下边沿稳定且方向正确。
2. 输入同范围正弦波，逐档旋转编码器，确认时基改变后仍持续采样；长按 KEY1 检查两档量程。
3. 输入带 1.65 V 偏置的 ECG 模拟波，长按 KEY2 进入 ECG 页面，确认波形方向、心率和峰峰值显示合理。
4. PA2 连接示波器，短按 KEY1 开启输出；短按 KEY2/KEY3 检查方波频率以及 SIN/ECG 的 PWM 包络参数。
5. 若要回环验收 SIN/ECG，在 PA2 后增加低通滤波，再接 PA3；不要把未滤波 PWM 误判为模拟波形失真。

## 构建与测试

- Keil 工程：`Project/Oscilloscope.uvprojx`
- 主机测试：在 PowerShell 执行 `Tests/run_tests.ps1`
- 当前工程使用 GD32E23x DFP 1.1.0 和 ArmClang 6，启用体积优化以满足 MDK-Lite 32 KB 限制。
