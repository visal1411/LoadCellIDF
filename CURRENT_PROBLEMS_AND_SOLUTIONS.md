# HX711 Current Problems And Solutions

## Problem 1: Weight Drifts While Object Stays In Place
### What it looks like
- The object is not moved, but weight slowly changes over time.
- Example behavior: stable around one value, then shifts to another plateau.

### Most likely causes
- Mechanical creep in load cell and platform.
- Platform touching frame or side walls.
- Cable tension pulling on sensor or platform.
- Temperature drift in HX711 and load cell.

### Possible solutions
- Keep platform fully free (no rubbing or side contact).
- Fix one side of load cell rigidly, keep load side free as designed.
- Secure wires so they do not pull during measurement.
- Warm up system 3 to 5 minutes before tare.
- Keep object centered and avoid touching table/cables.

## Problem 2: Random Jumps Or Drop To 0 kg
### What it looks like
- Weight suddenly jumps high or low.
- Sometimes value falls to 0 even when object is present.

### Most likely causes
- Electrical noise or temporary raw data spikes.
- Short communication glitches or power noise.

### Possible solutions
- Keep DT and SCK wires short and secure.
- Use stable 3.3V supply and good USB data cable.
- Avoid USB hubs if possible.
- Firmware already includes spike rejection and recovery logic.

## Problem 3: Filtered Value Is Slow After Placing Object
### What it looks like
- Instant weight changes quickly, but filtered value rises slowly.

### Most likely causes
- Moving-average smoothing intentionally adds delay.

### Possible solutions
- Wait until stable value locks before reading final result.
- Current firmware includes fast step re-prime to reduce lag.

## Problem 4: Calibration Factor Not Accurate
### What it looks like
- Measured weight is consistently too high or too low.

### Most likely causes
- Counts-per-kg is not matched to your specific load cell and mount.

### Possible solutions
- Use known reference mass and compute:
- HX711_COUNTS_PER_KG = Net_avg / Known_mass_kg
- Repeat with same object position and average multiple runs.

## Problem 5: Unstable Reading On Empty Scale
### What it looks like
- Empty platform still shows small random weight.

### Most likely causes
- Noise floor and zero drift.
- Platform preload from mounting stress.

### Possible solutions
- Perform tare with completely empty and untouched platform.
- Keep environment vibration-free.
- Firmware uses near-zero deadband and auto-zero tracking.

## Problem 6: ESP32 Boots To Download Mode
### What it looks like
- Monitor shows boot:0x3 and waiting for download.

### Most likely causes
- GPIO0 low at reset.
- BOOT button held or strapping pin issue.

### Possible solutions
- Press EN once, ensure BOOT is not pressed.
- Check strapping pins are not pulled incorrectly.

## Problem 7: Flash Size Warning In Boot Logs
### What it looks like
- Detected flash size larger than binary header size.

### Most likely causes
- Project flash config set to 2MB while chip is 4MB.

### Possible solutions
- Open menuconfig.
- Go to Serial flasher config.
- Set flash size to actual chip size.

## How To Decide Which Weight Is Correct
### Use this rule
- Trust stable value when spread is low.
- Good condition for this project:
- spread at or below 0.010 kg, ideally 0.002 to 0.006 kg.

### Read workflow
1. Place object at center and do not touch setup.
2. Wait for stable field to become non-zero.
3. Use stable value as final reading.
4. Repeat 3 times and average for best accuracy.

## Quick Verification Checklist
1. Empty scale: filtered near 0 and no large jumps.
2. Loaded scale: stable locks and spread stays low.
3. No repeated recovery tare while loaded.
4. Same object, same position, repeated runs are close.