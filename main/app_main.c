#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <sdkconfig.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

#include "hx711.h"

static const char* TAG = "HX711";

static hx711_t hx711;

const gpio_num_t GPIO_HX711_MISO    = GPIO_NUM_19;
const gpio_num_t GPIO_HX711_SCK     = GPIO_NUM_18;
static const uint32_t HX711_CALIB_DEVIATION_MAX = 300000;
static const uint8_t HX711_CALIB_MAX_FAIL = 50;
static const uint8_t HX711_CALIB_AVG_READ = 4;
static const uint8_t HX711_TARE_AVG_READ = 10;
static const uint8_t HX711_TARE_RETRY_MAX = 5;
static const uint8_t HX711_FAIL_RECOVERY_THRESHOLD = 5;
static const int32_t HX711_ZERO_BAND_COUNTS = 5500;
static const int32_t HX711_AUTO_ZERO_COUNTS = 7000;
static const int32_t HX711_RECOVERY_TARE_ALLOWED_COUNTS = 8000;
static const float HX711_DISPLAY_ZERO_BAND_KG = 0.050f;
static const float HX711_STEP_REPRIME_KG = 0.080f;
static const int32_t HX711_SPIKE_REJECT_COUNTS = 50000;
static const uint8_t HX711_SPIKE_CONFIRM_READS = 1;
static const uint8_t HX711_STABILITY_WINDOW = 12;
static const float HX711_STABILITY_SPREAD_KG = 0.010f;
static const float HX711_STABILITY_MIN_KG = 0.050f;

// 20kg load-cell setup values.
// counts_per_kg must be calibrated for your exact load-cell + HX711 module.
static const float LOAD_CELL_CAPACITY_KG = 20.0f;
// Temporary calibration from observed behavior: 500 ml bottle (~0.5 kg) was read as ~5 kg,
// so counts_per_kg needs roughly 10x increase from 10500.
static float HX711_COUNTS_PER_KG = 105000.0f;

// Simple moving average filter for weight stability
// Larger filter = slower response but more stable readings
#define WEIGHT_FILTER_SIZE 15
static float weight_filter[WEIGHT_FILTER_SIZE] = {0};
static uint8_t filter_index = 0;
static uint8_t filter_ready = 0;
static float stability_buf[32] = {0};
static uint8_t stability_index = 0;
static uint8_t stability_count = 0;

static void reset_weight_filter(void)
{
  for (uint8_t i = 0; i < WEIGHT_FILTER_SIZE; i++) {
    weight_filter[i] = 0.0f;
  }
  filter_index = 0;
  filter_ready = 0;
}

static void reset_stability_buffer(void)
{
  for (uint8_t i = 0; i < 32; i++) {
    stability_buf[i] = 0.0f;
  }
  stability_index = 0;
  stability_count = 0;
}

static void push_stability_sample(float weight_kg)
{
  stability_buf[stability_index] = weight_kg;
  stability_index = (stability_index + 1) % HX711_STABILITY_WINDOW;
  if (stability_count < HX711_STABILITY_WINDOW) {
    stability_count++;
  }
}

static bool get_stable_weight(float* stable_avg, float* spread)
{
  if (stability_count < HX711_STABILITY_WINDOW) {
    return false;
  }

  float sum = 0.0f;
  float min_w = stability_buf[0];
  float max_w = stability_buf[0];

  for (uint8_t i = 0; i < HX711_STABILITY_WINDOW; i++) {
    float w = stability_buf[i];
    sum += w;
    if (w < min_w) {
      min_w = w;
    }
    if (w > max_w) {
      max_w = w;
    }
  }

  *stable_avg = sum / HX711_STABILITY_WINDOW;
  *spread = max_w - min_w;

  if (*stable_avg < HX711_STABILITY_MIN_KG) {
    return false;
  }

  return *spread <= HX711_STABILITY_SPREAD_KG;
}

float apply_weight_filter(float new_weight)
{
  if (filter_ready == 0) {
    // Prime the filter to avoid slow startup ramp from initial zeros.
    for (uint8_t i = 0; i < WEIGHT_FILTER_SIZE; i++) {
      weight_filter[i] = new_weight;
    }
    filter_ready = WEIGHT_FILTER_SIZE;
    filter_index = 1 % WEIGHT_FILTER_SIZE;
    return new_weight;
  }

  weight_filter[filter_index] = new_weight;
  filter_index = (filter_index + 1) % WEIGHT_FILTER_SIZE;
  
  // Track if we've filled the buffer at least once
  if (filter_ready < WEIGHT_FILTER_SIZE) {
    filter_ready++;
  }
  
  float sum = 0.0f;
  uint8_t count = (filter_ready < WEIGHT_FILTER_SIZE) ? filter_ready : WEIGHT_FILTER_SIZE;
  for (uint8_t i = 0; i < count; i++) {
    sum += weight_filter[i];
  }
  return sum / count;
}

void app_main(void)
{
  hx711_init(&hx711, GPIO_HX711_MISO, GPIO_HX711_SCK, HX711_GAIN_128);
  hx711_set_max_deviation(&hx711, HX711_CALIB_DEVIATION_MAX);
  hx711_set_max_fail(&hx711, HX711_CALIB_MAX_FAIL);
  hx711_set_wait_timeout(&hx711, 1500);

  ESP_LOGI(TAG, "Taring... Make sure no load is on the scale");

  int32_t tare_offset = -1;
  for (uint8_t attempt = 1; attempt <= HX711_TARE_RETRY_MAX; attempt++) {
    tare_offset = hx711_read_avg(&hx711, HX711_TARE_AVG_READ);
    if (tare_offset != -1) {
      break;
    }
    ESP_LOGW(TAG, "Tare attempt %u/%u failed, retrying...", attempt, HX711_TARE_RETRY_MAX);
    vTaskDelay(pdMS_TO_TICKS(300));
  }

  if (tare_offset == -1) {
    ESP_LOGE(TAG, "Tare failed, check wiring/power and restart");
    return;
  }

  ESP_LOGI(TAG, "Tare offset: %ld", (long)tare_offset);
  ESP_LOGI(TAG, "Pin map: HX711 DT -> GPIO19, HX711 SCK -> GPIO18");

  int32_t last_good_avg = tare_offset;
  int32_t prev_read_avg = tare_offset;
  uint8_t spike_count = 0;
  uint32_t consecutive_fail = 0;
  bool stable_locked = false;
  float stable_weight_kg = 0.0f;
  float last_filtered_kg = 0.0f;

  while (1) {
    int32_t read_avg = hx711_read_avg(&hx711, HX711_CALIB_AVG_READ);
    if (read_avg == -1) {
      consecutive_fail++;
      ESP_LOGW(TAG, "ADC read failed (%lu)", (unsigned long)consecutive_fail);

      // Try single-shot recovery before falling back to stale data.
      if (hx711_wait(&hx711, 500)) {
        read_avg = hx711_read_data(&hx711);
        last_good_avg = read_avg;
        consecutive_fail = 0;
      } else {
        read_avg = last_good_avg;
        vTaskDelay(pdMS_TO_TICKS(300));
      }

      // If failures keep happening, re-sync gain. Re-tare only if scale is near zero-load.
      if (consecutive_fail >= HX711_FAIL_RECOVERY_THRESHOLD) {
        ESP_LOGW(TAG, "Recovering HX711 sync and tare baseline...");
        hx711_set_gain(&hx711, HX711_GAIN_128);

        int32_t load_from_tare = (int32_t)labs((long)(last_good_avg - tare_offset));
        if (load_from_tare <= HX711_RECOVERY_TARE_ALLOWED_COUNTS) {
          int32_t tare_retry = hx711_read_avg(&hx711, HX711_TARE_AVG_READ);
          if (tare_retry != -1) {
            tare_offset = tare_retry;
            last_good_avg = tare_retry;
            reset_weight_filter();
            reset_stability_buffer();
            stable_locked = false;
            stable_weight_kg = 0.0f;
            consecutive_fail = 0;
            ESP_LOGI(TAG, "Recovery tare offset (near zero-load): %ld", (long)tare_offset);
          }
        } else {
          ESP_LOGW(TAG, "Skip recovery tare: load detected (delta=%ld counts)", (long)load_from_tare);
          consecutive_fail = 0;
        }
      }
    } else {
      last_good_avg = read_avg;
      consecutive_fail = 0;
    }

    // Reject one-off large jumps that usually come from wiring/power/mechanical glitches.
    int32_t delta = read_avg - prev_read_avg;
    if (labs((long)delta) > HX711_SPIKE_REJECT_COUNTS) {
      spike_count++;
      if (spike_count < HX711_SPIKE_CONFIRM_READS) {
        ESP_LOGW(TAG, "Spike rejected: prev=%ld now=%ld delta=%ld", (long)prev_read_avg, (long)read_avg, (long)delta);
        read_avg = prev_read_avg;
      } else {
        // Accept if large change repeats, then reset spike confirmation state.
        spike_count = 0;
      }
    } else {
      spike_count = 0;
    }
    prev_read_avg = read_avg;
    last_good_avg = read_avg;

    // Positive net is expected when load is applied in this wiring setup.
    int32_t net_counts = read_avg - tare_offset;

    // Slowly compensate zero drift when the scale is effectively unloaded.
    if (net_counts < HX711_AUTO_ZERO_COUNTS && net_counts > -HX711_AUTO_ZERO_COUNTS) {
      tare_offset = (int32_t)((tare_offset * 99L + read_avg) / 100L);
      net_counts = read_avg - tare_offset;
      ESP_LOGD(TAG, "Auto-zero adjust: tare=%ld", (long)tare_offset);
    }

    // Ignore tiny near-zero changes so unloaded readings remain stable around zero.
    if (net_counts < HX711_ZERO_BAND_COUNTS && net_counts > -HX711_ZERO_BAND_COUNTS) {
      net_counts = 0;
    }

    float weight_kg = (float)net_counts / HX711_COUNTS_PER_KG;

    if (weight_kg < 0.0f) {
      weight_kg = 0.0f;
    }

    // If load changes quickly, re-prime filter so displayed value catches up faster.
    if (filter_ready > 0 && fabsf(weight_kg - last_filtered_kg) > HX711_STEP_REPRIME_KG) {
      reset_weight_filter();
      reset_stability_buffer();
      stable_locked = false;
      stable_weight_kg = 0.0f;
    }

    // Apply moving average filter for stability
    float weight_filtered = apply_weight_filter(weight_kg);

    // Keep near-zero display stable instead of showing grams-level jitter.
    if (weight_filtered < HX711_DISPLAY_ZERO_BAND_KG) {
      weight_filtered = 0.0f;
    }
    last_filtered_kg = weight_filtered;

    push_stability_sample(weight_filtered);
    float stable_avg = 0.0f;
    float stable_spread = 0.0f;
    bool is_stable = get_stable_weight(&stable_avg, &stable_spread);

    if (is_stable) {
      stable_locked = true;
      stable_weight_kg = stable_avg;
    } else if (weight_filtered < (HX711_STABILITY_MIN_KG * 0.5f)) {
      stable_locked = false;
      stable_weight_kg = 0.0f;
    }

    ESP_LOGI(TAG, "Raw: %ld, Net: %ld, Weight: %.3f kg (filtered: %.3f kg, stable: %.3f kg, spread: %.3f kg) / %.1f kg (counts/kg: %.1f)",
      (long)read_avg,
      (long)net_counts,
      weight_kg,
      weight_filtered,
      stable_locked ? stable_weight_kg : 0.0f,
      stable_spread,
      LOAD_CELL_CAPACITY_KG,
      HX711_COUNTS_PER_KG);

    if (weight_filtered > LOAD_CELL_CAPACITY_KG) {
      ESP_LOGW(TAG, "Overload: measured weight exceeds %.1f kg", LOAD_CELL_CAPACITY_KG);
    }

    // Slow read interval to allow load cell settling time between measurements
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
