#pragma once

#include <cstdint>
#include <functional>
#include <memory>

#include "EventBus.h"
#include "Event.h"

class Timer {
public:
    typedef std::shared_ptr<Timer> Ptr;

    virtual void attach(uint32_t milliseconds, bool repeat, const std::function<void()> &callback) = 0;

    template<uint16_t tid>
    void fire(uint32_t milliseconds, bool repeat) {
        attach(milliseconds, repeat, [this]() {
            TimerEvent<tid> event;
            getDefaultEventBus().post(event);
        });

    }

    virtual ~Timer() = default;
};

extern "C" {
#include "esp_timer.h"
}

class EspTimer : public Timer {
    std::string _name;
    std::function<void()> _callback;

    esp_timer_handle_t _timer{};
private:
    static void onCallback(void *arg) {
        auto *timer = static_cast<EspTimer *>(arg);
        timer->doCallback();
    }

    void doCallback() {
        _callback();
    }

public:
    EspTimer();

    explicit EspTimer(std::string name);

    void attach(uint32_t milliseconds, bool repeat, const std::function<void()> &callback) override;

    void detach();

    ~EspTimer() override {
        detach();
    }
};

#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>

class SoftwareTimer : public Timer {
    TimerHandle_t _timer{};

    std::function<void()> _callback;
private:
    static void onCallback(TimerHandle_t timer) {
        auto self = static_cast<SoftwareTimer *>( pvTimerGetTimerID(timer));
        self->doCallback();
    }

    void doCallback() {
        _callback();
    }

public:
    SoftwareTimer() = default;

    void attach(uint32_t milliseconds, bool repeat, const std::function<void()> &callback) override;

    ~SoftwareTimer() override;
};
