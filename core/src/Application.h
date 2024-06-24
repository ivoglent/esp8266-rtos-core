#pragma once

#include <esp_event.h>
#include <nvs_flash.h>
#include <esp_spiffs.h>

#include "Registry.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <algorithm>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <dirent.h>
#include "Env.h"


template<typename T>
class Application : public std::enable_shared_from_this<T> {
private:
    inline void list_files_in_spiffs() {
        esp_logi(app, "Listing files in SPIFFS");
        // Assuming "/spiffs" is the base path
        const char *base_path = "/spiffs";
        DIR *dir = opendir(base_path);

        if (dir != nullptr) {
            struct dirent *ent;

            // Read directory entries
            while ((ent = readdir(dir)) != nullptr) {
                // Print file name
                esp_logi(app, "Found file: %s", ent->d_name);

                // If you also want to print file sizes, you need to use stat() or similar
                // Example: struct stat st; stat(ent->d_name, &st); ESP_LOGI(TAG, "Size: %ld", st.st_size);
            }
            // Close the directory stream
            closedir(dir);
        } else {
            esp_loge(app, "Failed to open directory");
        }
    }
protected:
    virtual void userSetup() {}

public:

    virtual void setup() {
        esp_logi(app, "Setup nvs flash...");
        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_ERROR_CHECK(nvs_flash_erase());
            ret = nvs_flash_init();
        }
        ESP_ERROR_CHECK(ret);
        esp_logi(app, "Setup event loop...");
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        getDefaultEventBus();
        esp_vfs_spiffs_conf_t conf = {
                .base_path = "/spiffs",
                .partition_label = "spiffs",
                .max_files = 10,
                .format_if_mount_failed = true,
        };
        ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));
        //list_files_in_spiffs();
        esp_logi(app, "User setup...");
        getRegistryInstance().getPropsLoader().addReader("app", defaultPropertiesReader<AppProperties>);
        getRegistryInstance().getPropsLoader().addReader("mqtt", defaultPropertiesReader<MqttProperties>);
        getRegistryInstance().getPropsLoader().addReader("wifi", defaultPropertiesReader<WifiProperties>);

        userSetup();
        if (!getRegistryInstance().getServices().empty()) {
            /*std::sort(getRegistry().getServices().begin(), getRegistry().getServices().end(), [](Service* f, Service* s) {
                return f->getServiceId() < s->getServiceId();
            });*/
        }
        for (auto service: getRegistryInstance().getServices()) {
            //getDefaultEventBus().subscribe(reinterpret_cast<EventSubscriber *>(service));
            service->subscribe();
            service->setup();
            //esp_logd(app, "Setup: %d", service->getServiceId());
        }
        //Load config
        for(auto const &name: {"wifi", "mqtt", "app"}) {
            auto config_file = Env::getConfigFileName(name);
            esp_logi(app, "load %s config", config_file.c_str());
            getRegistryInstance().getPropsLoader().load(name, config_file);
        }
        esp_logi(app, "Setup done, firing events!");
        SystemConfigReady event{};
        getDefaultEventBus().post(event);
        xEventGroupSetBits(Registry::SYSTEM_STATE_BITs, SYS_SETUP_DONE);
    }

    void destroy() {
        //ESP_ERROR_CHECK(esp_vfs_spiffs_unregister("spiffs"));
        //ESP_ERROR_CHECK(esp_event_loop_delete_default());
    }
};