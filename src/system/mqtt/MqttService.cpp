#include <ping/ping_sock.h>
#include <lwip/netdb.h>
#include "MqttService.h"

uint32_t _mqttRetryCount = 0;
uint32_t _errorCount = 0;
bool MqttService::compareTopics(std::string topic, std::string origin) {
    enum class State {
        None,
        Slash,
    } state = State::None;
    auto it = topic.begin(), oit = origin.begin();

    while (it != topic.end() && oit != origin.end()) {
        switch (state) {
            case State::Slash:
                if (*it == '#') {
                    if (*oit == '/') return false;
                    if (++oit == origin.end()) ++it;
                    continue;
                } else if (*it == '+') {
                    if (*oit != '/') {
                        ++oit;
                        continue;
                    } else ++it;
                } else {
                    if (*it == '/') ++oit;
                    state = State::None;
                }
                break;
            case State::None:
            default:
                if (*it == '/') {
                    if (*oit != '/') return false;
                    state = State::Slash;
                } else if (*it == '+' || *it == '#') {
                    return false;
                }
                if (*it != *oit) return false;
        }

        ++it, ++oit;
    }

    if (it != topic.end() && *it == '+') {
        ++it;
    }

    return it == topic.end() && oit == origin.end();
}


MqttService::MqttService(Registry &registry) : TService(registry) {
    registry.getPropsLoader().addConsumer<MqttService>(this);
}


void MqttService::eventHandlerConnect(esp_event_base_t event_base, int32_t event_id, void *event_data) {
    esp_logi(mqtt, "MQTT connected successfully!");
    for (const auto &it: _handlers) {
        _subscribe(it.first, it.second);
    }
    _subscribed = true;
    getDefaultEventBus().post(SystemEventChanged{SystemStatus::MQTT_CONNECTED});
}

void MqttService::eventHandlerDisconnect(esp_event_base_t event_base, int32_t event_id, void *event_data) {
    esp_logi(mqtt, "MQTT disconnected!");
    getBus().post(SystemEventChanged{SystemStatus::MQTT_DISCONNECTED});
}

static std::unordered_map<std::string, std::string> buffers;
static std::string currentTopic;
void MqttService::eventHandlerData(esp_event_base_t event_base, int32_t event_id, void *event_data) {
    auto event = static_cast<esp_mqtt_event_handle_t>(event_data);
    esp_logi(mqtt, "Got message from topic: %s. Topic len=%d, data len=%d total len=%d", event->topic, event->topic_len, event->data_len, event->total_data_len);
    if (event->topic_len > 0) {
        char rawTopic[event->topic_len + 1];
        strncpy(rawTopic, event->topic, event->topic_len);
        currentTopic = std::string(rawTopic);
        esp_logd(mqtt, "Full topic: %s", currentTopic.data());
    }
    std::string segment;
    if (buffers.find(currentTopic) != buffers.end()) {
        esp_logi(mqtt, "Found in buffer, get mqtt message parts back");
        segment = buffers.at(currentTopic);
    }
    segment.append(event->data, event->data_len);
    // process segment data
    if (segment.size() < event->total_data_len) {
        esp_logd(mqtt, "Waiting more parts, current len=%d, total len=%d", segment.size(), event->total_data_len);
        buffers.emplace(currentTopic.c_str(), segment);
        return;
    }
    std::string message(segment.data(), event->total_data_len);
    esp_logd(mqtt, "Message content: %s", message.data());
    int handled = 0;
    for (const auto &it: _handlers) {
        std::string fullPath = _generateFullTopic(it.first, it.second);
        if (compareTopics(fullPath, currentTopic)) {
            esp_logi(mqtt, "Handling message %s from topic %s", message.data(), fullPath.c_str());
            auto json = cJSON_Parse(message.data());
            if (json) {
                if (it.second.param) {
                    it.second.handlerParam(json, it.second.param);
                } else {
                    it.second.handler(json);
                }

                cJSON_Delete(json);
            }
            handled++;
        }
    }
    segment.clear();
    buffers.erase(currentTopic);
    if (handled == 0) {
        esp_logw(mqtt, "unhandled: %s (%s)", currentTopic.data(), message.data());
    }
}

void MqttService::eventHandlerError(esp_event_base_t event_base, int32_t event_id, void *event_data) {
    auto event = static_cast<esp_mqtt_event_handle_t>(event_data);
    esp_logd(mqtt, "Last errno string (%s). Error count: %d", strerror(event->error_handle->error_type), _errorCount);
    _errorCount++;
    if (_errorCount >= 60) {
        system_restart();
    }
}


void MqttService::eventHandler(esp_event_base_t base, int32_t event_id, void *event_data) {
    switch (static_cast<esp_mqtt_event_id_t>(event_id)) {
        case MQTT_EVENT_CONNECTED:
            xEventGroupSetBits(Registry::SYSTEM_STATE_BITs, SYS_MQTT_READY);
            eventHandlerConnect(base, event_id, event_data);
            _mqttRetryCount = 0;
            break;
        case MQTT_EVENT_DISCONNECTED:
            xEventGroupClearBits(Registry::SYSTEM_STATE_BITs, SYS_MQTT_READY);
            eventHandlerDisconnect(base, event_id, event_data);
            _mqttRetryCount++;
            break;
        case MQTT_EVENT_DATA:
            eventHandlerData(base, event_id, event_data);
            break;
        case MQTT_EVENT_ERROR:
            eventHandlerError(base, event_id, event_data);
            break;
        default:
            break;
    }
}

void MqttService::connect() {
    if (!_mqttProperties.uri.empty()) {
        destroyConnection();
        esp_mqtt_client_config_t mqtt_cfg = {
                .uri = _mqttProperties.uri.data(),
                .client_id = _mqttProperties.clientId.c_str(),
                .username = _mqttProperties.username.c_str(),
                .password = _mqttProperties.password.c_str(),
                .keepalive = 30,
                .disable_auto_reconnect = true,
                .task_stack = 8912,
                .buffer_size = 1024,
                .reconnect_timeout_ms = 1000,
        };

        _client = esp_mqtt_client_init(&mqtt_cfg);
        esp_mqtt_client_register_event(_client, MQTT_EVENT_ANY, eventHandler, this);
        _prefix = "/user/" + _mqttProperties.clientId;
        _sysprefix = "/sys/" + _mqttProperties.clientId;
        _broadcast = "/broadcast/" + _mqttProperties.homeId;

        esp_mqtt_client_start(_client);
    } else {
        esp_logw(mqtt, "MQTT configuration is empty!");
        getBus().post(SystemOpenConfig{});
    }

}

void MqttService::destroyConnection() {
    if (_client) {
        esp_mqtt_client_destroy(_client);
        _client = nullptr;
    }
}

void MqttService::onEvent(const SystemEventChanged &msg) {
    //esp_logi(mqtt, "On SystemEventChanged event...");
    switch (msg.status) {
        case SystemStatus::WIFI_CONNECTED:
            _state = S_Mqtt_WaitConnection;
            esp_logi(mqtt, "Wifi connected. Starting MQTT...");
            connect();
            break;
        case SystemStatus::WIFI_DISCONNECTED:
            _state = S_Wifi_Disconnected;
            break;
        case SystemStatus::MQTT_CONNECTED:{
            _state = S_Mqtt_Connected;
            SystemEventChanged eventChanged(SystemStatus::READY);
            getDefaultEventBus().post(eventChanged);
            break;
        }
        case SystemStatus::MQTT_DISCONNECTED:
            if (_state == S_Wifi_Disconnected) {
                esp_logi(mqtt, "Disconnected, waiting for wifi ready");
                return;
            }
            if (xEventGroupWaitBits(Registry::SYSTEM_STATE_BITs, SYS_WIFI_READY, pdFALSE, pdFALSE, portMAX_DELAY)) {
                _reconTimer.attach(100, false, [this]() {
                    connect();
                });
            }
            _state = S_Mqtt_Disconnected;
            break;
        default:
            break;
    }
}

void MqttService::apply(const MqttProperties &props) {
    esp_logi(mqtt, "Read MQTT props and apply");
    _mqttProperties = props;
}

void MqttService::setup() {
    esp_logi(mqtt, "Setup MQTT");
}

MqttService::~MqttService() {
}

void MqttService::_subscribe(const std::string &topic, const mqtt_sub_info &info) {
    std::string fullPath = _generateFullTopic(topic, info);
    int msg_id = esp_mqtt_client_subscribe(_client, fullPath.c_str(), 0);
    esp_logi(mqtt, "sub successful, topic: %s, msg_id: %d", fullPath.c_str(), msg_id);
}

std::string MqttService::_generateFullTopic(const std::string &topic, const mqtt_sub_info &info) {
    std::string fullPath;
    switch (info.type) {
        case MQTT_SUB_RELATIVE:
            fullPath = _prefix;
            fullPath.append(topic);
            break;
        case MQTT_SUB_BROADCAST:
            fullPath = _broadcast;
            fullPath.append(topic);
        case MQTT_SUB_RELATIVE_SUBFIX:
            fullPath = topic + _prefix;
        default:
            break;
    }
    return fullPath;
}
