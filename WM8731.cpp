//
//  Created by Evixar Inc.
//  Copyright © 2018 Evixar Inc. All rights reserved.
//

#include "WM8731.h"
#include "math.h"

//WM8731 register addresses as defined in the datasheet
#define LEFT_LINE_INPUT_CHANNEL_VOLUME_CONTROL  (0x00 << 1)
#define RIGHT_LINE_INPUT_CHANNEL_VOLUME_CONTROL (0x01 << 1)
#define LEFT_CHANNEL_HEADPHONE_VOLUME_CONTROL   (0x02 << 1)
#define RIGHT_CHANNEL_HEADPHONE_VOLUME_CONTROL  (0x03 << 1)
#define ANALOG_AUDIO_PATH_CONTROL               (0x04 << 1)
#define DIGITAL_AUDIO_PATH_CONTROL              (0x05 << 1)
#define POWER_DOWN_CONTROL                      (0x06 << 1)
#define DIGITAL_AUDIO_INTERFACE_FORMAT          (0x07 << 1)
#define SAMPLE_RATE_CONTROL                     (0x08 << 1)
#define DIGITAL_INTERFACE_ACTIVATION            (0x09 << 1)
#define RESET_REGISTER                          (0x0F << 1)

WM8731::WM8731()
{

}

void WM8731::initialize(gpio_num_t i2c_sda, gpio_num_t i2c_scl, uint8_t i2c_addr)
{
    i2c.initialize(i2c_addr, i2c_sda, i2c_scl);
}

void WM8731::start()
{
    i2c.start();
}

void WM8731::stop()
{
    i2c.stop();
}

void WM8731::setup(int32_t fs, bool useMic, bool micBoost)
{ 
    // reset
    resetDevice();

    if (useMic) {
        //setLeftLineIn(int32_t LRINBOTH, int32_t LINMUTE, float LINVOL)
        // no simultaneous load, mute, -34.5dB gain
        setLeftLineIn(1, 1, -34.5);//-34.5dB
        setRightLineIn(1, 1, -34.5);//-34.5dB
    } else {
        //setLeftLineIn(int32_t LRINBOTH, int32_t LINMUTE, float LINVOL)
        // no simultaneous load, no mute, 0dB gain
        setLeftLineIn(1, 0, 0);//0dB
        setRightLineIn(1, 0, 0);//0dB
    }

    // Headphone out (0dB)
    setLeftHeadphoneOut(1, 1, -30);//-24dB
    setRightHeadphoneOut(1, 1, -30);//-24dB
    
    //setAnalogueAudioPathControl(SIDEATT,SIDETONE,DACSEL,BYPASS,INSEL,MUTEMIC,MICBOOST);
    if (useMic) {
	if(micBoost){
            setAnalogueAudioPathControl(0,0,1,0,1,0,1);
	}else{
            setAnalogueAudioPathControl(0,0,1,0,1,0,0);
	}
    } else {
        setAnalogueAudioPathControl(0,0,1,0,0,1,0);
    }
    
    //setDigitalAudioPathControl(int32_t HPOR, int32_t DACMU, int32_t DEEMP, int32_t ADCHPD)
    //clear DC offset, not software mute, no de-emphasis,no adc HPF  
    setDigitalAudioPathControl(0,0,0,1);
    
    //setPowerDownControl(int32_t POWEROFF, int32_t CLKOUTPD, int32_t OSCPD, int32_t OUTPD, int32_t DACPD, int32_t ADCPD, int32_t MICPD, int32_t LINEINPD)
    setPowerDownControl(0,1,1,0,0,0,0,0);
    
    //setDigitalAudioInterfaceFormat(int32_t BCLKINV, int32_t MS, int32_t LRSWAP, int32_t LRP, int32_t IWL, int32_t FORMAT)
    //no bit clocl invert, slave, no DAC Left Right Clock Swap, Right Channel DAC data when DACLRC low, 16bit, I2S Format MSB left justfied
    setDigitalAudioInterfaceFormat(0,0,1,0,0,0x2);
    
    //setSamplingControl(int32_t CLKODIV2, int32_t CLKIDIV2, int32_t SR, int32_t BOSR, int32_t USBNORM)
    if(fs==48000){
	setSamplingControl(0, 0, 0, 0, 0);//48kHz
    }
    else if(fs==96000){
	setSamplingControl(0, 0, 0x7, 0, 0);//96kHz
    }
    
    setActiveControl(1);
}

void WM8731::setLeftLineIn(int32_t LRINBOTH, int32_t LINMUTE,  float _LINVOL)
{
    // LINVOL is a 5 bit number to set volume from 0x00 = -34.5 dB to ox1F = +12dB, 1.5dB steps, 0x17 is 0dB
    // LINMUTE, LRINBOTH are single bit
    if(_LINVOL<-34.5)_LINVOL=-34.5;
    if(_LINVOL>12)_LINVOL=12;
    int32_t LINVOL=round((_LINVOL+34.5)/1.5);
    int32_t val = ((LRINBOTH & 0x01) << 8) | ((LINMUTE & 0x01) << 7) | (LINVOL & 0x1F);
    writeRegister(LEFT_LINE_INPUT_CHANNEL_VOLUME_CONTROL, val);
}

void WM8731::setRightLineIn(int32_t RLINBOTH, int32_t RINMUTE, float _RINVOL)
{
    // RINVOL is a 5 bit number to set volume from 0x00 = -34.5 dB to ox1F = +12dB, 1.5dB steps, 0x17 is 0dB
    // RINMUTE, RLINBOTH are single bit
    if(_RINVOL<-34.5)_RINVOL=-34.5;
    if(_RINVOL>12)_RINVOL=12;
    int32_t RINVOL=round((_RINVOL+34.5)/1.5);
    int32_t val = ((RLINBOTH & 0x01) << 8) | ((RINMUTE & 0x01) << 7) | (RINVOL & 0x1F);
    writeRegister(RIGHT_LINE_INPUT_CHANNEL_VOLUME_CONTROL, val);
}

void WM8731::setLeftHeadphoneOut(int32_t LRHPBOTH, int32_t LZCEN, int32_t LHPVOL)
{
    //LHPVOL=-73~+6
    if(LHPVOL<-73)LHPVOL=0;
    if(LHPVOL>6)LHPVOL=6;
    LHPVOL+=48+73;//LHPVOL=0110000 to 11111111, 48(0110000) means -73dB, 127 means +6dB
    int32_t val = ((LRHPBOTH & 0x01) << 8) | ((LZCEN & 0x01) << 7) | (LHPVOL & 0x7F);
    writeRegister(LEFT_CHANNEL_HEADPHONE_VOLUME_CONTROL, val);
}

void WM8731::setRightHeadphoneOut(int32_t RLHPBOTH, int32_t RZCEN, int32_t RHPVOL)
{
    //RHPVOL=-73~+6
    if(RHPVOL<-73)RHPVOL=0;
    if(RHPVOL>6)RHPVOL=6;
    RHPVOL+=48+73;//RHPVOL=0110000 to 11111111, 48(0110000) means -73dB, 127 means +6dB
    int32_t val = ((RLHPBOTH & 0x01) << 8) | ((RZCEN & 0x01) << 7) | (RHPVOL & 0x7F);
    writeRegister(RIGHT_CHANNEL_HEADPHONE_VOLUME_CONTROL, val);
}

void WM8731::setAnalogueAudioPathControl(int32_t SIDEATT, int32_t SIDETONE, int32_t DACSEL, int32_t BYPASS, int32_t INSEL, int32_t MUTEMIC, int32_t MICBOOST)
{
    // Insert description
    int32_t val = ((SIDEATT & 0x03) << 6) | ((SIDETONE & 0x01) << 5) | ((DACSEL & 0x01) << 4) | ((BYPASS & 0x01) << 3) | ((INSEL & 0x01) << 2) | ((MUTEMIC & 0x01) << 1) | ((MICBOOST & 0x01) << 0);
    writeRegister(ANALOG_AUDIO_PATH_CONTROL, val);
}

void WM8731::setDigitalAudioPathControl(int32_t HPOR, int32_t DACMU, int32_t DEEMP, int32_t ADCHPD)
{
    // Description
    int32_t val =  ((HPOR & 0x01) << 4)  | ((DACMU & 0x01) << 3) | ((DEEMP & 0x03) << 1) | ((ADCHPD & 0x01) << 0);
    writeRegister(DIGITAL_AUDIO_PATH_CONTROL, val);
}

void WM8731::setPowerDownControl(int32_t POWEROFF, int32_t CLKOUTPD, int32_t OSCPD, int32_t OUTPD, int32_t DACPD, int32_t ADCPD, int32_t MICPD, int32_t LINEINPD)
{

    int32_t val = ((POWEROFF & 0x01) << 7) | ((CLKOUTPD & 0x01) << 6) | ((OSCPD & 0x01) << 5) | ((OUTPD & 0x01) << 4) | ((DACPD & 0x01) << 3) | ((ADCPD & 0x01) << 2) | ((MICPD & 0x01) << 1) | (LINEINPD & 0x01);
    writeRegister(POWER_DOWN_CONTROL, val);
}

void WM8731::setDigitalAudioInterfaceFormat(int32_t BCLKINV, int32_t MS, int32_t LRSWAP, int32_t LRP, int32_t IWL, int32_t FORMAT)
{

    int32_t val = ((BCLKINV & 0x01) << 7) | ((MS & 0x01) << 6) | ((LRSWAP & 0x01) << 5) | ((LRP & 0x01) << 4) | ((IWL & 0x03) << 2) | (FORMAT & 0x03);
    writeRegister(DIGITAL_AUDIO_INTERFACE_FORMAT, val);
}

void WM8731::setSamplingControl(int32_t CLKODIV2, int32_t CLKIDIV2, int32_t SR, int32_t BOSR, int32_t USBNORM)
{

    int32_t val = ((CLKODIV2 & 0x01) << 7) | ((CLKIDIV2 & 0x01) << 6) | ((SR & 0x0F) << 2) | ((BOSR & 0x01) << 1) | (USBNORM & 0x01);
    writeRegister(SAMPLE_RATE_CONTROL, val);
}

void WM8731::setActiveControl(int32_t ACTIVE)
{

    int32_t val = (ACTIVE & 0x01);
    writeRegister(DIGITAL_INTERFACE_ACTIVATION, val);
}

void WM8731::resetDevice()
{
    // Reset device when 0x00 is written
    writeRegister(RESET_REGISTER, 0x00);
}

void WM8731::writeRegister(int32_t addr, uint8_t data)
{
    uint8_t send_data[1] = {data};
    i2c.write(addr, send_data, 1);
}
