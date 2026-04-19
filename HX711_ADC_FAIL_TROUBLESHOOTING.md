# HX711 ADC Fail Troubleshooting

## Problem observed
During monitor output, the app sometimes prints:
- `ADC value read failed`

The board still boots and runs, but some HX711 samples are rejected.

## Current problem summary from latest logs
- Weight scale factor was off by about 10x: 500 ml water bottle (~0.5 kg) appeared near 5 kg.
- Load-cell polarity was inverted in calculation path (now fixed in code by using `tare_offset - read_avg`).
- Readings can drift for 10-60 seconds after placing load due to normal load-cell creep and mechanical settling.
- Occasional spikes can still happen from wiring/power noise.

## Current code-side fix status
- `HX711_COUNTS_PER_KG` was updated to `105000.0` as a temporary corrected value.
- Moving average smoothing is enabled to reduce jitter in displayed weight.
- Tare retry and read-fail fallback are enabled.

Note: `105000.0` is a practical starting value from your observed 10x error. Final accurate value should still be fine-tuned using a known reference weight.

## How to get the correct calibration number
Do not use a random number. Calculate it from a known reference weight.

Steps:
1. Let startup tare complete with no load.
2. Place a known weight (best: 1.000 kg; 0.500 kg is also okay).
3. Wait 30-60 seconds for settling.
4. Record 20 `Net` readings and average them to get `Net_avg`.
5. Compute:

```text
HX711_COUNTS_PER_KG = Net_avg / Known_mass_kg
```

6. Put the result into `HX711_COUNTS_PER_KG` in `main/app_main.c`.

Quick correction formula using displayed value:

```text
C_new = C_old * (W_display / W_real)
```

Example from this project:
- `C_old = 10500`
- `W_display = 5.0 kg`
- `W_real = 0.5 kg`

```text
C_new = 10500 * (5.0 / 0.5) = 105000
```

That is why `105000.0` is currently used as a temporary calibration value.

## Meaning of Weight, Filtered, Stable, and Spread
When logs print a line like this:

```text
Weight: 0.921 kg (filtered: 0.923 kg, stable: 0.923 kg, spread: 0.002 kg)
```

Use this interpretation:
- `Weight`: Instant converted value from the latest sample. Fastest response, highest jitter.
- `filtered`: Moving-average value. Smoother than `Weight`, but can lag after sudden load changes.
- `stable`: Locked value from the stability window logic. This is the value to trust for final reading.
- `spread`: Max-min range across the stability window (in kg). Lower spread means better stability.

Practical rule:
- Use `stable` when it is non-zero and `spread` is low.
- As a guideline in this project, `spread <= 0.010 kg` indicates a stable measurement window.
- If `stable` is `0.000 kg`, either the window is not stable yet or measured load is below stability threshold.

## Why weight is not stable (increases/decreases over time)
Small drift and bounce are normal with load-cell systems. Main causes:
- Load-cell creep: metal strain changes slowly after applying load.
- Mechanical settling: platform, screws, and mount structure settle over time.
- Temperature drift: HX711 and load cell change slightly as they warm up.
- Electrical noise: USB power noise, long wires, or poor ground connection.
- Vibration/air movement: touching table, fan airflow, cable movement.

How to reduce it:
- Wait 30-60 seconds before trusting final value.
- Keep the scale platform rigid and level.
- Use short, secure wires and stable 3.3V power.
- Avoid moving cables or touching the setup while measuring.
- Keep using filtered value from firmware (moving average).

## Why this happens
In this project, the message appears when `hx711_read_avg()` returns `-1`.
That can happen when:
- HX711 is not ready within timeout.
- Too many samples are rejected as invalid.
- Too many samples are rejected due to large deviation from the previous valid sample.

## What was changed in code to improve stability
`main/app_main.c` was updated to make reading more robust:
- Increased max deviation filter threshold for noisy environments.
- Increased allowed failed samples (`hx711_set_max_fail`).
- Increased wait timeout for HX711 readiness.
- Added tare retry loop at startup.
- Added runtime fallback: if one average read fails, keep using the last good value instead of blocking for a long time.

## Wiring checks
For this project defaults:
- HX711 DT (DOUT) -> ESP32 GPIO19
- HX711 SCK -> ESP32 GPIO18
- HX711 VCC -> ESP32 3.3V
- HX711 GND -> ESP32 GND

Load cell to HX711 (typical 4-wire):
- Red -> E+
- Black -> E-
- Green -> A+
- White -> A-

If values are unstable, check:
- Common ground is solid.
- Data wires are short and secure.
- USB cable quality (data cable, not charge-only).
- No heavy movement/vibration during tare.

## Flash/monitor command
Use one command:

```powershell
idf.py -p COM9 -b 115200 flash monitor
```

## Optional board configuration warning
If monitor shows flash-size mismatch (4MB detected, 2MB configured):
- Run `idf.py menuconfig`
- Open `Serial flasher config`
- Set `Flash size` to your real chip size (often 4MB)
