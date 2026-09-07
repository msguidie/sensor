# TUNING — 拿到真机后需要调整的参数

**当前全部为占位值。** 每项标了实测步骤（M1–M8，见 `CALIBRATION_PROTOCOL.md`）。

分两类：
- 🔴 **必须实测** —— 不测就不能用，占位值几乎肯定不对
- 🟡 **可先跑再调** —— 占位值大概率能跑起来，跑起来后再优化

---

## A. `robot_config.h` —— 端口与机械（全部为 TODO，不填无法编译）

| 参数 | 类型 | 怎么定 |
|---|---|---|
| `PORT_SHARP_*` ×4 | 🔴 | 组内定端口。**先做 `PORT_MAP.md` 里方案 A / B 的取舍**（digital 口需求 17 路只有 12 路） |
| `PORT_LINE_*` ×4 | 🔴 | 同上 |
| `PORT_SW_*` ×4 + `SW_*_ON_ANALOG` ×4 | 🔴 | 每个开关接 analog 还是 digital，登记清楚 |
| `PORT_COMPASS_*` ×5 | 🔴 | 含供电引脚。供电可放 analog 口省一路 digital |
| `PORT_ENC_LEFT/RIGHT` | 🔴 | 方案 A 只装一个，另一个填 `SENS_PORT_NONE` |
| `WHEEL_DIAMETER_MM` | 🔴 | 量实物。CAD 里写 2.75in ≈ 69.85mm，但**器材单未注明轮型** |
| `TRACK_WIDTH_MM` | 🔴 | 量左右驱动轮中心距 |
| `DRIVE_GEAR_RATIO` | 🔴 | 直驱填 1 |
| `USING_OMNI_WHEELS` | 🔴 | **拿轮子看一眼**：轮缘有一圈小滚子就是全向轮。这决定编码器还有没有用 |

---

## B. `sensor_cal.h` —— 标定常数

### Sharp 测距

| 参数 | 占位值 | 来源 | 步骤 | |
|---|---|---|---|---|
| `CAL_SHARP_LONG_K` / `_C` | 24339 / 149.1 | JadeHouseDisco 的实测拟合 | M2 | 🔴 |
| `CAL_SHARP_SHORT_K` / `_C` | 9200 / 100 | 按量程比例猜的，**最没把握的一项** | M2 | 🔴 |
| `CAL_SHARP_LONG_FOLDBACK_RAW` | 2600 | 无往届数据，纯估计 | M2 | 🔴 |
| `CAL_SHARP_LONG_MIN/MAX_MM` | 100 / 700 | 标称量程收窄后的保守值 | M2 | 🔴 |
| `CAL_SHARP_SHORT_MIN/MAX_MM` | 40 / 280 | 同上 | M2 | 🔴 |
| `CAL_SHARP_IDLE_RAW` | 250 | 往届 clamp 值 | M1 | 🟡 |

> **三个长距传感器要分别标定。** 现在共用一组系数是简化，M2 做完应该拆成三组。个体差异在 Sharp 上不小。

### 滤波

| 参数 | 占位值 | 步骤 | |
|---|---|---|---|
| `CAL_FILTER_GAIN_LONG` | 0.50 | M2 | 🟡 |
| `CAL_FILTER_GAIN_SHORT` | 1.00（不滤波，保证响应快） | M2 | 🟡 |
| `CAL_DETECT_CONFIRM_COUNT` | 3 | M2 | 🟡 |

滤波越强越稳但越迟钝。先按占位值跑，看误触发多就调大 confirm count，看反应慢就调大 gain。

### 球 / 对手判别

| 参数 | 占位值 | 来源 | 步骤 | |
|---|---|---|---|---|
| `CAL_OPPONENT_DIFF_MM` | 90 | ntzeho 9cm / bentjh01 8cm | M4 | 🔴 |
| `CAL_UPPER_PRESENT_MM` | 600 | 估计 | M4 | 🔴 |
| `CAL_BALL_DETECT_MAX_MM` | 500 | 往届 28–60cm 差异很大 | M2/M4 | 🔴 |
| `CAL_BALL_GRAB_MM` | 120 | 估计，取决于铲子几何 | M2 | 🔴 |
| `CAL_HYSTERESIS_MM` | 60 | 无往届数据 | M4 | 🟡 |
| `CAL_SHARP_HALF_CONE_DEG` | 8 | **往届无人测过** | M3 | 🟡 |

> `CAL_SHARP_HALF_CONE_DEG` 决定搜索扫描的步进角上限（应 ≤ 2 倍此值），
> 定错了会在两次扫描之间留盲区漏球。这个数要给到写搜索策略的人。

### 线阵

| 参数 | 占位值 | 步骤 | |
|---|---|---|---|
| `CAL_LINE_DIGITAL_MODE` | 1（数字） | M5 | 🔴 拿到模块看有无电位器 |
| `CAL_LINE_ON_LEVEL` | 0（低有效） | M5 | 🟡 5 个仓库独立确认，置信度高 |
| `CAL_LINE_THRESH_*` ×4 | bentjh01 实测的四个不同中点 | M5 | 🔴 仅模拟模式需要 |
| `CAL_LINE_CONFIRM_COUNT` | 2 | M5 | 🟡 |

> ⚠️ **线阵阈值要测两次。** 实验室用器材单发的黑卡纸标定，比赛场地是黄色反光胶带，反射率不同。进场后必须复测。

### 限位开关

| 参数 | 占位值 | 步骤 | |
|---|---|---|---|
| `CAL_SWITCH_DIGITAL_PRESSED_LEVEL` | 0 | M6 | 🔴 `sensorTouch` 与 `sensorDigitalIn` 极性相反 |
| `CAL_SWITCH_ANALOG_PRESSED_MAX` | 2000（中点） | M6 | 🟡 裕度很大 |
| `CAL_SWITCH_CONFIRM_COUNT` | 2 | M6 | 🟡 |

### 罗盘

| 参数 | 占位值 | 步骤 | |
|---|---|---|---|
| `CAL_COMPASS_ACTIVE_LOW` | 1 | M7 | 🟡 5 个仓库一致 |
| `CAL_COMPASS_DRIVE_SUPPLY` | 1 | M7 | 🟡 往届做法不一，保守起见拉高 |
| `CAL_COMPASS_CONFIRM_COUNT` | 2 | M7 | 🟡 |

> **不需要标定"编码→方位"查表** —— 解码是结构式的，与接线顺序无关。
> M7 只需确认四个引脚各自对应哪个方向。

### 编码器

| 参数 | 占位值 | 步骤 | |
|---|---|---|---|
| `CAL_ENC_TICKS_PER_M` | 1641（理论值：360 counts/圈 ÷ 219.4mm 周长） | M8 | 🔴 |
| `CAL_ENC_TRUSTWORTHY` | 0（默认不信） | M8 | 🔴 确认轮型后定 |

---

## 优先级：如果只有一小时

1. 填 `robot_config.h` 端口块（不填什么都跑不了）
2. **M2 Sharp 距离标定** —— 影响面最大，也最花时间
3. **M5 线阵阈值** —— 不对就会冲出界
4. M6 限位开关极性 —— 很快，五分钟
5. 其余按 🟡 先用占位值跑起来，边跑边调

## 调完之后

- 把 `sensor_cal.h` 里的 `PROVISIONAL` 标记删掉
- 原始数据贴进 logbook，另存一份电子表 → 报告 checklist 第 4 项 `Design Calculations` 直接用
- 画一张 `raw ADC vs 距离` 散点+拟合曲线图 → 报告里最容易得分的一张图
