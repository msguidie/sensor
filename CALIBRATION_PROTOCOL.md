# CALIBRATION PROTOCOL — 台面实测程序

> 拿到传感器后照这份做。每个步骤编号（M1、M2…）对应 `sensor_cal.h` 里的待填常数。
> 全程用 `cal_harness.c`（有线 `Vex & USB` 模式，读 ROBOTC debug stream）。
> 预计耗时 **60–90 分钟**，两人配合更快（一人扶球一人按开关）。

---

## 准备

| 需要 | 说明 |
|---|---|
| Cortex + 电池 + USB A-A 线 + 笔记本 | Cortex 没有 LCD，读数只能走 debug stream，**必须有线** |
| 卷尺 / 直尺，贴在桌面上 | 量距离用 |
| **那颗网球** | 关键，见 M2 |
| 一块平板（书本/纸板） | 做对照，见 M2 |
| 赛场的黄色反光胶带（或器材单给的黑卡纸） | 见 M5 |
| 一个限位开关（当采样触发按钮） | 接 `cal_harness.c` 里 `TRIGGER_SENSOR` 指定的口（默认 `D12`） |

**上机前先做**：
1. 填 `robot_config.h` 顶部的端口 TODO 块
2. ROBOTC → Motors and Sensors Setup → 重新生成 `#pragma config`，**覆盖掉源文件里的占位块**
3. 编译 `cal_harness.c` → 下载

---

## M1 · 确认 ADC 量程与静息值

**目的**：确认模拟读数范围（预期 0–4095，12 位），以及各口的无信号基线。

1. 所有 Sharp 前方清空（对着 1.5m 以外的空处）
2. 看 dashboard 连续读数 30 秒

**记录**：每个 Sharp 的静息读数、抖动范围、最大观测值
**回填**（`sensor_cal.h`）：`CAL_ADC_MAX`、`CAL_SHARP_IDLE_RAW`

---

## M2 · Sharp 距离标定（核心，最花时间）

**⚠️ 必须用网球测，不能只用平板。** 网球小且是曲面，回波比平板弱得多；用平板标定出来的曲线放到实战会系统性高估距离。

对 **4 个 Sharp 逐个** 做（3 个 10–80cm 型号个体差异可能不小，不能共用一条曲线）：

**距离点位**（cm）：
```
3, 4, 5, 6, 7, 8, 9, 10, 12, 15, 20, 25, 30, 40, 50, 60, 70, 80
```
10 cm 以下加密，因为**折返区就在那里**。

**每个点位**：把球放在刻度上正对传感器 → 按触发开关 → harness 采 50 样本 → 记录 `均值 / 最小 / 最大 / 标准差`

**同时做平板对照**：至少在 10、20、40、60 cm 四个点各测一次平板，记下平板与网球的读数差。

**回填**（`sensor_cal.h`）：
- `CAL_SHARP_LONG_K` / `CAL_SHARP_LONG_C`（10–80cm 型号）与 `CAL_SHARP_SHORT_K` / `CAL_SHARP_SHORT_C`（4–30cm 型号）
  —— 代码用的是双曲反演 `distance_mm = 10 * K / (raw + C)`。Sharp 的特性是 `1/V` 与距离近似线性，
  所以把 `1/距离` 对 `raw` 做最小二乘拟合即可解出 K、C。
  **只用 15–80 cm 段的数据拟合**（3–12 cm 段是折返区，放进去会把曲线带偏）。
  > 三个长距传感器**要分别拟合**。现在三个共用一组 `CAL_SHARP_LONG_*` 是简化，
  > 实测后若个体差异明显，应把它拆成三组常数并相应改 `rawToMm()`。
- `CAL_SHARP_LONG_FOLDBACK_RAW` —— 读数峰值对应的原始值。**大于这个值就说明进了折返区，距离读数不可信**
- `CAL_SHARP_LONG_MIN_MM` / `CAL_SHARP_LONG_MAX_MM`、`CAL_SHARP_SHORT_MIN_MM` / `CAL_SHARP_SHORT_MAX_MM`
  —— 上限取标准差开始明显变大的那个距离，就是实际可用量程上限（多半远小于标称 80 cm）
- 顺带确认滤波参数 `CAL_FILTER_GAIN_LONG` / `CAL_FILTER_GAIN_SHORT` / `CAL_DETECT_CONFIRM_COUNT` 是否合适

---

## M3 · Sharp 有效检测锥角

**目的**：决定搜索时每次转多少度。转多了漏球，转少了浪费时间。

在 **20 cm、40 cm** 两个距离上，把球横向平移：
```
0, ±2, ±5, ±8, ±12, ±16 cm
```
找出读数还能可靠识别出球的最大横向偏移。

`半锥角 = atan(最大横向偏移 / 距离)`

**回填**（`sensor_cal.h`）：`CAL_SHARP_HALF_CONE_DEG`
**下游用途**：搜索扫描步进角应 **≤ 2 × 半锥角**，否则两次扫描之间会有盲区。

---

## M4 · 球 vs 对手机器人（上下叠放判据）

往届公认做法：下方 Sharp 对准球高度，上方 Sharp 高于球、只能看到对手。

1. 只放网球在 20 / 40 cm → 记录 `下读数 / 上读数`
2. 放一个高于球的物体（书本立起来，模拟对手车身）在同样距离 → 记录两个读数
3. 球和高物体同时在场 → 记录

**回填**（`sensor_cal.h`，注意代码里全部是**标定后的 mm**，不是原始 ADC 值）：
- `CAL_UPPER_PRESENT_MM` —— 上方传感器判定"有高物体"的最远距离，超过就算上方没东西
- `CAL_OPPONENT_DIFF_MM` —— 上下两读数之差小于此值即判为同一个高物体（对手）；大于则是"球在对手前方"
- `CAL_BALL_DETECT_MAX_MM` —— 认定"看见球"的最远距离
- `CAL_BALL_GRAB_MM` —— 短距传感器判定球已进入铲子可动作范围的距离
- `CAL_HYSTERESIS_MM` —— 迟滞量，释放距离 = 检测距离 + 此值

> 往届 `josephinemonica` 用的是 `SensorValue[B] > SensorValue[C] + 300`，300 是他们的 margin。**这个数不要直接抄，必须自己测**。

---

## M5 · IR 线阵阈值与朝向

**⚠️ 分两次做，因为标定面和比赛面不同。**

**第一次（实验室，用器材单发的黑卡纸/反光胶带）**
1. 四个模块逐个悬在**非线区域**（普通桌面）→ 记读数
2. 悬在**反光带上**→ 记读数
3. 若模块是数字输出（预期如此）：调模块上的电位器，直到两种情况稳定输出 0 / 1
4. 若是模拟输出：记录两个读数，中点作为阈值

**第二次（进赛场后必须重做）**
赛场边界是**黄色反光胶带**，与黑卡纸的反射率不同。**实验室调好的阈值到赛场不一定成立。**

同时记录：**离地高度**。IR 循线模块对安装高度极敏感，高了低了都失效。

**回填**（`sensor_cal.h`）：`CAL_LINE_DIGITAL_MODE`（1=数字 / 0=模拟）、`CAL_LINE_ON_LEVEL`（0 或 1）、
`CAL_LINE_CONFIRM_COUNT`、`CAL_LINE_HEIGHT_MM`（仅作记录）；
若为模拟模式，还要逐个回填 `CAL_LINE_THRESH_FL` / `_FR` / `_RL` / `_RR`（**四个各测各的，不要共用一个阈值**）

---

## M6 · 限位开关（含 analog 口读法）

按方案 A，有 3 个限位开关接在 **analog 口**上。

对每个：按下 → 记读数；松开 → 记读数。取中点作阈值。

**回填**（`sensor_cal.h`）：`CAL_SWITCH_ANALOG_PRESSED_MAX`（低于此值 = 按下；预期松开时接近满量程、按下时拉到 0，实测确认方向）、`CAL_SWITCH_CONFIRM_COUNT`

数字口上的开关直接读 0/1，记录**哪个值代表按下**（`sensorTouch` 与 `sensorDigitalIn` 极性可能相反）
→ 回填 `CAL_SWITCH_DIGITAL_PRESSED_LEVEL`。

同时在 `robot_config.h` 里登记每个开关接的是哪类口：`SW_SCOOP_TOP_ON_ANALOG` 等四个宏（1=analog，0=digital）。

---

## M7 · Digital Compass 1490 解码验证

> 器材单标注 "will be issued later"，**未发放前跳过本步**。

1. 确认供电引脚已拉高，罗盘有输出
2. 把机器人（或罗盘模块）缓慢转一整圈
3. 记录 4 个引脚在 8 个方位上的 0/1 组合

**回填**：
- `robot_config.h` 的 `PORT_COMPASS_N` / `_E` / `_S` / `_W` —— **本步真正要确认的就是这个**：
  哪根线是哪个方向。填对了解码就成立。
- `sensor_cal.h` 的 `CAL_COMPASS_ACTIVE_LOW`（预期 1）、`CAL_COMPASS_DRIVE_SUPPLY`、`CAL_COMPASS_CONFIRM_COUNT`

> **不需要标定"编码→方位"查表。** `compassDecodeDeg()` 是结构解码，不查表：
> 恰好一位有效 = 该正方向，环上相邻两位有效 = 两者之间的斜方向。
> 所以只要四个引脚对应关系填对，与具体接线顺序无关。

> 往届解码式：`code = 8*W + 4*S + 2*E + 1*N`，`josephinemonica` 得到的映射是
> `14=N, 13=E, 11=S, 7=W, 12=NE, 9=SE, 3=SW, 6=NW`（**引脚为低有效**）。
> 这套映射与模块批次和接线顺序有关，**必须自己转一圈实测确认**。

---

## M8 · Shaft Encoder 每米脉冲数

1. `clearTimer` / 清零编码器
2. 手推机器人沿直线走 **1.00 m**（用卷尺量）
3. 读脉冲数，重复 3 次取平均

**回填**（`sensor_cal.h`）：`CAL_ENC_TICKS_PER_M`；确认轮型后再定 `CAL_ENC_TRUSTWORTHY`（全向轮填 0）

**⚠️ 先确认轮型**：如果是全向轮，横向滚子会让推行时产生打滑，标定值和实际行驶差异较大。若为全向轮，此项标定意义有限，导航应以罗盘为主。

---

## 采完之后

1. 把全部数值填进 `sensor_cal.h`，把每项的 `PROVISIONAL` 标记删掉
2. 原始数据（每个点位的均值/标准差）**贴进 logbook**，并保留一份电子表 —— 报告 checklist 第 4 项 `Design Calculations` 直接用得上
3. 画一张 `raw ADC vs 距离` 的散点+拟合曲线图 —— 这是报告里最容易得分的一张图

---

## 现场调试清单（之后每次上场前）

- [ ] 电池电压（Sharp 读数随电压漂移，**低电量时阈值会失准**）
- [ ] IR 线阵离地高度没有被撞变
- [ ] 四个 Sharp 镜头没有被灰尘/手指印遮挡
- [ ] 在赛场实际的黄色胶带上复测一次线阵触发
