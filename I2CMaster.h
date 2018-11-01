//
//  Created by Evixar Inc.
//  Copyright © 2018 Evixar Inc. All rights reserved.
//

#ifndef __I2CMASTER_H__
#define __I2CMASTER_H__

#include "freertos/FreeRTOS.h"
#include "driver/i2c.h"

class I2CMaster
{
private:
    bool started;
    i2c_config_t i2c_config;
    uint8_t device_addr;
    
public:
    I2CMaster();
    ~I2CMaster();
    void initialize(uint8_t addr, gpio_num_t sda, gpio_num_t scl);
    void start();
    void stop();
    esp_err_t write(uint8_t control_address, uint8_t* data=nullptr, size_t data_size=0U);
    esp_err_t read(uint8_t control_address, uint8_t* data, size_t data_size);
};

#endif//