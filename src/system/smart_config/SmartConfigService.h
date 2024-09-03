#pragma once
#include "../../Core.h"
#include "../console/Console.h"
#include <stdio.h>
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "esp_smartconfig.h"
#include "smartconfig_ack.h"
#include "../config/ConfigurationService.h"

class SmartConfigService :public TService<SmartConfigService, Service_Sys_Smartconfig, CORE> {
private:
public:
    explicit SmartConfigService(Registry &registry);
    ~SmartConfigService() override;
    void setup() override;
};
