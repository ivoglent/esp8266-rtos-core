
#include "EventBus.h"

ESP_EVENT_DEFINE_BASE(CORE_EVENT);
ESP_EVENT_DEFINE_BASE(CORE_NT_EVENT);
static DefaultEventBus* eventBusPtr = nullptr; // Pointer to the DefaultEventBus instance
static BusOptions options{};

DefaultEventBus &getDefaultEventBus() {
    if (eventBusPtr == nullptr) {
        // Lazy initialization
        options.useSystemQueue=true;
        eventBusPtr = new DefaultEventBus(options);
    }
    return *eventBusPtr;
}
