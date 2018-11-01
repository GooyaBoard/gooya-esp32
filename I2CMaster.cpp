//
//  Created by Evixar Inc.
//  Copyright © 2018 Evixar Inc. All rights reserved.
//

#include "I2CMaster.h"

#define I2C_PORT I2C_NUM_1
#define ACK_CHECK_EN    0x1 /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS   0x0 /*!< I2C master will not check ack from slave */
#define ACK_VAL         0x0 /*!< I2C ack value */
#define NACK_VAL        0x1 /*!< I2C nack value */

I2CMaster::I2CMaster()
{
    started=false;
}

void I2CMaster::initialize(uint8_t addr, gpio_num_t sda, gpio_num_t scl)
{
    device_addr = addr;
    
    //i2c_config_t conf;
    i2c_config.mode = I2C_MODE_MASTER;
    i2c_config.sda_io_num = sda;
    i2c_config.sda_pullup_en = GPIO_PULLUP_ENABLE;
    i2c_config.scl_io_num = scl;
    i2c_config.scl_pullup_en = GPIO_PULLUP_ENABLE;
    i2c_config.master.clk_speed = 100000;
}

I2CMaster::~I2CMaster()
{
    stop();
}

void I2CMaster::start()
{
    if(!started)
    {
        started=true;
        i2c_param_config(I2C_PORT, &i2c_config);
        i2c_driver_install((i2c_port_t)I2C_PORT, i2c_config.mode, 0, 0, 0);
    }
}

void I2CMaster::stop()
{
    if(started)
    {
        started=false;
        i2c_driver_delete((i2c_port_t)I2C_PORT);
    }
    
}

esp_err_t I2CMaster::write(uint8_t control_address, uint8_t* data, size_t data_size)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, ( device_addr << 1 ) | I2C_MASTER_WRITE, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, control_address, ACK_CHECK_EN);
    if (data_size!=0) {
        i2c_master_write(cmd, data, data_size, ACK_CHECK_EN);
    }
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, cmd, portMAX_DELAY);
    i2c_cmd_link_delete(cmd);
    return ret;
}

esp_err_t I2CMaster::read(uint8_t control_address, uint8_t* data, size_t data_size)
{
    if (data_size == 0) {
        return ESP_OK;
    }

    write(control_address);
    
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, ( device_addr << 1 ) | I2C_MASTER_READ, ACK_CHECK_EN);
    if (data_size > 1) {
        i2c_master_read(cmd, data, data_size - 1, (i2c_ack_type_t)ACK_VAL);
    }
    i2c_master_read_byte(cmd, data + data_size - 1, (i2c_ack_type_t)NACK_VAL);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}
