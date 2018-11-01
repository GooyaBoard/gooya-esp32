//
//  Created by Evixar Inc.
//  Copyright © 2018 Evixar Inc. All rights reserved.
//

#include <Arduino.h>

#include "I2SMaster.h"
#include <stdio.h>
#include <soc/io_mux_reg.h>
#include <math.h>

void I2SMaster::audio_input_task(void* pvParameters)
{
    I2SMaster* _this=(I2SMaster*)pvParameters;
    uint8_t* buffer=(uint8_t*)&_this->inputbuffer[0];
    int n=0;
    
    while(eTaskGetState(_this->input_task)!=eDeleted){
        size_t readbytes=0;
        esp_err_t err = i2s_read(I2S_NUM_0, &buffer[n*DMA_BUFFER_SIZE], DMA_BUFFER_SIZE, &readbytes, portMAX_DELAY);
        if(err)
            log_w("i2s_read faild, esp_err_t=%d",err);
        
        (*_this->inputcallback)((int16_t*)&buffer[n*DMA_BUFFER_SIZE], DMA_AUDIO_FRAMES, _this->param);
        n = (n+1)%DMA_BUFFER_COUNT;
    }
}

void I2SMaster::audio_output_task(void* pvParameters)
{
    I2SMaster* _this=(I2SMaster*)pvParameters;
    uint8_t* buffer=(uint8_t*)&_this->outputbuffer[0];
    int n=0;
    
    while(eTaskGetState(_this->output_task)!=eDeleted){
        size_t writtenbytes=0;
        esp_err_t err = i2s_write(I2S_NUM_0, &buffer[n*DMA_BUFFER_SIZE], DMA_BUFFER_SIZE, &writtenbytes, portMAX_DELAY);
        if(err)
            log_w("i2s_write faild, esp_err_t=%d",err);

	    n = (n+1)%DMA_BUFFER_COUNT;
        (*_this->outputcallback)((int16_t*)&buffer[n*DMA_BUFFER_SIZE], DMA_AUDIO_FRAMES, _this->param);
    }
}

I2SMaster::I2SMaster()
{
    param = nullptr;
    started = false;
    inputcallback = nullptr;
    outputcallback = nullptr;
}

void I2SMaster::initialize(int sample_rate, int cpu_number,
                        I2S_AUDIOCALLBACK inputcallback,
                        I2S_AUDIOCALLBACK outputcallback, void* param)
{
    // PIN setting
    i2s_pin_config.bck_io_num = I2S_BCLK;
    i2s_pin_config.ws_io_num = I2S_ADC_LRCLK;
    i2s_pin_config.data_in_num = I2S_ADC_DATA;
    i2s_pin_config.data_out_num = I2S_DAC_DATA;

    // I2S setting
    i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX);
    i2s_config.sample_rate = sample_rate;
    i2s_config.bits_per_sample = I2S_BITS_PER_SAMPLE;
    i2s_config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
    i2s_config.communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB);
    i2s_config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
    i2s_config.dma_buf_count = DMA_BUFFER_COUNT;
    i2s_config.dma_buf_len = DMA_AUDIO_FRAMES;
    i2s_config.use_apll = 1; // accurate clock
    i2s_config.fixed_mclk = 12288000;
    
    this->cpu_number=cpu_number;
    this->inputcallback=inputcallback;
    this->outputcallback=outputcallback;  

    if(this->inputcallback!=nullptr)
        inputbuffer.resize(DMA_TOTAL_BUFFER_SIZE,0);
    if(this->outputcallback!=nullptr)
        outputbuffer.resize(DMA_TOTAL_BUFFER_SIZE,0);
    
    this->param=param;

    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_CLK_OUT1);
    REG_SET_FIELD(PIN_CTRL, CLK_OUT1, 0);

    //enable_out_clock();
}

void I2SMaster::start()
{
    if (started) return;
    started = true;

    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    
    i2s_set_pin(I2S_NUM_0, &i2s_pin_config);
    i2s_start(I2S_NUM_0);
    
    if(this->inputcallback!=nullptr)
        xTaskCreatePinnedToCore((TaskFunction_t)&audio_input_task, "audio_input_task", 10000, this, 5, &input_task, cpu_number);
    if(this->outputcallback!=nullptr)        
        xTaskCreatePinnedToCore((TaskFunction_t)&audio_output_task, "audio_output_task", 10000, this, 5, &output_task, cpu_number);
}

void I2SMaster::stop()
{
    if (!started) return;
    
    if(this->inputcallback!=nullptr)
        vTaskDelete(input_task);
    if(this->outputcallback!=nullptr)        
        vTaskDelete(output_task);
    
    i2s_stop(I2S_NUM_0);
    i2s_driver_uninstall(I2S_NUM_0);
    started = false;
}

