# SENSOR SCOPE — MA4012 传感器层职责界定

负责人：Yiheng · 平台：VEX Cortex + ROBOTC · 建立：2026-09-07
依据：器材借用单 · `MA4012 Assignment Document Sem 1 26-27.pdf` · 12 份往届同课仓库的 `#pragma config`

---

## 1. 可用 sensors（器材借用单第 13–19 项）

| # | 器件 | 数量 | 电气接口 | 端口占用 |
|---|---|---|---|---|
| 13 | Sharp Distance Sensor (10–80 cm) | 3 | 模拟电压 | 1 analog 每个 |
| 14 | Sharp Distance Sensor (4–30 cm) | 1 | 模拟电压 | 1 analog |
| 15 | IR Line Tracking Module | 4 | **数字**（板载比较器输出） | 1 digital 每个 |
| 17 | Limit Switch | 4 | 机械触点 | 1 digital **或** 1 analog |
| 19 | Vex Shaft Encoder | 2 | 正交 A/B | **2 digital 每个** |
| 18 | Digital Compass 1490 | 1 | **4 路数字输出 + 1 路供电** | 5 端口（供电可放 analog） |

### 两处与我最初假设不同的地方（依 12 份往届代码修正）

**① Digital Compass 1490 不是 I2C，是 4 位数字并口。**
12 份仓库无一例外把它接成 4 个 `sensorDigitalIn`（N/E/S/W）加 1 个供电引脚，解码成 8 个方位。Cortex 的 I2C 口在这门课里根本没被用过。

**② IR Line Tracking Module 是数字输出，不是模拟。**
几乎所有往届代码都声明为 `sensorDigitalIn`，读到 `0` 表示压到线上。模块自带比较器和调节电位器，阈值在硬件上调，不在代码里调。
（例外：`SKEW002_MiroMyro` 同时存在 `sensorReflection` 模拟版本，说明也可以当模拟用。**拿到实物后需确认板上有无电位器**。）

**这两处修正把 analog 口从「8/8 占满」变成「4/8，余 4」，反而是 digital 口变成瓶颈。**

---

## 2. 端口预算 —— 有硬冲突，必须取舍

VEX Cortex：**8 路 analog · 12 路 digital**

**全部器材同时上机的需求：**

| 资源 | 需求 | 明细 |
|---|---|---|
| analog | 4 | 4 × Sharp |
| digital | **17** | 4 线阵 + 4 罗盘 + 1 罗盘供电 + 4 编码器(2×2) + 4 限位 |

**digital 超支 5 路。**

### 往届证实可行的两个缓解手段

1. **罗盘供电放到 analog 口**，声明为 `sensorNone`（见 `Khoo395_Tennis_Ball_Collector`：`in8, compass_power, sensorNone`）→ 省 1 digital
2. **限位开关放到 analog 口**读（见 `Khoo395`：`in5, dispense_limit_switch, sensorAnalog`；`penghengx`：`in6/in7/in8` 三个 limit 全在 analog）→ 每个省 1 digital

### 算完之后的结论

设限位开关 L 个、编码器 E 个，罗盘供电占 analog（S=1）：

- analog：`4 + L_analog + 1 ≤ 8` → `L_analog ≤ 3`
- digital：`4 + 4 + 2E + L_digital ≤ 12` → `2E + L_digital ≤ 4`

**能同时满足的只有这两种组合：**

| 方案 | 限位开关 | 编码器 | 说明 |
|---|---|---|---|
| **A** | 4 个（3 在 analog + 1 在 digital） | **1 个** | 牺牲一个编码器 |
| **B** | **3 个**（全在 analog） | 2 个 | 牺牲一个限位开关 |

**做不到「4 限位 + 2 编码器」。这是硬约束，不是可以协商的。**

**参考往届的选择**：12 份仓库里只有 4 份用了编码器，其余全部靠「罗盘定向 + 定时器估距」导航。也就是说**方案 A（保限位、只留 1 个编码器）更符合这门课的主流做法**，因为差速转向 + 全向轮的里程推算本来就不准，编码器的边际价值有限。

> ⚠️ 这个取舍需要组内拍板，因为它同时影响机构设计（几个限位开关可用）和导航策略（要不要依赖里程）。

---

## 3. 每个 sensor 的用途

### Sharp 测距 ×4 —— 搜索、测距、以及区分球和对手

往届公认的做法是**两个 Sharp 上下叠放**：

- **下方**传感器高度对准网球
- **上方**传感器高于网球、只能看到对手机器人

判据：
- 下有、上无 → **是球**
- 下有、上有 → **是对手机器人**，避让

这直接解决了作业里「两台车同场」的核心难题。`penghengx` 直接把两个口命名为 `Ball_Sensor` 和 `Robot_Sensor`，`JadeHouseDisco` 命名为 `enemyDistanceSensor`。

剩余分配：1 个做侧向/斜前扫描，1 个 4–30 cm 短距做**近场确认**（解 Sharp 折返歧义）。

### IR 线阵 ×4 —— 边缘检测

前左 / 前右 / 后左 / 后右。场地边界是黄色反光胶带，读到 `0` 表示压线。
**必须四个角都有**，因为组内已定「倒车回箱不掉头」的策略，后方边缘检测不能省。

### 限位开关 —— 机构状态与碰撞

- 球到位检测（**替代原本想用第 5 个 IR 的方案**，因为 IR 只有 4 个且全部用于边缘）
- 铲子上/下限位
- 前保险杠碰撞（对手接触检测）
- 往届几乎都还额外做一个**启停开关**（我们器材单里没有专门的，需从这 4 个里挪用或另想办法）

### Shaft Encoder —— 里程推算

数轮子转数估算行进距离。**注意**：如果实物是全向轮（CAD 里填的是 `2.75in Omni-Directional`，但器材单未注明），横向漂移会让里程推算持续跑偏，编码器价值大打折扣。**拿到实物先确认轮型。**

### Digital Compass 1490 —— 航向

4 位数字输出 → 8 个方位（N/NE/E/SE/S/SW/W/NW）。分辨率只有 45°，只够做「大致朝向投递区」这类粗导航，不能用于精确对准。**器材单标注「will be issued later」，目前未发放**，接口留好、缺席时降级返回。

---

## 4. 我的 scope 边界

**在内**：传感器初始化 · 采样与滤波 · 原始值到物理量的标定转换 · 语义判定（有球吗/球抓到了吗/压线了吗/朝哪）· 接口导出 · 标定夹具程序

**在外**：电机驱动 · 差速运动控制 · 收集器机构动作 · 上层比赛策略状态机 · CAD · 报告主体

**边界原则**：我的代码**只读传感器，不碰电机**。往届代码最大的通病就是把两者搅在一起——例如 `josephinemonica` 的 `line_detection()` 既读线阵又直接驱动轮子，导致完全无法复用。

---

## 5. 我向上层提供什么

```c
void  sensInit(void);
void  sensUpdate(void);                  // 每主循环调用一次，非阻塞

int   sensDistMm(tDistSensor s);         // -1 = 无效
bool  sensDistValid(tDistSensor s);

bool  sensBallSeen(int *bearingDeg, int *rangeMm);
bool  sensBallHeld(void);
bool  sensOpponentSeen(void);            // 上下叠放 Sharp 的判据

bool  sensEdge(tEdgePos p);
int   sensEdgeMask(void);                // 位掩码 FL/FR/RL/RR

bool  sensScoopAtTop(void);
bool  sensScoopAtBottom(void);
bool  sensBumperHit(void);

long  sensEncTicks(tSide s);
int   sensHeadingDeg(void);              // 罗盘未发放前返回 SENS_NO_HEADING
```

上层（navigation / collector / 主控制）**只调用这些函数，不直接读 `SensorValue[]`**。这样标定结果怎么变、端口怎么改，上层都不用动。

---

## 6. 待实物确认的三件事

| # | 问题 | 为什么重要 |
|---|---|---|
| 1 | IR Line Tracking Module 是数字输出还是模拟输出？板上有无调节电位器？ | 决定占 digital 还是 analog，直接影响端口预算 |
| 2 | 轮子是全向轮还是普通胶轮？ | 决定编码器里程推算是否可用 |
| 3 | Digital Compass 1490 什么时候发放？ | 未发放前航向功能只能空实现 |
