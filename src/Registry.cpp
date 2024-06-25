#include "Registry.h"
static std::unique_ptr<Registry> instance = nullptr;
EventGroupHandle_t Registry::SYSTEM_STATE_BITs = xEventGroupCreate();
Registry& getRegistryInstance() {
    if (instance == nullptr) {
        esp_logi(reg, "Registry instance empty, creating new");
        instance = std::make_unique<Registry>();
    }
    return *instance;
};