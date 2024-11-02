#include "storage.h"

static const char* TAG = "STORAGE";

static const char* taskFilepath = "/storage/tasks.bin";

static nvs_handle_t handle;

void storageInit(){

  esp_vfs_spiffs_conf_t conf = {
    .base_path = "/storage",
    .partition_label = nullptr,
    .max_files = 3,
    .format_if_mount_failed = true,
  };
  int ret = esp_vfs_spiffs_register(&conf); 
  if(ret != ESP_OK){
    ESP_LOGE(TAG, "Failed to mount fs");
  }

  /*if(remove(taskFilepath) == 0){*/
  /*  ESP_LOGW("CHECK", "task file deleted");*/
  /*};*/
}

void write(text_t* task){
  FILE* f = fopen(taskFilepath, "ab");
  if(!f){
    ESP_LOGE("STORAGE", "Failed to open %s", taskFilepath);
    return;
  }

  //TODO malloc
  char buffer[1024];
  uint32_t len = strlen(task->text);
  uint32_t finalLen = 32 + 8 + 8 + 4 + len;
  memcpy(buffer, task->id, 32);
  memcpy(&buffer[32], &len, 4);
  memcpy(&buffer[32+4], task->due, 8);
  memcpy(&buffer[32+4+8], task->scheduled, 8);
  memcpy(&buffer[32+4+8+8], task->text, len);
  
  size_t written = fwrite(buffer, 1, finalLen, f);
  if(written != finalLen){
    ESP_LOGE(TAG, "Failed to fully write the buffer");
    //TODO doing cleanup
    fclose(f);
    return;
  }
  fclose(f);
  ESP_LOGW("CHECK", "writing: %s", &buffer[32+4+8+8]);
}

text_t* load(){
  FILE* f = fopen(taskFilepath, "rb");
  if(!f){
    ESP_LOGE("STORAGE", "Failed to open %s", taskFilepath);
    fclose(f);
    return nullptr;
  }
  text_t* head = nullptr;
  text_t* penulimate = head;
  char buffer[52];
  while(fread(buffer, 1, 52, f)){
    text_t* t  = (text_t*)malloc(sizeof(text_t));
    t->pNext = nullptr;

    t->id = (char*)malloc(32+1);
    t->id[32] = '\0';
    memcpy(t->id, buffer, 32);

    t->due = (time_t*)malloc(sizeof(time_t));
    t->scheduled = (time_t*)malloc(sizeof(time_t));
    memcpy(t->due, &buffer[32+4], 8);
    memcpy(t->scheduled, &buffer[32+4+8], 8);

    uint32_t stringlen;
    memcpy(&stringlen, &buffer[32], 4);
    t->text = (char*)malloc(stringlen+1);
    t->text[stringlen] = '\0';
    fread(t->text, 1, stringlen, f);
    
    if(!head){
      head = t;
      penulimate = head;
    }else{
      penulimate->pNext = t;
      penulimate = penulimate->pNext;
    }
  }
  fclose(f);
  return head;
}

void erase(const char* id){
  FILE* f = fopen(taskFilepath, "rb");

  uint32_t count = 0;
  char buffer[36];
  while(fread(buffer, 1, 36, f)){
    char _id[33];
    _id[32] = '\0';
    memcpy(_id, buffer, 32);

    //TODO make this without allocation
    if(strcmp(_id, id) == 0){
      char buffer2[256];

      fseek(f, 0, SEEK_SET);
      FILE* _f = fopen("/storage/.task_tmp.bin", "wb");
      uint32_t readBytes = 0;
      uint32_t totalBytes = 0;
      bool del = false;

      const uint32_t deletedLen = 32 + 4 + 8 + 8 + *(uint32_t*)&buffer[32];
      const uint32_t deletedPos = count;
      
      while((readBytes = fread(buffer2, 1, 256, f))){
        totalBytes += readBytes;
        if(totalBytes > deletedPos && !del){
          uint32_t diff = readBytes - (totalBytes - deletedPos);
          fwrite(buffer2, 1, diff, _f);
          fseek(f, deletedPos + deletedLen, SEEK_SET);
          del = true;
          continue;
        }
        fwrite(buffer2, 1, readBytes, _f);
      }

      remove(taskFilepath);
      rename("/storage/.task_tmp.bin", taskFilepath);
      fclose(_f);
      
      break;
    }

    count += 32 + 8 + 8 + 4 + *(uint32_t*)&buffer[32];
    fseek(f, count, SEEK_SET);
  }
  fclose(f);
};