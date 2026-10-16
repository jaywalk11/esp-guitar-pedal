// TEST 1 (CORRECTED): 24-bit DAC Tone Playback with Slow Heartbeat
#include <driver/i2s_std.h>
#include <driver/gpio.h>
#include <math.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#define I2S_MCLK    GPIO_NUM_2
#define I2S_BCLK    GPIO_NUM_5
#define I2S_WS      GPIO_NUM_6
#define I2S_DOUT    GPIO_NUM_7
#define ONBOARD_LED GPIO_NUM_21  

#define SAMPLE_RATE     44100
#define TONE_FREQ       440.0f
#define BUFFER_SAMPLES  256

i2s_chan_handle_t tx_chan = NULL;
int32_t audioBuffer[BUFFER_SAMPLES * 2];

void setup() {
  REG_WRITE(RTC_CNTL_BROWN_OUT_REG, 0); // Standalone power safety
  
  pinMode(ONBOARD_LED, OUTPUT);
  digitalWrite(ONBOARD_LED, HIGH); // Solid ON at start

  Serial.begin(115200);
  delay(2000);

  // Slow initial confirmation blink
  digitalWrite(ONBOARD_LED, LOW);
  delay(1000);
  digitalWrite(ONBOARD_LED, HIGH);

  i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
  ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_chan, NULL));

  i2s_std_config_t std_cfg = {
    .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
    .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_24BIT, I2S_SLOT_MODE_STEREO),
    .gpio_cfg = {
      .mclk = I2S_MCLK,
      .bclk = I2S_BCLK,
      .ws   = I2S_WS,
      .dout = I2S_DOUT,
      .din  = I2S_GPIO_UNUSED,
      .invert_flags = {false, false, false},
    },
  };
  std_cfg.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;

  ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_chan, &std_cfg));
  ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));
  Serial.println("[+] Playing test tone...");
}

void loop() {
  // Slow, distinct visual heartbeat (1000ms intervals = 2-second full cycle)
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink >= 1000) {
    lastBlink = millis();
    digitalWrite(ONBOARD_LED, !digitalRead(ONBOARD_LED));
  }

  static float phase = 0.0f;
  float phaseIncrement = (2.0f * M_PI * TONE_FREQ) / SAMPLE_RATE;

  for (int i = 0; i < BUFFER_SAMPLES; i++) {
    float sampleVal = sin(phase) * 0.2f * 8388607.0f; 
    int32_t intSample = (int32_t)sampleVal;

    audioBuffer[i * 2]     = intSample; // Left
    audioBuffer[i * 2 + 1] = intSample; // Right

    phase += phaseIncrement;
    if (phase >= 2.0f * M_PI) phase -= 2.0f * M_PI;
  }

  size_t bytes_written = 0;
  i2s_channel_write(tx_chan, audioBuffer, sizeof(audioBuffer), &bytes_written, portMAX_DELAY);
}
