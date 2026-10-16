#include <driver/i2s_std.h>
#include <driver/gpio.h>
#include <math.h>
#include <WiFi.h>

// ---------------- Working Pin Definitions ----------------
#define I2S_MCLK        GPIO_NUM_2
#define I2S_BCLK        GPIO_NUM_5
#define I2S_WS          GPIO_NUM_6
#define I2S_DIN         GPIO_NUM_15   // ADC Data In
#define I2S_DOUT        GPIO_NUM_7    // DAC Data Out

#define SAMPLE_RATE          44100
#define BUFFER_SAMPLES       256    
#define NOISE_GATE_THRESHOLD 8000

i2s_chan_handle_t rx_chan = NULL;
i2s_chan_handle_t tx_chan = NULL;

int32_t audioBuffer[BUFFER_SAMPLES * 2];

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

void setup() {
  WiFi.mode(WIFI_OFF);
  btStop();
  Serial.begin(115200);

  setupI2S();
  Serial.println("ADC Pass-Through Test Started: Play your instrument to monitor input");
}

void loop() {
  size_t bytes_read = 0, bytes_written = 0;
  esp_err_t ret = i2s_channel_read(rx_chan, audioBuffer, sizeof(audioBuffer), &bytes_read, pdMS_TO_TICKS(50));

  if (ret == ESP_OK && bytes_read > 0) {
    int numFrames = (bytes_read / sizeof(int32_t)) / 2;

    for (int i = 0; i < numFrames; i++) {
      int32_t rawL = audioBuffer[i * 2];
      int32_t rawR = audioBuffer[i * 2 + 1];

      // Simple noise gate
      if (abs(rawL) < NOISE_GATE_THRESHOLD) rawL = 0;
      if (abs(rawR) < NOISE_GATE_THRESHOLD) rawR = 0;

      // Direct copy (dry monitor loopback)
      audioBuffer[i * 2]     = rawL;
      audioBuffer[i * 2 + 1] = rawR;
    }

    i2s_channel_write(tx_chan, audioBuffer, bytes_read, &bytes_written, portMAX_DELAY);
  }
}