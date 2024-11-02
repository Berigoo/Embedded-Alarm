#pragma once
#include <stdint.h>
#include "driver/i2c_types.h"
#include "cstring"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "portmacro.h"
#include "driver/i2c_master.h"
#include "lcd1602_instructions.h"
#include "i2c_master.h"

#define LCD_REFRESH 1000
#define LCD_BACKLIGHT 0x08
#define LCD_EN 0x04
#define LCD_RW 0x02
#define LCD_RS 0x01
#define LCD_D7 0x80
#define LCD_D4 0x10

#define LCD_DATA_MASK 0xF0 // 4 bit chunks

#define MODE_ALL 0xFF
#define MODE_1 0x02
#define MODE_2 0x04

#define ON_MODE_CHANGE 0x02
void constructDevHandle(uint8_t deviceAddr, i2c_master_dev_handle_t* out);

void setTopText(const char* text, bool fillBlank=false, uint8_t forMode=0xFF);
void setBotText(const char* text, bool fillBlank=false, uint8_t forMode=0xFF);
void nextMode();
uint8_t getMode();
void setBacklight(i2c_master_dev_handle_t* deviceHandle, bool data);
void setInstruction(i2c_master_dev_handle_t* deviceHandle, uint8_t data);
void draw(void* pvParams);