# HX711 20kg Load Cell Calibration Guide

## Problem
Your weight readings are noisy and/or inaccurate because the calibration constant `HX711_COUNTS_PER_KG` is estimated, not precise for your specific load cell + HX711 module.

## What Changed
The code now includes:
- **Moving Average Filter**: Smooths weight output by averaging the last 5 samples
- **Calibration logging**: Serial output now shows:
  - Raw ADC value
  - Net counts (after tare subtraction)
  - Raw weight (from estimated calibration)
  - **Filtered weight** (smoothed, more stable)
  - Current counts/kg factor

## Step-by-Step Calibration

### 1. Flash Updated Code
```powershell
idf.py -p COM9 -b 115200 flash monitor
```

### 2. Wait for Tare to Complete
Once you see:
```
I (3881) HX711: Tare offset: -419574
I (3881) HX711: Pin map: HX711 DT -> GPIO19, HX711 SCK -> GPIO18
```
The scale is ready (no load on it).

### 3. Place Known Reference Weight
- Place exactly **1 kg** (or a known weight you can verify) on the scale
- Ensure the weight is centered and stable
- Wait 10-15 seconds for readings to stabilize

### 4. Note the Filtered Weight and Net Counts
From serial output, watch for a stable line like:
```
I (570321) HX711: Raw: -416894, Net: 2680, Weight: 0.255 kg (filtered: 0.256 kg) / 20.0 kg (counts/kg: 10500.0)
```

**Key values to record:**
- `Net: 2680` ← your net counts for known weight
- `(filtered: 0.256 kg)` ← the filtered output
- If you placed 1.000 kg: then **HX711_COUNTS_PER_KG = 2680 / 1.0 = 2680.0**

### 5. Update Code and Rebuild
Edit [main/app_main.c](main/app_main.c):
```c
static float HX711_COUNTS_PER_KG = 2680.0f;  // Replace 10500.0 with your calibrated value
```

### 6. Flash and Verify
```powershell
idf.py -p COM9 -b 115200 flash monitor
```

Place your reference weight again and verify:
- **Filtered weight** should now read close to 1.000 kg (or whatever your reference was)
- If readings are still drifting, wait longer for stabilization before reading

## Expected Output After Calibration
```
I (570321) HX711: Raw: -414214, Net: 2680, Weight: 1.000 kg (filtered: 1.001 kg) / 20.0 kg (counts/kg: 2680.0)
I (571701) HX711: Raw: -414225, Net: 2669, Weight: 0.996 kg (filtered: 0.998 kg) / 20.0 kg (counts/kg: 2680.0)
I (573081) HX711: Raw: -414195, Net: 2699, Weight: 1.007 kg (filtered: 1.002 kg) / 20.0 kg (counts/kg: 2680.0)
```

Notice:
- Raw weight bounces ±0.005 kg
- Filtered weight is smooth around 1.000 kg
- Trend is stable

## If You Don't Have a Known Weight
If you can't source a reference weight:
- Use common items:
  - 1-liter water bottle ≈ 1.0 kg
  - 5 × 200g weight ≈ 1.0 kg
  - Food scale or digital scale to verify your reference

Alternatively, you can estimate:
1. Record net counts for an unknown weight
2. Measure that weight on a reliable scale
3. Calculate: HX711_COUNTS_PER_KG = net_counts / measured_weight

## Ongoing Adjustments
If readings still drift after calibration:
- **Increase WEIGHT_FILTER_SIZE** in [main/app_main.c](main/app_main.c) from 5 to 10
- **Check wiring** for loose connections
- **Verify power** is clean (no voltage dips during reads)

## Quick Command Cheat
Flash + monitor + auto-restart on recompile:
```powershell
idf.py -p COM9 -b 115200 flash monitor
```

Exit monitor: `Ctrl + ]`
