# Tuning

Every value below is currently a placeholder. Step column refers to
`CALIBRATION_PROTOCOL.md`.

- **must measure** — the placeholder is almost certainly wrong
- **can run first** — the placeholder will probably work; refine later

## `robot_config.h` — ports and mechanics

All TODO. Nothing compiles until these are filled.

| Parameter | How to decide |
|---|---|
| `PORT_SHARP_*`, `PORT_LINE_*`, `PORT_COMPASS_*` | Group decides ports. **Settle option A vs B in `PORT_MAP.md` first.** |
| `PORT_SW_*` + `SW_*_ON_ANALOG` | Record whether each switch is on analog or digital |
| `PORT_ENC_LEFT` / `_RIGHT` | Option A uses one; set the other to `SENS_PORT_NONE` |
| `WHEEL_DIAMETER_MM` | Measure. CAD says 2.75 in ≈ 69.85 mm, unconfirmed |
| `TRACK_WIDTH_MM` | Centre-to-centre of the drive wheels |
| `DRIVE_GEAR_RATIO` | 1 for direct drive |
| `USING_OMNI_WHEELS` | Look at a wheel — a ring of small rollers on the rim means omni. Decides whether odometry is usable at all. |

## `sensor_cal.h` — calibration

| Parameter | Placeholder | Step | |
|---|---|---|---|
| `CAL_SHARP_LONG_K` / `_C` | 24339 / 149.1 | M2 | must measure |
| `CAL_SHARP_SHORT_K` / `_C` | 9200 / 100 | M2 | must measure — least confident value in the file |
| `CAL_SHARP_LONG_FOLDBACK_RAW` | 2600 | M2 | must measure — no prior data, pure estimate |
| `CAL_SHARP_LONG_MIN/MAX_MM` | 100 / 700 | M2 | must measure |
| `CAL_SHARP_SHORT_MIN/MAX_MM` | 40 / 280 | M2 | must measure |
| `CAL_SHARP_IDLE_RAW` | 250 | M1 | can run first |
| `CAL_FILTER_GAIN_LONG` | 0.50 | M2 | can run first |
| `CAL_FILTER_GAIN_SHORT` | 1.00 (unfiltered, for response) | M2 | can run first |
| `CAL_DETECT_CONFIRM_COUNT` | 3 | M2 | can run first |
| `CAL_OPPONENT_DIFF_MM` | 90 | M4 | must measure |
| `CAL_UPPER_PRESENT_MM` | 600 | M4 | must measure |
| `CAL_BALL_DETECT_MAX_MM` | 500 | M2/M4 | must measure — prior cohorts ranged 28–60 cm |
| `CAL_BALL_GRAB_MM` | 120 | M2 | must measure — depends on scoop geometry |
| `CAL_HYSTERESIS_MM` | 60 | M4 | can run first |
| `CAL_SHARP_HALF_CONE_DEG` | 8 | M3 | can run first |
| `CAL_LINE_DIGITAL_MODE` | 1 (digital) | M5 | must measure — check for a trim pot |
| `CAL_LINE_ON_LEVEL` | 0 (active low) | M5 | can run first |
| `CAL_LINE_THRESH_*` ×4 | four different midpoints | M5 | must measure — analog mode only |
| `CAL_LINE_CONFIRM_COUNT` | 2 | M5 | can run first |
| `CAL_SWITCH_DIGITAL_PRESSED_LEVEL` | 0 | M6 | must measure — `sensorTouch` and `sensorDigitalIn` have opposite polarity |
| `CAL_SWITCH_ANALOG_PRESSED_MAX` | 2000 | M6 | can run first — wide margin |
| `CAL_SWITCH_CONFIRM_COUNT` | 2 | M6 | can run first |
| `CAL_COMPASS_ACTIVE_LOW` | 1 | M7 | can run first |
| `CAL_COMPASS_DRIVE_SUPPLY` | 1 | M7 | can run first |
| `CAL_COMPASS_CONFIRM_COUNT` | 2 | M7 | can run first |
| `CAL_ENC_TICKS_PER_M` | 1641 (theory: 360 counts/rev ÷ 219.4 mm) | M8 | must measure |
| `CAL_ENC_TRUSTWORTHY` | 0 (distrust by default) | M8 | must measure — depends on wheel type |

Notes:

- The three long-range Sharps currently share one set of coefficients. That is a
  simplification; after M2 they should probably be split into three.
- `CAL_SHARP_HALF_CONE_DEG` caps the search scan step (≤ 2×). Get it wrong and
  sweeps leave blind gaps. Pass this number to whoever writes the search.
- Line thresholds need measuring twice — lab card and venue tape have different
  reflectance.

## If there is only an hour

1. Fill the ports in `robot_config.h` — nothing runs otherwise
2. M2 Sharp distance — biggest effect, most time
3. M5 line thresholds — wrong here means driving out of bounds
4. M6 switch polarity — five minutes
5. Leave the rest on placeholders and tune while driving
