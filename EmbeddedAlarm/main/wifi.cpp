#include "wifi.h"

void initWifi(const char* ssid, const char* pass){
  esp_netif_create_default_wifi_sta();
  wifi_init_config_t wifiConf = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&wifiConf));

  wifi_config_t conf = {
    .sta = {
      .ssid = "",
      .password = "",
      .threshold = {
        .authmode = WIFI_AUTH_WPA2_PSK,
      },
    },
  };
  strcpy((char*)conf.sta.ssid, ssid);
  strcpy((char*)conf.sta.password, pass);
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &conf));
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_ERROR_CHECK(esp_wifi_connect());
}
