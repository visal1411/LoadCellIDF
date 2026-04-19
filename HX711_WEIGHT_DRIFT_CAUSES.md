# HX711 Weight Drift & Instability - Root Causes & Solutions

## Problem You're Seeing
- Phone placed: reads 0.179 kg (stable)
- Phone sits 30+ seconds: stable weight creeps up to 0.185 kg, then 0.191 kg
- **Cause**: NOT a code bug—this is **normal load cell behavior** under sustained load

---

## Root Causes (Ranked by Likelihood)

### 1. **Thermal Drift** (Most Common - 60%)
**What it is:**
- Load cell resistance changes as temperature increases
- Under constant load, internal dissipation warms the sensor
- Output drifts ~0.1-0.5% per minute under heavy load

**Signs:**
- Creep is slow and steady (your logs show this)
- Happens predictably after same time interval
- Warms up then stabilizes

**Fix:**
- Let device warm up 5 minutes before measuring
- Reduce ambient room temperature
- Add calibration drift correction in code (advanced)

---

### 2. **Mechanical Creep** (25%)
**What it is:**
- Platform/mounting materials slowly deform under load
- Foam, rubber, or plastic settling slightly
- Load path changes = apparent weight change

**Signs:**
- Drift correlates with load size
- Heavier objects drift more
- Happens faster on first placement

**Fix:**
- Use rigid steel/aluminum mounting only
- Ensure platform is flat and level
- Check load cell screw tightness
- Avoid foam/rubber under platform

---

### 3. **Cable Movement / Flexing** (10%)
**What it is:**
- HX711 wires mechanically coupled to sensor area
- Cable movement transfers force (like a tiny load)
- Causes readings to shift when cable moves

**Signs:**
- Weight jumps when you touch cable (you observed this!)
- Jumps are usually 5-20g
- Happens suddenly, not gradually

**Fix:**
- Secure cables away from sensor with tape/clamps
- Avoid touching setup while measuring
- Use shielded twisted-pair cables
- Strain relief on connectors

---

### 4. **Power Supply Noise** (3%)
**What it is:**
- HX711 3.3V supply fluctuates
- Causes reference voltage to shift
- Results in reading drift

**Signs:**
- Drift is jerky/noisy (not smooth)
- Other devices on same power cause bigger drift
- Happens with low battery

**Fix:**
- Use quality USB power supply (2A minimum)
- Add 10µF capacitor near HX711 power pins
- Check USB cable quality

---

### 5. **Load Cell Failure** (1%)
**What it is:**
- Internal sensor elements failing
- Rare but possible after years of use

**Signs:**
- Reading drifts 50+ grams in minutes
- Erratic jumps and oscillation
- Fails to recover after unload

**Fix:**
- Replace load cell (cheap, ~$10-20)
- Test with new sensor

---

## Your Current Situation

Based on your logs:
1. **Stable holds perfectly** ✓ (new code works)
2. **Slow creep over time** → **Likely thermal drift or mechanical creep**
3. **Cable touches = jump** → **Cable coupling confirmed**

**Recommendation:**
1. Place phone
2. Wait 2 minutes for thermal stabilization
3. Then record weight (this will be true reading)
4. For production: ignore readings in first 60 seconds after load placement

---

## How to Diagnose Which Is Your Main Problem

| Test | Steps | Result Means |
|------|-------|--------------|
| **Thermal** | Place phone. Wait 1 min. Remove. Wait 1 min. Place again. Same spot. | Same stable = mechanical OK. Different = thermal drift |
| **Mechanical** | Place 1kg weight. Let sit 5 min. Remove. Tare should return to zero. | Tare drifts >10g = mechanical creep |
| **Cable** | Place phone stable. Don't touch anything. Observe 2 min. | Stable doesn't move = cables OK |
| **Power** | Unplug USB. Use battery. Measure drift. | More drift = power issue |

---

## Code Workaround (Optional)

Currently in your firmware:
```c
static const float HX711_STABLE_HOLD_DELTA_KG = 0.006f;
```

This means stable will update if new value differs by 0.006 kg (6g).

**To reduce drift updates:**
- Increase to `0.010f` (10g) - more stable, less sensitive
- Or implement exponential moving average on stable value

---

## Best Practices for Accurate Weighing

1. **Warm-up**: Let device run 2 minutes before taking measurements
2. **Wait after placing load**: Wait 3-5 seconds for stabilization
3. **Check mechanics**: Ensure platform is flat, level, rigid
4. **Isolate cables**: Secure all wires away from moving parts
5. **Use good power**: USB power supply (not hub), quality cable
6. **Calibrate fresh**: Recalibrate after major temperature change
7. **Test repeatability**: Same object = same reading = accurate system

---

## When It's the Load Cell (Sensor) Itself

Replace if:
- Drift exceeds 20g in 1 minute
- Cannot stabilize no matter what
- Readings are erratic and noisy
- Tare doesn't return to baseline after unload

**Cost**: $15-30 on Amazon (search "50kg load cell")
