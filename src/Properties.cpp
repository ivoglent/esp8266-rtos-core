#include "Logger.h"
#include "Properties.h"

[[maybe_unused]] void fromJson(cJSON *json, WifiProperties &props) {
    cJSON *item = json->child;
    while (item) {
        if (!strcmp(item->string, "ssid") && item->type == cJSON_String) {
            props.ssid = item->valuestring;
        } else if (!strcmp(item->string, "password") && item->type == cJSON_String) {
            props.password = item->valuestring;
        }

        item = item->next;
    }
}

[[maybe_unused]] void fromJson(cJSON *json, AppProperties &props) {
    cJSON *item = json->child;
    while (item) {
        if (!strcmp(item->string, "api") && item->type == cJSON_String) {
            props.api = item->valuestring;
        } else if (!strcmp(item->string, "type") && item->type == cJSON_String) {
            props.type = item->valuestring;
        }
        item = item->next;
    }
}

[[maybe_unused]] std::string toJson(const WifiProperties &wifiProperties) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "ssid", wifiProperties.ssid.c_str());
    cJSON_AddStringToObject(root, "password", wifiProperties.password.c_str());
    char *jsonString = cJSON_Print(root);
    cJSON_Delete(root);
    return jsonString;
}

void fromJson(cJSON *json, MqttProperties &props) {
    cJSON *item = json->child;
    while (item) {
        if (!strcmp(item->string, "uri") && item->type == cJSON_String) {
            props.uri = item->valuestring;
        } else if (!strcmp(item->string, "username") && item->type == cJSON_String) {
            props.username = item->valuestring;
        } else if (!strcmp(item->string, "password") && item->type == cJSON_String) {
            props.password = item->valuestring;
        } else if (!strcmp(item->string, "clientId") && item->type == cJSON_String) {
            props.clientId = item->valuestring;
        } else if (!strcmp(item->string, "homeId") && item->type == cJSON_String) {
            props.homeId = item->valuestring;
        } else if (!strcmp(item->string, "uri") && item->type == cJSON_String) {
            props.uri = item->valuestring;
        }
        item = item->next;
    }
}

[[maybe_unused]] std::string toJson(const MqttProperties &props) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "uri", props.uri.c_str());
    cJSON_AddStringToObject(root, "username", props.username.c_str());
    cJSON_AddStringToObject(root, "password", props.password.c_str());
    cJSON_AddStringToObject(root, "clientId", props.clientId.c_str());
    cJSON_AddStringToObject(root, "homeId", props.homeId.c_str());
    char *jsonString = cJSON_Print(root);
    cJSON_Delete(root);
    return jsonString;
}

[[maybe_unused]] std::string toJson(const AppProperties &props) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "api", props.api.c_str());
    cJSON_AddStringToObject(root, "type", props.type.c_str());
    char *jsonString = cJSON_Print(root);
    cJSON_Delete(root);
    return jsonString;
}

bool PropertiesLoader::load(const std::string& name, const std::string& filePath) {
    std::string cfg;
    char buf[64];
    if (auto *file = fopen(filePath.data(), "r"); file) {
        esp_logi(props, "read props file: %s", filePath.data());
        while (fgets(buf, 64, file) != nullptr) {
            cfg.append(buf);
        }
        fclose(file);
        if (cfg.empty()) {
            return false;
        }
        //printf("File name: %s, content: %s", filePath.data(), cfg.c_str());
        cJSON *json = cJSON_Parse(cfg.c_str());
        if (json == nullptr) {
            esp_loge(props, "Invalid json");
            return false;
        }
        if (auto it = _readers.find(name);it != _readers.end()) {
            esp_logi(props, "read props: %s", name.c_str());
            auto props = it->second(json);
            esp_logi(props, "apply to consumers: %d",_consumers.size());
            for (
                auto consumer: _consumers) {
                consumer->applyProperties(*props);
            }

        }
        esp_logi(props, "done load: %s", name.c_str());
        cJSON_Delete(json);
        return true;
    } else {
        esp_logi(props, "can't read props: %s at %s", name.c_str(), filePath.data());
    }
    return false;
}