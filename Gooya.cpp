//
//  Created by Evixar Inc.
//  Copyright © 2018 Evixar Inc. All rights reserved.
//

#include "Gooya.h"
#include <stdio.h>
#include <soc/io_mux_reg.h>
#include <math.h>

//recorder
void GooyaRecorder::_inputcallback(int16_t* input, int8_t chs, int32_t frames, void* param)
{
    GooyaRecorder* _this=(GooyaRecorder*)param;
    _this->inputcallback(input,chs,frames);
}

void GooyaRecorder::inputcallback(int16_t* input, int8_t chs, int32_t frames)
{
    int16_t* buff=&signal[pos*DMA_AUDIO_FRAMES*AUDIO_CHANNELS];
    
    for (int i=0; i<DMA_AUDIO_FRAMES; i++){
        for(int ch=0;ch<chs;ch++){
            buff[chs*i] = input[chs*i+ch];
        }
    }
    pos = (pos+1)%GOOYA_RECORD_BUFFER_COUNT;
    buff=&signal[pos*DMA_AUDIO_FRAMES*AUDIO_CHANNELS];
    if(xQueueSend(queue, &buff, 0)==errQUEUE_FULL)
        Serial.println("over flow\n");
    //else
    //    Serial.println("usual\n");

}

void GooyaRecorder::_recorder_task_function(void* param)
{
    GooyaRecorder* _this=(GooyaRecorder*)param;
    _this->recorder_task_function();
}

void GooyaRecorder::recorder_task_function()
{
    int16_t* buff=0;
    
    std::vector<int16_t> buff16;
    std::vector<float> buff32;
    
    if(!float32 && !stereo){
        buff16.resize(DMA_AUDIO_FRAMES);
    }
    if(!float32 && stereo){
        //do nothing
    }
    if(float32 && !stereo){
        buff32.resize(DMA_AUDIO_FRAMES);
    }
    if(float32 && stereo){
        buff32.resize(DMA_AUDIO_FRAMES*AUDIO_CHANNELS);
    }

    while(running)
    {        
        if(xQueueReceive(queue, &buff, 0) && buff!=0){
            if(!float32 && !stereo){
                for(int i=0;i<DMA_AUDIO_FRAMES;i++){
                    //record only left channel
                    buff16[i]=buff[AUDIO_CHANNELS*i];
                }
                file->write((const uint8_t*)buff16.data(),DMA_AUDIO_FRAMES*sizeof(int16_t));
            }
            if(!float32 && stereo){
                file->write((const uint8_t*)buff,DMA_AUDIO_FRAMES*AUDIO_CHANNELS*sizeof(int16_t));
            }
            if(float32 && !stereo){
                for(int i=0;i<DMA_AUDIO_FRAMES;i++){
                    //record only left channel
                    buff32[i]=buff[AUDIO_CHANNELS*i]/32768.f;
                }
                file->write((const uint8_t*)buff32.data(),DMA_AUDIO_FRAMES*sizeof(int16_t));
            }
            if(float32 && stereo){
                for(int i=0;i<DMA_AUDIO_FRAMES;i++){
                    for(int ch=0;ch<AUDIO_CHANNELS;ch++){
                        buff32[AUDIO_CHANNELS*i+ch]=buff[AUDIO_CHANNELS*i+ch]/32768.f;
                    }
                }
                file->write((const uint8_t*)buff32.data(),DMA_AUDIO_FRAMES*AUDIO_CHANNELS*sizeof(float));
            }
            //file->flush();
            //Serial.println("write\n");
        }
        else
        {
            //Serial.println("under flow\n");
            vTaskDelay(1);
        }
    }
    vTaskDelete(NULL);
}


GooyaRecorder::GooyaRecorder()
{
    file=nullptr;
    pos=0;
    chunk_count=0;
    queue=xQueueCreate(GOOYA_RECORD_BUFFER_COUNT,sizeof(int16_t*));
}

void GooyaRecorder::start(int devid, int sample_rate, File& file, bool _stereo, bool _float32)
{
    codec.initialize((gpio_num_t)I2C_SDIN, (gpio_num_t)I2C_SCLK, (gpio_num_t)I2C_ADR);
    codec.start();
    
    if(devid==GOOYARECORDER_MICIN)
        codec.setup(sample_rate,true,true);
    else if(devid==GOOYARECORDER_LINEIN)
        codec.setup(sample_rate,false,false);
    
    i2s.initialize(sample_rate,GOOYA_CPU,GooyaRecorder::_inputcallback,nullptr,this);

    this->file=&file;
    pos=0;

    stereo=_stereo;
    float32=_float32;

    running=true;
    xTaskCreatePinnedToCore((TaskFunction_t)&_recorder_task_function, "recorder_task", 2048*2, this, 6, &recorder_task, 0);
    
    i2s.start();
}

void GooyaRecorder::stop()
{
    running=false;
    recorder_task=NULL;
    i2s.stop();
    codec.stop();
}


//player
void GooyaPlayer::_outputcallback(int16_t* output, int8_t chs, int32_t frames, void* param)
{
    GooyaPlayer* _this=(GooyaPlayer*)param;
    _this->outputcallback(output,chs,frames);
}

void GooyaPlayer::outputcallback(int16_t* output, int8_t chs, int32_t frames)
{
    int16_t* buff=0;
    if(xQueueReceive(queue, &buff, 0) && buff!=0){
        for(int i=0;i<frames;i++){
            for(int ch=0;ch<chs;ch++){
                output[chs*i+ch]=buff[chs*i+ch];
            }
        }
    }
}

void GooyaPlayer::_player_task_function(void* param)
{
    GooyaPlayer* _this=(GooyaPlayer*)param;
    _this->player_task_function();
}

void GooyaPlayer::player_task_function()
{
    int pos=0;
    std::vector<int16_t> buff16;
    std::vector<float> buff32;
    
    if(!float32 && !stereo){
        buff16.resize(DMA_AUDIO_FRAMES);
    }
    if(!float32 && stereo){
        //do nothing
    }
    if(float32 && !stereo){
        buff32.resize(DMA_AUDIO_FRAMES);
    }
    if(float32 && stereo){
        buff32.resize(DMA_AUDIO_FRAMES*AUDIO_CHANNELS);
    }

    while(running)
    {
        int16_t* buff=&signal[pos*DMA_AUDIO_FRAMES*AUDIO_CHANNELS];
        if(!float32 && !stereo){
            //read mono 16bit
            file->read((uint8_t*)buff16.data(),DMA_AUDIO_FRAMES*sizeof(int16_t));
            //conver to stereo
            for(int i=0;i<DMA_AUDIO_FRAMES;i++){
                for(int ch=0;ch<AUDIO_CHANNELS;ch++){
                    buff[AUDIO_CHANNELS*i+ch]=buff16[i];
                }
            }
        }
        if(!float32 && stereo){
            file->read((uint8_t*)buff,DMA_AUDIO_FRAMES*AUDIO_CHANNELS*sizeof(int16_t));
        }
        if(float32 && !stereo){
            //read mono 32bit float
            file->read((uint8_t*)buff32.data(),DMA_AUDIO_FRAMES*sizeof(float));
            //conver to stereo
            for(int i=0;i<DMA_AUDIO_FRAMES;i++){
                for(int ch=0;ch<AUDIO_CHANNELS;ch++){
                    buff[AUDIO_CHANNELS*i+ch]=buff32[i];
                }
            }
        }
        if(float32 && stereo){
            //read stereo 32bit float
            file->read((uint8_t*)buff32.data(),DMA_AUDIO_FRAMES*AUDIO_CHANNELS*sizeof(float));
            //conver to stereo
            for(int i=0;i<DMA_AUDIO_FRAMES;i++){
                for(int ch=0;ch<AUDIO_CHANNELS;ch++){
                    buff[AUDIO_CHANNELS*i+ch]=buff32[AUDIO_CHANNELS*i+ch]*32767.f;
                }
            }
        }
        
        pos = (pos+1)%GOOYA_PLAY_BUFFER_COUNT;
        while( xQueueSend(queue, &buff, portMAX_DELAY) != pdPASS )
        {
            vTaskDelay(1);
        }
    }
    vTaskDelete(NULL);
}


GooyaPlayer::GooyaPlayer()
{
    file=nullptr;
    //signal.resize(DMA_AUDIO_FRAMES*AUDIO_CHANNELS*GOOYA_PLAY_BUFFER_COUNT);
    queue=xQueueCreate(GOOYA_PLAY_BUFFER_COUNT,sizeof(int16_t*));
}

void GooyaPlayer::start(int sample_rate, File& file, int headphonelevel, bool _stereo, bool _float32)
{
    codec.initialize((gpio_num_t)I2C_SDIN, (gpio_num_t)I2C_SCLK, (gpio_num_t)I2C_ADR);
    codec.start();
    codec.setup(sample_rate,true,true,headphonelevel);
    i2s.initialize(sample_rate,GOOYA_CPU,nullptr,GooyaPlayer::_outputcallback,this);

    this->file=&file;
    this->file->seek(0);

    stereo=_stereo;
    float32=_float32;

    running=true;
    xTaskCreatePinnedToCore((TaskFunction_t)&_player_task_function, "player_task", 2048*2, this, 5, &player_task, 0);
    vTaskDelay(500);//wait for expecting pre-read queue retension 
    i2s.start();
}

void GooyaPlayer::stop()
{
    running=false;
    player_task=NULL;
    i2s.stop();
    codec.stop();
}

// effect
void GooyaEffect::_inputcallback(int16_t* input, int8_t chs, int32_t frames, void* param)
{
    GooyaEffect* _this=(GooyaEffect*)param;
    _this->inputcallback(input,chs,frames);
}

void GooyaEffect::inputcallback(int16_t* input, int8_t chs, int32_t frames)
{
    int16_t* buff=&signal[pos*DMA_AUDIO_FRAMES*AUDIO_CHANNELS];
    
    for (int i=0; i<frames; i++) {
        for(int ch=0;ch<chs;ch++){
            buff[chs*i+ch] = input[chs*i+ch];
        }
    }
    pos = (pos+1)%GOOYA_RECORD_BUFFER_COUNT;
    buff=&signal[pos*DMA_AUDIO_FRAMES*AUDIO_CHANNELS];
    if(xQueueSend(inputqueue, &buff, 0)==errQUEUE_FULL)
        Serial.println("over flow\n");
}

void GooyaEffect::_outputcallback(int16_t* output, int8_t chs, int32_t frames, void* param)
{
    GooyaEffect* _this=(GooyaEffect*)param;
    _this->outputcallback(output,chs,frames);
}

void GooyaEffect::outputcallback(int16_t* output, int8_t chs, int32_t frames)
{
    int16_t* buff=0;
    if(xQueueReceive(outputqueue, &buff, 0) && buff!=0){
        for(int i=0;i<frames;i++){
            for(int ch=0;ch<chs;ch++){
                output[chs*i+ch]=buff[chs*i+ch];
            }
        }
    }
}

void GooyaEffect::_effect_task_function(void* param)
{
    GooyaEffect* _this=(GooyaEffect*)param;
    _this->effect_task_function();
}

void GooyaEffect::effect_task_function()
{
    int16_t* buff=0;
    float temp[AUDIO_CHANNELS*DMA_AUDIO_FRAMES];
    while(running)
    {        
        if(xQueueReceive(inputqueue, &buff, 0) && buff!=0){
            //file->write((const uint8_t*)buff,GOOYA_RECORD_CHUNK_COUNT*DMA_AUDIO_FRAMES*AUDIO_CHANNELS*sizeof(int16_t));
            for(int i=0;i<DMA_AUDIO_FRAMES;i++){
                for(int ch=0;ch<AUDIO_CHANNELS;ch++){
                    temp[AUDIO_CHANNELS*i+ch]=buff[AUDIO_CHANNELS*i+ch]/32768.f;
                }
            }
            callback((const float*)temp,(float*)temp,AUDIO_CHANNELS,DMA_AUDIO_FRAMES);
            for(int i=0;i<DMA_AUDIO_FRAMES;i++){
                for(int ch=0;ch<AUDIO_CHANNELS;ch++){
                    buff[AUDIO_CHANNELS*i+ch]=temp[AUDIO_CHANNELS*i+ch]*32767.f;
                }
            }
            if(xQueueSend(outputqueue, &buff, 0)==errQUEUE_FULL)
                Serial.println("over flow\n");
            //file->flush();
            //Serial.println("write\n");
        }
        else
        {
            //Serial.println("under flow\n");
            vTaskDelay(1);
        }
    }
    vTaskDelete(NULL);
}


GooyaEffect::GooyaEffect()
{
    pos=0;
    chunk_count=0;
    //signal.resize(DMA_AUDIO_FRAMES*AUDIO_CHANNELS*GOOYA_RECORD_BUFFER_COUNT);
    inputqueue=xQueueCreate(GOOYA_RECORD_BUFFER_COUNT,sizeof(int16_t*));
    outputqueue=xQueueCreate(GOOYA_RECORD_BUFFER_COUNT,sizeof(int16_t*));
}

void GooyaEffect::start(int devid, int sample_rate, GOOYA_AUDIOCALLBACK _callback, int headphonelevel)
{
    codec.initialize((gpio_num_t)I2C_SDIN, (gpio_num_t)I2C_SCLK, (gpio_num_t)I2C_ADR);
    codec.start();
    
    if(devid==GOOYARECORDER_MICIN)
        codec.setup(sample_rate,true,true,headphonelevel);
    else if(devid==GOOYARECORDER_LINEIN)
        codec.setup(sample_rate,false,false,headphonelevel);
    
    i2s.initialize(sample_rate,GOOYA_CPU,GooyaEffect::_inputcallback,GooyaEffect::_outputcallback,this);

    pos=0;
    running=true;
    callback=_callback;
    xTaskCreatePinnedToCore((TaskFunction_t)&_effect_task_function, "effect_task", 2048*2, this, 6, &effect_task, 0);
    
    i2s.start();
}

void GooyaEffect::stop()
{
    running=false;
    effect_task=NULL;
    i2s.stop();
    codec.stop();
}
