#include "driver/i2s.h"

#define READ_DELAY 1000 // millisec
const i2s_port_t I2S_PORT = I2S_NUM_0;
const int BLOCK_SIZE = 512;

void setup() {
    Serial.begin(115200);
    esp_err_t err;

    i2s_config_t audio_in_i2s_config = {
         .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
         .sample_rate = 48000,
         .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
         .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT, // although the SEL config should be left, it seems to transmit on right
         .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
         .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1, // high interrupt priority
         .dma_buf_count = 4,
         .dma_buf_len = BLOCK_SIZE
        };
    
    // This function must be called before any I2S driver read/write operations.
    err = i2s_driver_install(I2S_PORT, &audio_in_i2s_config, 0, NULL);
    if (err != ESP_OK) {
      Serial.printf("Failed installing driver: %d\n", err);
      while (true);
    }
  
    // configure which pin on the ESP32 is connected to which pin on the mic:
    i2s_pin_config_t audio_in_pin_config = {
        .bck_io_num = 14, // BCLK pin 14 -> GPIO 33
        .ws_io_num = 15,  // LRCL pin
        .data_out_num = -1, // Not used
        .data_in_num = 16   // DOUT pin
    };
    
    err = i2s_set_pin(I2S_PORT, &audio_in_pin_config);
    if (err != ESP_OK) {
      Serial.printf("Failed setting pin: %d\n", err);
      while (true);
    }
//    // set up other I2S parameters:
//


}

void loop() {

    // Read multiple samples at once and calculate the sound pressure
    short samples[BLOCK_SIZE];
    int num_bytes_read = i2s_read_bytes(I2S_PORT, 
                                        (char *)samples, 
                                        BLOCK_SIZE,     // the doc says bytes, but its elements.
                                        portMAX_DELAY); // no timeout
    int samples_read = num_bytes_read / 2;
    if (samples_read > 0) {
      for(int i=0; i < samples_read; i++) {
        Serial.println(samples[i]);
      }
      delay(5000);
//int nsamples = samples_read / 2;
//      float sum = 0;
//      for(int i=0; i < nsamples; i++) {
//        sum += samples[i * 2 + 1];
//      }    
//      float avg = sum / nsamples;
//      float rms = 0;
//      for(int i=0; i < nsamples; i++) {
//        float p = samples[i * 2 + 1] - avg;
//        rms = p * p; 
//      }
//      float spl = 20 * log10f(sqrtf(rms / nsamples));
//      Serial.println(spl);
    }
}
