#pragma once

#include <cstdint>
#include "Core.h"
#include <string>
#include <sstream>
#include <esp_interface.h>
#include <esp_wifi.h>

template<typename Base, typename T>
inline bool instanceof(const T *) {
    return std::is_base_of<Base, T>::value;
}

inline char* concat(const char* s1, const char* s2) {
    // Allocate memory for the concatenated string
    // +1 for the null terminator at the end
    char* result = static_cast<char *>(malloc(strlen(s1) + strlen(s2) + 1));
    if (result == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // Copy the first string into result
    strcpy(result, s1);

    // Concatenate the second string to result
    strcat(result, s2);

    return result;
}

inline void string_append(std::string &str, const char *src, const char *sep) {
    str += std::string(sep) + src;
}

inline void get_wifi_mac_address(char *mac) {
    uint8_t mac_addr[6];
    esp_wifi_get_mac(static_cast<wifi_interface_t>(ESP_IF_WIFI_STA), mac_addr);
    sprintf(mac, "%02X:%02X:%02X:%02X:%02X:%02X",
            mac_addr[0], mac_addr[1], mac_addr[2],
            mac_addr[3], mac_addr[4], mac_addr[5]);
}

inline void get_client_id_from_mac_address(char *client_id) {
    uint8_t mac_addr[6];
    esp_wifi_get_mac(static_cast<wifi_interface_t>(ESP_IF_WIFI_STA), mac_addr);
    sprintf(client_id, "%02X-%02X-%02X-%02X-%02X-%02X",
            mac_addr[0], mac_addr[1], mac_addr[2],
            mac_addr[3], mac_addr[4], mac_addr[5]);
}

inline int get_number_from_nvs(const std::string &name) {
    nvs_handle_t nvs_handle;
    auto ret = nvs_open("storage", NVS_READONLY, &nvs_handle);
    if (ret == ESP_OK) {
        int32_t var;
        ret = nvs_get_i32(nvs_handle, name.c_str(), &var);
        if (ret == ESP_OK) {
            nvs_close(nvs_handle);
            return var;
        }
    }
    return 0;
}

inline bool set_number_to_nvs(const std::string &name, int32_t value) {
    nvs_handle_t nvs_handle;
    auto ret = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (ret == ESP_OK) {
        ret = nvs_set_i32(nvs_handle, name.c_str(), value);
        if (ret == ESP_OK) {
            ret = nvs_commit(nvs_handle);
            if (ret != ESP_OK) {
                return true;
            }
        }
    }
    return false;
}


