#pragma once

#include "../../Core.h"
#include "../storage/Storage.h"
#include "../console/Console.h"
#include "esp_http_client.h"
#ifdef CONFIG_SUPPROT_CONFIG_PORTAL
#include "ConfigPortal.h"
#endif
#include "../smart_config/SmartConfigService.h"
#include "../mdns/MdnsService.h"
#include "../../Timer.h"

#ifndef SYSTEM_PORT
#define SYSTEM_PORT 80
#endif
#ifndef SYSTEM_DOMAIN
#define SYSTEM_DOMAIN "home.quangiaso.com"
#endif

class ConfigurationService :
        public TService<ConfigurationService, Service_Sys_Conf, CORE>,
        public TPropertiesConsumer<ConfigurationService, AppProperties, WifiProperties, MqttProperties>,
        public TEventSubscriber<ConfigurationService, SystemMissingMqtt, SystemOpenConfig, SystemLookupDnsConfig> {
private:
    std::string _version;
    std::string _clientId{};
    std::string _homeId{};
    bool _configEmpty = true;
    EspTimer _reconTimer;
    void _requestMqttConfig();
    WifiProperties _wifiProps;
    MqttProperties _mqttProps;
    AppProperties _appProps;
    bool _openedCp = false;
public:
    explicit ConfigurationService(Registry &registry, const std::string& version);

    void apply(const WifiProperties &props);
    void apply(const MqttProperties &props);
    void apply(const AppProperties &props);

    void onEvent(const SystemMissingMqtt& msg);
    void onEvent(const SystemOpenConfig& msg);
    void onEvent(const SystemLookupDnsConfig& msg);

    static int setupWifiCmd(int argc, char **argv);

    static int setupMqttCmd(int argc, char **argv);

    void setupWifi(const char *ssid, const char *password);

    void setupMqtt(const std::string& uri, const std::string& clientId, const std::string& homeId, const std::string& username, const std::string& password);


    ~ConfigurationService() override;

    static int getConfigFileContent(int argc, char **argv);

    static int getFileContent(int argc, char **argv);

    void saveWifiConfigToFile(const WifiProperties& config);
    void saveMqttConfigToFile(const MqttProperties& config);
    void saveAppConfigToFile(const AppProperties& config);


    void setup() override;

    static int resetConfigCmd(int argc, char **argv);

    std::string getClientId();
    std::string getHomeId();
};
