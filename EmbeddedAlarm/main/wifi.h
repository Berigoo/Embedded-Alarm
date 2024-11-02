#pragma once
#include "esp_wifi.h"
#include "cstring"
#include "nvs_flash.h"
#include "esp_log.h"

void initWifi(const char* ssid, const char* pass);
