#include <stdio.h>
#include <rom/gpio.h>
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_spi_flash.h"
#include "Core.h"
#include "Utils.h"
#include "system/mqtt/MqttService.h"
#include "system/wifi/WifiService.h"
#ifndef CONFIG_IS_1MB_FLASH
#include "system/ota/OtaService.h"
#endif
#include "system/console/Console.h"
#include "system/config/ConfigurationService.h"
#include "system/command/CommandService.h"
#include "build_info.h"
#include "UserEvent.h"
#include "driver/gpio.h"

class EspIdf8266Tuya
        : public Application<EspIdf8266Tuya>,
          public TEventSubscriber<EspIdf8266Tuya, SystemEventChanged> {
public:
    void userSetup() override {
        esp_logi(app, "Starting user setup");
        getDefaultEventBus().subscribe(this);
        getRegistryInstance().create<WifiService>();
        getRegistryInstance().create<ConsoleService>();
        getRegistryInstance().create<ConfigurationService>(APP_VERSION);
        getRegistryInstance().create<CommandService>();
        auto& mqtt = getRegistryInstance().create<MqttService>();
#ifndef CONFIG_IS_1MB_FLASH
        getRegistryInstance().create<OtaService>();
        mqtt.registerEventPublisher<OtaVersion>("/ota/device/inform", MQTT_PUB_RELATIVE_SUFFIX);
        mqtt.registerEventConsumer<OtaEvent>("/ota/device/upgrade", MQTT_SUB_RELATIVE_SUBFIX);
#endif
        esp_logi(app, "Done user setup!");
    }

    void onEvent(const SystemEventChanged &event) {
        //esp_logi(app, "Got event bus msg!");
        if (event.status == SystemStatus::READY) {
#ifndef CONFIG_IS_1MB_FLASH
            ReportVersionEvent versionEvent{};
            strcpy(versionEvent.version, APP_VERSION);
            getDefaultEventBus().post(versionEvent);
#endif
        }
    }

    ~EspIdf8266Tuya() override {
        destroy();
    }
};


EspIdf8266Tuya* app;

#define LED_PIN (gpio_num_t)2
void blinkTask(void* p) {
    gpio_pad_select_gpio(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    while (1) {
        gpio_set_level(LED_PIN, 1);
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        gpio_set_level(LED_PIN, 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(nullptr);
}


extern "C" void app_main() {
    esp_log_level_set("*", ESP_LOG_VERBOSE);
    uint32_t free = esp_get_free_heap_size();
    uint64_t start  = esp_timer_get_time() / 1000;
    esp_logi(app, "Before Free: %u", free);
    esp_logi(app, "Starting application....");
    app = new EspIdf8266Tuya();
    app->setup();
    uint64_t end  = esp_timer_get_time() / 1000;
    esp_logi(app, "Finished setup in %d ms",(int) (end - start));

    //Info
    char mac[18];
    get_wifi_mac_address(mac);
    esp_logi(app, "Mac address: %s", mac);
    esp_chip_info_t info;
    esp_chip_info(&info);
    esp_logi(app, "Chip: %s", info.model == 0 ? "CHIP_ESP8266": "CHIP_ESP32");
    esp_logi(app, "Chip revision: %u", info.revision);
    esp_logi(app, "Chip cores: %u", info.cores);

    free = esp_get_free_heap_size();
    esp_logi(app, "After Free: %u", free);

    xTaskCreate(&blinkTask, "test", 1024, NULL, 5, NULL);
}
