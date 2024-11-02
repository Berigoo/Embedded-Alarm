#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <malloc.h>
#include <stdio.h>
#include "esp_err.h"
#include "esp_sntp.h"
#include "esp_event.h"
#include "esp_event_base.h"
#include "esp_log.h"
#include "esp_wifi_types_generic.h"
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "portmacro.h"
#include "i2c_master.h"
#include "lcd1602.h"
#include "lcd1602_instructions.h"
#include "soc/gpio_num.h"
#include "wifi.h"
#include "socket.h"
#include "storage.h"

//TODO create task counter on bottom-right
#define BUZZER_PIN GPIO_NUM_16

static TaskHandle_t task1Handle = nullptr;
static TaskHandle_t task2Handle = nullptr;

#define EVENT_TEXT_BREAK 0x01
static uint8_t state = 0x00;
static i2c_master_dev_handle_t lcd1;
static void rollingText(void* pvPrams);
static text_t* rolledTexts = nullptr;

static void obtainTime();
static void updateTime(void* params);

static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id,void *event_data);

static void startTCP(void* pvParams);
static void clientHandle(int socket);
static socket_t serverSocket = {
  .socket = -1,
  .callback = nullptr
};


//TODO create a tag like (ex: overdue, etc...) use a bitmask
#define TASK_OP_INSERT 0x01
#define TASK_OP_DELETE 0x02
#define TASK_OP_FETCH 0x08
#define TASK_OP_NEXT_MODE 0x10

#define ON_LIST_CHANGE 0x01
static void onChangeEvent(void* pvParams);
static text_t* textLists = nullptr;
EventGroupHandle_t displayEvents = xEventGroupCreate();

#define EVENT_ALARM_DUE 0x1
#define EVENT_ALARM_SCHEDULED 0x2
#define EVENT_ALARM_DEL 0x4
static text_t* currentRinging = nullptr;
static text_t* penultimateRinging = nullptr;
static void buzzerOn(gpio_num_t pin, uint32_t duration, uint32_t highLen, uint32_t lowLen);
static void alarmHandle(void* pvParams);
static EventGroupHandle_t alarmEvents = xEventGroupCreate();

#define TIMEZONE "UTC-7"

extern "C" void app_main(void)
{
  setenv("TZ", TIMEZONE, 1);
  tzset();

  ESP_LOGW("CHECK", "Time_t %zu", sizeof(time_t));
  gpio_set_direction(BUZZER_PIN, gpio_mode_t::GPIO_MODE_OUTPUT);

  createMasterBus();
  constructDevHandle(0x27, &lcd1);

  setBacklight(&lcd1, true);
  setInstruction(&lcd1, LCD::Intructions::CLEAR);
  setInstruction(&lcd1, LCD::Intructions::HOME);

  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  storageInit();
  textLists = load();
  initWifi("Rizki", "ajrt6330");
  esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);

  xTaskCreate(updateTime, "update_time", 4096, nullptr, 10, &task1Handle);

  {
    struct sockaddr_in in{0};
    in.sin_family = AF_INET;
    in.sin_addr.s_addr = htonl(INADDR_ANY);
    in.sin_port = htons(8080);
    serverSocket = createSocket(&in);
    if(serverSocket.socket >= 0){
      setCallback(&serverSocket, clientHandle);
      xTaskCreate(startTCP, "tcp_server", 4096, (void*)&serverSocket, 5, &task2Handle);
    }
  }

  xTaskCreate(alarmHandle, "alarm_handle", 4096, nullptr, 15, nullptr);

  rolledTexts = textLists;
  xTaskCreate(rollingText, "rolling_text", 4096, nullptr, 15, nullptr);

  xTaskCreate(draw, "draw_lcd1608", 4096, &lcd1, 15, nullptr);

  xTaskCreate(onChangeEvent, "on_change_event", 4096, nullptr, 15, nullptr);

  while(1){
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

//TODO unexpected output on syncing
static void obtainTime(){
  ESP_LOGI("TIME", "Initialize SNTP");
  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0, "pool.ntp.org");
  /*sntp_set_time_sync_notification_cb()*/
  sntp_init();
  while(sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET){
    ESP_LOGI("TIME", "syncing...");
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
  ESP_LOGI("TIME", "Obtaining time completed");
  //draw(&lcd1, "Completed!");

  vTaskDelay(1000 / portTICK_PERIOD_MS);
  setInstruction(&lcd1, LCD::Intructions::CLEAR);

  time_t now;
  char strftime_buf[64];
  struct tm* timeinfo;
  time(&now);
  timeinfo = localtime(&now);
  strftime(strftime_buf, sizeof(strftime_buf), "%m/%d %H:%M", timeinfo);
    
  setInstruction(&lcd1, LCD::Intructions::CURSOR_X0_Y1);
  setBotText(strftime_buf, true, MODE_1);
  
}

//TODO null terminated the time section
static void updateTime(void* params){
  while(1){
    time_t now;
    struct tm* timeinfo;

    char strftime_buf[64];

    time(&now);
    timeinfo = localtime(&now);
    strftime(strftime_buf, sizeof(strftime_buf), "%m/%d %H:%M", timeinfo);
    setBotText(strftime_buf, true, MODE_1);

    text_t* curr = textLists;
    text_t* penultimate = curr;
    while(curr){
      if(curr->scheduled){
        struct tm* scheduledTimeinfo = localtime(curr->scheduled);
        char scheduledBuf[64];
        strftime(scheduledBuf, sizeof(scheduledBuf), "%m/%d %H:%M", scheduledTimeinfo);
        ESP_LOGW("CHECK", "%s : %s = %d", strftime_buf, scheduledBuf, strcmp(strftime_buf, scheduledBuf));
        if(strcmp(strftime_buf, scheduledBuf) == 0){
          currentRinging = curr;
          xEventGroupSetBits(alarmEvents, EVENT_ALARM_SCHEDULED);
          break;
        }
      }
      if(curr->due){
        struct tm* dueTimeinfo = localtime(curr->due);
        char dueBuf[64];
        strftime(dueBuf, sizeof(dueBuf), "%m/%d %H:%M", dueTimeinfo);
        ESP_LOGW("CHECK", "%s : %s = %d", strftime_buf, dueBuf, strcmp(strftime_buf, dueBuf));
        if(strcmp(strftime_buf, dueBuf) == 0){
          currentRinging = curr;
          penultimateRinging = penultimate;
          /*xEventGroupSetBits(alarmEvents, EVENT_ALARM_DUE);*/
          xEventGroupSetBits(alarmEvents, EVENT_ALARM_DUE | EVENT_ALARM_DEL);
          break;
        }
      }
      if(curr != textLists){
        penultimate = penultimate->pNext;
      }
      curr = curr->pNext;
    }

    vTaskDelay(60000 / portTICK_PERIOD_MS);
  }
}

//TODO set timeout and try
static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id,void *event_data){
  if(event_id == WIFI_EVENT_STA_START){
    ESP_LOGI("NET", "STA starting");
    setInstruction(&lcd1, LCD::Intructions::CLEAR);
    setTopText("Connecting...", true);
    ESP_LOGI("NET", "Connecting...");
  } else if (event_id == WIFI_EVENT_STA_CONNECTED){
    ESP_LOGI("NET", "Connected");
    setInstruction(&lcd1, LCD::Intructions::CLEAR);
    
    setTopText("Connected!", true);
    

    time_t now = time(&now);
    struct tm* timeinfo = localtime(&now);
    if(timeinfo->tm_year < 100){
      setInstruction(&lcd1, LCD::Intructions::CLEAR);
      setInstruction(&lcd1, LCD::Intructions::HOME);
      setTopText("Obtaining time...");
      obtainTime();
    }

  } else if (event_id == WIFI_EVENT_STA_DISCONNECTED){
    ESP_LOGI("NET", "Disconnected");
    setInstruction(&lcd1, LCD::Intructions::CLEAR);
    esp_wifi_connect();
  }
}

static void startTCP(void* pvParams){
  socket_t* socket = (socket_t*)pvParams;
  start(socket);
}

//TODO write to non-volatile memory
//TODO create new task for running text referencing global string var, this function modify global string var
static void clientHandle(int socket){
  int len;
  char bufferRX[128];

  do{
    len = recv(socket, bufferRX, sizeof(bufferRX)-1, 0);
    bufferRX[len] = '\0';

    if(len < 0){
      ESP_LOGE("SOCKET", "Error occurred during receiving: errno %d", errno);
    } else if(len == 0){
      ESP_LOGW("SOCKET", "Connection closed");
    } else{

      uint8_t bit = bufferRX[0];
      ESP_LOGW("CHECK", "bit command: %02x", bit);

      if(bit & TASK_OP_INSERT){
        {
          //check id
          text_t* tmp = textLists;
          bool isExist = false;
          char* tmpId = (char*)malloc(33);
          tmpId[32] = '\0';
          memcpy(tmpId, &bufferRX[5], 32);
          while(tmp){
            if(strcmp(tmpId, tmp->id) == 0){
              isExist = true;
              break;
            }
            tmp = tmp->pNext;
          }
          free(tmpId);
          if(isExist){
            // current id is exist
            ESP_LOGW("SOCKET", "Data signature exist!");
            const char* data = "500";
            send(socket, data, strlen(data), 0);
            break;
          }
        }

        // allocate struct
        ESP_LOGI("SOCKET", "INSERT Command");
        text_t* curr = textLists;
        if(curr == nullptr){
          curr = (text_t*)malloc(sizeof(text_t));
          curr->pNext = nullptr;
          curr->due = nullptr;
          curr->scheduled = nullptr;
          textLists = curr;
          if(!curr){
            ESP_LOGE("SOCKET", "Failed allocating resources!");
            const char* res = "500";
            send(socket, res, strlen(res), 0);
            break;
          }
          rolledTexts = textLists;
        }else{
          while(curr->pNext){
            curr = curr->pNext;
          }
          curr->pNext = (text_t*)malloc(sizeof(text_t));
          curr->pNext->pNext = nullptr;
          curr->pNext->due = nullptr;
          curr->pNext->scheduled = nullptr;
          if(!curr){
            ESP_LOGE("SOCKET", "Failed allocating resources!");
            const char* res = "500";
            send(socket, res, strlen(res), 0);
            break;
          }
          curr = curr->pNext;
        }

        // allocating members of struct
        uint32_t strLen = *(uint32_t*)&bufferRX[1];
        ESP_LOGI("SOCKET", "INSERT %d", (int)strLen);
        curr->id = (char*)malloc(33);
        curr->id[32] = '\0';
        curr->due = (time_t*)malloc(sizeof(time_t));
        curr->scheduled= (time_t*)malloc(sizeof(time_t));
        curr->text = (char*)malloc(strLen+2);
        curr->text[strLen] = ' ';
        curr->text[strLen+1] = '\0';
        if(!curr->id || !curr->text){
          ESP_LOGE("SOCKET", "Failed allocating resources!");
          free(curr);
          const char* res = "500";
          send(socket, res, strlen(res), 0);
          break;
        }

        memcpy(curr->id, &bufferRX[5], 32);
        memcpy(curr->due, &bufferRX[5+32], sizeof(time_t));
        memcpy(curr->scheduled, &bufferRX[5+32+sizeof(time_t)], sizeof(time_t));
        *curr->due = *curr->due / 1000;
        *curr->scheduled = *curr->scheduled / 1000;

        size_t written = (strLen > (127-(1 + 4 + 32 + (sizeof(time_t)*2)))) ? (127-(1 + 4 + 32 + (sizeof(time_t)*2))) : strLen;
        memcpy(curr->text, &bufferRX[5+32+(sizeof(time_t)*2)], written);

        // data copy message greater than the rest of space
        size_t diff = strLen - written;
        for (size_t i = 0; i < (int(strLen / 127)); i++) {
          len = recv(socket, bufferRX, sizeof(bufferRX)-1, 0);
          //TODO create len exceptions
          if(len >= 0){
            uint32_t towrite = (diff > 127) ? 127 : diff;
            memcpy(&curr->text[written + (127*i)], bufferRX, towrite);
            written += towrite;
            diff = strLen - written;
          }
        }
        write(curr);
        xEventGroupSetBits(displayEvents, ON_LIST_CHANGE);

        // ret
        ESP_LOGI("SOCKET", "Message: %s; id: %s", curr->text, curr->id);
        const char* res = "ok";
        send(socket, res, strlen(res), 0);

        if(shutdown(socket, 2) < 0){
          ESP_LOGW("SOCKET", "Failed to shutdown");
        }
        break;
      }

      else if(bit & TASK_OP_DELETE){
        ESP_LOGI("SOCKET", "DELETE Command");

        bool deleted = false;
        text_t* curr = textLists;
        if(!curr){
          const char* res = "failed to delete";
          send(socket, res, strlen(res), 0);
          shutdown(socket, 2);
          break;
        }
        char id[33];
        id[32] = '\0';
        if(strcmp(curr->id, &bufferRX[1]) == 0){
          if(curr->pNext){
            textLists = curr->pNext;
            rolledTexts = textLists;
          }else{
            textLists = nullptr;
            rolledTexts = nullptr;
          }
          memcpy(id, curr->id, 32);
          free(curr->id);
          free(curr->text);
          free(curr->due);
          free(curr->scheduled);
          free(curr);
          deleted = true;
        }else{
          text_t* penultimate = curr;
          while(curr->pNext){
            curr = curr->pNext;
            if(strcmp(curr->id, &bufferRX[1]) == 0){
              if(curr->pNext){
                penultimate->pNext = curr->pNext;
              }else{
                penultimate->pNext = nullptr;
              }
              memcpy(id, curr->id, 32);
              free(curr->id);
              free(curr->text);
              free(curr->due);
              free(curr->scheduled);
              free(curr);
              deleted = true;
              break;
            }
            penultimate = penultimate->pNext;
          }
        }
        if(!deleted){
          ESP_LOGI("CHECK", "Failed to delete");
          const char* res = "failed to delete";
          send(socket, res, strlen(res), 0);
          break;
        }else{
          erase(id);
          xEventGroupSetBits(displayEvents, ON_LIST_CHANGE);

          //TODO less chaotic text break
          state = EVENT_TEXT_BREAK;
          const char* res = "ok";
          send(socket, res, strlen(res), 0);

          if(shutdown(socket, 2) < 0){
            ESP_LOGW("SOCKET", "Failed to shutdown");
          }
          break;
        }
      }

      else if(bit & TASK_OP_FETCH){
        ESP_LOGI("SOCKET", "FETCH Command");

        text_t* curr = textLists;
        while(curr){
          uint32_t packetSize = strlen(curr->text) + sizeof(time_t) * 2 + sizeof(uint32_t);
          send(socket, &packetSize, sizeof(uint32_t), 0);
          send(socket, curr->text, strlen(curr->text), 0);
          send(socket, curr->due, sizeof(time_t), 0);
          send(socket, curr->scheduled, sizeof(time_t), 0);
          curr = curr->pNext;
          char buf[128];
          recv(socket, buf, 128, 0);
        }
        if(shutdown(socket, 2) < 0){
          ESP_LOGW("SOCKET", "Failed to shutdown");
        }
        break;
      }

      else if(bit & TASK_OP_NEXT_MODE){
        nextMode();
        if(shutdown(socket, 2) < 0){
          ESP_LOGW("SOCKET", "Failed to shutdown");
        }
        break;
      }
    }
    ESP_LOGD("CHECK", "Waiting next message");
  }while(len > 0);
  close(socket);
}

static void rollingText(void* pvParams){
  while(1){
    text_t* curr = rolledTexts;
    while(curr){
      //TODO isntead doing this, create a global var or wait for event then modify it
      if(state & EVENT_TEXT_BREAK){
        state = 0x00;
        break;
      }
      if(curr->text){
        ESP_LOGW("CHECK", "rolled txt: %s", rolledTexts->text);
        uint32_t count = (strlen(curr->text) - 16 < 0) ? 0 : strlen(curr->text);
        setTopText(curr->text, true);
        vTaskDelay(500 / portTICK_PERIOD_MS);

        if(count > 16){
          size_t loop = (16*(size_t)((strlen(curr->text)/16) - 1) + ((strlen(curr->text) % 16)));
          for (size_t i = 0; i < loop; i++) {
            if(state & EVENT_TEXT_BREAK){
              break;
            }
            char c[17] = {0};
            memcpy(c, curr->text+i, 16);
            setTopText(c);
            vTaskDelay(800 / portTICK_PERIOD_MS);
          }
        }
      }
      vTaskDelay(2000 / portTICK_PERIOD_MS);
      curr = curr->pNext;
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

static void buzzerOn(gpio_num_t pin, uint32_t duration, uint32_t highLen, uint32_t lowLen){
  uint32_t count = duration / highLen;
  for (uint32_t i = 0; i < count; i++) {
    gpio_set_level(pin, 1);
    vTaskDelay(highLen / portTICK_PERIOD_MS);
    gpio_set_level(pin, 0);
    vTaskDelay(lowLen / portTICK_PERIOD_MS);
  }
}

static void alarmHandle(void* pvParams){
  while(1){
    EventBits_t bits = xEventGroupWaitBits(alarmEvents, EVENT_ALARM_DUE | EVENT_ALARM_SCHEDULED | (EVENT_ALARM_DUE | EVENT_ALARM_DEL), pdTRUE, pdFALSE, portMAX_DELAY);
    
    //TODO highlight the task
    text_t tmp;
    tmp.text = (char*)malloc(strlen(currentRinging->text)+1);
    memcpy(tmp.text, currentRinging->text, strlen(currentRinging->text)+1);
    rolledTexts = &tmp;
    state = EVENT_TEXT_BREAK;

    if(bits & EVENT_ALARM_DUE){
      ESP_LOGI("ALARM", "Task in due");
      buzzerOn(BUZZER_PIN, 10000, 500, 200);

      if(bits & (EVENT_ALARM_DUE | EVENT_ALARM_DEL)){
        if(currentRinging->pNext){
          if(penultimateRinging != currentRinging) penultimateRinging->pNext = currentRinging->pNext;
        }else{
          if(penultimateRinging != currentRinging) penultimateRinging->pNext = nullptr;
        }
	      erase(currentRinging->id);
	      free(currentRinging->id);
	      free(currentRinging->due);
	      free(currentRinging->scheduled);
	      free(currentRinging->text);
	      free(currentRinging);
	}

    }else if(bits & EVENT_ALARM_SCHEDULED){
      ESP_LOGI("ALARM", "Task in schedule");
      buzzerOn(BUZZER_PIN, 10000, 1000, 500);
    }

    rolledTexts = textLists;
    state = EVENT_TEXT_BREAK;
    free(tmp.text);
  }
}

static void onChangeEvent(void* pvParams){
  while(1){
    EventBits_t bits = xEventGroupWaitBits(displayEvents, ON_LIST_CHANGE | ON_MODE_CHANGE, pdTRUE, pdFALSE, portMAX_DELAY);

    ESP_LOGW("CHECK", "Enter");
    ESP_LOGW("CHECK", "mode: %02x", getMode());
    if(getMode() & MODE_2){
      ESP_LOGW("CHECK", "Enter 2");
      time_t now;
      time(&now);

      text_t* curr = textLists;
      if(curr){
        time_t initial = 100000000000;
        text_t* sel = curr;
        while(curr){
          time_t d = *curr->due - now;
          if(d < initial){
            initial = d;
            sel = curr;
          }
          curr = curr->pNext;
        }
        static text_t tmp;
        tmp.text = sel->text;
        tmp.due = sel->due;
        tmp.scheduled = sel->scheduled;
        tmp.pNext = nullptr;

        rolledTexts = &tmp;
        state = EVENT_TEXT_BREAK;
        struct tm* timeinfo;
        char strftime_buf[64];
        timeinfo = localtime(sel->due);
        strftime(strftime_buf, sizeof(strftime_buf), "%m/%d %H:%M", timeinfo);
        setBotText(strftime_buf, true, MODE_2);
      }else{
        static char* placeholder = "\0";
        static time_t duePlaceholder = 0;
        static text_t tmp;
        tmp.text = placeholder;
        tmp.due = &duePlaceholder;
        tmp.pNext = nullptr;
        rolledTexts = &tmp;
        struct tm* timeinfo = localtime(tmp.due);
        char strftime_buf[64];
        strftime(strftime_buf, sizeof(strftime_buf), "%m/%d %H:%M", timeinfo);
        setBotText(strftime_buf, true, MODE_2);

      }
    }

    else if(getMode() & MODE_1){
      rolledTexts = textLists;
      if(!rolledTexts){
        static char* placeholder = "\0";
        static time_t duePlaceholder = 0;
        static text_t tmp;
        tmp.text = placeholder;
        tmp.due = &duePlaceholder;
        tmp.pNext = nullptr;
        rolledTexts = &tmp;
      }

      time_t now;
      time(&now);
      struct tm* timeinfo;
      char strftime_buf[64];
      timeinfo = localtime(&now);
      strftime(strftime_buf, sizeof(strftime_buf), "%m/%d %H:%M", timeinfo);
      setBotText(strftime_buf, true, MODE_1);
    }
  }
}