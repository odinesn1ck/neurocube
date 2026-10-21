#pragma once
#include <stdio.h>
#include <stdbool.h>
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#define printf(...) ((void)0) /* Keep production CDC strictly NC1 JSON. */
#define SENSOR_I2C_PORT i2c1
void DEV_Delay_Ms(uint32_t);
void DEV_I2C_Write_Byte(i2c_inst_t *, uint8_t, uint8_t, uint8_t);
uint8_t DEV_I2C_Read_Byte(i2c_inst_t *, uint8_t, uint8_t);
