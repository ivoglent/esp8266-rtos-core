#pragma once

#include "../../Core.h"
#include "../../Timer.h"

#include <esp_event.h>
#include <mqtt_client.h>
#include <set>
#include <map>

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <unordered_map>
#include <typeinfo>

typedef std::function<void(const cJSON *json)> MqttJsonHandler;
typedef std::function<void(const cJSON *json, void* param)> MqttJsonHandlerParam;

enum mqtt_sub_type_t {
    MQTT_SUB_RELATIVE,
    MQTT_SUB_BROADCAST,
    MQTT_SUB_RELATIVE_SUBFIX,
};

enum mqtt_pub_type_t {
    MQTT_PUB_RELATIVE_PREFIX,
    MQTT_PUB_RELATIVE_SYS_PREFIX,
    MQTT_PUB_RELATIVE_SUFFIX,
    MQTT_PUB_ABSOLUTE,
};

struct mqtt_sub_info {
    mqtt_sub_type_t type;
    MqttJsonHandler handler;
    MqttJsonHandlerParam handlerParam;
    void* param;
};

class MqttService
        : public TService<MqttService, Service_Sys_Mqtt, CORE>,
          public TPropertiesConsumer<MqttService, MqttProperties>,
          public TEventSubscriber<MqttService, SystemEventChanged> {

    esp_mqtt_client_handle_t _client{nullptr};

    std::string _prefix;
    std::string _sysprefix;
    std::string _broadcast;
    std::map<std::string, mqtt_sub_info> _handlers;
    MqttProperties _mqttProperties;

    EspTimer _reconTimer;

    bool _subscribed = false;

    void _subscribe(const std::string& topic, const mqtt_sub_info& info);
    std::string _generateFullTopic(const std::string& topic, const mqtt_sub_info& info);

    enum State {
        S_Wifi_Disconnected,
        S_Mqtt_Connected,
        S_Mqtt_Disconnected,
        S_Mqtt_WaitConnection,
    };
    State _state{S_Wifi_Disconnected};
private:
    static void eventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
        auto *self = static_cast<MqttService *>(arg);
        self->eventHandler(event_base, event_id, event_data);
    }

    static bool compareTopics(std::string topic, std::string origin);

    void eventHandler(esp_event_base_t event_base, int32_t event_id, void *event_data);

    void eventHandlerConnect(esp_event_base_t event_base, int32_t event_id, void *event_data);

    void eventHandlerDisconnect(esp_event_base_t event_base, int32_t event_id, void *event_data);

    void eventHandlerData(esp_event_base_t event_base, int32_t event_id, void *event_data);

    static void eventHandlerError(esp_event_base_t event_base, int32_t event_id, void *event_data);

    void connect();

public:
    MqttService() = delete;
    MqttService(const MqttService&) = delete;
    explicit MqttService(Registry &registry);

    ~MqttService() override;

    void onEvent(const SystemEventChanged &msg);

    void apply(const MqttProperties &props);

    void setup() override;

    template<typename T>
    void registerEventConsumer(std::string topic, mqtt_sub_type_t type) {
        registerEventConsumer<T>(topic, type, nullptr);
    }

    template<typename T>
    void registerEventConsumer(std::string topic, mqtt_sub_type_t type, void* param) {
        if (_handlers.find(topic) == _handlers.end()) {
            auto info = mqtt_sub_info{
                    .type = type,
                    .param = param
            };
            if (param) {
                info.handlerParam = [this](const cJSON *json, void* param) {
                    T msg;
                    fromJson(json, msg);
                    msg.param = param;
                    getBus().post(msg);
                };
            } else {
                info.handler = [this](const cJSON *json) {
                    T msg;
                    fromJson(json, msg);
                    getBus().post(msg);
                };
            }
            _handlers.emplace(
                    topic,
                    info
            );
            if (_subscribed) {
                _subscribe(topic, info);
            }
        }
    }

    template<typename T>
    void registerEventPublisher(std::string topic) {
        registerEventPublisher<T>(topic, MQTT_PUB_RELATIVE_PREFIX);
    }

    template<typename T>
    void registerEventPublisher(const std::string& topic, const mqtt_pub_type_t &pathType) {
        getBus().subscribe(static_cast<SubscriberCallback<T>>([this, topic, pathType](const T &msg) {
            if (xEventGroupWaitBits(Registry::SYSTEM_STATE_BITs, SYS_MQTT_READY, pdFALSE, pdFALSE, 0)) {
                esp_logi(mqtt, "Received message of topic: %s", topic.data());

                auto json = cJSON_CreateObject();
                toJson(json, msg);
                auto str = cJSON_PrintUnformatted(json);
                cJSON_Delete(json);
                std::string fullTopic;
                switch (pathType) {
                    case MQTT_PUB_RELATIVE_PREFIX:
                        fullTopic = _prefix + topic.data();
                        break;

                    case MQTT_PUB_RELATIVE_SYS_PREFIX:
                        fullTopic = _sysprefix + topic.data();
                        break;

                    case MQTT_PUB_RELATIVE_SUFFIX:
                        fullTopic = topic.data() + _prefix;
                        break;
                    case MQTT_PUB_ABSOLUTE:
                        fullTopic = topic.data();
                        break;

                }
                if (!str) {
                    const char *error_ptr = cJSON_GetErrorPtr();
                    if (error_ptr != nullptr) {
                        esp_loge(mqtt, "Error before: %s", error_ptr);
                    }
                    esp_loge(mqtt, "Message is empty");
                    cJSON_free(str);
                    return;
                }
                if (!fullTopic.empty()) {
                    esp_mqtt_client_publish(_client, fullTopic.c_str(), str, (int) strlen(str), 0, false);
                    esp_logi(mqtt, "Publish to topic: %s message: %s", fullTopic.data(), str);
                }
                cJSON_free(str);

            } else {
                esp_logi(mqtt, "client not ready");
            }
        }));
    }
    void destroyConnection();
};
