# Port Map

Fill the "actual" column once the group decides, then copy into the TODO block at
the top of `robot_config.h`. This table is also the draft for the wiring diagram
in the report.

Cortex: 8 analog (in1–in8), 12 digital (dgtl1–dgtl12).

## Decision needed first

Digital demand is 17, we have 12. Pick one (reasoning in `SENSOR_SCOPE.md`):

| | Limit switches | Encoders | Cost |
|---|---|---|---|
| **A** (suggested) | 4 | 1 | Lose an encoder. 8 of 12 prior repos used none at all. |
| **B** | 3 | 2 | Lose a switch. Not viable if the mechanism needs top + bottom + ball-present + bumper. |

Selected: ☐ A ☐ B

## Proposed assignment (option A)

### Analog

| Port | Device | Use | Actual |
|---|---|---|---|
| in1 | Sharp 10–80 | front lower — ball detection | |
| in2 | Sharp 10–80 | front upper — opponent detection | |
| in3 | Sharp 10–80 | side scan | |
| in4 | Sharp 4–30 | scoop mouth — close range | |
| in5 | Limit switch | scoop top stop | |
| in6 | Limit switch | scoop bottom stop | |
| in7 | Limit switch | ball present | |
| in8 | Compass | supply pin (declare `sensorNone`) | |

### Digital

| Port | Device | Use | Actual |
|---|---|---|---|
| dgtl1–4 | IR line | FL, FR, RL, RR | |
| dgtl5–8 | Compass | N, E, S, W | |
| dgtl9 | Limit switch | front bumper | |
| dgtl10 | Shaft encoder | left wheel A (`sensorQuadEncoder` also takes dgtl11) | |
| dgtl11 | Shaft encoder | left wheel B | |
| dgtl12 | — | spare (start switch?) | |

### Motors (not our scope, listed for completeness)

port1 and port10 are the 2-wire ports with built-in H-bridges — put the 393 drive
motors there and no Motor Controller 29 is needed. Collector and lift go on
port2–3, each via an MC29.

## Wiring notes

- The compass supply pin must be driven high before the compass outputs anything.
- IR line threshold is set by the trim pot on the module, not in code. It has to
  be adjusted against the actual yellow tape at the venue.
- Sharp cables are 3-wire (signal / power / ground). Reversing them destroys the part.
- `sensorQuadEncoder` is declared on the first port and silently takes the next one too.
- There is no dedicated start switch in the kit. Prior cohorts repurposed a limit
  switch. All four of ours are allocated, so we need another idea — e.g. a long
  press on the bumper switch.
