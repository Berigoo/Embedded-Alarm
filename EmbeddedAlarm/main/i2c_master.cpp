#include "i2c_master.h"

i2c_master_bus_handle_t masterBus = nullptr;

void createMasterBus(){
  i2c_master_bus_config_t conf = {
    .i2c_port = I2C_PORT,
    .sda_io_num = (gpio_num_t)CONFIG_SDA_PIN,
    .scl_io_num = (gpio_num_t)CONFIG_SCL_PIN,
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .glitch_ignore_cnt = 7,
    .flags = {
      .enable_internal_pullup = true,
    },
  };
  ESP_ERROR_CHECK(i2c_new_master_bus(&conf, &masterBus));
}
