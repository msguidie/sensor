# Calibration Protocol

Bench measurements, run once we have the sensors. Steps M1–M8 map to constants in
`sensor_cal.h`. 60–90 min, faster with two people.

Everything runs through `cal_harness.c`. Output goes to the ROBOTC debug stream,
so the Cortex must be tethered (Robot > VEX Cortex Comm Mode > VEX & USB) — there
is no LCD.

**Before starting:** fill the port block in `robot_config.h`, regenerate
`#pragma config` with Motors and Sensors Setup (overwrite the placeholder), build
and download `cal_harness.c`.

**Bring:** Cortex, battery, USB A-A, laptop, ruler taped to the bench, the tennis
ball, a flat board for comparison, the yellow tape, and a limit switch on
`TRIGGER_SENSOR` (default D12) as the capture button.

## M1 — ADC range and idle readings

All Sharps pointed at open space (>1.5 m), watch the dashboard 30 s, record idle
reading and jitter for each.

→ `CAL_ADC_MAX`, `CAL_SHARP_IDLE_RAW`

## M2 — Sharp distance

The long one. All four sensors separately — the three long-range parts vary too
much to share a curve.

**Use the tennis ball, not just a flat board.** A ball is small and curved and
reflects far less; a board-fitted curve overestimates distance in play.

Distances (cm): 3, 4, 5, 6, 7, 8, 9, 10, 12, 15, 20, 25, 30, 40, 50, 60, 70, 80.
Denser below 10 cm — that is where the fold-back is.

Per point: ball on the mark facing the sensor, press trigger, 50 samples, record
mean / min / max / sd. Also measure the board at 10, 20, 40, 60 cm and note the
difference from the ball.

→ `CAL_SHARP_LONG_K` / `_C`, `CAL_SHARP_SHORT_K` / `_C`. Code uses
`distance_mm = 10 * K / (raw + C)`, so fit `1/distance` against `raw`.
**Use only the 15–80 cm points** — 3–12 cm is fold-back and will skew the fit.
If the three long parts differ noticeably, split into three sets and update
`rawToMm()`.

→ `CAL_SHARP_LONG_FOLDBACK_RAW` — raw value at the reading's peak. Above this the
distance is meaningless.

→ `CAL_SHARP_LONG_MIN/MAX_MM`, `CAL_SHARP_SHORT_MIN/MAX_MM` — upper bound is where
sd starts climbing, usually well short of the rated 80 cm.

→ sanity-check `CAL_FILTER_GAIN_LONG` / `_SHORT`, `CAL_DETECT_CONFIRM_COUNT`.

## M3 — Detection cone

At 20 and 40 cm, move the ball sideways: 0, ±2, ±5, ±8, ±12, ±16 cm. Find the
largest offset still reliably detected. `half angle = atan(offset / distance)`.

→ `CAL_SHARP_HALF_CONE_DEG`. Search scan step must be ≤ 2× this or sweeps leave
blind gaps.

## M4 — Ball vs opponent

Ball alone at 20 and 40 cm; then a tall object (book on end) at the same
distances; then both at once. Record lower and upper readings each time.

→ `CAL_UPPER_PRESENT_MM`, `CAL_OPPONENT_DIFF_MM`, `CAL_BALL_DETECT_MAX_MM`,
`CAL_BALL_GRAB_MM`, `CAL_HYSTERESIS_MM` (all in calibrated mm, not raw counts)

## M5 — IR line array

**Do this twice.** Lab card and venue tape have different reflectance, so lab
thresholds will not necessarily hold.

Each module over plain bench, then over the tape, record both. Digital module:
adjust the trim pot until the two cases read a stable 0 / 1. Analog: take the
midpoint. Record ride height — TCRT5000 is very sensitive to it. Repeat at the venue.

→ `CAL_LINE_DIGITAL_MODE`, `CAL_LINE_ON_LEVEL`, `CAL_LINE_CONFIRM_COUNT`,
`CAL_LINE_HEIGHT_MM`; analog mode also `CAL_LINE_THRESH_FL` / `_FR` / `_RL` /
`_RR` — measure each separately, do not share one threshold.

## M6 — Limit switches

Press, record; release, record; take the midpoint.

→ `CAL_SWITCH_ANALOG_PRESSED_MAX` (below = pressed; expect near full-scale
released, pulled to 0 pressed — confirm direction), `CAL_SWITCH_DIGITAL_PRESSED_LEVEL`,
`CAL_SWITCH_CONFIRM_COUNT`, and `SW_*_ON_ANALOG` in `robot_config.h`.

## M7 — Compass

Skip until it is issued.

Confirm the supply pin is high and the compass is outputting, rotate slowly
through a full turn, record which pin is active in each of the eight sectors.

→ `PORT_COMPASS_N` / `_E` / `_S` / `_W` in `robot_config.h` — that mapping is the
whole point of this step. Plus `CAL_COMPASS_ACTIVE_LOW` (expect 1),
`CAL_COMPASS_DRIVE_SUPPLY`, `CAL_COMPASS_CONFIRM_COUNT`.

No code-to-heading table needed; decoding is structural and independent of wiring
order once the four pins are identified.

## M8 — Encoder ticks per metre

Zero the counter, push the robot 1.00 m straight, read the count. Three times, average.

→ `CAL_ENC_TICKS_PER_M`; `CAL_ENC_TRUSTWORTHY` once the wheel type is known (0 for omni).

## Afterwards

Fill `sensor_cal.h` and delete the `PROVISIONAL` markers. Raw per-point data goes
in the logbook with a spreadsheet copy — it feeds the Design Calculations section.
Plot raw ADC against distance with the fitted curve; that plot is easy marks.

## Pre-match checklist

- [ ] Battery voltage — Sharp readings drift with it, thresholds go off at low charge
- [ ] IR modules still at the right ride height
- [ ] Sharp lenses clean
- [ ] Re-check line triggering on the actual tape
