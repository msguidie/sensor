/*==============================================================================
  robot_config.h  —  端口分配与机械常数

  ⚠️ 本文件目前无法编译：下面的端口宏全部是 TODO 占位。
     组内定稿端口后填这一个块，其余文件都不用动。
     填写依据见 PORT_MAP.md（含 digital 口超支 5 路的取舍推导）。

  填完后还要做一步：用 ROBOTC 的 Motors and Sensors Setup 生成 #pragma config
  块，粘到主程序顶部。ROBOTC 的传感器名来自 pragma，这里的宏只是给人看的
  单一事实来源，两处必须一致。
==============================================================================*/

#ifndef ROBOT_CONFIG_H
#define ROBOT_CONFIG_H

/*============================================================================
  ===== 待填 1/2：端口分配 =====
  VEX Cortex 资源：8 路 analog(in1-in8) · 12 路 digital(dgtl1-dgtl12)
  需求 17 路 digital，超支 5 路 → 必须在 PORT_MAP.md 的方案 A / B 中二选一。
============================================================================*/

/*--- Sharp 测距（模拟）---------------------------------------------------*/
#define PORT_SHARP_BALL      TODO   /* 10-80cm  前·下，对准球高度         */
#define PORT_SHARP_UPPER     TODO   /* 10-80cm  前·上，高于球 → 识别对手  */
#define PORT_SHARP_SIDE      TODO   /* 10-80cm  斜前侧向，搜索扫描        */
#define PORT_SHARP_NEAR      TODO   /* 4-30cm   铲口近距，解折返歧义      */

/*--- IR 线阵 / 边缘检测（数字）------------------------------------------*/
#define PORT_LINE_FL         TODO   /* 前左 */
#define PORT_LINE_FR         TODO   /* 前右 */
#define PORT_LINE_RL         TODO   /* 后左 */
#define PORT_LINE_RR         TODO   /* 后右 */

/*--- 限位开关 -------------------------------------------------------------
  方案 A：3 个在 analog 口 + 1 个在 digital 口
  方案 B：3 个全在 analog 口（放弃第 4 个）
  在 analog 口读机械开关是往届证实可行的做法，见 SENSOR_SCOPE.md 第 2 节。
  每个开关在下面登记它接的是哪类口，sensors.c 据此选读法。
--------------------------------------------------------------------------*/
#define PORT_SW_SCOOP_TOP    TODO
#define PORT_SW_SCOOP_BOTTOM TODO
#define PORT_SW_BALL_PRESENT TODO
#define PORT_SW_BUMPER       TODO

/* 上面每个开关是接模拟口还是数字口：1 = analog，0 = digital */
#define SW_SCOOP_TOP_ON_ANALOG     TODO
#define SW_SCOOP_BOTTOM_ON_ANALOG  TODO
#define SW_BALL_PRESENT_ON_ANALOG  TODO
#define SW_BUMPER_ON_ANALOG        TODO

/*--- Digital Compass 1490（4 位数字并口 + 1 路供电）----------------------
  注意：不是 I2C。往届 12 份仓库全部这样接。
  供电引脚可放 analog 口并声明 sensorNone（省一路 digital）。
--------------------------------------------------------------------------*/
#define PORT_COMPASS_N       TODO
#define PORT_COMPASS_E       TODO
#define PORT_COMPASS_S       TODO
#define PORT_COMPASS_W       TODO
#define PORT_COMPASS_SUPPLY  TODO

/*--- 正交编码器（每个占两路连续 digital）---------------------------------
  ROBOTC 里 sensorQuadEncoder 声明在第一个口上，自动占用紧邻的下一个口。
  方案 A 只装一个；方案 B 装两个。用不到的填 SENS_PORT_NONE。
--------------------------------------------------------------------------*/
#define PORT_ENC_LEFT        TODO
#define PORT_ENC_RIGHT       TODO

/*--- 电机（不属于 sensor scope，仅为接线图完整性）-----------------------*/
#define PORT_MOTOR_LEFT      TODO   /* 2-Wire Motor 393，建议 port1  */
#define PORT_MOTOR_RIGHT     TODO   /* 2-Wire Motor 393，建议 port10 */

/*============================================================================
  ===== 待填 2/2：机械常数 =====
  这些数影响里程推算与速度估算，等车体组定稿后填。
============================================================================*/

#define WHEEL_DIAMETER_MM    TODO   /* CAD 里目前是 2.75in ≈ 69.85mm，待实物确认 */
#define TRACK_WIDTH_MM       TODO   /* 左右驱动轮中心距 */
#define DRIVE_GEAR_RATIO     TODO   /* 电机轴 : 车轮，直驱填 1 */

/* 车体是否使用全向轮。1 = 是。
   全向轮横向可滑动，编码器里程推算会持续漂移 → sensors.c 会据此降低
   里程置信度并在文档中提示改用罗盘导航。器材单未注明轮型，需实物确认。 */
#define USING_OMNI_WHEELS    TODO

/*============================================================================
  以下为不需要填的固定量
============================================================================*/

#define SENS_PORT_NONE       (-1)

#define CORTEX_ANALOG_COUNT   8
#define CORTEX_DIGITAL_COUNT 12
#define CORTEX_ADC_MAX     4095   /* 12-bit ADC，M1 步骤实测确认 */

#endif /* ROBOT_CONFIG_H */
