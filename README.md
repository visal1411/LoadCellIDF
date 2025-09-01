# HX711 IDF Component

An ESP-IDF component for interfacing with HX711 load cell amplifiers and weight sensors. This component provides a robust, thread-safe implementation for reading weight data from HX711 sensors connected to ESP32 microcontrollers.

## Features

- **Multiple Gain Support**: Supports HX711 gain settings for Channel A (128x, 64x) and Channel B (32x)
- **Robust Reading**: Includes error checking, deviation detection, and failure handling
- **Averaging**: Built-in support for reading multiple samples and calculating averages
- **Configurable Parameters**: Adjustable timeouts, deviation limits, and failure thresholds
- **Thread-Safe**: Uses portMUX_TYPE for safe multi-threaded operations
- **ESP-IDF Integration**: Designed specifically for ESP-IDF v4.0 and later

## Hardware Requirements

- ESP32 development board
- HX711 load cell amplifier module
- Load cell (typically 50kg, 100kg, 200kg, etc.)
- Jumper wires

## Pin Connections

| HX711 Pin | ESP32 GPIO | Description            |
|-----------|------------|------------------------|
| VCC       | 3.3V       | Power supply           |
| GND       | GND        | Ground                 |
| DT (MISO) | Any GPIO   | Data output from HX711 |
| SCK       | Any GPIO   | Clock input to HX711   |

## Installation

### Prerequisites
- [ESP-IDF](https://github.com/espressif/esp-idf) v4.0 or later
- ESP32 development board
- HX711 load cell amplifier

### Adding to Your Project

1.  Clone this repository:
    ```bash
    git clone https://github.com/ucukertz/hx711-idf hx711
    ```

2.  Add the hx711 component to your project's component folder:
    ```bash
    cp -r hx711/components/hx711 /path/to/your/esp-idf-project/components/
    ```
    Replace /path/to/your/esp-idf-project with the actual path to your ESP-IDF project.

4.  Include the header in your project's source code:
    ```c
    #include "hx711.h"
    ```

## API Reference

### Data Types

#### `hx711_gain_t`
Gain/channel configuration for HX711:
- HX711_GAIN_128 - Channel A with 128x gain (default)
- HX711_GAIN_32 - Channel B with 32x gain
- HX711_GAIN_64 - Channel A with 64x gain

#### `hx711_t`
HX711 device descriptor structure containing:
- gpio_num_t miso - GPIO for HX711 data pin (DT)
- gpio_num_t sck - GPIO for HX711 clock pin (SCK)
- hx711_gain_t gain - Current gain setting
- uint32_t timeout_ms - Wait timeout in milliseconds
- uint32_t max_deviation - Maximum allowed deviation for averaging
- uint8_t max_fail - Maximum allowed failures for averaging

### Functions

#### `bool hx711_init(hx711_t* hx711, gpio_num_t miso, gpio_num_t sck, hx711_gain_t gain)`
Initialize HX711 device descriptor. Parameters: pointer to HX711 device descriptor, GPIO for data pin, GPIO for clock pin, gain setting. Returns true if successful.

#### `bool hx711_set_gain(hx711_t* hx711, hx711_gain_t gain)`
Set the gain/channel for HX711 readings. Parameters: pointer to device descriptor, new gain setting. Returns true if successful.

#### `bool hx711_is_ready(hx711_t* hx711)`
Check if HX711 is ready for reading. Parameter: pointer to device descriptor. Returns true if ready.

#### `void hx711_set_wait_timeout(hx711_t* hx711, uint32_t timeout_ms)`
Set wait timeout for reading operations. Parameters: pointer to device descriptor, timeout in milliseconds.

#### `void hx711_set_max_deviation(hx711_t* hx711, uint32_t max_deviation)`
Set maximum acceptable deviation for averaging. Parameters: pointer to device descriptor, maximum allowed deviation.

#### `void hx711_set_max_fail(hx711_t* hx711, uint32_t max_fail)`
Set maximum acceptable failures for averaging. Parameters: pointer to device descriptor, maximum allowed failures.

#### `bool hx711_wait(hx711_t* hx711, size_t timeout_ms)`
Wait for HX711 readiness. Parameters: pointer to device descriptor, timeout in milliseconds. Returns true if ready within timeout.

#### `uint32_t hx711_read_raw(hx711_t* hx711)`
Read raw 24-bit data from HX711. Parameter: pointer to device descriptor. Returns raw 24-bit data.

#### `int32_t hx711_read_data(hx711_t* hx711)`
Read one parsed (signed 32-bit) HX711 value. Parameter: pointer to device descriptor. Returns parsed value.

#### `int32_t hx711_read_avg(hx711_t* hx711, uint16_t read_n)`
Read average of multiple parsed values with error handling. Parameters: pointer to device descriptor, number of readings to average. Returns average value or -1 on failure.

## Basic Usage
Include the headers and define a static HX711 device descriptor. Set GPIO pins for MISO (DT) and SCK. In app_main, initialize the HX711 with gain 128, configure parameters like max deviation, timeout, and max failures. Then in a loop, read average values and handle errors appropriately.

## Configuration Tips

### GPIO Selection
- Choose any available GPIO pins for DT (MISO) and SCK
- Ensure the pins are not used by other peripherals
- Avoid using strapping pins if possible

### Gain Settings
- HX711_GAIN_128: Most common, good for most load cells
- HX711_GAIN_64: Use when 128x gain is too high for your application
- HX711_GAIN_32: Channel B, typically used for different sensor types

### Error Handling Parameters
- max_deviation: Adjust based on noise levels in your environment
- max_fail: Increase if you experience frequent timeouts
- timeout_ms: Adjust based on your HX711's response time

## Troubleshooting

### Common Issues

1. **Always reading -1 or 0**
   - Check wiring connections
   - Verify GPIO pins are correct
   - Ensure HX711 is powered properly

2. **Unstable readings**
   - Increase max_deviation value
   - Add more samples to average (read_n parameter)
   - Check for electrical noise in the environment

3. **Timeout errors**
   - Increase timeout_ms value
   - Check HX711 clock frequency
   - Verify HX711 is functioning properly

## License

This project is licensed under the MIT License - see the LICENSE file for details.
