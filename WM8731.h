//
//  Created by Evixar Inc.
//  Copyright © 2018 Evixar Inc. All rights reserved.
//

#ifndef __WM8731_H__
#define __WM8731_H__

#include "freertos/FreeRTOS.h"
#include "I2CMaster.h"

#ifndef I2C_ADR
#define I2C_ADR 0x1A
#endif
#ifndef I2C_SDIN
#define I2C_SDIN 21
#endif
#ifndef I2C_SCLK
#define I2C_SCLK 22
#endif

class WM8731
{
public:
    WM8731();
    void initialize(gpio_num_t i2c_sda, gpio_num_t i2c_scl, uint8_t i2c_addr);
    void start();
    void setup(int32_t fs, bool useMic, bool micBoost);
    void stop();
private:
    I2CMaster i2c;
    void writeRegister(int addr, uint8_t data);

    void setLeftLineIn(int32_t LRINBOTH, int32_t LINMUTE, float LINVOL);
    void setRightLineIn(int32_t RLINBOTH, int32_t RINMUTE, float RINVOL);
    void setLeftHeadphoneOut(int32_t LRHPBOTH, int32_t LZCEN, int32_t LHPVOL);
    void setRightHeadphoneOut(int32_t RLHPBOTH, int32_t RZCEN, int32_t RHPVOL);
    void setAnalogueAudioPathControl(int32_t SIDEATT, int32_t SIDETONE, int32_t DACSEL, int32_t BYPASS, int32_t INSEL, int32_t MUTEMIC, int32_t MICBOOST);
    void setDigitalAudioPathControl(int32_t HPOR, int32_t DACMU, int32_t DEEMP, int32_t ADCHPD);
    void setPowerDownControl(int32_t POWEROFF, int32_t CLKOUTPD, int32_t OSCPD, int32_t OUTPD, int32_t DACPD, int32_t ADCPD, int32_t MICPD, int32_t LINEINPD);
    void setDigitalAudioInterfaceFormat(int32_t BCLKINV, int32_t MS, int32_t LRSWAP, int32_t LRP, int32_t IWL, int32_t FORMAT);
    void setSamplingControl(int32_t CLKODIV2, int32_t CLKIDIV2, int32_t SR, int32_t BOSR, int32_t USBNORM);
    void setActiveControl(int32_t ACTIVE);
    void resetDevice();
};

#endif//