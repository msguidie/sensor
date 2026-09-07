# PROGRESS REPORT — Sensor 部分进展

Yiheng · 2026-09-07 · MA4012 Week 5

> ⚠️ **本文只写已完成的事实。**
> 代码**未经任何硬件验证**：没有编译过、没有下载过、没有连过传感器。
> 端口分配未填，**当前状态下无法编译**（见下方"待组内决定"）。

---

## 做完了什么

**1. 调研了往届代码**
GitHub 上 `MA4012` 有 **20 个仓库，全部是我们同一门课、同一个捡球竞赛任务**，全部是 ROBOTC + VEX Cortex（没有 V5，硬件完全对得上）。精读了其中 **15 个、约 22,000 行**。来源与借用明细在 `SOURCES.md`。

**2. 写出了 sensor 层代码**

| 文件 | 内容 |
|---|---|
| `sensors.h` | 21 个函数的对外接口，上层据此编码 |
| `sensors.c` | 实现。零 `motor[]` 写入、零阻塞等待（已静态核验） |
| `sensor_cal.h` | 全部标定常数集中一处，24 项标了 `PROVISIONAL` |
| `robot_config.h` | 端口与机械常数（待填） |
| `cal_harness.c` | 独立标定夹具程序 |

**3. 写出了配套文档**
`SENSOR_SCOPE.md`（职责界定）· `PORT_MAP.md`（端口表 + 接线注意）· `CALIBRATION_PROTOCOL.md`（M1–M8 实测程序）· `TUNING.md`（待调参数清单）· `SOURCES.md`（来源）· `SENSOR_MODULES.md`（模块说明）

---

## 三个需要全组知道的发现

### ① digital 口不够用，必须做一个取舍

VEX Cortex 有 8 路 analog、12 路 digital。我们的器材全部上机需要 **17 路 digital**：

```
4 线阵 + 4 罗盘 + 1 罗盘供电 + 4 编码器(2个×2路) + 4 限位开关 = 17
```

**超支 5 路。** 把罗盘供电和部分限位开关挪到 analog 口（往届验证过的做法）之后，仍然只能二选一：

| | 限位开关 | 编码器 |
|---|---|---|
| **方案 A** | 4 个 | **1 个** |
| **方案 B** | **3 个** | 2 个 |

**做不到"4 限位 + 2 编码器"。**

参考：往届 15 个仓库里**没有一个**把编码器用于底盘里程，全部靠"罗盘定向 + 定时器估距"。所以方案 A（保限位）更符合这门课的实际做法。

**这个取舍同时影响机构设计（几个限位开关可用）和导航策略，需要组内拍板。**

### ② 两处硬件认知需要修正

- **Digital Compass 1490 不是 I2C**，是 4 路数字输出 + 1 路供电。12 个往届仓库无一例外。
- **IR 循线模块是数字输出**（板载比较器），不是模拟。这两点合起来把 analog 的压力转移到了 digital。

### ③ 有两件事必须拿实物确认

| 要确认 | 为什么 |
|---|---|
| **轮子是全向轮还是普通胶轮**（轮缘有没有一圈小滚子） | CAD 里填的是 `2.75in Omni-Directional`，但器材单只写 `Anti-Static Wheel`。如果是全向轮，横向滑动会让编码器里程持续漂移，回程导航方案要改 |
| **循线模块板上有没有调节电位器** | 有 = 数字输出占 digital 口；没有 = 模拟输出占 analog 口。直接影响上面那个取舍 |

---

## 上层可以开始写了

`sensors.h` 的接口已定稿，写 navigation / collector / 主控制的人现在就能按它编码，不用等我标定。标定结果只改 `sensor_cal.h` 一个文件，接口不变。

```c
sensInit();
while (true) {
    sensUpdate();                    // 非阻塞
    if (sensEdgeMask() != 0)     { /* 压线了 */ }
    if (sensOpponentSeen())      { /* 前面是对手不是球 */ }
    if (sensBallSeen(&brg, &rng)){ /* 看到球 */ }
    if (sensBallInGrabRange())   { /* 可以放铲子了 */ }
    if (sensBallHeld())          { /* 球到手了 */ }
}
```

**读不准时返回 `-1` / `SENS_NO_HEADING`，不返回猜测值 —— 上层必须处理这个返回值。**

罗盘器材单标注 "will be issued later"，尚未发放，所以 `sensHeadingValid()` 现在会返回 false。上层要能在没有航向的情况下工作。

---

## 待组内决定

1. **端口分配方案 A / B**（上面的取舍）→ 定了我填 `robot_config.h`，代码即可编译
2. 每个限位开关装在哪（铲子上限位 / 下限位 / 球到位 / 保险杠），机构组定
3. 四个 Sharp 的安装高度和角度 —— 上下叠放的两个必须**下方对准球高度、上方高于球**，这是区分球和对手的唯一手段

## 下一步（我）

1. 端口定了就填 `robot_config.h`，用 ROBOTC 的 Motors and Sensors Setup 重新生成 pragma，编译 `cal_harness.c`
2. 拿传感器按 `CALIBRATION_PROTOCOL.md` 做 M1–M8，回填 `sensor_cal.h`（预计 60–90 分钟，两人配合更快）
3. 标定原始数据整理进报告 checklist 第 4 项 `Design Calculations`
4. 出接线图 → 报告 checklist 第 7 项 `Control Systems / Wiring Diagrams`

**Week 6 的 Lab 议题正好是"传感器布置策略"，Week 7 是"传感器接线到 Vex + 软件策略"。上面这些正好对上。**
