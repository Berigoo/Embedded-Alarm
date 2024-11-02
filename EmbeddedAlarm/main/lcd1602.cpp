#include "lcd1602.h"
//TODO create mutex when drawing and setting texts

extern EventGroupHandle_t displayEvents;

static uint8_t isBacklightOn = 0x00;
static SemaphoreHandle_t lcdDrawWrite = nullptr;

#define MODE_LEN 2
static uint8_t currentModeIndex = 0;
static uint8_t modes[] = {MODE_1, MODE_2};

static char topText[17] = {' '};
static char botText[17] = {' '};

static void upperNibble(uint8_t data, uint8_t* out, uint8_t bit){
  uint8_t byteBacklight = (uint8_t)(isBacklightOn) & LCD_BACKLIGHT;
  uint8_t byteData = data & LCD_DATA_MASK;

  out[0] = byteData | byteBacklight | LCD_EN | bit;
  out[1] = byteData | byteBacklight | bit;
}

static void lowerNibble(uint8_t data, uint8_t* out, uint8_t bit){
  uint8_t byteBacklight = (uint8_t)(isBacklightOn) & LCD_BACKLIGHT;
  uint8_t byteData = (data << 4) /*& LCD_DATA_MASK*/ ;

  out[0] = byteData | byteBacklight | LCD_EN | bit;
  out[1] = byteData | byteBacklight | bit;
}

static void _write_bytes(i2c_master_dev_handle_t* devHandle, uint8_t data, uint8_t bit=0x00){
  uint8_t bytes[2];
  upperNibble(data, bytes, bit);
  ESP_ERROR_CHECK(i2c_master_transmit(*devHandle, bytes, 2, -1));

  lowerNibble(data, bytes, bit);
  ESP_ERROR_CHECK(i2c_master_transmit(*devHandle, bytes, 2, -1));

  vTaskDelay(10 / portTICK_PERIOD_MS);
}

void constructDevHandle(uint8_t deviceAddr, i2c_master_dev_handle_t* out){
  i2c_device_config_t conf = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = deviceAddr,
    .scl_speed_hz = 100000,
  };
  ESP_ERROR_CHECK(i2c_master_bus_add_device(masterBus, &conf, out));

  lcdDrawWrite = xSemaphoreCreateMutex();
  xSemaphoreGive(lcdDrawWrite);
}

void setBacklight(i2c_master_dev_handle_t* deviceHandle, bool data){
    isBacklightOn = (data) ? 0xFF : 0x00;
    _write_bytes(deviceHandle, 0x00);
}

void setInstruction(i2c_master_dev_handle_t* deviceHandle, uint8_t data){
    _write_bytes(deviceHandle, data);
}

void draw(void* pvParams){
  i2c_master_dev_handle_t* deviceHandle = (i2c_master_dev_handle_t*)pvParams;
  while(1){
    if(xSemaphoreTake(lcdDrawWrite, portMAX_DELAY)){
      setInstruction(deviceHandle, LCD::Intructions::CURSOR_X0_Y0);
      for (size_t i = 0; i < 16; i++) {
        _write_bytes(deviceHandle, topText[i], LCD_RS);
      }
      setInstruction(deviceHandle, LCD::Intructions::CURSOR_X0_Y1);
      for (size_t i = 0; i < 16; i++) {
        _write_bytes(deviceHandle, botText[i], LCD_RS);
      }
      xSemaphoreGive(lcdDrawWrite);
    }else{
      ESP_LOGW("CHECK", "waiting draw");
    }
    vTaskDelay(LCD_REFRESH / portTICK_PERIOD_MS);
  }
}

void setTopText(const char* text, bool fillBlank, uint8_t forMode){
  if(xSemaphoreTake(lcdDrawWrite, portMAX_DELAY)){
    bool write = false;
    if(forMode & modes[currentModeIndex]) write = true; 

    if(write){
      size_t len = (strlen(text) < 17) ? strlen(text) : 16;
      if(fillBlank && len < 16){
        const char* tmp = "                ";
        memcpy(&topText[len-1], tmp, 16 - len+1);
      }
      memcpy(topText, text, len);
    }
    xSemaphoreGive(lcdDrawWrite);
  }else{
      ESP_LOGW("CHECK", "waiting set top:  %s", text);
  }
}
void setBotText(const char* text, bool fillBlank, uint8_t forMode){
  if(xSemaphoreTake(lcdDrawWrite, portMAX_DELAY)){
    bool write = false;
    if(forMode & modes[currentModeIndex]) write = true; 

    if(write){
      size_t len = (strlen(text) < 17) ? strlen(text) : 16;
      if(fillBlank && len < 16){
        const char* tmp = "                ";
        memcpy(&botText[len-1], tmp, 16 - len+1);
      }
      memcpy(botText, text, len);
    }
    xSemaphoreGive(lcdDrawWrite);
  }else{
      ESP_LOGW("CHECK", "waiting set bot:  %s", text);
  }
}

void nextMode(){
  if(currentModeIndex < MODE_LEN-1){
    currentModeIndex++;
  }else{
    currentModeIndex = 0;
  }
  ESP_LOGW("CHECK", "Next mode");
  xEventGroupSetBits(displayEvents, ON_MODE_CHANGE);
}
uint8_t getMode(){
  return modes[currentModeIndex];
}