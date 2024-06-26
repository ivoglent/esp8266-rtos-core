#include "CommandService.h"



static float getStorageUsage() {
    size_t spiTotal, spiUsed;
    auto ret = esp_spiffs_info("spiffs", &spiTotal, &spiUsed);
    if (ret == ESP_OK) {
        esp_logi(cmd, "Storage total: %d", spiTotal);
        esp_logi(cmd, "Storage used: %d", spiUsed);
        float usage = ((float) spiUsed / spiTotal) * 100;
        esp_logi(cmd, "Used percentage: %f", usage);
        return usage;
    } else {
        esp_loge(cmd, "Storage error: %s", esp_err_to_name(ret));
    }
    return 0;
}


CommandService::CommandService(Registry &registry) : TService(registry) {
    register_console_command("restart", restart, "", "Restart device");
    register_console_command("mem", freeMem, "", "Check free heap");
    register_console_command("info", infoCmd, "", "Get some information from chip");
    register_console_command("version", versionCmd);
    register_console_command("config", openConfigPortal);
}

int CommandService::restart(int argc, char **argv) {
    esp_logi(uart, "restarting...");
    system_restart();
    return 0;
}

int CommandService::freeMem(int argc, char **argv) {
    esp_logi(cmd, "free heap:%" PRIu32 "\n", esp_get_free_heap_size());
    return 0;
}



int CommandService::infoCmd(int argc, char **argv) {
    char mac[18];
    get_wifi_mac_address(mac);
    esp_logi(cmd, "Mac address: %s", mac);

    uint32_t free = esp_get_free_heap_size();
    esp_logi(cmd, "Free: %u ", free);


    esp_chip_info_t cinfo;
    esp_chip_info(&cinfo);
    std::string chipName;
    switch (cinfo.model) {
        case CHIP_ESP8266:
            chipName = "ESP8266";
            break;
        case CHIP_ESP32:
            chipName = "ESP32";
            break;
        default:
            chipName = "UNKNOWN";
            break;
    }
    esp_logi(cmd, "Chip model: %s", chipName.data());
    esp_logi(cmd, "Chip revision: %u", cinfo.revision);
    esp_logi(cmd, "Chip cores: %u", cinfo.cores);

    wifi_ap_record_t ap_info;
    esp_wifi_sta_get_ap_info(&ap_info);
    int rssi = ap_info.rssi;
    esp_logi(cmd, "Wi-Fi RSSI: %d dBm", rssi);

    tcpip_adapter_ip_info_t ipInfo;
    char str[256];

    tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);
    sprintf(str, "%d.%d.%d.%d",
            IP2STR(&ipInfo.ip));
    esp_logi(cmd, "IP address: %s", str);
    getStorageUsage();

    return 0;
}

void CommandService::setup() {
}

int CommandService::versionCmd(int argc, char **argv) {
 /*   auto info = esp_app_get_description();
    esp_logi(cmd, "Master version : %s", info->version);*/
    return 0;
}


int CommandService::openConfigPortal(int argc, char **argv) {
    SystemOpenConfig evt{};
    getDefaultEventBus().post(evt);
    return 0;
}
