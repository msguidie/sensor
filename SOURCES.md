# SOURCES — 代码来源与修改记录

调研范围：GitHub 搜索 `MA4012` 命中 **20 个仓库，全部是 NTU 同课同任务**（网球收集竞赛机器人）。
全部为 **ROBOTC + VEX Cortex**，无一为 V5 / VEXcode，硬件匹配这一关全部通过。
实际精读 **15 个仓库、约 22,000 行 ROBOTC**（12 个由子代理分工精读，3 个由主代理亲读）。
旧课号 `MP4010` 在 GitHub 上无命中。

---

## 采纳明细

| 我们的代码 | 来源 | 我做了什么修改 |
|---|---|---|
| `sensors.c` `updateOneDistance()` 的**中值(3)+一阶IIR**滤波 | `bentjh01/MA4012_Ball_Fondlers` `roboC_ws/components/sensors.c` | 原样思路。往届 15 个仓库里唯一做了滤波的，其余全是裸读单次采样 |
| `updateOneDistance()` 的 **N 次连续确认**门限 | `ntzeho/MA4012_avengers` `sensor_detection.c:95` | 原样思路，常数提到 `sensor_cal.h` |
| `rawToMm()` 的**双曲反演** `d = K/(raw+C)` | `JadeHouseDisco/MA4012` `main.c:176`（`d=24339/(raw+149.1)`） | 改为 mm；加零除保护；长短距分用各自系数；不再把结果写回原始值变量（原代码是破坏性覆盖） |
| `updateTargets()` 的**球/对手比较式判别** | `isselin28/MA4012` `Functionsall.c:82`（±100 counts）<br>`ntzeho` `robot_state.c:146`（改用标定后 cm） | 改用 mm；**加迟滞**；上方传感器无效时明确按"无高物体"处理，而不是沿用旧值 |
| 上下叠放 Sharp 的**布置方式** | `Khoo395/Tennis_Ball_Collector` `sensor_output.h:182` | 概念采纳 |
| `updateEdges()` 的 **4 位线阵掩码** | `ntzeho/MA4012_avengers` `sensor_detection.c:124` | 原样思路；**去抖从"50ms 重读"改为计数式**（原做法会阻塞） |
| 线阵**数字/模拟双模式**支持 | `Khoo395` 同一台车上 `in6 sensorAnalog`(`<800`) 与 `dgtl5 sensorDigitalIn`(`==0`) 并存 | 做成 `CAL_LINE_DIGITAL_MODE` 编译期开关 |
| `switchRawPressed()` 的 **analog 口读机械开关** | `Khoo395` `sensor_output.h:80`<br>`penghengx` `competition.c:752` | **把 `!=0` / `==0` 精确比较改为阈值比较**。12 位 ADC 上 1 个 LSB 噪声就会让原代码永远不触发 |
| `compassDecodeDeg()` 的 **4 位低有效**结论 | `penghengx` `competition.c:139`<br>`ntzeho` `sensor_detection.c:40`<br>`Davidlequnchen` `competition_v3.c:306`<br>（三者逐字相同，且与 2018 年课程范例一致） | **改为结构解码而非查表**（见下）；**加 hold-last-good**；加连续确认 |
| 罗盘**抖动需多次确认** | `Khoo395` `spin_search.c:4` 注释 `"+2 is used because of the shaking compass"` | 做成 `CAL_COMPASS_CONFIRM_COUNT` |
| 罗盘**供电引脚可占 analog 口** | `Khoo395` `main.c:7` `in8, compass_power, sensorNone` | 写进 `PORT_MAP.md` 作为省 digital 口的手段 |
| `sensEncTicks()` 的**快照差值法** | `PatrickPetch/tennisbot` `competition.c:449` | 采纳。避免"清零再读"与中断计数的竞争 |
| 里程换算 | `PatrickPetch` `competition.c:460`（`arc = radians(ticks)*wheel_radius`） | **把隐含的 360 ticks/rev 提成具名常数** `CAL_ENC_TICKS_PER_M`。原代码通过 `degToRad()` 把这个假设藏在实现里，换编码器会静默出错 |
| 迟滞 | `Khoo395` `avoid_front_opponent.c:2-3`（触发 1100 / 释放 900） | 推广到球检测，做成 `CAL_HYSTERESIS_MM` |
| Sharp 型号确认 | `bentjh01` 仓库内附原厂 datasheet | 10-80cm = **GP2Y0A21YK0F**；4-30cm = **GP2Y0A41SK0F**；循线模块 = **TCRT5000** |

## 一处我们做得比所有往届都好的地方

**罗盘按结构解码，与接线顺序无关。**

各届的"编码→方位"查表互不相同（`Rzi98/FIVES-champions` `src/compass.h` 的表与 `penghengx` 完全不同），因为那取决于哪根线插哪个口。但结构是一致的：**一位为低 = 该正方向，相邻两位为低 = 两者之间的斜方向**。

交叉验证：把往届编码按 N/E/S/W 排序是 `14,12,13,9,11,3,7,6` —— **相邻方位恰好只差一位，是一个循环 4 位格雷码**。这既佐证了上述结构，也说明转动时在扇区交界处读到的仍是合法编码，不会出现野值。

所以 `sensors.c` 不查表，只需 M7 确认四个引脚各对应哪个方向即可。

## 四件往届普遍做错、我们明确避开的事

1. **传感器层直接驱动电机** —— `josephinemonica` 的 `line_detection()`、`Davidlequnchen` 的 `checking_reflective_sensor()` 都是既读传感器又写 `motor[]`，完全无法复用。我们的 `sensors.c` 零 `motor[]` 写入（已静态核验）。
2. **阻塞等待冻结全部感知** —— 往届大量 `while(条件){}` 空转和 `wait1Msec` 混在感知路径里。`JadeHouseDisco` 的 `reorient()` 有两个无界 `while(true)`，罗盘停在相邻扇区就永远转下去。我们的 `sensUpdate()` 无任何等待循环。
3. **传感器任务不让出 CPU** —— `abortTimeslice()` 在全部 15 个仓库中出现 **0 次**。多个 `while(true)` 任务全速空转互相饿死。
4. **Sharp 近距折返完全没人处理** —— 15 个仓库无一提及。`mehhh2u` 的 `<300` 钳位甚至把 4cm 的物体报成 92cm（"前方空旷"），恰好在要抓球的那一刻误判。我们用短距传感器交叉判定并返回"无效"而不是错误数值。

## 未采纳且明确剔除的

| 来源 | 剔除理由 |
|---|---|
| `PatrickPetch` 的 16 段手写分段线性表 | 不可维护，且 `LSH_F` 表里有两段不可达且 min/max 反了的死分支 |
| `Khoo395` / `bentjh01` 的罗盘解码函数 | 都有**八进制字面量 bug**：`case 0011:` 在 C 里是八进制 = 十进制 9，导致三个方位永远不可达 |
| `SKEW002` 的 Sharp 平均 | `for(i=0; i<=N; i++)` 循环 6 次却除以 5，所有距离系统性偏大 20%；且采样间无延时，等于重复读同一次转换 |
| `samruddhi13` / `isselin28` 的全部代码 | 两个仓库都**不能编译**（缺分号、`task main()` 无函数体、`while(i = 1)` 赋值当比较） |
| 各届的具体阈值常数 | 都是各组自己传感器在自己安装高度上测的，只能当量级参考。我们全部标 `PROVISIONAL` 并给出实测步骤 |

## 报告 checklist 第 8 项披露段（草稿）

> **Declaration of AI tools and external code**
>
> This project's sensor layer was developed with the assistance of an AI coding assistant (Claude), used for: surveying publicly available prior-year MA4012 repositories on GitHub, comparing their sensor implementations, and drafting the integrated sensor abstraction layer.
>
> The sensor layer additionally incorporates ideas and, in places, adapted code from publicly available GitHub repositories of prior MA4012 cohorts. Every borrowed element is annotated in-source with a `/* SRC: <repository> <file>:<line> */` comment and catalogued in `SOURCES.md`, together with the modifications made. The principal borrowings are: the median-plus-low-pass filter structure, the N-consecutive-reading confirmation gate, the hyperbolic Sharp linearisation form, the four-bit line-sensor bitmask, and the four-pin active-low decoding of the Digital Compass 1490.
>
> What this taught us: the survey showed that the compass decoding is a four-bit cyclic Gray code, which let us write a decoder that is independent of wiring order rather than copying any one cohort's lookup table; and that no prior cohort handled the Sharp sensor's sub-10 cm fold-back, which we addressed with short-range cross-checking and an explicit validity flag.
