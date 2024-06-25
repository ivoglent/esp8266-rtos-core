#pragma once

#include "../../Core.h"
#include <esp_event.h>
#include <freertos/event_groups.h>
#include <esp_netif.h>
#include "../../Logger.h"
#include "../../Event.h"
#include "esp_console.h"
#include <driver/uart.h>
#include "driver/gpio.h"
#include "../../Utils.h"
#include <sstream>
#include <esp_vfs_dev.h>
#include <linenoise/linenoise.h>

#define UART_NUM UART_NUM_0

class ConsoleService
        : public TService<ConsoleService, Service_Sys_Console, CORE> {
private:
    static std::vector<esp_console_cmd_t> commands;

    static void registerCommands();

public:

    explicit ConsoleService(Registry &registry);

    void setup() override;

    void consoleTask();

    static void registerCommand(const char *name, esp_console_cmd_func_t func);

    static void registerCommand(const char *name, esp_console_cmd_func_t func, const char *hint);

    static void registerCommand(const char *name, esp_console_cmd_func_t func, const char *hint, const char *desc);

};

static void register_console_command(const char *name, esp_console_cmd_func_t func) {
    ConsoleService::registerCommand(name, func);
}

static void register_console_command(const char *name, esp_console_cmd_func_t func, const char *hint) {
    ConsoleService::registerCommand(name, func, hint);
}

static void
register_console_command(const char *name, esp_console_cmd_func_t func, const char *hint, const char *help) {
    ConsoleService::registerCommand(name, func, hint, help);
}