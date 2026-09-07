# Sensor Scope

Owner: Yiheng · VEX Cortex + ROBOTC

## What this layer does

Reads sensors, filters them, converts raw counts to physical units, and answers
questions like "is there a ball ahead" and "am I on the boundary line".

It does not drive motors, move the collector, or make strategy decisions.
`sensors.c` contains zero `motor[]` writes and no blocking waits.

## Kit

| Sensor | Qty | Interface | Ports each |
|---|---|---|---|
| Sharp 10–80 cm | 3 | analog | 1 analog |
| Sharp 4–30 cm | 1 | analog | 1 analog |
| IR line module | 4 | digital (onboard comparator) | 1 digital |
| Limit switch | 4 | mechanical contact | 1 digital or 1 analog |
| Shaft encoder | 2 | quadrature | **2 digital** |
| Digital Compass 1490 | 1 | 4 digital outputs + supply | 5 (supply can go on analog) |

Two things worth noting, both confirmed across 12 prior-year repos:
the compass is **not I2C** — it is a 4-bit parallel digital output; and the IR
line modules are **digital**, not analog.

## Port budget: we are 5 digital short

Cortex has 8 analog and 12 digital. Running everything needs 17 digital
(4 line + 4 compass + 1 supply + 4 encoder + 4 switch).

Two mitigations work (both used by prior cohorts): put the compass supply pin on
an analog port declared `sensorNone`, and read limit switches on analog ports.
That still leaves a choice:

| Option | Limit switches | Encoders |
|---|---|---|
| **A** (suggested) | 4 | 1 |
| **B** | 3 | 2 |

4 switches + 2 encoders is not possible.

Option A is suggested because no prior cohort used encoders for chassis odometry
— all navigated by compass heading plus timed dead reckoning — and omni wheels
make odometry drift anyway.

**This needs a group decision.** It affects both the mechanism (how many switches
are available) and navigation (whether we can rely on distance travelled).

## What each sensor is for

**Sharp ×4.** Two mounted stacked: the lower one aimed at ball height, the upper
one above ball height so it can only see the opponent robot. Lower sees something
and upper does not → ball. Both see it at a similar distance → opponent. This is
the standard solution for this course and the only way we distinguish a ball from
the other robot. Third Sharp scans off to the side. Fourth is the 4–30 cm part at
the scoop mouth, used for close-range confirmation — the long-range parts fold
back below ~10 cm and report nonsense exactly when we are about to grab.

**IR line ×4.** One at each corner. Boundary is yellow reflective tape; on the
tape reads 0. All four corners are needed because we reverse into the delivery
box rather than turning around.

**Limit switches ×4.** Scoop top stop, scoop bottom stop, ball-present, front
bumper. Ball-present is the important one — it is the only thing that confirms a
ball is actually in the collector.

**Shaft encoders.** Distance travelled. If the wheels are omni, lateral slip makes
this drift continuously and `sensEncTrusted()` returns false.

**Compass.** Heading, 45° resolution, 8 sectors. Good enough for "point roughly at
the delivery box", not for precise alignment. Marked "will be issued later" and
has not arrived, so `sensHeadingValid()` currently returns false — strategy must
work without it.

## Open questions (need the physical parts)

1. Do the IR line modules have a trim pot on the board? Decides digital vs analog,
   which feeds back into the port budget.
2. Omni wheels or plain? Decides whether encoder odometry is usable.
3. When is the compass issued?
