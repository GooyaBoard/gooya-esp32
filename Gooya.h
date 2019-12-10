//
//  Created by Evixar Inc.
//  Copyright © 2018 Evixar Inc. All rights reserved.
//

#ifndef __GOOYA_H__
#define __GOOYA_H__

#ifndef GOOYA_CPU
    #define GOOYA_CPU (1)
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s.h"
#include <vector>

//for SD
#include <SD.h>

//codec
#if !defined(GOOYA_CODEC_WM8731) && !defined(GOOYA_CODEC_AK4951)
#define GOOYA_CODEC_WM8731
#endif

#ifdef GOOYA_CODEC_WM8731
#include "WM8731.h"
#endif
#ifdef GOOYA_CODEC_AK4951
#include "AK4951.h"
#endif

#include "I2SMaster.h"

#ifndef GOOYA_RECORD_BUFFER_COUNT
#define GOOYA_RECORD_BUFFER_COUNT (16)
#endif

#ifndef GOOYA_PLAY_BUFFER_COUNT
#define GOOYA_PLAY_BUFFER_COUNT (16)
#endif

using namespace std;

//callback
typedef void (* GOOYA_AUDIOCALLBACK)(const int16_t* input, int16_t* output, int8_t chs, int32_t frames); 

#define GOOYARECORDER_MICIN 0
#define GOOYARECORDER_LINEIN 1

class GooyaRecorder
{
public:
    GooyaRecorder();
    void start(int devid, int sample_rate, File& file);
    void stop();

private:
    I2SMaster i2s;
    WM8731 codec;

    File* file;

    xQueueHandle queue;
    int32_t pos;
    int32_t chunk_count;
    std::vector<int16_t> signal;//byte size of DMA_AUDIO_FRAMES*AUDIO_CHANNELS*GOOYA_RECORD_BUFFER_COUNT;
    
    void inputcallback(int16_t* input, int8_t chs, int32_t frames);
    static void _inputcallback(int16_t* input, int8_t chs, int32_t frames, void* param);

    bool running;
    TaskHandle_t recorder_task;
    void recorder_task_function();
    static void _recorder_task_function(void* param);
};

class GooyaPlayer
{
public:
    GooyaPlayer();
    void start(int sample_rate, File& file, int headphonelevel=-24);
    void stop();

private:
    I2SMaster i2s;
    WM8731 codec;

    File* file;

    xQueueHandle queue;
    std::vector<int16_t> signal;//byte size of DMA_AUDIO_FRAMES*AUDIO_CHANNELS*GOOYA_RECORD_BUFFER_COUNT;
    
    void outputcallback(int16_t* output, int8_t chs, int32_t frames);
    static void _outputcallback(int16_t* output, int8_t chs, int32_t frames, void* param);

    bool running;
    TaskHandle_t player_task;
    void player_task_function();
    static void _player_task_function(void* param);
};

class GooyaEffect
{
public:
    GooyaEffect();
    void start(int devid, int sample_rate, GOOYA_AUDIOCALLBACK callback);
    void stop();

private:
    I2SMaster i2s;
    WM8731 codec;

    xQueueHandle inputqueue;
    xQueueHandle outputqueue;
    int32_t pos;
    int32_t chunk_count;
    
    std::vector<int16_t> signal;//byte size of DMA_AUDIO_FRAMES*AUDIO_CHANNELS*GOOYA_RECORD_BUFFER_COUNT;
    
    void inputcallback(int16_t* output, int8_t chs, int32_t frames);
    static void _inputcallback(int16_t* output, int8_t chs, int32_t frames, void* param);

    void outputcallback(int16_t* output, int8_t chs, int32_t frames);
    static void _outputcallback(int16_t* output, int8_t chs, int32_t frames, void* param);

    GOOYA_AUDIOCALLBACK callback;

    bool running;
    TaskHandle_t effect_task;
    void effect_task_function();
    static void _effect_task_function(void* param);
};


/*
class GooyaAnalyzer
class GooyaGenerator
*/

#endif
