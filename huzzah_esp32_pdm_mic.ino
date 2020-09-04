#include "driver/i2s.h"
#include "dsps_fft2r.h"

#define READ_DELAY 3000 // millisec
const i2s_port_t I2S_PORT = I2S_NUM_0;
const int BLOCK_SIZE = 64;
int samples[BLOCK_SIZE];
#define SAMPLE_RATE 32000
long total_read = 0;
// http://www.schwietering.com/jayduino/filtuino/index.php?characteristic=bu&passmode=hp&order=1&usesr=usesr&sr=32000&frequencyLow=400&noteLow=&noteHigh=&pw=pw&calctype=float&run=Send
//High pass butterworth filter order=1 alpha1=0.0125 
class  FilterBuHp1
{
  public:
    FilterBuHp1()
    {
      v[0]=0.0;
    }
  private:
    float v[2];
  public:
    float step(float x) //class II 
    {
      v[0] = v[1];
      v[1] = (9.621952458291035404e-1f * x)
         + (0.92439049165820696974f * v[0]);
      return 
         (v[1] - v[0]);
    }
};
FilterBuHp1 filter;
float windowSum = 0;
int windowLength = 0;

void init_pdm() {
  
    esp_err_t err;

    i2s_config_t audio_in_i2s_config = {
         .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
         .sample_rate = SAMPLE_RATE,
         .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
         .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT, // although the SEL config should be left, it seems to transmit on right
         .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S),
         .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1, // high interrupt priority
         .dma_buf_count = 8,
         .dma_buf_len = BLOCK_SIZE,
         .use_apll = false
        };
    
    // This function must be called before any I2S driver read/write operations.
    err = i2s_driver_install(I2S_PORT, &audio_in_i2s_config, 0, NULL);
    if (err != ESP_OK) {
      Serial.printf("Failed installing driver: %d\n", err);
      while (true);
    }
  
    // configure which pin on the ESP32 is connected to which pin on the mic:
    i2s_pin_config_t audio_in_pin_config = {
        .bck_io_num = I2S_PIN_NO_CHANGE, // not used
        .ws_io_num = 15,  // IO 15 clock pin
        .data_out_num = I2S_PIN_NO_CHANGE, // Not used
        .data_in_num = 16   // data pin
    };
    
    err = i2s_set_pin(I2S_PORT, &audio_in_pin_config);
    if (err != ESP_OK) {
      Serial.printf("Failed setting pin: %d\n", err);
      while (true);
    }
}




void setup() {
    Serial.begin(2000000);
    delay(1000);
    while(!Serial){ delay(5);}
    // Initialize the I2S peripheral
    init_pdm();
    // Create a task that will read the data
    xTaskCreatePinnedToCore(process_samples, "PDM_reader", 2048, NULL, 1, NULL, 1);
}

void process_samples(void *pvParameters) {
  
    while(1){
      int num_bytes_read = i2s_read_bytes(I2S_PORT, 
                                          (char *)samples, 
                                          BLOCK_SIZE,     // the doc says bytes, but its elements.
                                          portMAX_DELAY); // no timeout
          if (num_bytes_read > 0) {
            
            int samples_read = num_bytes_read / 4;
            total_read += samples_read;
            unsigned char buff[sizeof(float)];
            float sample;
            float rms = 0;
            for(int i=0; i < samples_read; i++) {
              sample = filter.step((float)samples[i] / 134217728);
              sample = max(-1.0f, min(1.0f, sample));
              rms += sample * sample;              
              // memcpy(buff, &sample, sizeof(sample));
              // Serial.write(buff, sizeof(sample));
            }
            windowSum += rms;
            windowLength += samples_read;
            if(windowLength >= SAMPLE_RATE * 0.125) {
              float spl = 20 * log10f(sqrtf(windowSum / windowLength));
              Serial.println(spl);
              windowSum = 0;
              windowLength = 0;
            }
          }
    }
}

void loop() {
    delay(READ_DELAY); 
    // Read multiple samples at once and calculate the sound pressure
}
