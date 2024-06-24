#pragma once

#include <esp_event.h>
#include <freertos/event_groups.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <arpa/inet.h>

#include "../../Core.h"
#include "../../Event.h"

class WifiService
        : public TService<WifiService, Service_Sys_Wifi, CORE>,
          public TPropertiesConsumer<WifiService, WifiProperties>,
          public TEventSubscriber<WifiService, SystemConfigReady>
          {
    std::string _ssid;
    std::string _password;
    esp_event_handler_t _wifiEventHandle = nullptr;
    esp_event_handler_t _ipEventHandle = nullptr;
    EventGroupHandle_t s_wifi_event_group;
private:
    std::string _ip{};
public:
    void setIp(const std::string &ip);

    void setGateway(const std::string &gateway);

private:
    std::string _gateway{};
public:
    explicit WifiService(Registry &registry);

    void apply(const WifiProperties &props);

    void stop();

    void start(const std::string& ssid, const std::string& password);

    ~WifiService() override;

    void onEvent(const SystemConfigReady &event);

    void setup() override;

    const std::string &getIp() const;

    const std::string &getGateway() const;
};
