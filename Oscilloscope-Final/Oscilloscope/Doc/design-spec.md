# GD32E230 简易示波器 — 设计思路 Spec（精简版）

> 对应工程：`Oscilloscope-Final/`  
> 视频来源：作者演示手记（视频中出现的 ECG / PPG 模式是**其它工程**的演示，**不在本工程**中）  
> 文档定位：严格对应 `Oscilloscope-Final/User/` + `Hardware/` 当前代码，与其它工程（`Oscilloscope-ECG`、`Oscilloscope-V2.0`、`Demo13-ECG-Monitor`）的差异在文末说明。

---

## 0. 证据等级

| 标识 | 含义 |
|------|------|
| `[代码]` | 源码里能查到、运行行为确定 |
| `[视频]` | 视频画面里**直接能看到**的 |
| `[推测]` | 从代码 / 视频综合推断，**没有直接证据** |

---

## 1. 硬件设计

### 1.1 板级构成
| 模块 | 选型 | 证据 |
|------|------|------|
| MCU | 嘉立创 GD32E230 核心板（Cortex-M23 @ 72 MHz） | `[推测]` 板上丝印 |
| 显示屏 | 1.8 寸 TFT，160×128 像素 | `[代码]` `TFT_Fill(0, 0, 160, 128, BLACK)` |
| 信号输入 | BNC 母头 | `[视频]` |
| 电源 | USB Type-C（5 V 供电 + 板内 3.3 V LDO） | `[视频]` |
| 人机输入 | 3 颗轻触按键 + 1 颗 EC11 旋转编码器（带按压） | `[视频]` |
| 状态指示 | 红 + 绿 双色 LED | `[视频]` |
| 测试焊盘 | A1 / A2 / A3 / A4 | `[视频]` 板上丝印 |

### 1.2 引脚分配（按代码精确核对）
| 引脚 | 功能 | 证据 |
|------|------|------|
| PA3 | ADC 通道 3 输入（波形采样） | `[代码]` `adc.c::gpio_mode_set(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_3)` |
| PA6 | TIMER2_CH0 输入捕获（频率 / 占空比测量） | `[代码]` `freq.c::gpio_af_set(GPIOA, GPIO_AF_1, GPIO_PIN_6)` |
| PA2 | TIMER14_CH0 PWM 输出（信号源） | `[代码]` `timer.c::GPIO_AF设置--PA2-TIMER14_CH0` |
| PB13 / PB14 / PB15 | KEY1 / KEY2 / KEY3 外部中断输入 | `[代码]` `key.c::Init_Key_GPIO` |
| PB3 / PB4 | EC11 编码器 B 相 / A 相 | `[代码]` `key.c::EXTI4_15_IRQHandler` |
| PB9 | EC11 中心按键（KEYD，暂停/继续） | `[代码]` `key.c::KEYD_SCAN` |
| SPI 总线 | TFT 屏 | `[代码]` `TFT_Init()` |

---

## 2. 软件架构

### 2.1 顶层循环
```c
// User/main.c
while (1) {
    Key_Sacnf(&oscilloscope);     // KEY1/2/3 扫描
    KEYD_SCAN(&oscilloscope);     // EC11 中心按键扫描
    Key_Handle(&oscilloscope);    // 按键事件处理

    if ((oscilloscope.showbit != 0U) && (oscilloscope.paused == 0U)) {
        oscilloscope.showbit = 0U;
        Process_Captured_Frame(&oscilloscope);
        ADC_StartCapture();
        TFT_ShowUI(&oscilloscope);
    }
}
```
- **事件驱动**：`showbit` 由 DMA 传输完成中断置 1；
- **暂停语义**：`paused == 1` 时只冻结显示，不清缓冲。

### 2.2 采集通路
- ADC + DMA **单帧 300 点**（`OSC_CAPTURE_COUNT = 300U`）。
- ADC 时钟 = AHB/9 = 8 MHz；采样时间档位 5 档（239.5 / 71.5 / 55.5 / 41.5 / 28.5 cycles）。
- DMA **非循环模式**（`dma_circulation_disable`）：一帧完成即停，主循环处理期间不覆盖缓冲。
- 处理函数 `Process_Captured_Frame()`：
  1. ADC 原始值 → "5 V 中心"对称电压（`voltageValue[i] = 5.0 - 2.0 * adc_voltage`）；
  2. 调 `scope_analyze_frame` 拿 min / max / Vpp / trigger_index；
  3. 从 `trigger_index` 起取 100 点 → `drawCurve` 画波形。

### 2.3 触发与显示（`scope_math.c`）
- 帧内 `min / max` → `Vpp`；
- Vpp ≤ 0.001 V → 视作平线，居中显示，**不报触发**；
- 阈值 = min + 0.5 × Vpp，**从前往后找上升沿**作触发点；
- `i > count - display_width` 时强制用 `latest_start`，**避免触发点过尾、显示窗口越界**；
- 绘图坐标夹在 `[0, 49]`，**绝不越界**。

### 2.4 频率 / 占空比测量
- `TIMER2_CH0`（PA6）输入捕获，1 MHz 时基（prescaler=71，AHB 72 MHz / 72）；
- 16 位计数器回绕处理：溢出累加，最多 4 次（≈ 262 ms）后清零；
- 频率 = `1 MHz / 累计 tick`；
- 显示单位 **Hz / kHz 自适应**（`gatherFreq >= 1000` 时切 kHz）。

### 2.5 PWM 输出信号源（PA2）
- `TIMER14_CH0` PWM 模式 1 输出；
- 三档频率（`KEY3` 循环切换 `timerPeriod`）：
  | timerPeriod | 频率 | 来源 |
  |------------|------|------|
  | 1000 | 1 kHz | 默认 |
  | 500 | 2 kHz | `KEY3` 按 1 次 |
  | 250 | 4 kHz | `KEY3` 按 2 次 |
  | 1000 | 1 kHz | `KEY3` 再按循环回 |
- 占空比按 `timerPeriod * 4 / 100` 步进（每按 1 次 `KEY1` 加 4 %）；
- ARR 写 `period - 1`，切换频率时**按比例缩放 `pwmOut`，保持占空比不变**。

### 2.6 关键常量
```c
#define OSC_CAPTURE_COUNT 300U   // DMA 一帧采样数
#define OSC_DISPLAY_WIDTH 100U   // 屏幕显示宽度
#define OSC_PLOT_HEIGHT   50     // 像素高度（0~49）
```

---

## 3. HMI

### 3.1 键位定义（按 `key.c::Key_Handle` 严格核对）
| 操作 | 行为 | 触发 |
|------|------|------|
| KEY1 短按 | PWM 占空比 +4 % | PB13 |
| KEY2 短按 | 切换 PA2 PWM 输出使能（开/关） | PB14 |
| KEY3 短按 | 切换 PWM 输出频率档（1k→2k→4k→1k） | PB15 |
| EC11 顺时针 | ADC 采样时间档 239.5→71.5→55.5→41.5→28.5 | PB3/PB4 |
| EC11 逆时针 | ADC 采样时间档 28.5→41.5→55.5→71.5→239.5 | PB3/PB4 |
| KEYD 短按 | 暂停 / 继续（冻结当前帧） | PB9（EC11 中心键） |

### 3.2 屏幕布局（按 `tft.c::TFT_StaticUI` 核对）
| 区域 | 坐标 | 内容 |
|------|------|------|
| 标题 | (10, 0) | "简易示波器"（GBK 字库） |
| 波形区 | (0, 0)~(110, 100) | 100 点 × 50 像素 |
| 输入峰值 | (5, 106) | Vpp 读数（如 `1.40V`） |
| 输入频率 | (55, 106) | Hz / kHz 自适应 |
| PWM 标签 | (110, 0) | "  PWM  " |
| 输出状态 | (110, 20) | "输出状态" + 打开/关闭 |
| 输出频率 | (110, 56) | 1kHz/2kHz/4kHz |
| 占空比 | (110, 92) | "占空比" + 百分比 |

---

## 4. 视频实测到的能力

> 视频里**没有出现** ECG / PPG / SCOPE / RUN 模式切换；以下只是**信号源回灌到 PA3** 时在屏幕上看到的读数。

| 信号 | 视频中的屏幕读数 | 揭示 |
|------|----------------|------|
| 上电无输入 | 1.4 V 直流 | 默认可作 DC 电压表 |
| RIGOL 1 kHz 正弦 | 1.0 kHz / 0.7 V | kHz 级稳定 |
| RIGOL 9.8 Hz | 9.8 Hz | Hz 级稳定 |
| DC 16.0 V | 16.0 V | 读数能跨量程（分压前端） |
| 500 Hz 方波 | 500 Hz / 50 % | 占空比读数 |
| 时基 5.0us / 2.00us | `T=5.0us` / `T=2.00us` | µs 级时基 |

> 视频**没有**演示：1 MHz 以上信号、ECG / PPG 模式、真实人体电极接入。
> 视频中如出现 "ECG / PPG" 字样，那是**其他工程**（`Oscilloscope-ECG` / `Oscilloscope-V2.0`）的演示，**与本工程无关**。

---

## 5. 验证用例（推荐）

1. **DC 读数**：信号源 DC 0/1/5/15 V → 屏幕 Vpp 误差 ≤ 5 %；
2. **kHz 正弦**：1 kHz 1 Vpp → ≥ 3 完整周期；
3. **Hz 正弦**：10 Hz 1 Vpp → ≥ 1 完整周期；
4. **频率读数**：1 kHz → 显示 1.0 kHz；1 Hz → 显示 1 Hz；
5. **占空比**：KEY1 多次按 → 屏幕占空比每次 +4 %（`timerPeriod = 1000` 时）；
6. **PWM 频率切换**：KEY3 依次按 → 1k → 2k → 4k → 1k，占空比读数**不变**；
7. **PWM 输出开关**：KEY2 → 关闭时 PA2 拉低，重新开 → 恢复；
8. **暂停**：按 EC11 → 波形冻结，Vpp 读数也冻结；再按 → 恢复刷新。

---

## 6. 复刻与二开

### 6.1 完全复刻
- 嘉立创 EDA 工程 → 按 `Hardware/` 打板；
- 烧录 `Project/Oscilloscope.uvprojx`（**Keil AC6，不能切 AC5**）；
- 源码路径**不能含中文**（AC6 索引失败，AC6 编译器对中文支持差）。

### 6.2 二开方向
- **更稳触发**：固定半 Vpp → 可调触发电平 + 上升/下降沿可选；
- **加存储**：外挂 SPI Flash，每帧 300 点落盘；
- **加通信**：补 USB CDC，把原始 ADC 推 PC 上位机；
- **PWM 占空比细调**：当前按 4 % 步进，可改 1 % 步进；
- **双缓冲 DMA**：把"单帧 + showbit"改成"双缓冲 + 半传输中断"→ 屏刷更顺。

### 6.3 性能改进
- `scope_analyze_frame` 加 FFT 子模块（受 Cortex-M23 算力限制需谨慎）；
- UI 文字用统一字库（GBK 字节 + 字体表）→ 跨编译器一致。

---

## 7. 风险与红线

- **不**测市电 / 高压：板级 5 V USB 直接供电，无任何隔离；
- **不**让 PA3 ADC 输入超过 3.3 V：内部 ADC 量程限制；
- **不**接入人体电极 / 探头：电流路径无患者保护；
- **不**把"输入峰值"当医用监护数据：仅教学/演示用途；
- **不**替换编译器为 AC5：GD32E230 是 ARMv8M，AC5 不可用；
- **不**在源码路径中带中文：AC6 编译会失败。

---

## 8. 与其它工程的关系

工作区里**不是**所有工程都是这个 `Oscilloscope-Final`：

| 目录 | 主要功能 | 与本工程关系 |
|------|---------|------------|
| `Oscilloscope-Final` | 简易示波器 + 1k/2k/4kHz PWM 输出 | **本文档目标** |
| `Oscilloscope-V2.0` | 升级版（多模式） | 上游，本文档**不**覆盖 |
| `Oscilloscope-ECG` | 示波器 + ECG 监测 + 信号源（10Hz–10kHz） | 平行工程，**不**覆盖 |
| `Demo13-ECG-Monitor` | 单功能 ECG 教学板 | 平行工程，**不**覆盖 |

> 视频里出现的 **ECG / PPG / BPM / QRS 模板** 来自**这些其它工程**的演示，不在 `Oscilloscope-Final` 范围内。

---

## 9. 术语表

- **Vpp**：峰峰值；本工程屏幕"输入峰值"对应的就是 Vpp。
- **占空比**：高电平时间 / 周期。
- **PWM 输出频率**：1k / 2k / 4k Hz 三档可切换。
- **ADC 采样时间档**：239.5 / 71.5 / 55.5 / 41.5 / 28.5 cycles，档位越短 → 采样越快 → 越能跟得上高频。
- **不应期 / R 波 / BPM**：本工程**没有**这些概念，列在术语表只是因为读者可能从其它工程过来。
