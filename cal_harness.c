/*==============================================================================
  cal_harness.c  —  MA4012 传感器标定夹具（独立程序，不是比赛代码）

  用途：拿到传感器后跑这个，配合 CALIBRATION_PROTOCOL.md 采集标定数据。
  输出走 ROBOTC debug stream → 必须有线模式（Robot > Vex Cortex Comm Mode > Vex & USB）。
  Cortex 无 LCD，没有别的输出途径。

  本文件刻意不依赖 robot_config.h 的端口分配：它把 in1..in8 全部当模拟口、
  dgtl1..dgtl12 全部当数字输入通扫一遍，所以端口还没定就能编译运行。
  拨动/遮挡某个传感器，看哪一列在变，即可反查映射。

  写法刻意保守以迁就 ROBOTC 的 C 子集：变量全部在函数顶部声明、不用 string /
  sprintf / strcat（ROBOTC 的 string 只有 20 字符会截断）、不用 const 数组。

  ⚠️ 未经硬件验证。首次编译前请用 Motors and Sensors Setup 重新生成下面的
     #pragma 块并覆盖 —— 手写 pragma 容易出错。
==============================================================================*/

#pragma config(Sensor, in1,  A1, sensorAnalog)
#pragma config(Sensor, in2,  A2, sensorAnalog)
#pragma config(Sensor, in3,  A3, sensorAnalog)
#pragma config(Sensor, in4,  A4, sensorAnalog)
#pragma config(Sensor, in5,  A5, sensorAnalog)
#pragma config(Sensor, in6,  A6, sensorAnalog)
#pragma config(Sensor, in7,  A7, sensorAnalog)
#pragma config(Sensor, in8,  A8, sensorAnalog)
#pragma config(Sensor, dgtl1,  D1,  sensorDigitalIn)
#pragma config(Sensor, dgtl2,  D2,  sensorDigitalIn)
#pragma config(Sensor, dgtl3,  D3,  sensorDigitalIn)
#pragma config(Sensor, dgtl4,  D4,  sensorDigitalIn)
#pragma config(Sensor, dgtl5,  D5,  sensorDigitalIn)
#pragma config(Sensor, dgtl6,  D6,  sensorDigitalIn)
#pragma config(Sensor, dgtl7,  D7,  sensorDigitalIn)
#pragma config(Sensor, dgtl8,  D8,  sensorDigitalIn)
#pragma config(Sensor, dgtl9,  D9,  sensorDigitalIn)
#pragma config(Sensor, dgtl10, D10, sensorDigitalIn)
#pragma config(Sensor, dgtl11, D11, sensorDigitalIn)
#pragma config(Sensor, dgtl12, D12, sensorDigitalIn)

/*---------------------------------------------------------------- 参数 ------*/
#define CAL_SAMPLES        50    /* 每次触发采样数 */
#define CAL_SAMPLE_GAP_MS  20    /* 采样间隔 → 一次约 1 秒 */
#define DASH_PERIOD_MS    250    /* 仪表盘刷新周期 */
#define TRIGGER_SENSOR     D12   /* 采样触发开关插这里 */
#define TRIGGER_ACTIVE_LOW   1   /* 开关按下读 0 则设 1；读 1 则设 0 */

tSensors gAnalog[8];
tSensors gDigital[12];

void initPortTables()
{
	gAnalog[0]=A1; gAnalog[1]=A2; gAnalog[2]=A3; gAnalog[3]=A4;
	gAnalog[4]=A5; gAnalog[5]=A6; gAnalog[6]=A7; gAnalog[7]=A8;

	gDigital[0]=D1;  gDigital[1]=D2;  gDigital[2]=D3;  gDigital[3]=D4;
	gDigital[4]=D5;  gDigital[5]=D6;  gDigital[6]=D7;  gDigital[7]=D8;
	gDigital[8]=D9;  gDigital[9]=D10; gDigital[10]=D11; gDigital[11]=D12;
}

/*------------------------------------------------------------ 触发开关 ------*/
bool triggerPressed()
{
	int v;
	v = SensorValue[TRIGGER_SENSOR];
#if TRIGGER_ACTIVE_LOW
	return (v == 0);
#else
	return (v != 0);
#endif
}

/*------------------------------------------------------------- 仪表盘 ------*/
/* 连续打印全部端口原始值。用来反查端口映射、看噪声、确认接线。
   逐段 writeDebugStream 而不拼字符串 —— ROBOTC 的 string 只有 20 字符。 */
void printDashboard()
{
	int i;

	writeDebugStream("A:");
	for (i = 0; i < 8; i++) {
		writeDebugStream(" %4d", SensorValue[gAnalog[i]]);
	}
	writeDebugStream("   D:");
	for (i = 0; i < 12; i++) {
		writeDebugStream("%d", SensorValue[gDigital[i]]);
	}
	writeDebugStreamLine("");
}

/*--------------------------------------------------------- 触发式采样 ------*/
/* 对全部 8 路模拟口同时采 CAL_SAMPLES 次，打印 均值/最小/最大/标准差。
   标准差是关键：它从哪个距离开始变大，就是该传感器的实际可用量程上限。 */
void captureBurst(int tag)
{
	long sum[8];
	long sumSq[8];
	int  lo[8];
	int  hi[8];
	int  i, n, v;
	float mean, var;

	for (i = 0; i < 8; i++) {
		sum[i] = 0; sumSq[i] = 0; lo[i] = 32767; hi[i] = -32768;
	}

	for (n = 0; n < CAL_SAMPLES; n++) {
		for (i = 0; i < 8; i++) {
			v = SensorValue[gAnalog[i]];
			sum[i]   += v;
			sumSq[i] += (long)v * (long)v;
			if (v < lo[i]) lo[i] = v;
			if (v > hi[i]) hi[i] = v;
		}
		wait1Msec(CAL_SAMPLE_GAP_MS);
	}

	writeDebugStreamLine("--- capture #%d  (n=%d) ---", tag, CAL_SAMPLES);
	writeDebugStreamLine("port  mean   min   max    sd");
	for (i = 0; i < 8; i++) {
		mean = (float)sum[i] / CAL_SAMPLES;
		var  = (float)sumSq[i] / CAL_SAMPLES - mean * mean;
		if (var < 0) var = 0;
		/* sd 放大 100 倍按整数打印，避开 ROBOTC 浮点格式化的不确定性 */
		writeDebugStreamLine("A%d  %6d %5d %5d  %5d/100",
		                     i + 1, (int)mean, lo[i], hi[i], (int)(sqrt(var) * 100));
	}
	writeDebugStreamLine("");
}

/*----------------------------------------------------------------- main ----*/
task main()
{
	int  captureCount;
	bool prevTrigger;
	bool now;

	captureCount = 0;
	prevTrigger  = false;

	initPortTables();

	writeDebugStreamLine("=== MA4012 sensor calibration harness ===");
	writeDebugStreamLine("dashboard runs continuously; press trigger switch to capture");
	writeDebugStreamLine("");

	clearTimer(T1);

	while (true) {
		/* 触发开关上升沿 → 采一组。prevTrigger 去抖，避免按住时连采。 */
		now = triggerPressed();
		if (now && !prevTrigger) {
			captureCount++;
			captureBurst(captureCount);
			clearTimer(T1);
		}
		prevTrigger = now;

		if (time1[T1] >= DASH_PERIOD_MS) {
			printDashboard();
			clearTimer(T1);
		}

		abortTimeslice();
	}
}
