#pragma once

#include <cstdint>
#include <type_traits>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <esp_log.h>

#include <vector>
#include <functional>
#include <cstring>
#include <memory>
#include <list>
#include <esp_event.h>
#include "Logger.h"

#define DEF_MSG_ID(id, sysId, params) ((uint32_t)id | (((uint32_t)sysId & 0xFF) << 8) | (((uint32_t)params & 0xFF) << 16))

namespace std {
    template< bool B, class T = void >
    using enable_if_t = typename enable_if<B,T>::type;
}

typedef uint32_t MessageId;

struct Event {
};

template<uint8_t eventId, uint8_t sysId, uint8_t bits = 0>
struct TEvent : Event {
    enum {
        ID = DEF_MSG_ID(eventId, sysId, bits)
    };
};

struct IEvent {
    [[nodiscard]] virtual MessageId id() const = 0;

    virtual ~IEvent() = default;
};

template<uint8_t eventId, uint8_t sysId, uint8_t bits = 0>
struct CEvent : IEvent {
    enum {
        ID = DEF_MSG_ID(eventId, sysId, bits)
    };

    [[nodiscard]] MessageId id() const override {
        return ID;
    }
};


class EventSubscriber {
public:
    typedef EventSubscriber* Ptr;

    virtual bool onEventHandle(MessageId id, const void *msg) {
        //esp_logd(bus, "Handler default!");
        return true;
    }

    virtual ~EventSubscriber() = default;
};

template<typename T, typename ...Msgs>
class TEventSubscriber : public EventSubscriber {
private:
    template<typename Msg>
    bool onEventHandle(MessageId id, const void *msg) {
        if (Msg::ID == id) {
            static_cast<T *>(this)->onEvent(*(reinterpret_cast<const Msg *>(msg)));
            //esp_logd(bus, "HANDLE MSG SUCCESS");
            return true;
        }
        //esp_logd(bus, "Handled message failed. Msg::ID (%d) != id (%d)", Msg::ID, id);
        return false;
    }

public:
    bool onEventHandle(MessageId id, const void *msg) override {
        return (onEventHandle<Msgs>(id, msg) || ...);
    }
};
template <typename T>
using SubscriberCallback = std::function<void(const T &msg)>;

template<typename T>
class TFuncEventSubscriber : public TEventSubscriber<TFuncEventSubscriber<T>, T> {
    SubscriberCallback<T> _callback;
public:
    explicit TFuncEventSubscriber(const SubscriberCallback<T> &callback)
            : _callback(callback) {}

    void onEvent(const T &msg) {
        _callback(msg);
    }
};

ESP_EVENT_DECLARE_BASE(CORE_EVENT);

struct BusOptions {
    bool useSystemQueue{false};
    int32_t queueCapacity{32};
    uint32_t stackSize = configMINIMAL_STACK_SIZE;
    size_t priority{tskIDLE_PRIORITY};
    std::string name;
};


class EventBus {
    std::list<EventSubscriber*> _subscribers;

protected:
    void handleMessage(MessageId id, const void *msg) {
        bool handled = false;
        for (const auto &sub: _subscribers) {
            handled |= sub->onEventHandle(id, msg);
            //esp_logd(bus, "Handled message by Subscriber");
        }

        if (!handled) {
            esp_logw(bus, "%s, unhandled event: 0x%04" PRIxLEAST32, pcTaskGetName(nullptr), id);
        } else {
            //esp_logd(bus, "Handled message success by %d service", _subscribers.size());
        }
    }

public:
    template<typename T>
    void subscribe(const SubscriberCallback<T> &callback) {
        _subscribers.push_back(new TFuncEventSubscriber<T>(callback));
    }

    void subscribe(EventSubscriber* subscriber) {
        //esp_logd(bus, "Subscribed service");
        _subscribers.push_back(subscriber);
    }

    void unsubscribe(const EventSubscriber::Ptr &subscriber) {
        _subscribers.remove(subscriber);
    }

    ~EventBus() {
        _subscribers.clear();
    }
};


ESP_EVENT_DECLARE_BASE(CORE_EVENT);
ESP_EVENT_DECLARE_BASE(CORE_NT_EVENT);

class EspEventBus : public EventBus {
    std::string _task;
    esp_event_loop_handle_t _eventLoop{};
private:
    static void eventLoop(void *handler_arg, esp_event_base_t base, int32_t id, void *event_data) {
        auto self = static_cast<EspEventBus *>( handler_arg);
        //esp_logd(bus, "Event loop has message!");
        if (base == CORE_NT_EVENT) {
            auto ptr = *(IEvent **) event_data;
            //esp_logd(bus, "Handling CORE_NT_EVENT event");
            self->handleMessage(id, ptr);
            delete ptr;
        } else {
            //esp_logd(bus, "Handling CORE_EVENT event");
            self->handleMessage(id, event_data);
        }
    }

public:
    explicit EspEventBus(const BusOptions &options) {
        //esp_logd(bus, "create esp-event-bus: " LOG_COLOR_I "%s" LOG_RESET_COLOR ", size: %" PRIi32 "",
        //         options.name.c_str(), options.queueCapacity);
        if (options.useSystemQueue) {
            _task = "sys_evt";
            esp_event_loop_create_default();
            ESP_ERROR_CHECK(esp_event_handler_register(CORE_EVENT, ESP_EVENT_ANY_ID, eventLoop, this));
            ESP_ERROR_CHECK(esp_event_handler_register(CORE_NT_EVENT, ESP_EVENT_ANY_ID, eventLoop, this));
        } else {
            _task = options.name;
            esp_event_loop_args_t loop_args = {
                    .queue_size = options.queueCapacity,
                    .task_name = options.name.c_str(),
                    .task_priority = options.priority,
                    .task_stack_size = options.stackSize,
                    .task_core_id = 0
            };

            ESP_ERROR_CHECK(esp_event_loop_create(&loop_args, &_eventLoop));
            ESP_ERROR_CHECK(esp_event_handler_register_with(_eventLoop, CORE_EVENT, ESP_EVENT_ANY_ID, eventLoop, this));
            ESP_ERROR_CHECK(
                    esp_event_handler_register_with(_eventLoop, CORE_NT_EVENT, ESP_EVENT_ANY_ID, eventLoop, this));
        }
    }

    template<typename T, std::enable_if_t<std::is_trivially_copyable<T>::value, bool> = true>
    bool post(const T &msg) {
        static_assert((std::is_base_of<Event, T>::value), "Msg is not derived from trivial message");
        //esp_logd(bus, "Posting CORE_EVENT event. ID=%d", T::ID);
        auto ret =  esp_event_post(CORE_EVENT, T::ID,  const_cast<void*>(static_cast<const void*>(&msg)), sizeof(T), portMAX_DELAY);
        if (ret != ESP_OK) {
            esp_loge(bus, "Post event bus failed: %s", esp_err_to_name(ret));
            return false;
        }
        return true;
    }

    template<typename T, std::enable_if_t<!std::is_trivially_copyable<T>::value, bool> = true>
    bool post(const T &msg) {
        static_assert((std::is_base_of<IEvent, T>::value), "Msg is not derived from non-trivial message");
        auto ptr = new T(msg);
        //esp_logd(bus, "Posting CORE_NT_EVENT event");
        return ESP_OK == esp_event_post(CORE_NT_EVENT, T::ID, &ptr, sizeof(T *), portMAX_DELAY);
    }

    template<typename T>
    bool send(const T &msg) {
        static_assert((std::is_base_of<Event, T>::value || std::is_base_of<IEvent, T>::value),
                      "Msg is not derived from message");
        if (_task != pcTaskGetName(nullptr)) {
            esp_loge(bus, "can't send - incorrect task: 0x%04x, task: '%s'", T::ID, pcTaskGetName(nullptr));
            return false;
        }

        handleMessage(T::ID, &msg);
        return true;
    }
};

typedef EspEventBus DefaultEventBus;
DefaultEventBus &getDefaultEventBus();

