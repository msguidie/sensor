/*==============================================================================
  sensors.h  —  传感器层对外接口（契约）

  上层（navigation / collector / 主控制）只调用这里的函数，不直接读
  SensorValue[]。这样端口改了、标定数据变了，上层都不用动。

  使用方式：
      sensInit();                  // 开机一次
      while (true) {
          sensUpdate();            // 每个主循环调一次，非阻塞
          ... 用下面的查询函数做决策 ...
      }

  ⚠️ 未经硬件验证。全部阈值来自 sensor_cal.h，实测前为占位值。
==============================================================================*/

#ifndef SENSORS_H
#define SENSORS_H

#include "robot_config.h"

/*---------------------------------------------------------------- 类型 ------*/

typedef enum {
	DIST_BALL   = 0,   /* 前·下 Sharp，对准球高度 */
	DIST_UPPER  = 1,   /* 前·上 Sharp，高于球     */
	DIST_SIDE   = 2,   /* 斜前侧向 Sharp          */
	DIST_NEAR   = 3,   /* 4-30cm 近距 Sharp       */
	DIST_COUNT  = 4
} tDistSensor;

typedef enum {
	EDGE_FL = 0,
	EDGE_FR = 1,
	EDGE_RL = 2,
	EDGE_RR = 3,
	EDGE_COUNT = 4
} tEdgePos;

typedef enum { SIDE_LEFT = 0, SIDE_RIGHT = 1 } tSide;

/* 边缘位掩码 —— sensEdgeMask() 的返回值按位或 */
#define EDGE_MASK_FL   0x01
#define EDGE_MASK_FR   0x02
#define EDGE_MASK_RL   0x04
#define EDGE_MASK_RR   0x08
#define EDGE_MASK_FRONT (EDGE_MASK_FL | EDGE_MASK_FR)
#define EDGE_MASK_REAR  (EDGE_MASK_RL | EDGE_MASK_RR)

/* 无效返回值 */
#define SENS_DIST_INVALID   (-1)
#define SENS_NO_HEADING     (-1)

/* 航向（罗盘只有 45° 分辨率，8 个方位）*/
#define HEADING_N    0
#define HEADING_NE  45
#define HEADING_E   90
#define HEADING_SE 135
#define HEADING_S  180
#define HEADING_SW 225
#define HEADING_W  270
#define HEADING_NW 315

/*------------------------------------------------------------- 生命周期 ----*/

void sensInit(void);

/* 每个主循环调用一次。非阻塞：内部只推进一步采样，不含任何等待循环。
   Cortex 无抢占式调度，传感器层里一个 while 等待会同时冻结边缘检测，
   车会直接冲出界 —— 往届代码最常见的翻车原因。 */
void sensUpdate(void);

/*--------------------------------------------------------------- 测距 ------*/

/* 滤波后的距离，单位 mm。无效时返回 SENS_DIST_INVALID。
   无效的两种情况：读数超出可信量程；落入 Sharp 近距折返区且无法消歧。 */
int  sensDistMm(tDistSensor s);

bool sensDistValid(tDistSensor s);

/* 原始 ADC 值，调试与标定用 */
int  sensDistRaw(tDistSensor s);

/*----------------------------------------------------------- 目标识别 ------*/

/* 前方是否有球。bearingDeg / rangeMm 为输出参数，可传 NULL。
   bearingDeg：负值偏左，正值偏右，0 为正前方。分辨率受 Sharp 锥角限制。 */
bool sensBallSeen(int *bearingDeg, int *rangeMm);

/* 球是否已在收集器里（限位开关判定） */
bool sensBallHeld(void);

/* 球是否已近到收集机构可以动作。由 4-30cm 近距 Sharp 判定，
   上层据此决定何时停车并放下铲子。 */
bool sensBallInGrabRange(void);

/* 前方是否是对手机器人而非球。
   判据：上下叠放的两个 Sharp —— 下方看到、上方也看到 = 高物体 = 对手。
   这是往届公认解法，直接对应作业里「两台车同场」的规则。 */
bool sensOpponentSeen(void);

/*----------------------------------------------------------- 边缘检测 ------*/

bool sensEdge(tEdgePos p);

/* 四个边缘传感器的位掩码，一次读全。上层做避让决策时用这个。 */
int  sensEdgeMask(void);

/*------------------------------------------------------------- 机构状态 ----*/

bool sensScoopAtTop(void);
bool sensScoopAtBottom(void);
bool sensBumperHit(void);

/*--------------------------------------------------------------- 里程 ------*/

long sensEncTicks(tSide s);
void sensEncReset(void);

/* 累计行进距离 mm。若 USING_OMNI_WHEELS 为 1，全向轮横向滑动会让此值
   持续漂移，只应作粗略参考。 */
long sensEncDistMm(tSide s);

/* 里程是否可信（由 sensor_cal.h 的 CAL_ENC_TRUSTWORTHY 决定）。
   为 false 时上层不应把里程用于闭环，应以罗盘为主。 */
bool sensEncTrusted(void);

/*--------------------------------------------------------------- 航向 ------*/

/* 0-315 度，45 度一档。罗盘尚未发放或读数非法时返回 SENS_NO_HEADING，
   上层必须处理这个返回值而不是假定总有航向。 */
int  sensHeadingDeg(void);

bool sensHeadingValid(void);

/*--------------------------------------------------------------- 调试 ------*/

/* 把当前全部传感器状态打到 debug stream。调车时全组都会用到。 */
void sensDumpState(void);

#endif /* SENSORS_H */
