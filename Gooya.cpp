//
//  Created by Evixar Inc.
//  Copyright © 2018 Evixar Inc. All rights reserved.
//

#include "Gooya.h"
#include <stdio.h>
#include <soc/io_mux_reg.h>
#include <math.h>

//recorder
void GooyaRecorder::_inputcallback(int16_t* input, int32_t frames, void* param)
{
    GooyaRecorder* _this=(GooyaRecorder*)param;
    _this->inputcallback(input,frames);
}

void GooyaRecorder::inputcallback(int16_t* input, int32_t frames)
{
    int16_t* buff=&signal[pos*DMA_AUDIO_FRAMES*AUDIO_CHANNELS];
    
    for (int i=0; i<DMA_AUDIO_FRAMES; i++) {
        buff[AUDIO_CHANNELS*i] = input[AUDIO_CHANNELS*i];
        buff[AUDIO_CHANNELS*i+1] = input[AUDIO_CHANNELS*i+1];
    }
    pos = (pos+1)%( GOOYA_RECORD_CHUNK_COUNT * GOOYA_RECORD_BUFFER_COUNT);
    if(pos%GOOYA_RECORD_CHUNK_COUNT==0)
    {
        int _pos=pos/GOOYA_RECORD_CHUNK_COUNT;
        buff=&signal[_pos*(GOOYA_RECORD_CHUNK_COUNT*DMA_AUDIO_FRAMES)*AUDIO_CHANNELS];
        if(xQueueSend(queue, &buff, 0)==errQUEUE_FULL)
            Serial.println("over flow\n");
    }
    

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
    while(eTaskGetState(recorder_task)!=eDeleted)
    {        
        if(xQueueReceive(queue, &buff, 0) && buff!=0){
            file->write((const uint8_t*)buff,GOOYA_RECORD_CHUNK_COUNT*DMA_AUDIO_FRAMES*AUDIO_CHANNELS*sizeof(int16_t));
            //file->flush();
            //Serial.println("write\n");
        }
        else
        {
            //Serial.println("under flow\n");
            vTaskDelay(1);
        }
        
    }
}


GooyaRecorder::GooyaRecorder()
{
    file=nullptr;
    pos=0;
    chunk_count=0;
    signal.resize(GOOYA_RECORD_CHUNK_COUNT*DMA_AUDIO_FRAMES*AUDIO_CHANNELS*GOOYA_RECORD_BUFFER_COUNT);
    queue=xQueueCreate(GOOYA_RECORD_CHUNK_COUNT*GOOYA_RECORD_BUFFER_COUNT,sizeof(int16_t*));
}

void GooyaRecorder::start(int devid, int sample_rate, File& file)
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

    xTaskCreatePinnedToCore((TaskFunction_t)&_recorder_task_function, "recorder_task", 2048, this, 6, &recorder_task, 0);

    i2s.start();

}

void GooyaRecorder::stop()
{
    vTaskDelete(recorder_task);
    i2s.stop();
    codec.stop();
}


//player
void GooyaPlayer::_outputcallback(int16_t* output, int32_t frames, void* param)
{
    GooyaPlayer* _this=(GooyaPlayer*)param;
    _this->outputcallback(output,frames);
}

void GooyaPlayer::outputcallback(int16_t* output, int32_t frames)
{
    int16_t* buff=0;
    if(xQueueReceive(queue, &buff, 0) && buff!=0){
        for(int i=0;i<DMA_AUDIO_FRAMES;i++)
        {
            output[AUDIO_CHANNELS*i]=buff[AUDIO_CHANNELS*i];
            output[AUDIO_CHANNELS*i+1]=buff[AUDIO_CHANNELS*i+1];
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
    while(eTaskGetState(player_task)!=eDeleted)
    {
        int16_t* buff=&signal[pos*DMA_AUDIO_FRAMES*AUDIO_CHANNELS];
        file->read((uint8_t*)buff,DMA_AUDIO_FRAMES*AUDIO_CHANNELS*sizeof(int16_t));    
        pos = (pos+1)%GOOYA_PLAY_BUFFER_COUNT;
        while( xQueueSend(queue, &buff, portMAX_DELAY) != pdPASS )
        {
            vTaskDelay(1);
        }
    }
}


GooyaPlayer::GooyaPlayer()
{
    file=nullptr;
    signal.resize(DMA_AUDIO_FRAMES*AUDIO_CHANNELS*GOOYA_PLAY_BUFFER_COUNT);
    queue=xQueueCreate(GOOYA_PLAY_BUFFER_COUNT,sizeof(int16_t*));
}

void GooyaPlayer::start(int sample_rate, File& file)
{
    codec.initialize((gpio_num_t)I2C_SDIN, (gpio_num_t)I2C_SCLK, (gpio_num_t)I2C_ADR);
    codec.start();
    codec.setup(sample_rate,true,true);
    i2s.initialize(sample_rate,GOOYA_CPU,nullptr,GooyaPlayer::_outputcallback,this);

    this->file=&file;
    this->file->seek(0);
    
    xTaskCreatePinnedToCore((TaskFunction_t)&_player_task_function, "player_task", 2048, this, 5, &player_task, 0);
    vTaskDelay(500);//wait for expecting pre-read queue retension 
    i2s.start();
}

void GooyaPlayer::stop()
{
    vTaskDelete(player_task);
    i2s.stop();
    codec.stop();
}