/*==============================================================================
  sensor_cal.h  —  全部标定常数集中于此

  规则：sensors.c 里不允许出现任何魔数。要调数字，只改这一个文件。

  每项都标了 PROVISIONAL 和对应的实测步骤编号（M1..M8，见
  CALIBRATION_PROTOCOL.md）。实测完成后把数值替换掉并删除 PROVISIONAL 标记。

  占位值的来源：往届 MA4012 仓库的实测结果（逐条注明）。它们是别人的传感器
  在别人的安装高度上测的，只能当量级参考，不能当我们的标定值。
==============================================================================*/

#ifndef SENSOR_CAL_H
#define SENSOR_CAL_H

/*==============================================================================
  1. ADC 与采样

  注：本节四个常量（CAL_ADC_MAX / CAL_ADC_VOLT_SCALE / CAL_UPDATE_PERIOD_MS）
  与第 6 节的 CAL_LINE_HEIGHT_MM 是**参考值**，sensors.c 不引用它们。
  它们记录硬件事实与安装参数，供报告、复装和换算时对照。
==============================================================================*/

/* PROVISIONAL [M1] Cortex 为 12-bit ADC。ntzeho 用 5.0/4096 换算电压，
   佐证量程为 0..4095。开机跑 cal_harness 确认。 */
#define CAL_ADC_MAX              4095
#define CAL_ADC_VOLT_SCALE       (5.0 / 4096.0)

/* 采样周期。sensUpdate() 由主循环调用，这里只用于滤波时间常数换算。 */
#define CAL_UPDATE_PERIOD_MS     20

/*==============================================================================
  2. Sharp 测距 —— 原始值到距离

  确认型号（来自 bentjh01/MA4012_Ball_Fondlers 仓库内附的原厂 datasheet）：
    10-80cm = GP2Y0A21YK0F
    4-30cm  = GP2Y0A41SK0F

  两种可用的反演形式，往届都验证过：
    (a) 双曲型  d = K / (raw + C)          ← 简单，先用这个
    (b) 幂律型  d = A * V^B, V = raw*5/4096 ← 更贴合 datasheet 曲线

  我们采用 (a)，因为标定时只需拟合两个参数，M2 步骤的数据量够用。
==============================================================================*/

/* PROVISIONAL [M2] 长距 10-80cm。
   占位值来自 JadeHouseDisco/MA4012 main.c:176  d = 24339/(raw+149.1)
   三个长距传感器个体差异可能不小，M2 要求逐个标定，标完这里拆成三组。 */
#define CAL_SHARP_LONG_K         24339.0
#define CAL_SHARP_LONG_C         149.1

/* PROVISIONAL [M2] 短距 4-30cm。往届没有可直接借用的短距拟合，
   这里按量程比例先给一个占位，M2 必须实测覆盖。 */
#define CAL_SHARP_SHORT_K        9200.0
#define CAL_SHARP_SHORT_C        100.0

/* PROVISIONAL [M2] 可信量程 (mm)。超出即判为无效，不返回猜测值。
   上限取「标准差开始明显变大」的那个距离，通常远小于标称 80cm。 */
#define CAL_SHARP_LONG_MIN_MM    100
#define CAL_SHARP_LONG_MAX_MM    700
#define CAL_SHARP_SHORT_MIN_MM   40
#define CAL_SHARP_SHORT_MAX_MM   280

/* PROVISIONAL [M2] 折返保护。
   Sharp 在近于最小量程处特性曲线折返：5cm 与 35cm 读数可能相同。
   原始值高于此阈值 = 进入折返区，长距传感器的距离读数不可信，
   必须靠短距传感器交叉判定。
   ⚠️ 往届 12 个仓库没有一个处理这个问题，mehhh2u 的 <300 钳位甚至
      把 4cm 的物体报成 92cm（"前方无物"）。这是我们要补的最大缺口。 */
#define CAL_SHARP_LONG_FOLDBACK_RAW   2600

/* PROVISIONAL [M1] 无目标时的静息读数。低于此值视为「前方空旷」。 */
#define CAL_SHARP_IDLE_RAW       250

/*==============================================================================
  3. Sharp 滤波

  组合：中值滤波(3) 抑制脉冲噪声 → 一阶 IIR 低通 平滑。
  来源：bentjh01/MA4012_Ball_Fondlers roboC_ws/components/sensors.c
        （唯一做了滤波的往届仓库，其余 11 个全是裸读单次采样）
==============================================================================*/

/* PROVISIONAL [M2] IIR 增益。0=完全不更新，1=不滤波。
   bentjh01 长距用 0.5，短距用 1.0（短距不滤波，因为要快速响应）。 */
#define CAL_FILTER_GAIN_LONG     0.50
#define CAL_FILTER_GAIN_SHORT    1.00

/* PROVISIONAL [M2] N 次连续在范围内才认定有效，否则输出越界哨兵值。
   来源：ntzeho_MA4012_avengers sensor_detection.c:95（FRONT_DIST_CORRECT_VALUE=3）
   这条是往届最有效的抗误触发手段。 */
#define CAL_DETECT_CONFIRM_COUNT 3

/*==============================================================================
  4. 球 / 对手判别

  原理：上下叠放两个 Sharp。下方对准球高度，上方高于球只能看到对手。
    下有 + 上无           → 球
    下有 + 上有且距离相近  → 对手（高物体）
    下有 + 上有但差值大    → 球在对手前方

  这套判据在 5 个往届仓库中独立出现，是本课程的标准解法。
  采用比较式（两个传感器互比）而非绝对阈值，因为比较式不受标定漂移影响。
  来源：isselin28/MA4012 Functionsall.c:82（±100 counts 比较式，最早）
        ntzeho robot_state.c:146（改用标定后的 cm 比较，更好）
==============================================================================*/

/* PROVISIONAL [M4] 上下两传感器距离差小于此值 → 判为同一个高物体 = 对手。
   往届参考：ntzeho ROBOT_SENSOR_DIFF_THRESHOLD = 9 cm
             bentjh01 OPP_DIFFERENTIATION_THRESHOLD = 8.0 cm */
#define CAL_OPPONENT_DIFF_MM     90

/* PROVISIONAL [M4] 上方传感器判定「有高物体」的最远距离。
   超过这个距离就认为上方没东西。 */
#define CAL_UPPER_PRESENT_MM     600

/* PROVISIONAL [M2/M4] 认定「看见球」的最远距离。
   往届参考：ntzeho 55cm / bentjh01 38cm(左右) 28cm(中) / mehhh2u 60cm
   注意这些差异很大，取决于各组传感器安装角度，必须自己测。 */
#define CAL_BALL_DETECT_MAX_MM   500

/* PROVISIONAL [M2] 球进入收集器可动作范围的距离（短距传感器判定）。 */
#define CAL_BALL_GRAB_MM         120

/* PROVISIONAL [M4] 迟滞。检测阈值与释放阈值分开，避免目标在阈值附近抖动时
   状态反复翻转。往届 15 个仓库里只有 Khoo395 做了迟滞
   （avoid_front_opponent.c:2-3 触发 1100 / 释放 900），其余全部单阈值。
   释放距离 = 检测距离 + 此值。 */
#define CAL_HYSTERESIS_MM        60

/*==============================================================================
  5. Sharp 有效检测锥角
==============================================================================*/

/* PROVISIONAL [M3] 半锥角（度）。搜索扫描步进角应 ≤ 2 倍此值，
   否则两次扫描之间存在盲区会漏球。往届无人测过这个数。 */
#define CAL_SHARP_HALF_CONE_DEG  8

/*==============================================================================
  6. IR 线阵 (TCRT5000)

  型号确认：bentjh01 仓库内附 "IR Line Tracking Module tcrt5000 datasheet.pdf"

  模块有两种用法，两种往届都有人用：
    数字模式：板载比较器输出 0/1，阈值靠模块上的电位器调（11/12 个仓库这么用）
    模拟模式：读原始值，阈值在代码里调（bentjh01 这么用，逐个传感器测中点）

  数字模式占 digital 口，模拟模式占 analog 口 —— 直接影响端口预算，
  所以这个开关必须在接线前定。
==============================================================================*/

/* [M5] 1 = 数字模式（推荐，省 analog 口且往届主流）；0 = 模拟模式 */
#define CAL_LINE_DIGITAL_MODE    1

/* 数字模式下，哪个电平代表「压在线上」。
   往届 5 个仓库独立确认为 0（低有效）：
     penghengx competition.c:195 / mehhh2u :445 / samruddhi13 Test1.c:70 注释
     "black is 1 yellow is 0" / isselin28 Functionsall.c:6 同样注释 / lucvt001 main.c:93
   这一条置信度很高，但仍建议 M5 现场确认一次。 */
#define CAL_LINE_ON_LEVEL        0

/* PROVISIONAL [M5] 模拟模式下的阈值（黑与黄反光带读数的中点），逐个传感器测。
   占位值来自 bentjh01 config.h（他们实测的四个中点，各不相同 —— 这正说明
   必须逐个测，不能共用一个阈值）。 */
#define CAL_LINE_THRESH_FL       1252
#define CAL_LINE_THRESH_FR       1320
#define CAL_LINE_THRESH_RL       1147
#define CAL_LINE_THRESH_RR       1115

/* PROVISIONAL [M5] 去抖：连续 N 次读到压线才确认。
   来源：mehhh2u 用 50ms 重读确认（往届唯一做去抖的）。
   我们改成计数式，避免引入阻塞等待。 */
#define CAL_LINE_CONFIRM_COUNT   2

/* PROVISIONAL [M5] 模块离地高度 (mm)。TCRT5000 对安装高度极敏感。
   此值仅作记录，供报告和复装时对照。 */
#define CAL_LINE_HEIGHT_MM       10

/*==============================================================================
  7. 限位开关
==============================================================================*/

/* 数字口上：哪个电平代表按下。
   注意 sensorTouch 与 sensorDigitalIn 极性可能相反，M6 现场确认。
   往届：ntzeho 用 sensorTouch 读 ==1 为按下；
         lucvt001/mehhh2u 用 sensorDigitalIn 读 ==0 为按下。 */
#define CAL_SWITCH_DIGITAL_PRESSED_LEVEL  0

/* PROVISIONAL [M6] 模拟口上读机械开关的阈值。低于此值 = 按下。
   在 analog 口读机械开关是往届验证过的做法：
     Khoo395 sensor_output.h:80  in5 dispense_limit_switch, sensorAnalog
     penghengx competition.c:752 in6/in7/in8 三个 limit 全在 analog
   VEX 限位开关组件自带上拉，松开时拉到接近满量程，按下时硬拉到 0，
   所以取中点 2000 有最大噪声裕度。
   ⚠️ 两届用的都是 == 0 / != 0 精确比较，在 12 位 ADC 上只要 1 个 LSB
      噪声机器人就永远不启动。这里改用阈值。 */
#define CAL_SWITCH_ANALOG_PRESSED_MAX     2000

/* PROVISIONAL [M6] 去抖计数 */
#define CAL_SWITCH_CONFIRM_COUNT 2

/*==============================================================================
  8. Digital Compass 1490

  硬件形态：4 路数字输出 + 1 路供电引脚。不是 I2C。
  12 个往届仓库全部这样接，无一例外。

  解码：低有效。一位为 0 = 该正方向；相邻两位为 0 = 两者之间的斜方向。
  经 5 个仓库交叉确认（penghengx / mehhh2u / samruddhi13 / ntzeho /
  Davidlequnchen，其中后两个的 read_orientation() 与 2018 年课程范例逐字相同）。

  ⚠️ 各届的「编码→方位」查表并不一致（Rzi98 的表就和 penghengx 完全不同），
     因为那取决于哪根线插哪个口。所以 sensors.c 里不用查表，而是按
     "几位为低 + 哪几位相邻" 的结构解码，与接线顺序无关。
     只需 M7 确认 4 个引脚各自对应哪个方向即可。
==============================================================================*/

/* [M7] 1 = 罗盘引脚低有效（往届一致结论）。若 M7 实测相反改为 0。 */
#define CAL_COMPASS_ACTIVE_LOW   1

/* [M7] 罗盘供电引脚是否需要主动拉高。
   往届做法不一：Davidlequnchen 声明 sensorDigitalOut 但从不写（即恒为低）；
   Khoo395 把它放在 analog 口声明 sensorNone；mehhh2u 干脆不声明也能用。
   保守起见默认拉高，M7 若发现不需要则改为 0。 */
#define CAL_COMPASS_DRIVE_SUPPLY 1

/* PROVISIONAL [M7] 罗盘读数确认次数。转动过程中会经过非法编码，
   加确认可避免瞬时误判。 */
#define CAL_COMPASS_CONFIRM_COUNT 2

/*==============================================================================
  9. 编码器

  ⚠️ 往届 15 个仓库中，只有 4 个装了编码器，且没有一个用于底盘里程 ——
     全部用于机构限位（如 penghengx 的收集门）。所有导航都是
     「罗盘定向 + 定时器估距」的开环方式。
     所以这部分没有可借鉴的先例，我们要么自己做，要么跟随往届不做。
==============================================================================*/

/* PROVISIONAL [M8] 每米脉冲数。手推 1.00m 实测三次取平均。
   VEX 正交编码器标称 360 counts/圈，配 2.75in(69.85mm) 轮：
   周长 219.4mm → 约 1641 counts/m（仅为理论值，必须实测）。 */
#define CAL_ENC_TICKS_PER_M      1641

/* PROVISIONAL [M8] 里程可信度。若为全向轮，横向滚子会让推算持续漂移。
   1 = 里程可用于闭环；0 = 只作粗略参考，导航以罗盘为主。
   拿到实物确认轮型后再定。 */
#define CAL_ENC_TRUSTWORTHY      0

#endif /* SENSOR_CAL_H */
