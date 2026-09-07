/*==============================================================================
  sensors.c  —  传感器层实现

  硬性约束：本文件不含任何 motor[] 写入。传感器层只读，不动作。
  往届最普遍的通病就是把两者混在一起（例如 josephinemonica 的 line_detection()
  既读线阵又直接驱动轮子，导致完全无法复用）。

  非阻塞：sensUpdate() 内部没有任何 while 等待、没有 wait1Msec。
  Cortex 是协作式调度，传感器层里一个等待循环会同时冻结边缘检测。

  ⚠️ 未经硬件验证。robot_config.h 的端口宏填完之前无法编译。
     全部阈值来自 sensor_cal.h，实测前均为占位值。

  借用来源逐条标在函数上方的 "SRC:" 注释行里。
  （注意：C 的块注释不能嵌套，这里不能写出完整的注释定界符。）
==============================================================================*/

#include "robot_config.h"
#include "sensor_cal.h"
#include "sensors.h"

/*==============================================================================
  内部状态
==============================================================================*/

/* 开关索引，与 robot_config.h 的四个 PORT_SW_* 对应 */
#define SW_SCOOP_TOP     0
#define SW_SCOOP_BOTTOM  1
#define SW_BALL_PRESENT  2
#define SW_BUMPER        3
#define SW_COUNT         4

static int  sDistRaw[DIST_COUNT];        /* 本周期原始读数 */
static int  sDistMm[DIST_COUNT];         /* 滤波+标定后的距离，无效为 -1 */
static int  sDistFilt[DIST_COUNT];       /* IIR 滤波器状态（原始值域） */
static int  sDistHist1[DIST_COUNT];      /* 中值滤波历史 */
static int  sDistHist2[DIST_COUNT];
static int  sDistConfirm[DIST_COUNT];    /* 连续在范围内的次数 */
static bool sDistValid[DIST_COUNT];
static bool sPrimed;                     /* 滤波器是否已装填 */

static int  sEdgeConfirm[EDGE_COUNT];
static int  sEdgeMask;

static int  sSwConfirm[SW_COUNT];
static bool sSwState[SW_COUNT];

static int  sHeadingDeg;
static int  sHeadingCandidate;
static int  sHeadingConfirm;

static long sEncBase[2];                 /* 快照基准，见 sensEncReset */

static bool sBallSeen;
static bool sOpponentSeen;
static int  sBallBearingDeg;
static int  sBallRangeMm;

/*==============================================================================
  小工具
==============================================================================*/

static int median3(int a, int b, int c)
{
	if ((a <= b && b <= c) || (c <= b && b <= a)) return b;
	if ((b <= a && a <= c) || (c <= a && a <= b)) return a;
	return c;
}

/* 一阶 IIR 低通。gain 越接近 1 越不滤波。 */
static int lowPass(int newVal, int prevVal, float gain)
{
	return (int)(gain * newVal + (1.0 - gain) * prevVal);
}

/*==============================================================================
  Sharp 测距

  raw → mm 用双曲反演 d = K/(raw + C)。Sharp 的特性是 1/V 与距离近似线性，
  这个形式是往届唯一被多组独立采用的（JadeHouseDisco、Davidlequnchen 测试件、
  2018 课程范例三处形式一致，只是系数不同）。

  折返保护是往届 15 个仓库全都没做的一件事：10-80cm 型号在近于约 10cm 处
  曲线折返，5cm 与 35cm 读数可能相同。mehhh2u 的 <300 钳位甚至把 4cm 的物体
  报成 92cm（"前方空旷"）—— 恰好在要抓球的那一刻误判。
==============================================================================*/

static bool isShortSensor(tDistSensor s)
{
	return (s == DIST_NEAR);
}

/* SRC: JadeHouseDisco/MA4012 main.c:176 d=24339/(raw+149.1) — 改为 mm、加零除保护、
        长短距分别用各自系数 */
static int rawToMm(tDistSensor s, int raw)
{
	float k, c, denom;

	if (isShortSensor(s)) { k = CAL_SHARP_SHORT_K; c = CAL_SHARP_SHORT_C; }
	else                  { k = CAL_SHARP_LONG_K;  c = CAL_SHARP_LONG_C;  }

	denom = (float)raw + c;
	if (denom < 1.0) return -1;
	return (int)(k * 10.0 / denom);
}

static int distMinMm(tDistSensor s)
{
	return isShortSensor(s) ? CAL_SHARP_SHORT_MIN_MM : CAL_SHARP_LONG_MIN_MM;
}

static int distMaxMm(tDistSensor s)
{
	return isShortSensor(s) ? CAL_SHARP_SHORT_MAX_MM : CAL_SHARP_LONG_MAX_MM;
}

static tSensors distPort(tDistSensor s)
{
	switch (s) {
		case DIST_BALL:  return PORT_SHARP_BALL;
		case DIST_UPPER: return PORT_SHARP_UPPER;
		case DIST_SIDE:  return PORT_SHARP_SIDE;
		default:         return PORT_SHARP_NEAR;
	}
}

/* SRC: bentjh01/MA4012_Ball_Fondlers roboC_ws/components/sensors.c
        中值(3) → 一阶IIR 的组合。往届 15 个仓库里唯一做了滤波的。
   SRC: ntzeho_MA4012_avengers sensor_detection.c:95
        N 次连续在范围内才发布，否则输出越界哨兵。 */
static void updateOneDistance(tDistSensor s)
{
	int raw, med, mm, lo, hi;
	float gain;

	raw = SensorValue[distPort(s)];
	sDistRaw[s] = raw;

	/* 低于静息值 = 前方空旷。直接判无效，不必走标定换算 —— 双曲反演在
	   小 raw 处会给出巨大的距离值，看着像"很远处有东西"，其实什么都没有。 */
	if (raw < CAL_SHARP_IDLE_RAW) {
		sDistHist2[s]   = sDistHist1[s];
		sDistHist1[s]   = raw;
		sDistConfirm[s] = 0;
		sDistValid[s]   = false;
		sDistMm[s]      = SENS_DIST_INVALID;
		return;
	}

	med = median3(raw, sDistHist1[s], sDistHist2[s]);
	sDistHist2[s] = sDistHist1[s];
	sDistHist1[s] = raw;

	gain = isShortSensor(s) ? CAL_FILTER_GAIN_SHORT : CAL_FILTER_GAIN_LONG;
	sDistFilt[s] = sPrimed ? lowPass(med, sDistFilt[s], gain) : med;

	mm = rawToMm(s, sDistFilt[s]);
	lo = distMinMm(s);
	hi = distMaxMm(s);

	/* 折返区：长距传感器读数过高说明目标近于最小量程，此时距离值无意义。
	   不返回一个错的数，返回无效，交由短距传感器交叉判定。 */
	if (!isShortSensor(s) && sDistFilt[s] > CAL_SHARP_LONG_FOLDBACK_RAW) {
		sDistConfirm[s] = 0;
		sDistValid[s]   = false;
		sDistMm[s]      = SENS_DIST_INVALID;
		return;
	}

	if (mm >= lo && mm <= hi) {
		if (sDistConfirm[s] < CAL_DETECT_CONFIRM_COUNT) sDistConfirm[s]++;
	} else {
		sDistConfirm[s] = 0;
	}

	if (sDistConfirm[s] >= CAL_DETECT_CONFIRM_COUNT) {
		sDistValid[s] = true;
		sDistMm[s]    = mm;
	} else {
		sDistValid[s] = false;
		sDistMm[s]    = SENS_DIST_INVALID;
	}
}

int  sensDistRaw(tDistSensor s)   { return sDistRaw[s]; }
int  sensDistMm(tDistSensor s)    { return sDistMm[s]; }
bool sensDistValid(tDistSensor s) { return sDistValid[s]; }

/*==============================================================================
  球 / 对手判别

  上下叠放两个 Sharp：下方对准球高度，上方高于球只能看到对手。
    下有 + 上无            → 球
    下有 + 上有且距离相近   → 对手（同一个高物体）
    下有 + 上有但差值大     → 球在对手前方，仍可去捡

  用两个传感器互比而非绝对阈值，好处是不受标定漂移影响。
==============================================================================*/

/* SRC: isselin28/MA4012 Functionsall.c:82  比较式 ±100 counts 判别（最早出现）
   SRC: ntzeho_MA4012_avengers robot_state.c:146  改用标定后的 cm 比较（更好）
   SRC: Khoo395_Tennis_Ball_Collector sensor_output.h:182  上下叠放的布置
   改动：改用 mm、加迟滞、上方无效时明确按"无高物体"处理而不是沿用旧值 */
static void updateTargets(void)
{
	int  ballMm, upperMm, diff, detectMm;
	bool lowerHas, upperHas;

	detectMm = CAL_BALL_DETECT_MAX_MM;
	if (sBallSeen) detectMm += CAL_HYSTERESIS_MM;   /* 迟滞，防止阈值附近抖动 */

	lowerHas = sDistValid[DIST_BALL] && sDistMm[DIST_BALL] <= detectMm;
	ballMm   = sDistMm[DIST_BALL];

	upperHas = sDistValid[DIST_UPPER] && sDistMm[DIST_UPPER] <= CAL_UPPER_PRESENT_MM;
	upperMm  = sDistMm[DIST_UPPER];

	if (!lowerHas) {
		sBallSeen       = false;
		sOpponentSeen   = upperHas;
		sBallRangeMm    = SENS_DIST_INVALID;
		sBallBearingDeg = 0;
		return;
	}

	if (!upperHas) {
		sBallSeen     = true;
		sOpponentSeen = false;
		sBallRangeMm  = ballMm;
	} else {
		diff = upperMm - ballMm;
		if (diff < 0) diff = -diff;
		if (diff <= CAL_OPPONENT_DIFF_MM) {
			sBallSeen     = false;
			sOpponentSeen = true;
			sBallRangeMm  = SENS_DIST_INVALID;
		} else {
			sBallSeen     = true;
			sOpponentSeen = true;
			sBallRangeMm  = ballMm;
		}
	}

	/* 方位只能靠侧向传感器粗判，分辨率受锥角限制。
	   侧向比正前近 → 目标偏向侧向传感器那一边。 */
	sBallBearingDeg = 0;
	if (sBallSeen && sDistValid[DIST_SIDE] && sDistMm[DIST_SIDE] < ballMm) {
		sBallBearingDeg = CAL_SHARP_HALF_CONE_DEG;
	}
}

bool sensBallSeen(int *bearingDeg, int *rangeMm)
{
	if (bearingDeg != NULL) *bearingDeg = sBallBearingDeg;
	if (rangeMm    != NULL) *rangeMm    = sBallRangeMm;
	return sBallSeen;
}

bool sensOpponentSeen(void) { return sOpponentSeen; }

/* 近距 Sharp 判定球是否已进入机构可动作范围。
   用短距型号而非长距，正是因为长距在这个距离上已经进折返区了。 */
bool sensBallInGrabRange(void)
{
	return sDistValid[DIST_NEAR] && (sDistMm[DIST_NEAR] <= CAL_BALL_GRAB_MM);
}

/*==============================================================================
  IR 线阵 / 边缘检测

  4 位掩码，一次读全。上层拿到掩码做避让决策，而不是逐个问。
  模块两种用法都支持：数字口（板载比较器）或模拟口（代码里比阈值）。
  Khoo395 在同一台车上同时用了两种口，证明同一个模块两种接法都成立，
  且"压线"在两种口下都是低电平。
==============================================================================*/

static tSensors edgePort(tEdgePos p)
{
	switch (p) {
		case EDGE_FL: return PORT_LINE_FL;
		case EDGE_FR: return PORT_LINE_FR;
		case EDGE_RL: return PORT_LINE_RL;
		default:      return PORT_LINE_RR;
	}
}

#if !CAL_LINE_DIGITAL_MODE
static int edgeThreshold(tEdgePos p)
{
	switch (p) {
		case EDGE_FL: return CAL_LINE_THRESH_FL;
		case EDGE_FR: return CAL_LINE_THRESH_FR;
		case EDGE_RL: return CAL_LINE_THRESH_RL;
		default:      return CAL_LINE_THRESH_RR;
	}
}
#endif

static bool edgeRawOnLine(tEdgePos p)
{
#if CAL_LINE_DIGITAL_MODE
	return (SensorValue[edgePort(p)] == CAL_LINE_ON_LEVEL);
#else
	return (SensorValue[edgePort(p)] < edgeThreshold(p));
#endif
}

/* SRC: ntzeho_MA4012_avengers sensor_detection.c:124  4 位掩码 FL=1 FR=2 BL=4 BR=8
   改动：加计数式去抖（mehhh2u 用 50ms 重读确认，会阻塞，这里改成计数） */
static void updateEdges(void)
{
	int i, mask;

	mask = 0;
	for (i = 0; i < EDGE_COUNT; i++) {
		if (edgeRawOnLine((tEdgePos)i)) {
			if (sEdgeConfirm[i] < CAL_LINE_CONFIRM_COUNT) sEdgeConfirm[i]++;
		} else {
			sEdgeConfirm[i] = 0;
		}
		if (sEdgeConfirm[i] >= CAL_LINE_CONFIRM_COUNT) mask |= (1 << i);
	}
	sEdgeMask = mask;
}

bool sensEdge(tEdgePos p) { return (sEdgeMask & (1 << (int)p)) != 0; }
int  sensEdgeMask(void)   { return sEdgeMask; }

/*==============================================================================
  限位开关

  支持数字口与模拟口两种接法。模拟口读法是往届验证过的（Khoo395 in5、
  penghengx in6/in7/in8），因为 digital 口不够用时这是唯一出路。
==============================================================================*/

/* SRC: Khoo395_Tennis_Ball_Collector sensor_output.h:80  analog 口读机械开关
   改动：原代码用 != 0 精确比较，12 位 ADC 上 1 个 LSB 噪声即误判，改为阈值 */
static bool switchRawPressed(tSensors port, int onAnalog)
{
	if (onAnalog) return (SensorValue[port] < CAL_SWITCH_ANALOG_PRESSED_MAX);
	return (SensorValue[port] == CAL_SWITCH_DIGITAL_PRESSED_LEVEL);
}

static void updateOneSwitch(int idx, tSensors port, int onAnalog)
{
	if (switchRawPressed(port, onAnalog)) {
		if (sSwConfirm[idx] < CAL_SWITCH_CONFIRM_COUNT) sSwConfirm[idx]++;
	} else {
		sSwConfirm[idx] = 0;
	}
	sSwState[idx] = (sSwConfirm[idx] >= CAL_SWITCH_CONFIRM_COUNT);
}

static void updateSwitches(void)
{
	updateOneSwitch(SW_SCOOP_TOP,    PORT_SW_SCOOP_TOP,    SW_SCOOP_TOP_ON_ANALOG);
	updateOneSwitch(SW_SCOOP_BOTTOM, PORT_SW_SCOOP_BOTTOM, SW_SCOOP_BOTTOM_ON_ANALOG);
	updateOneSwitch(SW_BALL_PRESENT, PORT_SW_BALL_PRESENT, SW_BALL_PRESENT_ON_ANALOG);
	updateOneSwitch(SW_BUMPER,       PORT_SW_BUMPER,       SW_BUMPER_ON_ANALOG);
}

bool sensScoopAtTop(void)    { return sSwState[SW_SCOOP_TOP]; }
bool sensScoopAtBottom(void) { return sSwState[SW_SCOOP_BOTTOM]; }
bool sensBumperHit(void)     { return sSwState[SW_BUMPER]; }
bool sensBallHeld(void)      { return sSwState[SW_BALL_PRESENT]; }

/*==============================================================================
  Digital Compass 1490

  4 路数字输出（低有效）+ 1 路供电。不是 I2C。

  不用查表：往届各组的"编码→方位"表互不相同（Rzi98 的表和 penghengx 完全不同），
  因为那取决于哪根线插哪个口。但结构是一致的 ——
  一位为低 = 该正方向；相邻两位为低 = 两者之间的斜方向。
  按结构解码就与接线顺序无关，只要 M7 确认好四个引脚各对应哪个方向。

  交叉验证：把往届的编码按 N/E/S/W 排序后是 14,12,13,9,11,3,7,6，
  相邻方位恰好只差一位（循环格雷码）。这既佐证了上述结构，也说明
  转动时在扇区交界处读到的仍是合法编码，不会出现野值。

  往届无人做 hold-last-good：SKEW002 把 -1 直接写进全局，Khoo395 写 8，
  PatrickPetch 什么都不写留下陈旧值。这里在非法编码时保持上一个有效值。
==============================================================================*/

/* SRC: penghengx / ntzeho / Davidlequnchen 的 read_orientation()（三者逐字相同，
        且与 2018 年课程范例一致）—— 采纳其"4 位低有效"结论，
        但改为结构解码 + 保持上次有效值 */
/* 结构解码，不查表：
     mask 的 bit0..bit3 = N/E/S/W（由 robot_config.h 的 PORT_COMPASS_* 决定哪根
     线是哪一位，M7 只需确认这一件事），已在 compassBit() 里统一成 1 = 该方向有效。
       恰好一位为 1        → 该正方向          heading = bit * 90
       恰好两位且环上相邻   → 两者之间的斜方向   heading = 低位 * 90 + 45
       其余（0 位 / 对角两位 / 3-4 位）→ 非法编码
   环上相邻包含 (W,N) 这一对，即 bit3 与 bit0 —— 它对应 NW。
   往届各组的查表互不相同（取决于接线顺序），但这个结构在 5 个仓库里一致；
   把往届编码按 N..NW 排开是一个循环 4 位格雷码，正是上述结构的佐证。 */
static int compassDecodeDeg(int mask)
{
	int i, bits, lo, hi;

	bits = 0;
	lo   = -1;
	hi   = -1;
	for (i = 0; i < 4; i++) {
		if ((mask & (1 << i)) != 0) {
			bits++;
			if (lo < 0)      lo = i;
			else if (hi < 0) hi = i;
		}
	}

	if (bits == 1) return lo * 90;                       /* N / E / S / W */

	if (bits == 2) {
		if (hi == lo + 1)          return lo * 90 + 45;  /* NE / SE / SW  */
		if (lo == 0 && hi == 3)    return HEADING_NW;    /* 环绕的那一对   */
	}

	return SENS_NO_HEADING;
}

static int compassBit(tSensors port)
{
#if CAL_COMPASS_ACTIVE_LOW
	return (SensorValue[port] == 0) ? 1 : 0;
#else
	return (SensorValue[port] != 0) ? 1 : 0;
#endif
}

static void updateCompass(void)
{
	int mask, deg;

	mask = compassBit(PORT_COMPASS_N)
	     | (compassBit(PORT_COMPASS_E) << 1)
	     | (compassBit(PORT_COMPASS_S) << 2)
	     | (compassBit(PORT_COMPASS_W) << 3);

	deg = compassDecodeDeg(mask);

	if (deg == SENS_NO_HEADING) return;   /* 非法编码：保持上次有效值 */

	/* 罗盘在扇区边界会抖动（Khoo395 spin_search.c:4 的注释
	   "+2 is used because of the shaking compass" 是共同经验），
	   所以要连续读到同一值才更新。 */
	if (deg == sHeadingCandidate) {
		if (sHeadingConfirm < CAL_COMPASS_CONFIRM_COUNT) sHeadingConfirm++;
	} else {
		sHeadingCandidate = deg;
		sHeadingConfirm   = 1;
	}

	if (sHeadingConfirm >= CAL_COMPASS_CONFIRM_COUNT) sHeadingDeg = deg;
}

int  sensHeadingDeg(void)   { return sHeadingDeg; }
bool sensHeadingValid(void) { return (sHeadingDeg != SENS_NO_HEADING); }

/*==============================================================================
  编码器

  用"快照差值"而不是"清零再读"，避免清零与中断计数的竞争。
==============================================================================*/

/* SRC: PatrickPetch_tennisbot competition.c:449  快照差值法 + 弧长换算
        arc = radians(ticks) * wheel_radius
   改动：把隐含的 360 ticks/rev 提成具名常数 CAL_ENC_TICKS_PER_M（原代码
        通过 degToRad() 把这个假设藏在实现里，换编码器就会静默出错） */
static tSensors encPort(tSide s)
{
	return (s == SIDE_LEFT) ? PORT_ENC_LEFT : PORT_ENC_RIGHT;
}

long sensEncTicks(tSide s)
{
	if (encPort(s) == SENS_PORT_NONE) return 0;
	return (long)SensorValue[encPort(s)] - sEncBase[(int)s];
}

long sensEncDistMm(tSide s)
{
	return (sensEncTicks(s) * 1000L) / CAL_ENC_TICKS_PER_M;
}

bool sensEncTrusted(void)
{
	return (CAL_ENC_TRUSTWORTHY != 0);
}

void sensEncReset(void)
{
	if (PORT_ENC_LEFT  != SENS_PORT_NONE) sEncBase[SIDE_LEFT]  = SensorValue[PORT_ENC_LEFT];
	if (PORT_ENC_RIGHT != SENS_PORT_NONE) sEncBase[SIDE_RIGHT] = SensorValue[PORT_ENC_RIGHT];
}

/*==============================================================================
  生命周期
==============================================================================*/

void sensInit(void)
{
	int i;

	for (i = 0; i < DIST_COUNT; i++) {
		sDistRaw[i] = 0; sDistMm[i] = SENS_DIST_INVALID;
		sDistFilt[i] = 0; sDistHist1[i] = 0; sDistHist2[i] = 0;
		sDistConfirm[i] = 0; sDistValid[i] = false;
	}
	for (i = 0; i < EDGE_COUNT; i++) sEdgeConfirm[i] = 0;
	for (i = 0; i < SW_COUNT; i++) { sSwConfirm[i] = 0; sSwState[i] = false; }

	sEdgeMask         = 0;
	sHeadingDeg       = SENS_NO_HEADING;
	sHeadingCandidate = SENS_NO_HEADING;
	sHeadingConfirm   = 0;
	sBallSeen         = false;
	sOpponentSeen     = false;
	sBallBearingDeg   = 0;
	sBallRangeMm      = SENS_DIST_INVALID;
	sPrimed           = false;

#if CAL_COMPASS_DRIVE_SUPPLY
	/* 供电引脚须声明为 sensorDigitalOut 才能写。若按 Khoo395 的做法把它放在
	   analog 口声明 sensorNone（纯粹当电源接线柱），则把 CAL_COMPASS_DRIVE_SUPPLY
	   设为 0，此处不写。 */
	SensorValue[PORT_COMPASS_SUPPLY] = 1;
#endif

	sensEncReset();
}

void sensUpdate(void)
{
	int i;

	for (i = 0; i < DIST_COUNT; i++) updateOneDistance((tDistSensor)i);
	sPrimed = true;

	updateTargets();
	updateEdges();
	updateSwitches();
	updateCompass();
}

/*==============================================================================
  调试输出
==============================================================================*/

void sensDumpState(void)
{
	int i;

	writeDebugStream("dist:");
	for (i = 0; i < DIST_COUNT; i++) {
		writeDebugStream(" %d(%d)", sDistMm[i], sDistRaw[i]);
	}
	writeDebugStream("  edge:%d  ball:%d opp:%d held:%d  hdg:%d",
	                 sEdgeMask, (int)sBallSeen, (int)sOpponentSeen,
	                 (int)sSwState[SW_BALL_PRESENT], sHeadingDeg);
	writeDebugStreamLine("");
}
