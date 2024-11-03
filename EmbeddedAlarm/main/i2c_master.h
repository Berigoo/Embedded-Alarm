#pragma once

#include "driver/i2c_master.h"

// esp32-devkitc-32D
#define I2C_PORT I2C_NUM_0

extern i2c_master_bus_handle_t masterBus;

void createMasterBus();
