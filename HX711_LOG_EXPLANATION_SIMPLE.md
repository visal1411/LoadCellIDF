# HX711 Log Explanation (Simple Words)

## Why the weight is not stable

Your reading is drifting a little, so it does not lock as **stable**.

Three things are happening:

1. The reading changes slightly over time, so your stability condition is not reached.
2. HX711 read sometimes fails or times out.
3. CPU0 watchdog triggers because code waits too long in the HX711 read path.

You saw values around `0.965 kg` because the load was hovering there for a while, not because it became truly stable.

Also, in your log, `stable: 0.000 kg` means your firmware never accepted a final stable value at that moment.

## Simple meaning of log words

- **Raw**: direct ADC number from HX711 (just sensor counts, not kg).
- **Net**: `Raw - tare_offset` (counts after tare/zero removed).
- **Weight**: net value converted to kg using calibration.
- **filtered**: smoothed weight (less jumpy).
- **stable**: final locked value only when stability rule is met.
- **spread**: difference between recent samples (small spread = more steady).
- **counts/kg**: calibration factor, how many HX711 counts equal 1 kg.

## What the error log means

### `ADC read failed (1)`

HX711 data was not ready in time, or read timed out.

### `task_wdt ... IDLE0 did not reset`

The watchdog says CPU0 was blocked too long. In your trace, it happened in HX711 wait/read functions, so the idle task could not run enough.

## Quick practical causes of unstable readings

- Mechanical vibration or touching the platform
- Load cell not firmly mounted or side force
- Noisy power supply or weak grounding
- Long/noisy HX711 wires
- Temperature drift (no warm-up)
- Too little filtering

## What to do

1. Increase filtering and require longer quiet time before setting `stable`.
2. Make HX711 wait timeout faster and handle failures without long blocking.
3. Ensure wait loops yield often so watchdog does not trigger.
4. Improve wiring, grounding, and mechanical mounting.
5. Do tare and calibration after 2-5 minutes warm-up.
