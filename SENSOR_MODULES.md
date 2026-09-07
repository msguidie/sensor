# SENSOR MODULES — 每个模块负责什么

## 文件职责

| 文件 | 职责 | 谁改它 |
|---|---|---|
| `robot_config.h` | 端口分配 + 机械常数 | 组内定端口后填一次 |
| `sensor_cal.h` | **全部标定常数**。`sensors.c` 里不允许出现魔数 | 实测后只改这一个 |
| `sensors.h` | 对外接口契约 | 基本不动，动了要通知上层 |
| `sensors.c` | 实现。**零 `motor[]` 写入** | 我 |
| `cal_harness.c` | 独立标定夹具程序，不参与比赛代码 | 我 |

## 上层怎么用

```c
sensInit();
while (true) {
    sensUpdate();          // 每循环一次，非阻塞
    /* ...用下面的查询函数决策... */
}
```

**上层不要直接读 `SensorValue[]`。** 端口改了、标定变了，只要接口不变，上层一行都不用动。

## 六个传感器模块

### 1. Sharp 测距 ×4（模拟）

| 传感器 | 位置 | 干什么 |
|---|---|---|
| `DIST_BALL` | 前·下，对准球高度 | 主搜索 |
| `DIST_UPPER` | 前·上，高于球 | 只看得到对手 → 用于区分球和对手 |
| `DIST_SIDE` | 斜前侧向 | 扫描、粗略方位 |
| `DIST_NEAR` | 铲口，4–30cm 型号 | 近场确认、解折返歧义 |

处理链：原始值 → 中值(3) → 一阶IIR → 双曲反演 → 折返保护 → N次连续确认 → 发布。

```c
int  sensDistMm(tDistSensor s);     // -1 = 无效
bool sensDistValid(tDistSensor s);
int  sensDistRaw(tDistSensor s);    // 调试用
```

**读不准时返回 `-1`，不返回猜测值。** 上层必须处理这个返回值。

### 2. 球 / 对手判别

两个上下叠放的 Sharp 互相比较，而不是各自比绝对阈值 —— 这样不受标定漂移影响。

```
下有 + 上无            → 球
下有 + 上有且距离相近   → 对手
下有 + 上有但差值大     → 球在对手前方，仍可去捡
```

```c
bool sensBallSeen(int *bearingDeg, int *rangeMm);  // 输出参数可传 NULL
bool sensOpponentSeen(void);
bool sensBallInGrabRange(void);                    // 近距 Sharp 判定，可以动铲子了
bool sensBallHeld(void);                           // 限位开关判定，球已在收集器里
```

带迟滞：检测和释放用不同阈值，避免目标在阈值附近时状态反复翻转。

### 3. IR 线阵 ×4（边缘检测）

四个角各一个，压到黄色反光带即触发。**一次读全，返回位掩码**：

```c
int  sensEdgeMask(void);   // EDGE_MASK_FL | FR | RL | RR
bool sensEdge(tEdgePos p);
```

上层建议直接对掩码做 `switch`，而不是逐个问。往届最好的一组（`ntzeho`）就是这么做的，还专门标注了 `0`（无）、`6`（FR|BL）、`9`（FL|BR）这几个对角组合物理上不可能，`15`（全中）多半是噪声或对手的诱饵线 —— 值得照抄这个防御性思路。

数字口/模拟口两种接法都支持，切 `sensor_cal.h` 里的 `CAL_LINE_DIGITAL_MODE` 即可。

### 4. 限位开关 ×4

```c
bool sensScoopAtTop(void);
bool sensScoopAtBottom(void);
bool sensBumperHit(void);
bool sensBallHeld(void);
```

**支持接在模拟口上**（往届验证过的做法），因为 digital 口不够用时这是唯一出路。每个开关接哪类口在 `robot_config.h` 里登记。

### 5. Digital Compass 1490

4 路数字输出（低有效）+ 1 路供电。**不是 I2C。**

```c
int  sensHeadingDeg(void);    // 0/45/.../315，无效返回 SENS_NO_HEADING
bool sensHeadingValid(void);
```

分辨率只有 45°，只够做"大致朝向投递区"这类粗导航，**不能用于精确对准**。

器材单标注"will be issued later"，**目前未发放**。未发放时 `sensHeadingValid()` 返回 false，上层必须能在没有航向的情况下工作。

罗盘在扇区边界会抖动（往届共同经验），所以要连续读到同一值才更新；读到非法编码时保持上一个有效值。

### 6. 编码器 ×1–2

```c
long sensEncTicks(tSide s);
long sensEncDistMm(tSide s);
void sensEncReset(void);
bool sensEncTrusted(void);   // 全向轮时为 false
```

用"快照差值"而非"清零再读"，避免与中断计数竞争。

⚠️ 往届 15 个仓库里**没有一个**把编码器用于底盘里程 —— 全部靠"罗盘定向 + 定时器估距"。如果轮子是全向轮，横向滑动会让推算持续漂移，`sensEncTrusted()` 会返回 false，上层应改以罗盘为主。

### 调试

```c
void sensDumpState(void);   // 把全部传感器状态打到 debug stream
```

调车时全组都会用到，建议在主循环里按需调用。

## 两条设计红线

1. **`sensors.c` 不写 `motor[]`。** 感知与动作分离。往届最普遍的通病就是把两者搅在一起，导致代码完全无法复用。
2. **`sensUpdate()` 内无任何等待循环。** Cortex 是协作式调度，感知层里一个 `while` 等待会同时冻结边缘检测，车会直接冲出界。
