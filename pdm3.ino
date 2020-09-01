#include "driver/i2s.h"
#include "dsps_fft2r.h"

#define READ_DELAY 3000 // millisec
const i2s_port_t I2S_PORT = I2S_NUM_0;
const int BLOCK_SIZE = 512;
int samples[BLOCK_SIZE];
#define SAMPLE_RATE 48000
long total_read = 0;

// http://www.schwietering.com/jayduino/filtuino/index.php?characteristic=be&passmode=lp&order=1&usesr=usesr&sr=48000&frequencyLow=25&noteLow=&noteHigh=&pw=pw&calctype=float&run=Send
//Low pass bessel filter order=1 alpha1=0.00052083333333333 
class  FilterBeHp1
{
  public:
    FilterBeHp1()
    {
      v[0]=0.0;
    }
  private:
    float v[2];
  public:
    float step(float x) //class II 
    {
      v[0] = v[1];
      v[1] = (9.983664252987520804e-1 * x)
         + (0.99673285059750416082 * v[0]);
      return 
         (v[1] - v[0]);
    }
};

FilterBeHp1 highPassFilter;

void init_pdm() {
  
    esp_err_t err;

    i2s_config_t audio_in_i2s_config = {
         .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
         .sample_rate = SAMPLE_RATE,
         .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
         .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT, // although the SEL config should be left, it seems to transmit on right
         .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_LSB),
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
            float sample;
            for(int i=0; i < samples_read; i++) {
              sample = highPassFilter.step((float)samples[i] / INT_MAX);
              Serial.println((int)(sample * INT_MAX));
            }
            //      float rms = 0;
            //      for(int i=0; i < nsamples; i++) {
            //        float p = samples[i * 2 + 1] - avg;
            //        rms = p * p; 
            //      }
            //      float spl = 20 * log10f(sqrtf(rms / nsamples));
            //      Serial.println(spl);
          }
    }
}

void loop() {
    delay(READ_DELAY); 
    // Read multiple samples at once and calculate the sound pressure
}
