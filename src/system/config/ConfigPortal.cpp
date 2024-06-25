//
// Created by long.nguyenviet on 09/06/2024.
//



#include "ConfigPortal.h"
#include "ConfigurationService.h"


#define HTTP_SERVER_PORT "80"
static WebConfig config;
static bool otaUploading = false;
httpd_handle_t server = nullptr;

static esp_err_t handle_get_config(httpd_req_t *req) {
    cJSON *root = cJSON_CreateObject();

    // Device
    cJSON *device = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "device", device);
    cJSON_AddStringToObject(device, "version", config.device.version.c_str());
    cJSON_AddStringToObject(device, "type", config.device.type.c_str());
    cJSON_AddStringToObject(device, "macAddress", config.device.macAddress.c_str());

    // Network
    cJSON *network = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "network", network);
    cJSON_AddStringToObject(network, "ssid", config.network.ssid.c_str());
    cJSON_AddStringToObject(network, "password", config.network.password.c_str());
    cJSON_AddStringToObject(network, "gateway", config.network.gateway.c_str());
    cJSON_AddStringToObject(network, "ip", config.network.ip.c_str());

    // System
    cJSON *system = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "system", system);
    cJSON_AddStringToObject(system, "api", config.system.api.c_str());

    // Convert to JSON string
    char *jsonString = cJSON_Print(root);
    // Clean up
    cJSON_Delete(root);
    esp_logd(CP, "Config : %s", jsonString);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, jsonString, (int) strlen(jsonString));
    free(jsonString);
    return ESP_OK;
}

static esp_err_t handle_post_config(httpd_req_t *req) {
    char buf[100]; // Buffer to store incoming data
    int ret, remaining = req->content_len;
    char *data = (char *)malloc(req->content_len);
    if (!data) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    // Read data received in chunks
    while (remaining > 0) {
        // Read the chunk
        ret = httpd_req_recv(req, buf, sizeof(buf));
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                // Retry receiving if timeout
                continue;
            }
            free(data);
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }

        // Copy the chunk into the data buffer
        memcpy(data + (req->content_len - remaining), buf, ret);
        remaining -= ret;
    }

    // Null-terminate the data to create a valid C string
    data[req->content_len] = '\0';
    esp_logd(CP, "Request body: %s", data);
    char* jsonString = "{}";
    cJSON *root = cJSON_Parse(data);
    if (root != nullptr) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != nullptr) {
            esp_loge(CP, "Error before: %s\n", error_ptr);
            jsonString = "{\"success\": false}";
        } else {
            // Read values
            char *wifi = cJSON_GetObjectItemCaseSensitive(root, "wifi")->valuestring;
            char *password = cJSON_GetObjectItemCaseSensitive(root, "password")->valuestring;
            char *api = cJSON_GetObjectItemCaseSensitive(root, "api")->valuestring;
            char *type = cJSON_GetObjectItemCaseSensitive(root, "type")->valuestring;
            jsonString = "{\"success\": true}";
            WifiProperties wifiProperties{};
            wifiProperties.ssid = wifi;
            wifiProperties.password = password;

            AppProperties appProperties{};
            appProperties.api = api;
            appProperties.type = type;

            auto configService = getRegistryInstance().getService<ConfigurationService>();
            configService->saveAppConfigToFile(appProperties);
            configService->saveWifiConfigToFile(wifiProperties);
            esp_logi(CP, "Stored configs successfully!");
            cJSON_Delete(root);
        }
    } else {
        esp_loge(CP, "JSON error! Can not parse configuration!");
        return ESP_FAIL;
    }

    std::string httpResponse =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: " + std::to_string(strlen(jsonString)) + "\r\n"
                                                                      "Connection: close\r\n" // Optionally close the connection
                                                                      "\r\n" // End of headers
            + jsonString; // Add JSON content after headers

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, httpResponse.c_str(), (int) httpResponse.length());
    esp_logi(CP, "Finished, restarting device...");
    vTaskDelay(100);
    system_restart();
    return ESP_OK;
}

static esp_err_t handle_index(httpd_req_t *req) {
    FILE* f = fopen("/spiffs/index.html", "r");
    if (!f) {
        esp_loge(CP, "Failed to open file for reading");
        return ESP_ERR_HTTPD_INVALID_REQ;
    }
    char* buffer;//[CHUNK_SIZE];
    buffer = (char*) malloc (CHUNK_SIZE);
    if (buffer == nullptr) {
        esp_loge(CP, "Error located buff to read file");
        return ESP_ERR_HTTPD_INVALID_REQ;
    }

    // Determine file size
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET); // Reset file pointer to the beginning for reading

    httpd_resp_set_type(req, "text/html");

    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, CHUNK_SIZE, f)) > 0) {
        httpd_resp_send_chunk(req, buffer, (int) bytesRead);
    }
    httpd_resp_send_chunk(req, nullptr, 0);
    esp_logd(CP, "Send file done!");
    // Clean up
    fclose(f);
    free(buffer);
    return ESP_OK;
}

void ota_update_callback(esp_httpd_ota_cb_event_t event) {
    if (event == esp_httpd_ota_cb_event_t::ESP_HTTPD_OTA_PRE_UPDATE) {
        esp_logi(CP, "Starting OTA update...");
    } else {
        esp_logi(CP, "OTA update finished!");
       /* vTaskDelay(1000 / portTICK_PERIOD_MS);
        esp_restart();*/
    }
}

httpd_uri_t index_uri_handle = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = handle_index,
        .user_ctx  = nullptr
};

httpd_uri_t get_config_uri_handle = {
        .uri       = "/get-config.json",
        .method    = HTTP_GET,
        .handler   = handle_get_config,
        .user_ctx  = nullptr
};

httpd_uri_t post_config_uri_handle = {
        .uri       = "/save-config.json",
        .method    = HTTP_POST,
        .handler   = handle_post_config,
        .user_ctx  = nullptr
};

// Starting the HTTP server as a separate task
void start_webserver() {
    //xTaskCreate(&http_server, "http_server", 10240, nullptr, 5, nullptr);
    httpd_config_t svConfig = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    esp_logi(CP, "Starting server on port: '%d'", svConfig.server_port);
    if (httpd_start(&server, &svConfig) == ESP_OK) {
        // Set URI handlers
        esp_logi(CP, "Registering URI handlers");
        httpd_register_uri_handler(server, &index_uri_handle);
        httpd_register_uri_handler(server, &get_config_uri_handle);
        httpd_register_uri_handler(server, &post_config_uri_handle);
        esp_httpd_ota_update_init(&ota_update_callback, server);
        //httpd_register_uri_handler(server, &echo);
        //httpd_register_uri_handler(server, &ctrl);
    }

}

static esp_err_t event_handler(void *ctx, system_event_t *event) {
    switch (event->event_id) {
        case SYSTEM_EVENT_AP_STACONNECTED:
            esp_logi("AP_STA", "Station connected to SoftAP");
            break;
        case SYSTEM_EVENT_AP_STADISCONNECTED:
            esp_logi("AP_STA", "Station disconnected from SoftAP");
            break;
        default:
            break;
    }
    return ESP_OK;
}

void ConfigPortal::start() {
    tcpip_adapter_init();
    esp_event_loop_init(event_handler, nullptr);
    // Initialize the Wi-Fi stack in AP+STA mode
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    // Set mode to AP+STA without stopping Wi-Fi
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    char name[18];
    get_client_id_from_mac_address(name);

    std::string apName = SOFT_AP_SSID;
    apName += name;
    esp_logd(CP, "AP name: %s", apName.c_str());
    wifi_ap_config_t apConfig = {
            SOFT_AP_SSID,
            SOFT_AP_PASS,
            0,
            SOFT_AP_CHANNEL,
            WIFI_AUTH_OPEN,
            0,
            SOFT_AP_MAX_CONN,
            0,
    };
    memcpy(apConfig.ssid, apName.c_str(), apName.size());
   // apConfig.ssid[apName.size()] = '\0';
    esp_logi(CP, "Opening Config Portal SSID: %s", apConfig.ssid);

    wifi_config_t wifiConfig = {
            .ap = apConfig
    };

    // Apply AP configuration
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifiConfig));

    esp_wifi_start();
    esp_logi("AP_STA", "Switched to AP+STA mode and configured AP.");
    start_webserver();
}

ConfigPortal::ConfigPortal(const std::string& version, const WifiProperties& wifi, const AppProperties& app) {
    config.system.api = app.api;
    config.network.ssid = wifi.ssid;
    config.network.password = wifi.password;
    config.system.api = app.api;

    char mac[18];
    get_wifi_mac_address(mac);
    config.device.macAddress = mac;
    config.device.type = app.type;
    config.device.version = version;
}
