#pragma once
#include "nvs.h"
#include "esp_spiffs.h"
#include "cJSON.h"
#include <ctime>
#include <cstring>
#include "esp_log.h"
struct text_t{
  uint32_t nvsid;
  char* id;
  char* text;
  time_t* due;
  time_t* scheduled;
  text_t* pNext;
};

void storageInit();
void write(text_t* task);
text_t* load();
void erase(const char* id);