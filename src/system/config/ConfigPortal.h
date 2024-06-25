#pragma once
#include "../../Logger.h"
#include "../../Properties.h"
#include <esp_wifi_types.h>
#include <esp_wifi.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <cstdint>
#include "esp_http_server.h"
#include <esp_ota_ops.h>

#include <utility>
#include "../../Utils.h"
#include "../ota/esp_httpd_ota.h"
#include <strstream>
#define SOFT_AP_SSID     "Home-IoT-"
#define SOFT_AP_PASS     ""
#define SOFT_AP_MAX_CONN 1
#define SOFT_AP_CHANNEL  1
#define CHUNK_SIZE 512

struct WebDevice {
    std::string version;
    std::string type;
    std::string macAddress;
};
struct WebNetwork {
    std::string ssid;
    std::string password;
    std::string gateway;
    std::string ip;
};
struct WebSystem {
    std::string api;
};
struct WebConfig {
    WebDevice device;
    WebNetwork network;
    WebSystem system;
};

class ConfigPortal {
private:
    TaskHandle_t _serverTaskHandle{};
public:
    ConfigPortal(const std::string& version, const WifiProperties&  wifi, const AppProperties&  app);
    void start();
};
