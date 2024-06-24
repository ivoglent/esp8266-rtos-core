#include "Console.h"
#define PATTERN_CHR_NUM    (1)         /*!< Set the number of consecutive and identical characters received by receiver which defines a UART pattern*/

#define BUF_SIZE (4096)
#define UART_CHUNK_SIZE 512
#define RD_BUF_SIZE (UART_CHUNK_SIZE)

std::vector<esp_console_cmd_t> ConsoleService::commands = {};

ConsoleService::ConsoleService(Registry &registry) : TService(registry) {
}

static void consoleTaskHandle(void *p) {
    auto self = static_cast<ConsoleService*>(p);
    while(true) {
        self->consoleTask();
        vTaskDelay(64 / portTICK_PERIOD_MS);
    }
}


void ConsoleService::setup() {

    /* Disable buffering on stdin */
    setvbuf(stdin, NULL, _IONBF, 0);

    /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
    esp_vfs_dev_uart_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
    /* Move the caret to the beginning of the next line on '\n' */
    esp_vfs_dev_uart_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);

    /* Configure UART. Note that REF_TICK is used so that the baud rate remains
     * correct while APB frequency is changing in light sleep mode.
     */
    uart_config_t uart_config = {
            .baud_rate = CONFIG_ESP_CONSOLE_UART_BAUDRATE,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
    };
    ESP_ERROR_CHECK( uart_param_config((uart_port_t)CONFIG_ESP_CONSOLE_UART_NUM, &uart_config) );

    /* Install UART driver for interrupt-driven reads and writes */
    ESP_ERROR_CHECK( uart_driver_install((uart_port_t)CONFIG_ESP_CONSOLE_UART_NUM,
                                         256, 0, 0, NULL, 0) );

    /* Tell VFS to use UART driver */
    esp_vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);

    /* Initialize the console */
    esp_console_config_t console_config = {
            .max_cmdline_length = 256,
            .max_cmdline_args = 8,
#if CONFIG_LOG_COLORS
            .hint_color = atoi(LOG_COLOR_CYAN)
#endif
    };
    ESP_ERROR_CHECK( esp_console_init(&console_config) );

    /* Configure linenoise line completion library */
    /* Enable multiline editing. If not set, long commands will scroll within
     * single line.
     */
    linenoiseSetMultiLine(1);

    /* Tell linenoise where to get command completions and hints */
    linenoiseSetCompletionCallback(&esp_console_get_completion);
    linenoiseSetHintsCallback((linenoiseHintsCallback*) &esp_console_get_hint);

    /* Set command history size */
    linenoiseHistorySetMaxLen(10);

    registerCommands();

    xTaskCreate(consoleTaskHandle, "consoleTask", 4096, this, 10, nullptr);
    esp_logi(uart, "console started");
}

void ConsoleService::registerCommands() {
    esp_console_register_help_command();
    for (const auto &cmd: commands) {
        esp_logi(uart, "Register external command: %s", cmd.command);
        ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
    }

}

void ConsoleService::registerCommand(const char *name, esp_console_cmd_func_t func) {
    registerCommand(name, func, nullptr, nullptr);
}

void ConsoleService::registerCommand(const char *name, esp_console_cmd_func_t func, const char *hint) {
    registerCommand(name, func, hint, nullptr);
}

void ConsoleService::registerCommand(const char *name, esp_console_cmd_func_t func, const char *hint, const char *desc) {
    const esp_console_cmd_t cmd = {
            .command = name,
            .help = desc,
            .hint = hint,
            .func = func,
    };
    commands.push_back(cmd);
}

void ConsoleService::consoleTask() {
    const char* prompt = LOG_COLOR_I "cmd > " LOG_RESET_COLOR;
    while(true) {
        /* Get a line using linenoise.
         * The line is returned when ENTER is pressed.
         */
        char* line = linenoise(prompt);
        if (line == NULL) { /* Ignore empty lines */
            continue;
        }
        /* Add the command to the history */
        linenoiseHistoryAdd(line);

        /* Try to run the command */
        int ret;
        esp_err_t err = esp_console_run(line, &ret);
        if (err == ESP_ERR_NOT_FOUND) {
            printf("Unrecognized command\n");
        } else if (err == ESP_ERR_INVALID_ARG) {
            // command was empty
        } else if (err == ESP_OK && ret != ESP_OK) {
            printf("Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(err));
        } else if (err != ESP_OK) {
            printf("Internal error: %s\n", esp_err_to_name(err));
        }
        /* linenoise allocates line buffer on the heap, so need to free it */
        linenoiseFree(line);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}


