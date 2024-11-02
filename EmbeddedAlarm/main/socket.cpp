#include "socket.h"
//TODO mutex
#define KEEP_ALIVE_ENABLE 1
#define KEEP_ALIVE_IDLE 60
#define KEEP_ALIVE_INTERVAL 10
#define KEEP_ALIVE_MAX_COUNT 10

static void _handleClient(void* pvParams);

static const char* TAG = "SOCKET";

socket_t createSocket(sockaddr_in* destAddr){
  socket_t sock = {
    .socket = -1,
    .callback = nullptr,
  };

  ESP_LOGI(TAG, "Creating socket...");
  sock.socket = socket(destAddr->sin_family, SOCK_STREAM, 0);
  if(sock.socket < 0){
    ESP_LOGE(TAG, "Failed creating socket!");
    return sock;
  }
  int opt = 1;
  if(setsockopt(sock.socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) < 0){
    ESP_LOGE(TAG, "Failed to set option!");
    close(sock.socket);
    return sock;
  }
  ESP_LOGI(TAG, "Socket created!");

  ESP_LOGI(TAG, "Binding socket...");
  int err = bind(sock.socket, (sockaddr*)destAddr, sizeof(sockaddr_in));
  if(err < 0){
    ESP_LOGE(TAG, "Failed to bind! errno %d", errno);
    close(sock.socket);
    return sock;
  }
  ESP_LOGI(TAG, "Socket bound on %d", htons(destAddr->sin_port));
  
  err = listen(sock.socket, 1);
  if(err < 0){
    ESP_LOGE(TAG, "Failed to listen! errno %d", errno);
    close(sock.socket);
    return sock;
  }

  return sock;
}

void setCallback(socket_t* socket, void (*callback)(int)){
  if(callback && socket->socket >= 0){
    socket->callback = callback;
  }
}

void start(socket_t* socket){
  while(1){
    int sock = accept(socket->socket, nullptr, nullptr);
    ESP_LOGW(TAG, "Connection established!");
    socket_t* clientSocket = (socket_t*)malloc(sizeof(socket_t));
    clientSocket->socket = sock;
    clientSocket->callback = socket->callback;
    xTaskCreate(_handleClient, "handle_client", 4096, (void*)clientSocket, 5, nullptr);
  }
}

//TODO make sure not leak
static void _handleClient(void* pvParams){
  socket_t* socket = (socket_t*)pvParams;
  if(socket->socket < 0){
    ESP_LOGE(TAG, "Could not accept the connection!");
  }else{
    int enable = KEEP_ALIVE_ENABLE;
    int idle = KEEP_ALIVE_IDLE;
    int interval = KEEP_ALIVE_INTERVAL;
    int cnt = KEEP_ALIVE_MAX_COUNT;
    setsockopt(socket->socket, SOL_SOCKET, SO_KEEPALIVE, &enable, sizeof(int));
    setsockopt(socket->socket, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(int));
    setsockopt(socket->socket, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(int));
    setsockopt(socket->socket, IPPROTO_TCP, TCP_KEEPCNT, &cnt, sizeof(int));

    socket->callback(socket->socket);
  }
  ESP_LOGW("SOCKET", "Socket closed");
  free(socket);
  vTaskDelete(nullptr);
}