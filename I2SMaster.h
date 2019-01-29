//
//  Created by Evixar Inc.
//  Copyright © 2018 Evixar Inc. All rights reserved.
//

#ifndef __I2SMASTER_H__
#define __I2SMASTER_H__

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s.h"
#include <vector>

using namespace std;

#define AUDIO_CHANNELS (2)
#define AUDIO_CHANNEL_LEFT (0)
#define AUDIO_CHANNEL_RIGHT (1)
#define AUDIO_CHANNEL_NONE (2)

#ifndef I2S_BCLK
#define I2S_BCLK (25)
#endif
#ifndef I2S_ADC_DATA
#define I2S_ADC_DATA (2)
#endif
#ifndef I2S_DAC_DATA
#define I2S_DAC_DATA (17)
#endif
#ifndef I2S_ADC_LRCLK
#define I2S_ADC_LRCLK (26)
#endif

#ifndef I2S_BITS_PER_SAMPLE
#define I2S_BITS_PER_SAMPLE I2S_BITS_PER_SAMPLE_16BIT
#endif

#ifndef DMA_AUDIO_FRAMES
#define DMA_AUDIO_FRAMES (256)
#endif

#define DMA_BUFFER_SIZE (I2S_BITS_PER_SAMPLE/8*DMA_AUDIO_FRAMES*AUDIO_CHANNELS)

#ifndef DMA_BUFFER_COUNT
#define DMA_BUFFER_COUNT (2)
#endif

#define DMA_TOTAL_BUFFER_SIZE (DMA_BUFFER_SIZE*DMA_BUFFER_COUNT)

typedef void (* I2S_AUDIOCALLBACK)(int16_t* buffer, int8_t chs, int32_t frames, void* param); 

class I2SMaster
{
public:
    // cpu_number: 0 or 1
    I2SMaster();
    void initialize(int sample_rate, int cpu_number,
                        I2S_AUDIOCALLBACK inputcallback,
                        I2S_AUDIOCALLBACK outputcallback,
                        void* param);
    
    void start();
    void stop();

    static void audio_input_task(void* pvParameters);
    static void audio_output_task(void* pvParameters);
    
private:
    i2s_pin_config_t i2s_pin_config;
    i2s_config_t i2s_config;

    bool started;
    void* param;
    std::vector<uint8_t> inputbuffer;
    std::vector<uint8_t> outputbuffer;
    I2S_AUDIOCALLBACK inputcallback;
    I2S_AUDIOCALLBACK outputcallback;

    // task
    int cpu_number;
    TaskHandle_t input_task;
    TaskHandle_t output_task;    
};

#endif//