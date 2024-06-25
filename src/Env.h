#pragma once

#include <cstdint>
#include <nvs_flash.h>
#include "Logger.h"
#include <esp_system.h>
#include <string>

class Env {
public:
    static std::string getConfigFileName(const std::string& name) {
        return "/spiffs/config-" + name + ".json";
    }
};

