#include <driver/i2s_std.h>
#include <driver/gpio.h>
#include <math.h>
#include <WiFi.h>

#define I2S_MCLK        GPIO_NUM_2
#define I2S_BCLK        GPIO_NUM_5
#define I2S_WS          GPIO_NUM_6
#define I2S_DOUT        GPIO_NUM_7

#define SAMPLE_RATE     44100
#define BUFFER_SAMPLES  256
#define TONE_FREQ       440.0f

i2s_chan_handle_t tx_chan = NULL;
i2s_chan_handle_t rx_chan = NULL;
int32_t audioBuffer[BUFFER_SAMPLES * 2];

float phase = 0.0f;
const float phaseIncrement = (2.0f * M_PI * TONE_FREQ) / SAMPLE_RATE;

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
      .din  = I2S_GPIO_UNUSED
      .invert_flags = {false, false, false},
    },
  };
  std_cfg.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;

  ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_chan, &std_cfg));
  ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));
}

void setup() {
  WiFi.mode(WIFI_OFF);
  btStop();
  Serial.begin(115200);
  
  setupI2S();
  Serial.println("DAC Test Started: Outputting 440Hz Tone");
}

void loop() {
  for (int i = 0; i < BUFFER_SAMPLES; i++) {
    float sample = sinf(phase);
    phase += phaseIncrement;
    if (phase >= 2.0f * M_PI) phase -= 2.0f * M_PI;

    int32_t outSample = (int32_t)(sample * 2147483647.0f * 0.5f);

    audioBuffer[i * 2]     = outSample;
    audioBuffer[i * 2 + 1] = outSample;
  }

  size_t bytes_written = 0;
  i2s_channel_write(tx_chan, audioBuffer, sizeof(audioBuffer), &bytes_written, portMAX_DELAY);
}