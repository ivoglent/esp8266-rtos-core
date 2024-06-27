#pragma once
#ifndef CONFIG_IS_1MB_FLASH
#include "../../Core.h"
#include "../../system/SystemService.h"
#include "../../Event.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "../../system/SystemService.h"

class OtaService : public TService<OtaService, Service_Sys_Ota, CORE>,
                   public TEventSubscriber<OtaService, ReportVersionEvent> {
private:
    bool _reported = false;

public:

    explicit OtaService(Registry &reg);

    ~OtaService() override;

    void setup() override;

    void onEvent(const OtaEvent &event);

    void onEvent(const ReportVersionEvent &event);

};


#endif