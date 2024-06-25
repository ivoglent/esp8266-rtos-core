#pragma once

#include <cstring>
#include <string>
#include <memory>
#include <functional>
#include <map>
#include <set>
#include <cJSON.h>
#include "./system/SystemService.h"
#include "./system/SystemProperties.h"
#include <optional>
#include <vector>
#include <string_view>
#include <cstring>
#include "Logger.h"



struct Properties {
    typedef Properties* Ptr;

    [[nodiscard]] virtual uint16_t getPropId() const = 0;

    virtual ~Properties() = default;
};

template<uint8_t id, uint8_t sysId = System::CORE>
struct TProperties : Properties {
public:
    enum {
        ID = (static_cast<uint16_t>(id) << 8) | static_cast<uint16_t>(sysId)
    };

    [[nodiscard]] uint16_t getPropId() const override {
        return (static_cast<uint16_t>(id) << 8) | static_cast<uint16_t>(sysId);
    }
};

//struct WifiProperties;
struct WifiProperties : TProperties<Props_Sys_Wifi, System::CORE> {
    std::string ssid;
    std::string password;
    std::string toString() {
        return ssid + "/" + password;
    }

    void parseFromJson(cJSON* json)  {
        //fromJson(json, *this);
    }
};
struct AppProperties : TProperties<Props_App_Config, System::USER> {
    std::string api;
    std::string type;
};
struct MqttProperties : TProperties<Props_Sys_Mqtt, System::CORE> {
    std::string uri;
    std::string username;
    std::string password;
    std::string clientId;
    std::string homeId;

};


[[maybe_unused]] void fromJson(cJSON *json, WifiProperties &props);
[[maybe_unused]] std::string toJson(const WifiProperties &props);
[[maybe_unused]] void fromJson(cJSON *json, AppProperties &props);
[[maybe_unused]] void fromJson(cJSON *json, MqttProperties &props);
[[maybe_unused]] std::string toJson(const MqttProperties &props);
[[maybe_unused]] std::string toJson(const AppProperties &props);


class PropertiesConsumer {
public:
    virtual void applyProperties(const Properties &props) = 0;
};

template<typename T, typename Props, typename... Rest>
struct PropertyApplier {
    static void apply(T* instance, const Properties& props) {
        if (props.getPropId() == Props::ID) {
            instance->apply(static_cast<const Props&>(props));
        } else {
            PropertyApplier<T, Rest...>::apply(instance, props);
        }
    }
};

// Specialization to end recursion
template<typename T, typename Props>
struct PropertyApplier<T, Props> {
    static void apply(T* instance, const Properties& props) {
        //esp_logd(PRO, "Applying to consumer: %s props: %s",  typeid(T).name(), typeid(Props).name());
        if (props.getPropId() == Props::ID) {
            instance->apply(static_cast<const Props&>(props));
        } else {
            //esp_logw(PRO, "Mismatch prop consumer: %s != %s", typeid(props).name(), typeid(Props).name());
        }
    }
};

template<typename T, typename... Props>
class TPropertiesConsumer : public PropertiesConsumer {
public:
    void applyProperties(const Properties &props) override {
        esp_logd(PRO, "Apply props: %d", props.getPropId());
        PropertyApplier<T, Props...>::apply(static_cast<T*>(this), props);
    }
};

typedef std::function<Properties::Ptr(cJSON *props)> PropertiesReader;

template<typename Props>
Properties::Ptr defaultPropertiesReader(cJSON *json) {
    auto props = new Props();
    fromJson(json, *props);
    return props;
}

class PropertiesLoader {
    std::map<std::string, PropertiesReader> _readers;

    std::vector<PropertiesConsumer *> _consumers;
public:
    bool load(const std::string& name, const std::string& filePath);

    void addReader(const std::string& props, const PropertiesReader &callback) {
        _readers[props.data()] = callback;
    }

    template<typename T>
    void addConsumer(PropertiesConsumer *consumer) {
        //esp_logd(PRO, "Adding consumer: %s",  typeid(T).name());
        _consumers.push_back(consumer);
    }
};

