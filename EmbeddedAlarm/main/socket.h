#pragma once
#include <sys/param.h>
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"

struct socket_t{
  int socket;
  void (*callback)(int);
};

socket_t createSocket(sockaddr_in* destAddr);
void setCallback(socket_t* socket, void (*callback)(int));
void start(socket_t* socket);
