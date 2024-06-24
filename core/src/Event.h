#pragma once

#include <cstdint>
#include <string_view>
#include <optional>
#include <cJSON.h>
#include <cstring>
#include "system/SystemService.h"
#include "EventBus.h"

enum SystemEventId {
    EVT_STATUS_CHANGED,
    EVT_TIMER,
    EVT_OTA_NEW_VERSION,
    EVT_OTA_REPORT_VERSION,
    EVT_APP_REPORT_VERSION,
    EVT_CONFIG_READY,
    EVT_MISSING_MQTT,
    EVT_OPEN_CONFIG,
    EVT_SHUTDOWN

};

enum UserEventId {
    EVT_SWITCH_STATE,
    EVT_SWITCH_SET,
};

enum class SystemStatus {
    READY,
    WIFI_CONNECTED,
    WIFI_DISCONNECTED,
    MQTT_CONNECTED,
    MQTT_DISCONNECTED
};
struct SystemEventChanged : TEvent<EVT_STATUS_CHANGED, CORE> {
    SystemStatus status;

    explicit SystemEventChanged(SystemStatus s) : TEvent<EVT_STATUS_CHANGED, CORE>(), status(s) {}
};

struct SystemShutdown: TEvent<EVT_SHUTDOWN, CORE> {

};

struct SystemConfigReady: TEvent<EVT_CONFIG_READY, CORE> {

};
struct SystemMissingMqtt: TEvent<EVT_MISSING_MQTT, CORE> {

};

struct SystemOpenConfig: TEvent<EVT_OPEN_CONFIG, CORE> {

};

template<uint16_t timerId>
struct TimerEvent : TEvent<EVT_TIMER, CORE, timerId> {
    enum {
        TimerId = timerId
    };
};


struct OtaEventDataExt {
    char type[10];
};

struct OtaEventData {
    char url[256];
    char version[36];
    unsigned long size;
    OtaEventDataExt extData;
};

struct OtaEvent : TEvent<EVT_OTA_NEW_VERSION, CORE> {
    OtaEventData data;
};

struct OtaVersion: TEvent<EVT_OTA_REPORT_VERSION, CORE> {
    char version[32];
    char module[30];
};

struct MqttServerConfig {
    char username[36];
    char password[36];
    char clientId[36];
    char uri[36];
    char homeUuid[36];
};

struct ServerResponse {
    bool success;
    MqttServerConfig data;
};



struct ReportVersionEvent : TEvent<EVT_APP_REPORT_VERSION, CORE> {
    char version[20];
};

struct SwitchStateEvent : TEvent<EVT_SWITCH_STATE, USER> {
    char uuid[6];
    bool state;
};

struct SetStateEvent : TEvent<EVT_SWITCH_SET, USER> {
    char uuid[6];
    bool state;
};

inline void toJson(cJSON *json, const OtaVersion &event) {
    cJSON_AddStringToObject(json, "version", event.version);
    cJSON_AddStringToObject(json, "module", event.module);
}

inline void fromJson(const cJSON *json, OtaEvent &event) {
    cJSON *item = json->child;
    while (item) {
        if (!strcmp(item->string, "data") && item->type == cJSON_Object) {
            cJSON *data = cJSON_GetObjectItem(json, "data");
            cJSON *dataItem = data->child;
            OtaEventData eventData{};
            while (dataItem) {
                if (!strcmp(dataItem->string, "url") && dataItem->type == cJSON_String) {
                    memcpy(eventData.url, dataItem->valuestring, strlen(dataItem->valuestring));
                    eventData.url[strlen(dataItem->valuestring) + 1] = '\0';
                } else if (!strcmp(dataItem->string, "version") && dataItem->type == cJSON_String) {
                    memcpy(eventData.version, dataItem->valuestring, strlen(dataItem->valuestring));
                } else if (!strcmp(dataItem->string, "size") && dataItem->type == cJSON_Number) {
                    eventData.size = dataItem->valueint;
                } else if (!strcmp(dataItem->string, "extData") && dataItem->type == cJSON_Object) {
                    OtaEventDataExt eventDataExt{};
                    cJSON *itemExtData = dataItem->child;
                    while (itemExtData) {
                        if (!strcmp(itemExtData->string, "type") && itemExtData->type == cJSON_String) {
                            const char *type = itemExtData->valuestring;
                            memcpy(eventDataExt.type, type, strlen(type));
                        }
                        itemExtData = itemExtData->next;
                    }
                    eventData.extData = eventDataExt;
                }
                dataItem = dataItem->next;
            }
            event.data = eventData;
        }
        item = item->next;
    }
}


inline void fromJson(const cJSON *json, ServerResponse &event) {
    cJSON *item = json->child;
    while (item) {
        if (!strcmp(item->string, "success")) {
            event.success = cJSON_IsTrue(item);
        }
        if (!strcmp(item->string, "data") && item->type == cJSON_Object) {
            cJSON *data = cJSON_GetObjectItem(json, "data");
            cJSON *dataItem = data->child;
            MqttServerConfig config{};
            while (dataItem) {
                if (!strcmp(dataItem->string, "username") && dataItem->type == cJSON_String) {
                    memcpy(config.username, dataItem->valuestring, strlen(dataItem->valuestring));
                } else if (!strcmp(dataItem->string, "password") && dataItem->type == cJSON_String) {
                    memcpy(config.password, dataItem->valuestring, strlen(dataItem->valuestring));
                } else if (!strcmp(dataItem->string, "clientId") && dataItem->type == cJSON_String) {
                    memcpy(config.clientId, dataItem->valuestring, strlen(dataItem->valuestring));
                } else if (!strcmp(dataItem->string, "uri") && dataItem->type == cJSON_String) {
                    memcpy(config.uri, dataItem->valuestring, strlen(dataItem->valuestring));
                } else if (!strcmp(dataItem->string, "homeUuid") && dataItem->type == cJSON_String) {
                    memcpy(config.homeUuid, dataItem->valuestring, strlen(dataItem->valuestring));
                }
                dataItem = dataItem->next;
            }
            event.data = config;
        }
        item = item->next;
    }
}

struct RequestConfig {

};