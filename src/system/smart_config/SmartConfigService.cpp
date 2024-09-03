//
// Created by long.nguyenviet on 07/08/2024.
//

#include "SmartConfigService.h"

static const int CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;
static EventGroupHandle_t sc_wifi_event_group;

static void smartconfig_example_task(void *parm) {
    esp_logi(smcf, "Staring smart config...");
    EventBits_t uxBits;
    ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH));
    smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
    esp_esptouch_set_timeout(255);
    //esp_smartconfig_set_type(SC_TYPE_ESPTOUCH_V2);
    ESP_ERROR_CHECK(esp_smartconfig_start(&cfg));
    while (1) {
        uxBits = xEventGroupWaitBits(sc_wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false,
                                     portMAX_DELAY);
        if (uxBits & CONNECTED_BIT) {
            esp_logi(smcf, "WiFi Connected to ap");
        }
        if (uxBits & ESPTOUCH_DONE_BIT) {
            esp_logi(smcf, "Smartconfig over");
            esp_smartconfig_stop();
            vTaskDelete(NULL);
        }
    }
}

static void sc_event_handler(void *arg, esp_event_base_t event_base,
                             int32_t event_id, void *event_data) {
    if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD) {
        esp_logi(smcf, "Got SSID and password");

        smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *) event_data;
        wifi_config_t wifi_config;
        uint8_t ssid[33] = {0};
        uint8_t password[65] = {0};
        uint8_t rvd_data[33] = {0};

        bzero(&wifi_config, sizeof(wifi_config_t));
        memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
        wifi_config.sta.bssid_set = evt->bssid_set;

        if (wifi_config.sta.bssid_set == true) {
            memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
        }

        memcpy(ssid, evt->ssid, sizeof(evt->ssid));
        memcpy(password, evt->password, sizeof(evt->password));
        esp_logi(smcf, "SSID:%s", ssid);
        esp_logi(smcf, "PASSWORD:%s", password);
        if (evt->type == SC_TYPE_ESPTOUCH_V2) {
            ESP_ERROR_CHECK(esp_smartconfig_get_rvd_data(rvd_data, sizeof(rvd_data)));
            esp_logi(smcf, "RVD_DATA:%s", rvd_data);
        }

        auto service = getRegistryInstance().getService<ConfigurationService>();
        if (service) {
            service->setupWifi(reinterpret_cast<const char*>(ssid), reinterpret_cast<const char*>(password));
        } else {
            esp_loge(conf, "ConfigurationService not found!");
        }
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE) {
        xEventGroupSetBits(sc_wifi_event_group, ESPTOUCH_DONE_BIT);
    }

}

static int startSmartConfig(int argc, char **argv) {
    sc_wifi_event_group = xEventGroupCreate();
    esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &sc_event_handler, NULL);
    esp_wifi_disconnect();
    //wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    //esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();
    xTaskCreate(smartconfig_example_task, "smart_config_task", 4096, NULL, 5, NULL);
    return 0;
}

SmartConfigService::SmartConfigService(Registry &registry) : TService(registry) {
    register_console_command("smart_config", startSmartConfig, "", "Start smart config");
}

void SmartConfigService::setup() {
    startSmartConfig(0, nullptr);
}

SmartConfigService::~SmartConfigService() {

}