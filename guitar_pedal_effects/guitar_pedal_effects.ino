/*
  ESP32-S3 Guitar Pedal — Delay Only (Headless / Fixed 50% Mix & Feedback)
*/

#include <driver/i2s_std.h>
#include <driver/gpio.h>
#include <esp_heap_caps.h>
#include <math.h>
#include <WiFi.h>

// ---------------- Pin Definitions ----------------
#define I2S_MCLK        GPIO_NUM_2
#define I2S_BCLK        GPIO_NUM_5
#define I2S_WS          GPIO_NUM_6
#define I2S_DIN         GPIO_NUM_15   // ADC Data In
#define I2S_DOUT        GPIO_NUM_7    // DAC Data Out

// ---------------- Audio Config ----------------
#define SAMPLE_RATE          44100
#define BUFFER_SAMPLES         256    
#define NOISE_GATE_THRESHOLD  8000

i2s_chan_handle_t rx_chan = NULL;
i2s_chan_handle_t tx_chan = NULL;

int32_t audioBuffer[BUFFER_SAMPLES * 2];

// ---------------- Delay Buffer (2 seconds max) ----------------
#define DELAY_MAX_SECONDS   2
#define DELAY_MAX_SAMPLES   (SAMPLE_RATE * DELAY_MAX_SECONDS)
int16_t  *delayBuffer = nullptr;
uint32_t  delayWriteIdx   = 0;
uint32_t  delayLenSamples = SAMPLE_RATE / 2; // default 500 ms delay

// ---------------- Dynamic Limiter ----------------
const float LIMITER_THRESHOLD = 0.95f;
const float LIMITER_ATTACK    = 0.35f;
const float LIMITER_RELEASE   = 0.0005f;
float limiterEnvelope = 0.0f;

// ---------------- DC Blocking Filter State ----------------
float dcBlockX1 = 0.0f;
float dcBlockY1 = 0.0f;

// ---------------- Effect Parameters (Fixed) ----------------
bool delayOn = true;  
float feedbackSmoothed = 0.5f; // 50% feedback
float mixSmoothed      = 0.5f; // 50% mix

// ================= Helpers =================

void setupPSRAMBuffers() {
  delayBuffer = (int16_t *)heap_caps_malloc(DELAY_MAX_SAMPLES * sizeof(int16_t), MALLOC_CAP_SPIRAM);

  if (!delayBuffer) {
    while (1) delay(1000);
  }
  memset(delayBuffer, 0, DELAY_MAX_SAMPLES * sizeof(int16_t));
}

void setupI2S() {
  i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
  ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_chan, &rx_chan));

  i2s_std_config_t std_cfg = {
    .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
    .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
    .gpio_cfg = {
      .mclk = I2S_MCLK,
      .bclk = I2S_BCLK,
      .ws   = I2S_WS,
      .dout = I2S_DOUT,
      .din  = I2S_DIN,
      .invert_flags = {false, false, false},
    },
  };
  std_cfg.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;

  ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_chan, &std_cfg));
  ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_chan, &std_cfg));
  ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));
  ESP_ERROR_CHECK(i2s_channel_enable(rx_chan));
}

// ================= Setup =================

void setup() {
  WiFi.mode(WIFI_OFF);
  btStop();

  setupPSRAMBuffers();
  setupI2S();
}

// ================= Main Audio Loop (Core 1) =================

void loop() {
  size_t bytes_read = 0, bytes_written = 0;
  esp_err_t ret = i2s_channel_read(rx_chan, audioBuffer, sizeof(audioBuffer), &bytes_read, pdMS_TO_TICKS(50));

  if (ret == ESP_OK && bytes_read > 0) {
    int numFrames = (bytes_read / sizeof(int32_t)) / 2;

    for (int i = 0; i < numFrames; i++) {
      int32_t rawL = audioBuffer[i * 2];
      int32_t rawR = audioBuffer[i * 2 + 1];
      if (abs(rawL) < NOISE_GATE_THRESHOLD) rawL = 0;
      if (abs(rawR) < NOISE_GATE_THRESHOLD) rawR = 0;

      float sampleL = (rawL / 2147483648.0f) * 0.8f;
      float sampleR = (rawR / 2147483648.0f) * 0.8f;
      float sample = (sampleL + sampleR) * 0.5f;

      // ---- Delay Effect ----
      float processed = sample;
      if (delayOn) {
        uint32_t readIdx = (delayWriteIdx + DELAY_MAX_SAMPLES - delayLenSamples) % DELAY_MAX_SAMPLES;
        float delayed = delayBuffer[readIdx] / 32768.0f;

        float toStore = tanhf(sample + delayed * feedbackSmoothed);
        delayBuffer[delayWriteIdx] = (int16_t)(toStore * 32767.0f);
        delayWriteIdx = (delayWriteIdx + 1) % DELAY_MAX_SAMPLES;

        processed = sample * (1.0f - mixSmoothed) + delayed * mixSmoothed;
      }

      // ---- Dynamic Limiter ----
      float absVal = fabsf(processed);
      if (absVal > limiterEnvelope) {
        limiterEnvelope += (absVal - limiterEnvelope) * LIMITER_ATTACK;
      } else {
        limiterEnvelope += (absVal - limiterEnvelope) * LIMITER_RELEASE;
      }
      float limiterGain = (limiterEnvelope > LIMITER_THRESHOLD)
                          ? (LIMITER_THRESHOLD / limiterEnvelope)
                          : 1.0f;
      processed *= limiterGain;

      // ---- DC Blocking Filter ----
      float dcFiltered = processed - dcBlockX1 + 0.995f * dcBlockY1;
      dcBlockX1 = processed;
      dcBlockY1 = dcFiltered;
      processed = dcFiltered;

      if (processed > 1.0f)  processed = 1.0f;
      if (processed < -1.0f) processed = -1.0f;
      int32_t outSample = (int32_t)(processed * 2147483647.0f);

      audioBuffer[i * 2]     = outSample;
      audioBuffer[i * 2 + 1] = outSample;
    }

    i2s_channel_write(tx_chan, audioBuffer, bytes_read, &bytes_written, portMAX_DELAY);
  }
}