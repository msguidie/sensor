# Sensor Modules

## Files

| File | Contains | Edited |
|---|---|---|
| `robot_config.h` | Ports, mechanical constants | Once, after ports are fixed |
| `sensor_cal.h` | Every calibration constant (no magic numbers in `sensors.c`) | After bench calibration |
| `sensors.h` | Public interface | Rarely — changes affect callers |
| `sensors.c` | Implementation | Yiheng |
| `cal_harness.c` | Standalone calibration tool, not in the competition build | Yiheng |

## Usage

```c
sensInit();
while (true) {
    sensUpdate();      // once per loop, non-blocking
    /* query functions below */
}
```

Call these rather than reading `SensorValue[]`, so port and calibration changes
stay inside this layer.

## Interface

```c
/* distance — DIST_BALL / DIST_UPPER / DIST_SIDE / DIST_NEAR */
int  sensDistMm(tDistSensor s);            /* -1 if not trustworthy */
bool sensDistValid(tDistSensor s);
int  sensDistRaw(tDistSensor s);           /* debug */

/* targets */
bool sensBallSeen(int *bearingDeg, int *rangeMm);   /* outputs may be NULL */
bool sensOpponentSeen(void);
bool sensBallInGrabRange(void);            /* close enough to move the scoop */
bool sensBallHeld(void);                   /* ball is in the collector */

/* edges */
int  sensEdgeMask(void);                   /* EDGE_MASK_FL | FR | RL | RR */
bool sensEdge(tEdgePos p);

/* mechanism */
bool sensScoopAtTop(void);
bool sensScoopAtBottom(void);
bool sensBumperHit(void);

/* heading — 0/45/.../315, or SENS_NO_HEADING */
int  sensHeadingDeg(void);
bool sensHeadingValid(void);

/* odometry */
long sensEncTicks(tSide s);
long sensEncDistMm(tSide s);
void sensEncReset(void);
bool sensEncTrusted(void);                 /* false for omni wheels */

void sensDumpState(void);                  /* all state to debug stream */
```

## Notes per group

**Distance.** Chain is raw → median-of-3 → IIR → hyperbolic linearisation →
fold-back rejection → N consecutive readings in range → publish. Returns -1
rather than a guess when out of range or in the fold-back region. Handle it.

**Ball vs opponent.** The two stacked Sharps are compared against each other, not
against absolute thresholds, so calibration drift does not break it:

```
lower sees, upper does not          -> ball
lower sees, upper sees, similar     -> opponent
lower sees, upper sees, far apart   -> ball in front of opponent
```

Detection has hysteresis so a target near the threshold does not flicker.

**Edges.** Read all four at once and switch on the mask. Diagonal combinations
(FL|RR, FR|RL) are physically impossible and all-four is almost certainly noise —
worth handling defensively. Digital or analog wiring both work, via
`CAL_LINE_DIGITAL_MODE`.

**Switches.** Can sit on analog ports, which is the only way out when digital
ports run short. Which port type each uses is recorded in `robot_config.h`.

**Heading.** 45° resolution — enough to point roughly at the delivery box, not to
aim. Not yet issued, so this currently returns invalid; strategy must cope
without it. Decoding is structural rather than a lookup table (one active pin =
cardinal, two adjacent pins on the N-E-S-W ring = the diagonal between them), so
it does not depend on which wire goes to which port. Illegal codes hold the last
good value.

**Odometry.** Snapshot difference, not counter zeroing, to avoid racing the
interrupt count. Omni wheels make it drift; `sensEncTrusted()` then returns false
and navigation should use the compass.

## Two rules

1. `sensors.c` never writes `motor[]`.
2. `sensUpdate()` contains no waits. The Cortex is cooperatively scheduled, so a
   wait in the sensing path freezes edge detection too and the robot drives out
   of bounds.
