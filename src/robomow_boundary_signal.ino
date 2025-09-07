#include "waveform.h"   // WAVEFORM_LEN, waveform[]
#include "driver/i2s.h"
#include <stdint.h>
//#include <string.h>

// ===== Choose your rate =====
constexpr int    SAMPLE_RATE = 384000;      // try 192000, 256000, 384000, 480000
constexpr double F_OUT       = 3007.4825;   // Hz
constexpr bool   USE_APLL    = true;

constexpr i2s_port_t I2S_PORT = I2S_NUM_0;

// Stereo frames: two 16-bit words per frame (L,R). We'll duplicate mono → both.
constexpr int DMA_FRAMES = 256;                 // frames per write (128–512 ok)
static uint16_t dmaBuf[DMA_FRAMES * 2];         // *2 for L+R

// DDS constants
constexpr uint32_t PHASE_INC =
  (uint32_t)((F_OUT * (double)UINT32_MAX) / (double)SAMPLE_RATE + 0.5);

static_assert((WAVEFORM_LEN & (WAVEFORM_LEN - 1)) == 0,
              "WAVEFORM_LEN must be a power of two");

constexpr int log2i(unsigned n){ return (n<=1)?0:1+log2i(n>>1); }
constexpr int INDEX_BITS = log2i(WAVEFORM_LEN);
constexpr int SHIFT      = 32 - INDEX_BITS;
constexpr uint32_t INDEX_MASK = (1u<<INDEX_BITS) - 1u;
constexpr uint32_t FRAC_MASK  = (1u<<SHIFT) - 1u;

static uint8_t lut[WAVEFORM_LEN];

void setupI2S() {
  i2s_config_t cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,   // stereo frames
    .communication_format = I2S_COMM_FORMAT_STAND_MSB,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = DMA_FRAMES,
    .use_apll = USE_APLL,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };

  i2s_driver_install(I2S_PORT, &cfg, 0, nullptr);
  i2s_set_pin(I2S_PORT, nullptr);

  // Drive DAC1 (GPIO25). Use RIGHT_EN for DAC2 (GPIO26), or BOTH_EN to mirror.
  i2s_set_dac_mode(I2S_DAC_CHANNEL_LEFT_EN);

  i2s_set_sample_rates(I2S_PORT, SAMPLE_RATE);
}

void setup() {
  memcpy(lut, waveform, WAVEFORM_LEN);  // optional: bring LUT into RAM
  setupI2S();
}

void loop() {
  static uint32_t phase = 0;

  for (int i = 0; i < DMA_FRAMES; ++i) {
    // DDS index + fractional part for interpolation
    uint32_t idx  = (phase >> SHIFT) & INDEX_MASK;
    uint32_t frac =  phase & FRAC_MASK;

    uint8_t a = lut[idx];
    uint8_t b = lut[(idx + 1) & INDEX_MASK];

    // Linear interpolation: y = a + (b-a) * frac / 2^SHIFT
    int16_t diff = (int16_t)b - (int16_t)a;
    uint16_t y   = (uint16_t)(a + ((int32_t)diff * (int32_t)frac >> SHIFT));

    uint16_t s = (uint16_t)(y << 8);      // DAC uses high byte

    // write both channels each frame (duplicate mono)
    dmaBuf[2*i + 0] = s;  // Left  → DAC1 (GPIO25)
    dmaBuf[2*i + 1] = s;  // Right (discarded if LEFT_EN, but keeps timing correct)

    phase += PHASE_INC;
  }

  size_t written = 0;
  i2s_write(I2S_PORT, dmaBuf, sizeof(dmaBuf), &written, portMAX_DELAY);

  // Do other work here; I2S+DMA handles timing.
}
