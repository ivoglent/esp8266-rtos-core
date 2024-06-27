#include "ConfigurationService.h"

ConfigurationService::ConfigurationService(Registry &registry, const std::string& version) : TService(registry) {
    _version = version;
    registry.getPropsLoader().addConsumer<ConfigurationService>(this);
    register_console_command("setup_wifi", setupWifiCmd, "ssid password", "Setup wifi with ssid and password");
    register_console_command("setup_mqtt", setupMqttCmd, "uri client_id home_id username password",
                             "Setup mqtt credentials");
    register_console_command("config_file", getConfigFileContent, "", "Get content of config file");
    register_console_command("file", getFileContent, "", "Get content of file");
    register_console_command("reset_conf", resetConfigCmd);
}

void ConfigurationService::apply(const AppProperties &props) {
    esp_logd(conf, "Got AppProperties props: type->{%s} api->{%s}", props.type.c_str(), props.api.c_str());
    _appProps = props;
}

void ConfigurationService::apply(const WifiProperties &props) {
    esp_logd(conf, "Got WifiProperties: ssid = %s", props.ssid.c_str());
    _wifiProps = props;
}

void ConfigurationService::apply(const MqttProperties &props) {
    esp_logd(conf, "Got MQTT props");
    _mqttProps = props;
    if (props.uri.empty()) {
        _configEmpty = true;
        esp_logw(conf, "Mqtt config is empty, waiting for wifi connection...");
    } else {
        _clientId = props.clientId;
        _homeId = props.homeId;
        _configEmpty = false;
    }

}

void ConfigurationService::onEvent(const SystemMissingMqtt &msg) {
    if (_configEmpty) {
        //_requestMqttConfig();
    }
}

void ConfigurationService::saveWifiConfigToFile(const WifiProperties &config) {
    esp_logi(conf, "Saving wifi configs to file");
    Storage<std::string> storage;
    storage.write(toJson(config), Env::getConfigFileName("wifi").c_str());
}

void ConfigurationService::saveMqttConfigToFile(const MqttProperties &config) {
    esp_logi(conf, "Saving mqtt configs to file");
    Storage<std::string> storage;
    storage.write(toJson(config), Env::getConfigFileName("mqtt").c_str());
}

void ConfigurationService::saveAppConfigToFile(const AppProperties& config) {
    esp_logi(conf, "Saving app configs to file");
    Storage<std::string> storage;
    storage.write(toJson(config), Env::getConfigFileName("app").c_str());
}


int ConfigurationService::setupWifiCmd(int argc, char **argv) {
    if (argc < 3) {
        esp_loge(conf, "Invalid params");
        return -1;
    }
    char *ssid = argv[1];
    char *password = argv[2];
    esp_logi(conf, "Setting wifi ssid=%s password=%s", ssid, password);
    auto service = getRegistryInstance().getService<ConfigurationService>();
    if (service) {
        service->setupWifi(ssid, password);
    } else {
        esp_loge(conf, "ConfigurationService not found!");
    }
    return 0;
}

void ConfigurationService::setupWifi(const char *ssid, const char *password) {
    esp_logi(conf, "Setup wifi ssid=%s and password=%s", ssid, password);

    size_t total = 0, used = 0;
    auto ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        esp_loge(conf, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        esp_logi(conf, "Partition size: total: %d, used: %d", total, used);
    }

    WifiProperties wifiProperties{};
    wifiProperties.ssid = std::string(ssid);
    wifiProperties.password = std::string(password);
    saveWifiConfigToFile(wifiProperties);
    system_restart();
}


int ConfigurationService::getConfigFileContent(int argc, char **argv) {
    if (argc < 2) {
        esp_loge(conf, "Please specified config name");
        return -1;
    }
    auto file = Env::getConfigFileName(argv[1]);
    esp_logi(conf, "Current config file: %s", file.c_str());
    Storage<std::string> storage;
    esp_logi(conf, "Config: %s", storage.readString(file.c_str()).c_str());
    return 0;
}

int ConfigurationService::getFileContent(int argc, char **argv) {
    if (argc < 2) {
        esp_loge(conf, "Please specified file name");
        return -1;
    }
    std::string file = std::string(argv[1]);
    file = "/spiffs/" + file;
    esp_logi(conf, "Current file: %s", file.c_str());
    Storage<std::string> storage;
    esp_logi(conf, "File: %s", storage.readString(file.c_str()).c_str());
    return 0;
}

void ConfigurationService::setupMqtt(const std::string &uri, const std::string &clientId, const std::string &homeId,
                                     const std::string &username, const std::string &password) {
    MqttProperties mqttProperties{};
    mqttProperties.uri = uri;
    mqttProperties.homeId = homeId;
    mqttProperties.clientId = clientId;
    mqttProperties.username = username;
    mqttProperties.password = password;
    saveMqttConfigToFile(mqttProperties);
    system_restart();
}

int ConfigurationService::setupMqttCmd(int argc, char **argv) {
    if (argc < 6) {
        esp_loge(conf, "Invalid params");
        return -1;
    }
    getRegistryInstance().getService<ConfigurationService>()->setupMqtt(argv[1], argv[2], argv[3], argv[4], argv[5]);
    return 0;
}

void ConfigurationService::setup() {
}

int ConfigurationService::resetConfigCmd(int argc, char **argv) {
    if (argc < 2) {
        esp_loge(conf, "Please specified config name");
        return -1;
    }
    Storage<std::string> storage;
    storage.remove(Env::getConfigFileName(argv[1]).c_str());
    esp_logi(conf, "Removed config %s file. Restarting...", argv[1]);
    esp_restart();
    return 0;
}

std::string ConfigurationService::getClientId() {
    return _clientId;
}

std::string ConfigurationService::getHomeId() {
    return _homeId;
}

void ConfigurationService::_requestMqttConfig() {
    esp_http_client_config_t config = {
            .host = SYSTEM_DOMAIN,
            .port = SYSTEM_PORT,
            .path = "/api/device/iot/register",
            .timeout_ms = 3000
    };
    esp_logi(conf, "Requesting for mqtt config to %s%s port: %d...", config.host, config.path, config.port);
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    std::string post_data;
    esp_http_client_set_post_field(client, post_data.c_str(), post_data.length());
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        esp_logi(conf, "HTTP POST Status = %d", esp_http_client_get_status_code(client));
        esp_logi(conf, "Content-Length = %d", esp_http_client_get_content_length(client));

        // Handle the response (if needed)
        int content_length = esp_http_client_get_content_length(client);
        if (content_length > 0) {
            char *buffer = static_cast<char *>(malloc(content_length + 1));
            if (buffer) {
                int len = esp_http_client_read(client, buffer, content_length);
                buffer[len] = '\0';
                esp_logi(conf, "Response: %s", buffer);
                auto json = cJSON_Parse(buffer);
                ServerResponse serverResponse{};
                fromJson(json, serverResponse);
                if (serverResponse.success) {
                    MqttProperties properties{};
                    properties.homeId = serverResponse.data.homeUuid;
                    properties.clientId = serverResponse.data.clientId;
                    properties.username = serverResponse.data.username;
                    properties.uri = serverResponse.data.uri;
                    properties.password = serverResponse.data.password;
                    saveMqttConfigToFile(properties);
                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                    esp_restart();
                } else {
                    esp_logw(conf, "Request mqtt config failed. Retry after 10s...");
                    _reconTimer.attach(30000, false, [this]() {
                        _requestMqttConfig();
                    });
                }
                free(buffer);
            } else {
                esp_loge(conf, "Failed to allocate memory for response");
            }
        } else {
            esp_logw(conf, "Request mqtt config failed. Retry after 10s...");
            _reconTimer.attach(30000, false, [this]() {
                _requestMqttConfig();
            });
        }

    } else {
        esp_loge(conf, "HTTP POST request failed: %s", esp_err_to_name(err));
        esp_logw(conf, "Request mqtt config failed. Retry after 10s...");
        _reconTimer.attach(30000, false, [this]() {
            _requestMqttConfig();
        });
    }

    esp_http_client_cleanup(client);
}

void ConfigurationService::onEvent(const SystemOpenConfig &msg) {
#ifndef CONFIG_IS_1MB_FLASH
    if (!_openedCp) {
        _openedCp = true;
        auto portal = new ConfigPortal(_version , _wifiProps, _appProps);
        portal->start();
    }
#else
    esp_logw(conf, "Config portal is not supported!");
#endif
}

ConfigurationService::~ConfigurationService() {

}




