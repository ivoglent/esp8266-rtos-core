//
// Created by long.nguyenviet on 21/11/2023.
//
#ifndef CONFIG_SUPPORT_OTA_UPDATE
#include "OtaService.h"

static char urlUpdate[256];

esp_err_t otaHttpDownloadEventHandler(esp_http_client_event_t *evt) {
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            esp_logi(ota, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            esp_logi(ota, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            esp_logi(ota, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            esp_logi(ota, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            esp_logd(ota, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            break;
        case HTTP_EVENT_ON_FINISH:
            esp_logi(ota, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            esp_logi(ota, "HTTP_EVENT_DISCONNECTED");
            break;
    }
    return ESP_OK;
}

void otaUpdateTask(void *pvParameter) {
    esp_logi(ota, "Start OTA update with URL: %s", urlUpdate);
    esp_http_client_config_t clientConfig = {
            .url = urlUpdate,
            .auth_type = HTTP_AUTH_TYPE_NONE,
            .timeout_ms = 180000,
            .event_handler = otaHttpDownloadEventHandler,
            .transport_type = HTTP_TRANSPORT_OVER_SSL,
            .skip_cert_common_name_check = true,

    };

    esp_err_t ret = esp_https_ota(&clientConfig);
    if (ret == ESP_OK) {
        esp_logi(ota, "Firmware upgrade successfully, restarting...");
        vTaskDelay(500 / portTICK_PERIOD_MS);
        esp_restart();
    } else {
        esp_logi(ota, "Firmware upgrade failed");
    }

    vTaskDelete(nullptr);
}

OtaService::OtaService(Registry &reg) : TService(reg) {

}

void OtaService::onEvent(const OtaEvent &event) {
    esp_logi(ota, "OTA update for %s chip with fw: %s", event.data.extData.type, event.data.url);
    memcpy(urlUpdate, event.data.url, strlen(event.data.url));
    urlUpdate[strlen(event.data.url) + 1] = '\0';
    xTaskCreate(&otaUpdateTask, "otaUpdateTask", 8192, this, 5, nullptr);
}

void OtaService::onEvent(const ReportVersionEvent &evt) {
    if (!_reported) {
        OtaVersion event{};
        strcpy(event.version, evt.version);
        strcpy(event.module, CONFIG_APP_MODULE);
        getBus().post(event);
        esp_logi(ota, "Reported OTA version: %s", event.version);
        _reported = true;
    }
}

void OtaService::setup() {
}

OtaService::~OtaService() {

}

#endif