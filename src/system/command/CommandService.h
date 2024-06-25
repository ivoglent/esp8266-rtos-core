#pragma once

#include "../../Core.h"
#include "../../Event.h"
#include <esp_console.h>
#include "../console/Console.h"

class CommandService
        : public TService<CommandService, Service_Sys_Cmd, CORE>{
public:
    explicit CommandService(Registry &registry);

    void setup() override;

    static int restart(int argc, char **argv);

    static int setEnv(int argc, char **argv);

    static int freeMem(int argc, char **argv);


    static int infoCmd(int argc, char **argv);

    static int versionCmd(int argc, char **argv);

    static int setStateCmd(int argc, char **argv);

    static int openConfigPortal(int argc, char **argv);

    ~CommandService() override = default;
};