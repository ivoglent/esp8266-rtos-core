#pragma once

#include <utility>
#include <vector>

#include "Properties.h"
#include "EventBus.h"
#include "Event.h"

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <esp8266/eagle_soc.h>

namespace std {
    template<typename T, typename... Args>
    unique_ptr<T> make_unique(Args&&... args) {
        return unique_ptr<T>(new T(std::forward<Args>(args)...));
    }
}
typedef uint16_t ServiceId;
typedef uint8_t ServiceSubId;
typedef uint8_t SystemId;

class Registry;

#define SYS_SETUP_DONE   BIT0
#define SYS_WIFI_READY   BIT1
#define SYS_MQTT_READY   BIT2
#define SYS_CONF_READY   BIT3

class Service {
public:
    typedef std::shared_ptr<Service> Ptr;
    [[nodiscard]] virtual ServiceId getServiceId() const = 0;

    virtual Registry &getRegistry() = 0;

    virtual void setup() {}

    virtual void subscribe() {}

    virtual void loop() {}

    virtual void destroy() {}

    virtual ~Service() = default;
};

typedef std::vector<Service*> ServiceArray;

class Registry {
    ServiceArray _services;
    PropertiesLoader _propsLoader;
private:
    Service *doGetService(ServiceId serviceId) {
        //esp_logd(reg, "Getting service: %d", serviceId);
        for (auto &service: getServices()) {
            if (service->getServiceId() == serviceId) {
                return service;
            }
        }

        return nullptr;
    }

public:
    Registry() = default;
    Registry(const Registry &) = delete;
    Registry(Registry &&) = delete;
    static EventGroupHandle_t SYSTEM_STATE_BITs;
    template<typename C, typename... T>
    C &create(T &&... all) {
        // Manually allocate memory for new object of type C, passing arguments
        //esp_logd(reg, "Creating service: %s",  typeid(C).name());
        C *service = new C(*this, std::forward<T>(all)...);
        _services.push_back(service);  // Use std::unique_ptr for ownership
        //esp_logd(reg, "Current service = %d", _services.size());
        if constexpr (std::is_base_of<EventSubscriber, C>::value) {
            getEventBus().subscribe(service);
        }
        return *service;
    }


    template<typename C>
    C *getService() {
        //esp_logd(reg, "Querying service...:%d, total service:%d", C::ID, _services.size());
        return static_cast<C *>(doGetService(C::ID));
    }

    ServiceArray &getServices() {
        return _services;
    }

    PropertiesLoader &getPropsLoader() {
        return _propsLoader;
    }

    static DefaultEventBus &getEventBus() {
        return getDefaultEventBus();
    }


};

template<typename T, ServiceSubId Id, SystemId systemId = USER>
class TService : public Service, public std::enable_shared_from_this<T> {
    Registry &_registry;
    static T *instance;
public:
    explicit TService(Registry &registry) : _registry(registry) {}

    enum {
        ID = Id | (((uint16_t) systemId) << 8)
    };

    [[nodiscard]] ServiceId getServiceId() const override {
        return Id | (((uint16_t) systemId) << 8);
    }

    Registry &getRegistry() override {
        return _registry;
    }

    EspEventBus &getBus() {
        return getDefaultEventBus();
    }

    void subscribe() override {
        if constexpr (std::is_base_of<EventSubscriber, T>::value) {
            esp_logd(tmpl, "Subscribe service: %d", ID);
            getBus().subscribe(reinterpret_cast<EventSubscriber *>(this));
        } else {
            esp_logd(tmpl, "Not subscribe, not extended from EventSubscriber: %d", ID);
        }
    }

    void setup() override {

    }
};

Registry& getRegistryInstance();

inline static void system_restart() {
    getDefaultEventBus().post(SystemShutdown{});
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    taskENTER_CRITICAL();
    esp_restart();
    taskEXIT_CRITICAL();
}