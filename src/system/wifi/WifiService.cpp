#include "WifiService.h"
uint32_t _retryCount = 0;
static std::string ip4addrToString(const ip4_addr_t& ip)
{
    // inet_ntoa converts the IP address to a C-style string in IPv4 dotted-decimal notation.
    // Note that inet_ntoa expects in_addr structure which is compatible with ip4_addr_t.
    const char* cstr_ip = inet_ntoa(*reinterpret_cast<const in_addr*>(&ip));

    // Construct and return an std::string from the C-style string.
    return cstr_ip;
}
WifiService::WifiService(Registry &registry) : TService(registry) {
    registry.getPropsLoader().addConsumer<WifiService>(this);
}

static esp_err_t generalEventHandler(void *ctx, system_event_t *event) {
    return ESP_OK;
}

void eventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_logi(wifi, "Wifi started, connecting...");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        auto *reason = static_cast<wifi_event_sta_disconnected_t *>(event_data);
        esp_logw(wifi, "Wifi disconnected! Reason = %d", reason->reason);
        getDefaultEventBus().post(SystemEventChanged{SystemStatus::WIFI_DISCONNECTED});
        xEventGroupClearBits(Registry::SYSTEM_STATE_BITs, SYS_WIFI_READY);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        _retryCount++;
        if (_retryCount >= 60) {
            getDefaultEventBus().post(SystemOpenConfig{});
        }
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        esp_logi(wifi, "Connected!");
        _retryCount = 0;
        getDefaultEventBus().post(SystemEventChanged{SystemStatus::WIFI_CONNECTED,});
        auto *event = (ip_event_got_ip_t *) event_data;
        esp_logi(wifi, "got ip: " IPSTR, IP2STR(&event->ip_info.ip));
        auto self = static_cast<WifiService*>(arg);
        self->setIp(ip4addrToString(event->ip_info.ip));
        self->setGateway(ip4addrToString(event->ip_info.gw));
        xEventGroupSetBits(Registry::SYSTEM_STATE_BITs, SYS_WIFI_READY);

    } else {
        esp_logi(wifi, "wifi event: %d", event_id);
    }
}

void WifiService::apply(const WifiProperties &props) {
    _ssid = props.ssid;
    _password = props.password;
    esp_logi(wifi, "Got WifiProperties!");
}

void WifiService::stop() {
    if (_wifiEventHandle != nullptr) {
        ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, _wifiEventHandle));
    }
    if (_ipEventHandle != nullptr) {
        ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, ESP_EVENT_ANY_ID, _ipEventHandle));
    }
    _wifiEventHandle = nullptr;
    _ipEventHandle = nullptr;
    esp_wifi_disconnect();
    esp_wifi_stop();
    esp_wifi_deinit();
    esp_netif_deinit();
    xEventGroupClearBits(Registry::SYSTEM_STATE_BITs, SYS_WIFI_READY);
    vEventGroupDelete(s_wifi_event_group);
}

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

void WifiService::start(const std::string &ssid, const std::string &password) {
    s_wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();
    esp_event_loop_init(generalEventHandler, NULL);
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &eventHandler, this));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &eventHandler, this));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    if (_ssid.empty()) {
        esp_loge(wifi, "Wifi SSID is empty. Stopped!");
        getBus().post(SystemOpenConfig{});
        return;
    }
    wifi_config_t wifi_config{};
    memcpy(wifi_config.sta.ssid, ssid.c_str(), ssid.size());
    memcpy(wifi_config.sta.password, password.c_str(), password.size());

    if (strlen((char *)wifi_config.sta.password)) {
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start());

    esp_logi(wifi, "wifi_init_sta finished.");

}

WifiService::~WifiService() {
    stop();
}

void WifiService::onEvent(const SystemConfigReady &event) {
    start(_ssid, _password);
}

void WifiService::setup() {

}

const std::string &WifiService::getIp() const {
    return _ip;
}

const std::string &WifiService::getGateway() const {
    return _gateway;
}

void WifiService::setIp(const std::string &ip) {
    _ip = ip;
}

void WifiService::setGateway(const std::string &gateway) {
    _gateway = gateway;
}
