# PORT MAP — 端口分配（待填）

> 组内定稿后，把「实际」列填上，然后同步抄进 `robot_config.h` 顶部的 TODO 块。
> 本表同时是报告 checklist 第 7 项 `Control Systems / Wiring Diagrams` 的底稿。

VEX Cortex 资源：**8 路 analog（in1–in8）· 12 路 digital（dgtl1–dgtl12）**

---

## ⚠️ 先做一个取舍决定

digital 口需求 17 路，实际只有 12 路。**必须二选一**（推导见 `SENSOR_SCOPE.md` 第 2 节）：

| | 限位开关 | 编码器 | 代价 |
|---|---|---|---|
| **方案 A（推荐）** | 4 个 | **1 个** | 放弃一个编码器。往届 12 份仓库里 8 份根本不用编码器 |
| **方案 B** | **3 个** | 2 个 | 放弃一个限位开关。若机构需要「上限位+下限位+球到位+保险杠」四个，则不可行 |

**选定方案：** ☐ A ☐ B ← 组内 Week 5/6 决定后勾选

---

## 建议分配（方案 A）

### Analog（8 路）

| 口 | 器件 | 用途 | 实际 |
|---|---|---|---|
| in1 | Sharp 10–80 | 前·下 —— 球检测（对准球高度） | |
| in2 | Sharp 10–80 | 前·上 —— 对手检测（高于球） | |
| in3 | Sharp 10–80 | 斜前侧向 —— 搜索扫描 | |
| in4 | Sharp 4–30 | 铲口近距 —— 解 Sharp 折返歧义 | |
| in5 | Limit Switch | 铲子上限位（analog 读，往届证实可行） | |
| in6 | Limit Switch | 铲子下限位 | |
| in7 | Limit Switch | 球到位检测 | |
| in8 | Digital Compass | 供电引脚（声明 `sensorNone`） | |

### Digital（12 路）

| 口 | 器件 | 用途 | 实际 |
|---|---|---|---|
| dgtl1 | IR Line | 边缘 前左 | |
| dgtl2 | IR Line | 边缘 前右 | |
| dgtl3 | IR Line | 边缘 后左 | |
| dgtl4 | IR Line | 边缘 后右 | |
| dgtl5 | Compass | North | |
| dgtl6 | Compass | East | |
| dgtl7 | Compass | South | |
| dgtl8 | Compass | West | |
| dgtl9 | Limit Switch | 前保险杠碰撞 | |
| dgtl10 | Shaft Encoder | 左轮 A（`sensorQuadEncoder` 占 dgtl10+11） | |
| dgtl11 | Shaft Encoder | 左轮 B | |
| dgtl12 | — | 预留（启停开关？） | |

> `sensorQuadEncoder` 在 ROBOTC 里声明在第一个口上，**自动占用紧邻的下一个口**。
> 例：`#pragma config(Sensor, dgtl10, encLeft, sensorQuadEncoder)` 会同时占掉 dgtl11。

---

## 电机口（不属于我的 scope，仅为完整性记录）

Cortex 有 8 路 3-wire（port2–9）+ 2 路 2-wire（port1、port10）。

| 口 | 器件 | 用途 | 实际 |
|---|---|---|---|
| port1 | 2-Wire Motor 393 | 左驱动轮 | |
| port10 | 2-Wire Motor 393 | 右驱动轮 | |
| port2 | Continuous Rotation Motor | 收集器 | |
| port3 | Continuous Rotation Motor | 备用 / 举升 | |

2-Wire Motor 393 接 port1/port10（内置 H 桥）可省掉 Motor Controller 29；
若要接 port2–9，则每个 393 需串一个 MC29（器材单给了 2 个）。

---

## 接线注意（往届踩过的坑）

1. **罗盘供电引脚**必须先拉高，罗盘才有输出。往届有的用 `sensorDigitalOut` 写 1，有的直接用 analog 口当电源。
2. **IR Line Module 的阈值在模块的电位器上调，不在代码里调。** 装好后要对着赛场的黄色反光带现场旋调。
3. **Sharp 传感器线序**：VEX 三线（信号/电源/地），插反会烧。
4. 器材单没有专门的启停开关，往届都从限位开关里挪一个来做。**我们 4 个限位开关已排满，需要额外想办法**（例如复用保险杠开关做长按启动）。
