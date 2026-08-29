# GD32E230 Oscilloscope + ECG + Signal Generator

本目录从上游 `Oscilloscope-V2.0` 独立复制，形成可烧录的三功能固件：示波器、ECG 监测和自由拓展的 PWM 信号发生器。原上游目录不承载本次功能改动；新增目录、文件、C 标识符和屏幕标签均使用英文/ASCII 命名。

## 功能分区

- `Middle/ecg_core.*`：与硬件无关的 ECG 波形、R 峰检测、BPM、PWM 映射和显示跨度算法，可在电脑上测试。
- `APP/ecg_task.*`：ECG 运行状态、PWM 输出、ADC 输入检测、波形缓冲、TFT 页面和心形动画。
- `APP/osc_window.*`：基础示波器的安全触发窗口选择算法。
- `APP/signal_gen_task.*`：20 Hz–20 kHz、5%–95% 占空比的 PA2 PWM 信号发生器。
- `Middle/mid_timer.*`、`Middle/timer_math.*`：以 1 ms 中断驱动 ECG 固定节拍采样，并安全计算跨回绕的输入捕获周期。
- `User/main.c`：模式切换和按键/编码器事件路由。
- `Tests/`：核心算法、ECG 应用层硬件桩与示波器窗口的主机端行为测试。
- `Project/Oscilloscope.uvprojx`：Keil 工程，目标为 GD32E230C8T6。

## 操作

- 长按编码器按键（KEYD）：按 `OSC -> ECG -> SIGNAL GEN -> OSC` 循环切换页面。
- ECG 页面旋转编码器：调节横坐标为 1–4 个心动周期。
- ECG 页面 KEY1：打开或关闭 ECG PWM 输出。
- ECG 页面 KEY2 单击：提高模拟心率；双击：降低模拟心率。
- 返回基础示波器后，PWM 周期、占空比和开关状态恢复为进入 ECG 前的值。
- Signal Generator 页面旋转编码器或按 KEY1/KEY2 调整频率；KEY3 单击/双击调整占空比；KEYD 短按打开或关闭 PA2 输出。
- 信号发生器与 ECG 共用 TIMER14_CH0/PA2，页面进入/退出会保存并恢复 PWM，三种模式不会同时争用外设。

## Signal generator

- 输出引脚：PA2 (`TIMER14_CH0`)，1 MHz 定时器基准。
- 频率预设：20、50、100、200、500 Hz，以及 1、2、5、10、20 kHz。
- 占空比范围：5%–95%，步进 5%；周期和脉宽均在写入硬件前钳位。
- 闭环自测：使用跳线将 PA2 接至 PA3，在 Signal Generator 页面设置波形，再切回 Oscilloscope 页面观察。接线前必须确认两端共地且 PA3 不承受超过 3.3 V 的外部电压。

## ECG 输出与显示

- TIMER14/PA2 保持 1 MHz 定时器基准，ECG 模式使用 1000 个计数的载波周期（约 1 kHz），兼顾 PWM 分辨率与 ECG 包络带宽。
- ECG 包络以 250 Hz 更新，占空比限制在 10%–90%，便于后级低通或实验设备观察。
- R 峰检测保持 250 Hz；显示历史按 125 Hz 存储，因此 1000 字节缓冲可覆盖 30 BPM 下完整的 4 个周期，而不额外占用紧张的 RAM。
- PA2 只负责输出模板波形；PA3 的连续 ADC 最新值独立用于屏幕波形、R 峰、实测 BPM 和心形同步。需要观察自身输出时，应按实验电路经低通/调理后接入 PA3。
- 默认输出心率为 72 BPM；实测 BPM 可与输出 OBPM 不同。R 峰检测包含 250 ms 不应期，并使用最近 4 个有效 RR 间期平滑 BPM；连续 3 秒无 R 峰时实测 BPM 清零并重置检测历史。
- 检测到 R 峰后，屏幕心形在 160 ms 内放大显示，从而与心搏同步。

### Signal quality and alarms

- `ECG_GetSignalQuality()` reports `ECG_SIGNAL_UNKNOWN`, `ECG_SIGNAL_GOOD`,
  `ECG_SIGNAL_POOR`, or `ECG_SIGNAL_LOST`. The monitor uses a fixed one-second
  window at 250 Hz and does not allocate memory dynamically.
- A centered peak-to-peak span below 80 ADC counts is treated as signal loss.
  A span below 300 counts or at least 5 percent rail-clipped samples is treated
  as poor quality. Thresholds are conservative defaults and must be checked
  against the actual PA3 analog front end during board acceptance.
- `ECG_GetAlarmFlags()` returns the bit mask `ECG_ALARM_BRADY`,
  `ECG_ALARM_TACHY`, and `ECG_ALARM_SIGNAL_LOST`. Rate alarms are enabled only
  for good-quality input; signal loss or the existing three-second BPM timeout
  takes priority.
- The TFT status uses `Q:G`, `Q:P`, `Q:L`, and `Q:?`. Alarm text is `ALM:LOW`,
  `ALM:HIGH`, or `ALM:SIG`. All new source identifiers and UI labels are ASCII
  English.

## 检查点

1. **来源与需求**：仓库固定在上游 `master` 的 `401d0f1`；PPT 第 16、17 页需求已逐项映射。
2. **基础工程**：原 V2.0 使用 Arm Compiler 6.19；本机缺少该版本。新分区仅将工程声明迁移到已安装的 6.24，迁移前失败原因和迁移后构建结果均保留在任务记录中。
3. **基础示波器**：波形显示、PWM 开关/频率/占空比和编码器横轴调节沿用 V2.0，并通过完整工程构建回归。
4. **ECG 核心**：先运行缺少实现的测试得到链接失败（红灯），再实现并通过波形形态、PWM 边界、R 峰不应期、BPM、多周期跨度和心形时窗测试（绿灯）。
5. **ECG 集成**：应用层测试验证每 4 ms 只更新一次 PWM、60 BPM ADC 输入与 72 BPM 输出相互独立、3 秒无信号后 BPM 清零、按键边界、1–4 周期、网格重绘、大小心形和 PWM 状态恢复。
6. **示波器回归**：触发窗口测试覆盖短数据、末端触发、负坐标和无触发回退；横轴步进限制为 1–10，保证始终有 100 个有效显示点。定时器测试覆盖单次及多次 16 位回绕。
7. **PWM 回归**：测试验证软件周期与硬件 ARR 的 `N`/`N-1` 关系、占空比钳位、零周期保护和影子寄存器提交。
8. **信号发生器**：主机测试验证频率/占空比边界、预设步进、输出开关和 PWM 状态恢复；Keil 工程契约确认模块被编译进固件。
9. **完整构建**：Keil 完整构建必须为 0 Error、0 Warning；同时复跑全部主机端测试。
10. **上板验收**：烧录后分别验证 PA2 输出、PA3 实测输入、三模式切换、1–4 周期显示、BPM、报警和心形同步。没有连接实物时，该项必须标记为待上板，不以软件测试代替。

## 主机端测试

在本目录执行 `Tests/run_tests.ps1`，或手动运行：

```powershell
gcc -std=c11 -O2 -Wall -Wextra -Werror -fanalyzer -I .\Middle .\Tests\test_ecg_core.c .\Middle\ecg_core.c -o $env:TEMP\gd32_ecg_core_tests.exe
& $env:TEMP\gd32_ecg_core_tests.exe
```

## Keil 构建

使用 Keil MDK 5.43a、Arm Compiler for Embedded 6.24，并先安装 `GigaDevice.GD32E23x_DFP 1.0.2`。打开 `Project/Oscilloscope.uvprojx`，选择 `GD32E230C8T6` 后 Build。工程使用 `-Oz`，以满足 MDK Lite 的 32 KiB 镜像限制；输出文件位于 `Project/Objects/Oscilloscope.hex`。

2026-08-29 三功能固件全量 Rebuild 结果：`0 Error(s), 0 Warning(s)`；程序尺寸为 `Code=19638, RO-data=7574, RW-data=28, ZI-data=7364`，最大静态栈深为 472 字节。Flash 的 `Code+RO` 为 27212/32768 字节；RAM 的 `RW+ZI` 为 7392/8192 字节，仅剩余 800 字节。静态栈报告仍含不可追踪函数指针，上板前必须保留栈余量和长时间运行检查点。可烧录 HEX 的 SHA-256 为 `65CD6E69E213F8D1AC74C0B07775E2B9E7CEA96BDABA024D38820B10187F71EA`。

> 注意：PPT 与上游说明均要求最终做现场实物演示。本项目可以完成可复现的软件构建和算法验证，但 PA2 波形质量、实际板级按键/编码器极性、TFT 刷新效果仍需在目标板上确认。
